from Carbon import Dlg
from Carbon import Res

splash = Dlg.GetNewDialog(468, -1)
splash.DrawDialog()

from Carbon import Qd, TE, Fm

from Carbon import Win
from Carbon.Fonts import *
from Carbon.QuickDraw import *
from Carbon.TextEdit import teJustCenter
import string
import sys

_about_width = 440
_about_height = 340

_keepsplashscreenopen = 0

abouttext1 = """The Python Integrated Development Environment for the Macintosh\xaa
Version: %s
Copyright 1997-2001 Just van Rossum, Letterror. <just@letterror.com>
Python %s
%s
See: <http://www.python.org/> for information and documentation."""

flauwekul = [   "Goodday, Bruce.",
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
    splash.GetDialogWindow().ValidWindowRect(splash.GetDialogPort().GetPortBounds())
    splash.GetDialogWindow().GetWindowPort().QDFlushPortBuffer(None)

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
    from Carbon import Evt
    from Carbon import Events
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
