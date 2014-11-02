Building Python using Microsoft Visual C++
------------------------------------------

This directory is used to build CPython for Microsoft Windows NT version
5.1 or higher (Windows XP, Windows Server 2003, or later) on 32 and 64
bit platforms.  Using this directory requires an installation of
Microsoft Visual C++ 2010 (MSVC 10.0) of any edition.  The specific
requirements are as follows:

Visual C++ 2010 Express Edition
    Required for building 32-bit Debug and Release configuration builds.
    The Python build solution pcbuild.sln makes use of Solution Folders,
    which this edition does not support.  Any time pcbuild.sln is opened
    or reloaded by Visual C++, a warning about Solution Folders will be
    displayed which can be safely dismissed with no impact on your
    ability to build Python.
Visual Studio 2010 Professional Edition
    Required for building 64-bit Debug and Release configuration builds
Visual Studio 2010 Premium Edition
    Required for building Release configuration builds that make use of
    Profile Guided Optimization (PGO), on either platform.

Installing Service Pack 1 for Visual Studio 2010 is highly recommended
to avoid LNK1123 errors.

All you need to do to build is open the solution "pcbuild.sln" in Visual
Studio, select the desired combination of configuration and platform,
then build with "Build Solution" or the F7 keyboard shortcut.  You can
also build from the command line using the "build.bat" script in this
directory.  The solution is configured to build the projects in the
correct order.

The solution currently supports two platforms.  The Win32 platform is
used to build standard x86-compatible 32-bit binaries, output into this
directory.  The x64 platform is used for building 64-bit AMD64 (aka
x86_64 or EM64T) binaries, output into the amd64 sub-directory which
will be created if it doesn't already exist.  The Itanium (IA-64)
platform is no longer supported.  See the "Building for AMD64" section
below for more information about 64-bit builds.

Four configuration options are supported by the solution:
Debug
    Used to build Python with extra debugging capabilities, equivalent
    to using ./configure --with-pydebug on UNIX.  All binaries built
    using this configuration have "_d" added to their name:
    python34_d.dll, python_d.exe, parser_d.pyd, and so on.  Both the
    build and rt (run test) batch files in this directory accept a -d
    option for debug builds.  If you are building Python to help with
    development of CPython, you will most likely use this configuration.
PGInstrument, PGUpdate
    Used to build Python in Release configuration using PGO, which
    requires Premium Edition of Visual Studio.  See the "Profile
    Guided Optimization" section below for more information.  Build
    output from each of these configurations lands in its own
    sub-directory of this directory.  The official Python releases are
    built using these configurations.
Release
    Used to build Python as it is meant to be used in production
    settings, though without PGO.


Legacy support
--------------

You can find build directories for older versions of Visual Studio and
Visual C++ in the PC directory. The legacy build directories are no
longer actively maintained and may not work out of the box.

Currently, the only legacy build directory is PC\VS9.0, for Visual
Studio 2008 (9.0).


C Runtime
---------

Visual Studio 2010 uses version 10 of the C runtime (MSVCRT10).  The
executables no longer use the "Side by Side" assemblies used in previous
versions of the compiler.  This simplifies distribution of applications.

The run time libraries are available under the VC/Redist folder of your
Visual Studio distribution. For more info, see the Readme in the
VC/Redist folder.


Sub-Projects
------------

The CPython project is split up into several smaller sub-projects which
are managed by the pcbuild.sln solution file.  Each sub-project is
represented by a .vcxproj and a .vcxproj.filters file starting with the
name of the sub-project.  These sub-projects fall into a few general
categories:

The following sub-projects represent the bare minimum required to build
a functioning CPython interpreter.  If nothing else builds but these,
you'll have a very limited but usable python.exe:
pythoncore
    .dll and .lib
python
    .exe
kill_python
    kill_python.exe, a small program designed to kill any instances of
    python(_d).exe that are running and live in the build output
    directory; this is meant to avoid build issues due to locked files
make_buildinfo, make_versioninfo
    helpers to provide necessary information to the build process

These sub-projects provide extra executables that are useful for running
CPython in different ways:
pythonw
    pythonw.exe, a variant of python.exe that doesn't open a Command
    Prompt window
