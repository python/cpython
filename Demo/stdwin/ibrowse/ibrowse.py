# Browser for "Info files" as used by the Emacs documentation system.
#
# Now you can read Info files even if you can't spare the memory, time or
# disk space to run Emacs.  (I have used this extensively on a Macintosh
# with 1 Megabyte main memory and a 20 Meg harddisk.)
#
# You can give this to someone with great fear of complex computer
# systems, as long as they can use a mouse.
#
# Another reason to use this is to encourage the use of Info for on-line
# documentation of software that is not related to Emacs or GNU.
# (In particular, I plan to redo the Python and STDWIN documentation
# in texinfo.)


# NB: this is not a self-executing script.  You must startup Python,
# import ibrowse, and call ibrowse.main().  On UNIX, the script 'ib'
# runs the browser.


# Configuration:
#
# - The pathname of the directory (or directories) containing
#   the standard Info files should be set by editing the
#   value assigned to INFOPATH in module ifile.py.
#
# - The default font should be set by editing the value of FONT
#   in this module (ibrowse.py).
#
# - For fastest I/O, you may look at BLOCKSIZE and a few other
#   constants in ifile.py.


# This is a fairly large Python program, split in the following modules:
#
# ibrowse.py	Main program and user interface.
#		This is the only module that imports stdwin.
#
# ifile.py	This module knows about the format of Info files.
#		It is imported by all of the others.
#
# itags.py	This module knows how to read prebuilt tag tables,
#		including indirect ones used by large texinfo files.
#
# icache.py	Caches tag tables and visited nodes.


# XXX There should really be a different tutorial, as the user interface
# XXX differs considerably from Emacs...


import sys
import regexp
import stdwin
from stdwinevents import *
import string
from ifile import NoSuchFile, NoSuchNode
import icache


# Default font.
# This should be an acceptable argument for stdwin.setfont();
# on the Mac, this can be a pair (fontname, pointsize), while
# under X11 it should be a standard X11 font name.
# For best results, use a constant width font like Courier;
# many Info files contain tabs that don't align with other text
# unless all characters have the same width.
#
#FONT = ('Monaco', 9)		# Mac
FONT = '-schumacher-clean-medium-r-normal--14-140-75-75-c-70-iso8859-1'	# X11


# Try not to destroy the list of windows when reload() is used.
# This is useful during debugging, and harmless in production...
#
try:
	dummy = windows
	del dummy
except NameError:
	windows = []


# Default main function -- start at the '(dir)' node.
#
def main():
	start('(dir)')


# Start at an arbitrary node.
# The default file is 'ibrowse'.
#
def start(ref):
	stdwin.setdefscrollbars(0, 1)
	stdwin.setfont(FONT)
	stdwin.setdefwinsize(76*stdwin.textwidth('x'), 22*stdwin.lineheight())
	makewindow('ibrowse', ref)
	mainloop()


# Open a new browser window.
# Arguments specify the default file and a node reference
# (if the node reference specifies a file, the default file is ignored).
#
def makewindow(file, ref):
	win = stdwin.open('Info file Browser, by Guido van Rossum')
	win.mainmenu = makemainmenu(win)
	win.navimenu = makenavimenu(win)
	win.textobj = win.textcreate((0, 0), win.getwinsize())
	win.file = file
	win.node = ''
	win.last = []
	win.pat = ''
	win.dispatch = idispatch
	windows.append(win)
	imove(win, ref)

# Create the 'Ibrowse' menu for a new browser window.
#
def makemainmenu(win):
	mp = win.menucreate('Ibrowse')
	mp.callback = []
	additem(mp, 'New window (clone)',	'K', iclone)
	additem(mp, 'Help (tutorial)',		'H', itutor)
	additem(mp, 'Command summary',		'?', isummary)
	additem(mp, 'Close this window',	'W', iclose)
	additem(mp, '', '', None)
	additem(mp, 'Copy to clipboard',	'C', icopy)
	additem(mp, '', '', None)
	additem(mp, 'Search regexp...',		'S', isearch)
	additem(mp, '', '', None)
	additem(mp, 'Reset node cache',		'',  iresetnodecache)
	additem(mp, 'Reset entire cache',	'',  iresetcache)
	additem(mp, '', '', None)
	additem(mp, 'Quit',			'Q', iquit)
	return mp

