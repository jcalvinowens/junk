#!/usr/bin/env python3
#
# Copyright (C) 2017 Calvin Owens <jcalvinowens@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 2 only.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.

import argparse
import itertools
import math
import os
import string
import sys
import time

import random
random = random.SystemRandom()

def BUG_ON(v):
	if v: raise AssertionError

class CWBase(object):
	DIT = 1
	DAH = 2

	TABLE = {
		'A': [DIT, DAH],
		'B': [DAH, DIT, DIT, DIT],
		'C': [DAH, DIT, DAH, DIT],
		'D': [DAH, DIT, DIT],
		'E': [DIT],
		'F': [DIT, DIT, DAH, DIT],
		'G': [DAH, DAH, DIT],
		'H': [DIT, DIT, DIT, DIT],
		'I': [DIT, DIT],
		'J': [DIT, DAH, DAH, DAH],
		'K': [DAH, DIT, DAH],
		'L': [DIT, DAH, DIT, DIT],
		'M': [DAH, DAH],
		'N': [DAH, DIT],
		'O': [DAH, DAH, DAH],
		'P': [DIT, DAH, DAH, DIT],
		'Q': [DAH, DAH, DIT, DAH],
		'R': [DIT, DAH, DIT],
		'S': [DIT, DIT, DIT],
		'T': [DAH],
		'U': [DIT, DIT, DAH],
		'V': [DIT, DIT, DIT, DAH],
		'W': [DIT, DAH, DAH],
		'X': [DAH, DIT, DIT, DAH],
		'Y': [DAH, DIT, DAH, DAH],
		'Z': [DAH, DAH, DIT, DIT],
		'0': [DAH, DAH, DAH, DAH, DAH],
		'1': [DIT, DAH, DAH, DAH, DAH],
		'2': [DIT, DIT, DAH, DAH, DAH],
		'3': [DIT, DIT, DIT, DAH, DAH],
		'4': [DIT, DIT, DIT, DIT, DAH],
		'5': [DIT, DIT, DIT, DIT, DIT],
		'6': [DAH, DIT, DIT, DIT, DIT],
		'7': [DAH, DAH, DIT, DIT, DIT],
		'8': [DAH, DAH, DAH, DIT, DIT],
		'9': [DAH, DAH, DAH, DAH, DIT],
		'.': [DIT, DAH, DIT, DAH, DIT, DAH],
		',': [DAH, DAH, DIT, DIT, DAH, DAH],
		':': [DAH, DAH, DAH, DIT, DIT, DIT],
		"'": [DIT, DAH, DAH, DAH, DAH, DIT],
		'-': [DAH, DIT, DIT, DIT, DIT, DAH],
		'/': [DAH, DIT, DIT, DAH, DIT],
		'_': [DIT, DIT, DAH, DAH, DIT, DAH],
		'=': [DAH, DIT, DIT, DIT, DAH],
		'?': [DIT, DIT, DAH, DAH, DIT, DIT],
		'|': [DAH, DIT, DAH, DAH, DIT, DAH],
		'@': [DIT, DAH, DAH, DIT, DAH, DIT],
		' ': [-DIT],
		'\\': [DIT, DAH, DIT, DIT, DAH],

		# Control codes
		'<ERR>': [DIT, DIT, DIT, DIT, DIT, DIT, DIT, DIT],
		'<START>': [DAH, DIT, DAH, DIT, DAH],
		'<INTR>': [DIT, DAH, DIT, DIT, DIT],
		'<CLOSE>': [DIT, DIT, DIT, DAH, DIT, DAH],
		'<END>': [DIT, DAH, DIT, DAH, DIT],
		'<ACK>': [DIT, DIT, DIT, DAH, DIT],
	}

class CWDecoder(CWBase):
	# Someday...
	pass

class CWEncoder(CWBase):
	"""
	Very simple abstract CW encoder, which supports both synchronous and
	bulk encoding of messages. Derived classes provide an __init__ which
	initializes SILENCE and TONE to one ticksworth of silence/tone.
	"""
	SILENCE = NotImplemented
	TONE = NotImplemented

	def __init__(self, wpm=20.0, weight=3, spaces=0, **kwargs):
		self.RATE = wpm * 50 / 60
		self.TICK = 1.0 / self.RATE
		self.WEIGHT = weight
		self.SEP = ' ' * spaces

	def __key(self, code):
		length = self.WEIGHT if abs(code) == self.DAH else 1
		ret = self.TONE if code > 0 else self.SILENCE
		return [ret] * length

	def __code(self, msg):
		ret = []
		msg = self.SEP.join(msg)

		for char in msg:
			encoding = self.TABLE[char.upper()]
			for code in encoding[:-1]:
				ret.extend(self.__key(code))
				ret.extend(self.__key(-self.DIT))

			ret.extend(self.__key(encoding[-1]))
			ret.extend(self.__key(-self.DAH))

		return ret

	def encode(self, msg):
		return bytes(itertools.chain(*self.__code(msg)))

	def writemsg(self, fd, msg, sync=False, lazy=False):
		if not sync:
			os.write(fd, self.encode(msg))
			return

		for c in msg:
			last = None
			for sample in self.__code(c):
				time.sleep(self.TICK)
				if lazy and last == sample:
					continue

				os.write(fd, sample)
				last = sample

