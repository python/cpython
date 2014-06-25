:mod:`dumbdbm` --- Portable DBM implementation
==============================================

.. module:: dumbdbm
   :synopsis: Portable implementation of the simple DBM interface.

.. note::
   The :mod:`dumbdbm` module has been renamed to :mod:`dbm.dumb` in Python 3.
   The :term:`2to3` tool will automatically adapt imports when converting your
   sources to Python 3.

.. index:: single: databases

.. note::

   The :mod:`dumbdbm` module is intended as a last resort fallback for the
   :mod:`anydbm` module when no more robust module is available. The :mod:`dumbdbm`
   module is not written for speed and is not nearly as heavily used as the other
   database modules.

The :mod:`dumbdbm` module provides a persistent dictionary-like interface which
is written entirely in Python.  Unlike other modules such as :mod:`gdbm` and
:mod:`bsddb`, no external library is required.  As with other persistent
mappings, the keys and values must always be strings.

The module defines the following:


.. exception:: error

   Raised on dumbdbm-specific errors, such as I/O errors.  :exc:`KeyError` is
   raised for general mapping errors like specifying an incorrect key.


.. function:: open(filename[, flag[, mode]])

   Open a dumbdbm database and return a dumbdbm object.  The *filename* argument is
   the basename of the database file (without any specific extensions).  When a
   dumbdbm database is created, files with :file:`.dat` and :file:`.dir` extensions
   are created.

   The optional *flag* argument is currently ignored; the database is always opened
   for update, and will be created if it does not exist.

   The optional *mode* argument is the Unix mode of the file, used only when the
   database has to be created.  It defaults to octal ``0666`` (and will be modified
   by the prevailing umask).

   .. versionchanged:: 2.2
      The *mode* argument was ignored in earlier versions.

In addition to the dictionary-like methods, ``dumbdm`` objects
provide the following method:


.. function:: close()

   Close the ``dumbdm`` database.


.. seealso::

   Module :mod:`anydbm`
      Generic interface to ``dbm``\ -style databases.

   Module :mod:`dbm`
      Similar interface to the DBM/NDBM library.

   Module :mod:`gdbm`
      Similar interface to the GNU GDBM library.

   Module :mod:`shelve`
      Persistence module which stores non-string data.

   Module :mod:`whichdb`
      Utility module used to determine the type of an existing database.


.. _dumbdbm-objects:

Dumbdbm Objects
---------------

In addition to the methods provided by the :class:`UserDict.DictMixin` class,
:class:`dumbdbm` objects provide the following methods.


.. method:: dumbdbm.sync()

   Synchronize the on-disk directory and data files.  This method is called by the
   :meth:`sync` method of :class:`Shelve` objects.

