#! /ufs/guido/bin/sgi/python

# XXX This file is being hacked -- some functionality has been taken out!

# JUKEBOX: browse directories full of sampled sound files.
#
# One or more "list windows" display the files and subdirectories of
# the arguments.  Double-clicking on a subdirectory opens a new window
# displaying its contents (and so on recursively).  Double clicking
# on a file plays it as a sound file (assuming it is one).
#
# Playing is asynchronous: the application keeps listening to events
# while the sample is playing, so you can change the volume (gain)
# during playing, cancel playing or start a new sample right away.
#
# The control window displays the current output gain and a primitive
# "stop button" to cancel the current play request.
#
# Sound files must currently be in Dik Winter's compressed Mac format.
# Since decompression is costly, decompressed samples are saved in
# /usr/tmp/@j* until the application is left.  The files are read
# afresh each time, though.

import commands
import getopt
import os
import rand
import stdwin
from stdwinevents import *
import string
import sys
import tempfile

from WindowParent import WindowParent
from HVSplit import VSplit
from Buttons import PushButton
from Sliders import ComplexSlider

# Pathnames

DEF_DB = '/usr/local/sounds/aiff'	# Default directory of sounds
SOX = '/usr/local/sox'			# Sound format conversion program
SFPLAY = '/usr/sbin/sfplay'		# Sound playing program


# Global variables

class struct(): pass		# Class to define featureless structures

G = struct()			# Holds writable global variables


# Main program

def main():
	G.synchronous = 0	# If set, use synchronous audio.write()
	G.debug = 0		# If set, print debug messages
	G.busy = 0		# Set while asynchronous playing is active
	G.windows = []		# List of open windows, except control
	G.mode = ''		# File type (default any that sfplay knows)
	G.rate = 0		# Sampling rate (default " " " ")
	G.tempprefix = tempfile.mktemp()
	#
	try:
		optlist, args = getopt.getopt(sys.argv[1:], 'dr:st:')
	except getopt.error, msg:
		sys.stdout = sys.stderr
		print msg
		print 'usage: jukebox [-d] [-s] [-t type] [-r rate]'
		print '  -d        debugging'
		print '  -s        synchronous playing'
		print '  -t type   file type'
		print '  -r rate   sampling rate'
		sys.exit(2)
	#
	for optname, optarg in optlist:
		if   optname == '-d':
			G.debug = 1
		elif optname == '-r':
			G.rate = int(eval(optarg))
		elif optname == '-s':
			G.synchronous = 1
		elif optname == '-t':
			G.mode = optarg
	#
	if not args:
		args = [DEF_DB]
	#
	G.cw = opencontrolwindow()
	for dirname in args:
		G.windows.append(openlistwindow(dirname))
	#
	#
	try:
		maineventloop()
	finally:
		clearcache()
		killchild()

def maineventloop():
	mouse_events = WE_MOUSE_DOWN, WE_MOUSE_MOVE, WE_MOUSE_UP
	while G.windows:
		type, w, detail = event = stdwin.getevent()
		if w == G.cw.win:
			if type == WE_CLOSE:
				return
			if type == WE_TIMER:
				checkchild()
				if G.busy:
					G.cw.win.settimer(1)
			else:
				G.cw.dispatch(event)
		else:
			if type == WE_DRAW:
				w.drawproc(w, detail)
			elif type in mouse_events:
				w.mouse(w, type, detail)
			elif type == WE_CLOSE:
				w.close(w)
				del w, event
			else:
				if G.debug: print type, w, detail

def checkchild():
	if G.busy:
		waitchild(1)

def killchild():
	if G.busy:
		os.kill(G.busy, 9)
		waitchild(0)

def waitchild(options):
	pid, sts = os.wait(G.busy, options)
	if pid == G.busy:
		G.busy = 0
		G.stop.enable(0)


# Control window -- to set gain and cancel play operations in progress

def opencontrolwindow():
	stdwin.setdefscrollbars(0, 0)
	cw = WindowParent().create('Jukebox', (0, 0))
	v = VSplit().create(cw)
	#
	stop = PushButton().definetext(v, 'Stop')
	stop.hook = stop_hook
	stop.enable(0)
	G.stop = stop
	#
	cw.realize()
	return cw

def stop_hook(self):
	killchild()