class CWASCIIEncoder(CWEncoder):
	def __init__(self, **kwargs):
		super().__init__(**kwargs)
		self.SILENCE = kwargs.get("off", b"0")
		self.TONE = kwargs.get("on", b"1")

class CWPCMEncoder(CWEncoder):
	@staticmethod
	def tone(seconds, frequency, amplitude=1.0, samplerate=44100, bits=16,
		 signed=True, endian="little", **_):

		BUG_ON(bits % 8)
		BUG_ON(frequency < 20 or frequency * 2 > samplerate)
		BUG_ON(endian not in ["big", "little"])
		BUG_ON(amplitude <= 0.0 or amplitude > 1.0)

		maxval = (2 ** bits) - 1
		signadj = (maxval + 1) // 2 if signed else 0
		step = (2 * math.pi) / samplerate
		end = (2 * math.pi) * seconds
		x = 0.0

		ret = bytes()
		while x < end:
			raw = math.sin(x * frequency)
			raw = (raw + 1) / 2 # [-1, 1] => [0, 1]
			x += step

			val = (raw * maxval - signadj) * amplitude
			ret += int.to_bytes(int(round(val)), byteorder=endian,
					    length=bits // 8, signed=signed)

		return ret

	def __init__(self, frequency=700, **kwargs):
		super().__init__(**kwargs)

		# Round the tick to the nearest multiple of the frequency, so we
		# can cleanly concatenate pieces of PCM output.
		self.TICK = round(self.TICK * frequency) / frequency
		self.TONE = self.tone(self.TICK, frequency, **kwargs)
		self.SILENCE = bytes(b'\x00') * len(self.TONE)

def get_encoder(args):
	return {
		"pcm": CWPCMEncoder,
		"raw": CWASCIIEncoder,
	}[args.encoder](**vars(args))

def do_train(args):
	encoder = get_encoder(args)
	print("Playing %d groups of %d from [%s]" % (args.count, args.length,
	      "".join(args.letters)), file=sys.stderr)

	while True:
		words = [''.join(random.choices(args.letters, k=args.length))
			 for _ in range(args.count)]
		msg = ' '.join(words)

		os.write(sys.stdout.fileno(), encoder.encode(msg))
		input()

		print(msg, file=sys.stderr)
		input()

def do_key(args):
	encoder = get_encoder(args)
	while not args.msg:
		try:
			encoder.writemsg(sys.stdout.fileno(), input().rstrip(),
					 args.sync, args.lazy)
		except EOFError:
			return 0

	for m in args.msg:
		encoder.writemsg(sys.stdout.fileno(), m, args.sync, args.lazy)

	return 0

def add_encoder_subparsers(parser):
	subs = parser.add_subparsers(dest="encoder")
	parser.add_argument("--wpm", type=int, default=20)
	parser.add_argument("--weight", type=int, default=3)
	parser.add_argument("--spaces", type=int, default=0)

	pcmenc = subs.add_parser("pcm", help="Raw PCM audio stream")
	pcmenc.add_argument("--samplerate", type=int, default=44100)
	pcmenc.add_argument("--amplitude", type=float, default=0.25)
	pcmenc.add_argument("--frequency", type=int, default=700)
	pcmenc.add_argument("--bits", type=int, default=16)
	pcmenc.add_argument("--endian", choices=["big", "little"],
			    default="little")
	pcmenc.add_argument("msg", type=str, nargs="*")

	rawenc = subs.add_parser("raw", help="On/off character stream")
	rawenc.add_argument("--on", type=bytes, default=b"1")
	rawenc.add_argument("--off", type=bytes, default=b"0")
	rawenc.add_argument("msg", type=str, nargs="*")

def parse_arguments():
	parser = argparse.ArgumentParser()
	subs = parser.add_subparsers(dest="action")

	keycmd = subs.add_parser("key", help="Generate CW from input text")
	keycmd.add_argument("--sync", action="store_true", default=False,
			    help="Write keysamples in real time")
	keycmd.add_argument("--lazy", action="store_true", default=False,
			    help="Only write samples that change state")
	add_encoder_subparsers(keycmd)

	traincmd = subs.add_parser("train", help="Play sets of random strings")
	traincmd.add_argument("--count", type=int, default=1)
	traincmd.add_argument("--length", type=int, default=4)
	traincmd.add_argument("--letters", type=str,
			      default=string.ascii_uppercase)
	add_encoder_subparsers(traincmd)

	return parser.parse_args()

def main():
	args = parse_arguments()
	return {
		"key": do_key,
		"train": do_train,
	}[args.action](args)

if __name__ == "__main__":
	sys.exit(main())
