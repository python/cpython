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

The distributed Makefile is vc40.mak.  This file is distributed with
CRLF line separators, otherwise Developer Studio won't like it.  It
will NOT work from this PC directory.  To use it, first copy it to the
Python distribution directory, e.g. with this command:
    copy vc40.mak ..
To convert the Makefile into a project file, start Developer Studio,
choose Open Workspace, change the file name pattern to *.mak, find and
select the file vc40.mak, and click OK.  Developer Studio will create
additional project files vc40.ncb and vc40.mdp when you use the
project.  The project contains three targets, which should be built in
this order:

python14    The Python core as a DLL, named python14.dll.

python	    The Python main program, named python.exe.  This should
	    work as a console program under Windows 95 or NT, as well
	    as under Windows 3.1(1) when using win32s.  It uses
	    python14.dll.

_tkinter    The optional _tkinter extension, _tkinter.dll; see below.

ALl end products of the compilation are placed in the subdirectory
vc40 (which Developer Studio creates); object files are placed in
vc40/tmp.  There are no separate Release and Debug project variants.
Note that the python and _tkinter projects require that the
python14.lib file exists in the vc40 subdirectory before they can be
built.

*** How to build the _tkinter extension ***

This assumes that you have installed the Tcl/Tk binary distribution
for Windows 95/NT with version numbers 7.5p1/4.1p1, in the default
installation location (C:\tcl).  (Ftp to ftp.sunlabs.com in /pub/tcl,
file win41p1.exe.)  You must also have fetched and unpacked the zip
file vclibs41.zip which contains the files tcl75.lib and tk41.lib, and
placed those files in C:\tcl\bin.  In order to use _tkinter, the
Tkinter.py module must be on PYTHONPATH.  It is found in the
Lib\tkinter subdirectory.


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