pylauncher
    py.exe, the Python Launcher for Windows, see
        http://docs.python.org/3/using/windows.html#launcher
pywlauncher
    pyw.exe, a variant of py.exe that doesn't open a Command Prompt
    window
_testembed
    _testembed.exe, a small program that embeds Python for testing
    purposes, used by test_capi.py

These are miscellaneous sub-projects that don't really fit the other
categories.  By default, these projects do not build in Debug
configuration:
_freeze_importlib
    _freeze_importlib.exe, used to regenerate Python\importlib.h after
    changes have been made to Lib\importlib\_bootstrap.py
bdist_wininst
    ..\Lib\distutils\command\wininst-10.0[-amd64].exe, the base
    executable used by the distutils bdist_wininst command
python3dll
    python3.dll, the PEP 384 Stable ABI dll
xxlimited
    builds an example module that makes use of the PEP 384 Stable ABI,
    see Modules\xxlimited.c

The following sub-projects are for individual modules of the standard
library which are implemented in C; each one builds a DLL (renamed to
.pyd) of the same name as the project:
_ctypes
_ctypes_test
_decimal
_elementtree
_hashlib
_msi
_multiprocessing
_overlapped
_socket
_testcapi
_testbuffer
_testimportmultiple
pyexpat
select
unicodedata
winsound

The following Python-controlled sub-projects wrap external projects.
Note that these external libraries are not necessary for a working
interpreter, but they do implement several major features.  See the
"Getting External Sources" section below for additional information
about getting the source for building these libraries.  The sub-projects
are:
_bz2
    Python wrapper for version 1.0.6 of the libbzip2 compression library
    Homepage:
        http://www.bzip.org/
_lzma
    Python wrapper for the liblzma compression library, using pre-built
    binaries of XZ Utils version 5.0.5
    Homepage:
        http://tukaani.org/xz/
_ssl
    Python wrapper for version 1.0.1j of the OpenSSL secure sockets
    library, which is built by ssl.vcxproj
    Homepage:
        http://www.openssl.org/

    Building OpenSSL requires nasm.exe (the Netwide Assembler), version
    2.10 or newer from
        http://www.nasm.us/
    to be somewhere on your PATH.  More recent versions of OpenSSL may
    need a later version of NASM. If OpenSSL's self tests don't pass,
    you should first try to update NASM and do a full rebuild of
    OpenSSL.  If you use the Tools\buildbot\external(-amd64).bat method
    for getting sources, it also downloads a version of NASM which the
    ssl build script will add to PATH.

    If you like to use the official sources instead of the files from
    python.org's subversion repository, Perl is required to build the
    necessary makefiles and assembly files.  ActivePerl is available
    from
        http://www.activestate.com/activeperl/
    The svn.python.org version contains pre-built makefiles and assembly
    files.

    The build process makes sure that no patented algorithms are
    included.  For now RC5, MDC2 and IDEA are excluded from the build.
    You may have to manually remove $(OBJ_D)\i_*.obj from ms\nt.mak if
    using official sources; the svn.python.org-hosted version is already
    fixed.

    The ssl.vcxproj sub-project simply invokes PCbuild/build_ssl.py,
    which locates and builds OpenSSL.

    build_ssl.py attempts to catch the most common errors (such as not
    being able to find OpenSSL sources, or not being able to find a Perl
    that works with OpenSSL) and give a reasonable error message.  If
    you have a problem that doesn't seem to be handled correctly (e.g.,
    you know you have ActivePerl but we can't find it), please take a
    peek at build_ssl.py and suggest patches.  Note that build_ssl.py
    should be able to be run directly from the command-line.

    The ssl sub-project does not have the ability to clean the OpenSSL
    build; if you need to rebuild, you'll have to clean it by hand.
_sqlite3
    Wraps SQLite 3.8.3.1, which is itself built by sqlite3.vcxproj
    Homepage:
        http://www.sqlite.org/
_tkinter
    Wraps version 8.6.1 of the Tk windowing system.
    Homepage:
        http://www.tcl.tk/

    Unlike the other external libraries listed above, Tk must be built
    separately before the _tkinter module can be built. This means that
    a pre-built Tcl/Tk installation is expected in ..\externals\tcltk
    (tcltk64 for 64-bit) relative to this directory.  See "Getting
    External Sources" below for the easiest method to ensure Tcl/Tk is
    built.


