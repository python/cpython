#! /usr/bin/env python

# "M Clock"
#
# An implementation in software of an original design by Rob Juda.
# Clock implementation: Guido van Rossum.
# Alarm and Gong features: Sape Mullender.
#
# XXX TO DO:
# add arguments to specify initial window position and size
# find out local time zone difference automatically
# add a date indicator
# allow multiple alarms
# allow the menu to change more parameters

import sys

from gl import *
from GL import *
from DEVICE import *
import time
import getopt
import string
import os
from math import pi
import math

FULLC = 3600		# Full circle in 1/10-ths of a degree
MIDN = 900		# Angle of the 12 o'clock position
R, G, B = 0, 1, 2	# Indices of colors in RGB list

HOUR = 3600		# Number of seconds per hour
MINUTE = 60		# Number of seconds per minute

class struct: pass	# Class to define featureless structures
Gl = struct()		# Object to hold writable global variables

# Default constants (used in multiple places)

SCREENBG = 127, 156, 191
NPARTS = 9
TITLE = 'M Clock'

# Set timezone, check for daylight saving time
TZDIFF = time.timezone
if time.localtime(time.time())[-1]:
	TZDIFF = time.altzone

# Default parameters

Gl.foreground = 0	# If set, run in the foreground
Gl.fullscreen = 0	# If set, run on full screen
Gl.tzdiff = TZDIFF	# Seconds west of Greenwich (winter time)
Gl.nparts = NPARTS	# Number of parts each circle is divided in (>= 2)
Gl.debug = 0		# If set, print debug output
Gl.doublebuffer = 1	# If set, use double buffering
Gl.update = 0		# Update interval; seconds hand is suppressed if > 1
Gl.colorsubset = 0	# If set, display only a subset of the colors
Gl.cyan = 0		# If set, display cyan overlay (big hand)
Gl.magenta = 0		# If set, display magenta overlay (little hand)
Gl.yellow = 0		# If set, display yellow overlay (fixed background)
Gl.black = 0		# If set, display black overlay (hands)
Gl.colormap = 0		# If set, use colormap mode instead of RGB mode
Gl.warnings = 0		# If set, print warnings
Gl.title = ''		# Window title (default set later)
Gl.name = 'mclock'	# Window title for resources
Gl.border = 1		# If set, use a window border (and title)
Gl.bg = 0, 0, 0		# Background color R, G, B value
Gl.iconic = 0		# Set in iconic state
Gl.fg = 255, 0, 0	# Alarm background RGB (either normal or alarm)
Gl.ox,Gl.oy = 0,0	# Window origin
Gl.cx,Gl.cy = 0,0	# Window size
Gl.alarm_set = 0	# Alarm on or off
Gl.alarm_on = 0		# Alarm is ringing
Gl.alarm_time = 0	# Alarm time in seconds after midnight
Gl.alarm_hours = 0	# Alarm hour setting, 24 hour clock
Gl.alarm_minutes = 0	# Alarm minutes setting
Gl.alarm_rgb = 0,0,0	# Alarm display RGB colors
Gl.alarm_cmd = ''	# Command to execute when alarm goes off
Gl.mouse2down = 0	# Mouse button state
Gl.mouse3down = 0	# Mouse button state
Gl.gong_cmd = ''	# Command to execute when chimes go off
Gl.gong_int = 3600	# Gong interval
Gl.indices = R, G, B	# Colors (permuted when alarm is on)

