import subprocess
import sys
from test import support
import tkinter
import unittest

_tk_available = None

def check_tk_availability():
    """Check that Tk is installed and available."""
    global _tk_available

    if _tk_available is not None:
        return

    if sys.platform == 'darwin':
        # The Aqua Tk implementations on OS X can abort the process if
        # being called in an environment where a window server connection
        # cannot be made, for instance when invoked by a buildbot or ssh
        # process not running under the same user id as the current console
        # user.  Instead, try to initialize Tk under a subprocess.
        p = subprocess.Popen(
                [sys.executable, '-c', 'import tkinter; tkinter.Button()'],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stderr = support.strip_python_stderr(p.communicate()[1])
        if stderr or p.returncode:
            raise unittest.SkipTest("tk cannot be initialized: %s" % stderr)
    else:
        try:
            tkinter.Button()
        except tkinter.TclError as msg:
            # assuming tk is not available
            raise unittest.SkipTest("tk not available: %s" % msg)

    _tk_available = True
    return

def get_tk_root():
    check_tk_availability()     # raise exception if tk unavailable
    try:
        root = tkinter._default_root
    except AttributeError:
        # it is possible to disable default root in Tkinter, although
        # I haven't seen people doing it (but apparently someone did it
        # here).
        root = None

    if root is None:
        # create a new master only if there isn't one already
        root = tkinter.Tk()

    return root

def root_deiconify():
    root = get_tk_root()
    root.deiconify()

def root_withdraw():
    root = get_tk_root()
    root.withdraw()


def simulate_mouse_click(widget, x, y):
    """Generate proper events to click at the x, y position (tries to act
    like an X server)."""
    widget.event_generate('<Enter>', x=0, y=0)
    widget.event_generate('<Motion>', x=x, y=y)
    widget.event_generate('<ButtonPress-1>', x=x, y=y)
    widget.event_generate('<ButtonRelease-1>', x=x, y=y)
