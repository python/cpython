Building Python using VC++ 6.0 or 5.0
-------------------------------------
This directory is used to build Python for Win32 platforms, e.g. Windows
95, 98 and NT.  It requires Microsoft Visual C++ 6.x or 5.x.
(For other Windows platforms and compilers, see ../readme.txt.)

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
    Tcl/Tk first.  Following are instructions for Tcl/Tk 8.4.3:

    Get source
    ----------
    Go to
        http://prdownloads.sourceforge.net/tcl/
    and download
        tcl843-src.zip
        tk843-src.zip
    Unzip into
        dist\tcl8.4.3\
        dist\tk8.4.3\
    respectively.

    Build Tcl first (done here w/ MSVC 6 on Win98SE)
    ---------------
    cd dist\tcl8.4.3\win
    run vcvars32.bat [necessary even on Win2K]
    nmake -f makefile.vc
    nmake -f makefile.vc INSTALLDIR=..\..\tcl84 install

    XXX Should we compile with OPTS=threads?

    XXX Some tests failed in "nmake -f makefile.vc test".
    XXX all.tcl:  Total 10480   Passed 9743    Skipped 719     Failed 18
    XXX
    XXX That was on Win98SE.  On Win2K:
    XXX all.tcl   Total 10480   Passed 9781    Skipped 698     Failed  1

    Build Tk
    --------
    cd dist\tk8.4.3\win
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.3
    nmake -f makefile.vc TCLDIR=..\..\tcl8.4.3 INSTALLDIR=..\..\tcl84 install

    XXX Should we compile with OPTS=threads?

    XXX I have no idea whether "nmake -f makefile.vc test" passed or
    XXX failed.  It popped up tons of little windows, and did lots of
    XXX stuff, and nothing blew up.

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

    and download version 4.1.25.  The file name is db-4.1.25.NC.zip.
    XXX with or without strong cryptography?  I picked "without".

    Unpack into
        dist\db-4.1.25

    [If using WinZip to unpack the db-4.1.25.NC distro, that requires
     renaming the directory (to remove ".NC") after unpacking.
    ]

    Open
        dist\db-4.1.25\docs\index.html

    and follow the Windows instructions for building the Sleepycat
    software.  Note that Berkeley_DB.dsw is in the build_win32 subdirectory.
    Build the Release version ("build_all -- Win32 Release").

    XXX We're actually linking against Release_static\libdb41s.lib.
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

    To run extensive tests, pass "-u bsddb" to regrtest.py.  test_bsddb3.py
    is then enabled.  Running in verbose mode may be helpful.

    XXX The test_bsddb3 tests don't always pass, on Windows (according to
    XXX me) or on Linux (according to Barry).  I had much better luck
    XXX on Win2K than on Win98SE.  The common failure mode across platforms
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


_ssl
    Python wrapper for the secure sockets library.

    Get the latest source code for OpenSSL from
        http://www.openssl.org

    You (probably) don't want the "engine" code.  For example, get
        openssl-0.9.6g.tar.gz
    not
        openssl-engine-0.9.6g.tar.gz

    Unpack into the "dist" directory, retaining the folder name from
    the archive - for example, the latest stable OpenSSL will install as
        dist/openssl-0.9.6g

    You can (theoretically) use any version of OpenSSL you like - the
    build process will automatically select the latest version.

    You must also install ActivePerl from
        http://www.activestate.com/Products/ActivePerl/
    as this is used by the OpenSSL build process.  Complain to them <wink>.

    The MSVC project simply invokes PCBuild/build_ssl.py to perform
    the build.  This Python script locates and builds your OpenSSL
    installation, then invokes a simple makefile to build the final .pyd.

    Win9x users:  see "Win9x note" below.

    build_ssl.py attempts to catch the most common errors (such as not
    being able to find OpenSSL sources, or not being able to find a Perl
    that works with OpenSSL) and give a reasonable error message.
    If you have a problem that doesn't seem to be handled correctly
    (eg, you know you have ActivePerl but we can't find it), please take
    a peek at build_ssl.py and suggest patches.  Note that build_ssl.py
    should be able to be run directly from the command-line.

    build_ssl.py/MSVC isn't clever enough to clean OpenSSL - you must do
    this by hand.

    Win9x note:  If, near the start of the build process, you see
    something like

        C:\Code\openssl-0.9.6g>set OPTS=no-asm
        Out of environment space

    then you're in trouble, and will probably also see these errors near
    the end of the process:

        NMAKE : fatal error U1073: don't know how to make
            'crypto\md5\asm\m5_win32.asm'
        Stop.
        NMAKE : fatal error U1073: don't know how to make
            'C:\Code\openssl-0.9.6g/out32/libeay32.lib'
        Stop.

    You need more environment space.  Win9x only has room for 256 bytes
    by default, and especially after installing ActivePerl (which fiddles
    the PATH envar), you're likely to run out.  KB Q230205

        http://support.microsoft.com/default.aspx?scid=KB;en-us;q230205

    explains how to edit CONFIG.SYS to cure this.


YOUR OWN EXTENSION DLLs
-----------------------
If you want to create your own extension module DLL, there's an example
with easy-to-follow instructions in ../PC/example/; read the file
readme.txt there first.
