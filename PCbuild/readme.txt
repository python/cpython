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
their name:  python24_d.dll, python_d.exe, parser_d.pyd, and so on.

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
    Tcl/Tk first.  Following are instructions for Tcl/Tk 8.4.7; these
    should work for version 8.4.6 too, with suitable substitutions:

    Get source
    ----------
    Go to
        http://prdownloads.sourceforge.net/tcl/
    and download
        tcl847-src.zip
        tk847-src.zip
    Unzip into
        dist\tcl8.4.7\
        dist\tk8.4.7\
    respectively.

    Build Tcl first (done here w/ MSVC 7.1 on Windows XP)
    ---------------
    Use "Start -> All Programs -> Microsoft Visual Studio .NET 2003
         -> Visual Studio .NET Tools -> Visual Studio .NET 2003 Command Prompt"
    to get a shell window with the correct environment settings
    cd dist\tcl8.4.7\win
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
    cd dist\tk8.4.7\win
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.7
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.7 INSTALLDIR=..\..\tcltk install

    XXX Should we compile with OPTS=threads?

    XXX Our installer copies a lot of stuff out of the Tcl/Tk install
    XXX directory.  Is all of that really needed for Python use of Tcl/Tk?

    Optional:  run tests, via
        nmake -f makefile.vc TCLDIR=..\..\tcl8.4.7 test

        On WinXP Pro, wholly up to date as of 30-Aug-2004:
        all.tcl:        Total   8420    Passed  6826    Skipped 1581    Failed  13
        Sourced 91 Test Files.
        Files with failing tests: canvImg.test scrollbar.test textWind.test winWm.test
        
   Built Tix
   ---------
   Download from http://prdownloads.sourceforge.net/tix/tix-8.1.4.tar.gz
   cd dist\tix-8.1.4
   [cygwin]patch -p1 < ..\..\python\PC\tix.diff
   cd win
   nmake -f makefile.vc
   nmake -f makefile.vc install

zlib
    Python wrapper for the zlib compression library.  Get the source code
    for version 1.2.1 from a convenient mirror at:
        http://www.gzip.org/zlib/
    Unpack into dist\zlib-1.2.1.
    A custom pre-link step in the zlib project settings should manage to
    build zlib-1.2.1\zlib.lib by magic before zlib.pyd (or zlib_d.pyd) is
    linked in PCbuild\.
    However, the zlib project is not smart enough to remove anything under
    zlib-1.2.1\ when you do a clean, so if you want to rebuild zlib.lib
    you need to clean up zlib-1.2.1\ by hand.

bz2
    Python wrapper for the libbz2 compression library.  Homepage
        http://sources.redhat.com/bzip2/
    Download the source tarball, bzip2-1.0.2.tar.gz.
    Unpack into dist\bzip2-1.0.2.  WARNING:  If you're using WinZip, you
    must disable its "TAR file smart CR/LF conversion" feature (under
    Options -> Configuration -> Miscellaneous -> Other) for the duration.

    A custom pre-link step in the bz2 project settings should manage to
    build bzip2-1.0.2\libbz2.lib by magic before bz2.pyd (or bz2_d.pyd) is
    linked in PCbuild\.
    However, the bz2 project is not smart enough to remove anything under
    bzip2-1.0.2\ when you do a clean, so if you want to rebuild bzip2.lib
    you need to clean up bzip2-1.0.2\ by hand.

    The build step shouldn't yield any warnings or errors, and should end
    by displaying 6 blocks each terminated with
        FC: no differences encountered
    If FC finds differences, see the warning abou WinZip above (when I
    first tried it, sample3.ref failed due to CRLF conversion).

    All of this managed to build bzip2-1.0.2\libbz2.lib, which the Python
    project links in.


_bsddb
    Go to Sleepycat's download page:
        http://www.sleepycat.com/download/

    and download version 4.2.52.

    With or without strong cryptography? You can choose either with or
    without strong cryptography, as per the instructions below.  By
    default, Python is built and distributed WITHOUT strong crypto.

    Unpack into the dist\. directory, ensuring you expand with folder names.

    If you downloaded with strong crypto, this will create a dist\db-4.2.52
    directory, and is ready to use.

    If you downloaded WITHOUT strong crypto, this will create a
    dist\db-4.2.52.NC directory - this directory should be renamed to
    dist\db-4.2.52 before use.

    As of 11-Apr-2004, you also need to download and manually apply two
    patches before proceeding (and the sleepycat download page tells you
    about this).  Cygwin patch worked for me.  cd to dist\db-4.2.52 and
    use "patch -p0 < patchfile" once for each downloaded patchfile.

    Open
        dist\db-4.2.52\docs\index.html

    and follow the "Windows->Building Berkeley DB with Visual C++ .NET"
    instructions for building the Sleepycat
    software.  Note that Berkeley_DB.dsw is in the build_win32 subdirectory.
    Build the "Release Static" version.

    XXX We're linking against Release_static\libdb42s.lib.
    XXX This yields the following warnings:
"""
Compiling...
_bsddb.c
Linking...
   Creating library ./_bsddb.lib and object ./_bsddb.exp
_bsddb.obj : warning LNK4217: locally defined symbol _malloc imported in function __db_associateCallback
_bsddb.obj : warning LNK4217: locally defined symbol _free imported in function __DB_consume
_bsddb.obj : warning LNK4217: locally defined symbol _fclose imported in function _DB_verify
_bsddb.obj : warning LNK4217: locally defined symbol _fopen imported in function _DB_verify
_bsddb.obj : warning LNK4217: locally defined symbol _strncpy imported in function _init_pybsddb
__bsddb - 0 error(s), 5 warning(s)
"""
    XXX This isn't encouraging, but I don't know what to do about it.

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

    XXX 11-Apr-2004 tim
    XXX On WinXP Pro, I got one failure from test_bsddb3:
    XXX
    XXX ERROR: test04_n_flag (bsddb.test.test_compat.CompatibilityTestCase)
    XXX Traceback (most recent call last):
    XXX  File "C:\Code\python\lib\bsddb\test\test_compat.py", line 86, in test04_n_flag
    XXX    f = hashopen(self.filename, 'n')
    XXX File "C:\Code\python\lib\bsddb\__init__.py", line 293, in hashopen
    XXX    d.open(file, db.DB_HASH, flags, mode)
    XXX DBInvalidArgError: (22, 'Invalid argument -- DB_TRUNCATE illegal with locking specified')

_ssl
    Python wrapper for the secure sockets library.

    Get the latest source code for OpenSSL from
        http://www.openssl.org

    You (probably) don't want the "engine" code.  For example, get
        openssl-0.9.7d.tar.gz
    not
        openssl-engine-0.9.7d.tar.gz

    Unpack into the "dist" directory, retaining the folder name from
    the archive - for example, the latest stable OpenSSL will install as
        dist/openssl-0.9.7d

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

The Itanium build has seen little testing. The SDK compiler reports a lot
of warnings about conversion from size_t to int, which will be fixed in
future Python releases.

YOUR OWN EXTENSION DLLs
-----------------------
If you want to create your own extension module DLL, there's an example
with easy-to-follow instructions in ../PC/example/; read the file
readme.txt there first.