# Create the 'Navigation' menu for a new browser window.
#
def makenavimenu(win):
	mp = win.menucreate('Navigation')
	mp.callback = []
	additem(mp, 'Menu item...',		'M', imenu)
	additem(mp, 'Follow reference...',	'F', ifollow)
	additem(mp, 'Go to node...',		'G', igoto)
	additem(mp, '', '', None)
	additem(mp, 'Next node in tree',	'N', inext)
	additem(mp, 'Previous node in tree',	'P', iprev)
	additem(mp, 'Up in tree',		'U', iup)
	additem(mp, 'Last visited node',	'L', ilast)
	additem(mp, 'Top of tree',		'T', itop)
	additem(mp, 'Directory node',		'D', idir)
	return mp

# Add an item to a menu, and a function to its list of callbacks.
# (Specifying all in one call is the only way to keep the menu
# and the list of callbacks in synchrony.)
#
def additem(mp, text, shortcut, function):
	if shortcut:
		mp.additem(text, shortcut)
	else:
		mp.additem(text)
	mp.callback.append(function)


# Stdwin event processing main loop.
# Return when there are no windows left.
# Note that windows not in the windows list don't get their events.
#
def mainloop():
	while windows:
		event = stdwin.getevent()
		if event[1] in windows:
			try:
				event[1].dispatch(event)
			except KeyboardInterrupt:
				# The user can type Control-C (or whatever)
				# to leave the browser without closing
				# the window.  Mainly useful for
				# debugging.
				break
			except:
				# During debugging, it was annoying if
				# every mistake in a callback caused the
				# whole browser to crash, hence this
				# handler.  In a production version
				# it may be better to disable this.
				#
				msg = sys.exc_type
				if sys.exc_value:
					val = sys.exc_value
					if type(val) <> type(''):
						val = `val`
					msg = msg + ': ' + val
				msg = 'Oops, an exception occurred: ' + msg
				event = None
				stdwin.message(msg)
		event = None


# Handle one event.  The window is taken from the event's window item.
# This function is placed as a method (named 'dispatch') on the window,
# so the main loop will be able to handle windows of a different kind
# as well, as long as they are all placed in the list of windows.
#
def idispatch(event):
	type, win, detail = event
	if type == WE_CHAR:
		if not keybindings.has_key(detail):
			detail = string.lower(detail)
		if keybindings.has_key(detail):
			keybindings[detail](win)
			return
		if detail in '0123456789':
			i = eval(detail) - 1
			if 0 <= i < len(win.menu):
				topic, ref = win.menu[i]
				imove(win, ref)
				return
		stdwin.fleep()
		return
	if type == WE_COMMAND:
		if detail == WC_LEFT:
			iprev(win)
		elif detail == WC_RIGHT:
			inext(win)
		elif detail == WC_UP:
			iup(win)
		elif detail == WC_DOWN:
			idown(win)
		elif detail == WC_BACKSPACE:
			ibackward(win)
		elif detail == WC_RETURN:
			idown(win)
		else:
			stdwin.fleep()
		return
	if type == WE_MENU:
		mp, item = detail
		if mp == None:
			pass # A THINK C console menu was selected
		elif mp in (win.mainmenu, win.navimenu):
			mp.callback[item](win)
		elif mp == win.nodemenu:
			topic, ref = win.menu[item]
			imove(win, ref)
		elif mp == win.footmenu:
			topic, ref = win.footnotes[item]
			imove(win, ref)
		return
	if type == WE_SIZE:
		win.textobj.move((0, 0), win.getwinsize())
		(left, top), (right, bottom) = win.textobj.getrect()
		win.setdocsize(0, bottom)
		return
	if type == WE_CLOSE:
		iclose(win)
		return
	if not win.textobj.event(event):
		pass


# Paging callbacks

def ibeginning(win):
	win.setorigin(0, 0)
	win.textobj.setfocus(0, 0) # To restart searches

def iforward(win):
	lh = stdwin.lineheight() # XXX Should really use the window's...
	h, v = win.getorigin()
	docwidth, docheight = win.getdocsize()
	width, height = win.getwinsize()
	if v + height >= docheight:
		stdwin.fleep()
		return
	increment = max(lh, ((height - 2*lh) / lh) * lh)
	v = v + increment
	win.setorigin(h, v)

