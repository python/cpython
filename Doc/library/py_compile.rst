:mod:`py_compile` --- Compile Python source files
=================================================

.. module:: py_compile
   :synopsis: Generate byte-code files from Python source files.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. documentation based on module docstrings

.. index:: pair: file; byte-code

The :mod:`py_compile` module provides a function to generate a byte-code file
from a source file, and another function used when the module source file is
invoked as a script.

Though not often needed, this function can be useful when installing modules for
shared use, especially if some of the users may not have permission to write the
byte-code cache files in the directory containing the source code.


.. exception:: PyCompileError

   Exception raised when an error occurs while attempting to compile the file.


.. function:: compile(file[, cfile[, dfile[, doraise]]])

   Compile a source file to byte-code and write out the byte-code cache  file.  The
   source code is loaded from the file name *file*.  The  byte-code is written to
   *cfile*, which defaults to *file* ``+`` ``'c'`` (``'o'`` if optimization is
   enabled in the current interpreter).  If *dfile* is specified, it is used as the
   name of the source file in error messages instead of *file*.  If *doraise* is
   true, a :exc:`PyCompileError` is raised when an error is encountered while
   compiling *file*. If *doraise* is false (the default), an error string is
   written to ``sys.stderr``, but no exception is raised.


.. function:: main([args])

   Compile several source files.  The files named in *args* (or on the command
   line, if *args* is not specified) are compiled and the resulting bytecode is
   cached in the normal manner.  This function does not search a directory
   structure to locate source files; it only compiles files named explicitly.

When this module is run as a script, the :func:`main` is used to compile all the
files named on the command line.  The exit status is nonzero if one of the files
could not be compiled.


.. seealso::

   Module :mod:`compileall`
      Utilities to compile all Python source files in a directory tree.

