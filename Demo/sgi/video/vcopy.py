# Copy a video file, interactively, frame-by-frame.

import sys
import getopt
from gl import *
from DEVICE import *

def loadframe(f, w, h, pf):
	line = f.readline()
	if not line or line == '\n':
		raise EOFError
	x = eval(line[:-1])
	if type(x) == type(0):
		if pf: size = w*h*4
		else: size = w*h*pf
	else:
		time, size = x
	data = f.read(size)
	if len(data) <> size:
		raise EOFError
	if pf:
		windata = unpackrect(w/pf, h/pf, 1, data)
		rectzoom(pf, pf)
		lrectwrite(0, 0, w/pf-1, h/pf-1, windata)
	else:
		lrectwrite(0, 0, w-1, h-1, data)
	return time, data

def report(time, iframe):
	print 'Frame', iframe, ': t =', time

def usage():
	sys.write('usage: vcopy infile outfile\n')
	sys.exit(2)

def help():
	print 'Command summary:'
	print 'n   get next image from input'
	print 'w   write current image to output'

def main():
	opts, args = getopt.getopt(sys.argv[1:], '')
	if len(args) <> 2:
		usage()
	[ifile, ofile] = args
	ifp = open(ifile, 'r')
	ofp = open(ofile, 'w')
	#
	line = ifp.readline()
	if line[:4] == 'CMIF':
		line = ifp.readline()
	x = eval(line[:-1])
	if len(x) == 3:
		w, h, pf = x
	else:
		w, h = x
		pf = 2
	if pf:
		w = (w/pf)*pf
		h = (h/pf)*pf
	#
	ofp.write('CMIF video 1.0\n')
	ofp.write(`w, h, pf` + '\n')
	#
	foreground()
	prefsize(w, h)
	wid = winopen(ifile + ' -> ' + ofile)
	RGBmode()
	gconfig()
	qdevice(KEYBD)
	qdevice(ESCKEY)
	qdevice(WINQUIT)
	qdevice(WINSHUT)
	#
	help()
	#
	time, data = loadframe(ifp, w, h, pf)
	iframe = 1
	report(time, iframe)
	#
	while 1:
		dev, val = qread()
		if dev in (ESCKEY, WINQUIT, WINSHUT):
			break
		if dev == REDRAW:
			reshapeviewport()
		elif dev == KEYBD:
			c = chr(val)
			if c == 'n':
				try:
					time, data = loadframe(ifp, w, h, pf)
					iframe = iframe+1
					report(time, iframe)
				except EOFError:
					print 'EOF'
					ringbell()
			elif c == 'w':
				ofp.write(`time, len(data)` + '\n')
				ofp.write(data)
				print 'Frame', iframe, 'written.'
			else:
				print 'Character', `c`, 'ignored'
		elif dev == INPUTCHANGE:
			pass
		else:
			print '(dev, val) =', (dev, val)

main()
