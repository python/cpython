# -*- mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#	$Id: WmDefault.py,v 1.2 2001/12/09 05:03:09 idiscovery Exp $
#


"""One of the bad things about Tk/Tkinter is that it does not pick up
the current color and font scheme from the prevailing CDE/KDE/GNOME/Windows 
window manager scheme.

One of the good things about Tk/Tkinter is that it is not tied to one
particular widget set so it could pick up the current color and font scheme 
from the prevailing CDE/KDE/GNOME/Windows window manager scheme.

The WmDefault package is for making Tk/Tkinter applications use the 
prevailing CDE/KDE/GNOME/Windows scheme. It tries to find the files
and/or settings that the current window manager is using, and then
sets the Tk options database accordingly.

Download the latest version of wm_default from http://tix.sourceforge.net
either as a part of the standard Tix distribution, or as a part of the
Tix Applications: http://tix.sourceforge.net/Tide. wm_default does not
require Tix, but is Tix enabled.
"""

import os, sys, traceback, string
import tkMessageBox

def setup(root, wm=''):
    """1) find the files and/or settings (::wm_default::setup).
    Takes one optional argument: wm, the name of the window manager
    as a string, if known. One of: windows gnome kde1 kde2 cde kde.
    """
    try:
        try:
            # Make sure Tcl/Tk knows wm_default is installed
            root.tk.eval("package require wm_default")
        except:
            # Try again with this directory on the Tcl/Tk path
            dir = os.path.dirname (self.__file__)
            root.tk.eval('global auto_path; lappend auto_path {%s}' % dir)
            root.tk.eval("package require wm_default")
    except:
        t, v, tb = sys.exc_info()
        text = "Error loading WmDefault\n"
        for line in traceback.format_exception(t,v,tb): text = text + line + '\n'
        try:
            tkMessageBox.showerror ('WmDefault Error', text)
        except:
            sys.stderr.write( text )

    return root.tk.call('::wm_default::setup', wm)

def addoptions(root, cnf=None, **kw):
    """2) Setting the Tk options database (::wm_default::addoptions).
    You can override the settings in 1) by adding your values to the
    call to addoptions().
    """
    if cnf is None:
        return root.tk.splitlist(root.tk.call('::wm_default::addoptions'))
    return root.tk.splitlist(
        apply(root.tk.call,
              ('::wm_default::addoptions',) + root._options(cnf,kw)))

def getoptions(root):
    """Returns the current settings, as a dictionary.
    """
    words = root.tk.splitlist(root.tk.call('::wm_default::getoptions'))
    dict = {}
    for i in range(0, len(words), 2):
        key = words[i]
        value = words[i+1]
        dict[key] = value
    return dict

def parray(root):
    """Returns a string of the current settings, one value-pair per line.
    """
    return root.tk.call('::wm_default::parray')

if __name__ == "__main__":
    dir = ""
    if len(sys.argv) > 0:
        # Assume the name of the file containing the tixinspect Tcl source
        # is the same as argument on the command line with .tcl
	dir = os.path.dirname(sys.argv[0])
    if not dir or not os.path.isdir(dir) or not os.path.isabs(dir):
        # Or, assume it's in the same directory as this one:
        dir = os.getcwd()
    import Tkinter
    root = Tkinter.Tk()
    setup(root)
    addoptions(root, {'foreground': 'red'})
    retval = getoptions(root)
    print retval

