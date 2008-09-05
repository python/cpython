:mod:`dbhash` --- DBM-style interface to the BSD database library
=================================================================

.. module:: dbhash
   :synopsis: DBM-style interface to the BSD database library.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

.. deprecated:: 2.6
    The :mod:`dbhash` module has been deprecated for removal in Python 3.0.

.. index:: module: bsddb

The :mod:`dbhash` module provides a function to open databases using the BSD
``db`` library.  This module mirrors the interface of the other Python database
modules that provide access to DBM-style databases.  The :mod:`bsddb` module is
required  to use :mod:`dbhash`.

This module provides an exception and a function:


.. exception:: error

   Exception raised on database errors other than :exc:`KeyError`.  It is a synonym
   for :exc:`bsddb.error`.


.. function:: open(path[, flag[, mode]])

   Open a ``db`` database and return the database object.  The *path* argument is
   the name of the database file.

   The *flag* argument can be:

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

   For platforms on which the BSD ``db`` library supports locking, an ``'l'``
   can be appended to indicate that locking should be used.

   The optional *mode* parameter is used to indicate the Unix permission bits that
   should be set if a new database must be created; this will be masked by the
   current umask value for the process.


.. seealso::

   Module :mod:`anydbm`
      Generic interface to ``dbm``\ -style databases.

   Module :mod:`bsddb`
      Lower-level interface to the BSD ``db`` library.

   Module :mod:`whichdb`
      Utility module used to determine the type of an existing database.


.. _dbhash-objects:

Database Objects
----------------

The database objects returned by :func:`open` provide the methods  common to all
the DBM-style databases and mapping objects.  The following methods are
available in addition to the standard methods.


.. method:: dbhash.first()

   It's possible to loop over every key/value pair in the database using this
   method   and the :meth:`next` method.  The traversal is ordered by the databases
   internal hash values, and won't be sorted by the key values.  This method
   returns the starting key.


.. method:: dbhash.last()

   Return the last key/value pair in a database traversal.  This may be used to
   begin a reverse-order traversal; see :meth:`previous`.


.. method:: dbhash.next()

   Returns the key next key/value pair in a database traversal.  The following code
   prints every key in the database ``db``, without having to create a list in
   memory that contains them all::

      print db.first()
      for i in xrange(1, len(db)):
          print db.next()


.. method:: dbhash.previous()

   Returns the previous key/value pair in a forward-traversal of the database. In
   conjunction with :meth:`last`, this may be used to implement a reverse-order
   traversal.


.. method:: dbhash.sync()

   This method forces any unwritten data to be written to the disk.

