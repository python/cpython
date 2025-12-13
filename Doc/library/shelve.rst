:mod:`!shelve` --- Python object persistence
============================================

.. module:: shelve
   :synopsis: Python object persistence.

**Source code:** :source:`Lib/shelve.py`

.. index:: pair: module; pickle

--------------

A "shelf" is a persistent, dictionary-like object.  The difference with "dbm"
databases is that the values (not the keys!) in a shelf can be essentially
arbitrary Python objects --- anything that the :mod:`pickle` module can handle.
This includes most class instances, recursive data types, and objects containing
lots of shared  sub-objects.  The keys are ordinary strings.


.. function:: open(filename, flag='c', protocol=None, writeback=False, *, \
                   serializer=None, deserializer=None)

   Open a persistent dictionary.  The filename specified is the base filename for
   the underlying database.  As a side-effect, an extension may be added to the
   filename and more than one file may be created.  By default, the underlying
   database file is opened for reading and writing.  The optional *flag* parameter
   has the same interpretation as the *flag* parameter of :func:`dbm.open`.

   By default, pickles created with :const:`pickle.DEFAULT_PROTOCOL` are used
   to serialize values.  The version of the pickle protocol can be specified
   with the *protocol* parameter.

   Because of Python semantics, a shelf cannot know when a mutable
   persistent-dictionary entry is modified.  By default modified objects are
   written *only* when assigned to the shelf (see :ref:`shelve-example`).  If the
   optional *writeback* parameter is set to ``True``, all entries accessed are also
   cached in memory, and written back on :meth:`~Shelf.sync` and
   :meth:`~Shelf.close`; this can make it handier to mutate mutable entries in
   the persistent dictionary, but, if many entries are accessed, it can consume
   vast amounts of memory for the cache, and it can make the close operation
   very slow since all accessed entries are written back (there is no way to
   determine which accessed entries are mutable, nor which ones were actually
   mutated).

   By default, :mod:`shelve` uses :func:`pickle.dumps` and :func:`pickle.loads`
   for serializing and deserializing. This can be changed by supplying
   *serializer* and *deserializer*, respectively.

   The *serializer* argument must be a callable which takes an object ``obj``
   and the *protocol* as inputs and returns the representation ``obj`` as a
   :term:`bytes-like object`; the *protocol* value may be ignored by the
   serializer.

   The *deserializer* argument must be callable which takes a serialized object
   given as a :class:`bytes` object and returns the corresponding object.

   A :exc:`ShelveError` is raised if *serializer* is given but *deserializer*
   is not, or vice-versa.

   .. versionchanged:: 3.10
      :const:`pickle.DEFAULT_PROTOCOL` is now used as the default pickle
      protocol.

   .. versionchanged:: 3.11
      Accepts :term:`path-like object` for filename.

   .. versionchanged:: 3.15
      Accepts custom *serializer* and *deserializer* functions in place of
      :func:`pickle.dumps` and :func:`pickle.loads`.

   .. note::

      Do not rely on the shelf being closed automatically; always call
      :meth:`~Shelf.close` explicitly when you don't need it any more, or
      use :func:`shelve.open` as a context manager::

          with shelve.open('spam') as db:
              db['eggs'] = 'eggs'

.. _shelve-security:

.. warning::

   Because the :mod:`shelve` module is backed by :mod:`pickle`, it is insecure
   to load a shelf from an untrusted source.  Like with pickle, loading a shelf
   can execute arbitrary code.

Shelf objects support most of methods and operations supported by dictionaries
(except copying, constructors and operators ``|`` and ``|=``).  This eases the
transition from dictionary based scripts to those requiring persistent storage.

Two additional methods are supported:

.. method:: Shelf.sync()

   Write back all entries in the cache if the shelf was opened with *writeback*
   set to :const:`True`.  Also empty the cache and synchronize the persistent
   dictionary on disk, if feasible.  This is called automatically when
   :meth:`reorganize` is called or the shelf is closed with :meth:`close`.

.. method:: Shelf.reorganize()

   Calls :meth:`sync` and attempts to shrink space used on disk by removing empty
   space resulting from deletions.

   .. versionadded:: 3.15

.. method:: Shelf.close()

   Synchronize and close the persistent *dict* object.  Operations on a closed
   shelf will fail with a :exc:`ValueError`.


.. seealso::

   `Persistent dictionary recipe <https://code.activestate.com/recipes/576642-persistent-dict-with-multiple-standard-file-format/>`_
   with widely supported storage formats and having the speed of native
   dictionaries.


Restrictions
------------

.. index::
   pair: module; dbm.ndbm
   pair: module; dbm.gnu

* The choice of which database package will be used (such as :mod:`dbm.ndbm` or
  :mod:`dbm.gnu`) depends on which interface is available.  Therefore it is not
  safe to open the database directly using :mod:`dbm`.  The database is also
  (unfortunately) subject to the limitations of :mod:`dbm`, if it is used ---
  this means that (the pickled representation of) the objects stored in the
  database should be fairly small, and in rare cases key collisions may cause
  the database to refuse updates.

* The :mod:`shelve` module does not support *concurrent* read/write access to
  shelved objects.  (Multiple simultaneous read accesses are safe.)  When a
  program has a shelf open for writing, no other program should have it open for
  reading or writing.  Unix file locking can be used to solve this, but this
  differs across Unix versions and requires knowledge about the database
  implementation used.

