#! /usr/local/python

# Play CMIF movie files


# Usage:
#
# Vplay [-L] [-M maginfy] [-m msec] [-q] [-r msec] [-s factor] [file] ...


# Options:
#
# -L         : loop, playing the movie over and over again
# -M magnify : magnify the image by the given factor
# -m n       : drop frames closer than n msec (default 0)
# -q         : quiet, no informative messages
# -r n       : regenerate input time base n msec apart
# -s speed   : speed change factor after other processing (default 1.0)
# file ...   : file(s) to play; default film.video


# User interface:
#
# Place the windo where you want it.  The size is determined by the
# movie file and the -m option.
#
# Press ESC or select the window manager Quit or Close window option
# to close a window; if more files are given the window for the next
# file now pops up.  Unless looping, the window closes automatically
# at the end of the movie.


import sys
sys.path.append('/ufs/guido/src/video')
import VFile
import time
import gl, GL
from DEVICE import *
import getopt
import string


# Global options

magnify = 1
looping = 0
speed = 1.0
mindelta = 0
quiet = 0
regen = None


# Main program -- mostly command line parsing

def main():
	global magnify, looping, speed, mindelta, quiet, regen
	opts, args = getopt.getopt(sys.argv[1:], 'LM:m:qr:s:')
	for opt, arg in opts:
		if opt == '-L':
			looping = 1
		elif opt == '-M':
			magnify = string.atoi(arg)
		elif opt == '-m':
			mindelta = string.atoi(arg)
		elif opt == '-q':
			quiet = 1
		elif opt == '-r':
			regen = string.atoi(arg)
		elif opt == '-s':
			speed = float(eval(arg))
	if not args:
		args = ['film.video']
	for filename in args:
		process(filename)


# Process one movie file

def process(filename):
	try:
		vin = VFile.VinFile().init(filename)
	except IOError, msg:
		sys.stderr.write(filename + ': I/O error: ' + `msg` + '\n')
		return
	except VFile.Error, msg:
		sys.stderr.write(msg + '\n')
		return
	except EOFError:
		sys.stderr.write(filename + ': EOF in video header\n')
		return

	if not quiet:
		print 'File:    ', filename
		print 'Version: ', vin.version
		print 'Size:    ', vin.width, 'x', vin.height
		print 'Pack:    ', vin.packfactor, '; chrom:', vin.chrompack
		print 'Bits:    ', vin.c0bits, vin.c1bits, vin.c2bits
		print 'Format:  ', vin.format
		print 'Offset:  ', vin.offset
	
	gl.foreground()
	gl.prefsize(vin.width * magnify, vin.height * magnify)
	win = gl.winopen(filename)
	if quiet:
		vin.quiet = 1
	vin.initcolormap()

	gl.qdevice(ESCKEY)
	gl.qdevice(WINSHUT)
	gl.qdevice(WINQUIT)

	while 1:
		stop = playonce(vin)
		if stop or not looping:
			break

	gl.wintitle('(done) ' + filename)

	while not stop:
		dev, val = gl.qread()
		if dev in (ESCKEY, WINSHUT, WINQUIT):
			stop = 1

	gl.winclose(win)


# Play a movie once; return 1 if user wants to stop, 0 if not

def playonce(vin):
	if vin.hascache:
		vin.rewind()
	vin.colormapinited = 1
	vin.magnify = magnify

	tin = 0
	told = 0
	nin = 0
	nout = 0
	nmissed = 0
	nskipped = 0

	t0 = time.millitimer()

	while 1:
		try:
			tin, size, csize = vin.getnextframeheader()
		except EOFError:
			break
		nin = nin+1
		if regen:
			tout = nin * regen
		else:
			tout = tin
		tout = int(tout / speed)
		if tout - told < mindelta:
			try:
				vin.skipnextframedata(size, csize)
			except EOFError:
				print '[short data at EOF]'
				break
			nskipped = nskipped + 1
		else:
			told = tout
			dt = tout + t0 - time.millitimer()
			if dt < 0:
				try:
					vin.skipnextframedata(size, csize)
				except EOFError:
					print '[short data at EOF]'
					break
				nmissed = nmissed + 1
			else:
				try:
					data, cdata = vin.getnextframedata \
						(size, csize)
				except EOFError:
					print '[short data at EOF]'
					break
				dt = tout + t0 - time.millitimer()
				if dt > 0:
					time.millisleep(dt)
				vin.showframe(data, cdata)
				nout = nout + 1
		if gl.qtest():
			dev, val = gl.qread()
			if dev in (ESCKEY, WINSHUT, WINQUIT):
				return 1
			if dev == REDRAW:
				gl.reshapeviewport()
				if data:
					vin.showframe(data, cdata)

	t1 = time.millitimer()

	if quiet:
		return 0

	print 'Recorded:', nin, 'frames in', tin*0.001, 'sec.',
	if tin: print '-- average', int(nin*10000.0/tin)*0.1, 'frames/sec',
	print

	if nskipped:
		print 'Skipped', nskipped, 'frames'

	tout = t1-t0
	print 'Played:', nout,
	print 'frames in', tout*0.001, 'sec.',
	if tout: print '-- average', int(nout*10000.0/tout)*0.1, 'frames/sec',
	print

	if nmissed:
		print 'Missed', nmissed, 'frames'

	return 0


# Don't forget to call the main program

main()
