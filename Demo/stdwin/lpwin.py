#! /usr/local/python

# Watch line printer queues (only works with BSD 4.3 lpq).
#
# This brings up a window containing one line per printer argument.
#
# Each line gives a small summary of the printer's status and queue.
# The status tries to give as much relevant information as possible,
# and gives extra info if you have jobs in the queue.
#
# The line's background color gives a hint at the status: navajo white
# for idle, green if your job is now printing, yellow/orange for
# small/large queue, red for errors.
#
# To reduce the duration of the unresponsive time while it is waiting
# for an lpq subprocess to finish, it polls one printer every
# delay/len(printers) seconds.  A tiny dot indicates the last printer
# updated.  Hit the mouse button in the window to update the next one.
#
# To do:
# - add an argument to override the default delay
# - add arguments to override the default colors
# - better heuristic for small/large queue (and more colors!)
# - mouse clicks should update the printer clicked in
# - better visual appearance, e.g., boxes around the lines?

import posix
import sys
import time
import string

import stdwin
from stdwinevents import *
import mainloop

# Default parameters
DEF_PRINTER = 'oce' # This is CWI specific!
DEF_DELAY = 10

# Color assignments
c_unknown = stdwin.fetchcolor('white')
c_idle = stdwin.fetchcolor('navajo white')
c_ontop = stdwin.fetchcolor('green')
c_smallqueue = stdwin.fetchcolor('yellow')
c_bigqueue = stdwin.fetchcolor('orange')
c_error = stdwin.fetchcolor('red')

def main():
	delay = DEF_DELAY
	#
	try:
		thisuser = posix.environ['LOGNAME']
	except:
		thisuser = posix.environ['USER']
	#
	printers = sys.argv[1:]
	if printers:
		# Strip '-P' from printer names just in case
		# the user specified it...
		for i in range(len(printers)):
			if printers[i][:2] == '-P':
				printers[i] = printers[i][2:]
	else:
		if posix.environ.has_key('PRINTER'):
			printers = [posix.environ['PRINTER']]
		else:
			printers = [DEF_PRINTER]
	#
	width = stdwin.textwidth('in')*20
	height = len(printers) * stdwin.lineheight() + 5
	stdwin.setdefwinsize(width, height)
	stdwin.setdefscrollbars(0, 0)
	#
	win = stdwin.open('lpwin')
	#
	win.printers = printers
	win.colors = [c_unknown] * len(printers)
	win.texts = printers[:]
	win.next = 0
	win.delay = DEF_DELAY
	win.thisuser = thisuser
	win.dispatch = lpdispatch
	#
	win.settimer(1)
	#
	mainloop.register(win)
	mainloop.mainloop()

def lpdispatch(type, win, detail):
	if type == WE_CLOSE or type == WE_CHAR and detail in ('q', 'Q'):
		mainloop.unregister(win)
	elif type == WE_DRAW:
		drawproc(win)
	elif type == WE_TIMER:
		update(win)
		win.change((0,0), (10000, 10000))
	elif type == WE_MOUSE_UP:
		win.settimer(1)

def drawproc(win):
	d = win.begindrawing()
	offset = d.textwidth('.')
	h, v = 0, 0
	for i in range(len(win.printers)):
		text = win.texts[i]
		color = win.colors[i]
		d.setbgcolor(color)
		d.erase((h, v), (h+10000, v+d.lineheight()))
		if (i+1) % len(win.printers) == win.next and color <> c_unknown:
			d.text((h, v), '.')
		d.text((h+offset, v), text)
		v = v + d.lineheight()

def update(win):
	i = win.next
	win.next = (i+1) % len(win.printers)
	win.texts[i], win.colors[i] = makestatus(win.printers[i], win.thisuser)
	win.settimer(int(win.delay * 10.0 / len(win.printers)))

def makestatus(name, thisuser):
	pipe = posix.popen('lpq -P' + name + ' 2>&1', 'r')
	lines = []
	users = {}
	aheadbytes = 0
	aheadjobs = 0
	userseen = 0
	totalbytes = 0
	totaljobs = 0
	color = c_unknown
	while 1:
		line = pipe.readline()
		if not line: break
		fields = string.split(line)
		n = len(fields)
		if len(fields) >= 6 and fields[n-1] == 'bytes':
			rank = fields[0]
			user = fields[1]
			job = fields[2]
			files = fields[3:-2]
			bytes = eval(fields[n-2])
			if user == thisuser:
				userseen = 1
				if aheadjobs == 0:
					color = c_ontop
			elif not userseen:
				aheadbytes = aheadbytes + bytes
				aheadjobs = aheadjobs + 1
			totalbytes = totalbytes + bytes
			totaljobs = totaljobs + 1
			if color == c_unknown:
				color = c_smallqueue
			elif color == c_smallqueue:
				color = c_bigqueue
			if users.has_key(user):
				ujobs, ubytes = users[user]
			else:
				ujobs, ubytes = 0, 0
			ujobs = ujobs + 1
			ubytes = ubytes + bytes
			users[user] = ujobs, ubytes
		else:
			if fields and fields[0] <> 'Rank':
				line = string.strip(line)
				if line == 'no entries':
					line = name + ': idle'
					if color == c_unknown:
						color = c_idle
				elif line[-22:] == ' is ready and printing':
					line = line[:-22]
				else:
					line = name + ': ' + line
					color = c_error
				lines.append(line)
	#
	if totaljobs:
		line = `(totalbytes+1023)/1024` + ' K'
		if totaljobs <> len(users):
			line = line + ' (' + `totaljobs` + ' jobs)'
		if len(users) == 1:
			line = line + ' for ' + users.keys()[0]
		else:
			line = line + ' for ' + `len(users)` + ' users'
			if userseen:
				if aheadjobs == 0:
				  line =  line + ' (' + thisuser + ' first)'
				else:
				  line = line + ' (' + `(aheadbytes+1023)/1024`
				  line = line + ' K before ' + thisuser + ')'
		lines.append(line)
	#
	sts = pipe.close()
	if sts:
		lines.append('lpq exit status ' + `sts`)
		color = c_error
	return string.joinfields(lines, ': '), color

main()