def main():
	#
	sys.stdout = sys.stderr		# All output is errors/warnings etc.
	#
	try:
		args = getoptions()
	except string.atoi_error, value:
		usage(string.atoi_error, value)
	except getopt.error, msg:
		usage(getopt.error, msg)
	#
	if args:
		realtime = 0
		hours = string.atoi(args[0])
		minutes = seconds = 0
		if args[1:]: minutes = string.atoi(args[1])
		if args[2:]: seconds = string.atoi(args[2])
		localtime = ((hours*60)+minutes)*60+seconds
	else:
		realtime = 1
	#
	if Gl.title == '':
		if realtime:
			Gl.title = TITLE
		else:
			title = ''
			for arg in args: title = title + ' ' + arg
			Gl.title = title[1:]
			del title
	#
	wid = makewindow()
	Gl.ox,Gl.oy = getorigin()
	Gl.cx,Gl.cy = getsize()
	initmenu()
	clearall()
	#
	if not Gl.update:
		Gl.update = 60
	#
	if Gl.update <= 1:
		Gl.timernoise = 6
	else:
		Gl.timernoise = 60
	noise(TIMER0, Gl.timernoise)
	#
	qdevice(WINSHUT)
	qdevice(WINQUIT)
	qdevice(ESCKEY)
	if realtime:
		qdevice(TIMER0)
	qdevice(REDRAW)
	qdevice(WINFREEZE)
	qdevice(WINTHAW)
	qdevice(MENUBUTTON)	# MOUSE1
	qdevice(MOUSE3)		# Left button
	qdevice(MOUSE2)		# Middle button
	unqdevice(INPUTCHANGE)
	#
	lasttime = 0
	Gl.change = 1
	while 1:
		if realtime:
			localtime = int(time.time() - Gl.tzdiff)
		if Gl.alarm_set:
			if localtime%(24*HOUR) == Gl.alarm_time:
				# Ring the alarm!
				if Gl.debug:
					print 'Rrrringg!'
				Gl.alarm_on = 1
				if Gl.alarm_cmd <> '':
					d = os.system(Gl.alarm_cmd+' '+`Gl.alarm_time/3600`+' '+`(Gl.alarm_time/60)%60` + ' &')
				Gl.change = 1
				clearall()
		if Gl.alarm_on:
			if (localtime - Gl.alarm_time) % (24*HOUR) > 300:
				# More than 5 minutes away from alarm
				Gl.alarm_on = 0
				if Gl.debug:
					print 'Alarm turned off'
				Gl.change = 1
				clearall()
				Gl.indices = R, G, B
			else:
				if localtime % 2 == 0:
				  # Permute color indices
				  Gl.indices = Gl.indices[2:] + Gl.indices[:2]
				  Gl.change = 1
		if Gl.gong_cmd <> '' and localtime%Gl.gong_int == 0:
			d = os.system(Gl.gong_cmd+' '+`(localtime/3600)%24`+' '+`(localtime/60)%60` + ' &')
		if localtime/Gl.update <> lasttime/Gl.update:
			if Gl.debug: print 'new time'
			Gl.change = 1
		if Gl.change:
			if Gl.debug: print 'drawing'
			doit(localtime)
			lasttime = localtime
			Gl.change = 0
		dev, data = qread()
		if Gl.debug and dev <> TIMER0:
			print dev, data
		if dev == TIMER0:
			if Gl.debug > 1:
				print dev, data
		elif dev == MOUSE3:
			mousex = getvaluator(MOUSEX)
			mousey = getvaluator(MOUSEY)
			if mouseclick(3, data, mousex, mousey):
				Gl.change = 1
		elif dev == MOUSE2:
			mousex = getvaluator(MOUSEX)
			mousey = getvaluator(MOUSEY)
			if mouseclick(2, data, mousex, mousey):
				Gl.change = 1
		elif dev == MOUSEX:
			mousex = data
			if Gl.mouse2down:
				mouse2track(mousex, mousey)
			if Gl.mouse3down:
				mouse3track(mousex, mousey)
		elif dev == MOUSEY:
			mousey = data
			if Gl.mouse2down:
				mouse2track(mousex, mousey)
			if Gl.mouse3down:
				mouse3track(mousex, mousey)
		elif dev == REDRAW or dev == REDRAWICONIC:
			if Gl.debug:
				if dev == REDRAW: print 'REDRAW'
				else: print 'REDRAWICONIC'
			reshapeviewport()
			Gl.ox,Gl.oy = getorigin()
			Gl.cx,Gl.cy = getsize()
			Gl.change = 1
			clearall()
		elif dev == MENUBUTTON:
			if Gl.debug: print 'MENUBUTTON'
			handlemenu()
		elif dev == WINFREEZE:
			if Gl.debug: print 'WINFREEZE'
			Gl.iconic = 1
			noise(TIMER0, 60*60) # Redraw every 60 seconds only
		elif dev == WINTHAW:
			if Gl.debug: print 'WINTHAW'
			Gl.iconic = 0
			noise(TIMER0, Gl.timernoise)
			Gl.change = 1
		elif dev == ESCKEY or dev == WINSHUT or dev == WINQUIT:
			if Gl.debug: print 'Exit'
			sys.exit(0)

