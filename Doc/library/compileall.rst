:mod:`compileall` --- Byte-compile Python libraries
===================================================

.. module:: compileall
   :synopsis: Tools for byte-compiling all Python source files in a directory tree.


This module provides some utility functions to support installing Python
libraries.  These functions compile Python source files in a directory tree.
This module can be used to create the cached byte-code files at library
installation time, which makes them available for use even by users who don't
have write permission to the library directories.


Command-line use
----------------

This module can work as a script (using :program:`python -m compileall`) to
compile Python sources.

.. program:: compileall

.. cmdoption:: [directory|file]...

   Positional arguments are files to compile or directories that contain
   source files, traversed recursively.  If no argument is given, behave as if
   the command line was ``-l <directories from sys.path>``.

.. cmdoption:: -l

   Do not recurse into subdirectories, only compile source code files directly
   contained in the named or implied directories.

.. cmdoption:: -f

   Force rebuild even if timestamps are up-to-date.

.. cmdoption:: -q

   Do not print the list of files compiled, print only error messages.

.. cmdoption:: -d destdir

   Directory prepended to the path to each file being compiled.  This will
   appear in compilation time tracebacks, and is also compiled in to the
   byte-code file, where it will be used in tracebacks and other messages in
   cases where the source file does not exist at the time the byte-code file is
   executed.

.. cmdoption:: -x regex

   regex is used to search the full path to each file considered for
   compilation, and if the regex produces a match, the file is skipped.

.. cmdoption:: -i list

   Read the file ``list`` and add each line that it contains to the list of
   files and directories to compile.  If ``list`` is ``-``, read lines from
   ``stdin``.

.. cmdoption:: -b

   Write the byte-code files to their legacy locations and names, which may
   overwrite byte-code files created by another version of Python.  The default
   is to write files to their :pep:`3147` locations and names, which allows
   byte-code files from multiple versions of Python to coexist.

.. versionchanged:: 3.2
   Added the ``-i``, ``-b`` and ``-h`` options.


Public functions
----------------

.. function:: compile_dir(dir, maxlevels=10, ddir=None, force=False, rx=None, quiet=False, legacy=False, optimize=-1)

   Recursively descend the directory tree named by *dir*, compiling all :file:`.py`
   files along the way.

   The *maxlevels* parameter is used to limit the depth of the recursion; it
   defaults to ``10``.

   If *ddir* is given, it is prepended to the path to each file being compiled
   for use in compilation time tracebacks, and is also compiled in to the
   byte-code file, where it will be used in tracebacks and other messages in
   cases where the source file does not exist at the time the byte-code file is
   executed.

   If *force* is true, modules are re-compiled even if the timestamps are up to
   date.

   If *rx* is given, its search method is called on the complete path to each
   file considered for compilation, and if it returns a true value, the file
   is skipped.

   If *quiet* is true, nothing is printed to the standard output unless errors
   occur.

   If *legacy* is true, byte-code files are written to their legacy locations
   and names, which may overwrite byte-code files created by another version of
   Python.  The default is to write files to their :pep:`3147` locations and
   names, which allows byte-code files from multiple versions of Python to
   coexist.

   *optimize* specifies the optimization level for the compiler.  It is passed to
   the built-in :func:`compile` function.

   .. versionchanged:: 3.2
      Added the *legacy* and *optimize* parameter.


.. function:: compile_file(fullname, ddir=None, force=False, rx=None, quiet=False, legacy=False, optimize=-1)

   Compile the file with path *fullname*.

   If *ddir* is given, it is prepended to the path to the file being compiled
   for use in compilation time tracebacks, and is also compiled in to the
   byte-code file, where it will be used in tracebacks and other messages in
   cases where the source file does not exist at the time the byte-code file is
   executed.

   If *ra* is given, its search method is passed the full path name to the
   file being compiled, and if it returns a true value, the file is not
   compiled and ``True`` is returned.

   If *quiet* is true, nothing is printed to the standard output unless errors
   occur.

   If *legacy* is true, byte-code files are written to their legacy locations
   and names, which may overwrite byte-code files created by another version of
   Python.  The default is to write files to their :pep:`3147` locations and
   names, which allows byte-code files from multiple versions of Python to
   coexist.

   *optimize* specifies the optimization level for the compiler.  It is passed to
   the built-in :func:`compile` function.

   .. versionadded:: 3.2


.. function:: compile_path(skip_curdir=True, maxlevels=0, force=False, legacy=False, optimize=-1)

   Byte-compile all the :file:`.py` files found along ``sys.path``. If
   *skip_curdir* is true (the default), the current directory is not included
   in the search.  All other parameters are passed to the :func:`compile_dir`
   function.  Note that unlike the other compile functions, ``maxlevels``
   defaults to ``0``.

   .. versionchanged:: 3.2
      Added the *legacy* and *optimize* parameter.


To force a recompile of all the :file:`.py` files in the :file:`Lib/`
subdirectory and all its subdirectories::

   import compileall

   compileall.compile_dir('Lib/', force=True)

   # Perform same compilation, excluding files in .svn directories.
   import re
   compileall.compile_dir('Lib/', rx=re.compile('/[.]svn'), force=True)


.. seealso::

   Module :mod:`py_compile`
      Byte-compile a single source file.
