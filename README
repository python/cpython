This is Python version 2.7.3
============================

Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011,
2012 Python Software Foundation.  All rights reserved.

Copyright (c) 2000 BeOpen.com.
All rights reserved.

Copyright (c) 1995-2001 Corporation for National Research Initiatives.
All rights reserved.

Copyright (c) 1991-1995 Stichting Mathematisch Centrum.
All rights reserved.


License information
-------------------

See the file "LICENSE" for information on the history of this
software, terms & conditions for usage, and a DISCLAIMER OF ALL
WARRANTIES.

This Python distribution contains no GNU General Public Licensed
(GPLed) code so it may be used in proprietary projects just like prior
Python distributions.  There are interfaces to some GNU code but these
are entirely optional.

All trademarks referenced herein are property of their respective
holders.


What's new in this release?
---------------------------

See the file "Misc/NEWS".


If you don't read instructions
------------------------------

Congratulations on getting this far. :-)

To start building right away (on UNIX): type "./configure" in the
current directory and when it finishes, type "make".  This creates an
executable "./python"; to install in /usr/local, first do "su root"
and then "make install".

The section `Build instructions' below is still recommended reading.


What is Python anyway?
----------------------

Python is an interpreted, interactive object-oriented programming
language suitable (amongst other uses) for distributed application
development, scripting, numeric computing and system testing.  Python
is often compared to Tcl, Perl, Java, JavaScript, Visual Basic or
Scheme.  To find out more about what Python can do for you, point your
browser to http://www.python.org/.


How do I learn Python?
----------------------

The official tutorial is still a good place to start; see
http://docs.python.org/ for online and downloadable versions, as well
as a list of other introductions, and reference documentation.

There's a quickly growing set of books on Python.  See
http://wiki.python.org/moin/PythonBooks for a list.


Documentation
-------------

All documentation is provided online in a variety of formats.  In
order of importance for new users: Tutorial, Library Reference,
Language Reference, Extending & Embedding, and the Python/C API.  The
Library Reference is especially of immense value since much of
Python's power is described there, including the built-in data types
and functions!

All documentation is also available online at the Python web site
(http://docs.python.org/, see below).  It is available online for occasional
reference, or can be downloaded in many formats for faster access.  The
documentation is downloadable in HTML, PostScript, PDF, LaTeX, and
reStructuredText (2.6+) formats; the LaTeX and reStructuredText versions are
primarily for documentation authors, translators, and people with special
formatting requirements.


Web sites
---------

New Python releases and related technologies are published at
http://www.python.org/.  Come visit us!


Newsgroups and Mailing Lists
----------------------------

Read comp.lang.python, a high-volume discussion newsgroup about
Python, or comp.lang.python.announce, a low-volume moderated newsgroup
for Python-related announcements.  These are also accessible as
mailing lists: see http://www.python.org/community/lists/ for an
overview of these and many other Python-related mailing lists.

Archives are accessible via the Google Groups Usenet archive; see
http://groups.google.com/.  The mailing lists are also archived, see
http://www.python.org/community/lists/ for details.


Bug reports
-----------

To report or search for bugs, please use the Python Bug
Tracker at http://bugs.python.org/.


Patches and contributions
-------------------------

To submit a patch or other contribution, please use the Python Patch
Manager at http://bugs.python.org/.  Guidelines
for patch submission may be found at http://www.python.org/dev/patches/.

If you have a proposal to change Python, you may want to send an email to the
comp.lang.python or python-ideas mailing lists for inital feedback. A Python
Enhancement Proposal (PEP) may be submitted if your idea gains ground. All
current PEPs, as well as guidelines for submitting a new PEP, are listed at
http://www.python.org/dev/peps/.


Questions
---------

For help, if you can't find it in the manuals or on the web site, it's
best to post to the comp.lang.python or the Python mailing list (see
above).  If you specifically don't want to involve the newsgroup or
mailing list, send questions to help@python.org (a group of volunteers
who answer questions as they can).  The newsgroup is the most
efficient way to ask public questions.


Build instructions
==================

Before you can build Python, you must first configure it.
Fortunately, the configuration and build process has been automated
for Unix and Linux installations, so all you usually have to do is
type a few commands and sit back.  There are some platforms where
things are not quite as smooth; see the platform specific notes below.
If you want to build for multiple platforms sharing the same source
tree, see the section on VPATH below.

Start by running the script "./configure", which determines your
system configuration and creates the Makefile.  (It takes a minute or
two -- please be patient!)  You may want to pass options to the
configure script -- see the section below on configuration options and
variables.  When it's done, you are ready to run make.

To build Python, you normally type "make" in the toplevel directory.
If you have changed the configuration, the Makefile may have to be
rebuilt.  In this case, you may have to run make again to correctly
build your desired target.  The interpreter executable is built in the
top level directory.

Once you have built a Python interpreter, see the subsections below on
testing and installation.  If you run into trouble, see the next
section.

Previous versions of Python used a manual configuration process that
involved editing the file Modules/Setup.  While this file still exists
and manual configuration is still supported, it is rarely needed any
more: almost all modules are automatically built as appropriate under
guidance of the setup.py script, which is run by Make after the
interpreter has been built.


Troubleshooting
---------------

See also the platform specific notes in the next section.

If you run into other trouble, see the FAQ
(http://www.python.org/doc/faq/) for hints on what can go wrong, and
how to fix it.

If you rerun the configure script with different options, remove all
object files by running "make clean" before rebuilding.  Believe it or
not, "make clean" sometimes helps to clean up other inexplicable
problems as well.  Try it before sending in a bug report!

If the configure script fails or doesn't seem to find things that
should be there, inspect the config.log file.

If you get a warning for every file about the -Olimit option being no
longer supported, you can ignore it.  There's no foolproof way to know
whether this option is needed; all we can do is test whether it is
accepted without error.  On some systems, e.g. older SGI compilers, it
is essential for performance (specifically when compiling ceval.c,
which has more basic blocks than the default limit of 1000).  If the
warning bothers you, edit the Makefile to remove "-Olimit 1500" from
the OPT variable.

If you get failures in test_long, or sys.maxint gets set to -1, you
are probably experiencing compiler bugs, usually related to
optimization.  This is a common problem with some versions of gcc, and
some vendor-supplied compilers, which can sometimes be worked around
by turning off optimization.  Consider switching to stable versions
(gcc 2.95.2, gcc 3.x, or contact your vendor.)

From Python 2.0 onward, all Python C code is ANSI C.  Compiling using
old K&R-C-only compilers is no longer possible.  ANSI C compilers are
available for all modern systems, either in the form of updated
compilers from the vendor, or one of the free compilers (gcc).

If "make install" fails mysteriously during the "compiling the library"
step, make sure that you don't have any of the PYTHONPATH or PYTHONHOME
environment variables set, as they may interfere with the newly built
executable which is compiling the library.

Unsupported systems
-------------------

A number of systems are not supported in Python 2.7 anymore. Some
support code is still present, but will be removed in later versions.
If you still need to use current Python versions on these systems,
please send a message to python-dev@python.org indicating that you
volunteer to support this system. For a more detailed discussion 
regarding no-longer-supported and resupporting platforms, as well
as a list of platforms that became or will be unsupported, see PEP 11.

More specifically, the following systems are not supported any
longer:
- SunOS 4
- DYNIX
- dgux
- Minix
- NeXT
- Irix 4 and --with-sgi-dl
- Linux 1
- Systems defining __d6_pthread_create (configure.in)
- Systems defining PY_PTHREAD_D4, PY_PTHREAD_D6,
  or PY_PTHREAD_D7 in thread_pthread.h
- Systems using --with-dl-dld
- Systems using --without-universal-newlines
- MacOS 9
- Systems using --with-wctype-functions
- Win9x, WinME


Platform specific notes
-----------------------

(Some of these may no longer apply.  If you find you can build Python
on these platforms without the special directions mentioned here,
submit a documentation bug report to SourceForge (see Bug Reports
above) so we can remove them!)

Unix platforms: If your vendor still ships (and you still use) Berkeley DB
        1.85 you will need to edit Modules/Setup to build the bsddb185
        module and add a line to sitecustomize.py which makes it the
        default.  In Modules/Setup a line like

            bsddb185 bsddbmodule.c

        should work.  (You may need to add -I, -L or -l flags to direct the
        compiler and linker to your include files and libraries.)

XXX I think this next bit is out of date:

64-bit platforms: The modules audioop, and imageop don't work.
        The setup.py script disables them on 64-bit installations.
        Don't try to enable them in the Modules/Setup file.  They
        contain code that is quite wordsize sensitive.  (If you have a
        fix, let us know!)

Solaris: When using Sun's C compiler with threads, at least on Solaris
        2.5.1, you need to add the "-mt" compiler option (the simplest
        way is probably to specify the compiler with this option as
        the "CC" environment variable when running the configure
        script).

        When using GCC on Solaris, beware of binutils 2.13 or GCC
        versions built using it.  This mistakenly enables the
        -zcombreloc option which creates broken shared libraries on
        Solaris.  binutils 2.12 works, and the binutils maintainers
        are aware of the problem.  Binutils 2.13.1 only partially
        fixed things.  It appears that 2.13.2 solves the problem
        completely.  This problem is known to occur with Solaris 2.7
        and 2.8, but may also affect earlier and later versions of the
        OS.

        When the dynamic loader complains about errors finding shared
        libraries, such as

        ld.so.1: ./python: fatal: libstdc++.so.5: open failed:
        No such file or directory

        you need to first make sure that the library is available on
        your system. Then, you need to instruct the dynamic loader how
        to find it. You can choose any of the following strategies:

        1. When compiling Python, set LD_RUN_PATH to the directories
           containing missing libraries.
        2. When running Python, set LD_LIBRARY_PATH to these directories.
        3. Use crle(8) to extend the search path of the loader.
        4. Modify the installed GCC specs file, adding -R options into the
           *link: section.

        The complex object fails to compile on Solaris 10 with gcc 3.4 (at
        least up to 3.4.3).  To work around it, define Py_HUGE_VAL as
        HUGE_VAL(), e.g.:

          make CPPFLAGS='-D"Py_HUGE_VAL=HUGE_VAL()" -I. -I$(srcdir)/Include'
          ./python setup.py CPPFLAGS='-D"Py_HUGE_VAL=HUGE_VAL()"'

Linux:  A problem with threads and fork() was tracked down to a bug in
        the pthreads code in glibc version 2.0.5; glibc version 2.0.7
        solves the problem.  This causes the popen2 test to fail;
        problem and solution reported by Pablo Bleyer.

Red Hat Linux: Red Hat 9 built Python2.2 in UCS-4 mode and hacked
        Tcl to support it. To compile Python2.3 with Tkinter, you will
        need to pass --enable-unicode=ucs4 flag to ./configure.

        There's an executable /usr/bin/python which is Python
        1.5.2 on most older Red Hat installations; several key Red Hat tools
        require this version.  Python 2.1.x may be installed as
        /usr/bin/python2.  The Makefile installs Python as
        /usr/local/bin/python, which may or may not take precedence
        over /usr/bin/python, depending on how you have set up $PATH.

FreeBSD 3.x and probably platforms with NCurses that use libmytinfo or
        similar: When using cursesmodule, the linking is not done in
        the correct order with the defaults.  Remove "-ltermcap" from
        the readline entry in Setup, and use as curses entry: "curses
        cursesmodule.c -lmytinfo -lncurses -ltermcap" - "mytinfo" (so
        called on FreeBSD) should be the name of the auxiliary library
        required on your platform.  Normally, it would be linked
        automatically, but not necessarily in the correct order.

BSDI:   BSDI versions before 4.1 have known problems with threads,
        which can cause strange errors in a number of modules (for
        instance, the 'test_signal' test script will hang forever.)
        Turning off threads (with --with-threads=no) or upgrading to
        BSDI 4.1 solves this problem.

DEC Unix: Run configure with --with-dec-threads, or with
        --with-threads=no if no threads are desired (threads are on by
        default).  When using GCC, it is possible to get an internal
        compiler error if optimization is used.  This was reported for
        GCC 2.7.2.3 on selectmodule.c.  Manually compile the affected
        file without optimization to solve the problem.

DEC Ultrix: compile with GCC to avoid bugs in the native compiler,
        and pass SHELL=/bin/sh5 to Make when installing.

AIX:    A complete overhaul of the shared library support is now in
        place.  See Misc/AIX-NOTES for some notes on how it's done.
        (The optimizer bug reported at this place in previous releases
        has been worked around by a minimal code change.) If you get
        errors about pthread_* functions, during compile or during
        testing, try setting CC to a thread-safe (reentrant) compiler,
        like "cc_r".  For full C++ module support, set CC="xlC_r" (or
        CC="xlC" without thread support).

AIX 5.3: To build a 64-bit version with IBM's compiler, I used the
        following:

        export PATH=/usr/bin:/usr/vacpp/bin
        ./configure --with-gcc="xlc_r -q64" --with-cxx="xlC_r -q64" \
                    --disable-ipv6 AR="ar -X64"
        make

HP-UX:  When using threading, you may have to add -D_REENTRANT to the
        OPT variable in the top-level Makefile; reported by Pat Knight,
        this seems to make a difference (at least for HP-UX 10.20)
        even though pyconfig.h defines it. This seems unnecessary when
        using HP/UX 11 and later - threading seems to work "out of the
        box".

HP-UX ia64: When building on the ia64 (Itanium) platform using HP's
        compiler, some experience has shown that the compiler's
        optimiser produces a completely broken version of python
        (see http://bugs.python.org/814976). To work around this,
        edit the Makefile and remove -O from the OPT line.

        To build a 64-bit executable on an Itanium 2 system using HP's
        compiler, use these environment variables:

                CC=cc
                CXX=aCC
                BASECFLAGS="+DD64"
                LDFLAGS="+DD64 -lxnet"

        and call configure as:

                ./configure --without-gcc

        then *unset* the environment variables again before running
        make.  (At least one of these flags causes the build to fail
        if it remains set.)  You still have to edit the Makefile and
        remove -O from the OPT line.

HP PA-RISC 2.0: A recent bug report (http://bugs.python.org/546117)
        suggests that the C compiler in this 64-bit system has bugs
        in the optimizer that break Python.  Compiling without
        optimization solves the problems.

SCO:    The following apply to SCO 3 only; Python builds out of the box
        on SCO 5 (or so we've heard).

        1) Everything works much better if you add -U__STDC__ to the
        defs.  This is because all the SCO header files are broken.
        Anything that isn't mentioned in the C standard is
        conditionally excluded when __STDC__ is defined.

        2) Due to the U.S. export restrictions, SCO broke the crypt
        stuff out into a separate library, libcrypt_i.a so the LIBS
        needed be set to:

                LIBS=' -lsocket -lcrypt_i'

UnixWare: There are known bugs in the math library of the system, as well as
        problems in the handling of threads (calling fork in one
        thread may interrupt system calls in others). Therefore, test_math and
        tests involving threads will fail until those problems are fixed.

QNX:    Chris Herborth (chrish@qnx.com) writes:
        configure works best if you use GNU bash; a port is available on
        ftp.qnx.com in /usr/free.  I used the following process to build,
        test and install Python 1.5.x under QNX:

        1) CONFIG_SHELL=/usr/local/bin/bash CC=cc RANLIB=: \
            ./configure --verbose --without-gcc --with-libm=""

        2) edit Modules/Setup to activate everything that makes sense for
           your system... tested here at QNX with the following modules:

                array, audioop, binascii, cPickle, cStringIO, cmath,
                crypt, curses, errno, fcntl, gdbm, grp, imageop,
                _locale, math, md5, new, operator, parser, pcre,
                posix, pwd, readline, regex, reop,
                select, signal, socket, soundex, strop, struct,
                syslog, termios, time, timing, zlib, audioop, imageop

        3) make SHELL=/usr/local/bin/bash

           or, if you feel the need for speed:

           make SHELL=/usr/local/bin/bash OPT="-5 -Oil+nrt"

        4) make SHELL=/usr/local/bin/bash test

           Using GNU readline 2.2 seems to behave strangely, but I
           think that's a problem with my readline 2.2 port.  :-\

        5) make SHELL=/usr/local/bin/bash install

        If you get SIGSEGVs while running Python (I haven't yet, but
        I've only run small programs and the test cases), you're
        probably running out of stack; the default 32k could be a
        little tight.  To increase the stack size, edit the Makefile
        to read: LDFLAGS = -N 48k

BeOS:   See Misc/BeOS-NOTES for notes about compiling/installing
        Python on BeOS R3 or later.  Note that only the PowerPC
        platform is supported for R3; both PowerPC and x86 are
        supported for R4.

Cray T3E: Mark Hadfield (m.hadfield@niwa.co.nz) writes:
        Python can be built satisfactorily on a Cray T3E but based on
        my experience with the NIWA T3E (2002-05-22, version 2.2.1)
        there are a few bugs and gotchas. For more information see a
        thread on comp.lang.python in May 2002 entitled "Building
        Python on Cray T3E".

        1) Use Cray's cc and not gcc. The latter was reported not to
           work by Konrad Hinsen. It may work now, but it may not.

        2) To set sys.platform to something sensible, pass the
           following environment variable to the configure script:

             MACHDEP=unicosmk

        2) Run configure with option "--enable-unicode=ucs4".

        3) The Cray T3E does not support dynamic linking, so extension
           modules have to be built by adding (or uncommenting) lines
           in Modules/Setup. The minimum set of modules is

             posix, new, _sre, unicodedata

           On NIWA's vanilla T3E system the following have also been
           included successfully:

             _codecs, _locale, _socket, _symtable, _testcapi, _weakref
             array, binascii, cmath, cPickle, crypt, cStringIO, dbm
             errno, fcntl, grp, math, md5, operator, parser, pcre, pwd
             regex, rotor, select, struct, strop, syslog, termios
             time, timing, xreadlines

        4) Once the python executable and library have been built, make
           will execute setup.py, which will attempt to build remaining
           extensions and link them dynamically. Each of these attempts
           will fail but should not halt the make process. This is
           normal.

        5) Running "make test" uses a lot of resources and causes
           problems on our system. You might want to try running tests
           singly or in small groups.

SGI:    SGI's standard "make" utility (/bin/make or /usr/bin/make)
        does not check whether a command actually changed the file it
        is supposed to build.  This means that whenever you say "make"
        it will redo the link step.  The remedy is to use SGI's much
        smarter "smake" utility (/usr/sbin/smake), or GNU make.  If
        you set the first line of the Makefile to #!/usr/sbin/smake
        smake will be invoked by make (likewise for GNU make).

        WARNING: There are bugs in the optimizer of some versions of
        SGI's compilers that can cause bus errors or other strange
        behavior, especially on numerical operations.  To avoid this,
        try building with "make OPT=".

OS/2:   If you are running Warp3 or Warp4 and have IBM's VisualAge C/C++
        compiler installed, just change into the pc\os2vacpp directory
        and type NMAKE.  Threading and sockets are supported by default
        in the resulting binaries of PYTHON15.DLL and PYTHON.EXE.

Reliant UNIX: The thread support does not compile on Reliant UNIX, and
        there is a (minor) problem in the configure script for that
        platform as well.  This should be resolved in time for a
        future release.

MacOSX: The tests will crash on both 10.1 and 10.2 with SEGV in
        test_re and test_sre due to the small default stack size.  If
        you set the stack size to 2048 before doing a "make test" the
        failure can be avoided.  If you're using the tcsh or csh shells,
        use "limit stacksize 2048" and for the bash shell (the default
        as of OSX 10.3), use "ulimit -s 2048".

        On naked Darwin you may want to add the configure option
        "--disable-toolbox-glue" to disable the glue code for the Carbon
        interface modules. The modules themselves are currently only built
        if you add the --enable-framework option, see below.

        On a clean OSX /usr/local does not exist. Do a
        "sudo mkdir -m 775 /usr/local"
        before you do a make install. It is probably not a good idea to
        do "sudo make install" which installs everything as superuser,
        as this may later cause problems when installing distutils-based
        additions.

        Some people have reported problems building Python after using "fink"
        to install additional unix software. Disabling fink (remove all 
        references to /sw from your .profile or .login) should solve this.

        You may want to try the configure option "--enable-framework"
        which installs Python as a framework. The location can be set
        as argument to the --enable-framework option (default
        /Library/Frameworks). A framework install is probably needed if you
        want to use any Aqua-based GUI toolkit (whether Tkinter, wxPython,
        Carbon, Cocoa or anything else).

        You may also want to try the configure option "--enable-universalsdk"
        which builds Python as a universal binary with support for the 
        i386 and PPC architetures. This requires Xcode 2.1 or later to build.

        See Mac/README for more information on framework and 
        universal builds.

Cygwin: With recent (relative to the time of writing, 2001-12-19)
        Cygwin installations, there are problems with the interaction
        of dynamic linking and fork().  This manifests itself in build
        failures during the execution of setup.py.

        There are two workarounds that both enable Python (albeit
        without threading support) to build and pass all tests on
        NT/2000 (and most likely XP as well, though reports of testing
        on XP would be appreciated).

        The workarounds:

        (a) the band-aid fix is to link the _socket module statically
        rather than dynamically (which is the default).

        To do this, run "./configure --with-threads=no" including any
        other options you need (--prefix, etc.).  Then in Modules/Setup
        uncomment the lines:

        #SSL=/usr/local/ssl
        #_socket socketmodule.c \
        #       -DUSE_SSL -I$(SSL)/include -I$(SSL)/include/openssl \
        #       -L$(SSL)/lib -lssl -lcrypto

        and remove "local/" from the SSL variable.  Finally, just run
        "make"!

        (b) The "proper" fix is to rebase the Cygwin DLLs to prevent
        base address conflicts.  Details on how to do this can be
        found in the following mail:

           http://sources.redhat.com/ml/cygwin/2001-12/msg00894.html

        It is hoped that a version of this solution will be
        incorporated into the Cygwin distribution fairly soon.

        Two additional problems:

        (1) Threading support should still be disabled due to a known
        bug in Cygwin pthreads that causes test_threadedtempfile to
        hang.

        (2) The _curses module does not build.  This is a known
        Cygwin ncurses problem that should be resolved the next time
        that this package is released.

        On older versions of Cygwin, test_poll may hang and test_strftime
        may fail.

        The situation on 9X/Me is not accurately known at present.
        Some time ago, there were reports that the following
        regression tests failed:

            test_pwd
            test_select (hang)
            test_socket

        Due to the test_select hang on 9X/Me, one should run the
        regression test using the following:

            make TESTOPTS='-l -x test_select' test

        News regarding these platforms with more recent Cygwin
        versions would be appreciated!

Windows: When executing Python scripts on the command line using file type
        associations (i.e. starting "script.py" instead of "python script.py"),
        redirects may not work unless you set a specific registry key.  See
        the Knowledge Base article <http://support.microsoft.com/kb/321788>.


Configuring the bsddb and dbm modules
-------------------------------------

Beginning with Python version 2.3, the PyBsddb package
<http://pybsddb.sf.net/> was adopted into Python as the bsddb package,
exposing a set of package-level functions which provide
backwards-compatible behavior.  Only versions 3.3 through 4.4 of
Sleepycat's libraries provide the necessary API, so older versions
aren't supported through this interface.  The old bsddb module has
been retained as bsddb185, though it is not built by default.  Users
wishing to use it will have to tweak Modules/Setup to build it.  The
dbm module will still be built against the Sleepycat libraries if
other preferred alternatives (ndbm, gdbm) are not found.

Building the sqlite3 module
---------------------------

To build the sqlite3 module, you'll need the sqlite3 or libsqlite3
packages installed, including the header files. Many modern operating
systems distribute the headers in a separate package to the library -
often it will be the same name as the main package, but with a -dev or
-devel suffix. 

The version of pysqlite2 that's including in Python needs sqlite3 3.0.8
or later. setup.py attempts to check that it can find a correct version.

Configuring threads
-------------------

As of Python 2.0, threads are enabled by default.  If you wish to
compile without threads, or if your thread support is broken, pass the
--with-threads=no switch to configure.  Unfortunately, on some
platforms, additional compiler and/or linker options are required for
threads to work properly.  Below is a table of those options,
collected by Bill Janssen.  We would love to automate this process
more, but the information below is not enough to write a patch for the
configure.in file, so manual intervention is required.  If you patch
the configure.in file and are confident that the patch works, please
send in the patch.  (Don't bother patching the configure script itself
-- it is regenerated each time the configure.in file changes.)

Compiler switches for threads
.............................

The definition of _REENTRANT should be configured automatically, if
that does not work on your system, or if _REENTRANT is defined
incorrectly, please report that as a bug.

    OS/Compiler/threads                     Switches for use with threads
    (POSIX is draft 10, DCE is draft 4)     compile & link

    SunOS 5.{1-5}/{gcc,SunPro cc}/solaris   -mt
    SunOS 5.5/{gcc,SunPro cc}/POSIX         (nothing)
    DEC OSF/1 3.x/cc/DCE                    -threads
            (butenhof@zko.dec.com)
    Digital UNIX 4.x/cc/DCE                 -threads
            (butenhof@zko.dec.com)
    Digital UNIX 4.x/cc/POSIX               -pthread
            (butenhof@zko.dec.com)
    AIX 4.1.4/cc_r/d7                       (nothing)
            (buhrt@iquest.net)
    AIX 4.1.4/cc_r4/DCE                     (nothing)
            (buhrt@iquest.net)
    IRIX 6.2/cc/POSIX                       (nothing)
            (robertl@cwi.nl)


Linker (ld) libraries and flags for threads
...........................................

    OS/threads                          Libraries/switches for use with threads

    SunOS 5.{1-5}/solaris               -lthread
    SunOS 5.5/POSIX                     -lpthread
    DEC OSF/1 3.x/DCE                   -lpthreads -lmach -lc_r -lc
            (butenhof@zko.dec.com)
    Digital UNIX 4.x/DCE                -lpthreads -lpthread -lmach -lexc -lc
            (butenhof@zko.dec.com)
    Digital UNIX 4.x/POSIX              -lpthread -lmach -lexc -lc
            (butenhof@zko.dec.com)
    AIX 4.1.4/{draft7,DCE}              (nothing)
            (buhrt@iquest.net)
    IRIX 6.2/POSIX                      -lpthread
            (jph@emilia.engr.sgi.com)


Building a shared libpython
---------------------------

Starting with Python 2.3, the majority of the interpreter can be built
into a shared library, which can then be used by the interpreter
executable, and by applications embedding Python. To enable this feature,
configure with --enable-shared.

If you enable this feature, the same object files will be used to create
a static library.  In particular, the static library will contain object
files using position-independent code (PIC) on platforms where PIC flags
are needed for the shared library.


Configuring additional built-in modules
---------------------------------------

Starting with Python 2.1, the setup.py script at the top of the source
distribution attempts to detect which modules can be built and
automatically compiles them.  Autodetection doesn't always work, so
you can still customize the configuration by editing the Modules/Setup
file; but this should be considered a last resort.  The rest of this
section only applies if you decide to edit the Modules/Setup file.
You also need this to enable static linking of certain modules (which
is needed to enable profiling on some systems).

This file is initially copied from Setup.dist by the configure script;
if it does not exist yet, create it by copying Modules/Setup.dist
yourself (configure will never overwrite it).  Never edit Setup.dist
-- always edit Setup or Setup.local (see below).  Read the comments in
the file for information on what kind of edits are allowed.  When you
have edited Setup in the Modules directory, the interpreter will
automatically be rebuilt the next time you run make (in the toplevel
directory).

Many useful modules can be built on any Unix system, but some optional
modules can't be reliably autodetected.  Often the quickest way to
determine whether a particular module works or not is to see if it
will build: enable it in Setup, then if you get compilation or link
errors, disable it -- you're either missing support or need to adjust
the compilation and linking parameters for that module.

On SGI IRIX, there are modules that interface to many SGI specific
system libraries, e.g. the GL library and the audio hardware.  These
modules will not be built by the setup.py script.

In addition to the file Setup, you can also edit the file Setup.local.
(the makesetup script processes both).  You may find it more
convenient to edit Setup.local and leave Setup alone.  Then, when
installing a new Python version, you can copy your old Setup.local
file.


Setting the optimization/debugging options
------------------------------------------

If you want or need to change the optimization/debugging options for
the C compiler, assign to the OPT variable on the toplevel make
command; e.g. "make OPT=-g" will build a debugging version of Python
on most platforms.  The default is OPT=-O; a value for OPT in the
environment when the configure script is run overrides this default
(likewise for CC; and the initial value for LIBS is used as the base
set of libraries to link with).

When compiling with GCC, the default value of OPT will also include
the -Wall and -Wstrict-prototypes options.

Additional debugging code to help debug memory management problems can
be enabled by using the --with-pydebug option to the configure script.

For flags that change binary compatibility, use the EXTRA_CFLAGS
variable.


Profiling
---------

If you want C profiling turned on, the easiest way is to run configure
with the CC environment variable to the necessary compiler
invocation.  For example, on Linux, this works for profiling using
gprof(1):

    CC="gcc -pg" ./configure

Note that on Linux, gprof apparently does not work for shared
libraries.  The Makefile/Setup mechanism can be used to compile and
link most extension modules statically.


Coverage checking
-----------------

For C coverage checking using gcov, run "make coverage".  This will
build a Python binary with profiling activated, and a ".gcno" and
".gcda" file for every source file compiled with that option.  With
the built binary, now run the code whose coverage you want to check.
Then, you can see coverage statistics for each individual source file
by running gcov, e.g.

    gcov -o Modules zlibmodule

This will create a "zlibmodule.c.gcov" file in the current directory
containing coverage info for that source file.

This works only for source files statically compiled into the
executable; use the Makefile/Setup mechanism to compile and link
extension modules you want to coverage-check statically.


Testing
-------

To test the interpreter, type "make test" in the top-level directory.
This runs the test set twice (once with no compiled files, once with
the compiled files left by the previous test run).  The test set
produces some output.  You can generally ignore the messages about
skipped tests due to optional features which can't be imported.
If a message is printed about a failed test or a traceback or core
dump is produced, something is wrong.  On some Linux systems (those
that are not yet using glibc 6), test_strftime fails due to a
non-standard implementation of strftime() in the C library. Please
ignore this, or upgrade to glibc version 6.

By default, tests are prevented from overusing resources like disk space and
memory.  To enable these tests, run "make testall".

IMPORTANT: If the tests fail and you decide to mail a bug report,
*don't* include the output of "make test".  It is useless.  Run the
failing test manually, as follows:

        ./python Lib/test/regrtest.py -v test_whatever

(substituting the top of the source tree for '.' if you built in a
different directory).  This runs the test in verbose mode.


Installing
----------

To install the Python binary, library modules, shared library modules
(see below), include files, configuration files, and the manual page,
just type

        make install

This will install all platform-independent files in subdirectories of
the directory given with the --prefix option to configure or to the
`prefix' Make variable (default /usr/local).  All binary and other
platform-specific files will be installed in subdirectories if the
directory given by --exec-prefix or the `exec_prefix' Make variable
(defaults to the --prefix directory) is given.

If DESTDIR is set, it will be taken as the root directory of the
installation, and files will be installed into $(DESTDIR)$(prefix),
$(DESTDIR)$(exec_prefix), etc.

All subdirectories created will have Python's version number in their
name, e.g. the library modules are installed in
"/usr/local/lib/python<version>/" by default, where <version> is the
<major>.<minor> release number (e.g. "2.1").  The Python binary is
installed as "python<version>" and a hard link named "python" is
created.  The only file not installed with a version number in its
name is the manual page, installed as "/usr/local/man/man1/python.1"
by default.

If you want to install multiple versions of Python see the section below
entitled "Installing multiple versions".

The only thing you may have to install manually is the Python mode for
Emacs found in Misc/python-mode.el.  (But then again, more recent
versions of Emacs may already have it.)  Follow the instructions that
came with Emacs for installation of site-specific files.

On Mac OS X, if you have configured Python with --enable-framework, you
should use "make frameworkinstall" to do the installation. Note that this
installs the Python executable in a place that is not normally on your
PATH, you may want to set up a symlink in /usr/local/bin.


Installing multiple versions
----------------------------

On Unix and Mac systems if you intend to install multiple versions of Python
using the same installation prefix (--prefix argument to the configure
script) you must take care that your primary python executable is not
overwritten by the installation of a different version.  All files and
directories installed using "make altinstall" contain the major and minor
version and can thus live side-by-side.  "make install" also creates
${prefix}/bin/python which refers to ${prefix}/bin/pythonX.Y.  If you intend
to install multiple versions using the same prefix you must decide which
version (if any) is your "primary" version.  Install that version using
"make install".  Install all other versions using "make altinstall".

For example, if you want to install Python 2.5, 2.6 and 3.0 with 2.6 being
the primary version, you would execute "make install" in your 2.6 build
directory and "make altinstall" in the others.


Configuration options and variables
-----------------------------------

Some special cases are handled by passing options to the configure
script.

WARNING: if you rerun the configure script with different options, you
must run "make clean" before rebuilding.  Exceptions to this rule:
after changing --prefix or --exec-prefix, all you need to do is remove
Modules/getpath.o.

--with(out)-gcc: The configure script uses gcc (the GNU C compiler) if
        it finds it.  If you don't want this, or if this compiler is
        installed but broken on your platform, pass the option
        --without-gcc.  You can also pass "CC=cc" (or whatever the
        name of the proper C compiler is) in the environment, but the
        advantage of using --without-gcc is that this option is
        remembered by the config.status script for its --recheck
        option.

--prefix, --exec-prefix: If you want to install the binaries and the
        Python library somewhere else than in /usr/local/{bin,lib},
        you can pass the option --prefix=DIRECTORY; the interpreter
        binary will be installed as DIRECTORY/bin/python and the
        library files as DIRECTORY/lib/python/*.  If you pass
        --exec-prefix=DIRECTORY (as well) this overrides the
        installation prefix for architecture-dependent files (like the
        interpreter binary).  Note that --prefix=DIRECTORY also
        affects the default module search path (sys.path), when
        Modules/config.c is compiled.  Passing make the option
        prefix=DIRECTORY (and/or exec_prefix=DIRECTORY) overrides the
        prefix set at configuration time; this may be more convenient
        than re-running the configure script if you change your mind
        about the install prefix.

--with-readline: This option is no longer supported.  GNU
        readline is automatically enabled by setup.py when present.

--with-threads: On most Unix systems, you can now use multiple
        threads, and support for this is enabled by default.  To
        disable this, pass --with-threads=no.  If the library required
        for threads lives in a peculiar place, you can use
        --with-thread=DIRECTORY.  IMPORTANT: run "make clean" after
        changing (either enabling or disabling) this option, or you
        will get link errors!  Note: for DEC Unix use
        --with-dec-threads instead.

--with-sgi-dl: On SGI IRIX 4, dynamic loading of extension modules is
        supported by the "dl" library by Jack Jansen, which is
        ftp'able from ftp://ftp.cwi.nl/pub/dynload/dl-1.6.tar.Z.
        This is enabled (after you've ftp'ed and compiled the dl
        library) by passing --with-sgi-dl=DIRECTORY where DIRECTORY
        is the absolute pathname of the dl library.  (Don't bother on
        IRIX 5, it already has dynamic linking using SunOS style
        shared libraries.)  THIS OPTION IS UNSUPPORTED.

--with-dl-dld: Dynamic loading of modules is rumored to be supported
        on some other systems: VAX (Ultrix), Sun3 (SunOS 3.4), Sequent
        Symmetry (Dynix), and Atari ST.  This is done using a
        combination of the GNU dynamic loading package
        (ftp://ftp.cwi.nl/pub/dynload/dl-dld-1.1.tar.Z) and an
        emulation of the SGI dl library mentioned above (the emulation
        can be found at
        ftp://ftp.cwi.nl/pub/dynload/dld-3.2.3.tar.Z).  To
        enable this, ftp and compile both libraries, then call
        configure, passing it the option
        --with-dl-dld=DL_DIRECTORY,DLD_DIRECTORY where DL_DIRECTORY is
        the absolute pathname of the dl emulation library and
        DLD_DIRECTORY is the absolute pathname of the GNU dld library.
        (Don't bother on SunOS 4 or 5, they already have dynamic
        linking using shared libraries.)  THIS OPTION IS UNSUPPORTED.

--with-libm, --with-libc: It is possible to specify alternative
        versions for the Math library (default -lm) and the C library
        (default the empty string) using the options
        --with-libm=STRING and --with-libc=STRING, respectively.  For
        example, if your system requires that you pass -lc_s to the C
        compiler to use the shared C library, you can pass
        --with-libc=-lc_s. These libraries are passed after all other
        libraries, the C library last.

--with-libs='libs': Add 'libs' to the LIBS that the python interpreter
        is linked against.

--with-cxx-main=<compiler>: If you plan to use C++ extension modules,
        then -- on some platforms -- you need to compile python's main()
        function with the C++ compiler. With this option, make will use
        <compiler> to compile main() *and* to link the python executable.
        It is likely that the resulting executable depends on the C++
        runtime library of <compiler>. (The default is --without-cxx-main.)

        There are platforms that do not require you to build Python
        with a C++ compiler in order to use C++ extension modules.
        E.g., x86 Linux with ELF shared binaries and GCC 3.x, 4.x is such
        a platform. We recommend that you configure Python
        --without-cxx-main on those platforms because a mismatch
        between the C++ compiler version used to build Python and to
        build a C++ extension module is likely to cause a crash at
        runtime.

        The Python installation also stores the variable CXX that
        determines, e.g., the C++ compiler distutils calls by default
        to build C++ extensions. If you set CXX on the configure command
        line to any string of non-zero length, then configure won't
        change CXX. If you do not preset CXX but pass
        --with-cxx-main=<compiler>, then configure sets CXX=<compiler>.
        In all other cases, configure looks for a C++ compiler by
        some common names (c++, g++, gcc, CC, cxx, cc++, cl) and sets
        CXX to the first compiler it finds. If it does not find any
        C++ compiler, then it sets CXX="".

        Similarly, if you want to change the command used to link the
        python executable, then set LINKCC on the configure command line.


--with-pydebug:  Enable additional debugging code to help track down
        memory management problems.  This allows printing a list of all
        live objects when the interpreter terminates.

--with(out)-universal-newlines: enable reading of text files with
        foreign newline convention (default: enabled). In other words,
        any of \r, \n or \r\n is acceptable as end-of-line character.
        If enabled import and execfile will automatically accept any newline
        in files. Python code can open a file with open(file, 'U') to
        read it in universal newline mode. THIS OPTION IS UNSUPPORTED.

--with-tsc: Profile using the Pentium timestamping counter (TSC).

--with-system-ffi:  Build the _ctypes extension module using an ffi
        library installed on the system.

--with-dbmliborder=db1:db2:...:  Specify the order that backends for the
	dbm extension are checked. Valid value is a colon separated string
	with the backend names `ndbm', `gdbm' and `bdb'.

Building for multiple architectures (using the VPATH feature)
-------------------------------------------------------------

If your file system is shared between multiple architectures, it
usually is not necessary to make copies of the sources for each
architecture you want to support.  If the make program supports the
VPATH feature, you can create an empty build directory for each
architecture, and in each directory run the configure script (on the
appropriate machine with the appropriate options).  This creates the
necessary subdirectories and the Makefiles therein.  The Makefiles
contain a line VPATH=... which points to a directory containing the
actual sources.  (On SGI systems, use "smake -J1" instead of "make" if
you use VPATH -- don't try gnumake.)

For example, the following is all you need to build a minimal Python
in /usr/tmp/python (assuming ~guido/src/python is the toplevel
directory and you want to build in /usr/tmp/python):

        $ mkdir /usr/tmp/python
        $ cd /usr/tmp/python
        $ ~guido/src/python/configure
        [...]
        $ make
        [...]
        $

Note that configure copies the original Setup file to the build
directory if it finds no Setup file there.  This means that you can
edit the Setup file for each architecture independently.  For this
reason, subsequent changes to the original Setup file are not tracked
automatically, as they might overwrite local changes.  To force a copy
of a changed original Setup file, delete the target Setup file.  (The
makesetup script supports multiple input files, so if you want to be
fancy you can change the rules to create an empty Setup.local if it
doesn't exist and run it with arguments $(srcdir)/Setup Setup.local;
however this assumes that you only need to add modules.)

Also note that you can't use a workspace for VPATH and non VPATH builds. The
object files left behind by one version confuses the other.


Building on non-UNIX systems
----------------------------

For Windows (2000/NT/ME/98/95), assuming you have MS VC++ 7.1, the
project files are in PCbuild, the workspace is pcbuild.dsw.  See
PCbuild\readme.txt for detailed instructions.

For other non-Unix Windows compilers, in particular MS VC++ 6.0 and
for OS/2, enter the directory "PC" and read the file "readme.txt".

For the Mac, a separate source distribution will be made available,
for use with the CodeWarrior compiler.  If you are interested in Mac
development, join the PythonMac Special Interest Group
(http://www.python.org/sigs/pythonmac-sig/, or send email to
pythonmac-sig-request@python.org).

Of course, there are also binary distributions available for these
platforms -- see http://www.python.org/.

To port Python to a new non-UNIX system, you will have to fake the
effect of running the configure script manually (for Mac and PC, this
has already been done for you).  A good start is to copy the file
pyconfig.h.in to pyconfig.h and edit the latter to reflect the actual
configuration of your system.  Most symbols must simply be defined as
1 only if the corresponding feature is present and can be left alone
otherwise; however the *_t type symbols must be defined as some
variant of int if they need to be defined at all.

For all platforms, it's important that the build arrange to define the
preprocessor symbol NDEBUG on the compiler command line in a release
build of Python (else assert() calls remain in the code, hurting
release-build performance).  The Unix, Windows and Mac builds already
do this.


Miscellaneous issues
====================

Emacs mode
----------

There's an excellent Emacs editing mode for Python code; see the file
Misc/python-mode.el.  Originally written by the famous Tim Peters, it is now
maintained by the equally famous Barry Warsaw.  The latest version, along with
various other contributed Python-related Emacs goodies, is online at
http://launchpad.net/python-mode/.


Tkinter
-------

The setup.py script automatically configures this when it detects a
usable Tcl/Tk installation.  This requires Tcl/Tk version 8.0 or
higher.

For more Tkinter information, see the Tkinter Resource page:
http://www.python.org/topics/tkinter/

There are demos in the Demo/tkinter directory.

Note that there's a Python module called "Tkinter" (capital T) which
lives in Lib/lib-tk/Tkinter.py, and a C module called "_tkinter"
(lower case t and leading underscore) which lives in
Modules/_tkinter.c.  Demos and normal Tk applications import only the
Python Tkinter module -- only the latter imports the C _tkinter
module.  In order to find the C _tkinter module, it must be compiled
and linked into the Python interpreter -- the setup.py script does
this.  In order to find the Python Tkinter module, sys.path must be
set correctly -- normal installation takes care of this.


Distribution structure
----------------------

Most subdirectories have their own README files.  Most files have
comments.

Demo/           Demonstration scripts, modules and programs
Doc/            Documentation sources (reStructuredText)
Grammar/        Input for the parser generator
Include/        Public header files
LICENSE         Licensing information
Lib/            Python library modules
Mac/            Macintosh specific resources
Makefile.pre.in Source from which config.status creates the Makefile.pre
Misc/           Miscellaneous useful files
Modules/        Implementation of most built-in modules
Objects/        Implementation of most built-in object types
PC/             Files specific to PC ports (DOS, Windows, OS/2)
PCbuild/        Build directory for Microsoft Visual C++
Parser/         The parser and tokenizer and their input handling
Python/         The byte-compiler and interpreter
README          The file you're reading now
RISCOS/         Files specific to RISC OS port
Tools/          Some useful programs written in Python
pyconfig.h.in   Source from which pyconfig.h is created (GNU autoheader output)
configure       Configuration shell script (GNU autoconf output)
configure.in    Configuration specification (input for GNU autoconf)
install-sh      Shell script used to install files
setup.py        Python script used to build extension modules

The following files will (may) be created in the toplevel directory by
the configuration and build processes:

Makefile        Build rules
Makefile.pre    Build rules before running Modules/makesetup
buildno         Keeps track of the build number
config.cache    Cache of configuration variables
pyconfig.h      Configuration header
config.log      Log from last configure run
config.status   Status from last run of the configure script
getbuildinfo.o  Object file from Modules/getbuildinfo.c
libpython<version>.a    The library archive
python          The executable interpreter
reflog.txt      Output from running the regression suite with the -R flag 
tags, TAGS      Tags files for vi and Emacs


That's all, folks!
------------------


--Guido van Rossum (home page: http://www.python.org/~guido/)
