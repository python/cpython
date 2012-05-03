:mod:`dbm` --- Simple "database" interface
==========================================

.. module:: dbm
   :platform: Unix
   :synopsis: The standard "database" interface, based on ndbm.

.. note::
   The :mod:`dbm` module has been renamed to :mod:`dbm.ndbm` in Python 3.  The
   :term:`2to3` tool will automatically adapt imports when converting your
   sources to Python 3.


The :mod:`dbm` module provides an interface to the Unix "(n)dbm" library.  Dbm
objects behave like mappings (dictionaries), except that keys and values are
always strings. Printing a dbm object doesn't print the keys and values, and the
:meth:`items` and :meth:`values` methods are not supported.

This module can be used with the "classic" ndbm interface, the BSD DB
compatibility interface, or the GNU GDBM compatibility interface. On Unix, the
:program:`configure` script will attempt to locate the appropriate header file
to simplify building this module.

The module defines the following:


.. exception:: error

   Raised on dbm-specific errors, such as I/O errors. :exc:`KeyError` is raised for
   general mapping errors like specifying an incorrect key.


.. data:: library

   Name of the ``ndbm`` implementation library used.


.. function:: open(filename[, flag[, mode]])

   Open a dbm database and return a dbm object.  The *filename* argument is the
   name of the database file (without the :file:`.dir` or :file:`.pag` extensions;
   note that the BSD DB implementation of the interface will append the extension
   :file:`.db` and only create one file).

   The optional *flag* argument must be one of these values:

   +---------+-------------------------------------------+
   | Value   | Meaning                                   |
   +=========+===========================================+
   | ``'r'`` | Open existing database for reading only   |
   |         | (default)                                 |
   +---------+-------------------------------------------+
   | ``'w'`` | Open existing database for reading and    |
   |         | writing                                   |
   +---------+-------------------------------------------+
   | ``'c'`` | Open database for reading and writing,    |
   |         | creating it if it doesn't exist           |
   +---------+-------------------------------------------+
   | ``'n'`` | Always create a new, empty database, open |
   |         | for reading and writing                   |
   +---------+-------------------------------------------+

   The optional *mode* argument is the Unix mode of the file, used only when the
   database has to be created.  It defaults to octal ``0666`` (and will be
   modified by the prevailing umask).


.. seealso::

   Module :mod:`anydbm`
      Generic interface to ``dbm``\ -style databases.

   Module :mod:`gdbm`
      Similar interface to the GNU GDBM library.

   Module :mod:`whichdb`
      Utility module used to determine the type of an existing database.

