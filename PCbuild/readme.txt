Building Python using VC++ 6.0 or 5.0
-------------------------------------
This directory is used to build Python for Win32 platforms, e.g. Windows
95, 98 and NT.  It requires Microsoft Visual C++ 6.x or 5.x.
(For other Windows platforms and compilers, see ../PC/readme.txt.)

All you need to do is open the workspace "pcbuild.dsw" in MSVC++, select
the Debug or Release setting (using Build -> Set Active Configuration...),
and build the projects.

The proper order to build subprojects:

1) pythoncore (this builds the main Python DLL and library files,
               python21.{dll, lib} in Release mode)
              NOTE:  in previous releases, this subproject was
              named after the release number, e.g. python20.

2) python (this builds the main Python executable,
           python.exe in Release mode)

3) the other subprojects, as desired or needed (note:  you probably don't
   want to build most of the other subprojects, unless you're building an
   entire Python distribution from scratch, or specifically making changes
   to the subsystems they implement; see SUBPROJECTS below)

When using the Debug setting, the output files have a _d added to
their name:  python21_d.dll, python_d.exe, parser_d.pyd, and so on.

SUBPROJECTS
-----------
These subprojects should build out of the box.  Subprojects other than the
main ones (pythoncore, python, pythonw) generally build a DLL (renamed to
.pyd) from a specific module so that users don't have to load the code
supporting that module unless they import the module.

pythoncore
    .dll and .lib
python
    .exe
pythonw
    pythonw.exe, a variant of python.exe that doesn't pop up a DOS box
_socket
    socketmodule.c
_sre
    Unicode-aware regular expression engine
_symtable
    the _symtable module, symtablemodule.c
_testcapi
    tests of the Python C API, run via Lib/test/test_capi.py, and
    implemented by module Modules/_testcapimodule.c
mmap
    mmapmodule.c
parser
    the parser module
pyexpat
    Python wrapper for accelerated XML parsing, which incorporates stable
    code from the Expat project:  http://sourceforge.net/projects/expat/
select
    selectmodule.c
unicodedata
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
    NOTE: This procedure is new (& simpler, & safer) for 2.1a2.
    Python wrapper for the zlib compression library.  Get the source code
    for version 1.1.3 from a convenient mirror at:
        http://www.info-zip.org/pub/infozip/zlib/
    Unpack into dist\zlib-1.1.3.
    A custom pre-link step in the zlib project settings should manage to
    build zlib-1.1.3\zlib.lib by magic before zlib.pyd (or zlib_d.pyd) is
    linked in PCbuild\.
    However, the zlib project is not smart enough to remove anything under
    zlib-1.1.3\ when you do a clean, so if you want to rebuild zlib.lib
    you need to clean up zlib-1.1.3\ by hand.

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
    TODO:  make this work like zlib (in particular, MSVC runs the prelink
    step in an enviroment that already has the correct envars set up).


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
