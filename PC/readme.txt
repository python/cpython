Welcome to the "PC" subdirectory of the Python distribution!


This "PC" subdirectory contains complete project files to make
several PC ports of Python, as well as all the PC-specific
Python source files.  It should be located in the root of the
Python distribution, and there should be directories "Modules",
"Objects", "Python", etc. in the parent directory of this "PC"
subdirectory.

Be sure to read the documentation in the Python distribution.  You
must set the environment variable PYTHONPATH to point to your Python
library directory.  This is "../Lib", but you must use an absolute path,
and perhaps copy it somewhere else.  Be sure to include the Windows
specific directory "win" too.  If you use a DOS FAT file system and
either a DOS or Windows 3.1x Python version, you should also put
../Lib/dos_8x3 on your PYTHONPATH too, since it has DOS 8x3 names
for the standard Python library names.  So your autoexec.bat should have:
   set PYTHONPATH=.;c:\python\lib;c:\python\lib\win
for Windows NT or
   set PYTHONPATH=.;c:\python\lib;c:\python\lib\win;c:\python\lib\dos_8x3
for DOS or Windows 3.1x (change the path to the correct path).

There are several add-in modules to build Python programs which use
the native Windows operating environment.  The ports here just make
"QuickWin" and DOS Python versions which support a character-mode
(console) environment.  Look in www.python.org for Tkinter, PythonWin,
WPY and wxPython.

To make a Python port, start the Integrated Development Environment
(IDE) of your compiler, and read in the native "project file"
(or makefile) provided.  This will enable you to change any source
files or build settings so you can make custom builds.

config.h    An important configuration file specific to PC's.

config.c    The list of C modules to include in the Python PC
            version.  Manually edit this file to add or
            remove Python modules.

testpy.py   A Python test program.  Run this to test your
            Python port.  It should say "all tests OK".

src         A subdirectory used only for VC++ version 1.5 Python
            source files.  See below.  The other compilers do not
            use it.  They reference the actual distribution
            directories instead.

Watcom C++ Version 10.6
=======================

The project file for the Watcom compiler is ./python.wpj.
It will build Watcom versions in the directories wat_*.

wat_dos     A 32-bit extended DOS Python (console-mode) using the
            dos4gw DOS extender.  Sockets are not included.

wat_os2     A 32-bit OS/2 Python (console-mode).
            Sockets are not included.


Microsoft Visual C++ Version 4.0 (32-bit Windows)
=================================================

The project files are vc40.mdp, vc40.ncb and vc40.mak.  They
will NOT work from this PC directory.  To use them, first copy
them to the Python distribution directory with this command:
    copy vc40.* ..
You may then want to remove them from here to prevent confusion.

Once the project files are located in the directory just above
this one, start VC++ and read in the project.  The targets are built
in the subdirectories vc40_*.

vc40_dll    The Python core built as an NT DLL.

vc40_nt     A Windows NT and 95 Python QuickWin (console-mode)
            version of Python including sockets.  It is
            self-contained, and does not require any DLL's.


Microsoft Visual C++ Version 1.5 (16-bit Windows)
=================================================

Since VC++1.5 does not handle long file names, it is necessary
to run the "makesrc.exe" program in this directory to copy
Python files from the distribution to the directory "src"
with shortened names.  Included file names are shortened too.
Do this before you attempt to build Python.

The "makesrc.exe" program is a native NT program, and you must
have NT, Windows 95 or Win32s to run it.  Otherwise you will need
to copy distribution files to src yourself.

The makefiles are named *.mak and are located in directories
starting with "vc15_".  NOTE:  When dependencies are scanned
VC++ will create dependencies for directories which are not
used because it fails to evaluate "#define" properly.  You
must manaully edit makefiles (*.mak) to remove references to
"sys/" and other bad directories.

vc15_lib    A static Python library.  Create this first because is
            is required for vc15_w31.

vc15_w31    A Windows 3.1x Python QuickWin (console-mode)
            Python including sockets.  Requires vc15_lib.
