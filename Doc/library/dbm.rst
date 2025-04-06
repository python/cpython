:mod:`!dbm` --- Interfaces to Unix "databases"
==============================================

.. module:: dbm
   :synopsis: Interfaces to various Unix "database" formats.

**Source code:** :source:`Lib/dbm/__init__.py`

--------------

:mod:`dbm` is a generic interface to variants of the DBM database:

* :mod:`dbm.sqlite3`
* :mod:`dbm.gnu`
* :mod:`dbm.ndbm`

If none of these modules are installed, the
slow-but-simple implementation in module :mod:`dbm.dumb` will be used.  There
is a `third party interface <https://www.jcea.es/programacion/pybsddb.htm>`_ to
the Oracle Berkeley DB.

.. exception:: error

   A tuple containing the exceptions that can be raised by each of the supported
   modules, with a unique exception also named :exc:`dbm.error` as the first
   item --- the latter is used when :exc:`dbm.error` is raised.


.. function:: whichdb(filename)

   This function attempts to guess which of the several simple database modules
   available --- :mod:`dbm.sqlite3`, :mod:`dbm.gnu`, :mod:`dbm.ndbm`,
   or :mod:`dbm.dumb` --- should be used to open a given file.

   Return one of the following values:

   * ``None`` if the file can't be opened because it's unreadable or doesn't exist
   * the empty string (``''``) if the file's format can't be guessed
   * a string containing the required module name, such as ``'dbm.ndbm'`` or ``'dbm.gnu'``

   .. versionchanged:: 3.11
      *filename* accepts a :term:`path-like object`.

.. Substitutions for the open() flag param docs;
   all submodules use the same text.

.. |flag_r| replace::
   Open existing database for reading only.

.. |flag_w| replace::
   Open existing database for reading and writing.

.. |flag_c| replace::
   Open database for reading and writing, creating it if it doesn't exist.

.. |flag_n| replace::
   Always create a new, empty database, open for reading and writing.

.. |mode_param_doc| replace::
   The Unix file access mode of the file (default: octal ``0o666``),
   used only when the database has to be created.

.. function:: open(file, flag='r', mode=0o666)

   Open a database and return the corresponding database object.

   :param file:
      The database file to open.

      If the database file already exists, the :func:`whichdb` function is used to
      determine its type and the appropriate module is used; if it does not exist,
      the first submodule listed above that can be imported is used.
   :type file: :term:`path-like object`

   :param str flag:
      * ``'r'`` (default): |flag_r|
      * ``'w'``: |flag_w|
      * ``'c'``: |flag_c|
      * ``'n'``: |flag_n|

   :param int mode:
      |mode_param_doc|

   .. versionchanged:: 3.11
      *file* accepts a :term:`path-like object`.

The object returned by :func:`~dbm.open` supports the same basic functionality as a
:class:`dict`; keys and their corresponding values can be stored, retrieved, and
deleted, and the :keyword:`in` operator and the :meth:`!keys` method are
available, as well as :meth:`!get` and :meth:`!setdefault` methods.

Key and values are always stored as :class:`bytes`. This means that when
strings are used they are implicitly converted to the default encoding before
being stored.

These objects also support being used in a :keyword:`with` statement, which
will automatically close them when done.

.. versionchanged:: 3.2
   :meth:`!get` and :meth:`!setdefault` methods are now available for all
   :mod:`dbm` backends.

.. versionchanged:: 3.4
   Added native support for the context management protocol to the objects
   returned by :func:`~dbm.open`.

.. versionchanged:: 3.8
   Deleting a key from a read-only database raises a database module specific exception
   instead of :exc:`KeyError`.

The following example records some hostnames and a corresponding title,  and
then prints out the contents of the database::

   import dbm

   # Open database, creating it if necessary.
   with dbm.open('cache', 'c') as db:

       # Record some values
       db[b'hello'] = b'there'
       db['www.python.org'] = 'Python Website'
       db['www.cnn.com'] = 'Cable News Network'

       # Note that the keys are considered bytes now.
       assert db[b'www.python.org'] == b'Python Website'
       # Notice how the value is now in bytes.
       assert db['www.cnn.com'] == b'Cable News Network'

       # Often-used methods of the dict interface work too.
       print(db.get('python.org', b'not present'))

       # Storing a non-string key or value will raise an exception (most
       # likely a TypeError).
       db['www.yahoo.com'] = 4

   # db is automatically closed when leaving the with statement.


.. seealso::

   Module :mod:`shelve`
      Persistence module which stores non-string data.


The individual submodules are described in the following sections.

