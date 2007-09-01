
:mod:`bsddb` --- Interface to Berkeley DB library
=================================================

.. module:: bsddb
   :synopsis: Interface to Berkeley DB database library
.. sectionauthor:: Skip Montanaro <skip@mojam.com>


The :mod:`bsddb` module provides an interface to the Berkeley DB library.  Users
can create hash, btree or record based library files using the appropriate open
call. Bsddb objects behave generally like dictionaries.  Keys and values must be
strings, however, so to use other objects as keys or to store other kinds of
objects the user must serialize them somehow, typically using
:func:`marshal.dumps` or  :func:`pickle.dumps`.

The :mod:`bsddb` module requires a Berkeley DB library version from 3.3 thru
4.5.


.. seealso::

   http://pybsddb.sourceforge.net/
      The website with documentation for the :mod:`bsddb.db` Python Berkeley DB
      interface that closely mirrors the object oriented interface provided in
      Berkeley DB 3 and 4.

   http://www.oracle.com/database/berkeley-db/
      The Berkeley DB library.

A more modern DB, DBEnv and DBSequence object interface is available in the
:mod:`bsddb.db` module which closely matches the Berkeley DB C API documented at
the above URLs.  Additional features provided by the :mod:`bsddb.db` API include
fine tuning, transactions, logging, and multiprocess concurrent database access.

The following is a description of the legacy :mod:`bsddb` interface compatible
with the old Python bsddb module.  Starting in Python 2.5 this interface should
be safe for multithreaded access.  The :mod:`bsddb.db` API is recommended for
threading users as it provides better control.

The :mod:`bsddb` module defines the following functions that create objects that
access the appropriate type of Berkeley DB file.  The first two arguments of
each function are the same.  For ease of portability, only the first two
arguments should be used in most instances.


.. function:: hashopen(filename[, flag[, mode[, pgsize[, ffactor[, nelem[, cachesize[, lorder[, hflags]]]]]]]])

   Open the hash format file named *filename*.  Files never intended to be
   preserved on disk may be created by passing ``None`` as the  *filename*.  The
   optional *flag* identifies the mode used to open the file.  It may be ``'r'``
   (read only), ``'w'`` (read-write) , ``'c'`` (read-write - create if necessary;
   the default) or ``'n'`` (read-write - truncate to zero length).  The other
   arguments are rarely used and are just passed to the low-level :cfunc:`dbopen`
   function.  Consult the Berkeley DB documentation for their use and
   interpretation.


.. function:: btopen(filename[, flag[, mode[, btflags[, cachesize[, maxkeypage[, minkeypage[, pgsize[, lorder]]]]]]]])

   Open the btree format file named *filename*.  Files never intended  to be
   preserved on disk may be created by passing ``None`` as the  *filename*.  The
   optional *flag* identifies the mode used to open the file.  It may be ``'r'``
   (read only), ``'w'`` (read-write), ``'c'`` (read-write - create if necessary;
   the default) or ``'n'`` (read-write - truncate to zero length).  The other
   arguments are rarely used and are just passed to the low-level dbopen function.
   Consult the Berkeley DB documentation for their use and interpretation.


.. function:: rnopen(filename[, flag[, mode[, rnflags[, cachesize[, pgsize[, lorder[, rlen[, delim[, source[, pad]]]]]]]]]])

   Open a DB record format file named *filename*.  Files never intended  to be
   preserved on disk may be created by passing ``None`` as the  *filename*.  The
   optional *flag* identifies the mode used to open the file.  It may be ``'r'``
   (read only), ``'w'`` (read-write), ``'c'`` (read-write - create if necessary;
   the default) or ``'n'`` (read-write - truncate to zero length).  The other
   arguments are rarely used and are just passed to the low-level dbopen function.
   Consult the Berkeley DB documentation for their use and interpretation.


.. class:: StringKeys(db)

   Wrapper class around a DB object that supports string keys (rather than bytes).
   All keys are encoded as UTF-8, then passed to the underlying object.


.. class:: StringValues(db)

   Wrapper class around a DB object that supports string values (rather than bytes).
   All values are encoded as UTF-8, then passed to the underlying object.


.. seealso::

   Module :mod:`dbhash`
      DBM-style interface to the :mod:`bsddb`


.. _bsddb-objects:

Hash, BTree and Record Objects
------------------------------

Once instantiated, hash, btree and record objects support the same methods as
dictionaries.  In addition, they support the methods listed below.


.. method:: bsddbobject.close()

   Close the underlying file.  The object can no longer be accessed.  Since there
   is no open :meth:`open` method for these objects, to open the file again a new
   :mod:`bsddb` module open function must be called.


.. method:: bsddbobject.keys()

   Return the list of keys contained in the DB file.  The order of the list is
   unspecified and should not be relied on.  In particular, the order of the list
   returned is different for different file formats.


.. method:: bsddbobject.has_key(key)

   Return ``1`` if the DB file contains the argument as a key.


.. method:: bsddbobject.set_location(key)

   Set the cursor to the item indicated by *key* and return a tuple containing the
   key and its value.  For binary tree databases (opened using :func:`btopen`), if
   *key* does not actually exist in the database, the cursor will point to the next
   item in sorted order and return that key and value.  For other databases,
   :exc:`KeyError` will be raised if *key* is not found in the database.


.. method:: bsddbobject.first()

   Set the cursor to the first item in the DB file and return it.  The order of
   keys in the file is unspecified, except in the case of B-Tree databases. This
   method raises :exc:`bsddb.error` if the database is empty.


.. method:: bsddbobject.next()

   Set the cursor to the next item in the DB file and return it.  The order of
   keys in the file is unspecified, except in the case of B-Tree databases.


.. method:: bsddbobject.previous()

   Set the cursor to the previous item in the DB file and return it.  The order of
   keys in the file is unspecified, except in the case of B-Tree databases.  This
   is not supported on hashtable databases (those opened with :func:`hashopen`).


.. method:: bsddbobject.last()

   Set the cursor to the last item in the DB file and return it.  The order of keys
   in the file is unspecified.  This is not supported on hashtable databases (those
   opened with :func:`hashopen`). This method raises :exc:`bsddb.error` if the
   database is empty.


.. method:: bsddbobject.sync()

   Synchronize the database on disk.

Example::

   >>> import bsddb
   >>> db = bsddb.btopen('/tmp/spam.db', 'c')
   >>> for i in range(10): db['%d'%i] = '%d'% (i*i)
   ... 
   >>> db['3']
   '9'
   >>> db.keys()
   ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
   >>> db.first()
   ('0', '0')
   >>> db.next()
   ('1', '1')
   >>> db.last()
   ('9', '81')
   >>> db.set_location('2')
   ('2', '4')
   >>> db.previous() 
   ('1', '1')
   >>> for k, v in db.iteritems():
   ...     print k, v
   0 0
   1 1
   2 4
   3 9
   4 16
   5 25
   6 36
   7 49
   8 64
   9 81
   >>> '8' in db
   True
   >>> db.sync()
   0

