
.. _persistence:

****************
Data Persistence
****************

The modules described in this chapter support storing Python data in a
persistent form on disk.  The :mod:`pickle` and :mod:`marshal` modules can turn
many Python data types into a stream of bytes and then recreate the objects from
the bytes.  The various DBM-related modules support a family of hash-based file
formats that store a mapping of strings to other strings.  The :mod:`bsddb`
module also provides such disk-based string-to-string mappings based on hashing,
and also supports B-Tree and record-based formats.

The list of modules described in this chapter is:


.. toctree::

   pickle.rst
   copy_reg.rst
   shelve.rst
   marshal.rst
   anydbm.rst
   whichdb.rst
   dbm.rst
   gdbm.rst
   dbhash.rst
   bsddb.rst
   dumbdbm.rst
   sqlite3.rst
