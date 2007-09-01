
:mod:`shelve` --- Python object persistence
===========================================

.. module:: shelve
   :synopsis: Python object persistence.


.. index:: module: pickle

A "shelf" is a persistent, dictionary-like object.  The difference with "dbm"
databases is that the values (not the keys!) in a shelf can be essentially
arbitrary Python objects --- anything that the :mod:`pickle` module can handle.
This includes most class instances, recursive data types, and objects containing
lots of shared  sub-objects.  The keys are ordinary strings.


.. function:: open(filename[, flag='c'[, protocol=None[, writeback=False]]])

   Open a persistent dictionary.  The filename specified is the base filename for
   the underlying database.  As a side-effect, an extension may be added to the
   filename and more than one file may be created.  By default, the underlying
   database file is opened for reading and writing.  The optional *flag* parameter
   has the same interpretation as the *flag* parameter of :func:`anydbm.open`.

   By default, version 0 pickles are used to serialize values.  The version of the
   pickle protocol can be specified with the *protocol* parameter.

   By default, mutations to persistent-dictionary mutable entries are not
   automatically written back.  If the optional *writeback* parameter is set to
   *True*, all entries accessed are cached in memory, and written back at close
   time; this can make it handier to mutate mutable entries in the persistent
   dictionary, but, if many entries are accessed, it can consume vast amounts of
   memory for the cache, and it can make the close operation very slow since all
   accessed entries are written back (there is no way to determine which accessed
   entries are mutable, nor which ones were actually mutated).

Shelve objects support all methods supported by dictionaries.  This eases the
transition from dictionary based scripts to those requiring persistent storage.

One additional method is supported:


.. method:: Shelf.sync()

   Write back all entries in the cache if the shelf was opened with *writeback* set
   to *True*. Also empty the cache and synchronize the persistent dictionary on
   disk, if feasible.  This is called automatically when the shelf is closed with
   :meth:`close`.


Restrictions
------------

  .. index::
     module: dbm
     module: gdbm
     module: bsddb

* The choice of which database package will be used (such as :mod:`dbm`,
  :mod:`gdbm` or :mod:`bsddb`) depends on which interface is available.  Therefore
  it is not safe to open the database directly using :mod:`dbm`.  The database is
  also (unfortunately) subject to the limitations of :mod:`dbm`, if it is used ---
  this means that (the pickled representation of) the objects stored in the
  database should be fairly small, and in rare cases key collisions may cause the
  database to refuse updates.

* Depending on the implementation, closing a persistent dictionary may or may
  not be necessary to flush changes to disk.  The :meth:`__del__` method of the
  :class:`Shelf` class calls the :meth:`close` method, so the programmer generally
  need not do this explicitly.

* The :mod:`shelve` module does not support *concurrent* read/write access to
  shelved objects.  (Multiple simultaneous read accesses are safe.)  When a
  program has a shelf open for writing, no other program should have it open for
  reading or writing.  Unix file locking can be used to solve this, but this
  differs across Unix versions and requires knowledge about the database
  implementation used.


.. class:: Shelf(dict[, protocol=None[, writeback=False]])

   A subclass of :class:`UserDict.DictMixin` which stores pickled values in the
   *dict* object.

   By default, version 0 pickles are used to serialize values.  The version of the
   pickle protocol can be specified with the *protocol* parameter. See the
   :mod:`pickle` documentation for a discussion of the pickle protocols.

   If the *writeback* parameter is ``True``, the object will hold a cache of all
   entries accessed and write them back to the *dict* at sync and close times.
   This allows natural operations on mutable entries, but can consume much more
   memory and make sync and close take a long time.


.. class:: BsdDbShelf(dict[, protocol=None[, writeback=False]])

   A subclass of :class:`Shelf` which exposes :meth:`first`, :meth:`next`,
   :meth:`previous`, :meth:`last` and :meth:`set_location` which are available in
   the :mod:`bsddb` module but not in other database modules.  The *dict* object
   passed to the constructor must support those methods.  This is generally
   accomplished by calling one of :func:`bsddb.hashopen`, :func:`bsddb.btopen` or
   :func:`bsddb.rnopen`.  The optional *protocol* and *writeback* parameters have
   the same interpretation as for the :class:`Shelf` class.


.. class:: DbfilenameShelf(filename[, flag='c'[, protocol=None[, writeback=False]]])

   A subclass of :class:`Shelf` which accepts a *filename* instead of a dict-like
   object.  The underlying file will be opened using :func:`anydbm.open`.  By
   default, the file will be created and opened for both read and write.  The
   optional *flag* parameter has the same interpretation as for the :func:`open`
   function.  The optional *protocol* and *writeback* parameters have the same
   interpretation as for the :class:`Shelf` class.


Example
-------

To summarize the interface (``key`` is a string, ``data`` is an arbitrary
object)::

   import shelve

   d = shelve.open(filename) # open -- file may get suffix added by low-level
                             # library

   d[key] = data   # store data at key (overwrites old data if
                   # using an existing key)
   data = d[key]   # retrieve a COPY of data at key (raise KeyError if no
                   # such key)
   del d[key]      # delete data stored at key (raises KeyError
                   # if no such key)
   flag = key in d   # true if the key exists
   klist = d.keys() # a list of all existing keys (slow!)

   # as d was opened WITHOUT writeback=True, beware:
   d['xx'] = range(4)  # this works as expected, but...
   d['xx'].append(5)   # *this doesn't!* -- d['xx'] is STILL range(4)!!!

   # having opened d without writeback=True, you need to code carefully:
   temp = d['xx']      # extracts the copy
   temp.append(5)      # mutates the copy
   d['xx'] = temp      # stores the copy right back, to persist it

   # or, d=shelve.open(filename,writeback=True) would let you just code
   # d['xx'].append(5) and have it work as expected, BUT it would also
   # consume more memory and make the d.close() operation slower.

   d.close()       # close it


.. seealso::

   Module :mod:`anydbm`
      Generic interface to ``dbm``\ -style databases.

   Module :mod:`bsddb`
      BSD ``db`` database interface.

   Module :mod:`dbhash`
      Thin layer around the :mod:`bsddb` which provides an :func:`open` function like
      the other database modules.

   Module :mod:`dbm`
      Standard Unix database interface.

   Module :mod:`dumbdbm`
      Portable implementation of the ``dbm`` interface.

   Module :mod:`gdbm`
      GNU database interface, based on the ``dbm`` interface.

   Module :mod:`pickle`
      Object serialization used by :mod:`shelve`.

   Module :mod:`cPickle`
      High-performance version of :mod:`pickle`.

