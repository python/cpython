#! /ufs/guido/bin/sgi/python

# Play CMIF movie files


# Help function

def help():
	print 'Usage: Vplay [options] [file] ...'
	print
	print 'Options:'
	print '-M magnify : magnify the image by the given factor'
	print '-d         : write some debug stuff on stderr'
	print '-l         : loop, playing the movie over and over again'
	print '-m delta   : drop frames closer than delta msec (default 0)'
	print '-n         : don\'t wait after each file'
	print '-q         : quiet, no informative messages'
	print '-r delta   : regenerate input time base delta msec apart'
	print '-s speed   : speed change factor (default 1.0)'
	print '-t         : use a 2nd thread for read-ahead'
	print '-x left    : window offset from left of screen'
	print '-y top     : window offset from top of screen'
	print 'file ...   : file(s) to play; default film.video'
	print
	print 'User interface:'
	print 'Press the left mouse button to stop or restart the movie.'
	print 'Press ESC or use the window manager Close or Quit command'
	print 'to close the window and play the next file (if any).'


# Imported modules

import sys
sys.path.append('/ufs/guido/src/video') # Increase chance of finding VFile
import VFile
import time
import gl, GL
from DEVICE import REDRAW, ESCKEY, LEFTMOUSE, WINSHUT, WINQUIT
import getopt
import string


# Global options

debug = 0
looping = 0
magnify = 1
mindelta = 0
nowait = 0
quiet = 0
regen = None
speed = 1.0
threading = 0
xoff = yoff = None


# Main program -- mostly command line parsing

def main():
	global debug, looping, magnify, mindelta, nowait, quiet, regen, speed
	global threading, xoff, yoff

	# Parse command line
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'M:dlm:nqr:s:tx:y:')
	except getopt.error, msg:
		sys.stdout = sys.stderr
		print 'Error:', msg, '\n'
		help()
		sys.exit(2)

	# Interpret options
	try:
		for opt, arg in opts:
			if opt == '-M': magnify = float(eval(arg))
			if opt == '-d': debug = debug + 1
			if opt == '-l': looping = 1
			if opt == '-m': mindelta = string.atoi(arg)
			if opt == '-n': nowait = 1
			if opt == '-q': quiet = 1
			if opt == '-r': regen = string.atoi(arg)
			if opt == '-s':
				try:
					speed = float(eval(arg))
				except:
					sys.stdout = sys.stderr
					print 'Option -s needs float argument'
					sys.exit(2)
			if opt == '-t':
				try:
					import thread
					threading = 1
				except ImportError:
					print 'Sorry, this version of Python',
					print 'does not support threads:',
					print '-t ignored'
			if opt == '-x': xoff = string.atoi(arg)
			if opt == '-y': yoff = string.atoi(arg)
	except string.atoi_error:
		sys.stdout = sys.stderr
		print 'Option', opt, 'requires integer argument'
		sys.exit(2)

	# Check validity of certain options combinations
	if nowait and looping:
		print 'Warning: -n and -l are mutually exclusive; -n ignored'
		nowait = 0
	if xoff <> None and yoff == None:
		print 'Warning: -x without -y ignored'
	if xoff == None and yoff <> None:
		print 'Warning: -y without -x ignored'

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
		vin = VFile.VinFile().init(filename)
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
	
	gl.foreground()

	width, height = int(vin.width * magnify), int(vin.height * magnify)
	if xoff <> None and yoff <> None:
		scrheight = gl.getgdesc(GL.GD_YPMAX)
		gl.prefposition(xoff, xoff+width-1, \
			scrheight-yoff-height, scrheight-yoff-1)
	else:
		gl.prefsize(width, height)

	win = gl.winopen(filename)
	gl.clear()

	if quiet: vin.quiet = 1
	vin.initcolormap()

	gl.qdevice(ESCKEY)
	gl.qdevice(WINSHUT)
	gl.qdevice(WINQUIT)
	gl.qdevice(LEFTMOUSE)

	stop = 0

	while not stop:
		gl.wintitle(filename)
		stop = (playonce(vin) or nowait)
		gl.wintitle('(done) ' + filename)
		if not looping:
			while not stop:
				dev, val = gl.qread()
				if dev == REDRAW:
					vin.clear()
				if dev == LEFTMOUSE and val == 1:
					break # Continue outer loop
				if dev == ESCKEY and val == 1 or \
						dev in (WINSHUT, WINQUIT):
					stop = 1

	# Set xoff, yoff for the next window from the current window
	global xoff, yoff
	xoff, yoff = gl.getorigin()
	width, height = gl.getsize()
	scrheight = gl.getgdesc(GL.GD_YPMAX)
	yoff = scrheight - yoff - height
	gl.winclose(win)

	return 0