def getoptions():
	optlist, args = getopt.getopt(sys.argv[1:], 'A:a:B:bc:dFfG:g:n:sT:t:u:wCMYK')
	for optname, optarg in optlist:
		if optname == '-A':
			Gl.fg = eval(optarg)	# Should be (r,g,b)
		elif optname == '-a':
			Gl.alarm_cmd = optarg
		elif optname == '-B':
			Gl.bg = eval(optarg)	# Should be (r,g,b)
		elif optname == '-b':
			Gl.border = 0
		elif optname == '-c':
			Gl.colormap = string.atoi(optarg)
		elif optname == '-d':
			Gl.debug = Gl.debug + 1
			Gl.warnings = 1
		elif optname == '-F':
			Gl.foreground = 1
		elif optname == '-f':
			Gl.fullscreen = 1
		elif optname == '-G':
			Gl.gong_int = 60*string.atoi(optarg)
		elif optname == '-g':
			Gl.gong_cmd = optarg
		elif optname == '-n':
			Gl.nparts = string.atoi(optarg)
		elif optname == '-s':
			Gl.doublebuffer = 0
		elif optname == '-T':
			Gl.title = Gl.name = optarg
		elif optname == '-t':
			Gl.tzdiff = string.atoi(optarg)
		elif optname == '-u':
			Gl.update = string.atoi(optarg)
		elif optname == '-w':
			Gl.warnings = 1
		elif optname == '-C':
			Gl.cyan = Gl.colorsubset = 1
		elif optname == '-M':
			Gl.magenta = Gl.colorsubset = 1
		elif optname == '-Y':
			Gl.yellow = Gl.colorsubset = 1
		elif optname == '-K':
			Gl.black = Gl.colorsubset = 1
		else:
			print 'Unsupported option', optname
	return args

def usage(exc, msg):
	if sys.argv:
		progname = os.path.basename(sys.argv[0])
	else:
		progname = 'mclock'
	#
	print progname + ':',
	if exc == string.atoi_error:
		print 'non-numeric argument:',
	print msg
	#
	print 'usage:', progname, '[options] [hh [mm [ss]]]'
	#
	print '-A r,g,b  : alarm background red,green,blue [255,0,0]'
	print '-a cmd    : shell command executed when alarm goes off'
	print '-B r,g,b  : background red,green,blue [0,0,0]'
	print '            (-B SCREENBG uses the default screen background)'
	print '-b        : suppress window border and title'
	print '-c cmapid : select explicit colormap'
	print '-d        : more debug output (implies -F, -w)'
	print '-F        : run in foreground'
	print '-f        : use full screen'
	print '-G intrvl : interval between chimes in minutes [60]'
	print '-g cmd    : shell command executed when chimes go off'
	print '-s        : single buffer mode'
	print '-w        : print various warnings'
	print '-n nparts : number of parts [' + `NPARTS` + ']'
	print '-T title  : alternate window title [\'' + TITLE + '\']'
	print '-t tzdiff : time zone difference [' + `TZDIFF` + ']'
	print '-u update : update interval [60]'
	print '-CMYK     : Cyan, Magenta, Yellow or blacK overlay only'
	print 'if hh [mm [ss]] is specified, display that time statically'
	print 'on machines with < 12 bitplanes, -s is forced on'
	#
	sys.exit(2)

def doit(localtime):
	hands = makehands(localtime)
	list = makelist(hands)
	render(list, hands)

def makehands(localtime):
	localtime = localtime % (12*HOUR)
	seconds_hand = MIDN + FULLC - (localtime*60) % FULLC
	big_hand = (MIDN + FULLC - (localtime%HOUR)) % FULLC
	little_hand = (MIDN + FULLC - ((localtime/12) % HOUR)) % FULLC
	return little_hand, big_hand, seconds_hand

def makelist(hands):
	little_hand, big_hand, seconds_hand = hands
	total = []
	if Gl.cyan or not Gl.colorsubset:
		total = total + makesublist(big_hand, Gl.indices[0])
	if Gl.magenta or not Gl.colorsubset:
		total = total + makesublist(little_hand, Gl.indices[1])
	if Gl.yellow or not Gl.colorsubset:
		total = total + makesublist(MIDN, Gl.indices[2])
	total.sort()
	return total

