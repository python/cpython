Quick Start Guide
-----------------

1.  Install Microsoft Visual Studio 2017 or later with Python workload and
    Python native development component.
1a. Optionally install Python 3.10 or later.  If not installed,
    get_externals.bat (via build.bat) will download and use Python via
    NuGet.
2.  Run "build.bat" to build Python in 32-bit Release configuration.
3.  (Optional, but recommended) Run the test suite with "rt.bat -q".


Building Python using Microsoft Visual C++
------------------------------------------

This directory is used to build CPython for Microsoft Windows on 32- and 64-
bit platforms.  Using this directory requires an installation of
Microsoft Visual Studio (MSVC) with the *Python workload* and
its optional *Python native development* component selected.

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


Building Python using Clang/LLVM
--------------------------------

See https://learn.microsoft.com/cpp/build/clang-support-msbuild
for how to install and use clang-cl bundled with Microsoft Visual Studio.
You can use the IDE to switch to clang-cl for local development,
but because this alters the *.vcxproj files, the recommended way is
to use build.bat:

build.bat "/p:PlatformToolset=ClangCL"

All other build.bat options continue to work as with MSVC, so this
will create a 64bit release binary.

You can also use a specific version of clang-cl downloaded from
https://github.com/llvm/llvm-project/releases, e.g.
clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz.
Given you have extracted that to <my-clang-dir>, you can use it like so
build.bat --pgo "/p:PlatformToolset=ClangCL" "/p:LLVMInstallDir=<my-clang-dir>" "/p:LLVMToolsVersion=18"

Setting LLVMToolsVersion to the major version is enough, although you
can be specific and use 18.1.8 in the above example, too.

Use the --pgo option to build with PGO (Profile Guided Optimization).

However, if you want to run the PGO task
on a different host than the build host, you must pass
"/p:CLANG_PROFILE_PATH=<relative-path-to-instrumented-dir-on-remote-host>"
in the PGInstrument step to make sure the profile data is generated
into the instrumented directory when running the PGO task.
E.g., if you place the instrumented binaries into the folder
"workdir/instrumented" and then run the PGO task using "workdir"
as the current working directory, the usage is
"/p:CLANG_PROFILE_PATH=instrumented"

Like in the MSVC case, after fetching (or manually copying) the instrumented
folder back into your build tree, you can continue with the PGUpdate
step with no further parameters.

Building Python using the build.bat script
----------------------------------------------

In this directory you can find build.bat, a script designed to make
building Python on Windows simpler.  This script will use the env.bat
script to detect either Visual Studio 2017 or later, either of
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
        https://docs.python.org/3/using/windows.html#launcher
pywlauncher
    pyw.exe, a variant of py.exe that doesn't open a Command Prompt
    window
_testembed
    _testembed.exe, a small program that embeds Python for testing
    purposes, used by test_capi.py

These are miscellaneous sub-projects that don't really fit the other
categories:
_freeze_module
    _freeze_module.exe, used to regenerate frozen modules in Python
    after changes have been made to the corresponding source files
    (e.g. Lib\importlib\_bootstrap.py).
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
 * _asyncio
 * _ctypes
 * _ctypes_test
 * _decimal
 * _elementtree
 * _hashlib
 * _multiprocessing
 * _overlapped
 * _queue
 * _remote_debugging
 * _socket
 * _testbuffer
 * _testcapi
 * _testclinic
 * _testclinic_limited
 * _testconsole
 * _testimportmultiple
 * _testinternalcapi
 * _testlimitedcapi
 * _testmultiphase
 * _testsinglephase
 * _uuid
 * _wmi
 * _zoneinfo
 * pyexpat
 * select
 * unicodedata
 * winsound

The following Python-controlled sub-projects wrap external projects.
Note that these external libraries are not necessary for a working
interpreter, but they do implement several major features.  See the
"Getting External Sources" section below for additional information
about getting the source for building these libraries.  The sub-projects
are:

_bz2
    Python wrapper for version 1.0.8 of the libbzip2 compression library
    Homepage:
        http://www.bzip.org/

_lzma
    Python wrapper for version 5.2.2 of the liblzma compression library,
    which is itself built by liblzma.vcxproj.
    Homepage:
        https://tukaani.org/xz/

_ssl
    Python wrapper for version 3.0.15 of the OpenSSL secure sockets
    library, which is itself downloaded from our binaries repository at
    https://github.com/python/cpython-bin-deps and built by openssl.vcxproj.

    Homepage:
        https://www.openssl.org/

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
    Wraps SQLite 3.49.1, which is itself built by sqlite3.vcxproj
    Homepage:
        https://www.sqlite.org/

