#! /usr/local/bin/python

# XXX This only works on SGIs running IRIX 4.0 or higher

# JUKEBOX: browse directories full of sampled sound files.
#
# One or more "list windows" display the files and subdirectories of
# the arguments.  Double-clicking on a subdirectory opens a new window
# displaying its contents (and so on recursively).  Double clicking
# on a file plays it as a sound file (assuming it is one).
#
# Playing is asynchronous: the application keeps listening for events
# while the sample is playing, so you can cancel playing or start a
# new sample right away.  Synchronous playing is available through the
# -s option.
#
# The control window displays a "stop button" that cancel the current
# play request.
#
# Most sound file formats recognized by SOX or SFPLAY are recognized.
# Since conversion is costly, converted files are cached in
# /usr/tmp/@j* until the user quits or changes the sampling rate via
# the Rate menu.

import commands
import getopt
import os
from stat import *
import rand
import stdwin
from stdwinevents import *
import sys
import tempfile
import sndhdr

from WindowParent import WindowParent
from Buttons import PushButton

# Pathnames

DEF_DB = '/usr/local/sounds'		# Default directory of sounds
SOX = '/usr/local/bin/sox'		# Sound format conversion program
SFPLAY = '/usr/sbin/sfplay'		# Sound playing program


# Global variables

class struct: pass		# Class to define featureless structures

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
		print '  -d        debugging (-dd event debugging)'
		print '  -s        synchronous playing'
		print '  -t type   file type'
		print '  -r rate   sampling rate'
		sys.exit(2)
	#
	for optname, optarg in optlist:
		if   optname == '-d':
			G.debug = G.debug + 1
		elif optname == '-r':
			G.rate = int(eval(optarg))
		elif optname == '-s':
			G.synchronous = 1
		elif optname == '-t':
			G.mode = optarg
	#
	if G.debug:
		for name in G.__dict__.keys():
			print 'G.' + name, '=', `G.__dict__[name]`
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

# Entries in Rate menu:
rates = ['default', '7350', \
	'8000', '11025', '16000', '22050', '32000', '41000', '48000']

def maineventloop():
	mouse_events = WE_MOUSE_DOWN, WE_MOUSE_MOVE, WE_MOUSE_UP
	while G.windows:
		try:
			type, w, detail = event = stdwin.getevent()
		except KeyboardInterrupt:
			killchild()
			continue
		if w == G.cw.win:
			if type == WE_CLOSE:
				return
			if type == WE_TIMER:
				checkchild()
				if G.busy:
					G.cw.win.settimer(1)
			elif type == WE_MENU:
				menu, item = detail
				if menu is G.ratemenu:
					clearcache()
					if item == 0:
						G.rate = 0
					else:
						G.rate = eval(rates[item])
					for i in range(len(rates)):
						menu.check(i, (i == item))
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
				if G.debug > 1: print type, w, detail

def checkchild():
	if G.busy:
		waitchild(1)

def killchild():
	if G.busy:
		os.kill(G.busy, 9)
		waitchild(0)

def waitchild(options):
	pid, sts = os.waitpid(G.busy, options)
	if pid == G.busy:
		G.busy = 0
		G.stop.enable(0)


# Control window -- to set gain and cancel play operations in progress

def opencontrolwindow():
	stdwin.setdefscrollbars(0, 0)
	cw = WindowParent().create('Jukebox', (0, 0))
	#
	stop = PushButton().definetext(cw, '        Stop        ')
	stop.hook = stop_hook
	stop.enable(0)
	G.stop = stop
	#
	cw.realize()
	#
	G.ratemenu = cw.win.menucreate('Rate')
	for r in rates:
		G.ratemenu.additem(r)
	if G.rate == 0:
		G.ratemenu.check(0, 1)
	else:
		for i in len(range(rates)):
			if rates[i] == `G.rate`:
				G.ratemenu.check(i, 1)
	#
	return cw

def stop_hook(self):
	killchild()


# List windows -- to display list of files and subdirectories

