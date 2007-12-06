Building Python using VC++ 9.0
------------------------------
This directory is used to build Python for Win32 platforms, e.g. Windows
2000, XP and Vista.  It requires Microsoft Visual C++ 9.0
(a.k.a. Visual Studio .NET 2008).
(For other Windows platforms and compilers, see ../PC/readme.txt.)

All you need to do is open the workspace "pcbuild.sln" in Visual Studio,
select the desired combination of configuration and platform and eventually
build the solution. Unless you are going to debug a problem in the core or
you are going to create an optimized build you want to select "Release" as
configuration.

The PCbuild9 directory is compatible with all versions of Visual Studio from
VS C++ Express Edition over the standard edition up to the professional
edition. However the express edition does support features like solution
folders or profile guided optimization (PGO). The missing bits and pieces
won't stop you from building Python.

The solution is configured to build the projects in the correct order. "Build
Solution" or F6 takes care of dependencies except for x64 builds. To make
cross compiling x64 builds on a 32bit OS possible the x64 builds require a 
32bit version of Python.


NOTE:
   You probably don't want to build most of the other subprojects, unless
   you're building an entire Python distribution from scratch, or
   specifically making changes to the subsystems they implement, or are
   running a Python core buildbot test slave; see SUBPROJECTS below)

When using the Debug setting, the output files have a _d added to
their name:  python30_d.dll, python_d.exe, parser_d.pyd, and so on.

The 32bit builds end up in the solution folder PCbuild9 while the x64 builds
land in the amd64 subfolder. The PGI and PGO builds for profile guided
optimization end up in their own folders, too.

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
_testcapi
    tests of the Python C API, run via Lib/test/test_capi.py, and
    implemented by module Modules/_testcapimodule.c
pyexpat
    Python wrapper for accelerated XML parsing, which incorporates stable
    code from the Expat project:  http://sourceforge.net/projects/expat/
select
    selectmodule.c
unicodedata
    large tables of Unicode data
winsound
    play sounds (typically .wav files) under Windows

The following subprojects will generally NOT build out of the box. They
wrap code Python doesn't control, and you'll need to download the base
packages first and unpack them into siblings of PCbuilds's parent
directory; for example, if your PCbuild9 is  ..\dist\py3k\PCbuild9\,
unpack into new subdirectories of ..\dist\.

_tkinter
    Python wrapper for the Tk windowing system.  Requires building
    Tcl/Tk first.  Following are instructions for Tcl/Tk 8.4.16.
    
    NOTE: The 64 build builds must land in tcltk64 instead of tcltk.

    Get source
    ----------
    In the dist directory, run
    svn export http://svn.python.org/projects/external/tcl8.4.16
    svn export http://svn.python.org/projects/external/tk8.4.16
    svn export http://svn.python.org/projects/external/tix-8.4.0

    Build with build_tkinter.py
    ---------------------------
    The PCbuild9 directory contains a Python script which automates all
    steps. Run the script in a Visual Studio 2009 command prompt with 

      python build_tkinter.py Win32

    Use x64 instead of Win32 for the x64 platform.
    
    Build Tcl first 
    ---------------
    Use "Start -> All Programs -> Microsoft Visual Studio 2008
         -> Visual Studio Tools -> Visual Studio 2008 Command Prompt"
    to get a shell window with the correct environment settings
    cd dist\tcl8.4.16\win
    nmake -f makefile.vc
    nmake -f makefile.vc INSTALLDIR=..\..\tcltk install

    XXX Should we compile with OPTS=threads?

    Optional:  run tests, via
        nmake -f makefile.vc test

        On WinXP Pro, wholly up to date as of 30-Aug-2004:
        all.tcl:        Total   10678   Passed  9969    Skipped 709     Failed  0
        Sourced 129 Test Files.

    Build Tk
    --------
    cd dist\tk8.4.16\win
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.16
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.16 INSTALLDIR=..\..\tcltk install

    XXX Should we compile with OPTS=threads?

    XXX Our installer copies a lot of stuff out of the Tcl/Tk install
    XXX directory.  Is all of that really needed for Python use of Tcl/Tk?

    Optional:  run tests, via
        nmake -f makefile.vc TCLDIR=..\..\tcl8.4.16 test

        On WinXP Pro, wholly up to date as of 30-Aug-2004:
        all.tcl:        Total   8420    Passed  6826    Skipped 1581    Failed  13
        Sourced 91 Test Files.
        Files with failing tests: canvImg.test scrollbar.test textWind.test winWm.test

   Built Tix
   ---------
   cd dist\tix-8.4.0\win
   nmake -f python9.mak
   nmake -f python9.mak install

