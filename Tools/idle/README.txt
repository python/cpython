IDLE 0.5 - February 2000
------------------------

This is an early release of IDLE, my own attempt at a Tkinter-based
IDE for Python.

For news about this release, see the file NEWS.txt.  (For a more
detailed change log, see the file ChangeLog.)

FEATURES

IDLE has the following features:

- coded in 100% pure Python, using the Tkinter GUI toolkit (i.e. Tcl/Tk)

- cross-platform: works on Windows and Unix (on the Mac, there are
currently problems with Tcl/Tk)

- multi-window text editor with multiple undo, Python colorizing
and many other features, e.g. smart indent and call tips

- Python shell window (a.k.a. interactive interpreter)

- debugger (not complete, but you can set breakpoints, view  and step)

USAGE

The main program is in the file "idle.py"; on Unix, you should be able
to run it by typing "./idle.py" to your shell.  On Windows, you can
run it by double-clicking it; you can use idle.pyw to avoid popping up
a DOS console.  If you want to pass command line arguments on Windows,
use the batch file idle.bat.

Command line arguments: files passed on the command line are executed,
not opened for editing, unless you give the -e command line option.
Try "./idle.py -h" to see other command line options.

IDLE requires Python 1.5.2, so it is currently only usable with a
Python 1.5.2 distribution.  (An older version of IDLE is distributed
with Python 1.5.2; you can drop this version on top of it.)

COPYRIGHT

IDLE is covered by the standard Python copyright notice
(http://www.python.org/doc/Copyright.html).

FEEDBACK

For feedback, please use the Python Bugs List
(http://www.python.org/search/search_bugs.html).

--Guido van Rossum (home page: http://www.python.org/~guido/)
