Building Python using VC++ 7.1
-------------------------------------
This directory is used to build Python for Win32 platforms, e.g. Windows
95, 98 and NT.  It requires Microsoft Visual C++ 7.1
(a.k.a. Visual Studio .NET 2003).
(For other Windows platforms and compilers, see ../PC/readme.txt.)

All you need to do is open the workspace "pcbuild.sln" in MSVC++, select
the Debug or Release setting (using "Solution Configuration" from
the "Standard" toolbar"), and build the projects.

The proper order to build subprojects:

1) pythoncore (this builds the main Python DLL and library files,
               python25.{dll, lib} in Release mode)
              NOTE:  in previous releases, this subproject was
              named after the release number, e.g. python20.

2) python (this builds the main Python executable,
           python.exe in Release mode)

3) the other subprojects, as desired or needed (note:  you probably don't
   want to build most of the other subprojects, unless you're building an
   entire Python distribution from scratch, or specifically making changes
   to the subsystems they implement, or are running a Python core buildbot
   test slave; see SUBPROJECTS below)

When using the Debug setting, the output files have a _d added to
their name:  python25_d.dll, python_d.exe, parser_d.pyd, and so on.

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

The following subprojects will generally NOT build out of the box.  They
wrap code Python doesn't control, and you'll need to download the base
packages first and unpack them into siblings of PCbuilds's parent
directory; for example, if your PCbuild is  .......\dist\src\PCbuild\,
unpack into new subdirectories of dist\.

_tkinter
    Python wrapper for the Tk windowing system.  Requires building
    Tcl/Tk first.  Following are instructions for Tcl/Tk 8.4.12.

    Get source
    ----------
    In the dist directory, run
    svn export http://svn.python.org/projects/external/tcl8.4.12
    svn export http://svn.python.org/projects/external/tk8.4.12
    svn export http://svn.python.org/projects/external/tix-8.4.0

    Build Tcl first (done here w/ MSVC 7.1 on Windows XP)
    ---------------
    Use "Start -> All Programs -> Microsoft Visual Studio .NET 2003
         -> Visual Studio .NET Tools -> Visual Studio .NET 2003 Command Prompt"
    to get a shell window with the correct environment settings
    cd dist\tcl8.4.12\win
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
    cd dist\tk8.4.12\win
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.12
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.12 INSTALLDIR=..\..\tcltk install

    XXX Should we compile with OPTS=threads?

    XXX Our installer copies a lot of stuff out of the Tcl/Tk install
    XXX directory.  Is all of that really needed for Python use of Tcl/Tk?

    Optional:  run tests, via
        nmake -f makefile.vc TCLDIR=..\..\tcl8.4.12 test

        On WinXP Pro, wholly up to date as of 30-Aug-2004:
        all.tcl:        Total   8420    Passed  6826    Skipped 1581    Failed  13
        Sourced 91 Test Files.
        Files with failing tests: canvImg.test scrollbar.test textWind.test winWm.test

   Built Tix
   ---------
   cd dist\tix-8.4.0\win
   nmake -f python.mak
   nmake -f python.mak install

bz2
    Python wrapper for the libbz2 compression library.  Homepage
        http://sources.redhat.com/bzip2/
    Download the source from the python.org copy into the dist
    directory:

    svn export http://svn.python.org/projects/external/bzip2-1.0.3

    A custom pre-link step in the bz2 project settings should manage to
    build bzip2-1.0.3\libbz2.lib by magic before bz2.pyd (or bz2_d.pyd) is
    linked in PCbuild\.
    However, the bz2 project is not smart enough to remove anything under
    bzip2-1.0.3\ when you do a clean, so if you want to rebuild bzip2.lib
    you need to clean up bzip2-1.0.3\ by hand.

    The build step shouldn't yield any warnings or errors, and should end
    by displaying 6 blocks each terminated with
        FC: no differences encountered

    All of this managed to build bzip2-1.0.3\libbz2.lib, which the Python
    project links in.