* On macOS :mod:`dbm.ndbm` can silently corrupt the database file on updates,
  which can cause hard crashes when trying to read from the database.

* :meth:`Shelf.reorganize` may not be available for all database packages and
  may temporarily increase resource usage (especially disk space) when called.
  Additionally, it will never run automatically and instead needs to be called
  explicitly.


.. class:: Shelf(dict, protocol=None, writeback=False, \
                 keyencoding='utf-8', *, serializer=None, deserializer=None)

   A subclass of :class:`collections.abc.MutableMapping` which stores pickled
   values in the *dict* object.

   By default, pickles created with :const:`pickle.DEFAULT_PROTOCOL` are used
   to serialize values.  The version of the pickle protocol can be specified
   with the *protocol* parameter.  See the :mod:`pickle` documentation for a
   discussion of the pickle protocols.

   If the *writeback* parameter is ``True``, the object will hold a cache of all
   entries accessed and write them back to the *dict* at sync and close times.
   This allows natural operations on mutable entries, but can consume much more
   memory and make sync and close take a long time.

   The *keyencoding* parameter is the encoding used to encode keys before they
   are used with the underlying dict.

   The *serializer* and *deserializer* parameters have the same interpretation
   as in :func:`~shelve.open`.

   A :class:`Shelf` object can also be used as a context manager, in which
   case it will be automatically closed when the :keyword:`with` block ends.

   .. versionchanged:: 3.2
      Added the *keyencoding* parameter; previously, keys were always encoded in
      UTF-8.

   .. versionchanged:: 3.4
      Added context manager support.

   .. versionchanged:: 3.10
      :const:`pickle.DEFAULT_PROTOCOL` is now used as the default pickle
      protocol.

   .. versionchanged:: 3.15
      Added the *serializer* and *deserializer* parameters.


.. class:: BsdDbShelf(dict, protocol=None, writeback=False, \
                      keyencoding='utf-8', *, \
                      serializer=None, deserializer=None)

   A subclass of :class:`Shelf` which exposes :meth:`!first`, :meth:`!next`,
   :meth:`!previous`, :meth:`!last` and :meth:`!set_location` methods.
   These are available
   in the third-party :mod:`!bsddb` module from `pybsddb
   <https://www.jcea.es/programacion/pybsddb.htm>`_ but not in other database
   modules.  The *dict* object passed to the constructor must support those
   methods.  This is generally accomplished by calling one of
   :func:`!bsddb.hashopen`, :func:`!bsddb.btopen` or :func:`!bsddb.rnopen`.  The
   optional *protocol*, *writeback*, *keyencoding*, *serializer* and *deserializer*
   parameters have the same interpretation as in :func:`~shelve.open`.

   .. versionchanged:: 3.15
      Added the *serializer* and *deserializer* parameters.


.. class:: DbfilenameShelf(filename, flag='c', protocol=None, \
                           writeback=False, *, serializer=None, \
                           deserializer=None)

   A subclass of :class:`Shelf` which accepts a *filename* instead of a dict-like
   object.  The underlying file will be opened using :func:`dbm.open`.  By
   default, the file will be created and opened for both read and write.  The
   optional *flag* parameter has the same interpretation as for the
   :func:`.open` function.  The optional *protocol*, *writeback*, *serializer*
   and *deserializer* parameters have the same interpretation as in
   :func:`~shelve.open`.

   .. versionchanged:: 3.15
      Added the *serializer* and *deserializer* parameters.


.. _shelve-example:

Example
-------

To summarize the interface (``key`` is a string, ``data`` is an arbitrary
object)::

   import shelve

   d = shelve.open(filename)  # open -- file may get suffix added by low-level
                              # library

   d[key] = data              # store data at key (overwrites old data if
                              # using an existing key)
   data = d[key]              # retrieve a COPY of data at key (raise KeyError
                              # if no such key)
   del d[key]                 # delete data stored at key (raises KeyError
                              # if no such key)

   flag = key in d            # true if the key exists
   klist = list(d.keys())     # a list of all existing keys (slow!)

   # as d was opened WITHOUT writeback=True, beware:
   d['xx'] = [0, 1, 2]        # this works as expected, but...
   d['xx'].append(3)          # *this doesn't!* -- d['xx'] is STILL [0, 1, 2]!

   # having opened d without writeback=True, you need to code carefully:
   temp = d['xx']             # extracts the copy
   temp.append(5)             # mutates the copy
   d['xx'] = temp             # stores the copy right back, to persist it

   # or, d=shelve.open(filename,writeback=True) would let you just code
   # d['xx'].append(5) and have it work as expected, BUT it would also
   # consume more memory and make the d.close() operation slower.

   d.close()                  # close it


Exceptions
----------

.. exception:: ShelveError

   Exception raised when one of the arguments *deserializer* and *serializer*
   is missing in the :func:`~shelve.open`, :class:`Shelf`, :class:`BsdDbShelf`
   and :class:`DbfilenameShelf`.

   The *deserializer* and *serializer* arguments must be given together.

   .. versionadded:: 3.15


.. seealso::

   Module :mod:`dbm`
      Generic interface to ``dbm``-style databases.

   Module :mod:`pickle`
      Object serialization used by :mod:`shelve`.

