"""curses

The main package for curses support for Python.  Normally used by importing
the package, and perhaps a particular module inside it.

   import curses
   from curses import textpad
   curses.initwin()
   ...
   
"""

__revision__ = "$Id$"

from _curses import *
from curses.wrapper import wrapper