# Play a movie once; return 1 if user wants to stop, 0 if not

def playonce(vin):
	vin.rewind()
	vin.colormapinited = 1
	vin.magnify = magnify

	if threading:
		MAXSIZE = 20 # Don't read ahead too much
		import thread
		import Queue
		queue = Queue.Queue().init(MAXSIZE)
		stop = []
		thread.start_new_thread(read_ahead, (vin, queue, stop))
		# Get the read-ahead thread going
		while queue.qsize() < MAXSIZE/2 and not stop:
			time.millisleep(100)

	tin = 0
	toffset = 0
	oldtin = 0
	told = 0
	nin = 0
	nout = 0
	nlate = 0
	nskipped = 0
	data = None

	tlast = t0 = time.millitimer()

	while 1:
		if gl.qtest():
			dev, val = gl.qread()
			if dev == ESCKEY and val == 1 or \
					dev in (WINSHUT, WINQUIT) or \
					dev == LEFTMOUSE and val == 1:
				if debug: sys.stderr.write('\n')
				if threading:
					stop.append(None)
					while 1:
						item = queue.get()
						if item == None: break
				return (dev != LEFTMOUSE)
			if dev == REDRAW:
				gl.reshapeviewport()
				if data: vin.showframe(data, cdata)
		if threading:
			if debug and queue.empty(): sys.stderr.write('.')
			item = queue.get()
			if item == None: break
			tin, data, cdata = item
		else:
			try:
				tin, size, csize = vin.getnextframeheader()
			except EOFError:
				break
		nin = nin+1
		if tin+toffset < oldtin:
			print 'Fix reversed time:', oldtin, 'to', tin
			toffset = oldtin - tin
		tin = tin + toffset
		oldtin = tin
		if regen: tout = nin * regen
		else: tout = tin
		tout = int(tout / speed)
		if tout - told < mindelta:
			nskipped = nskipped + 1
			if not threading:
				vin.skipnextframedata(size, csize)
		else:
			if not threading:
				try:
					data, cdata = \
					  vin.getnextframedata(size, csize)
				except EOFError:
					if not quiet:
						print '[incomplete last frame]'
					break
			now = time.millitimer()
			dt = (tout-told) - (now-tlast)
			told = tout
			if debug: sys.stderr.write(`dt/10` + ' ')
			if dt < 0: nlate = nlate + 1
			if dt > 0:
				time.millisleep(dt)
				now = now + dt
			tlast = now
			vin.showframe(data, cdata)
			nout = nout + 1

	t1 = time.millitimer()

	if debug: sys.stderr.write('\n')

	if quiet: return 0

	print 'Recorded:', nin, 'frames in', tin*0.001, 'sec.',
	if tin: print '-- average', int(nin*10000.0/tin)*0.1, 'frames/sec',
	print

	if nskipped: print 'Skipped', nskipped, 'frames'

	tout = t1-t0
	print 'Played:', nout,
	print 'frames in', tout*0.001, 'sec.',
	if tout: print '-- average', int(nout*10000.0/tout)*0.1, 'frames/sec',
	print

	if nlate: print 'There were', nlate, 'late frames'

	return 0


# Read-ahead thread

def read_ahead(vin, queue, stop):
	try:
		while not stop: queue.put(vin.getnextframe())
	except EOFError:
		pass
	queue.put(None)
	stop.append(None)


# Don't forget to call the main program

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