_tkinter
    Wraps version 8.6.15 of the Tk windowing system, which is downloaded
    from our binaries repository at
    https://github.com/python/cpython-bin-deps.

    Homepage:
        https://www.tcl.tk/

    Building Tcl and Tk can be performed by running
    PCbuild\prepare_tcltk.bat. This will retrieve the version of the
    sources matched to the current commit from the Tcl and Tk branches
    in our source repository at
    https://github.com/python/cpython-source-deps and build them via the
    tcl.vcxproj and tk.vcxproj sub-projects.

    The two projects install their respective components in a
    directory alongside the source directories called "tcltk" on
    Win32 and "tcltk64" on x64.  They also copy the Tcl and Tk DLLs
    into the current output directory, which should ensure that Tkinter
    is able to load Tcl/Tk without having to change your PATH.

_zstd
    Python wrapper for version 1.5.7 of the zstd compression library
    Homepage:
        https://facebook.github.io/zstd/

zlib-ng
    Compiles zlib-ng as a static library, which is later included by
    pythoncore.vcxproj. This was generated using CMake against zlib-ng
    version 2.2.4, and should be minimally updated as needed to adapt
    to changes in their source layout. The zbuild.h, zconf.h and
    zconf-ng.h file in the PC directory were likewise generated and
    vendored.

    Sources for zlib-ng are imported unmodified into our source
    repository at https://github.com/python/cpython-source-deps.
_zstd
    Python wrapper for version 1.5.7 of the Zstandard compression library
    Homepage:
        https://facebook.github.io/zstd/


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
Everything downloaded by these scripts is stored in ..\externals
(relative to this directory), or the path specified by the EXTERNALS_DIR
environment variable.

The path or command to use for Python can be provided with the
PYTHON_FOR_BUILD environment variable. If this is not set, an active
virtual environment will be used. If none is active, and HOST_PYTHON is
set to a recent enough version or "py.exe" is able to find a recent
enough version, those will be used. If all else fails, a copy of Python
will be downloaded from NuGet and extracted to the externals directory.
This will then be used for later builds (see PCbuild/find_python.bat
for the full logic).

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

The build.bat script has an argument `--pgo` that automate the creation
of optimized binaries.
It creates the PGI files, runs the unit test suite with the PGI python,
and finally creates the optimized files.
You can customize the job for profiling with `--pgo-job <job>` option.

See
    https://docs.microsoft.com/en-us/cpp/build/profile-guided-optimizations
for more on this topic.


Optimization flags
------------------

You can set optimization flags either via

* environment variables, for example:

    set WITH_COMPUTED_GOTOS=true

* or pass them as parameters to `build.bat`, for example:

    build.bat "/p:WITH_COMPUTED_GOTOS=true"

* or put them in `msbuild.rsp` in the `PCbuild` directory, one flag per line.

Supported flags are:

* WITH_COMPUTED_GOTOS: build the interpreter using "computed gotos".
  Currently only supported by clang-cl.


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
 * tcltk (used by _tkinter, tcl, and tk projects)

The pyproject property file defines all of the build settings for each
project, with some projects overriding certain specific values. The GUI
doesn't always reflect the correct settings and may confuse the user
with false information, especially for settings that automatically adapt
for different configurations.

Add a new project
-----------------

For example, add a new _testclinic_limited project to build a new
_testclinic_limited extension, the file Modules/_testclinic_limited.c:

* In PCbuild/, copy _testclinic.vcxproj to _testclinic_limited.vcxproj,
  replace RootNamespace value with `_testclinic_limited`, replace
  `_asyncio.c` with `_testclinic_limited.c`.
* In PCbuild/, copy _testclinic.vcxproj.filters to
  _testclinic_limited.vcxproj.filters, edit the list of files in the new file.
* Open Visual Studio, open PCbuild\pcbuild.sln solution, add the
  PCbuild\_testclinic_limited.vcxproj project to the solution ("add existing
  project).
* Add a dependency on the python project to the new _testclinic_limited
  project.
* Save and exit Visual Studio.
* Add `;_testclinic_limited` to `<TestModules Include="...">` in
  PCbuild\pcbuild.proj.
* Update "exts" in Tools\msi\lib\lib_files.wxs file or in
  Tools\msi\test\test_files.wxs file (for tests).
* PC\layout\main.py needs updating if you add a test-only extension whose name
  doesn't start with "_test".
* Add the extension to PCbuild\readme.txt (this file).
* Build Python from scratch (clean the solution) to check that the new project
  is built successfully.
* Ensure the new .vcxproj and .vcxproj.filters files are added to your commit,
  as well as the changes to pcbuild.sln, pcbuild.proj and any other modified
  files.
