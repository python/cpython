:mod:`anydbm` --- Generic access to DBM-style databases
=======================================================

.. module:: anydbm
   :synopsis: Generic interface to DBM-style database modules.


.. note::
   The :mod:`anydbm` module has been renamed to :mod:`dbm` in Python 3.  The
   :term:`2to3` tool will automatically adapt imports when converting your
   sources to Python 3.

.. index::
   module: dbhash
   module: bsddb
   module: gdbm
   module: dbm
   module: dumbdbm

:mod:`anydbm` is a generic interface to variants of the DBM database ---
:mod:`dbhash` (requires :mod:`bsddb`), :mod:`gdbm`, or :mod:`dbm`.  If none of
these modules is installed, the slow-but-simple implementation in module
:mod:`dumbdbm` will be used.


.. function:: open(filename[, flag[, mode]])

   Open the database file *filename* and return a corresponding object.

   If the database file already exists, the :mod:`whichdb` module is used to
   determine its type and the appropriate module is used; if it does not exist,
   the first module listed above that can be imported is used.

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

   If not specified, the default value is ``'r'``.

   The optional *mode* argument is the Unix mode of the file, used only when the
   database has to be created.  It defaults to octal ``0666`` (and will be
   modified by the prevailing umask).


.. exception:: error

   A tuple containing the exceptions that can be raised by each of the supported
   modules, with a unique exception also named :exc:`anydbm.error` as the first
   item --- the latter is used when :exc:`anydbm.error` is raised.

The object returned by :func:`.open` supports most of the same functionality as
dictionaries; keys and their corresponding values can be stored, retrieved, and
deleted, and the :meth:`has_key` and :meth:`keys` methods are available.  Keys
and values must always be strings.

The following example records some hostnames and a corresponding title,  and
then prints out the contents of the database::

   import anydbm

   # Open database, creating it if necessary.
   db = anydbm.open('cache', 'c')

   # Record some values
   db['www.python.org'] = 'Python Website'
   db['www.cnn.com'] = 'Cable News Network'

   # Loop through contents.  Other dictionary methods
   # such as .keys(), .values() also work.
   for k, v in db.iteritems():
       print k, '\t', v

   # Storing a non-string key or value will raise an exception (most
   # likely a TypeError).
   db['www.yahoo.com'] = 4

   # Close when done.
   db.close()


.. seealso::

   Module :mod:`dbhash`
      BSD ``db`` database interface.

   Module :mod:`dbm`
      Standard Unix database interface.

   Module :mod:`dumbdbm`
      Portable implementation of the ``dbm`` interface.

   Module :mod:`gdbm`
      GNU database interface, based on the ``dbm`` interface.

   Module :mod:`shelve`
      General object persistence built on top of  the Python ``dbm`` interface.

   Module :mod:`whichdb`
      Utility module used to determine the type of an existing database.

