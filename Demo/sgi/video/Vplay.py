#! /usr/local/python

# Play CMIF movie files


# Usage:
#
# Vplay [-l] [-m maginfy] [file] ...


# Options:
#
# -l         : loop, playing the movie over and over again
# -m magnify : magnify the image by the given factor
# file ...   : file(s) to play; default film.video


# User interface:
#
# Place the windo where you want it.  The size is determined by the
# movie file and the -m option.
#
# Press ESC or select the window manager Quit or Close window option
# to close a window; if more files are given the window for the next
# file now pops up.


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


# Main program -- mostly command line parsing

def main():
	global magnify, looping
	opts, args = getopt.getopt(sys.argv[1:], 'lm:')
	for opt, arg in opts:
		if opt == '-l':
			looping = 1
		elif opt == '-m':
			magnify = string.atoi(arg)
	if not args:
		args = ['film.video']
	for filename in args:
		process(filename)


# Process one file

def process(filename):
	vin = VFile.VinFile().init(filename)
	print 'File:    ', filename
	print 'Version: ', vin.version
	print 'Size:    ', vin.width, 'x', vin.height
	print 'Pack:    ', vin.packfactor, '; chrom:', vin.chrompack
	print 'Bits:    ', vin.c0bits, vin.c1bits, vin.c2bits
	print 'Format:  ', vin.format
	print 'Offset:  ', vin.offset
	vin.magnify = magnify
	
	gl.foreground()
	gl.prefsize(vin.width * magnify, vin.height * magnify)
	win = gl.winopen('* ' + filename)
	vin.initcolormap()

	gl.qdevice(ESCKEY)
	gl.qdevice(WINSHUT)
	gl.qdevice(WINQUIT)

	t0 = time.millitimer()
	running = 1
	data = None
	t = 0
	n = 0
	while 1:
		if running:
			try:
				t, data, chromdata = vin.getnextframe()
				n = n+1
			except EOFError:
				t1 = time.millitimer()
				gl.wintitle(filename)
				print 'Recorded:', n,
				print 'frames in', t*0.001, 'sec.',
				if t:
					print '-- average',
					print int(n*10000.0/t)*0.1,
					print 'frames/sec',
				print
				t = t1-t0
				print 'Played:', n,
				print 'frames in', t*0.001, 'sec.',
				if t:
					print '-- average',
					print int(n*10000.0/t)*0.1,
					print 'frames/sec',
				print
				if looping and n > 0:
					vin.rewind()
					vin.magnify = magnify
					continue
				else:
					running = 0
		if running:
			dt = t + t0 - time.millitimer()
			if dt > 0:
				time.millisleep(dt)
			vin.showframe(data, chromdata)
		if not running or gl.qtest():
			dev, val = gl.qread()
			if dev in (ESCKEY, WINSHUT, WINQUIT):
				gl.winclose(win)
				break
			if dev == REDRAW:
				gl.reshapeviewport()
				if data:
					vin.showframe(data, chromdata)


# Don't forget to call the main program

main()
