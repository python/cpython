Building Python using VC++ 6.0 or 5.0
-------------------------------------
This directory is used to build Python for Win32 platforms, e.g. Windows
95, 98 and NT.  It requires Microsoft Visual C++ 6.x or 5.x.
(For other Windows platforms and compilers, see ../PC/readme.txt.)
XXX There are still (Python 2.0b1) a few compiler warnings under VC6.
XXX There are likely a few more under VC5.

Unlike older versions, there's no longer a need to copy the project files
from a subdirectory of PC/ to the PCbuild directory -- they come in PCbuild.

All you need to do is open the workspace "pcbuild.dsw" in MSVC++, select
the Debug or Release setting (using Build -> Set Active Configuration...),
and build the projects.

The proper order to build subprojects is:

1) python20 (this builds the main Python DLL and library files,
             python20.{dll, lib})

2) python   (this builds the main Python executable, python.exe)

3) the other subprojects, as desired or needed (note:  you probably don't
   want to build most of the other subprojects, unless you're building an
   entire Python distribution from scratch, or specifically making changes
   to the subsystems they implement; see SUBPROJECTS below)

When using the Debug setting, the output files have a _d added to
their name:  python20_d.dll, python_d.exe, parser_d.pyd, and so on.

SUBPROJECTS
-----------
These subprojects should build out of the box.  Subprojects other than the
main ones (python20, python, pythonw) generally build a DLL (renamed to
.pyd) from a specific module so that users don't have to load the code
supporting that module unless they import the module.

python20
    .dll and .lib
python
    .exe
pythonw
    pythonw.exe, a variant of python.exe that doesn't pop up a DOS box
_socket
    socketmodule.c
_sre
    Unicode-aware regular expression engine
mmap
    mmapmodule.c
parser
    the parser module
select
    selectmodule.c
ucnhash, unicodedata
    large tables of Unicode data
winreg
    Windows registry API
winsound
    play sounds (typically .wav files) under Windows

The following subprojects will generally NOT build out of the box.  They
wrap code Python doesn't control, and you'll need to download the base
packages first and unpack them into siblings of PCbuilds's parent
directory; for example, if your PCbuild is  .......\dist\src\PCbuild\,
unpack into new subdirectories of dist\.

_tkinter
    Python wrapper for the Tk windowing system.  Requires tcl832.exe from
        http://dev.scriptics.com/software/tcltk/downloadnow83.html
    Run the installer, forcing installation into dist\Tcl.
    Be sure to install everything, including the Tcl/Tk header files.

zlib
    Python wrapper for the zlib compression library.  Requires
        http://www.winimage.com/zLibDll/zlib133dll.zip
    and
        ftp://ftp.uu.net/graphics/png/src/zlib133.zip
    Unpack the former into dist\zlib113dll.
    Uppack the latter into dist\zlib113.

bsddb
    Python wrapper for the BSD database 1.85.  Requires db.1.85.win32.zip,
    from the "bsd db" link at
        http://www.nightmare.com/software.html
    Unpack into dist\bsddb.
    You then need to compile it:  cd to dist\bsddb\Port\win32, and run
        nmake -f makefile_nt.msc
    This builds bsddb\Port\win32\db.lib, which the MSVC project links in.
    Note that if you're running Win9X, you'll need to run vcvars32.bat
    before running nmake (this batch file is in your MSVC installation).

pyexpat
    Python wrapper for accelerated XML parsing.  Requires
        ftp://ftp.jclark.com/pub/xml/expat.zip
    Unpack into dist\expat.


NOTE ON CONFIGURATIONS
----------------------
Under Build -> Configuration ..., you'll find several Alpha configurations,
such as "Win32 Alpha Release".  These do not refer to alpha versions (as in
alpha, beta, final), but to the DEC/COMPAQ Alpha processor.  Ignore them if
you're not building on an Alpha box.


YOUR OWN EXTENSION DLLs
-----------------------
If you want to create your own extension module DLL, there's an example
with easy-to-follow instructions in ../PC/example/; read the file
readme.txt there first.