def ibackward(win):
	lh = stdwin.lineheight() # XXX Should really use the window's...
	h, v = win.getorigin()
	if v <= 0:
		stdwin.fleep()
		return
	width, height = win.getwinsize()
	increment = max(lh, ((height - 2*lh) / lh) * lh)
	v = max(0, v - increment)
	win.setorigin(h, v)


# Ibrowse menu callbacks

def iclone(win):
	stdwin.setdefwinsize(win.getwinsize())
	makewindow(win.file, win.node)

def itutor(win):
	# The course looks best at 76x22...
	stdwin.setdefwinsize(76*stdwin.textwidth('x'), 22*stdwin.lineheight())
	makewindow('ibrowse', 'Help')

def isummary(win):
	stdwin.setdefwinsize(76*stdwin.textwidth('x'), 22*stdwin.lineheight())
	makewindow('ibrowse', 'Summary')

def iclose(win):
	#
	# Remove the window from the windows list so the mainloop
	# will notice if all windows are gone.
	# Delete the textobj since it constitutes a circular reference
	# to the window which would prevent it from being closed.
	# (Deletion is done by assigning None to avoid crashes
	# when closing a half-initialized window.)
	#
	if win in windows:
		windows.remove(win)
	win.textobj = None

def icopy(win):
	focustext = win.textobj.getfocustext()
	if not focustext:
		stdwin.fleep()
	else:
		stdwin.rotatecutbuffers(1)
		stdwin.setcutbuffer(0, focustext)
		# XXX Should also set the primary selection...

def isearch(win):
	try:
		pat = stdwin.askstr('Search pattern:', win.pat)
	except KeyboardInterrupt:
		return
	if not pat:
		pat = win.pat
		if not pat:
			stdwin.message('No previous pattern')
			return
	try:
		cpat = regexp.compile(pat)
	except regexp.error, msg:
		stdwin.message('Bad pattern: ' + msg)
		return
	win.pat = pat
	f1, f2 = win.textobj.getfocus()
	text = win.text
	match = cpat.match(text, f2)
	if not match:
		stdwin.fleep()
		return
	a, b = match[0]
	win.textobj.setfocus(a, b)


def iresetnodecache(win):
	icache.resetnodecache()

def iresetcache(win):
	icache.resetcache()

def iquit(win):
	for win in windows[:]:
		iclose(win)


# Navigation menu callbacks

def imenu(win):
	ichoice(win, 'Menu item (abbreviated):', win.menu, whichmenuitem(win))

def ifollow(win):
	ichoice(win, 'Follow reference named (abbreviated):', \
		win.footnotes, whichfootnote(win))

def igoto(win):
	try:
		choice = stdwin.askstr('Go to node (full name):', '')
	except KeyboardInterrupt:
		return
	if not choice:
		stdwin.message('Sorry, Go to has no default')
		return
	imove(win, choice)

def inext(win):
	prev, next, up = win.header
	if next:
		imove(win, next)
	else:
		stdwin.fleep()

def iprev(win):
	prev, next, up = win.header
	if prev:
		imove(win, prev)
	else:
		stdwin.fleep()

def iup(win):
	prev, next, up = win.header
	if up:
		imove(win, up)
	else:
		stdwin.fleep()

def ilast(win):
	if not win.last:
		stdwin.fleep()
	else:
		i = len(win.last)-1
		lastnode, lastfocus = win.last[i]
		imove(win, lastnode)
		if len(win.last) > i+1:
			# The move succeeded -- restore the focus
			win.textobj.setfocus(lastfocus)
		# Delete the stack top even if the move failed,
		# else the whole stack would remain unreachable
		del win.last[i:] # Delete the entry pushed by imove as well!

def itop(win):
	imove(win, '')

def idir(win):
	imove(win, '(dir)')


# Special and generic callbacks

def idown(win):
	if win.menu:
		default = whichmenuitem(win)
		for topic, ref in win.menu:
			if default == topic:
				break
		else:
			topic, ref = win.menu[0]
		imove(win, ref)
	else:
		inext(win)

