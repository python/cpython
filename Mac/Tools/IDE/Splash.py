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
	rect = (35, 265, 365, 281)
	if module:
		TE.TETextBox('Importing: ' + module, rect, 0)
		if not _progress:
			Qd.FrameRect((35, 281, 365, 289))
		pos = min(36 + 330 * _progress / 44, 364)
		Qd.PaintRect((36, 282, pos, 288))
		_progress = _progress + 1
	else:
		Qd.EraseRect(rect)
		Qd.PaintRect((36, 282, pos, 288))
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

abouttext1 = """The Python Integrated Development Environment for the MacintoshÅ
Version: %s
Copyright 1997-2000 Just van Rossum, Letterror. <just@letterror.com>
Python %s
%s
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
	rect = (10, 115, 390, 290)
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
	#Res.CloseResFile(splashresfile)

def about():
	global splash, splashresfile, _keepsplashscreenopen
	_keepsplashscreenopen = 1
	splash = Dlg.GetNewDialog(468, -1)
	splash.DrawDialog()
	wait()
