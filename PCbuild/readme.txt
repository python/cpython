Quick Start Guide
-----------------

1.  Install Microsoft Visual Studio 2015, any edition.
2.  Install Subversion, and make sure 'svn.exe' is on your PATH.
3.  Run "build.bat -e" to build Python in 32-bit Release configuration.
4.  (Optional, but recommended) Run the test suite with "rt.bat -q".


Building Python using Microsoft Visual C++
------------------------------------------

This directory is used to build CPython for Microsoft Windows NT version
6.0 or higher (Windows Vista, Windows Server 2008, or later) on 32 and 64
bit platforms.  Using this directory requires an installation of
Microsoft Visual C++ 2015 (MSVC 14.0) of any edition.  The specific
requirements are as follows:

Visual Studio Express 2015 for Desktop
Visual Studio Professional 2015
    Either edition is sufficient for building all configurations except
    for Profile Guided Optimization.
    The Python build solution pcbuild.sln makes use of Solution Folders,
    which this edition does not support.  Any time pcbuild.sln is opened
    or reloaded by Visual Studio, a warning about Solution Folders will
    be displayed, which can be safely dismissed with no impact on your
    ability to build Python.
    Required for building 64-bit Debug and Release configuration builds
Visual Studio Premium 2015
    Required for building Release configuration builds that make use of
    Profile Guided Optimization (PGO), on either platform.

All you need to do to build is open the solution "pcbuild.sln" in Visual
Studio, select the desired combination of configuration and platform,
then build with "Build Solution".  You can also build from the command
line using the "build.bat" script in this directory; see below for
details.  The solution is configured to build the projects in the correct
order.

The solution currently supports two platforms.  The Win32 platform is
used to build standard x86-compatible 32-bit binaries, output into the
win32 sub-directory.  The x64 platform is used for building 64-bit AMD64
(aka x86_64 or EM64T) binaries, output into the amd64 sub-directory.
The Itanium (IA-64) platform is no longer supported.  See the "Building
for AMD64" section below for more information about 64-bit builds.

Four configuration options are supported by the solution:
Debug
    Used to build Python with extra debugging capabilities, equivalent
    to using ./configure --with-pydebug on UNIX.  All binaries built
    using this configuration have "_d" added to their name:
    python35_d.dll, python_d.exe, parser_d.pyd, and so on.  Both the
    build and rt (run test) batch files in this directory accept a -d
    option for debug builds.  If you are building Python to help with
    development of CPython, you will most likely use this configuration.
PGInstrument, PGUpdate
    Used to build Python in Release configuration using PGO, which
    requires Premium Edition of Visual Studio.  See the "Profile
    Guided Optimization" section below for more information.  Build
    output from each of these configurations lands in its own
    sub-directory of this directory.  The official Python releases may
    be built using these configurations.
Release
    Used to build Python as it is meant to be used in production
    settings, though without PGO.


Building Python using the build.bat script
----------------------------------------------

In this directory you can find build.bat, a script designed to make
building Python on Windows simpler.  This script will use the env.bat
script to detect one of Visual Studio 2015, 2013, 2012, or 2010, any of
which may be used to build Python, though only Visual Studio 2015 is
officially supported.

By default, build.bat will build Python in Release configuration for
the 32-bit Win32 platform.  It accepts several arguments to change
this behavior:

   -c <configuration>  Set the configuration (see above)
   -d                  Shortcut for "-c Debug"
   -p <platform>       Set the platform to build for ("Win32" or "x64")
   -r                  Rebuild instead of just building
   -t <target>         Set the target (Build, Rebuild, Clean or CleanAll)
   -e                  Use get_externals.bat to fetch external sources
   -M                  Don't build in parallel
   -v                  Increased output messages

Up to 9 MSBuild switches can also be passed, though they must be passed
after specifying any of the above switches.  For example, use:

   build.bat -e -d /fl

to do a debug build with externals fetched as needed and write detailed
build logs to a file.  If the MSBuild switch requires an equal sign
("="), the entire switch must be quoted:

   build.bat -e -d "/p:ExternalsDir=P:\cpython-externals"

There may also be other situations where quotes are necessary.


C Runtime
---------

Visual Studio 2015 uses version 14 of the C runtime (MSVCRT14).  The
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
categories:
_freeze_importlib
    _freeze_importlib.exe, used to regenerate Python\importlib.h after
    changes have been made to Lib\importlib\_bootstrap.py
