#! /usr/bin/env python

# Play synchronous video and audio.
# Highly experimental!

import sys
import getopt
import string
import os

import VFile
import aifc

import gl, GL, DEVICE
import al, AL


def usage():
	sys.stderr.write( \
		'usage: aplay [-o offset] [-q qsize] videofile audiofile\n')
	sys.exit(2)

def main():
	offset = 0
	qsize = 0 # This defaults to 1/10 second of sound
	videofile = 'film.video'
	audiofile = 'film.aiff'

	try:
		opts, args = getopt.getopt(sys.argv[1:], 'o:q:')
	except getopt.error, msg:
		sys.stderr.write(msg + '\n')
		usage()

	try:
		for o, a in opts:
			if o == '-o':
				offset = string.atoi(a)
			if o == '-q':
				qsize = string.atoi(a)
	except string.atoi_error:
		sys.stderr.write(o + ' arg must be integer\n')
		usage()

	if len(args) > 2:
		usage()

	if args: videofile = args[0]
	if args[1:]: audiofile = args[1]

	if not os.path.exists(videofile) and \
		os.path.exists(videofile + '.video'):
		if not args[1:] and os.path.exists(videofile + '.aiff'):
			audiofile = videofile + '.aiff'
		videofile = videofile + '.video'

	print 'Opening video input file..'
	vin = VFile.VinFile(videofile)

	print 'Opening audio input file..'
	ain = aifc.open(audiofile, 'r')
	print 'rate    :', ain.getframerate()
	print 'channels:', ain.getnchannels()
	print 'frames  :', ain.getnframes()
	print 'width   :', ain.getsampwidth()
	print 'kbytes  :', \
		  ain.getnframes() * ain.getnchannels() * ain.getsampwidth()

	print 'Opening audio output port..'
	c = al.newconfig()
	c.setchannels(ain.getnchannels())
	c.setwidth(ain.getsampwidth())
	nullsample = '\0' * ain.getsampwidth()
	samples_per_second = ain.getnchannels() * ain.getframerate()
	if qsize <= 0: qsize = samples_per_second / 10
	qsize = max(qsize, 512)
	c.setqueuesize(qsize)
	saveparams = [AL.OUTPUT_RATE, 0]
	al.getparams(AL.DEFAULT_DEVICE, saveparams)
	newparams = [AL.OUTPUT_RATE, ain.getframerate()]
	al.setparams(AL.DEFAULT_DEVICE, newparams)
	aport = al.openport(audiofile, 'w', c)

	print 'Opening video output window..'
	gl.foreground()
	gl.prefsize(vin.width, vin.height)
	wid = gl.winopen(videofile + ' + ' + audiofile)
	gl.clear()
	vin.initcolormap()

	print 'Playing..'
	gl.qdevice(DEVICE.ESCKEY)
	gl.qdevice(DEVICE.LEFTARROWKEY)
	gl.qdevice(DEVICE.RIGHTARROWKEY)
##	gl.qdevice(DEVICE.UPARROWKEY)
##	gl.qdevice(DEVICE.DOWNARROWKEY)
	gl.qdevice(DEVICE.SPACEKEY)

	while 1:
		samples_written = 0
		samples_read = 0
		lastt = 0
		pause = 0
		while 1:
			if gl.qtest():
				dev, val = gl.qread()
				if val == 1:
					if dev == DEVICE.ESCKEY:
						sys.exit(0)
					elif dev == DEVICE.LEFTARROWKEY:
						offset = offset - 100
						print 'offset =', offset
					elif dev == DEVICE.RIGHTARROWKEY:
						offset = offset + 100
						print 'offset =', offset
					elif dev == DEVICE.SPACEKEY:
						pause = (not pause)

			if pause:
				continue

			try:
				t, data, cdata = vin.getnextframe()
			except EOFError:
				break
			t = int(t)
			dt = t - lastt
			lastt = t
			target = samples_per_second * t / 1000
			n = target - samples_written + qsize - offset
			if n > 0:
				# This call will block until the time is right:
				try:
					samples = ain.readframes(n)
				except EOFError:
					samples = ''
				k = len(samples) / len(nullsample)
				samples_read = samples_read + k
				if k < n:
					samples = samples + (n-k) * nullsample
				aport.writesamps(samples)
				samples_written = samples_written + n
			vin.showframe(data, cdata)

		while 1:
			try:
				samples = ain.readframes(qsize)
			except EOFError:
				break
			if not samples:
				break
			aport.writesamps(samples)
			k = len(samples) / len(nullsample)
			samples_read = samples_read + k
			samples_written = samples_written + k

		print samples_read, 'samples ==',
		print samples_read * 1.0 / samples_per_second, 'sec.'
		print lastt, 'milliseconds'

		print 'Restarting..'
		ain.close()
		ain = aifc.open(audiofile, 'r')
		vin.rewind()


main()
