=====================================
==> Release 1.4 (October 25 1996) <==
=====================================

(Starting in reverse chronological order:)

- Changed disclaimer notice.

- Added SHELL=/bin/sh to Misc/Makefile.pre.in -- some Make versions
default to the user's login shell.

- In Lib/tkinter/Tkinter.py, removed bogus binding of <Delete> in Text
widget, and bogus bspace() function.

- In Lib/cgi.py, bumped __version__ to 2.0 and restored a truncated
paragraph.

- Fixed the NT Makefile (PC/vc40.mak) for VC 4.0 to set /MD for all
subprojects, and to remove the (broken) experimental NumPy
subprojects.

- In Lib/py_compile.py, cast mtime to long() so it will work on Mac
(where os.stat() returns mtimes as floats.)
- Set self.rfile unbuffered (like self.wfile) in SocketServer.py, to
fix POST in CGIHTTPServer.py.

- Version 2.83 of Misc/python-mode.el for Emacs is included.

- In Modules/regexmodule.c, fixed symcomp() to correctly handle a new
group starting immediately after a group tag.

- In Lib/SocketServer.py, changed the mode for rfile to unbuffered.

- In Objects/stringobject.c, fixed the compare function to do the
first char comparison in unsigned mode, for consistency with the way
other characters are compared by memcmp().

- In Lib/tkinter/Tkinter.py, fixed Scale.get() to support floats.

- In Lib/urllib.py, fix another case where openedurl wasn't set.

(XXX Sorry, the rest is in totally random order.  No time to fix it.)

- SyntaxError exceptions detected during code generation
(e.g. assignment to an expression) now include a line number.

- Don't leave trailing / or \ in script directory inserted in front of
sys.path.

- Added a note to Tools/scripts/classfix.py abouts its historical
importance.

- Added Misc/Makefile.pre.in, a universal Makefile for extensions
built outside the distribution.

- Rewritten Misc/faq2html.py, by Ka-Ping Yee.

- Install shared modules with mode 555 (needed for performance on some
platforms).

- Some changes to standard library modules to avoid calling append()
with more than one argument -- while supported, this should be
outlawed, and I don't want to set a bad example.

- bdb.py (and hence pdb.py) supports calling run() with a code object
instead of a code string.

- Fixed an embarrassing bug cgi.py which prevented correct uploading
of binary files from Netscape (which doesn't distinguish between
binary and text files).  Also added dormant logging support, which
makes it easier to debug the cgi module itself.

- Added default writer to constructor of NullFormatter class.

- Use binary mode for socket.makefile() calls in ftplib.py.

- The ihooks module no longer "installs" itself upon import -- this
was an experimental feature that helped ironing out some bugs but that
slowed down code that imported it without the need to install it
(e.g. the rexec module).  Also close the file in some cases and add
the __file__ attribute to loaded modules.

- The test program for mailbox.py is now more useful.

- Added getparamnames() to Message class in mimetools.py -- it returns
the names of parameters to the content-type header.

- Fixed a typo in ni that broke the loop stripping "__." from names.

- Fix sys.path[0] for scripts run via pdb.py's new main program.

- profile.py can now also run a script, like pdb.

- Fix a small bug in pyclbr -- don't add names starting with _ when
emulating from ... import *.

- Fixed a series of embarrassing typos in rexec's handling of standard
I/O redirection.  Added some more "safe" built-in modules: cmath,
errno, operator.

- Fixed embarrassing typo in shelve.py.

- Added SliceType and EllipsisType to types.py.

- In urllib.py, added handling for error 301 (same as 302); added
geturl() method to get the URL after redirection.

- Fixed embarrassing typo in xdrlib.py.  Also fixed typo in Setup.in
for _xdrmodule.c and removed redundant #include from _xdrmodule.c.

- Fixed bsddbmodule.c to add binary mode indicator on platforms that
have it.  This should make it working on Windows NT.

