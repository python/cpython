#! /usr/bin/env python

# Universal (non-interactive) CMIF video file copier.


# Possibilities:
#
# - Manipulate the time base:
#   = resample at a fixed rate
#   = divide the time codes by a speed factor (to make it go faster/slower)
#   = drop frames that are less than n msec apart (to accommodate slow players)
# - Convert to a different format
# - Magnify (scale) the image


# Usage function (keep this up-to-date if you change the program!)

def usage():
	print 'Usage: Vcopy [options] [infile [outfile]]'
	print
	print 'Options:'
	print
	print '-t type    : new image type (default unchanged)'
	print
	print '-M magnify : image magnification factor (default unchanged)'
	print '-w width   : output image width (default height*4/3 if -h used)'
	print '-h height  : output image height (default width*3/4 if -w used)'
	print
	print '-p pf      : new x and y packfactor (default unchanged)'
	print '-x xpf     : new x packfactor (default unchanged)'
	print '-y ypf     : new y packfactor (default unchanged)'
	print
	print '-m delta   : drop frames closer than delta msec (default 0)'
	print '-r delta   : regenerate input time base delta msec apart'
	print '-s speed   : speed change factor (default unchanged)'
	print
	print 'infile     : input file (default film.video)'
	print 'outfile    : output file (default out.video)'


import sys
sys.path.append('/ufs/guido/src/video')

import VFile
import imgconv
import imageop
import getopt
import string


# Global options

speed = 1.0
mindelta = 0
regen = None
newpf = None
newtype = None
magnify = None
newwidth = None
newheight = None


# Function to turn a string into a float

atof_error = 'atof_error' # Exception if it fails

def atof(s):
	try:
		return float(eval(s))
	except:
		raise atof_error


# Main program -- mostly command line parsing

def main():
	global speed, mindelta, regen, newpf, newtype, \
	       magnify, newwidth, newheight

	# Parse command line
	try:
		opts, args = getopt.getopt(sys.argv[1:], \
			  'M:h:m:p:r:s:t:w:x:y:')
	except getopt.error, msg:
		sys.stdout = sys.stderr
		print 'Error:', msg, '\n'
		usage()
		sys.exit(2)

	xpf = ypf = None
	
	# Interpret options
	try:
		for opt, arg in opts:
			if opt == '-M': magnify = atof(arg)
			if opt == '-h': height = string.atoi(arg)
			if opt == '-m': mindelta = string.atoi(arg)
			if opt == '-p': xpf = ypf = string.atoi(arg)
			if opt == '-r': regen = string.atoi(arg)
			if opt == '-s': speed = atof(arg)
			if opt == '-t': newtype = arg
			if opt == '-w': newwidth = string.atoi(arg)
			if opt == '-x': xpf = string.atoi(arg)
			if opt == '-y': ypf = string.atoi(arg)
	except string.atoi_error:
		sys.stdout = sys.stderr
		print 'Option', opt, 'requires integer argument'
		sys.exit(2)
	except atof_error:
		sys.stdout = sys.stderr
		print 'Option', opt, 'requires float argument'
		sys.exit(2)

	if xpf or ypf:
		newpf = (xpf, ypf)

	if newwidth or newheight:
		if magnify:
			sys.stdout = sys.stderr
			print 'Options -w or -h are incompatible with -M'
			sys.exit(2)
		if not newheight:
			newheight = newwidth * 3 / 4
		elif not newwidth:
			newwidth = newheight * 4 / 3

	# Check filename arguments
	if len(args) < 1:
		args.append('film.video')
	if len(args) < 2:
		args.append('out.video')
	if len(args) > 2:
		usage()
		sys.exit(2)
	if args[0] == args[1]:
		sys.stderr.write('Input file can\'t be output file\n')
		sys.exit(2)

	# Do the right thing
	sts = process(args[0], args[1])

	# Exit
	sys.exit(sts)


# Copy one file to another

def process(infilename, outfilename):
	global newwidth, newheight, newpf

	try:
		vin = VFile.BasicVinFile(infilename)
	except IOError, msg:
		sys.stderr.write(infilename + ': I/O error: ' + `msg` + '\n')
		return 1
	except VFile.Error, msg:
		sys.stderr.write(msg + '\n')
		return 1
	except EOFError:
		sys.stderr.write(infilename + ': EOF in video file\n')
		return 1

	try:
		vout = VFile.BasicVoutFile(outfilename)
	except IOError, msg:
		sys.stderr.write(outfilename + ': I/O error: ' + `msg` + '\n')
		return 1

	print '=== input file ==='
	vin.printinfo()

	vout.setinfo(vin.getinfo())

	scale = 0
	flip = 0
	decompress = 0

	vinfmt = vin.format
	if vinfmt == 'compress':
		if not newtype or newtype == 'compress':
			# compressed->compressed: copy compression header
			vout.setcompressheader(vin.getcompressheader())
		else:
			# compressed->something else: go via rgb-24
			decompress = 1
			vinfmt = 'rgb'
	elif newtype == 'compress':
		# something else->compressed: not implemented
		sys.stderr.write('Sorry, conversion to compressed not yet implemented\n')
		return 1
	if newtype:
		vout.setformat(newtype)
		try:
			convert = imgconv.getconverter(vinfmt, vout.format)
		except imgconv.error, msg:
			sys.stderr.write(str(msg) + '\n')
			return 1

	if newpf:
		xpf, ypf = newpf
		if not xpf: xpf = vin.xpf
		if not ypf: ypf = vout.ypf
		newpf = (xpf, ypf)
		vout.setpf(newpf)

	if newwidth and newheight:
		scale = 1

	if vin.upside_down <> vout.upside_down or \
		  vin.mirror_image <> vout.mirror_image:
		flip = 1

	inwidth, inheight = vin.getsize()
	inwidth = inwidth / vin.xpf
	inheight = inheight / vin.ypf

	if magnify:
		newwidth = int(vout.width * magnify)
		newheight = int(vout.height * magnify)
		scale = 1

	if scale:
		vout.setsize(newwidth, newheight)
	else:
		newwidth, newheight = vout.getsize()

	if vin.packfactor <> vout.packfactor:
		scale = 1

	if scale or flip:
		if vout.bpp not in (8, 32):
			sys.stderr.write('Can\'t scale or flip this type\n')
			return 1

	newwidth = newwidth / vout.xpf
	newheight = newheight / vout.ypf

	print '=== output file ==='
	vout.printinfo()
	vout.writeheader()

	told = 0
	nin = 0
	nout = 0
	tin = 0
	tout = 0

	while 1:
		try:
			tin, data, cdata = vin.getnextframe()
		except EOFError:
			break
		if decompress:
			data = vin.decompress(data)
		nin = nin + 1
		if regen:
			tout = nin * regen
		else:
			tout = tin
		tout = int(tout / speed)
		if tout - told < mindelta:
			continue
		told = tout
		if newtype:
			data = convert(data, inwidth, inheight)
		if scale:
			data = imageop.scale(data, vout.bpp/8, \
				  inwidth, inheight, newwidth, newheight)
		if flip:
			x0, y0 = 0, 0
			x1, y1 = newwidth-1, newheight-1
			if vin.upside_down <> vout.upside_down:
				y1, y0 = y0, y1
			if vin.mirror_image <> vout.mirror_image:
				x1, x0 = x0, x1
			data = imageop.crop(data, vout.bpp/8, \
				  newwidth, newheight, x0, y0, x1, y1)
		print 'Writing frame', nout
		vout.writeframe(tout, data, cdata)
		nout = nout + 1

	vout.close()
	vin.close()


# Don't forget to call the main program

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
