import Dlg
import Res

splash = Dlg.GetNewDialog(468, -1)
splash.DrawDialog()

import Qd, TE, Fm, sys

_real__import__ = None

def install_importhook():
	global _real__import__
	import __builtin__
	if _real__import__ is None:
		_real__import__ = __builtin__.__import__
		__builtin__.__import__ = my__import__

def uninstall_importhook():
	global _real__import__
	if _real__import__ is not None:
		import __builtin__
		__builtin__.__import__ = _real__import__
		_real__import__ = None

_progress = 0

def importing(module):
	global _progress
	Qd.SetPort(splash)
	fontID = Fm.GetFNum("Python-Sans")
	if not fontID:
		fontID = geneva
	Qd.TextFont(fontID)
	Qd.TextSize(9)
	rect = (35, 260, 365, 276)
	if module:
		TE.TETextBox('Importing: ' + module, rect, 0)
		if not _progress:
			Qd.FrameRect((35, 276, 365, 284))
		pos = min(36 + 330 * _progress / 44, 364)
		Qd.PaintRect((36, 277, pos, 283))
		_progress = _progress + 1
	else:
		Qd.EraseRect(rect)
		Qd.PaintRect((36, 277, pos, 283))

def my__import__(name, globals=None, locals=None, fromlist=None):
	try:
		return sys.modules[name]
	except KeyError:
		try:
			importing(name)
		except:
			try:
				rv = _real__import__(name)
			finally:
				uninstall_importhook()
			return rv
		return _real__import__(name)

install_importhook()

kHighLevelEvent = 23
import Win
from Fonts import *
from QuickDraw import *
from TextEdit import *
import string
import sys
import random

_keepsplashscreenopen = 0

abouttext1 = """The Python Integrated Developement Environment for the MacintoshÅ
Version: %s
Copyright 1997 Just van Rossum, Letterror. <just@knoware.nl>

Python %s
%s
Written by Guido van Rossum with Jack Jansen (and others)

See: <http://www.python.org/> for information and documentation."""

flauwekul = [	'Goodday, Bruce.', 
			'Whatπs new?', 
			'Nudge, nudge, say no more!', 
			'No, no sir, itπs not dead. Itπs resting.',
			'Albatros!',
			'Itπs . . .',
			'Is your name not Bruce, then?',
			"""But Mr F.G. Superman has a secret identity . . . 
when trouble strikes at any time . . . 
at any place . . . he is ready to become . . . 
Bicycle Repair Man!"""
			]

def nl2return(text):
	return string.join(string.split(text, '\n'), '\r')

def UpdateSplash(drawdialog = 0, what = 0):
	if drawdialog:
		splash.DrawDialog()
	drawtext(what)
	Win.ValidRect(splash.GetWindowPort().portRect)

def drawtext(what = 0):
	Qd.SetPort(splash)
	fontID = Fm.GetFNum("Python-Sans")
	if not fontID:
		fontID = geneva
	Qd.TextFont(fontID)
	Qd.TextSize(9)
	rect = (10, 125, 390, 260)
	if not what:
		import __main__
		abouttxt = nl2return(abouttext1  \
			% (__main__.__version__, sys.version, sys.copyright))
	else:
		abouttxt = nl2return(random.choice(flauwekul))
	TE.TETextBox(abouttxt, rect, teJustCenter)

UpdateSplash(1)

def wait():
	import Evt
	from Events import *
	global splash
	try:
		splash
	except NameError:
		return
	Qd.InitCursor()
	time = Evt.TickCount()
	whattext = 0
	while _keepsplashscreenopen:
		ok, event = Evt.EventAvail(highLevelEventMask)
		if ok:
			# got apple event, back to mainloop
			break
		ok, event = Evt.EventAvail(mDownMask | keyDownMask | updateMask)
		if ok:
			ok, event = Evt.WaitNextEvent(mDownMask | keyDownMask | updateMask, 30)
			if ok:
				(what, message, when, where, modifiers) = event
				if what == updateEvt:
					if Win.WhichWindow(message) == splash:
						UpdateSplash(1, whattext)
				else:
					break
		if Evt.TickCount() - time > 360:
			whattext = not whattext
			drawtext(whattext)
			time = Evt.TickCount()
	del splash
	#Res.CloseResFile(splashresfile)

def about():
	global splash, splashresfile, _keepsplashscreenopen
	_keepsplashscreenopen = 1
	splash = Dlg.GetNewDialog(468, -1)
	splash.DrawDialog()
	wait()
