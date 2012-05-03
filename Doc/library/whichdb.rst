:mod:`whichdb` --- Guess which DBM module created a database
============================================================

.. module:: whichdb
   :synopsis: Guess which DBM-style module created a given database.

.. note::
   The :mod:`whichdb` module's only function has been put into the :mod:`dbm`
   module in Python 3.  The :term:`2to3` tool will automatically adapt imports
   when converting your sources to Python 3.


The single function in this module attempts to guess which of the several simple
database modules available--\ :mod:`dbm`, :mod:`gdbm`, or :mod:`dbhash`\
--should be used to open a given file.


.. function:: whichdb(filename)

   Returns one of the following values: ``None`` if the file can't be opened
   because it's unreadable or doesn't exist; the empty string (``''``) if the
   file's format can't be guessed; or a string containing the required module name,
   such as ``'dbm'`` or ``'gdbm'``.