Getting External Sources
------------------------

The last category of sub-projects listed above wrap external projects
Python doesn't control, and as such a little more work is required in
order to download the relevant source files for each project before they
can be built.  The buildbots must ensure that all libraries are present
before building, so the easiest approach is to run either external.bat
or external-amd64.bat (depending on platform) in the ..\Tools\buildbot
directory from ..\, i.e.:

    C:\python\cpython\PCbuild>cd ..
    C:\python\cpython>Tools\buildbot\external.bat

This extracts all the external sub-projects from
    http://svn.python.org/projects/external
via Subversion (so you'll need an svn.exe on your PATH) and places them
in ..\externals (relative to this directory).

It is also possible to download sources from each project's homepage,
though you may have to change the names of some folders in order to make
things work.  For instance, if you were to download a version 5.0.7 of
XZ Utils, you would need to extract the archive into ..\externals\xz-5.0.5
anyway, since that is where the solution is set to look for xz.  The
same is true for all other external projects.

The external(-amd64).bat scripts will also build a debug build of
Tcl/Tk, but there aren't any equivalent batch files for building release
versions of Tcl/Tk currently available.  If you need to build a release
version of Tcl/Tk, just take a look at the relevant external(-amd64).bat
file and find the two nmake lines, then call each one without the
'DEBUG=1' parameter, i.e.:

The external-amd64.bat file contains this for tcl:
    nmake -f makefile.vc DEBUG=1 MACHINE=AMD64 INSTALLDIR=..\..\tcltk64 clean all install

So for a release build, you'd call it as:
    nmake -f makefile.vc MACHINE=AMD64 INSTALLDIR=..\..\tcltk64 clean all install

Note that the above command is called from within ..\externals\tcl-8.6.1.0\win
(relative to this directory); don't forget to build Tk as well as Tcl!

This will be cleaned up in the future; http://bugs.python.org/issue15968
tracks adding a new tcltk.vcxproj file that will build Tcl/Tk and Tix
the same way the other external projects listed above are built.


Building for AMD64
------------------

The build process for AMD64 / x64 is very similar to standard builds,
you just have to set x64 as platform. In addition, the HOST_PYTHON
environment variable must point to a Python interpreter (at least 2.4),
to support cross-compilation from Win32.  Note that Visual Studio
requires Professional Edition or better in order to build 64-bit
binaries.


Profile Guided Optimization
---------------------------

The solution has two configurations for PGO. The PGInstrument
configuration must be built first. The PGInstrument binaries are linked
against a profiling library and contain extra debug information. The
PGUpdate configuration takes the profiling data and generates optimized
binaries.

The build_pgo.bat script automates the creation of optimized binaries.
It creates the PGI files, runs the unit test suite or PyBench with the
PGI python, and finally creates the optimized files.

See
    http://msdn.microsoft.com/en-us/library/e7k32f4k(VS.100).aspx
for more on this topic.


Static library
--------------

The solution has no configuration for static libraries. However it is
easy to build a static library instead of a DLL. You simply have to set
the "Configuration Type" to "Static Library (.lib)" and alter the
preprocessor macro "Py_ENABLE_SHARED" to "Py_NO_ENABLE_SHARED". You may
also have to change the "Runtime Library" from "Multi-threaded DLL
(/MD)" to "Multi-threaded (/MT)".


Visual Studio properties
------------------------

The PCbuild solution makes heavy use of Visual Studio property files
(*.props). The properties can be viewed and altered in the Property
Manager (View -> Other Windows -> Property Manager).

The property files used are (+-- = "also imports"):
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
 * sqlite3 (used only by sqlite3.vcxproj)
 * x64 (AMD64 / x64 platform specific settings)

The pyproject property file defines _WIN32 and x64 defines _WIN64 and
_M_X64 although the macros are set by the compiler, too. The GUI doesn't
always know about the macros and confuse the user with false
information.


Your Own Extension DLLs
-----------------------

If you want to create your own extension module DLL (.pyd), there's an
example with easy-to-follow instructions in ..\PC\example\; read the
file readme.txt there first.
