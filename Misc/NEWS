=================================
==> Release 1.1 (11 Oct 1994) <==
=================================

This release adds several new features, improved configuration and
portability, and fixes more bugs than I can list here (including some
memory leaks).

The source compiles and runs out of the box on more platforms than
ever -- including Windows NT.  Makefiles or projects for a variety of
non-UNIX platforms are provided.

APOLOGY: some new features are badly documented or not at all.  I had
the choice -- postpone the new release indefinitely, or release it
now, with working code but some undocumented areas.  The problem with
postponing the release is that people continue to suffer from existing
bugs, and send me patches based on the previous release -- which I
can't apply directly because my own source has changed.  Also, some
new modules (like signal) have been ready for release for quite some
time, and people are anxiously waiting for them.  In the case of
signal, the interface is simple enough to figure out without
documentation (if you're anxious enough :-).  In this case it was not
simple to release the module on its own, since it relies on many small
patches elsewhere in the source.

For most new Python modules, the source code contains comments that
explain how to use them.  Documentation for the Tk interface, written
by Matt Conway, is available as tkinter-doc.tar.gz from the Python
home and mirror ftp sites (see Misc/FAQ for ftp addresses).  For the
new operator overloading facilities, have a look at Demo/classes:
Complex.py and Rat.py show how to implement a numeric type without and
with __coerce__ method.  Also have a look at the end of the Tutorial
document (Doc/tut.tex).  If you're still confused: use the newsgroup
or mailing list.


New language features:

    - More flexible operator overloading for user-defined classes
    (INCOMPATIBLE WITH PREVIOUS VERSIONS!)  See end of tutorial.

    - Classes can define methods named __getattr__, __setattr__ and
    __delattr__ to trap attribute accesses.  See end of tutorial.

    - Classes can define method __call__ so instances can be called
    directly.  See end of tutorial.


New support facilities:

    - The Makefiles (for the base interpreter as well as for extensions)
    now support creating dynamically loadable modules if the platform
    supports shared libraries.

    - Passing the interpreter a .pyc file as script argument will execute
    the code in that file.  (On the Mac such files can be double-clicked!)

    - New Freeze script, to create independently distributable "binaries"
    of Python programs -- look in Demo/freeze

    - Improved h2py script (in Demo/scripts) follows #includes and
    supports macros with one argument

    - New module compileall generates .pyc files for all modules in a
    directory (tree) without also executing them

    - Threads should work on more platforms


New built-in modules:

    - tkinter (support for Tcl's Tk widget set) is now part of the base
    distribution

    - signal allows catching or ignoring UNIX signals (unfortunately still
    undocumented -- any taker?)

    - termios provides portable access to POSIX tty settings

    - curses provides an interface to the System V curses library

    - syslog provides an interface to the (BSD?) syslog daemon

    - 'new' provides interfaces to create new built-in object types
    (e.g. modules and functions)

    - sybase provides an interface to SYBASE database


New/obsolete built-in methods:

    - callable(x) tests whether x can be called

    - sockets now have a setblocking() method

    - sockets no longer have an allowbroadcast() method

    - socket methods send() and sendto() return byte count


New standard library modules:

    - types.py defines standard names for built-in types, e.g. StringType

    - urlparse.py parses URLs according to the latest Internet draft

    - uu.py does uuencode/uudecode (not the fastest in the world, but
    quicker than installing uuencode on a non-UNIX machine :-)

    - New, faster and more powerful profile module.py

    - mhlib.py provides interface to MH folders and messages


New facilities for extension writers (unfortunately still
undocumented):

    - newgetargs() supports optional arguments and improved error messages

    - O!, O& O? formats for getargs allow more versatile type checking of
    non-standard types

    - can register pending asynchronous callback, to be called the next
    time the Python VM begins a new instruction (Py_AddPendingCall)

    - can register cleanup routines to be called when Python exits
    (Py_AtExit)

    - makesetup script understands C++ files in Setup file (use file.C
    or file.cc)

    - Make variable OPT is passed on to sub-Makefiles

    - An init<module>() routine may signal an error by not entering
    the module in the module table and raising an exception instead

    - For long module names, instead of foobarbletchmodule.c you can
    use foobarbletch.c

    - getintvalue() and getfloatvalue() try to convert any object
    instead of requiring an "intobject" or "floatobject"

    - All the [new]getargs() formats that retrieve an integer value
    will now also work if a float is passed

    - C function listtuple() converts list to tuple, fast

    - You should now call sigcheck() instead of intrcheck();
    sigcheck() also sets an exception when it returns nonzero


--Guido van Rossum, CWI, Amsterdam <Guido.van.Rossum@cwi.nl>
URL:  <http://www.cwi.nl/cwi/people/Guido.van.Rossum.html>