def makesublist(first, icolor):
	list = []
	alpha = FULLC/Gl.nparts
	a = first - alpha/2
	for i in range(Gl.nparts):
		angle = (a + i*alpha + FULLC) % FULLC
		value = 255*(Gl.nparts-1-i)/(Gl.nparts-1)
		list.append((angle, icolor, value))
	list.sort()
	a, icolor, value = list[0]
	if a <> 0:
		a, icolor, value = list[len(list)-1]
		t = 0, icolor, value
		list.insert(0, t)
	return list

def rgb_fg():
	return Gl.fg
	# Obsolete code:
	if Gl.alarm_on:
		return Gl.bg
	else:
		return Gl.fg

def rgb_bg():
	return Gl.bg
	# Obsolete code:
	if Gl.alarm_on:
		return Gl.fg
	else:
		return Gl.bg

def clearall():
	Gl.c3i(rgb_bg())
	clear()
	if Gl.doublebuffer:
		swapbuffers()
		clear()

def draw_alarm(color):
	frontbuffer(TRUE)
	Gl.c3i(color)
	pushmatrix()
	rotate(-((Gl.alarm_time/12)%3600), 'z')
	bgnpolygon()
	v2f( 0.00,1.00)
	v2f( 0.04,1.05)
	v2f(-0.04,1.05)
	endpolygon()
	popmatrix()
	#
	pushmatrix()
	rotate(-((Gl.alarm_time)%3600), 'z')
	bgnpolygon()
	v2f( 0.00,1.05)
	v2f( 0.07,1.10)
	v2f(-0.07,1.10)
	endpolygon()
	popmatrix()
	#
	cmov2(-1.06, -1.06)
	charstr(string.rjust(`Gl.alarm_time/3600`,2))
	charstr(':')
	charstr(string.zfill((Gl.alarm_time/60)%60,2))
	frontbuffer(FALSE)

def render(list, (little_hand, big_hand, seconds_hand)):
	#
	if Gl.colormap:
		resetindex()
	#
	if not list:
		Gl.c3i((255, 255, 255)) # White
		circf(0.0, 0.0, 1.0)
	else:
		list.append((3600, 0, 255)) # Sentinel
	#
	rgb = [255, 255, 255]
	a_prev = 0
	for a, icolor, value in list:
		if a <> a_prev:
			[r, g, b] = rgb
			if Gl.debug > 1:
				print rgb, a_prev, a
			Gl.c3i((r, g, b))
			arcf(0.0, 0.0, 1.0, a_prev, a)
		rgb[icolor] = value
		a_prev = a
	#
	if Gl.black or not Gl.colorsubset:
		#
		# Draw the hands -- in black
		#
		Gl.c3i((0, 0, 0))
		#
		if Gl.update == 1 and not Gl.iconic:
			# Seconds hand is only drawn if we update every second
			pushmatrix()
			rotate(seconds_hand, 'z')
			bgnline()
			v2f(0.0, 0.0)
			v2f(1.0, 0.0)
			endline()
			popmatrix()
		#
		pushmatrix()
		rotate(big_hand, 'z')
		rectf(0.0, -0.01, 0.97, 0.01)
		circf(0.0, 0.0, 0.01)
		circf(0.97, 0.0, 0.01)
		popmatrix()
		#
		pushmatrix()
		rotate(little_hand, 'z')
		rectf(0.04, -0.02, 0.63, 0.02)
		circf(0.04, 0.0, 0.02)
		circf(0.63, 0.0, 0.02)
		popmatrix()
		#
		# Draw the alarm time, if set or being set
		#
		if Gl.alarm_set:
			draw_alarm(rgb_fg())
	#
	if Gl.doublebuffer: swapbuffers()

