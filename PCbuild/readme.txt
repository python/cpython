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
    Python wrapper for the Tk windowing system.  Requires building
    Tcl/Tk first.  Following are instructions for Tcl/Tk 8.4.1:

    Get source
    ----------
    Go to
        http://prdownloads.sourceforge.net/tcl/
    and download
        tcl841-src.zip
        tk841-src.zip
    Unzip into
        dist\tcl8.4.1\
        dist\tk8.4.1\
    respectively.

    Build Tcl first (done here w/ MSVC 6 on Win2K; also Win98SE)
    ---------------
    cd dist\tcl8.4.1\win
    run vcvars32.bat [necessary even on Win2K]
    nmake -f makefile.vc
    nmake -f makefile.vc INSTALLDIR=..\..\tcl84 install

    XXX Should we compile with OPTS=threads?

    XXX Some tests failed in "nmake -f makefile.vc test".

    Build Tk
    --------
    cd dist\tk8.4.1\win
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.1
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.1 INSTALLDIR=..\..\tcl84 install

    XXX Should we compile with OPTS=threads?

    XXX Some tests failed in "nmake -f makefile.vc test".

    XXX Our installer copies a lot of stuff out of the Tcl/Tk install
    XXX directory.  Is all of that really needed for Python use of Tcl/Tk?

    Make sure the installer matches
    -------------------------------
    Ensure that the Wise compiler vrbl _TCLDIR_ is set to the name of
    the common Tcl/Tk installation directory (tcl84 for the instructions
    above).  This is needed so the installer can copy various Tcl/Tk
    files into the Python distribution.


zlib
    Python wrapper for the zlib compression library.  Get the source code
    for version 1.1.4 from a convenient mirror at:
        http://www.gzip.org/zlib/
    Unpack into dist\zlib-1.1.4.
    A custom pre-link step in the zlib project settings should manage to
    build zlib-1.1.4\zlib.lib by magic before zlib.pyd (or zlib_d.pyd) is
    linked in PCbuild\.
    However, the zlib project is not smart enough to remove anything under
    zlib-1.1.4\ when you do a clean, so if you want to rebuild zlib.lib
    you need to clean up zlib-1.1.4\ by hand.

bz2
    Python wrapper for the libbz2 compression library.  Homepage
        http://sources.redhat.com/bzip2/
    Download the source tarball, bzip2-1.0.2.tar.gz.
    Unpack into dist\bzip2-1.0.2.  WARNING:  If you're using WinZip, you
    must disable its "TAR file smart CR/LF conversion" feature (under
    Options -> Configuration -> Miscellaneous -> Other) for the duration.

    Don't bother trying to use libbz2.dsp with MSVC.  After 10 minutes
    of fiddling, I couldn't get it to work.  Perhaps it works with
    MSVC 5 (I used MSVC 6).  It's better to run the by-hand makefile
    anyway, because it runs a helpful test step at the end.

    cd into dist\bzip2-1.0.2, and run
        nmake -f makefile.msc
    [Note that if you're running Win9X, you'll need to run vcvars32.bat
     before running nmake (this batch file is in your MSVC installation).
     TODO:  make this work like zlib (in particular, MSVC runs the prelink
     step in an enviroment that already has the correct envars set up).
    ]
    The make step shouldn't yield any warnings or errors, and should end
    by displaying 6 blocks each terminated with
        FC: no differences encountered
    If FC finds differences, see the warning abou WinZip above (when I
    first tried it, sample3.ref failed due to CRLF conversion).

    All of this managed to build bzip2-1.0.2\libbz2.lib, which the Python
    project links in.


_bsddb
    XXX The Sleepycat release we use will probably change before
    XXX 2.3a1.
    Go to Sleepycat's patches page:
        http://www.sleepycat.com/update/index.html
    and download
        4.0.14.zip
    from the download page.  The file name is db-4.0.14.zip.  Unpack into
        dist\db-4.0.14

    Apply the patch file bsddb_patch.txt in this (PCbuild) directory
    against the file
        dist\db-4.0.14\db\db_reclaim.c

    Go to
        http://www.sleepycat.com/docs/ref/build_win/intro.html
    and follow the instructions for building the Sleepycat software.
    Build the Release version.
    NOTE:  The instructions are for a later release of the software,
    so use your imagination.  Berkeley_DB.dsw in this release was
    also pre-MSVC6, so you'll be prompted to upgrade the format (say
    yes, of course).  Choose configuration "db_buildall - Win32 Release",
    and build db_buildall.exe.

    XXX We're actually linking against Release_static\libdb40s.lib.
    XXX This yields the following warnings:
"""
Compiling...
_bsddb.c
Linking...
   Creating library ./_bsddb.lib and object ./_bsddb.exp
LINK : warning LNK4049: locally defined symbol "_malloc" imported
LINK : warning LNK4049: locally defined symbol "_free" imported
LINK : warning LNK4049: locally defined symbol "_fclose" imported
LINK : warning LNK4049: locally defined symbol "_fopen" imported
_bsddb.pyd - 0 error(s), 4 warning(s)
"""
    XXX This isn't encouraging, but I don't know what to do about it.



YOUR OWN EXTENSION DLLs
-----------------------
If you want to create your own extension module DLL, there's an example
with easy-to-follow instructions in ../PC/example/; read the file
readme.txt there first.