def ichoice(win, prompt, list, default):
	if not list:
		stdwin.fleep()
		return
	if not default:
		topic, ref = list[0]
		default = topic
	try:
		choice = stdwin.askstr(prompt, default)
	except KeyboardInterrupt:
		return
	if not choice:
		return
	choice = string.lower(choice)
	n = len(choice)
	for topic, ref in list:
		topic = string.lower(topic)
		if topic[:n] == choice:
			imove(win, ref)
			return
	stdwin.message('Sorry, no topic matches ' + `choice`)


# Follow a reference, in the same window.
#
def imove(win, ref):
	savetitle = win.gettitle()
	win.settitle('Looking for ' + ref + '...')
	#
	try:
		file, node, header, menu, footnotes, text = \
			icache.get_node(win.file, ref)
	except NoSuchFile, file:
		win.settitle(savetitle)
		stdwin.message(\
		'Sorry, I can\'t find a file named ' + `file` + '.')
		return
	except NoSuchNode, node:
		win.settitle(savetitle)
		stdwin.message(\
		'Sorry, I can\'t find a node named ' + `node` + '.')
		return
	#
	win.settitle('Found (' + file + ')' + node + '...')
	#
	if win.file and win.node:
		lastnode = '(' + win.file + ')' + win.node
		win.last.append(lastnode, win.textobj.getfocus())
	win.file = file
	win.node = node
	win.header = header
	win.menu = menu
	win.footnotes = footnotes
	win.text = text
	#
	win.setorigin(0, 0) # Scroll to the beginnning
	win.textobj.settext(text)
	win.textobj.setfocus(0, 0)
	(left, top), (right, bottom) = win.textobj.getrect()
	win.setdocsize(0, bottom)
	#
	win.footmenu = None
	win.nodemenu = None
	#
	win.menu = menu
	if menu:
		win.nodemenu = win.menucreate('Menu')
		digit = 1
		for topic, ref in menu:
			if digit < 10:
				win.nodemenu.additem(topic, `digit`)
			else:
				win.nodemenu.additem(topic)
			digit = digit + 1
	#
	win.footnotes = footnotes
	if footnotes:
		win.footmenu = win.menucreate('Footnotes')
		for topic, ref in footnotes:
			win.footmenu.additem(topic)
	#
	win.settitle('(' + win.file + ')' + win.node)


# Find menu item at focus
#
findmenu = regexp.compile('^\* [mM]enu:').match
findmenuitem = regexp.compile( \
	'^\* ([^:]+):[ \t]*(:|\([^\t]*\)[^\t,\n.]*|[^:(][^\t,\n.]*)').match
#
def whichmenuitem(win):
	if not win.menu:
		return ''
	match = findmenu(win.text)
	if not match:
		return ''
	a, b = match[0]
	i = b
	f1, f2 = win.textobj.getfocus()
	lastmatch = ''
	while i < len(win.text):
		match = findmenuitem(win.text, i)
		if not match:
			break
		(a, b), (a1, b1), (a2, b2) = match
		if a > f1:
			break
		lastmatch = win.text[a1:b1]
		i = b
	return lastmatch


# Find footnote at focus
#
findfootnote = \
	regexp.compile('\*[nN]ote ([^:]+):[ \t]*(:|[^:][^\t,\n.]*)').match
#
def whichfootnote(win):
	if not win.footnotes:
		return ''
	i = 0
	f1, f2 = win.textobj.getfocus()
	lastmatch = ''
	while i < len(win.text):
		match = findfootnote(win.text, i)
		if not match:
			break
		(a, b), (a1, b1), (a2, b2) = match
		if a > f1:
			break
		lastmatch = win.text[a1:b1]
		i = b
	return lastmatch


# Now all the "methods" are defined, we can initialize the table
# of key bindings.
#
keybindings = {}

# Window commands

keybindings['k'] = iclone
keybindings['h'] = itutor
keybindings['?'] = isummary
keybindings['w'] = iclose

keybindings['c'] = icopy

keybindings['s'] = isearch

keybindings['q'] = iquit

# Navigation commands

keybindings['m'] = imenu
keybindings['f'] = ifollow
keybindings['g'] = igoto

keybindings['n'] = inext
keybindings['p'] = iprev
keybindings['u'] = iup
keybindings['l'] = ilast
keybindings['d'] = idir
keybindings['t'] = itop

# Paging commands

keybindings['b'] = ibeginning
keybindings['.'] = ibeginning
keybindings[' '] = iforward
