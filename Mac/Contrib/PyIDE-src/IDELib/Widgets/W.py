"""Widgets for the Macintosh. Built on top of FrameWork"""

__version__ = "0.1"

from Wbase import *
from Wcontrols import *
from Wtext import *
from Wlist import *
from Wwindows import *
from Wmenus import *

_application = None
_signature = None

AlertError = 'AlertError'

def setapplication(app, sig):
	global _application, _signature
	_application = app
	_signature = sig

def getapplication():
	if _application is None:
		raise WidgetsError, 'W not properly initialized: unknown Application'
	return _application

def Message(text):
	import EasyDialogs, Qd
	Qd.InitCursor()
	if text:
		EasyDialogs.Message(text)
	else:
		EasyDialogs.Message('<Alert text not specified>')
