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

_about_width = 440
_about_height = 340

def importing(module):
	global _progress
	Qd.SetPort(splash)
	fontID = Fm.GetFNum("Python-Sans")
	if not fontID:
		fontID = geneva
	Qd.TextFont(fontID)
	Qd.TextSize(9)
	labelrect = (35, _about_height - 35, _about_width - 35, _about_height - 19)
	framerect = (35, _about_height - 19, _about_width - 35, _about_height - 11)
	l, t, r, b = progrect = Qd.InsetRect(framerect, 1, 1)
	if module:
		TE.TETextBox('Importing: ' + module, labelrect, 0)
		if not _progress:
			Qd.FrameRect(framerect)
		pos = min(r, l + ((r - l) * _progress) / 44)
		Qd.PaintRect((l, t, pos, b))
		_progress = _progress + 1
	else:
		Qd.EraseRect(labelrect)
		Qd.PaintRect((l, t, pos, b))
	Qd.QDFlushPortBuffer(splash.GetDialogWindow().GetWindowPort(), None)

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

_keepsplashscreenopen = 0

abouttext1 = """The Python Integrated Development Environment for the Macintosh\xaa
Version: %s
Copyright 1997-2001 Just van Rossum, Letterror. <just@letterror.com>
Python %s
%s
See: <http://www.python.org/> for information and documentation."""

flauwekul = [	"Goodday, Bruce.", 
			"What's new?",
			"Nudge, nudge, say no more!", 
			"No, no sir, it's not dead. It's resting.",
			"Albatros!",
			"It's . . .",
			"Is your name not Bruce, then?",
			"""But Mr F.G. Superman has a secret identity . . . 
when trouble strikes at any time . . . 
at any place . . . he is ready to become . . . 
Bicycle Repair Man!"""
			]

def skipdoublereturns(text):
	return string.replace(text, '\n\n', '\n')

def nl2return(text):
	return string.replace(text, '\n', '\r')

def UpdateSplash(drawdialog = 0, what = 0):
	if drawdialog:
		splash.DrawDialog()
	drawtext(what)
	splash.GetDialogWindow().ValidWindowRect(splash.GetDialogPort().portRect)
	Qd.QDFlushPortBuffer(splash.GetDialogWindow().GetWindowPort(), None)

def drawtext(what = 0):
	Qd.SetPort(splash)
	fontID = Fm.GetFNum("Python-Sans")
	if not fontID:
		fontID = geneva
	Qd.TextFont(fontID)
	Qd.TextSize(9)
	rect = (10, 115, _about_width - 10, _about_height - 30)
	if not what:
		import __main__
		abouttxt = nl2return(abouttext1 % (
				__main__.__version__, sys.version, skipdoublereturns(sys.copyright)))
	else:
		import random
		abouttxt = nl2return(random.choice(flauwekul))
	TE.TETextBox(abouttxt, rect, teJustCenter)

UpdateSplash(1)

def wait():
	import Evt
	import Events
	global splash
	try:
		splash
	except NameError:
		return
	Qd.InitCursor()
	time = Evt.TickCount()
	whattext = 0
	drawtext(whattext)
	while _keepsplashscreenopen:
		ok, event = Evt.EventAvail(Events.highLevelEventMask)
		if ok:
			# got apple event, back to mainloop
			break
		ok, event = Evt.EventAvail(Events.mDownMask | Events.keyDownMask | Events.updateMask)
		if ok:
			ok, event = Evt.WaitNextEvent(Events.mDownMask | Events.keyDownMask | Events.updateMask, 30)
			if ok:
				(what, message, when, where, modifiers) = event
				if what == Events.updateEvt:
					if Win.WhichWindow(message) == splash:
						UpdateSplash(1, whattext)
				else:
					break
		if Evt.TickCount() - time > 360:
			whattext = not whattext
			drawtext(whattext)
			time = Evt.TickCount()
	del splash


def about():
	global splash, splashresfile, _keepsplashscreenopen
	_keepsplashscreenopen = 1
	splash = Dlg.GetNewDialog(468, -1)
	splash.DrawDialog()
	wait()