bz2
    Python wrapper for the libbz2 compression library.  Homepage
        http://sources.redhat.com/bzip2/
    Download the source from the python.org copy into the dist
    directory:

    svn export http://svn.python.org/projects/external/bzip2-1.0.3

    A custom pre-link step in the bz2 project settings should manage to
    build bzip2-1.0.3\libbz2.lib by magic before bz2.pyd (or bz2_d.pyd) is
    linked in PCbuild9\.
    However, the bz2 project is not smart enough to remove anything under
    bzip2-1.0.3\ when you do a clean, so if you want to rebuild bzip2.lib
    you need to clean up bzip2-1.0.3\ by hand.

    All of this managed to build libbz2.lib in 
    bzip2-1.0.3\$platform-$configuration\, which the Python project links in.


_bsddb
    To use the version of bsddb that Python is built with by default, invoke
    (in the dist directory)

      svn export http://svn.python.org/projects/external/db-4.4.20

    Next open the solution file db-4.4.20\build_win32\Berkeley_DB.sln with
    Visual Studio and convert the projects to the new format. The standard
    and professional version of VS 2008 builds the necessary libraries
    in a pre-link step of _bsddb. However the express edition is missing
    some pieces and you have to build the libs yourself.
    
    The _bsddb subprojects depends only on the db_static project of 
    Berkeley DB. You have to choose either "Release", "Release AMD64", "Debug"
    or "Debug AMD64" as configuration.

    Alternatively, if you want to start with the original sources,
    go to Sleepycat's download page:
        http://www.sleepycat.com/downloads/releasehistorybdb.html

    and download version 4.4.20.

    With or without strong cryptography? You can choose either with or
    without strong cryptography, as per the instructions below.  By
    default, Python is built and distributed WITHOUT strong crypto.

    Unpack the sources; if you downloaded the non-crypto version, rename
    the directory from db-4.4.20.NC to db-4.4.20.

    Now apply any patches that apply to your version.

    Open
        db-4.4.20\docs\ref\build_win\intro.html

    and follow the "Windows->Building Berkeley DB with Visual C++ .NET"
    instructions for building the Sleepycat
    software.  Note that Berkeley_DB.dsw is in the build_win32 subdirectory.
    Build the "db_static" project, for "Release" mode.

    To run extensive tests, pass "-u bsddb" to regrtest.py.  test_bsddb3.py
    is then enabled.  Running in verbose mode may be helpful.

_sqlite3
    Python wrapper for SQLite library.
    
    Get the source code through
    
    svn export http://svn.python.org/projects/external/sqlite-source-3.3.4
    
    To use the extension module in a Python build tree, copy sqlite3.dll into
    the PCbuild folder. The source directory in svn also contains a .def file
    from the binary release of sqlite3.

