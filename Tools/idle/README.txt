IDLE 0.2 - 8 January 1999
-------------------------

For news about this release, see the file NEWS.txt.

This is an early release of IDLE, my own attempt at a Tkinter-based
IDE for Python.  It has the following features:

- 100% pure Python
- multi-window text editor with multiple undo and Python colorizing
- Python shell (a.k.a. interactive interpreter) window subclass
- debugger (not complete, but you can set breakpoints and step)
- works on Windows and Unix (probably works on Mac too)

The main program is in the file "idle"; on Windows you can use idle.pyw
to avoid popping up a DOS console.  Any arguments passed are interpreted
as files that will be opened for editing.

IDLE requires Python 1.5.2, so it is currently only usable with the
Python 1.5.2 beta distribution (luckily, IDLE is bundled with Python
1.5.2).

Please send feedback to the Python newsgroup, comp.lang.python.

--Guido van Rossum (home page: http://www.python.org/~guido/)
