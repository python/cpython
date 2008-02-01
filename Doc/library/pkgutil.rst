
:mod:`pkgutil` --- Package extension utility
============================================

.. module:: pkgutil
   :synopsis: Utilities to support extension of packages.


This module provides a single function:


.. function:: extend_path(path, name)

   Extend the search path for the modules which comprise a package. Intended use is
   to place the following code in a package's :file:`__init__.py`::

      from pkgutil import extend_path
      __path__ = extend_path(__path__, __name__)

   This will add to the package's ``__path__`` all subdirectories of directories on
   ``sys.path`` named after the package.  This is useful if one wants to distribute
   different parts of a single logical package as multiple directories.

   It also looks for :file:`\*.pkg` files beginning where ``*`` matches the *name*
   argument.  This feature is similar to :file:`\*.pth` files (see the :mod:`site`
   module for more information), except that it doesn't special-case lines starting
   with ``import``.  A :file:`\*.pkg` file is trusted at face value: apart from
   checking for duplicates, all entries found in a :file:`\*.pkg` file are added to
   the path, regardless of whether they exist on the filesystem.  (This is a
   feature.)

   If the input path is not a list (as is the case for frozen packages) it is
   returned unchanged.  The input path is not modified; an extended copy is
   returned.  Items are only appended to the copy at the end.

   It is assumed that ``sys.path`` is a sequence.  Items of ``sys.path`` that are
   not strings referring to existing directories are ignored. Unicode items on
   ``sys.path`` that cause errors when used as filenames may cause this function
   to raise an exception (in line with :func:`os.path.isdir` behavior).