_ssl
    Python wrapper for the secure sockets library.

    Get the source code through

    svn export http://svn.python.org/projects/external/openssl-0.9.8g

    Alternatively, get the latest version from http://www.openssl.org.
    You can (theoretically) use any version of OpenSSL you like - the
    build process will automatically select the latest version.

    You must install the NASM assembler from
        http://www.kernel.org/pub/software/devel/nasm/binaries/win32/
    for x86 builds.  Put nasmw.exe anywhere in your PATH.

    You can also install ActivePerl from
        http://www.activestate.com/Products/ActivePerl/
    if you like to use the official sources instead of the files from 
    python's subversion repository. The svn version contains pre-build
    makefiles and assembly files.

    The build process makes sure that no patented algorithms are included.
    For now RC5, MDC2 and IDEA are excluded from the build. You may have 
    to manually remove $(OBJ_D)\i_*.obj from ms\nt.mak if the build process
    complains about missing files or forbidden IDEA. Again the files provided
    in the subversion repository are already fixed.

    The MSVC project simply invokes PCBuild/build_ssl.py to perform
    the build.  This Python script locates and builds your OpenSSL
    installation, then invokes a simple makefile to build the final .pyd.

    build_ssl.py attempts to catch the most common errors (such as not
    being able to find OpenSSL sources, or not being able to find a Perl
    that works with OpenSSL) and give a reasonable error message.
    If you have a problem that doesn't seem to be handled correctly
    (eg, you know you have ActivePerl but we can't find it), please take
    a peek at build_ssl.py and suggest patches.  Note that build_ssl.py
    should be able to be run directly from the command-line.

    build_ssl.py/MSVC isn't clever enough to clean OpenSSL - you must do
    this by hand.

Building for Itanium
--------------------

NOTE:
Official support for Itanium builds have been dropped from the build. Please
contact as and provide patches if you are interested in Itanium builds.

The project files support a ReleaseItanium configuration which creates
Win64/Itanium binaries. For this to work, you need to install the Platform
SDK, in particular the 64-bit support. This includes an Itanium compiler
(future releases of the SDK likely include an AMD64 compiler as well).
In addition, you need the Visual Studio plugin for external C compilers,
from http://sf.net/projects/vsextcomp. The plugin will wrap cl.exe, to
locate the proper target compiler, and convert compiler options
accordingly. The project files require atleast version 0.9.

Building for AMD64
------------------

The build process for AMD64 / x64 is very similar to standard builds. You just
have to set x64 as platform. 

Building Python Using the free MS Toolkit Compiler
--------------------------------------------------

Microsoft has withdrawn the free MS Toolkit Compiler, so this can no longer
be considered a supported option. Instead you can use the free VS C++ Express
Edition.

Profile Guided Optimization
---------------------------

The solution has two configurations for PGO. The PGInstrument configuration
must be build first. The PGInstrument binaries are lniked against a profiling
library and contain extra debug information. The PGUpdate configuration takes the profiling data and generates optimized binaries.

The build_pgo.bat script automates the creation of optimized binaries. It
creates the PGI files, runs the unit test suite or PyBench with the PGI
python and finally creates the optimized files.

http://msdn2.microsoft.com/en-us/library/e7k32f4k(VS.90).aspx

Static library
--------------

The solution has no configuration for static libraries. However it is easy
it build a static library instead of a DLL. You simply have to set the 
"Configuration Type" to "Static Library (.lib)" and alter the preprocessor
macro "Py_ENABLE_SHARED" to "Py_NO_ENABLE_SHARED". You may also have to
change the "Runtime Library" from "Multi-threaded DLL (/MD)" to 
"Multi-threaded (/MT)".

Visual Studio properties
------------------------

The PCbuild9 solution makes heavy use of Visual Studio property files 
(*.vsprops). The properties can be viewed and altered in the Property
Manager (View -> Other Windows -> Property Manager).

 * debug (debug macro: _DEBUG)
 * pginstrument (PGO)
 * pgupdate (PGO)
    +-- pginstrument
 * pyd (python extension, release build)
    +-- release
    +-- pyproject
 * pyd_d (python extension, debug build)
    +-- debug
    +-- pyproject
 * pyproject (base settings for all projects, user macros like PyDllName)
 * release (release macro: NDEBUG)
 * x64 (AMD64 / x64 platform specific settings)

The pyproject propertyfile defines _WIN32 and x64 defines _WIN64 and _M_X64
although the macros are set by the compiler, too. The GUI doesn't always know
about the macros and confuse the user with false information.

YOUR OWN EXTENSION DLLs
-----------------------

If you want to create your own extension module DLL, there's an example
with easy-to-follow instructions in ../PC/example/; read the file
readme.txt there first.
