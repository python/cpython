Welcome to the "PC" subdirectory of the Python distribution!
************************************************************


This "PC" subdirectory contains complete project files to make
several PC ports of Python, as well as all the PC-specific
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
  3) For Win32 platforms (NT/95), paths specified in the Registry.
  4) Default directories lib, lib/win, lib/test, lib/tkinter;
     these are searched relative to the environment variable
     PYTHONHOME, if set, or relative to the executable and its
     ancestors, if a landmark file (Lib/string.py) is found ,
     or the current directory (not useful).
  5) The directory containing the executable.

The best installation strategy is to put the Python executable (and
DLL, for Win32 platforms) in some convenient directory such as
C:/python, and copy all library files and subdirectories (using XCOPY)
to C:/python/lib.  Then you don't need to set PYTHONPATH.  Otherwise,
set the environment variable PYTHONPATH to your Python search path.
For example,
   set PYTHONPATH=.;d:\python\lib;d:\python\lib\win;d:\python\lib\dos_8x3

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
            Python port.  It should produce copious output,
	    ending in a report on how many tests were OK, how many
	    failed, and how many were skipped.  Don't worry about
	    skipped tests (these test unavailable optional features).

src         A subdirectory used only for VC++ version 1.5 Python
            source files.  See below.  The other compilers do not
            use it.  They reference the actual distribution
            directories instead.


Microsoft Visual C++ Version 4.x (32-bit Windows)
=================================================

(For historic reasons this uses the filename "vc40"; it has been tested
most recently with VC 4.2.  See below for VC 5.x.)

NOTE: VC 4.2 support is eroding, as I no longer have a VC 4.2
installation.  Some newer files need to be added to the project.

The distributed Makefile is vc40.mak.  This file is distributed with
CRLF line separators, otherwise Developer Studio won't like it.  It
will NOT work from this PC directory.  To use it, first copy it to the
Python distribution directory, e.g. with this command:
    copy vc40.mak ..
To convert the Makefile into a project file, start Developer Studio,
choose Open Workspace, change the file name pattern to *.mak, find and
select the file vc40.mak, and click OK.  Developer Studio will create
additional project files vc40.ncb and vc40.mdp when you use the
project.  The project contains six targets, which should be built in
this order:

python15    The Python core as a DLL, named python15.dll.

python      The Python main program, named python.exe.  This should
            work as a console program under Windows 95 or NT, as well
            as under Windows 3.1(1) when using win32s.  It uses
            python15.dll.

_tkinter    The optional _tkinter extension, _tkinter.dll; see below.

All end products of the compilation are placed in the subdirectory
vc40 (which Developer Studio creates); object files are placed in
vc40/tmp.  There are no separate Release and Debug project variants.
Note that the python and _tkinter projects require that the
python15.lib file exists in the vc40 subdirectory before they can be
built.

*** How to build the _tkinter extension ***

This assumes that you have installed the Tcl/Tk binary distribution for
Windows 95/NT with version numbers 7.5p1/4.1p1, in the default
installation location (C:\tcl).  (Ftp to ftp.sunlabs.com in /pub/tcl,
file win41p1.exe.)  You must also fetch and unpack the zip file
vclibs41.zip which contains the files tcl75.lib and tk41.lib, and place
those files in the PC subdirectory.  In order to use _tkinter, the
Tkinter.py module must be on PYTHONPATH.  It is found in the
Lib\tkinter subdirectory.

Microsoft Visual C++ Version 5.x (Developer Studio)
===================================================

For Visual C++ 5.x (Developer Studio) the instructions are somewhat
different again.  Three project files (*.dsp) and a workspace file
(pcbuild.dsw) are provided in the subdirectory vc5x.  (These are the
same three subprojects as discussed for VC++ 4.x.)

To use these, copy the files from vc5x to the toplevel PCbuild
directory.  Then open the pcbuild.dsw workspace file with Developer
Studio.  Select the Debug configuration (use Set Active
Configuration... in the Build menu) and build the python15 and python
projects (in that order).  If you have Tcl/Tk 8.0 installed you can
also try building the _tkinter project.  If you plan to use the parser
module, also build that project.


Additional files and subdirectories for 32-bit Windows
======================================================

python_nt.def  Exports definition file for python15.dll.

python_nt.rc   Resource compiler input for python15.dll.

dl_nt.c, import_nt.c
               Additional sources used for 32-bit Windows features.

getpathp.c     Default sys.path calculations (for all PC platforms).

dllbase_nt.txt A (manually maintained) list of base addresses for
               various DLLs, to avoid run-time relocation.

_tkinter.def   The export definitions file for _tkinter.dll.
	       (No longer needed; the /export:init_tkinter takes care
	       of this.)

make_nt.in     Include file for nmake-based builds (unsupported).

example_nt     A subdirectory showing how to build an extension as a
               DLL.

setup_nt       A subdirectory containing an experimental installer
               using Python only.


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


Watcom C++ Version 10.6
=======================

The project file for the Watcom compiler is ./python.wpj.
It will build Watcom versions in the directories wat_*.

wat_dos     A 32-bit extended DOS Python (console-mode) using the
            dos4gw DOS extender.  Sockets are not included.

wat_os2     A 32-bit OS/2 Python (console-mode).
            Sockets are not included.


IBM VisualAge C/C++ for OS/2
============================

See os2vacpp/readme.txt.  This platform is supported by Jeff Rush.
