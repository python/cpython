#! /usr/bin/env python

import sys
import audio

import string
import getopt
import auds

debug = []

DEF_RATE = 3

def main():
	#
	gain = 100
	rate = 0
	starter = audio.write
	stopper = 0
	#
	optlist, args = getopt.getopt(sys.argv[1:], 'adg:r:')
	#
	for optname, optarg in optlist:
		if 0:
			pass
		elif optname == '-d':
			debug.append(1)
		elif optname == '-g':
			gain = string.atoi(optarg)
			if not (0 < gain < 256):
				raise optarg.error, '-g gain out of range'
		elif optname == '-r':
			rate = string.atoi(optarg)
			if not (1 <= rate <= 3):
				raise optarg.error, '-r rate out of range'
		elif optname == '-a':
			starter = audio.start_playing
			stopper = audio.wait_playing
	#
	audio.setoutgain(gain)
	audio.setrate(rate)
	#
	if not args:
		play(starter, rate, auds.loadfp(sys.stdin))
	else:
		real_stopper = 0
		for file in args:
			if real_stopper:
				real_stopper()
			play(starter, rate, auds.load(file))
			real_stopper = stopper

def play(starter, rate, data):
	magic = data[:4]
	if magic == '0008':
		mrate = 3
	elif magic == '0016':
		mrate = 2
	elif magic == '0032':
		mrate = 1
	else:
		mrate = 0
	if mrate:
		data = data[4:]
	else:
		mrate = DEF_RATE
	if not rate: rate = mrate
	audio.setrate(rate)
	starter(data)

try:
	main()
finally:
	audio.setoutgain(0)
	audio.done()