def makewindow():
	#
	if Gl.debug or Gl.foreground:
		foreground()
	#
	if Gl.fullscreen:
		scrwidth, scrheight = getgdesc(GD_XPMAX), getgdesc(GD_YPMAX)
		prefposition(0, scrwidth-1, 0, scrheight-1)
	else:
		keepaspect(1, 1)
		prefsize(80, 80)
	#
	if not Gl.border:
		noborder()
	wid = winopen(Gl.name)
	wintitle(Gl.title)
	#
	if not Gl.fullscreen:
		keepaspect(1, 1)
		minsize(10, 10)
		maxsize(2000, 2000)
		iconsize(66, 66)
		winconstraints()
	#
	nplanes = getplanes()
	nmaps = getgdesc(GD_NMMAPS)
	if Gl.warnings:
		print nplanes, 'color planes,', nmaps, 'color maps'
	#
	if Gl.doublebuffer and not Gl.colormap and nplanes < 12:
		if Gl.warnings: print 'forcing single buffer mode'
		Gl.doublebuffer = 0
	#
	if Gl.colormap:
		if not Gl.colormap:
			Gl.colormap = nmaps - 1
			if Gl.warnings:
				print 'not enough color planes available',
				print 'for RGB mode; forcing colormap mode'
				print 'using color map number', Gl.colormap
		if not Gl.colorsubset:
			needed = 3
		else:
			needed = Gl.cyan + Gl.magenta + Gl.yellow
		needed = needed*Gl.nparts
		if Gl.bg <> (0, 0, 0):
			needed = needed+1
		if Gl.fg <> (0, 0, 0):
			needed = needed+1
		if Gl.doublebuffer:
			if needed > available(nplanes/2):
				Gl.doublebuffer = 0
				if Gl.warnings:
					print 'not enough colors available',
					print 'for double buffer mode;',
					print 'forcing single buffer mode'
			else:
				nplanes = nplanes/2
		if needed > available(nplanes):
			# Do this warning always
			print 'still not enough colors available;',
			print 'parts will be left white'
			print '(needed', needed, 'but have only',
			print available(nplanes), 'colors available)'
	#
	if Gl.doublebuffer:
		doublebuffer()
		gconfig()
	#
	if Gl.colormap:
		Gl.c3i = pseudo_c3i
		fixcolormap()
	else:
		Gl.c3i = c3i
		RGBmode()
		gconfig()
	#
	if Gl.fullscreen:
		# XXX Should find out true screen size using getgdesc()
		ortho2(-1.1*1.280, 1.1*1.280, -1.1*1.024, 1.1*1.024)
	else:
		ortho2(-1.1, 1.1, -1.1, 1.1)
	#
	return wid

def available(nplanes):
	return pow(2, nplanes) - 1	# Reserve one pixel for black

def fixcolormap():
	multimap()
	gconfig()
	nplanes = getplanes()
	if Gl.warnings:
		print 'multimap mode has', nplanes, 'color planes'
	imap = Gl.colormap
	Gl.startindex = pow(2, nplanes) - 1
	Gl.stopindex = 1
	setmap(imap)
	mapcolor(0, 0, 0, 0) # Fixed entry for black
	if Gl.bg <> (0, 0, 0):
		r, g, b = Gl.bg
		mapcolor(1, r, g, b) # Fixed entry for Gl.bg
		Gl.stopindex = 2
	if Gl.fg <> (0, 0, 0):
		r, g, b = Gl.fg
		mapcolor(2, r, g, b) # Fixed entry for Gl.fg
		Gl.stopindex = 3
	Gl.overflow_seen = 0
	resetindex()

def resetindex():
	Gl.index = Gl.startindex

r0g0b0 = (0, 0, 0)

def pseudo_c3i(rgb):
	if rgb == r0g0b0:
		index = 0
	elif rgb == Gl.bg:
		index = 1
	elif rgb == Gl.fg:
		index = 2
	else:
		index = definecolor(rgb)
	color(index)

def definecolor(rgb):
	index = Gl.index
	if index < Gl.stopindex:
		if Gl.debug: print 'definecolor hard case', rgb
		# First see if we already have this one...
		for index in range(Gl.stopindex, Gl.startindex+1):
			if rgb == getmcolor(index):
				if Gl.debug: print 'return', index
				return index
		# Don't clobber reserverd colormap entries
		if not Gl.overflow_seen:
			# Shouldn't happen any more, hence no Gl.warnings test
			print 'mclock: out of colormap entries'
			Gl.overflow_seen = 1
		return Gl.stopindex
	r, g, b = rgb
	if Gl.debug > 1: print 'mapcolor', (index, r, g, b)
	mapcolor(index, r, g, b)
	Gl.index = index - 1
	return index

# Compute n**i
def pow(n, i):
	x = 1
	for j in range(i): x = x*n
	return x

