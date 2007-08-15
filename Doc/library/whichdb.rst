
:mod:`whichdb` --- Guess which DBM module created a database
============================================================

.. module:: whichdb
   :synopsis: Guess which DBM-style module created a given database.


The single function in this module attempts to guess which of the several simple
database modules available--\ :mod:`dbm`, :mod:`gdbm`, or :mod:`dbhash`\
--should be used to open a given file.


.. function:: whichdb(filename)

   Returns one of the following values: ``None`` if the file can't be opened
   because it's unreadable or doesn't exist; the empty string (``''``) if the
   file's format can't be guessed; or a string containing the required module name,
   such as ``'dbm'`` or ``'gdbm'``.

