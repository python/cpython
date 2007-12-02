"""icopen patch

OVERVIEW

icopen patches MacOS Python to use the Internet Config file mappings to select
the type and creator for a file.

Version 1 released to the public domain 3 November 1999
by Oliver Steele (steele@cs.brandeis.edu).

DETAILS

This patch causes files created by Python's open(filename, 'w') command (and
by functions and scripts that call it) to set the type and creator of the file
to the type and creator associated with filename's extension (the
portion of the filename after the last period), according to Internet Config.
Thus, a script that creates a file foo.html will create one that opens in whatever
browser you've set to handle *.html files, and so on.

Python IDE uses its own algorithm to select the type and creator for saved
editor windows, so this patch won't effect their types.

As of System 8.6 at least, Internet Config is built into the system, and the
file mappings are accessed from the Advanced pane of the Internet control
panel.  User Mode (in the Edit menu) needs to be set to Advanced in order to
access this pane.

INSTALLATION

Put this file in your Python path, and create a file named {Python}:sitecustomize.py
that contains:
        import icopen

(If {Python}:sitecustomizer.py already exists, just add the 'import' line to it.)

The next time you launch PythonInterpreter or Python IDE, the patch will take
effect.
"""

import builtins

_builtin_open = globals().get('_builtin_open', builtins.open)

def _open_with_typer(*args):
    file = _builtin_open(*args)
    filename = args[0]
    mode = 'r'
    if args[1:]:
        mode = args[1]
    if mode[0] == 'w':
        from ic import error, settypecreator
        try:
            settypecreator(filename)
        except error:
            pass
    return file

builtins.open = _open_with_typer

"""
open('test.py')
_open_with_typer('test.py', 'w')
_open_with_typer('test.txt', 'w')
_open_with_typer('test.html', 'w')
_open_with_typer('test.foo', 'w')
"""