def mouseclick(mouse, updown, x, y):
	if updown == 1:
		# mouse button came down, start tracking
		if Gl.debug:
			print 'mouse', mouse, 'down at', x, y
		if mouse == 2:
			Gl.mouse2down = 1
			mouse2track(x, y)
		elif mouse == 3:
			Gl.mouse3down = 1
			mouse3track(x, y)
		else:
			print 'fatal error'
		qdevice(MOUSEX)
		qdevice(MOUSEY)
		return 0
	else:
		# mouse button came up, stop tracking
		if Gl.debug:
			print 'mouse', mouse, 'up at', x, y
		unqdevice(MOUSEX)
		unqdevice(MOUSEY)
		if mouse == 2:
			mouse2track(x, y)
			Gl.mouse2down = 0
		elif mouse == 3:
			mouse3track(x, y)
			Gl.mouse3down = 0
		else:
			print 'fatal error'
		Gl.alarm_set = 1
		return 1

def mouse3track(x, y):
	# first compute polar coordinates from x and y
	cx, cy = Gl.ox + Gl.cx/2, Gl.oy + Gl.cy/2
	x, y = x - cx, y - cy
	if (x, y) == (0, 0): return	# would cause an exception
	minutes = int(30.5 + 30.0*math.atan2(float(-x), float(-y))/pi)
	if minutes == 60: minutes = 0
	a,b = Gl.alarm_minutes/15, minutes/15
	if (a,b) == (0,3):
		# Moved backward through 12 o'clock:
		Gl.alarm_hours = Gl.alarm_hours - 1
		if Gl.alarm_hours < 0: Gl.alarm_hours = Gl.alarm_hours + 24
	if (a,b) == (3,0):
		# Moved forward through 12 o'clock:
		Gl.alarm_hours = Gl.alarm_hours + 1
		if Gl.alarm_hours >= 24: Gl.alarm_hours = Gl.alarm_hours - 24
	Gl.alarm_minutes = minutes
	seconds = Gl.alarm_hours * HOUR + Gl.alarm_minutes * MINUTE
	if seconds <> Gl.alarm_time:
		draw_alarm(rgb_bg())
		Gl.alarm_time = seconds
		draw_alarm(rgb_fg())

def mouse2track(x, y):
	# first compute polar coordinates from x and y
	cx, cy = Gl.ox + Gl.cx/2, Gl.oy + Gl.cy/2
	x, y = x - cx, y - cy
	if (x, y) == (0, 0): return	# would cause an exception
	hours = int(6.5 - float(Gl.alarm_minutes)/60.0 + 6.0*math.atan2(float(-x), float(-y))/pi)
	if hours == 12: hours = 0
	if (Gl.alarm_hours,hours) == (0,11):
		# Moved backward through midnight:
		Gl.alarm_hours = 23
	elif (Gl.alarm_hours,hours) == (12,11):
		# Moved backward through noon:
		Gl.alarm_hours = 11
	elif (Gl.alarm_hours,hours) == (11,0):
		# Moved forward through noon:
		Gl.alarm_hours = 12
	elif (Gl.alarm_hours,hours) == (23,0):
		# Moved forward through midnight:
		Gl.alarm_hours = 0
	elif Gl.alarm_hours < 12:
		Gl.alarm_hours = hours
	else:
		Gl.alarm_hours = hours + 12
	seconds = Gl.alarm_hours * HOUR + Gl.alarm_minutes * MINUTE
	if seconds <> Gl.alarm_time:
		draw_alarm(rgb_bg())
		Gl.alarm_time = seconds
		draw_alarm(rgb_fg())

def initmenu():
	Gl.pup = pup = newpup()
	addtopup(pup, 'M Clock%t|Alarm On/Off|Seconds Hand On/Off|Quit', 0)

def handlemenu():
	item = dopup(Gl.pup)
	if item == 1:
		# Toggle alarm
		if Gl.alarm_set:
			Gl.alarm_set = 0
			Gl.alarm_on = 0
		else:
			Gl.alarm_set = 1
		Gl.change = 1
		clearall()
	elif item == 2:
		# Toggle Seconds Hand
		if Gl.update == 1:
			Gl.update = 60
			Gl.timernoise = 60
		else:
			Gl.update = 1
			Gl.timernoise = 6
		Gl.change = 1
	elif item == 3:
		if Gl.debug: print 'Exit'
		sys.exit(0)

main()
