.. highlightlang:: rest

The Sphinx build system
=======================

.. XXX: intro...

.. _doc-build-config:

The build configuration file
----------------------------

The documentation root, that is the ``Doc`` subdirectory of the source
distribution, contains a file named ``conf.py``.  This file is called the "build
configuration file", and it contains several variables that are read and used
during a build run.

These variables are:

version : string
   A string that is used as a replacement for the ``|version|`` reST
   substitution.  It should be the Python version the documentation refers to.
   This consists only of the major and minor version parts, e.g. ``2.5``, even
   for version 2.5.1.

release : string
   A string that is used as a replacement for the ``|release|`` reST
   substitution.  It should be the full version string including
   alpha/beta/release candidate tags, e.g. ``2.5.2b3``.

Both ``release`` and ``version`` can be ``'auto'``, which means that they are
determined at runtime from the ``Include/patchlevel.h`` file, if a complete
Python source distribution can be found, or else from the interpreter running
Sphinx.

today_fmt : string
   A ``strftime`` format that is used to format a replacement for the
   ``|today|`` reST substitution.

today : string
   A string that can contain a date that should be written to the documentation
   output literally.  If this is nonzero, it is used instead of
   ``strftime(today_fmt)``.

unused_files : list of strings
   A list of reST filenames that are to be disregarded during building.  This
   could be docs for temporarily disabled modules or documentation that's not
   yet ready for public consumption.

last_updated_format : string
   If this is not an empty string, it will be given to ``time.strftime()`` and
   written to each generated output file after "last updated on:".

use_smartypants : bool
   If true, use SmartyPants to convert quotes and dashes to the typographically
   correct entities.

add_function_parentheses : bool
   If true, ``()`` will be appended to the content of ``:func:``, ``:meth:`` and
   ``:cfunc:`` cross-references.