# Copy a video file, interactively, frame-by-frame.

import sys
import getopt
from gl import *
from DEVICE import *
import VFile
import string
import imageop

def report(time, iframe):
	print 'Frame', iframe, ': t =', time

def usage():
	sys.stderr.write('usage: Vcopy [-t type] [-m treshold] [-a] infile outfile\n')
	sys.stderr.write('-t Convert to other type\n')
	sys.stderr.write('-a Automatic\n')
	sys.stderr.write('-m Convert grey to mono with treshold\n')
	sys.stderr.write('-d Convert grey to mono with dithering\n')
	sys.exit(2)

def help():
	print 'Command summary:'
	print 'n   get next image from input'
	print 'w   write current image to output'

def main():
	foreground()
	opts, args = getopt.getopt(sys.argv[1:], 't:am:d')
	if len(args) <> 2:
		usage()
	[ifile, ofile] = args
	print 'open film ', ifile
	ifilm = VFile.VinFile().init(ifile)
	print 'open output ', ofile
	ofilm = VFile.VoutFile().init(ofile)
	
	ofilm.setinfo(ifilm.getinfo())

	use_grabber = 0
	continuous = 0
	tomono = 0
	tomonodither = 0
	for o, a in opts:
		if o == '-t':
			ofilm.format = a
			use_grabber = 1
		if o == '-a':
			continuous = 1
		if o == '-m':
			if ifilm.format <> 'grey':
				print '-m only supported for greyscale'
				sys.exit(1)
			tomono = 1
			treshold = string.atoi(a)
			ofilm.format = 'mono'
		if o == '-d':
			if ifilm.format <> 'grey':
				print '-m only supported for greyscale'
				sys.exit(1)
			tomonodither = 1
			ofilm.format = 'mono'
			
	ofilm.writeheader()
	#
	prefsize(ifilm.width, ifilm.height)
	w = winopen(ifile)
	qdevice(KEYBD)
	qdevice(ESCKEY)
	qdevice(WINQUIT)
	qdevice(WINSHUT)
	print 'qdevice calls done'
	#
	help()
	#
	time, data, cdata = ifilm.getnextframe()
	ifilm.showframe(data, cdata)
	iframe = 1
	report(time, iframe)
	#
	while 1:
		if continuous:
			dev = KEYBD
		else:
			dev, val = qread()
		if dev in (ESCKEY, WINQUIT, WINSHUT):
			break
		if dev == REDRAW:
			reshapeviewport()
		elif dev == KEYBD:
			if continuous:
				c = '0'
			else:
				c = chr(val)
			#XXX Debug
			if c == 'R':
				c3i(255,0,0)
				clear()
			if c == 'G':
				c3i(0,255,0)
				clear()
			if c == 'B':
				c3i(0,0,255)
				clear()
			if c == 'w' or continuous:
				if use_grabber:
					data, cdata = ofilm.grabframe()
				if tomono:
					data = imageop.grey2mono(data, \
						  ifilm.width, ifilm.height, \
						  treshold)
				if tomonodither:
					data = imageop.dither2mono(data, \
						  ifilm.width, ifilm.height)
				ofilm.writeframe(time, data, cdata)
				print 'Frame', iframe, 'written.'
			if c == 'n' or continuous:
				try:
					time,data,cdata = ifilm.getnextframe()
					ifilm.showframe(data, cdata)
					iframe = iframe+1
					report(time, iframe)
				except EOFError:
					print 'EOF'
					if continuous:
						break
					ringbell()
		elif dev == INPUTCHANGE:
			pass
		else:
			print '(dev, val) =', (dev, val)
	ofilm.close()

main()
