#!/usr/bin/python3 -i
#
# wsclient.py: Example UDP client code for ws2811d.c
#
# Copyright (C) 2022 Calvin Owens <jcalvinowens@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 2 only.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.

DIGITS = {
	0: [[0,0,0,0],
	    [0,1,0,0],
	    [1,0,1,0],
	    [1,0,1,0],
	    [1,0,1,0],
	    [0,1,0,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	1: [[0,0,0,0],
	    [0,1,0,0],
	    [1,1,0,0],
	    [0,1,0,0],
	    [0,1,0,0],
	    [1,1,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	2: [[0,0,0,0],
	    [1,1,0,0],
	    [0,0,1,0],
	    [0,1,0,0],
	    [1,0,0,0],
	    [1,1,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	3: [[0,0,0,0],
	    [1,1,1,0],
	    [0,0,1,0],
	    [0,1,1,0],
	    [0,0,1,0],
	    [1,1,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	4: [[0,0,0,0],
	    [1,0,1,0],
	    [1,0,1,0],
	    [1,1,1,0],
	    [0,0,1,0],
	    [0,0,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	5: [[0,0,0,0],
	    [1,1,1,0],
	    [1,0,0,0],
	    [1,1,1,0],
	    [0,0,1,0],
	    [1,1,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	6: [[0,0,0,0],
	    [1,1,1,0],
	    [1,0,0,0],
	    [1,1,1,0],
	    [1,0,1,0],
	    [1,1,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	7: [[0,0,0,0],
	    [1,1,1,0],
	    [0,0,1,0],
	    [0,0,1,0],
	    [0,0,1,0],
	    [0,0,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	8: [[0,0,0,0],
	    [1,1,1,0],
	    [1,0,1,0],
	    [1,1,1,0],
	    [1,0,1,0],
	    [1,1,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
	9: [[0,0,0,0],
	    [1,1,1,0],
	    [1,0,1,0],
	    [1,1,1,0],
	    [0,0,1,0],
	    [0,0,1,0],
	    [0,0,0,0],
	    [0,0,0,0]],
}

import argparse
import time
import socket
import struct

p = argparse.ArgumentParser()
p.add_argument("dst", type=str, help="Server to yell at")
p.add_argument("LEDS_X", metavar="X", type=int, help="X axis LED count")
p.add_argument("LEDS_Y", metavar="Y", type=int, help="Y axis LED count (1 for linear)")
args = p.parse_args()

DST = (args.dst, 1337)
LEDS_X = args.LEDS_X
LEDS_Y = args.LEDS_Y
LED_COUNT = args.LEDS_X * args.LEDS_Y
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
STARTEPOCH = int(time.time())

def setrgb(r, g, b):
	s.sendto(struct.pack("BBB", r, g, b) * LED_COUNT, DST)

def sety(y):
	setrgb(y, y, y)

def setpattern(*masks):
	pkt = [0 for _ in range(LED_COUNT * 3)]

	for mask, r, g, b in masks:
		for i in range(LED_COUNT):
			if i >= len(mask) or not mask[i]:
				continue

			pkt[i*3+0] = r
			pkt[i*3+1] = g
			pkt[i*3+2] = b

	# Every other row needs to be inverted
	for rowstart in range(0, LED_COUNT, args.LEDS_X * 2):
		for i in range(args.LEDS_Y // 2):
			for j in range(3):
				tmp = pkt[(rowstart + i) * 3 + j]
				pkt[(rowstart + i) * 3 + j] = \
					pkt[(rowstart + args.LEDS_Y - 1 - i) * 3 + j]
				pkt[(rowstart + args.LEDS_Y - 1 - i) * 3 + j] = tmp

	bpkt = struct.pack("B" * LED_COUNT * 3, *pkt)
	s.sendto(bpkt, DST)

def digitmask2(d):
	hi = d // 10;
	lo = d % 10;
	out = []

	for hilines, lolines in zip(DIGITS[hi], DIGITS[lo]):
		out.extend(hilines)
		out.extend(lolines)

	return out

def digitmask4(d):
	p3 = d // 1000
	d %= 1000
	p2 = d // 100
	d %= 100
	p1 = d // 10
	d %= 10
	p0 = d

	out = []
	for p3lines, p2lines, p1lines, p0lines in \
	zip(DIGITS[p3], DIGITS[p2], DIGITS[p1], DIGITS[p0]):
		out.extend(p3lines)
		out.extend(p2lines)
		out.extend(p1lines)
		out.extend(p0lines)

	return out

def digitcounter2(interval=0.1, *rgbs):
	while True:
		for i in range(100):
			r = rgbs[i % len(rgbs)][0]
			g = rgbs[i % len(rgbs)][1]
			b = rgbs[i % len(rgbs)][2]
			setpattern((digitmask2(i),r,g,b))
			time.sleep(interval)

def digitcounter4(interval=0.1, *rgbs):
	while True:
		for i in range(10000):
			r = rgbs[i % len(rgbs)][0]
			g = rgbs[i % len(rgbs)][1]
			b = rgbs[i % len(rgbs)][2]
			setpattern((digitmask4(i),r,g,b))
			time.sleep(interval)

def digitcounter8(interval=0.1, *rgbs):
	while True:
		for i in range(10000000):
			r = rgbs[i % len(rgbs)][0]
			g = rgbs[i % len(rgbs)][1]
			b = rgbs[i % len(rgbs)][2]
			setpattern((
				digitmask4(i // 10000) + digitmask4(i % 10000),
				r,g,b
			))
			time.sleep(interval)

def pxcounter(interval=0.1, r=10, g=10, b=10):
	i = 0

	while True:
		m = [0 for _ in range(LED_COUNT)]
		i += 1

		d = i
		for x in range(LED_COUNT):
			m[x] = d % 2
			d = d // 2

		setpattern((m,r,g,b))
		time.sleep(interval)
