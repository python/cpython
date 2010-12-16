:mod:`compileall` --- Byte-compile Python libraries
===================================================

.. module:: compileall
   :synopsis: Tools for byte-compiling all Python source files in a directory tree.


This module provides some utility functions to support installing Python
libraries.  These functions compile Python source files in a directory tree,
allowing users without permission to write to the libraries to take advantage of
cached byte-code files.


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

   Do not recurse.

.. cmdoption:: -f

   Force rebuild even if timestamps are up-to-date.

.. cmdoption:: -q

   Do not print the list of files compiled.

.. cmdoption:: -d destdir

   Purported directory name for error messages.

.. cmdoption:: -x regex

   Skip files with a full path that matches given regular expression.

.. cmdoption:: -i list

   Expand list with its content (file and directory names).

.. cmdoption:: -b

   Write legacy ``.pyc`` file path names.  Default is to write :pep:`3147`-style
   byte-compiled path names.

.. versionadded:: 3.2
   The ``-i`` and ``-b`` options.


Public functions
----------------

.. function:: compile_dir(dir, maxlevels=10, ddir=None, force=False, rx=None, quiet=False, legacy=False, optimize=-1)

   Recursively descend the directory tree named by *dir*, compiling all :file:`.py`
   files along the way.  The *maxlevels* parameter is used to limit the depth of
   the recursion; it defaults to ``10``.  If *ddir* is given, it is used as the
   base path from  which the filenames used in error messages will be generated.
   If *force* is true, modules are re-compiled even if the timestamps are up to
   date.

   If *rx* is given, it specifies a regular expression of file names to exclude
   from the search; that expression is searched for in the full path.

   If *quiet* is true, nothing is printed to the standard output in normal
   operation.

   If *legacy* is true, old-style ``.pyc`` file path names are written,
   otherwise (the default), :pep:`3147`-style path names are written.

   *optimize* specifies the optimization level for the compiler.  It is passed to
   the built-in :func:`compile` function.

   .. versionchanged:: 3.2
      Added the *optimize* parameter.


.. function:: compile_path(skip_curdir=True, maxlevels=0, force=False, legacy=False, optimize=-1)

   Byte-compile all the :file:`.py` files found along ``sys.path``. If
   *skip_curdir* is true (the default), the current directory is not included in
   the search.  All other parameters are passed to the :func:`compile_dir`
   function.

   .. versionchanged:: 3.2
      Added the *optimize* parameter.


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