bdist_wininst
    ..\Lib\distutils\command\wininst-14.0[-amd64].exe, the base
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
    Python wrapper for version 1.0.2d of the OpenSSL secure sockets
    library, which is built by ssl.vcxproj
    Homepage:
        http://www.openssl.org/

    Building OpenSSL requires nasm.exe (the Netwide Assembler), version
    2.10 or newer from
        http://www.nasm.us/
    to be somewhere on your PATH.  More recent versions of OpenSSL may
    need a later version of NASM. If OpenSSL's self tests don't pass,
    you should first try to update NASM and do a full rebuild of
    OpenSSL.  If you use the PCbuild\get_externals.bat method
    for getting sources, it also downloads a version of NASM which the
    libeay/ssleay sub-projects use.

    The libeay/ssleay sub-projects expect your OpenSSL sources to have
    already been configured and be ready to build.  If you get your sources
    from svn.python.org as suggested in the "Getting External Sources"
    section below, the OpenSSL source will already be ready to go.  If
    you want to build a different version, you will need to run

       PCbuild\prepare_ssl.py path\to\openssl-source-dir

    That script will prepare your OpenSSL sources in the same way that
    those available on svn.python.org have been prepared.  Note that
    Perl must be installed and available on your PATH to configure
    OpenSSL.  ActivePerl is recommended and is available from
        http://www.activestate.com/activeperl/

    The libeay and ssleay sub-projects will build the modules of OpenSSL
    required by _ssl and _hashlib and may need to be manually updated when
    upgrading to a newer version of OpenSSL or when adding new
    functionality to _ssl or _hashlib. They will not clean up their output
    with the normal Clean target; CleanAll should be used instead.
_sqlite3
    Wraps SQLite 3.8.11.0, which is itself built by sqlite3.vcxproj
    Homepage:
        http://www.sqlite.org/
_tkinter
    Wraps version 8.6.4 of the Tk windowing system.
    Homepage:
        http://www.tcl.tk/

    Tkinter's dependencies are built by the tcl.vcxproj and tk.vcxproj
    projects.  The tix.vcxproj project also builds the Tix extended
    widget set for use with Tkinter.

    Those three projects install their respective components in a
    directory alongside the source directories called "tcltk" on
    Win32 and "tcltk64" on x64.  They also copy the Tcl and Tk DLLs
    into the current output directory, which should ensure that Tkinter
    is able to load Tcl/Tk without having to change your PATH.

    The tcl, tk, and tix sub-projects do not clean their builds with
    the normal Clean target; if you need to rebuild, you should use the
    CleanAll target or manually delete their builds.


Getting External Sources
------------------------

The last category of sub-projects listed above wrap external projects
Python doesn't control, and as such a little more work is required in
order to download the relevant source files for each project before they
can be built.  However, a simple script is provided to make this as
painless as possible, called "get_externals.bat" and located in this
directory.  This script extracts all the external sub-projects from
    http://svn.python.org/projects/external
via Subversion (so you'll need svn.exe on your PATH) and places them
in ..\externals (relative to this directory).

It is also possible to download sources from each project's homepage,
though you may have to change folder names or pass the names to MSBuild
as the values of certain properties in order for the build solution to
find them.  This is an advanced topic and not necessarily fully
supported.


Building for AMD64
------------------

The build process for AMD64 / x64 is very similar to standard builds,
you just have to set x64 as platform. In addition, the HOST_PYTHON
environment variable must point to a Python interpreter (at least 2.4),
to support cross-compilation from Win32.


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

The PCbuild solution makes use of Visual Studio property files (*.props)
to simplify each project. The properties can be viewed in the Property
Manager (View -> Other Windows -> Property Manager) but should be
carefully modified by hand.

The property files used are:
 * python (versions, directories and build names)
 * pyproject (base settings for all projects)
 * openssl (used by libeay and ssleay projects)
 * tcltk (used by _tkinter, tcl, tk and tix projects)

The pyproject property file defines all of the build settings for each
project, with some projects overriding certain specific values. The GUI
doesn't always reflect the correct settings and may confuse the user
with false information, especially for settings that automatically adapt
for diffirent configurations.


Your Own Extension DLLs
-----------------------

If you want to create your own extension module DLL (.pyd), there's an
example with easy-to-follow instructions in ..\PC\example\; read the
file readme.txt there first.
