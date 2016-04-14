Building Python using VC++ 9.0
------------------------------

This directory is used to build Python for Win32 and x64 platforms, e.g.
Windows 2000, XP, Vista and Windows Server 2008.  In order to build 32-bit
debug and release executables, Microsoft Visual C++ 2008 Express Edition is
required at the very least.  In order to build 64-bit debug and release
executables, Visual Studio 2008 Standard Edition is required at the very
least.  In order to build all of the above, as well as generate release builds
that make use of Profile Guided Optimisation (PG0), Visual Studio 2008
Professional Edition is required at the very least.  The official Python
releases are built with this version of Visual Studio.

For other Windows platforms and compilers, see PC/readme.txt.

All you need to do is open the workspace "pcbuild.sln" in Visual Studio,
select the desired combination of configuration and platform and eventually
build the solution. Unless you are going to debug a problem in the core or
you are going to create an optimized build you want to select "Release" as
configuration.

The PCbuild directory is compatible with all versions of Visual Studio from
VS C++ Express Edition over the standard edition up to the professional
edition. However the express edition does not support features like solution
folders or profile guided optimization (PGO). The missing bits and pieces
won't stop you from building Python.

The solution is configured to build the projects in the correct order. "Build
Solution" or F7 takes care of dependencies except for x64 builds. To make
cross compiling x64 builds on a 32bit OS possible the x64 builds require a
32bit version of Python.

NOTE:
   You probably don't want to build most of the other subprojects, unless
   you're building an entire Python distribution from scratch, or
   specifically making changes to the subsystems they implement, or are
   running a Python core buildbot test slave; see SUBPROJECTS below)

When using the Debug setting, the output files have a _d added to
their name:  python27_d.dll, python_d.exe, parser_d.pyd, and so on. Both
the build and rt batch files accept a -d option for debug builds.

The 32bit builds end up in the solution folder PCbuild while the x64 builds
land in the amd64 subfolder. The PGI and PGO builds for profile guided
optimization end up in their own folders, too.

Legacy support
--------------

You can find build directories for older versions of Visual Studio and
Visual C++ in the PC directory. The legacy build directories are no longer
actively maintained and may not work out of the box.

PC/VC6/
    Visual C++ 6.0
PC/VS7.1/
    Visual Studio 2003 (7.1)
PC/VS8.0/
    Visual Studio 2005 (8.0)


C RUNTIME
---------

Visual Studio 2008 uses version 9 of the C runtime (MSVCRT9).  The executables
are linked to a CRT "side by side" assembly which must be present on the target
machine.  This is available under the VC/Redist folder of your visual studio
distribution. On XP and later operating systems that support
side-by-side assemblies it is not enough to have the msvcrt90.dll present,
it has to be there as a whole assembly, that is, a folder with the .dll
and a .manifest.  Also, a check is made for the correct version.
Therefore, one should distribute this assembly with the dlls, and keep
it in the same directory.  For compatibility with older systems, one should
also set the PATH to this directory so that the dll can be found.
For more info, see the Readme in the VC/Redist folder.

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

Python-controlled subprojects that wrap external projects:
_bsddb
    Wraps Berkeley DB 4.7.25, which is currently built by _bsddb.vcproj.
    project.
_sqlite3
    Wraps SQLite 3.6.21, which is currently built by sqlite3.vcproj.
_tkinter
    Wraps the Tk windowing system.  Unlike _bsddb and _sqlite3, there's no
    corresponding tcltk.vcproj-type project that builds Tcl/Tk from vcproj's
    within our pcbuild.sln, which means this module expects to find a
    pre-built Tcl/Tk in either ..\externals\tcltk for 32-bit or
    ..\externals\tcltk64 for 64-bit (relative to this directory).  See below
    for instructions to build Tcl/Tk.
bz2
    Python wrapper for the libbz2 compression library.  Homepage
        http://sources.redhat.com/bzip2/
    Download the source from the python.org copy into the dist
    directory:

    svn export http://svn.python.org/projects/external/bzip2-1.0.6

    ** NOTE: if you use the PCbuild\get_externals.bat approach for
    obtaining external sources then you don't need to manually get the source
    above via subversion. **

_ssl
    Python wrapper for the secure sockets library.

    Get the source code through

    svn export http://svn.python.org/projects/external/openssl-1.0.2g

    ** NOTE: if you use the PCbuild\get_externals.bat approach for
    obtaining external sources then you don't need to manually get the source
    above via subversion. **

    The NASM assembler is required to build OpenSSL.  If you use the
    PCbuild\get_externals.bat script to get external library sources, it also
    downloads a version of NASM, which the ssl build script will add to PATH.
    Otherwise, you can download the NASM installer from
        http://www.nasm.us/
    and add NASM to your PATH.

    You will also need ActivePerl from
        http://www.activestate.com/activeperl/
    in order to create the necessary makefiles and .asm files for building
    OpenSSL.

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

The subprojects above wrap external projects Python doesn't control, and as
such, a little more work is required in order to download the relevant source
files for each project before they can be built.  The easiest way to do this
is to use the `build.bat` script in this directory to build Python, and pass
the '-e' switch to tell it to use get_externals.bat to fetch external sources
and build Tcl/Tk and Tix.  To use get_externals.bat, you'll need to have
Subversion installed and svn.exe on your PATH.  The script will fetch external
library sources from http://svn.python.org/external and place them in
..\externals (relative to this directory).

Building for Itanium
--------------------

Official support for Itanium builds have been dropped from the build. Please
contact us and provide patches if you are interested in Itanium builds.

Building for AMD64
------------------

The build process for AMD64 / x64 is very similar to standard builds. You just
have to set x64 as platform. In addition, the HOST_PYTHON environment variable
must point to a Python interpreter (at least 2.4), to support cross-compilation.

Building Python Using the free MS Toolkit Compiler
--------------------------------------------------

Microsoft has withdrawn the free MS Toolkit Compiler, so this can no longer
be considered a supported option. Instead you can use the free VS C++ Express
Edition.

Profile Guided Optimization
---------------------------

The solution has two configurations for PGO. The PGInstrument
configuration must be build first. The PGInstrument binaries are
linked against a profiling library and contain extra debug
information. The PGUpdate configuration takes the profiling data and
generates optimized binaries.

The build_pgo.bat script automates the creation of optimized binaries. It
creates the PGI files, runs the unit test suite or PyBench with the PGI
python and finally creates the optimized files.

http://msdn.microsoft.com/en-us/library/e7k32f4k(VS.90).aspx

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

The PCbuild solution makes heavy use of Visual Studio property files
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
