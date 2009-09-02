.. _persistence:

****************
Data Persistence
****************

The modules described in this chapter support storing Python data in a
persistent form on disk.  The :mod:`pickle` and :mod:`marshal` modules can turn
many Python data types into a stream of bytes and then recreate the objects from
the bytes.  The various DBM-related modules support a family of hash-based file
formats that store a mapping of strings to other strings.

The list of modules described in this chapter is:


.. toctree::

   pickle.rst
   copyreg.rst
   shelve.rst
   marshal.rst
   dbm.rst
   sqlite3.rst
