Welcome to the "PC" subdirectory of the Python distribution
***********************************************************

This "PC" subdirectory contains complete project files to make
several older PC ports of Python, as well as all the PC-specific
Python source files.  It should be located in the root of the
Python distribution, and there should be directories "Modules",
"Objects", "Python", etc. in the parent directory of this "PC"
subdirectory.  Be sure to read the documentation in the Python
distribution.

Python requires library files such as string.py to be available in
one or more library directories.  The search path of libraries is
set up when Python starts.  To see the current Python library search
path, start Python and enter "import sys" and "print sys.path".

All PC ports use this scheme to try to set up a module search path:

  1) The script location; the current directory without script.
  2) The PYTHONPATH variable, if set.
  3) Paths specified in the Registry.
  4) Default directories lib, lib/win, lib/test, lib/tkinter;
     these are searched relative to the environment variable
     PYTHONHOME, if set, or relative to the executable and its
     ancestors, if a landmark file (Lib/string.py) is found ,
     or the current directory (not useful).
  5) The directory containing the executable.

The best installation strategy is to put the Python executable and
DLL in some convenient directory such as
C:/python, and copy all library files and subdirectories (using XCOPY)
to C:/python/lib.  Then you don't need to set PYTHONPATH.  Otherwise,
set the environment variable PYTHONPATH to your Python search path.
For example,
   set PYTHONPATH=.;d:\python\lib;d:\python\lib\win;d:\python\lib\dos-8x3

There are several add-in modules to build Python programs which use
the native Windows operating environment.  The ports here just make
"QuickWin" and DOS Python versions which support a character-mode
(console) environment.  Look in www.python.org for Tkinter, PythonWin,
WPY and wxPython.

To make a Python port, start the Integrated Development Environment
(IDE) of your compiler, and read in the native "project file"
(or makefile) provided.  This will enable you to change any source
files or build settings so you can make custom builds.

pyconfig.h    An important configuration file specific to PC's.

config.c    The list of C modules to include in the Python PC
            version.  Manually edit this file to add or
            remove Python modules.


Additional files and subdirectories for 32-bit Windows
======================================================

python_nt.rc   Resource compiler input for python15.dll.

dl_nt.c
               Additional sources used for 32-bit Windows features.


Note for Windows 3.x and DOS users
==================================

Neither Windows 3.x nor DOS is supported any more.  The last Python
version that supported these was Python 1.5.2; the support files were
present in Python 2.0 but weren't updated, and it is not our intention
to support these platforms for Python 2.x.
