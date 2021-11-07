Quick Start Guide
-----------------

1.  Install Microsoft Visual Studio 2017 with Python workload and
    Python native development component.
1a. Optionally install Python 3.6 or later.  If not installed,
    get_externals.bat (via build.bat) will download and use Python via
    NuGet.
2.  Run "build.bat" to build Python in 32-bit Release configuration.
3.  (Optional, but recommended) Run the test suite with "rt.bat -q".


Building Python using Microsoft Visual C++
------------------------------------------

This directory is used to build CPython for Microsoft Windows NT version
6.0 or higher (Windows Vista, Windows Server 2008, or later) on 32 and 64
bit platforms.  Using this directory requires an installation of
Microsoft Visual Studio 2017 (MSVC 14.1) with the *Python workload* and
its optional *Python native development* component selected. (For
command-line builds, Visual Studio 2015 may also be used.)

Building from the command line is recommended in order to obtain any
external dependencies. To build, simply run the "build.bat" script without
any arguments. After this succeeds, you can open the "pcbuild.sln"
solution in Visual Studio to continue development.

To build an installer package, refer to the README in the Tools/msi folder.

The solution currently supports two platforms.  The Win32 platform is
used to build standard x86-compatible 32-bit binaries, output into the
win32 sub-directory.  The x64 platform is used for building 64-bit AMD64
(aka x86_64 or EM64T) binaries, output into the amd64 sub-directory.
The Itanium (IA-64) platform is no longer supported.

Four configuration options are supported by the solution:
Debug
    Used to build Python with extra debugging capabilities, equivalent
    to using ./configure --with-pydebug on UNIX.  All binaries built
    using this configuration have "_d" added to their name:
    python310_d.dll, python_d.exe, parser_d.pyd, and so on.  Both the
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
script to detect either Visual Studio 2017 or 2015, either of
which may be used to build Python. Currently Visual Studio 2017 is
officially supported.

By default, build.bat will build Python in Release configuration for
the 32-bit Win32 platform.  It accepts several arguments to change
this behavior, try `build.bat -h` to learn more.


C Runtime
---------

Visual Studio 2017 uses version 14.0 of the C runtime (vcruntime140).
The executables no longer use the "Side by Side" assemblies used in
previous versions of the compiler.  This simplifies distribution of
applications.

The run time libraries are available under the redist folder of your
Visual Studio distribution. For more info, see the Readme in the
redist folder.


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
pyshellext
    pyshellext.dll, the shell extension deployed with the launcher
python3dll
    python3.dll, the PEP 384 Stable ABI dll
xxlimited
    builds an example module that makes use of the PEP 384 Stable ABI,
    see Modules\xxlimited.c
xxlimited_35
    ditto for testing the Python 3.5 stable ABI, see
    Modules\xxlimited_35.c

The following sub-projects are for individual modules of the standard
library which are implemented in C; each one builds a DLL (renamed to
.pyd) of the same name as the project:
_asyncio
_ctypes
_ctypes_test
_zoneinfo
_decimal
_elementtree
_hashlib
_msi
_multiprocessing
_overlapped
_socket
_testbuffer
_testcapi
_testconsole
_testimportmultiple
_testmultiphase
_tkinter
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
    Python wrapper for version 5.2.2 of the liblzma compression library
    Homepage:
        http://tukaani.org/xz/
_ssl
    Python wrapper for version 1.1.1i of the OpenSSL secure sockets
    library, which is downloaded from our binaries repository at
    https://github.com/python/cpython-bin-deps.

    Homepage:
        http://www.openssl.org/

    Building OpenSSL requires Perl on your path, and can be performed by
    running PCbuild\prepare_ssl.bat. This will retrieve the version of
    the sources matched to the current commit from the OpenSSL branch
    in our source repository at
    https://github.com/python/cpython-source-deps.

    To use an alternative build of OpenSSL completely, you should replace
    the files in the externals/openssl-bin-<version> folder with your own.
    As long as this folder exists, its contents will not be downloaded
    again when building.

_sqlite3
    Wraps SQLite 3.34.0, which is itself built by sqlite3.vcxproj
    Homepage:
        http://www.sqlite.org/
_tkinter
    Wraps version 8.6.6 of the Tk windowing system, which is downloaded
    from our binaries repository at
    https://github.com/python/cpython-bin-deps.

    Homepage:
        http://www.tcl.tk/

    Building Tcl and Tk can be performed by running
    PCbuild\prepare_tcltk.bat. This will retrieve the version of the
    sources matched to the current commit from the Tcl and Tk branches
    in our source repository at
    https://github.com/python/cpython-source-deps.

    The two projects install their respective components in a
    directory alongside the source directories called "tcltk" on
    Win32 and "tcltk64" on x64.  They also copy the Tcl and Tk DLLs
    into the current output directory, which should ensure that Tkinter
    is able to load Tcl/Tk without having to change your PATH.


Getting External Sources
------------------------

The last category of sub-projects listed above wrap external projects
Python doesn't control, and as such a little more work is required in
order to download the relevant source files for each project before they
can be built.  However, a simple script is provided to make this as
painless as possible, called "get_externals.bat" and located in this
directory.  This script extracts all the external sub-projects from
    https://github.com/python/cpython-source-deps
and
    https://github.com/python/cpython-bin-deps
via a Python script called "get_external.py", located in this directory.
If Python 3.6 or later is not available via the "py.exe" launcher, the
path or command to use for Python can be provided in the PYTHON_FOR_BUILD
environment variable, or get_externals.bat will download the latest
version of NuGet and use it to download the latest "pythonx86" package
for use with get_external.py.  Everything downloaded by these scripts is
stored in ..\externals (relative to this directory).

It is also possible to download sources from each project's homepage,
though you may have to change folder names or pass the names to MSBuild
as the values of certain properties in order for the build solution to
find them.  This is an advanced topic and not necessarily fully
supported.

The get_externals.bat script is called automatically by build.bat
unless you pass the '-E' option.


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
    http://msdn.microsoft.com/en-us/library/e7k32f4k(VS.140).aspx
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
 * openssl (used by projects dependent upon OpenSSL)
 * tcltk (used by _tkinter, tcl, tk and tix projects)

The pyproject property file defines all of the build settings for each
project, with some projects overriding certain specific values. The GUI
doesn't always reflect the correct settings and may confuse the user
with false information, especially for settings that automatically adapt
for different configurations.