def openlistwindow(dirname):
	list = os.listdir(dirname)
	list.sort()
	i = 0
	while i < len(list):
		if list[i][0] == '.':
			del list[i]
		else:
			i = i+1
	for i in range(len(list)):
		fullname = os.path.join(dirname, list[i])
		if os.path.isdir(fullname):
			info = '/'
		else:
			try:
				size = os.stat(fullname)[ST_SIZE]
				info = `(size + 1023)/1024` + 'k'
			except IOError:
				info = '???'
			info = '(' + info + ')'
		list[i] = list[i], info
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
	for name, info in list:
		w = stdwin.textwidth(name + '  ' + info)
		if w > width: width = w
	return width

def drawlistwindow(w, area):
##	(left, top), (right, bottom) = area
	d = w.begindrawing()
	d.erase((0, 0), (1000, 10000))
	lh = d.lineheight()
	h, v = 0, 0
	for name, info in w.list:
		if info == '/':
			text = name + '/'
		else:
			text = name + '  ' + info
		d.text((h, v), text)
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
		name, info = w.list[i]
		fullname = os.path.join(w.dirname, name)
		if info == '/':
			if clicks == 2:
				G.windows.append(openlistwindow(fullname))
		else:
			playfile(fullname)
		setcursors('cross')

def closelistwindow(w):
	G.windows.remove(w)

def setcursors(cursor):
	for w in G.windows:
		w.setwincursor(cursor)
	G.cw.win.setwincursor(cursor)


# Playing tools

cache = {}

def clearcache():
	for x in cache.keys():
		cmd = 'rm -f ' + cache[x]
		if G.debug: print cmd
		sts = os.system(cmd)
		if sts:
			print cmd
			print 'Exit status', sts
		del cache[x]

validrates = (8000, 11025, 16000, 22050, 32000, 44100, 48000)

def playfile(filename):
	killchild()
	try:
		tuple = sndhdr.what(filename)
	except IOError, msg:
		print 'Can\'t open', filename, msg
		stdwin.fleep()
		return
	raw = 0
	if tuple:
		mode, rate = tuple[:2]
		if rate == 0:
			rate = G.rate
			if rate == 0:
				rate = 8000
	else:
		mode = G.mode
		rate = G.rate
	if G.debug: print 'mode =', mode, 'rate =', rate
	if mode in ('au', 'aiff', 'wav', 'aifc', 'ul', 'ub', 'sb') and \
		  rate in validrates:
		tempname = filename
		if mode in ('ul', 'ub', 'sb'):
			raw = 1
	elif cache.has_key(filename):
		tempname = cache[filename]
	else:
		tempname = G.tempprefix + `rand.rand()` + '.aiff'
		cmd = SOX
		if G.debug:
			cmd = cmd + ' -V'
		if mode <> '':
			cmd = cmd + ' -t ' + mode
		cmd = cmd + ' ' + commands.mkarg(filename)
		cmd = cmd + ' -t aiff'
		if rate not in validrates:
			rate = 32000
		if rate:
			cmd = cmd + ' -r ' + `rate`
		cmd = cmd + ' ' + tempname
		if G.debug: print cmd
		sts = os.system(cmd)
		if sts:
			print cmd
			print 'Exit status', sts
			stdwin.fleep()
			try:
				os.unlink(tempname)
			except:
				pass
			return
		cache[filename] = tempname
	if raw:
		pid = sfplayraw(tempname, tuple)
	else:
		pid = sfplay(tempname, [])
	if G.synchronous:
		sts = os.wait(pid, 0)
	else:
		G.busy = pid
		G.stop.enable(1)
		G.cw.win.settimer(1)

def sfplayraw(filename, tuple):
	args = ['-i']
	type, rate, channels, frames, bits = tuple
	if type == 'ul':
		args.append('mulaw')
	elif type == 'ub':
		args = args + ['integer', '8', 'unsigned']
	elif type == 'sb':
		args = args + ['integer', '8', '2scomp']
	else:
		print 'sfplayraw: warning: unknown type in', tuple
	if channels > 1:
		args = args + ['channels', `channels`]
	if not rate:
		rate = G.rate
	if rate:
		args = args + ['rate', `rate`]
	args.append('end')
	return sfplay(filename, args)

def sfplay(filename, args):
	if G.debug:
		args = ['-p'] + args
	args = [SFPLAY, '-r'] + args + [filename]
	if G.debug: print 'sfplay:', args
	pid = os.fork()
	if pid == 0:
		# Child
		os.execv(SFPLAY, args)
		# NOTREACHED
	else:
		# Parent
		return pid

main()
