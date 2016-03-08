Quick Start Guide
-----------------

1.  Install Microsoft Visual Studio 2008, any edition.
2.  Install Microsoft Visual Studio 2010, any edition, or Windows SDK 7.1
    and any version of Microsoft Visual Studio newer than 2010.
3.  Install Subversion, and make sure 'svn.exe' is on your PATH.
4.  Run "build.bat -e" to build Python in 32-bit Release configuration.
5.  (Optional, but recommended) Run the test suite with "rt.bat -q".


Building Python using MSVC 9.0 via MSBuild
------------------------------------------

This directory is used to build Python for Win32 and x64 platforms, e.g.
Windows 2000 and later.  In order to use the project files in this
directory, you must have installed the MSVC 9.0 compilers, the v90
PlatformToolset project files for MSBuild, and MSBuild version 4.0 or later.
The easiest way to make sure you have all of these components is to install
Visual Studio 2008 and Visual Studio 2010.  Another configuration proven
to work is Visual Studio 2008, Windows SDK 7.1, and Visual Studio 2013.

If you only have Visual Studio 2008 available, use the project files in
../PC/VS9.0 which are fully supported and specifically for VS 2008.

If you do not have Visual Studio 2008 available, you can use these project
files to build using a different version of MSVC.  For example, use

   PCbuild\build.bat "/p:PlatformToolset=v100"

to build using MSVC10 (Visual Studio 2010).

***WARNING***
Building Python 2.7 for Windows using any toolchain that doesn't link
against MSVCRT90.dll is *unsupported* as the resulting python.exe will
not be able to use precompiled extension modules that do link against
MSVCRT90.dll.

For other Windows platforms and compilers, see ../PC/readme.txt.

All you need to do to build is open the solution "pcbuild.sln" in Visual
Studio, select the desired combination of configuration and platform,
then build with "Build Solution".  You can also build from the command
line using the "build.bat" script in this directory; see below for
details.  The solution is configured to build the projects in the correct
order.

The solution currently supports two platforms.  The Win32 platform is
used to build standard x86-compatible 32-bit binaries, output into this
directory.  The x64 platform is used for building 64-bit AMD64 (aka
x86_64 or EM64T) binaries, output into the amd64 sub-directory.  The
Itanium (IA-64) platform is no longer supported.

Four configuration options are supported by the solution:
Debug
    Used to build Python with extra debugging capabilities, equivalent
    to using ./configure --with-pydebug on UNIX.  All binaries built
    using this configuration have "_d" added to their name:
    python27_d.dll, python_d.exe, parser_d.pyd, and so on.  Both the
    build and rt (run test) batch files in this directory accept a -d
    option for debug builds.  If you are building Python to help with
    development of CPython, you will most likely use this configuration.
PGInstrument, PGUpdate
    Used to build Python in Release configuration using PGO, which
    requires Professional Edition of Visual Studio 2008.  See the
    "Profile Guided Optimization" section below for more information.
    Build output from each of these configurations lands in its own
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
which contains a usable version of MSBuild.

By default, build.bat will build Python in Release configuration for
the 32-bit Win32 platform.  It accepts several arguments to change
this behavior, try `build.bat -h` to learn more.


Legacy support
--------------

You can find build directories for older versions of Visual Studio and
Visual C++ in the PC directory.  The project files in PC/VS9.0/ are
specific to Visual Studio 2008, and will be fully supported for the life
of Python 2.7.

The following legacy build directories are no longer maintained and may
not work out of the box.

PC/VC6/
    Visual C++ 6.0
PC/VS7.1/
    Visual Studio 2003 (7.1)
PC/VS8.0/
    Visual Studio 2005 (8.0)


C Runtime
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

The following sub-projects are for individual modules of the standard
library which are implemented in C; each one builds a DLL (renamed to
.pyd) of the same name as the project:
_ctypes
_ctypes_test
_elementtree
_hashlib
_msi
_multiprocessing
_socket
_testcapi
pyexpat
select
unicodedata
winsound

There is also a w9xpopen project to build w9xpopen.exe, which is used
for platform.popen() on platforms whose COMSPEC points to 'command.com'.

The following Python-controlled sub-projects wrap external projects.
Note that these external libraries are not necessary for a working
interpreter, but they do implement several major features.  See the
"Getting External Sources" section below for additional information
about getting the source for building these libraries.  The sub-projects
are:
_bsddb
    Python wrapper for Berkeley DB version 4.7.25.
    Homepage:
        http://www.oracle.com/us/products/database/berkeley-db/
_bz2
    Python wrapper for version 1.0.6 of the libbzip2 compression library
    Homepage:
        http://www.bzip.org/
_ssl
    Python wrapper for version 1.0.2g of the OpenSSL secure sockets
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
    Wraps SQLite 3.6.21, which is itself built by sqlite3.vcxproj
    Homepage:
        http://www.sqlite.org/
_tkinter
    Wraps version 8.5.15 of the Tk windowing system.
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

The get_externals.bat script is called automatically by build.bat when
you pass the '-e' option to it.


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
    http://msdn.microsoft.com/en-us/library/e7k32f4k(VS.90).aspx
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