:mod:`dbm.sqlite3` --- SQLite backend for dbm
---------------------------------------------

.. module:: dbm.sqlite3
   :platform: All
   :synopsis: SQLite backend for dbm

.. versionadded:: 3.13

**Source code:** :source:`Lib/dbm/sqlite3.py`

--------------

This module uses the standard library :mod:`sqlite3` module to provide an
SQLite backend for the :mod:`dbm` module.
The files created by :mod:`dbm.sqlite3` can thus be opened by :mod:`sqlite3`,
or any other SQLite browser, including the SQLite CLI.

.. include:: ../includes/wasm-notavail.rst

.. function:: open(filename, /, flag="r", mode=0o666)

   Open an SQLite database.
   The returned object behaves like a :term:`mapping`,
   implements a :meth:`!close` method,
   and supports a "closing" context manager via the :keyword:`with` keyword.

   :param filename:
      The path to the database to be opened.
   :type filename: :term:`path-like object`

   :param str flag:

      * ``'r'`` (default): |flag_r|
      * ``'w'``: |flag_w|
      * ``'c'``: |flag_c|
      * ``'n'``: |flag_n|

   :param mode:
      The Unix file access mode of the file (default: octal ``0o666``),
      used only when the database has to be created.


:mod:`dbm.gnu` --- GNU database manager
---------------------------------------

.. module:: dbm.gnu
   :platform: Unix
   :synopsis: GNU database manager

**Source code:** :source:`Lib/dbm/gnu.py`

--------------

The :mod:`dbm.gnu` module provides an interface to the :abbr:`GDBM (GNU dbm)`
library, similar to the :mod:`dbm.ndbm` module, but with additional
functionality like crash tolerance.

.. note::

   The file formats created by :mod:`dbm.gnu` and :mod:`dbm.ndbm` are incompatible
   and can not be used interchangeably.

.. include:: ../includes/wasm-mobile-notavail.rst

.. exception:: error

   Raised on :mod:`dbm.gnu`-specific errors, such as I/O errors. :exc:`KeyError` is
   raised for general mapping errors like specifying an incorrect key.


.. function:: open(filename, flag="r", mode=0o666, /)

   Open a GDBM database and return a :class:`!gdbm` object.

   :param filename:
      The database file to open.
   :type filename: :term:`path-like object`

   :param str flag:
      * ``'r'`` (default): |flag_r|
      * ``'w'``: |flag_w|
      * ``'c'``: |flag_c|
      * ``'n'``: |flag_n|

      The following additional characters may be appended
      to control how the database is opened:

      * ``'f'``: Open the database in fast mode.
        Writes to the database will not be synchronized.
      * ``'s'``: Synchronized mode.
        Changes to the database will be written immediately to the file.
      * ``'u'``: Do not lock database.

      Not all flags are valid for all versions of GDBM.
      See the :data:`open_flags` member for a list of supported flag characters.

   :param int mode:
      |mode_param_doc|

   :raises error:
      If an invalid *flag* argument is passed.

   .. versionchanged:: 3.11
      *filename* accepts a :term:`path-like object`.

   .. data:: open_flags

      A string of characters the *flag* parameter of :meth:`~dbm.gnu.open` supports.

   :class:`!gdbm` objects behave similar to :term:`mappings <mapping>`,
   but :meth:`!items` and :meth:`!values` methods are not supported.
   The following methods are also provided:

   .. method:: gdbm.firstkey()

      It's possible to loop over every key in the database using this method  and the
      :meth:`nextkey` method.  The traversal is ordered by GDBM's internal
      hash values, and won't be sorted by the key values.  This method returns
      the starting key.

   .. method:: gdbm.nextkey(key)

      Returns the key that follows *key* in the traversal.  The following code prints
      every key in the database ``db``, without having to create a list in memory that
      contains them all::

         k = db.firstkey()
         while k is not None:
             print(k)
             k = db.nextkey(k)

   .. method:: gdbm.reorganize()

      If you have carried out a lot of deletions and would like to shrink the space
      used by the GDBM file, this routine will reorganize the database.  :class:`!gdbm`
      objects will not shorten the length of a database file except by using this
      reorganization; otherwise, deleted file space will be kept and reused as new
      (key, value) pairs are added.

   .. method:: gdbm.sync()

      When the database has been opened in fast mode, this method forces any
      unwritten data to be written to the disk.

   .. method:: gdbm.close()

      Close the GDBM database.

   .. method:: gdbm.clear()

      Remove all items from the GDBM database.

      .. versionadded:: 3.13


:mod:`dbm.ndbm` --- New Database Manager
----------------------------------------

