
:mod:`compileall` --- Byte-compile Python libraries
===================================================

.. module:: compileall
   :synopsis: Tools for byte-compiling all Python source files in a directory tree.


This module provides some utility functions to support installing Python
libraries.  These functions compile Python source files in a directory tree,
allowing users without permission to write to the libraries to take advantage of
cached byte-code files.

This module may also be used as a script (using the :option:`-m` Python flag) to
compile Python sources.  Directories to recursively traverse (passing
:option:`-l` stops the recursive behavior) for sources are listed on the command
line.  If no arguments are given, the invocation is equivalent to ``-l
sys.path``.  Printing lists of the files compiled can be disabled with the
:option:`-q` flag.  In addition, the :option:`-x` option takes a regular
expression argument.  All files that match the expression will be skipped.


.. function:: compile_dir(dir[, maxlevels[, ddir[, force[,  rx[, quiet]]]]])

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


.. function:: compile_path([skip_curdir[, maxlevels[, force]]])

   Byte-compile all the :file:`.py` files found along ``sys.path``. If
   *skip_curdir* is true (the default), the current directory is not included in
   the search.  The *maxlevels* and *force* parameters default to ``0`` and are
   passed to the :func:`compile_dir` function.

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

