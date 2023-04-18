#!/usr/bin/env python3
#
# Copyright (C) 2018 Calvin Owens <jcalvinowens@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 2 only.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
# --
# planeyeller.py: Tool that uses dump1090-mutability and espeak to announce the
#		  planes flying overhead by decoding their ADS-B transmissions.

import argparse
import json
import logging
import math
import re
import shutil
import subprocess
import sys
import tempfile
import time

CARDINALS = [
	"north",
	"north north east",
	"north east",
	"east north east",
	"east",
	"east south east",
	"south east",
	"south south east",
	"south",
	"south south west",
	"south west",
	"west south west",
	"west",
	"west north west",
	"north west",
	"north north west",
	"north",
]

def direction(heading):
	return CARDINALS[math.floor(heading / 22.5)]

PHONETICS = {
	"A": "alfa",
	"B": "bravo",
	"C": "charlie",
	"D": "delta",
	"E": "echo",
	"F": "foxtrot",
	"G": "golf",
	"H": "hotel",
	"I": "india",
	"J": "juliet",
	"K": "kilo",
	"L": "lima",
	"M": "mike",
	"N": "november",
	"O": "oscar",
	"P": "papa",
	"Q": "quebec",
	"R": "romeo",
	"S": "sierra",
	"T": "tango",
	"U": "uniform",
	"V": "victor",
	"W": "whisky",
	"X": "x-ray",
	"Y": "yankee",
	"Z": "zulu",
	"0": "zero",
	"1": "one",
	"2": "two",
	"3": "tree",
	"4": "four",
	"5": "fife",
	"6": "six",
	"7": "seven",
	"8": "eight",
	"9": "niner",
	"/": "slash",
	".": "point",
}

def phonetic(string):
	return " ".join([PHONETICS.get(l.upper()) for l in string if l in
			 PHONETICS])

AIRLINES = {
	"AAL": "American Airlines",
	"ACA": "Air Canada",
	"ASA": "Alaska Airlines",
	"BAW": "British Airways",
	"CAL": "China Airlines",
	"CES": "China Eastern Airlines",
	"CSC": "Sichuan Airlines",
	"CMP": "Copa Airlines",
	"DAL": "Delta Airlines",
	"EJA": "Netjets",
	"EVA": "EVA Air",
	"FDX": "FedEx",
	"PAL": "Philippine Airlines",
	"SKW": "Skywest Airlines",
	"SWA": "Southwest Airlines",
	"QXE": "Horizon Air",
	"UAL": "United Airlines",
	"USC": "AirNet Express",
}

def airline_name(carrier):
	return AIRLINES.get(carrier, phonetic(carrier))

ICAO_FLIGHTNO_RE = re.compile("[A-Z]{3}[0-9]+")

def aircraft_ident(string):
	if string is None:
		return "Aircraft"

	if ICAO_FLIGHTNO_RE.match(string):
		carrier, number = string[:3], string[3:]
		name = airline_name(carrier.strip().rstrip())
		return f"{name} flight {phonetic(number)}"

	if string[0] == "N":
		return phonetic(string[1:])

	return phonetic(string)

SQUAWKS = {
	"1200": "Ve Ef Ar",
	"7500": "Hijacked",
	"7600": "NORDO",
	"7700": "Mayday",
}

def squawk(code):
	return SQUAWKS.get(str(code), phonetic(code))

p = argparse.ArgumentParser()
p.add_argument("--interval", type=int, default=2, metavar="seconds")
p.add_argument("--angle", type=int, default=40, metavar="degrees")
p.add_argument("--rssi", type=float, default=-3.0, metavar="dB")
p.add_argument("--distance", type=float, default=1.0, metavar="miles")
p.add_argument("--lat", type=float)
p.add_argument("--lon", type=float)
p.add_argument("--verbose", "-v", action="count", default=0)
p.add_argument("--announce-all", action="store_true", default=False)
ARGS = p.parse_args()

formatter = logging.Formatter(
	"%(asctime)s %(filename)s:%(lineno)04d [%(levelname)7s] %(message)s",
)

console = logging.StreamHandler(sys.stderr)
console.setFormatter(formatter)
console.setLevel(logging.DEBUG)

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO - 10 * ARGS.verbose)
logger.addHandler(console)