- Changed last uses of #ifdef NT to #ifdef MS_WINDOWS or MS_WIN32,
whatever applies.  Also rationalized some other tests for various MS
platforms.

- Added the sources for the NT installer script used for Python
1.4beta3.  Not tested with this release, but better than nothing.

- A compromise in pickle's defenses against Trojan horses: a
user-defined function is now okay where a class is expected.  A
built-in function is not okay, to prevent pickling something that
will execute os.system("rm -f *") when unpickling.

- dis.py will print the name of local variables referenced by local
load/store/delete instructions.

- Improved portability of SimpleHTTPServer module to non-Unix
platform.

- The thread.h interface adds an extra argument to down_sema().  This
only affects other C code that uses thread.c; the Python thread module
doesn't use semaphores (which aren't provided on all platforms where
Python threads are supported).  Note: on NT, this change is not
implemented.

- Fixed some typos in abstract.h; corrected signature of
PyNumber_Coerce, added PyMapping_DelItem.  Also fixed a bug in
abstract.c's PyObject_CallMethod().

- apply(classname, (), {}) now works even if the class has no
__init__() method.

- Implemented complex remainder and divmod() (these would dump core!).
Conversion of complex numbers to int, long int or float now raises an
exception, since there is no meaningful way to do it without losing
information.

- Fixed bug in built-in complex() function which gave the wrong result
for two real arguments.

- Change the hash algorithm for strings -- the multiplier is now
1000003 instead of 3, which gives better spread for short strings.

- New default path for Windows NT, the registry structure now supports
default paths for different install packages.  (Mark Hammond -- the
next PythonWin release will use this.)

- Added more symbols to the python_nt.def file.

- When using GNU readline, set rl_readline_name to "python".

- The Ellipses built-in name has been renamed to Ellipsis -- this is
the correct singular form.  Thanks to Ka-Ping Yee, who saved us from
eternal embarrassment.

- Bumped the PYTHON_API_VERSION to 1006, due to the Ellipses ->
Ellipsis name change.

- Updated the library reference manual.  Added documentation of
restricted mode (rexec, Bastion) and the formatter module (for use
with the htmllib module).  Fixed the documentation of htmllib
(finally).

- The reference manual is now maintained in FrameMaker.

- Upgraded scripts Doc/partparse.py and Doc/texi2html.py.

- Slight improvements to Doc/Makefile.

- Added fcntl.lockf(). This should be used for Unix file locking
instead of the posixfile module; lockf() is more portable.

- The getopt module now supports long option names, thanks to Lars
Wizenius.

- Plenty of changes to Tkinter and Canvas, mostly due to Fred Drake
and Nils Fischbeck.

- Use more bits of time.time() in whrandom's default seed().

- Performance hack for regex module's regs attribute.

- Don't close already closed socket in socket module.

- Correctly handle separators containing embedded nulls in
strop.split, strop.find and strop.rfind.  Also added more detail to
error message for strop.atoi and friends.

- Moved fallback definition for hypot() to Python/hypot.c.

- Added fallback definition for strdup, in Python/strdup.c.

- Fixed some bugs where a function would return 0 to indicate an error
where it should return -1.

- Test for error returned by time.localtime(), and rationalized its MS
tests.

- Added Modules/Setup.local file, which is processed after Setup.

- Corrected bug in toplevel Makefile.in -- execution of regen script
would not use the right PATH and PYTHONPATH.

- Various and sundry NeXT configuration changes (sigh).

- Support systems where libreadline needs neither termcap nor curses.

- Improved ld_so_aix script and python.exp file (for AIX).

- More stringent test for working <stdarg.h> in configure script.

- Removed Demo/www subdirectory -- it was totally out of date.

- Improved demos and docs for Fred Drake's parser module; fixed one
typo in the module itself.


=========================================
==> Release 1.4beta3 (August 26 1996) <==
=========================================