_bsddb
    To use the version of bsddb that Python is built with by default, invoke
    (in the dist directory)

     svn export http://svn.python.org/projects/external/db-4.4.20


    Then open a VS.NET 2003 shell, and invoke:

       devenv db-4.4.20\build_win32\Berkeley_DB.sln /build Release /project db_static

    and do that a second time for a Debug build too:

       devenv db-4.4.20\build_win32\Berkeley_DB.sln /build Debug /project db_static

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
        dist\db-4.4.20\docs\index.html

    and follow the "Windows->Building Berkeley DB with Visual C++ .NET"
    instructions for building the Sleepycat
    software.  Note that Berkeley_DB.dsw is in the build_win32 subdirectory.
    Build the "db_static" project, for "Release" mode.

    To run extensive tests, pass "-u bsddb" to regrtest.py.  test_bsddb3.py
    is then enabled.  Running in verbose mode may be helpful.

    XXX The test_bsddb3 tests don't always pass, on Windows (according to
    XXX me) or on Linux (according to Barry).  (I had much better luck
    XXX on Win2K than on Win98SE.)  The common failure mode across platforms
    XXX is
    XXX     DBAgainError: (11, 'Resource temporarily unavailable -- unable
    XXX                         to join the environment')
    XXX
    XXX and it appears timing-dependent.  On Win2K I also saw this once:
    XXX
    XXX test02_SimpleLocks (bsddb.test.test_thread.HashSimpleThreaded) ...
    XXX Exception in thread reader 1:
    XXX Traceback (most recent call last):
    XXX File "C:\Code\python\lib\threading.py", line 411, in __bootstrap
    XXX    self.run()
    XXX File "C:\Code\python\lib\threading.py", line 399, in run
    XXX    apply(self.__target, self.__args, self.__kwargs)
    XXX File "C:\Code\python\lib\bsddb\test\test_thread.py", line 268, in
    XXX                  readerThread
    XXX    rec = c.next()
    XXX DBLockDeadlockError: (-30996, 'DB_LOCK_DEADLOCK: Locker killed
    XXX                                to resolve a deadlock')
    XXX
    XXX I'm told that DBLockDeadlockError is expected at times.  It
    XXX doesn't cause a test to fail when it happens (exceptions in
    XXX threads are invisible to unittest).

    Building for Win64:
    - open a VS.NET 2003 command prompt
    - run the SDK setenv.cmd script, passing /RETAIL and the target
      architecture (/SRV64 for Itanium, /X64 for AMD64)
    - build BerkeleyDB with the solution configuration matching the
      target ("Release IA64" for Itanium, "Release AMD64" for AMD64), e.g.
    devenv db-4.4.20\build_win32\Berkeley_DB.sln /build "Release AMD64" /project db_static /useenv


_ssl
    Python wrapper for the secure sockets library.

    Get the source code through

    svn export http://svn.python.org/projects/external/openssl-0.9.8a

    Alternatively, get the latest version from http://www.openssl.org.
    You can (theoretically) use any version of OpenSSL you like - the
    build process will automatically select the latest version.

    You must also install ActivePerl from
        http://www.activestate.com/Products/ActivePerl/
    as this is used by the OpenSSL build process.  Complain to them <wink>.

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

The project files support a ReleaseItanium configuration which creates
Win64/Itanium binaries. For this to work, you need to install the Platform
SDK, in particular the 64-bit support. This includes an Itanium compiler
(future releases of the SDK likely include an AMD64 compiler as well).
In addition, you need the Visual Studio plugin for external C compilers,
from http://sf.net/projects/vsextcomp. The plugin will wrap cl.exe, to
locate the proper target compiler, and convert compiler options
accordingly.

Building for AMD64
------------------

The build process for the ReleaseAMD64 configuration is very similar
to the Itanium configuration; make sure you use the latest version of
vsextcomp.

YOUR OWN EXTENSION DLLs
-----------------------
If you want to create your own extension module DLL, there's an example
with easy-to-follow instructions in ../PC/example/; read the file
readme.txt there first.
