#! /usr/local/python

# 'clock' -- A simple alarm clock

# The alarm can be set at 5 minute intervals on a 12 hour basis.
# It is controlled with the mouse:
# - Click and drag around the circle to set the alarm.
# - Click far outside the circle to clear the alarm.
# - Click near the center to set the alarm at the last time set.
# The alarm time is indicated by a small triangle just outside the circle,
# and also by a digital time at the bottom.
# The indicators disappear when the alarm is not set.
# When the alarm goes off, it beeps every minute for five minutes,
# and the clock turns into inverse video.
# Click or activate the window to turn the ringing off.

import stdwin
from stdwinevents import WE_MOUSE_DOWN, WE_MOUSE_MOVE, WE_MOUSE_UP, \
	WE_TIMER, WE_DRAW, WE_SIZE, WE_CLOSE, WE_ACTIVATE
import mainloop
import time
from math import sin, cos, atan2, pi, sqrt

DEFWIDTH, DEFHEIGHT = 200, 200

MOUSE_EVENTS = (WE_MOUSE_DOWN, WE_MOUSE_MOVE, WE_MOUSE_UP)
ORIGIN = 0, 0
FARAWAY = 2000, 2000
EVERYWHERE = ORIGIN, FARAWAY

# TZDIFF = 5*3600	# THINK C 3.0 returns UCT if local time is EST
TZDIFF = 0		# THINK C 4.0 always returns local time


def main():
	win = makewindow()
	del win
	mainloop.mainloop()

def makewindow():
	stdwin.setdefwinsize(DEFWIDTH, DEFHEIGHT + stdwin.lineheight())
	win = stdwin.open('clock')
	setdimensions(win)
	win.set = 1		# True when alarm is set
	win.time = 11*60 + 40	# Time when alarm must go off
	win.ring = 0		# True when alarm is ringing
	win.dispatch = cdispatch
	mainloop.register(win)
	settimer(win)
	return win

def cdispatch(event):
	type, win, detail = event
	if type == WE_DRAW:
		drawproc(win, detail)
	elif type == WE_TIMER:
		settimer(win)
		drawproc(win, EVERYWHERE)
	elif type in MOUSE_EVENTS:
		mouseclick(win, type, detail)
	elif type == WE_ACTIVATE:
		if win.ring:
			# Turn the ringing off
			win.ring = 0
			win.begindrawing().invert(win.mainarea)
	elif type == WE_SIZE:
		win.change(EVERYWHERE)
		setdimensions(win)
	elif type == WE_CLOSE:
		mainloop.unregister(win)

def setdimensions(win):
	width, height = win.getwinsize()
	height = height - stdwin.lineheight()
	if width < height: size = width
	else: size = height
	halfwidth = width/2
	halfheight = height/2
	win.center = halfwidth, halfheight
	win.radius = size*45/100
	win.width = width
	win.height = height
	win.corner = width, height
	win.mainarea = ORIGIN, win.corner
	win.lineheight = stdwin.lineheight()
	win.farcorner = width, height + win.lineheight
	win.statusarea = (0, height), win.farcorner
	win.fullarea = ORIGIN, win.farcorner

def settimer(win):
	now = getlocaltime()
	win.times = calctime(now)
	delay = 61 - now % 60
	win.settimer(10 * delay)
	minutes = (now/60) % 720
	if win.ring:
		# Is it time to stop the alarm ringing?
		since = (minutes - win.time + 720) % 720
		if since >= 5:
			# Stop it now
			win.ring = 0
		else:
			# Ring again, once every minute
			stdwin.fleep()
	elif win.set and minutes == win.time:
		# Start the alarm ringing
		win.ring = 1
		stdwin.fleep()

def drawproc(win, area):
	hours, minutes, seconds = win.times
	d = win.begindrawing()
	d.cliprect(area)
	d.erase(EVERYWHERE)
	d.circle(win.center, win.radius)
	d.line(win.center, calcpoint(win, hours*30 + minutes/2, 0.6))
	d.line(win.center, calcpoint(win, minutes*6, 1.0))
	str = dd(hours) + ':' + dd(minutes)
	p = (win.width - d.textwidth(str))/2, win.height * 3 / 4
	d.text(p, str)
	if win.set:
		drawalarm(win, d)
		drawalarmtime(win, d)
	if win.ring:
		d.invert(win.mainarea)

def mouseclick(win, type, detail):
	d = win.begindrawing()
	if win.ring:
		# First turn the ringing off
		win.ring = 0
		d.invert(win.mainarea)
	h, v = detail[0]
	ch, cv = win.center
	x, y = h-ch, cv-v
	dist = sqrt(x*x + y*y) / float(win.radius)
	if dist > 1.2:
		if win.set:
			drawalarm(win, d)
			erasealarmtime(win, d)
			win.set = 0
	elif dist < 0.8:
		if not win.set:
			win.set = 1
			drawalarm(win, d)
			drawalarmtime(win, d)
	else:
		# Convert to half-degrees (range 0..720)
		alpha = atan2(y, x)
		hdeg = alpha*360.0/pi
		hdeg = 180.0 - hdeg
		hdeg = (hdeg + 720.0) % 720.0
		atime = 5*int(hdeg/5.0 + 0.5)
		if atime <> win.time or not win.set:
			if win.set:
				drawalarm(win, d)
				erasealarmtime(win, d)
			win.set = 1
			win.time = atime
			drawalarm(win, d)
			drawalarmtime(win, d)

def drawalarm(win, d):
	p1 = calcpoint(win, float(win.time)/2.0, 1.02)
	p2 = calcpoint(win, float(win.time)/2.0 - 4.0, 1.1)
	p3 = calcpoint(win, float(win.time)/2.0 + 4.0, 1.1)
	d.xorline(p1, p2)
	d.xorline(p2, p3)
	d.xorline(p3, p1)

def erasealarmtime(win, d):
	d.erase(win.statusarea)

def drawalarmtime(win, d):
	# win.time is in the range 0..720 with origin at 12 o'clock
	# Convert to hours (0..12) and minutes (12*(0..60))
	hh = win.time/60
	mm = win.time%60
	str = 'Alarm@' + dd(hh) + ':' + dd(mm)
	p1 = (win.width - d.textwidth(str))/2, win.height
	d.text(p1, str)

def calcpoint(win, degrees, size):
	alpha = pi/2.0 - float(degrees) * pi/180.0
	x, y = cos(alpha), sin(alpha)
	h, v = win.center
	r = float(win.radius)
	return h + int(x*size*r), v - int(y*size*r)

def calctime(now):
	seconds = now % 60
	minutes = (now/60) % 60
	hours = (now/3600) % 12
	return hours, minutes, seconds

def dd(n):
	s = `n`
	return '0'*(2-len(s)) + s

def getlocaltime():
	return time.time() - TZDIFF

main()
