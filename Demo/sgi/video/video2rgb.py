#! /usr/bin/env python

# Convert CMIF movie file(s) to a sequence of rgb images


# Help function

def help():
	print 'Usage: video2rgb [options] [file] ...'
	print
	print 'Options:'
	print '-q         : quiet, no informative messages'
	print '-m         : create monochrome (greyscale) image files'
	print '-f prefix  : create image files with names "prefix0000.rgb"'
	print 'file ...   : file(s) to convert; default film.video'


# Imported modules

import sys
sys.path.append('/ufs/jack/src/av/video') # Increase chance of finding VFile
import VFile
import time
import getopt
import string
import imgfile
import imgconv


# Global options

quiet = 0
prefix = 'film'
seqno = 0
mono = 0


# Main program -- mostly command line parsing

def main():
	global quiet, prefix, mono

	# Parse command line
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'qmf:')
	except getopt.error, msg:
		sys.stdout = sys.stderr
		print 'Error:', msg, '\n'
		help()
		sys.exit(2)

	# Interpret options
	try:
		for opt, arg in opts:
			if opt == '-q': quiet = 1
			if opt == '-f': prefix = arg
			if opt == '-m': mono = 1
	except string.atoi_error:
		sys.stdout = sys.stderr
		print 'Option', opt, 'requires integer argument'
		sys.exit(2)

	# Process all files
	if not args: args = ['film.video']
	sts = 0
	for filename in args:
		sts = (process(filename) or sts)

	# Exit with proper exit status
	sys.exit(sts)


# Process one movie file

def process(filename):
	try:
		vin = VFile.VinFile(filename)
	except IOError, msg:
		sys.stderr.write(filename + ': I/O error: ' + `msg` + '\n')
		return 1
	except VFile.Error, msg:
		sys.stderr.write(msg + '\n')
		return 1
	except EOFError:
		sys.stderr.write(filename + ': EOF in video header\n')
		return 1

	if not quiet:
		vin.printinfo()
	
	width, height = int(vin.width), int(vin.height)

	try:
		if mono:
			cf = imgconv.getconverter(vin.format, 'grey')
		else:
			cf = imgconv.getconverter(vin.format, 'rgb')
	except imgconv.error:
		print 'Sorry, no converter available for type',vin.format
		return

	if mono:
		depth = 1
		bpp = 1
	else:
		depth = 3
		bpp = 4

	convert(vin, cf, width, height, depth, bpp, vin.packfactor)

def convert(vin, cf, width, height, depth, bpp, pf):
	global seqno

	if type(pf) == type(()):
		xpf, ypf = pf
	elif pf == 0:
		xpf = ypf = 1
	else:
		xpf = ypf = pf
	while 1:
		try:
			time, data, cdata = vin.getnextframe()
		except EOFError:
			return
		if cdata:
			print 'Film contains chromdata!'
			return
		data = cf(data, width/xpf, height/abs(ypf))
		if pf:
			data = applypackfactor(data, width, height, pf, bpp)
		s = `seqno`
		s = '0'*(4-len(s)) + s
		fname = prefix + s + '.rgb'
		seqno = seqno + 1
		if not quiet:
			print 'Writing',fname,'...'
		imgfile.write(fname, data, width, height, depth)
	
def applypackfactor(image, w, h, pf, bpp):
	import imageop
	if type(pf) == type(()):
		xpf, ypf = pf
	elif pf == 0:
		xpf = ypf = 1
	else:
		xpf = ypf = pf
	w1 = w/xpf
	h1 = h/abs(ypf)
	if ypf < 0:
		ypf = -ypf
		image = imageop.crop(image, bpp, w1, h1, 0, h1-1, w1-1, 0)
	return imageop.scale(image, bpp, w1, h1, w, h)
	
# Don't forget to call the main program

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