(XXX This is less readable that it should.  I promise to restructure
it for the final 1.4 release.)


What's new in 1.4beta3 (since beta2)?
-------------------------------------

- Name mangling to implement a simple form of class-private variables.
A name of the form "__spam" can't easily be used outside the class.
(This was added in 1.4beta3, but left out of the 1.4beta3 release
message.)

- In urllib.urlopen(): HTTP URLs containing user:passwd@host are now
handled correctly when using a proxy server.

- In ntpath.normpath(): don't truncate to 8+3 format.

- In mimetools.choose_boundary(): don't die when getuid() or getpid()
aren't defined.

- Module urllib: some optimizations to (un)quoting.

- New module MimeWriter for writing MIME documents.

- More changes to formatter module.

- The freeze script works once again and is much more robust (using
sys.prefix etc.).  It also supports a -o option to specify an
output directory.

- New module whichdb recognizes dbm, gdbm and bsddb/dbhash files.

- The Doc/Makefile targets have been reorganized somewhat to remove the 
insistence on always generating PostScript.

- The texinfo to html filter (Doc/texi2html.py) has been improved somewhat.

- "errors.h" has been renamed to "pyerrors.h" to resolve a long-standing 
name conflict on the Mac.

- Linking a module compiled with a different setting for Py_TRACE_REFS now 
generates a linker error rather than a core dump.

- The cgi module has a new convenience function print_exception(), which 
formats a python exception using HTML.  It also fixes a bug in the 
compatibility code and adds a dubious feature which makes it possible to 
have two query strings, one in the URL and one in the POST data.

- A subtle change in the unpickling of class instances makes it possible 
to unpickle in restricted execution mode, where the __dict__ attribute is 
not available (but setattr() is).

- Documentation for os.path.splitext() (== posixpath.splitext()) has been 
cleared up.  It splits at the *last* dot.

- posixfile locking is now also correctly supported on AIX.

- The tempfile module once again honors an initial setting of tmpdir.  It 
now works on Windows, too.

- The traceback module has some new functions to extract, format and print 
the active stack.

- Some translation functions in the urllib module have been made a little 
less sluggish.

- The addtag_* methods for Canvas widgets in Tkinter as well as in the 
separate Canvas class have been fixed so they actually do something 
meaningful.

- A tiny _test() function has been added to Tkinter.py.

- A generic Makefile for dynamically loaded modules is provided in the Misc 
subdirectory (Misc/gMakefile).

- A new version of python-mode.el for Emacs is provided.  See
http://www.python.org/ftp/emacs/pmdetails.html for details.  The
separate file pyimenu.el is no longer needed, imenu support is folded
into python-mode.el.

- The configure script can finally correctly find the readline library in a 
non-standard location.  The LDFLAGS variable is passed on the the Makefiles 
from the configure script.

- Shared libraries are now installed as programs (i.e. with executable 
permission).  This is required on HP-UX and won't hurt on other systems.

- The objc.c module is no longer part of the distribution.  Objective-C 
support may become available as contributed software on the ftp site.

- The sybase module is no longer part of the distribution.  A much
improved sybase module is available as contributed software from the
ftp site.

