# GL STDWIN
#
# See stdwingl for a convenient hack to use this instead of built-in stdwin
# without modifying your application, except for one line in the main file.
#
# Intrinsic differences with built-in stdwin (hard or impossible to fix):
# - Need to call w.close() to close a window !!!
# - Need to call m.close() to remove a menu !!!
# - Doesn't enforce the existence of at most one drawing object
# - No textedit package
# - No X11 selections
#
# Not yet implemented:
# - shade drawing
# - elliptical arc drawing (need to play with transformation)
# - more than one mouse button
# - scroll bars (need to redo viewport handling to get this)
# - partial redraws
# - dialog boxes
# - timer events
# - cursors
#
# Extra features:
# - color (for now, you need to know the colormap index)


import gl
import fm
from GL import *
from DEVICE import *
from stdwinevents import *


# Customizable constants
#
DEF_FONT = 'Times-Roman'		# Default font
DEF_SIZE = 12				# Default font size (points)
MASK = 20				# Viewport minus scrmask


# A structure to hold global variables
#
class Struct: pass
G = Struct()
#
G.queue = []				# Pending STDWIN events
G.drawqueue = []			# Windows that need WE_REDRAW
G.windowmap = {}			# Map window id to window object
G.windowmap['0'] = None			# For convenience
G.focus = None				# Input focus
G.fg = BLACK				# Foreground color
G.bg = WHITE				# Background color
G.def_size = 0, 0			# Default window size
G.def_pos = 0, 0			# Default window position
#
G.size = DEF_SIZE
G.font = fm.findfont(DEF_FONT).scalefont(G.size)


# Initialize GL
#
gl.foreground()
gl.noport()
dummygid = gl.winopen('')

# Ask for all sorts of events
#
# Both REDRAW (= resize and/or redraw!) and INPUTCHANGE are implicitly queued
#qdevice(REDRAW)
#qdevice(INPUTCHANGE)
#
# Keyboard
gl.qdevice(KEYBD)
gl.qdevice(LEFTARROWKEY)
gl.qdevice(RIGHTARROWKEY)
gl.qdevice(UPARROWKEY)
gl.qdevice(DOWNARROWKEY)
gl.qdevice(LEFTALTKEY)
gl.qdevice(RIGHTALTKEY)
#
# Mouse
gl.qdevice(LEFTMOUSE)
#gl.qdevice(MIDDLEMOUSE)
gl.qdevice(RIGHTMOUSE)			# Menu button
# NB MOUSEX, MOUSEY events are queued on button down
#
# Window close requests
gl.qdevice(WINQUIT)
gl.qdevice(WINSHUT)
#
# These aren't needed
#gl.qdevice(TIMER0)
#gl.qdevice(WINFREEZE)
#gl.qdevice(WINTHAW)
#gl.qdevice(REDRAWICONIC)


# STDWIN: create a new window
#
def open(title):
	h, v = G.def_pos
	width, height = G.def_size
	if h > 0 or v > 0:
		# Choose arbitrary defaults
		if h < 0: h = 10
		if v < 0: v = 30
		if width <= 0: width = 400
		if height <= 0: height = 300
		gl.prefposition(h, h+width, 1024-v, 1024-v-height)
	elif width > 0 or height > 0:
		if width <= 0: width = 400
		if height <= 0: height = 300
		gl.prefsize(width, height)
	from glstdwwin import WindowObject
	win = WindowObject()._init(title)
	G.windowmap[`win._gid`] = win
	return win


# STDWIN: set default initial window position (0 means use default)
#
def setdefwinpos(h, v):
	G.def_pos = h, v


# STDWIN: set default window size (0 means use default)
#
def setdefwinsize(width, height):
	G.def_size = width, height


# STDWIN: beep or ring the bell
#
def fleep():
	gl.ringbell()


# STDWIN: set default foreground color
#
def setfgcolor(color):
	G.fg = color


# STDWIN: set default background color
#
def setbgcolor(color):
	G.bg = color


# STDWIN: get default foreground color
#
def getfgcolor():
	return G.fgcolor


# STDWIN: get default background color
#
def getbgcolor():
	return G.bgcolor


# Table mapping characters to key codes
#
key2code = key = {}
key['A'] = AKEY
key['B'] = BKEY
key['C'] = CKEY
key['D'] = DKEY
key['E'] = EKEY
key['F'] = FKEY
key['G'] = GKEY
key['H'] = HKEY
key['I'] = IKEY
key['J'] = JKEY
key['K'] = KKEY
key['L'] = LKEY
key['M'] = MKEY
key['N'] = NKEY
key['O'] = OKEY
key['P'] = PKEY
key['Q'] = QKEY
key['R'] = RKEY
key['S'] = SKEY
key['T'] = TKEY
key['U'] = UKEY
key['V'] = VKEY
key['W'] = WKEY
key['X'] = XKEY
key['Y'] = YKEY
key['Z'] = ZKEY
key['0'] = ZEROKEY
key['1'] = ONEKEY
key['2'] = TWOKEY
key['3'] = THREEKEY
key['4'] = FOURKEY
key['5'] = FIVEKEY
key['6'] = SIXKEY
key['7'] = SEVENKEY
key['8'] = EIGHTKEY
key['9'] = NINEKEY
del key
#
code2key = {}
codelist = []
for key in key2code.keys():
	code = key2code[key]
	code2key[`code`] = key
	codelist.append(code)
del key


