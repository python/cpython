===================================
==> Release 1.2 <==
===================================

- Changes to Misc/python-mode.el:
  - Wrapping and indentation within triple quote strings should work
    properly now.
  - `Standard' bug reporting mechanism (use C-c C-b)
  - py-mark-block was moved to C-c C-m
  - C-c C-v shows you the python-mode version
  - a basic python-font-lock-keywords has been added for Emacs 19
    font-lock colorizations.
  - proper interaction with pending-del and del-sel modes.
  - New py-electric-colon (:) command for improved outdenting.  Also
    py-indent-line (TAB) should handle outdented lines better.
  - New commands py-outdent-left (C-c C-l) and py-indent-right (C-c C-r)

- The Library Reference has been restructured, and many new and
existing modules are now documented, in particular the debugger and
the profiler, as well as the persistency and the WWW/Internet support
modules.

- All known bugs have been fixed.  For example the pow(2,2,3L) bug on
Linux has been fixed.  Also the re-entrancy problems with __del__ have
been fixed.

- All known memory leaks have been fixed.

- Phase 2 of the Great Renaming has been executed.  The header files
now use the new names (PyObject instead of object, etc.).  The linker
also sees the new names.  Most source files still use the old names,
by virtue of the rename2.h header file.  If you include Python.h, you
only see the new names.  Dynamically linked modules have to be
recompiled.  (Phase 3, fixing the rest of the sources, will be
executed gradually with the release later versions.)

- The hooks for implementing "safe-python" (better called "restricted
execution") are in place.  Specifically, the import statement is
implemented by calling the built-in function __import__, and the
built-in names used in a particular scope are taken from the
dictionary __builtins__ in that scope's global dictionary.  See also
the new (unsupported, undocumented) module rexec.py.

- The import statement now supports the syntax "import a.b.c" and
"from a.b.c import name".  No officially supported implementation
exists, but one can be prototyped by replacing the built-in __import__
function.  A proposal by Ken Manheimer is provided as newimp.py.

- All machinery used by the import statement (or the built-in
__import__ function) is now exposed through the new built-in module
"imp" (see the library reference manual).  All dynamic loading
machinery is moved to the new file importdl.c.

- Persistent storage is supported through the use of the modules
"pickle" and "shelve" (implemented in Python).  There's also a "copy"
module implementing deepcopy and normal (shallow) copy operations.
See the library reference manual.

- Documentation strings for many objects types are accessible through
the __doc__ attribute.  Modules, classes and functions support special
syntax to initialize the __doc__ attribute: if the first statement
consists of just a string literal, that string literal becomes the
value of the __doc__ attribute.  The default __doc__ attribute is
None.  Documentation strings are also supported for built-in
functions, types and modules; however this feature hasn't been widely
used yet.  See the 'new' module for an example.  (Basically, the type
object's tp_doc field contains the doc string for the type, and the
4th member of the methodlist structure contains the doc string for the
method.)

- The __coerce__ and __cmp__ methods for user-defined classes once
again work as expected.  As an example, there's a new standard class
Complex in the library.

- The functions posix.popen() and posix.fdopen() now have an optional
third argument to specify the buffer size, and default their second
(mode) argument to 'r' -- in analogy to the builtin open() function.
The same applies to posixfile.open() and the socket method makefile().

- The thread.exit_thread() function now raises SystemExit so that
'finally' clauses are honored and a memory leak is plugged.

- Improved X11 and Motif support, by Sjoerd Mullender.  This extension
is being maintained and distributed separately.

- Improved support for the Apple Macintosh, in part by Jack Jansen,
e.g. interfaces to (a few) resource mananger functions, get/set file
type and creator, gestalt, sound manager, speech manager, MacTCP, comm
toolbox, and the think C console library.  This is being maintained
and distributed separately.

- Improved version for Windows NT, by Mark Hammond.  This is being
maintained and distributed separately.

- Used autoconf 2.0 to generate the configure script.  Adapted
configure.in to use the new features in autoconf 2.0.

- It now builds on the NeXT without intervention, even on the 3.3
Sparc pre-release.

- Characters passed to isspace() and friends are masked to nonnegative
values.

- Correctly compute pow(-3.0, 3).

- Fix portability problems with getopt (configure now checks for a
non-GNU getopt).

- Don't add frozenmain.o to libPython.a.

- Exceptions can now be classes.  ALl built-in exceptions are still
string objects, but this will change in the future.

- The socket module exports a long list of socket related symbols.
(More built-in modules will export their symbolic constants instead of
relying on a separately generated Python module.)

- When a module object is deleted, it clears out its own dictionary.
This fixes a circularity in the references between functions and
their global dictionary.

- Changed the error handling by [new]getargs() e.g. for "O&".

- Dynamic loading of modules using shared libraries is supported for
several new platforms.

- Support "O&", "[...]" and "{...}" in mkvalue().

- Extension to findmethod(): findmethodinchain() (where a chain is a
linked list of methodlist arrays).  The calling interface for
findmethod() has changed: it now gets a pointer to the (static!)
methodlist structure rather than just to the function name -- this
saves copying flags etc. into the (short-lived) method object.

- The callable() function is now public.

- Object types can define a few new operations by setting function
pointers in the type object structure: tp_call defines how an object
is called, and tp_str defines how an object's str() is computed.

--Guido van Rossum, CWI, Amsterdam <mailto:guido@cwi.nl>
<http://www.cwi.nl/~guido/>