# List windows -- to display list of files and subdirectories

def openlistwindow(dirname):
	list = os.listdir(dirname)
	list.sort()
	i = 0
	while i < len(list):
		if list[i] == '.' or list[i] == '..':
			del list[i]
		else:
			i = i+1
	for i in range(len(list)):
		name = list[i]
		if os.path.isdir(os.path.join(dirname, name)):
			list[i] = list[i] + '/'
	width = maxwidth(list)
	# width = width + stdwin.textwidth(' ')	# XXX X11 stdwin bug workaround
	height = len(list) * stdwin.lineheight()
	stdwin.setdefwinsize(width, min(height, 500))
	stdwin.setdefscrollbars(0, 1)
	w = stdwin.open(dirname)
	stdwin.setdefwinsize(0, 0)
	w.setdocsize(width, height)
	w.drawproc = drawlistwindow
	w.mouse = mouselistwindow
	w.close = closelistwindow
	w.dirname = dirname
	w.list = list
	w.selected = -1
	return w

def maxwidth(list):
	width = 1
	for name in list:
		w = stdwin.textwidth(name)
		if w > width: width = w
	return width

def drawlistwindow(w, area):
##	(left, top), (right, bottom) = area
	d = w.begindrawing()
	d.erase((0, 0), (1000, 10000))
	lh = d.lineheight()
	h, v = 0, 0
	for name in w.list:
		d.text((h, v), name)
		v = v + lh
	showselection(w, d)
	d.close()

def hideselection(w, d):
	if w.selected >= 0:
		invertselection(w, d)

def showselection(w, d):
	if w.selected >= 0:
		invertselection(w, d)

def invertselection(w, d):
	lh = d.lineheight()
	h1, v1 = p1 = 0, w.selected*lh
	h2, v2 = p2 = 1000, v1 + lh
	d.invert(p1, p2)

def mouselistwindow(w, type, detail):
	(h, v), clicks, button = detail[:3]
	d = w.begindrawing()
	lh = d.lineheight()
	if 0 <= v < lh*len(w.list):
		i = v / lh
	else:
		i = -1
	if w.selected <> i:
		hideselection(w, d)
		w.selected = i
		showselection(w, d)
	d.close()
	if type == WE_MOUSE_DOWN and clicks >= 2 and i >= 0:
		setcursors('watch')
		name = os.path.join(w.dirname, w.list[i])
		if name[-1:] == '/':
			if clicks == 2:
				G.windows.append(openlistwindow(name[:-1]))
		else:
			playfile(name)
		setcursors('cross')

def closelistwindow(w):
	remove(G.windows, w)

def remove(list, item):
	for i in range(len(list)):
		if list[i] == item:
			del list[i]
			break

def setcursors(cursor):
	for w in G.windows:
		w.setwincursor(cursor)
	G.cw.win.setwincursor(cursor)


# Playing tools

cache = {}

def clearcache():
	for x in cache.keys():
		try:
			sts = os.system('rm -f ' + cache[x])
			if sts:
				print cmd
				print 'Exit status', sts
		except:
			print cmd
			print 'Exception?!'
		del cache[x]

def playfile(name):
	killchild()
	if G.mode in ('', 'au', 'aiff'):
		tempname = name
	elif cache.has_key(name):
		tempname = cache[name]
	else:
		tempname = G.tempprefix + `rand.rand()` + '.aiff'
		cmd = SOX
		if G.mode <> '' and G.mode <> 'sox':
			cmd = cmd + ' -t ' + G.mode
		cmd = cmd + ' ' + commands.mkarg(name)
		cmd = cmd + ' -t aiff'
		if G.rate:
			cmd = cmd + ' -r ' + `G.rate`
		cmd = cmd + ' ' + tempname
		if G.debug: print cmd
		sts = os.system(cmd)
		if sts:
			print cmd
			print 'Exit status', sts
			stdwin.fleep()
			return
		cache[name] = tempname
	pid = os.fork()
	if pid == 0:
		# Child
		os.exec(SFPLAY, [SFPLAY, '-r', tempname])
		# NOTREACHED
	# Parent
	if G.synchronous:
		sts = os.wait(pid, 0)
	else:
		G.busy = pid
		G.stop.enable(1)
		G.cw.win.settimer(1)

main()