# STDWIN: wait for the next event
#
commands = {}
commands['\r'] = WC_RETURN
commands['\b'] = WC_BACKSPACE
commands['\t'] = WC_TAB
#
def getevent():
	while 1:
		#
		# Get next event from the processed queue, if any
		#
		if G.queue:
			event = G.queue[0]
			del G.queue[0]
			#print 'getevent from queue -->', event
			return event
		#
		# Get next event from the draw queue, if any,
		# but only if there is nothing in the system queue.
		#
		if G.drawqueue and not gl.qtest():
			win = G.drawqueue[0]
			del G.drawqueue[0]
			gl.winset(win._gid)
			gl.color(win._bg)
			gl.clear()
			event = WE_DRAW, win, win._area
			#print 'getevent from drawqueue -->', event
			return event
		#
		# Get next event from system queue, blocking if necessary
		# until one is available.
		# Some cases immediately return the event, others do nothing
		# or append one or more events to the processed queue.
		#
		dev, val = gl.qread()
		#
		if dev == REDRAW:
			win = G.windowmap[`val`]
			old_area = win._area
			win._fixviewport()
			win._needredraw()
			if old_area <> win._area:
				#print 'getevent --> WE_SIZE'
				return WE_SIZE, win, None
		elif dev == KEYBD:
			if val == 3:
				raise KeyboardInterrupt # Control-C in window
			character = chr(val)
			if commands.has_key(character):
				return WE_COMMAND, G.focus, commands[character]
			return WE_CHAR, G.focus, character
		elif dev == LEFTARROWKEY:
			if val:
				return WE_COMMAND, G.focus, WC_LEFT
		elif dev == RIGHTARROWKEY:
			if val:
				return WE_COMMAND, G.focus, WC_RIGHT
		elif dev == UPARROWKEY:
			if val:
				return WE_COMMAND, G.focus, WC_UP
		elif dev == DOWNARROWKEY:
			if val:
				return WE_COMMAND, G.focus, WC_DOWN
		elif dev in (LEFTALTKEY, RIGHTALTKEY):
			if val:
				for code in codelist:
					gl.qdevice(code)
			else:
				for code in codelist:
					gl.unqdevice(code)
		elif dev in codelist:
			if val:
				event = G.focus._doshortcut(code2key[`dev`])
				if event:
					return event
		elif dev == LEFTMOUSE:
			G.mousex = gl.getvaluator(MOUSEX)
			G.mousey = gl.getvaluator(MOUSEY)
			if val:
				type = WE_MOUSE_DOWN
				gl.qdevice(MOUSEX)
				gl.qdevice(MOUSEY)
			else:
				type = WE_MOUSE_UP
				gl.unqdevice(MOUSEX)
				gl.unqdevice(MOUSEY)
			return _mouseevent(type)
		elif dev == MOUSEX:
			G.mousex = val
			return _mouseevent(WE_MOUSE_MOVE)
		elif dev == MOUSEY:
			G.mousey = val
			return _mouseevent(WE_MOUSE_MOVE)
		elif dev == RIGHTMOUSE:		# Menu button press/release
			if val:			# Press
				event = G.focus._domenu()
				if event:
					return event
		elif dev == INPUTCHANGE:
			if G.focus:
				G.queue.append(WE_DEACTIVATE, G.focus, None)
			G.focus = G.windowmap[`val`]
			if G.focus:
				G.queue.append(WE_ACTIVATE, G.focus, None)
		elif dev in (WINSHUT, WINQUIT):
			return WE_CLOSE, G.windowmap[`val`], None
		else:
			print '*** qread() --> dev:', dev, 'val:', val

# Helper routine to construct a mouse (up, move or down) event
#
def _mouseevent(type):
	gl.winset(G.focus._gid)
	orgx, orgy = gl.getorigin()
	sizex, sizey = gl.getsize()
	x = G.mousex - orgx
	y = G.mousey - orgy
	return type, G.focus, ((x, sizey-y), 1, 0, 0)




# STDWIN: text measuring functions

def baseline():
	(printermatched, fixed_width, xorig, yorig, xsize, ysize, \
		height, nglyphs) = G.font.getfontinfo()
	return height - yorig

def lineheight():
	(printermatched, fixed_width, xorig, yorig, xsize, ysize, \
			height, nglyphs) = G.font.getfontinfo()
	return height

def textbreak(string, width):
	# XXX Slooooow!
	n = len(string)
	nwidth = textwidth(string[:n])
	while nwidth > width:
		n = n-1
		nwidth = textwidth(string[:n])
	return n

def textwidth(string):
	return G.font.getstrwidth(string)


# STDWIN: set default font and size

def setfont(fontname):
	G.font = fm.findfont(fontname).scalefont(G.size)

def setsize(size):
	ratio = float(size) / float(G.size)
	G.size = size
	G.font = G.font.scalefont(ratio)


# Utility functions

# Exclusive-or of two BYTES
#
def xor(x, y):
	a = bits(x)
	b = bits(y)
	c = [0, 0, 0, 0, 0, 0, 0, 0]
	for i in range(8):
		c[i] = (a[i] + b[i]) % 2
	return stib(c)

# Return the bits of a byte as a list of 8 integers
#
def bits(x):
	b = [0, 0, 0, 0, 0, 0, 0, 0]
	for i in range(8):
		x, b[i] = divmod(x, 2)
	return b

# Convert a list of 8 integers (0|1) to a byte
#
def stib(b):
	x = 0
	shift = 1
	for i in range(8):
		x = x + b[i]*shift
		shift = shift*2
	return x