.. module:: dbm.ndbm
   :platform: Unix
   :synopsis: The New Database Manager

**Source code:** :source:`Lib/dbm/ndbm.py`

--------------

The :mod:`dbm.ndbm` module provides an interface to the
:abbr:`NDBM (New Database Manager)` library.
This module can be used with the "classic" NDBM interface or the
:abbr:`GDBM (GNU dbm)` compatibility interface.

.. note::

   The file formats created by :mod:`dbm.gnu` and :mod:`dbm.ndbm` are incompatible
   and can not be used interchangeably.

.. warning::

   The NDBM library shipped as part of macOS has an undocumented limitation on the
   size of values, which can result in corrupted database files
   when storing values larger than this limit. Reading such corrupted files can
   result in a hard crash (segmentation fault).

.. include:: ../includes/wasm-mobile-notavail.rst

.. exception:: error

   Raised on :mod:`dbm.ndbm`-specific errors, such as I/O errors. :exc:`KeyError` is raised
   for general mapping errors like specifying an incorrect key.


.. data:: library

   Name of the NDBM implementation library used.


.. function:: open(filename, flag="r", mode=0o666, /)

   Open an NDBM database and return an :class:`!ndbm` object.

   :param filename:
      The basename of the database file
      (without the :file:`.dir` or :file:`.pag` extensions).
   :type filename: :term:`path-like object`

   :param str flag:
      * ``'r'`` (default): |flag_r|
      * ``'w'``: |flag_w|
      * ``'c'``: |flag_c|
      * ``'n'``: |flag_n|

   :param int mode:
      |mode_param_doc|

   :class:`!ndbm` objects behave similar to :term:`mappings <mapping>`,
   but :meth:`!items` and :meth:`!values` methods are not supported.
   The following methods are also provided:

   .. versionchanged:: 3.11
      Accepts :term:`path-like object` for filename.

   .. method:: ndbm.close()

      Close the NDBM database.

   .. method:: ndbm.clear()

      Remove all items from the NDBM database.

      .. versionadded:: 3.13


:mod:`dbm.dumb` --- Portable DBM implementation
-----------------------------------------------

.. module:: dbm.dumb
   :synopsis: Portable implementation of the simple DBM interface.

**Source code:** :source:`Lib/dbm/dumb.py`

.. index:: single: databases

.. note::

   The :mod:`dbm.dumb` module is intended as a last resort fallback for the
   :mod:`dbm` module when a more robust module is not available. The :mod:`dbm.dumb`
   module is not written for speed and is not nearly as heavily used as the other
   database modules.

--------------

The :mod:`dbm.dumb` module provides a persistent :class:`dict`-like
interface which is written entirely in Python.
Unlike other :mod:`dbm` backends, such as :mod:`dbm.gnu`, no
external library is required.

The :mod:`!dbm.dumb` module defines the following:

.. exception:: error

   Raised on :mod:`dbm.dumb`-specific errors, such as I/O errors.  :exc:`KeyError` is
   raised for general mapping errors like specifying an incorrect key.


.. function:: open(filename, flag="c", mode=0o666)

   Open a :mod:`!dbm.dumb` database.
   The returned database object behaves similar to a :term:`mapping`,
   in addition to providing :meth:`~dumbdbm.sync` and :meth:`~dumbdbm.close`
   methods.

   :param filename:
      The basename of the database file (without extensions).
      A new database creates the following files:

      - :file:`{filename}.dat`
      - :file:`{filename}.dir`
   :type database: :term:`path-like object`

   :param str flag:
      * ``'r'``: |flag_r|
      * ``'w'``: |flag_w|
      * ``'c'`` (default): |flag_c|
      * ``'n'``: |flag_n|

   :param int mode:
      |mode_param_doc|

   .. warning::
      It is possible to crash the Python interpreter when loading a database
      with a sufficiently large/complex entry due to stack depth limitations in
      Python's AST compiler.

   .. versionchanged:: 3.5
      :func:`~dbm.dumb.open` always creates a new database when *flag* is ``'n'``.

   .. versionchanged:: 3.8
      A database opened read-only if *flag* is ``'r'``.
      A database is not created if it does not exist if *flag* is ``'r'`` or ``'w'``.

   .. versionchanged:: 3.11
      *filename* accepts a :term:`path-like object`.

   In addition to the methods provided by the
   :class:`collections.abc.MutableMapping` class,
   the following methods are provided:

   .. method:: dumbdbm.sync()

      Synchronize the on-disk directory and data files.  This method is called
      by the :meth:`shelve.Shelf.sync` method.

   .. method:: dumbdbm.close()

      Close the database.
