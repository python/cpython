"""Widgets for the Macintosh. Built on top of FrameWork"""

__version__ = "0.3"

from Wbase import *
from Wcontrols import *
from Wtext import *
from Wlists import *
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

def getdefaultfont():
    prefs = getapplication().getprefs()
    if not prefs.defaultfont:
        prefs.defaultfont = ("Geneva", 0, 10, (0, 0, 0))
    return prefs.defaultfont

def Message(text):
    import EasyDialogs, string
    from Carbon import Qd
    Qd.InitCursor()
    text = string.replace(text, "\n", "\r")
    if not text:
        text = '<Alert text not specified>'
    EasyDialogs.Message(text)