# Return heading/distance vector from our location to @dlat/@dlon
def get_vector(dlat, dlon, radius_of_earth):
	srclat = math.radians(ARGS.lat)
	srclon = math.radians(ARGS.lon)
	dstlat = math.radians(dlat)
	dstlon = math.radians(dlon)
	lmbda = dstlon - srclon

	# https://en.wikipedia.org/wiki/Great-circle_distance#Formulas
	rho = math.acos(math.sin(srclat) * math.sin(dstlat) +
			math.cos(srclat) * math.cos(dstlat) *
			math.cos(lmbda)
	)

	# https://en.wikipedia.org/wiki/Great-circle_navigation
	rads = math.atan2(math.sin(lmbda),
			math.cos(srclat) * math.tan(dstlat) -
			math.sin(srclat) * math.cos(lmbda)
	)

	heading = math.degrees(rads)
	distance = int(abs(rho * radius_of_earth))
	return heading, distance

def get_announcement(d, altitude, cardinal, angle, miles):
	ann = [f"{aircraft_ident(d.get('flight'))} in sight"]

	if angle:
		ann.append(f"{phonetic(str(angle))} degrees elevation")

	if cardinal:
		ann.append(f"{phonetic(str(miles))} miles {cardinal}")

	if "track" in d:
		ann.append(f"tracking {direction(d['track'])}")

	if altitude:
		ann.append(f"altitude {altitude}")

	if "vert_rate" in d:
		rate = int(d["vert_rate"]) // 100 * 100
		m = ["level", f"climbing at {rate}",  f"descending at {-rate}"]
		ann.append(m[min(max(-1, rate), 1)])

	if "speed" in d:
		ann.append(f"speed {d['speed']} knots")

	logger.info(ann)
	return ann

def process(d, announced):
	logger.debug(json.dumps(d, sort_keys=True))

	if "hex" not in d:
		logger.debug("No hex address, ignoring")
		return None

	if "altitude" in d and not str(d["altitude"]).isdigit():
		logger.debug("Ignoring non-numeric altitude '{d['altitude']}'")
		return None

	if d["hex"] in announced and time.time() - announced[d["hex"]] < 300:
		logger.debug(f"Duplicate: {d['hex']}")
		return None

	altitude = int(d.get("altitude", "0"))
	cardinal = None
	angle = None
	miles = None

	if "lat" in d and "lon" in d:
		hdg, feet = get_vector(d["lat"], d["lon"], 20903520)
		miles = round(feet / 5280.0, 1 if feet < 52800 else 0)
		cardinal = direction(hdg)

		if altitude:
			theta = math.atan2(altitude, feet)
			angle = int(round(math.degrees(theta) // 5 * 5, 0))

	if angle is not None and angle >= ARGS.angle \
	   or miles is not None and miles <= ARGS.distance \
	   or "rssi" in d and float(d["rssi"]) >= ARGS.rssi \
	   or ARGS.announce_all:
		announced[d["hex"]] = time.time()
		return get_announcement(d, altitude, cardinal, angle, miles)

def speak(child, queue):
	cmd = ["espeak", "-s210", "-p55", "-ven-us"]

	if not child or child.poll() is not None:
		child = subprocess.Popen(cmd, stdin=subprocess.PIPE,
					 stdout=subprocess.DEVNULL,
					 stderr=subprocess.DEVNULL,
					 bufsize=2**20)
		logger.info(f"New espeak, pid={child.pid}")

	string = ". Break! ".join([", ".join(x) for x in queue]) + "\n"
	child.stdin.write(string.encode("ascii"))
	child.stdin.flush()

	logger.debug(f"Enqueued {len(queue)} announcements to espeak")
	queue.clear()
	return child

def run(tmpdir, child):
	jsonfile = f"{tmpdir}/aircraft.json"
	announced = {}
	yeller = None
	queue = []

	while True:
		try:
			logger.debug(f"Sleeping for INTERVAL...")
			time.sleep(ARGS.interval)
			data = {}

			with open(jsonfile, "r") as f:
				data = json.loads(f.read())["aircraft"]

			logger.debug(f"Read {len(data)} records from JSON")

			for d in data:
				p = process(d, announced)
				if p:
					queue.append(p)

			yeller = speak(yeller, queue)

		except KeyboardInterrupt:
			logger.info("Interrupted!")
			child.kill()
			break
		except:
			logger.critical("Fatal exception")
			child.kill()
			raise

def main():
	try:
		tmpdir = tempfile.mkdtemp(prefix="planeyeller")
		outfd = sys.stderr if ARGS.verbose >= 2 else subprocess.DEVNULL
		DUMP1090CMD = [
			"dump1090-mutability",
			"--write-json", tmpdir,
			"--write-json-every", str(ARGS.interval),
			"--fix",
		]

		with subprocess.Popen(DUMP1090CMD, stdout=outfd, stderr=outfd,
				      stdin=subprocess.DEVNULL) as p:
			run(tmpdir, p)
	finally:
		shutil.rmtree(tmpdir, ignore_errors=True)

if __name__ == '__main__':
	main()
