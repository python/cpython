#! /usr/local/python

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

import audio
import sunaudio
import commands
import getopt
import path
import posix
import rand
import stdwin
from stdwinevents import *
import string
import sys

from WindowParent import WindowParent
from HVSplit import VSplit
from Buttons import PushButton
from Sliders import ComplexSlider

# Pathnames

HOME_BIN_SGI = '/ufs/guido/bin/sgi/'	# Directory where macsound2sgi lives
DEF_DB = '/ufs/dik/sounds/Mac/HCOM'	# Default directory of sounds


# Global variables

class struct: pass		# Class to define featureless structures

G = struct()			# Holds writable global variables


# Main program

def main():
	G.synchronous = 0	# If set, use synchronous audio.write()
	G.debug = 0		# If set, print debug messages
	G.gain = 75		# Output gain
	G.rate = 3		# Sampling rate
	G.busy = 0		# Set while asynchronous playing is active
	G.windows = []		# List of open windows (except control)
	G.mode = 'mac'		# Macintosh mode
	G.tempprefix = '/usr/tmp/@j' + `rand.rand()` + '-'
	#
	optlist, args = getopt.getopt(sys.argv[1:], 'dg:r:sSa')
	for optname, optarg in optlist:
		if   optname == '-d':
			G.debug = 1
		elif optname == '-g':
			G.gain = string.atoi(optarg)
			if not (0 < G.gain < 256):
				raise optarg.error, '-g gain out of range'
		elif optname == '-r':
			G.rate = string.atoi(optarg)
			if not (1 <= G.rate <= 3):
				raise optarg.error, '-r rate out of range'
		elif optname == '-s':
			G.synchronous = 1
		elif optname == '-S':
			G.mode = 'sgi'
		elif optname == '-a':
			G.mode = 'sun'
	#
	if not args:
		args = [DEF_DB]
	#
	G.cw = opencontrolwindow()
	for dirname in args:
		G.windows.append(openlistwindow(dirname))
	#
	#
	savegain = audio.getoutgain()
	try:
		# Initialize stdaudio
		audio.setoutgain(0)
		audio.start_playing('')
		dummy = audio.wait_playing()
		audio.setoutgain(0)
		maineventloop()
	finally:
		audio.setoutgain(savegain)
		audio.done()
		clearcache()

def maineventloop():
	mouse_events = WE_MOUSE_DOWN, WE_MOUSE_MOVE, WE_MOUSE_UP
	while G.windows:
		type, w, detail = event = stdwin.getevent()
		if w == G.cw.win:
			if type == WE_CLOSE:
				return
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

# Control window -- to set gain and cancel play operations in progress

def opencontrolwindow():
	cw = WindowParent().create('Jukebox', (0, 0))
	v = VSplit().create(cw)
	#
	gain = ComplexSlider().define(v)
	gain.setminvalmax(0, G.gain, 255)
	gain.settexts('  ', '  ')
	gain.sethook(gain_setval_hook)
	#
	stop = PushButton().definetext(v, 'Stop')
	stop.hook = stop_hook
	#
	cw.realize()
	return cw

def gain_setval_hook(self):
	G.gain = self.val
	if G.busy: audio.setoutgain(G.gain)

def stop_hook(self):
	if G.busy:
		audio.setoutgain(0)
		dummy = audio.stop_playing()
		G.busy = 0


# List windows -- to display list of files and subdirectories

def openlistwindow(dirname):
	list = posix.listdir(dirname)
	list.sort()
	i = 0
	while i < len(list):
		if list[i] == '.' or list[i] == '..':
			del list[i]
		else:
			i = i+1
	for i in range(len(list)):
		name = list[i]
		if path.isdir(path.join(dirname, name)):
			list[i] = list[i] + '/'
	width = maxwidth(list)
	width = width + stdwin.textwidth(' ')	# XXX X11 stdwin bug workaround
	height = len(list) * stdwin.lineheight()
	stdwin.setdefwinsize(width, min(height, 500))
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
	d = w.begindrawing()
	d.erase((0, 0), (1000, 10000))
	lh = d.lineheight()
	h, v = 0, 0
	for name in w.list:
		d.text((h, v), name)
		v = v + lh
	showselection(w, d)

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
	if type == WE_MOUSE_DOWN and clicks >= 2 and i >= 0:
		name = path.join(w.dirname, w.list[i])
		if name[-1:] == '/':
			if clicks == 2:
				G.windows.append(openlistwindow(name[:-1]))
		else:
			playfile(name)

def closelistwindow(w):
	remove(G.windows, w)

def remove(list, item):
	for i in range(len(list)):
		if list[i] == item:
			del list[i]
			break


# Playing tools

cache = {}

def clearcache():
	for x in cache.keys():
		try:
			sts = posix.system('rm -f ' + cache[x])
			if sts:
				print cmd
				print 'Exit status', sts
		except:
			print cmd
			print 'Exception?!'
		del cache[x]

def playfile(name):
	if G.mode <> 'mac':
		tempname = name
	elif cache.has_key(name):
		tempname = cache[name]
	else:
		tempname = G.tempprefix + `rand.rand()`
		cmd = HOME_BIN_SGI + 'macsound2sgi'
		cmd = cmd + ' ' + commands.mkarg(name)
		cmd = cmd + ' >' + tempname
		if G.debug: print cmd
		sts = posix.system(cmd)
		if sts:
			print cmd
			print 'Exit status', sts
			stdwin.fleep()
			return
		cache[name] = tempname
	fp = open(tempname, 'r')
	try:
		hdr = sunaudio.gethdr(fp)
	except sunaudio.error, msg:
		hdr = ()
	if hdr:
		data_size = hdr[0]
		data = fp.read(data_size)
		# XXX this doesn't work yet, need to convert from uLAW!!!
		del fp
	else:
		del fp
		data = readfile(tempname)
	if G.debug: print len(data), 'bytes read from', tempname
	if G.busy:
		G.busy = 0
		dummy = audio.stop_playing()
	#
	# Completely reset the audio device
	audio.setrate(G.rate)
	audio.setduration(0)
	audio.setoutgain(G.gain)
	#
	if G.synchronous:
		audio.write(data)
		audio.setoutgain(0)
	else:
		try:
			audio.start_playing(data)
			G.busy = 1
		except:
			stdwin.fleep()
	del data

def readfile(filename):
	return readfp(open(filename, 'r'))

def readfp(fp):
	data = ''
	while 1:
		buf = fp.read(102400) # Reads most samples in one fell swoop
		if not buf:
			return data
		data = data + buf

main()
