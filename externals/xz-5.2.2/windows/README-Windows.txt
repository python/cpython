
XZ Utils for Windows
====================

Introduction
------------

    This package includes command line tools (xz.exe and a few others)
    and the liblzma compression library from XZ Utils. You can find the
    latest version and full source code from <https://tukaani.org/xz/>.

    The parts of the XZ Utils source code, that are relevant to this
    binary package, are in the public domain. XZ Utils have been built
    for this package with MinGW-w64 and linked statically against its
    runtime libraries. See COPYING-Windows.txt for the copyright and
    license information that applies to the MinGW-w64 runtime. You must
    include it when redistributing these XZ Utils binaries.


Package contents
----------------

    All executables and libraries in this package require msvcrt.dll.
    It's included in all recent Windows versions. (On Windows 95 it
    might be missing, but once you get it somewhere, the i686 binaries
    should run even on Windows 95 if the processor is new enough.)

    There is a SSE2 optimization in the compression code but this
    version of XZ Utils doesn't include run-time processor detection.
    This is why there is a separate i686-SSE2 version.

    There is one directory for each type of executable and library files:

        bin_i686        32-bit x86 (i686 and newer), Windows 95 and later
        bin_i686-sse2   32-bit x86 (i686 with SSE2), Windows 98 and later
        bin_x86-64      64-bit x86-64, Windows Vista and later

    Each of the above directories have the following files:

        *.exe       Command line tools. (It's useless to double-click
                    these; use the command prompt instead.) These have
                    been linked statically against liblzma, so they
                    don't require liblzma.dll. Thus, you can copy e.g.
                    xz.exe to a directory that is in PATH without copying
                    any other files from this package.

        liblzma.dll Shared version of the liblzma compression library.
                    This file is mostly useful to developers, although
                    some non-developers might use it to upgrade their
                    copy of liblzma.

        liblzma.a   Static version of the liblzma compression library.
                    This file is useful only for developers.

    The rest of the directories contain architecture-independent files:

        doc         Documentation in the plain text (TXT) format. The
                    manuals of the command line tools are provided also
                    in the PDF format. liblzma.def is in this directory
                    too.

        include     C header files for liblzma. These should be
                    compatible with most C and C++ compilers. If you
                    have problems, try to fix it and send your fixes
                    upstream, or at least report a bug, thanks.


Linking against liblzma
-----------------------

MinGW

    If you use MinGW, linking against liblzma.dll or liblzma.a should
    be straightforward. You don't need an import library to link
    against liblzma.dll, and for static linking, you don't need to
    worry about the LZMA_API_STATIC macro.

    Note that the MinGW distribution includes liblzma. If you are
    building packages that will be part of the MinGW distribution, you
    probably should use the version of liblzma shipped in MinGW instead
    of this package.


Microsoft Visual C++

    To link against liblzma.dll, you need to create an import library
    first. You need the "lib" command from MSVC and liblzma.def from
    the "doc" directory of this package. Here is the command that works
    on 32-bit x86:

        lib /def:liblzma.def /out:liblzma.lib /machine:ix86

    On x86-64, the /machine argument has to naturally be changed:

        lib /def:liblzma.def /out:liblzma.lib /machine:x64

    If you need to link statically against liblzma, you should build
    liblzma with MSVC 2013 update 2 or later. Alternatively, if having
    a decompressor is enough, consider using XZ Embedded or LZMA SDK.

    When you plan to link against static liblzma, you need to tell
    lzma.h to not use __declspec(dllimport) by defining the macro
    LZMA_API_STATIC. You can do it either in the C/C++ code

        #define LZMA_API_STATIC
        #include <lzma.h>

    or by adding it to compiler options.


Other compilers

    If you are using some other compiler, see its documentation how to
    create an import library (if it is needed). If it is simple, I
    might consider including the instructions here.


Reporting bugs
--------------

    Report bugs to <lasse.collin@tukaani.org> (in English or Finnish).

