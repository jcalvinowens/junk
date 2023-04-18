#!/usr/bin/python3

import os
import subprocess
import sys

if len(sys.argv) != 3:
	print("Usage: ./encode.py <cutlist> <out_vid>")
	print("Cutlist format: in_file start_ts end_ts name [name [name...]]")
	sys.exit(1)

VCODEC = "copy"
ACODEC = "copy"

cutlist = sys.argv[1]
out_path = sys.argv[2]
toff = 0.0
idx = 0

with open(sys.argv[1], "r") as cutlist:
	cuts = [x.rstrip().split() for x in cutlist.readlines()]

for l in cuts:
	in_file = l[0]
	start_ts = l[1]
	end_ts = l[2]
	desc = ' '.join(l[3:])

	subprocess.check_call([
		"ffmpeg", "-ss", start_ts, "-to", end_ts,
		"-i", in_file, "-c:v", VCODEC, "-c:a", ACODEC,
		f"tmp-{idx:04}.mkv"],
		stdin=subprocess.DEVNULL,
		stdout=subprocess.DEVNULL,
		stderr=subprocess.STDOUT,
	)

	out = subprocess.check_output(
		["ffprobe", f"tmp-{idx:04}.mkv"],
		stderr=subprocess.STDOUT,
	).decode('utf-8').splitlines()
	off = [x for x in out if "Duration: " in x][0].split()[1][:-1]
	hours, mins, secs = off.split(':')
	hours = int(hours)
	mins = int(mins)
	secs = float(secs)

	psecs = int(toff % 60) + 1 if toff % 60 != int(toff % 60) else 0
	print(f"({int(toff // 60):02}:{psecs:02}) {desc}")
	toff += hours * 3600 + mins * 60 + secs
	idx += 1

with open("fconcat", "w+") as f:
	for i in range(idx):
		f.write(f"file 'tmp-{i:04}.mkv'\n")

try:
	os.unlink(out_path)
except:
	pass

output = subprocess.check_output([
	"ffmpeg", "-f", "concat", "-i", "fconcat", "-safe", "0",
	"-c:v", "copy", "-c:a", "copy", out_path],
	stderr=subprocess.STDOUT,
).decode('utf-8').splitlines()

print(output[-2])
os.unlink("fconcat")
for i in range(idx):
	os.unlink(f"tmp-{i:04}.mkv")