- _tkinter is now compatible with Tcl 7.5 / Tk 4.1 patch1 on Windows and 
Mac (don't use unpatched Tcl/Tk!).  The default line in the Setup.in file 
now links with Tcl 7.5 / Tk 4.1 rather than 7.4/4.0.

- In Setup, you can now write "*shared*" instead of "*noconfig*", and you 
can use *.so and *.sl as shared libraries.

- Some more fidgeting for AIX shared libraries.

- The mpz module is now compatible with GMP 2.x.  (Not tested by me.)
(Note -- a complete replacement by Niels Mo"ller, called gpmodule, is
available from the contrib directory on the ftp site.)

- A warning is written to sys.stderr when a __del__ method raises an 
exception (formerly, such exceptions were completely ignored).

- The configure script now defines HAVE_OLD_CPP if the C preprocessor is 
incapable of ANSI style token concatenation and stringification.

- All source files (except a few platform specific modules) are once again 
compatible with K&R C compilers as well as ANSI compilers.  In particular,
ANSI-isms have been removed or made conditional in complexobject.c, 
getargs.c and operator.c.

- The abstract object API has three new functions, PyObject_DelItem, 
PySequence_DelItem, and PySequence_DelSlice.

- The operator module has new functions delitem and delslice, and the 
functions "or" and "and" are renamed to "or_" and "and_" (since "or" and 
"and" are reserved words).  ("__or__" and "__and__" are unchanged.)

- The environment module is no longer supported; putenv() is now a function 
in posixmodule (also under NT).

- Error in filter(<function>, "") has been fixed.

- Unrecognized keyword arguments raise TypeError, not KeyError.

- Better portability, fewer bugs and memory leaks, fewer compiler warnings, 
some more documentation.

- Bug in float power boundary case (0.0 to the negative integer power) 
fixed.

- The test of negative number to the float power has been moved from the 
built-in pow() functin to floatobject.c (so complex numbers can yield the 
correct result).

- The bug introduced in beta2 where shared libraries loaded (using 
dlopen()) from the current directory would fail, has been fixed.

- Modules imported as shared libraries now also have a __file__ attribute, 
giving the filename from which they were loaded.  The only modules without 
a __file__ attribute now are built-in modules.

- On the Mac, dynamically loaded modules can end in either ".slb" or 
".<platform>.slb" where <platform> is either "CFM68K" or "ppc".  The ".slb" 
extension should only be used for "fat" binaries.

- C API addition: marshal.c now supports 
PyMarshal_WriteObjectToString(object).

- C API addition: getargs.c now supports
PyArg_ParseTupleAndKeywords(args, kwdict, format, kwnames, ...)
to parse keyword arguments.

- The PC versioning scheme (sys.winver) has changed once again.  the 
version number is now "<digit>.<digit>.<digit>.<apiversion>", where the 
first three <digit>s are the Python version (e.g. "1.4.0" for Python 1.4, 
"1.4.1" for Python 1.4.1 -- the beta level is not included) and 
<apiversion> is the four-digit PYTHON_API_VERSION (currently 1005).

- h2py.py accepts whitespace before the # in CPP directives

- On Solaris 2.5, it should now be possible to use either Posix threads or 
Solaris threads (XXX: how do you select which is used???).  (Note: the 
Python pthreads interface doesn't fully support semaphores yet -- anyone 
care to fix this?)

- Thread support should now work on AIX, using either DCE threads or 
pthreads.

- New file Demo/sockets/unicast.py

- Working Mac port, with CFM68K support, with Tk 4.1 support (though not 
both) (XXX)

- New project setup for PC port, now compatible with PythonWin, with 
_tkinter and NumPy support (XXX)

- New module site.py (XXX)

- New module xdrlib.py and optional support module _xdrmodule.c (XXX)

- parser module adapted to new grammar, complete w/ Doc & Demo (XXX)

- regen script fixed (XXX)

- new machdep subdirectories Lib/{aix3,aix4,next3_3,freebsd2,linux2} (XXX)

- testall now also tests math module (XXX)

- string.atoi c.s. now raise an exception for an empty input string.

- At last, it is no longer necessary to define HAVE_CONFIG_H in order to 
have config.h included at various places.

- Unrecognized keyword arguments now raise TypeError rather than KeyError.

- The makesetup script recognizes files with extension .so or .sl as
(shared) libraries.

- 'access' is no longer a reserved word, and all code related to its 
implementation is gone (or at least #ifdef'ed out).  This should make 
Python a little speedier too!

- Performance enhancements suggested by Sjoerd Mullender.  This includes 
the introduction of two new optional function pointers in type object, 
getattro and setattro, which are like getattr and setattr but take a 
string object instead of a C string pointer.

- New operations in string module: lstrip(s) and rstrip(s) strip whitespace 
only on the left or only on the right, A new optional third argument to 
split() specifies the maximum number of separators honored (so 
splitfields(s, sep, n) returns a list of at most n+1 elements).  (Since 
1.3, splitfields(s, None) is totally equivalent to split(s).)
string.capwords() has an optional second argument specifying the 
separator (which is passed to split()).

- regsub.split() has the same addition as string.split().  regsub.splitx(s, 
sep, maxsep) implements the functionality that was regsub.split(s, 1) in 
1.4beta2 (return a list containing the delimiters as well as the words).

- Final touch for AIX loading, rewritten Misc/AIX-NOTES.

- In Modules/_tkinter.c, when using Tk 4.1 or higher, use className
argument to _tkinter.create() to set Tcl's argv0 variable, so X
resources use the right resource class again.

- Add #undef fabs to Modules/mathmodule.c for macintosh.

- Added some macro renames for AIX in Modules/operator.c.

- Removed spurious 'E' from Doc/liberrno.tex.

- Got rid of some cruft in Misc/ (dlMakefile, pyimenu.el); added new
Misc/gMakefile and new version of Misc/python-mode.el.

- Fixed typo in Lib/ntpath.py (islink has "return false" which gives a
NameError).

- Added missing "from types import *" to Lib/tkinter/Canvas.py.

- Added hint about using default args for __init__ to pickle docs.

- Corrected typo in Inclide/abstract.h: PySequence_Lenth ->
PySequence_Length.

- Some improvements to Doc/texi2html.py.

- In Python/import.c, Cast unsigned char * in struct _frozen to char *
in calls to rds_object().

- In doc/ref4.tex, added note about scope of lambda bodies.

What's new in 1.4beta2 (since beta1)?
-------------------------------------

- Portability bug in the md5.h header solved.

- The PC build procedure now really works, and sets sys.platform to a
meaningful value (a few things were botched in beta 1).  Lib/dos_8x3
is now a standard part of the distribution (alas).

- More improvements to the installation procedure.  Typing "make install" 
now inserts the version number in the pathnames of almost everything 
installed, and creates the machine dependent modules (FCNTL.py etc.) if not 
supplied by the distribution.  (XXX There's still a problem with the latter 
because the "regen" script requires that Python is installed.  Some manual 
intervention may still be required.) (This has been fixed in 1.4beta3.)

- New modules: errno, operator (XXX).

- Changes for use with Numerical Python: builtin function slice() and
Ellipses object, and corresponding syntax:

	x[lo:hi:stride]		==	x[slice(lo, hi, stride)]
	x[a, ..., z]		==	x[(a, Ellipses, z)]

- New documentation for errno and cgi mdoules.

- The directory containing the script passed to the interpreter is
inserted in from of sys.path; "." is no longer a default path
component.

- Optional third string argument to string.translate() specifies
characters to delete.  New function string.maketrans() creates a
translation table for translate() or for regex.compile().

- Module posix (and hence module os under Unix) now supports putenv().
Moreover, module os is enhanced so that if putenv() is supported,
assignments to os.environ entries make the appropriate putenv() call.
(XXX the putenv() implementation can leak a small amount of memory per
call.)

- pdb.py can now be invoked from the command line to debug a script:
python pdb.py <script> <arg> ...

- Much improved parseaddr() in rfc822.

- In cgi.py, you can now pass an alternative value for environ to
nearly all functions.

- You can now assign to instance variables whose name begins and ends
with '__'.

- New version of Fred Drake's parser module and associates (token,
symbol, AST).

- New PYTHON_API_VERSION value and .pyc file magic number (again!).

- The "complex" internal structure type is now called "Py_complex" to
avoid name conflicts.

- Numerous small bugs fixed.

- Slight pickle speedups.

- Some slight speedups suggested by Sjoerd (more coming in 1.4 final).

- NeXT portability mods by Bill Bumgarner integrated.

- Modules regexmodule.c, bsddbmodule.c and xxmodule.c have been
converted to new naming style.


What's new in 1.4beta1 (since 1.3)?
-----------------------------------

- Added sys.platform and sys.exec_platform for Bill Janssen.

- Installation has been completely overhauled.  "make install" now installs 
everything, not just the python binary.  Installation uses the install-sh 
script (borrowed from X11) to install each file.

- New functions in the posix module: mkfifo, plock, remove (== unlink),
and ftruncate.  More functions are also available under NT.

- New function in the fcntl module: flock.

- Shared library support for FreeBSD.

- The --with-readline option can now be used without a DIRECTORY argument, 
for systems where libreadline.* is in one of the standard places.  It is 
also possible for it to be a shared library.

- The extension tkinter has been renamed to _tkinter, to avoid confusion 
with Tkinter.py oncase insensitive file systems.  It now supports Tk 4.1 as 
well as 4.0.

- Author's change of address from CWI in Amsterdam, The Netherlands, to 
CNRI in Reston, VA, USA.

- The math.hypot() function is now always available (if it isn't found in 
the C math library, Python provides its own implementation).

- The latex documentation is now compatible with latex2e, thanks to David 
Ascher.

- The expression x**y is now equivalent to pow(x, y).

- The indexing expression x[a, b, c] is now equivalent to x[(a, b, c)].

- Complex numbers are now supported.  Imaginary constants are written with 
a 'j' or 'J' prefix, general complex numbers can be formed by adding a real 
part to an imaginary part, like 3+4j.  Complex numbers are always stored in 
floating point form, so this is equivalent to 3.0+4.0j.  It is also 
possible to create complex numbers with the new built-in function 
complex(re, [im]).  For the footprint-conscious, complex number support can 
be disabled by defining the symbol WITHOUT_COMPLEX.

- New built-in function list() is the long-awaited counterpart of tuple().

- There's a new "cmath" module which provides the same functions as the 
"math" library but with complex arguments and results.  (There are very 
good reasons why math.sqrt(-1) still raises an exception -- you have to use 
cmath.sqrt(-1) to get 1j for an answer.)

- The Python.h header file (which is really the same as allobjects.h except 
it disables support for old style names) now includes several more files, 
so you have to have fewer #include statements in the average extension.

- The NDEBUG symbol is no longer used.  Code that used to be dependent on 
the presence of NDEBUG is now present on the absence of DEBUG.  TRACE_REFS 
and REF_DEBUG have been renamed to Py_TRACE_REFS and Py_REF_DEBUG, 
respectively.  At long last, the source actually compiles and links without 
errors when this symbol is defined.

- Several symbols that didn't follow the new naming scheme have been 
renamed (usually by adding to rename2.h) to use a Py or _Py prefix.  There 
are no external symbols left without a Py or _Py prefix, not even those 
defined by sources that were incorporated from elsewhere (regexpr.c, 
md5c.c).  (Macros are a different story...)

- There are now typedefs for the structures defined in config.c and 
frozen.c.

- New PYTHON_API_VERSION value and .pyc file magic number.

- New module Bastion.  (XXX)

- Improved performance of StringIO module.

- UserList module now supports + and * operators.

- The binhex and binascii modules now actually work.

- The cgi module has been almost totally rewritten and documented.
It now supports file upload and a new data type to handle forms more 
flexibly.

- The formatter module (for use with htmllib) has been overhauled (again).

- The ftplib module now supports passive mode and has doc strings.

- In (ideally) all places where binary files are read or written, the file 
is now correctly opened in binary mode ('rb' or 'wb') so the code will work 
on Mac or PC.

- Dummy versions of os.path.expandvars() and expanduser() are now provided 
on non-Unix platforms.

- Module urllib now has two new functions url2pathname and pathname2url 
which turn local filenames into "file:..." URLs using the same rules as 
Netscape (why be different).  it also supports urlretrieve() with a 
pathname parameter, and honors the proxy environment variables (http_proxy 
etc.).  The URL parsing has been improved somewhat, too.

- Micro improvements to urlparse.  Added urlparse.urldefrag() which 
removes a trailing ``#fragment'' if any.

- The mailbox module now supports MH style message delimiters as well.

- The mhlib module contains some new functionality: setcontext() to set the 
current folder and parsesequence() to parse a sequence as commonly passed 
to MH commands (e.g. 1-10 or last:5).

- New module mimify for conversion to and from MIME format of email 
messages.

- Module ni now automatically installs itself when first imported -- this 
is against the normal rule that modules should define classes and functions 
but not invoke them, but appears more useful in the case that two 
different, independent modules want to use ni's features.

- Some small performance enhancements in module pickle.

- Small interface change to the profile.run*() family of functions -- more 
sensible handling of return values.

- The officially registered Mac creator for Python files is 'Pyth'.  This 
replaces 'PYTH' which was used before but never registered.

- Added regsub.capwords().  (XXX)

- Added string.capwords(), string.capitalize() and string.translate().  
(XXX)

- Fixed an interface bug in the rexec module: it was impossible to pass a 
hooks instance to the RExec class.  rexec now also supports the dynamic 
loading of modules from shared libraries.  Some other interfaces have been 
added too.

- Module rfc822 now caches the headers in a dictionary for more efficient 
lookup.

- The sgmllib module now understands a limited number of SGML "shorthands" 
like <A/.../ for <A>...</A>.  (It's not clear that this was a good idea...)

- The tempfile module actually tries a number of different places to find a 
usable temporary directory.  (This was prompted by certain Linux 
installations that appear to be missing a /usr/tmp directory.) [A bug in 
the implementation that would ignore a pre-existing tmpdir global has been 
fixed in beta3.]

- Much improved and enhanved FileDialog module for Tkinter.

- Many small changes to Tkinter, to bring it more in line with Tk 4.0 (as 
well as Tk 4.1).

- New socket interfaces include ntohs(), ntohl(), htons(), htonl(), and 
s.dup().  Sockets now work correctly on Windows.  On Windows, the built-in 
extension is called _socket and a wrapper module win/socket.py provides 
"makefile()" and "dup()" functionality.  On Windows, the select module 
works only with socket objects.

- Bugs in bsddb module fixed (e.g. missing default argument values).

- The curses extension now includes <ncurses.h> when available.

- The gdbm module now supports opening databases in "fast" mode by 
specifying 'f' as the second character or the mode string.

- new variables sys.prefix and sys.exec_prefix pass corresponding 
configuration options / Makefile variables to the Python programmer.

- The ``new'' module now supports creating new user-defined classes as well 
as instances thereof.

- The soundex module now sports get_soundex() to get the soundex value for an 
arbitrary string (formerly it would only do soundex-based string 
comparison) as well as doc strings.

- New object type "cobject" to safely wrap void pointers for passing them 
between various extension modules.

- More efficient computation of float**smallint.

- The mysterious bug whereby "x.x" (two occurrences of the same 
one-character name) typed from the commandline would sometimes fail 
mysteriously.

- The initialization of the readline function can now be invoked by a C 
extension through PyOS_ReadlineInit().

- There's now an externally visible pointer PyImport_FrozenModules which 
can be changed by an embedding application.

- The argument parsing functions now support a new format character 'D' to 
specify complex numbers.

- Various memory leaks plugged and bugs fixed.

- Improved support for posix threads (now that real implementations are 
beginning to apepar).  Still no fully functioning semaphores.

- Some various and sundry improvements and new entries in the Tools 
directory.
