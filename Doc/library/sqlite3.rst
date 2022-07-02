:mod:`sqlite3` --- DB-API 2.0 interface for SQLite databases
============================================================

.. module:: sqlite3
   :synopsis: A DB-API 2.0 implementation using SQLite 3.x.

.. sectionauthor:: Gerhard Häring <gh@ghaering.de>

**Source code:** :source:`Lib/sqlite3/`

--------------

SQLite is a C library that provides a lightweight disk-based database that
doesn't require a separate server process and allows accessing the database
using a nonstandard variant of the SQL query language. Some applications can use
SQLite for internal data storage.  It's also possible to prototype an
application using SQLite and then port the code to a larger database such as
PostgreSQL or Oracle.

The sqlite3 module was written by Gerhard Häring.  It provides an SQL interface
compliant with the DB-API 2.0 specification described by :pep:`249`, and
requires SQLite 3.7.15 or newer.

To use the module, start by creating a :class:`Connection` object that
represents the database.  Here the data will be stored in the
:file:`example.db` file::

   import sqlite3
   con = sqlite3.connect('example.db')

The special path name ``:memory:`` can be provided to create a temporary
database in RAM.

Once a :class:`Connection` has been established, create a :class:`Cursor` object
and call its :meth:`~Cursor.execute` method to perform SQL commands::

   cur = con.cursor()

   # Create table
   cur.execute('''CREATE TABLE stocks
                  (date text, trans text, symbol text, qty real, price real)''')

   # Insert a row of data
   cur.execute("INSERT INTO stocks VALUES ('2006-01-05','BUY','RHAT',100,35.14)")

   # Save (commit) the changes
   con.commit()

   # We can also close the connection if we are done with it.
   # Just be sure any changes have been committed or they will be lost.
   con.close()

The saved data is persistent: it can be reloaded in a subsequent session even
after restarting the Python interpreter::

   import sqlite3
   con = sqlite3.connect('example.db')
   cur = con.cursor()

To retrieve data after executing a SELECT statement, either treat the cursor as
an :term:`iterator`, call the cursor's :meth:`~Cursor.fetchone` method to
retrieve a single matching row, or call :meth:`~Cursor.fetchall` to get a list
of the matching rows.

This example uses the iterator form::

   >>> for row in cur.execute('SELECT * FROM stocks ORDER BY price'):
           print(row)

   ('2006-01-05', 'BUY', 'RHAT', 100, 35.14)
   ('2006-03-28', 'BUY', 'IBM', 1000, 45.0)
   ('2006-04-06', 'SELL', 'IBM', 500, 53.0)
   ('2006-04-05', 'BUY', 'MSFT', 1000, 72.0)


.. _sqlite3-placeholders:

SQL operations usually need to use values from Python variables. However,
beware of using Python's string operations to assemble queries, as they
are vulnerable to SQL injection attacks (see the `xkcd webcomic
<https://xkcd.com/327/>`_ for a humorous example of what can go wrong)::

   # Never do this -- insecure!
   symbol = 'RHAT'
   cur.execute("SELECT * FROM stocks WHERE symbol = '%s'" % symbol)

Instead, use the DB-API's parameter substitution. To insert a variable into a
query string, use a placeholder in the string, and substitute the actual values
into the query by providing them as a :class:`tuple` of values to the second
argument of the cursor's :meth:`~Cursor.execute` method. An SQL statement may
use one of two kinds of placeholders: question marks (qmark style) or named
placeholders (named style). For the qmark style, ``parameters`` must be a
:term:`sequence <sequence>`. For the named style, it can be either a
:term:`sequence <sequence>` or :class:`dict` instance. The length of the
:term:`sequence <sequence>` must match the number of placeholders, or a
:exc:`ProgrammingError` is raised. If a :class:`dict` is given, it must contain
keys for all named parameters. Any extra items are ignored. Here's an example of
both styles:

.. literalinclude:: ../includes/sqlite3/execute_1.py


.. seealso::

   https://www.sqlite.org
      The SQLite web page; the documentation describes the syntax and the
      available data types for the supported SQL dialect.

   https://www.w3schools.com/sql/
      Tutorial, reference and examples for learning SQL syntax.

   :pep:`249` - Database API Specification 2.0
      PEP written by Marc-André Lemburg.


.. _sqlite3-module-contents:

Module functions and constants
------------------------------


.. data:: apilevel

   String constant stating the supported DB-API level. Required by the DB-API.
   Hard-coded to ``"2.0"``.

.. data:: paramstyle

   String constant stating the type of parameter marker formatting expected by
   the :mod:`sqlite3` module. Required by the DB-API. Hard-coded to
   ``"qmark"``.

   .. note::

      The :mod:`sqlite3` module supports both ``qmark`` and ``numeric`` DB-API
      parameter styles, because that is what the underlying SQLite library
      supports. However, the DB-API does not allow multiple values for
      the ``paramstyle`` attribute.

.. data:: version

   The version number of this module, as a string. This is not the version of
   the SQLite library.

   .. deprecated-removed:: 3.12 3.14
      This constant used to reflect the version number of the ``pysqlite``
      package, a third-party library which used to upstream changes to
      ``sqlite3``. Today, it carries no meaning or practical value.


.. data:: version_info

   The version number of this module, as a tuple of integers. This is not the
   version of the SQLite library.

   .. deprecated-removed:: 3.12 3.14
      This constant used to reflect the version number of the ``pysqlite``
      package, a third-party library which used to upstream changes to
      ``sqlite3``. Today, it carries no meaning or practical value.


.. data:: sqlite_version

   The version number of the run-time SQLite library, as a string.


.. data:: sqlite_version_info

   The version number of the run-time SQLite library, as a tuple of integers.


.. data:: threadsafety

   Integer constant required by the DB-API 2.0, stating the level of thread
   safety the :mod:`sqlite3` module supports. This attribute is set based on
   the default `threading mode <https://sqlite.org/threadsafe.html>`_ the
   underlying SQLite library is compiled with. The SQLite threading modes are:

     1. **Single-thread**: In this mode, all mutexes are disabled and SQLite is
        unsafe to use in more than a single thread at once.
     2. **Multi-thread**: In this mode, SQLite can be safely used by multiple
        threads provided that no single database connection is used
        simultaneously in two or more threads.
     3. **Serialized**: In serialized mode, SQLite can be safely used by
        multiple threads with no restriction.

   The mappings from SQLite threading modes to DB-API 2.0 threadsafety levels
   are as follows:

   +------------------+-----------------+----------------------+-------------------------------+
   | SQLite threading | `threadsafety`_ | `SQLITE_THREADSAFE`_ | DB-API 2.0 meaning            |
   | mode             |                 |                      |                               |
   +==================+=================+======================+===============================+
   | single-thread    | 0               | 0                    | Threads may not share the     |
   |                  |                 |                      | module                        |
   +------------------+-----------------+----------------------+-------------------------------+
   | multi-thread     | 1               | 2                    | Threads may share the module, |
   |                  |                 |                      | but not connections           |
   +------------------+-----------------+----------------------+-------------------------------+
   | serialized       | 3               | 1                    | Threads may share the module, |
   |                  |                 |                      | connections and cursors       |
   +------------------+-----------------+----------------------+-------------------------------+

   .. _threadsafety: https://peps.python.org/pep-0249/#threadsafety
   .. _SQLITE_THREADSAFE: https://sqlite.org/compile.html#threadsafe

   .. versionchanged:: 3.11
      Set *threadsafety* dynamically instead of hard-coding it to ``1``.

.. data:: PARSE_DECLTYPES

   Pass this flag value to the *detect_types* parameter of
   :func:`connect` to look up a converter function using
   the declared types for each column.
   The types are declared when the database table is created.
   ``sqlite3`` will look up a converter function using the first word of the
   declared type as the converter dictionary key.
   For example:


   .. code-block:: sql

      CREATE TABLE test(
         i integer primary key,  ! will look up a converter named "integer"
         p point,                ! will look up a converter named "point"
         n number(10)            ! will look up a converter named "number"
       )

   This flag may be combined with :const:`PARSE_COLNAMES` using the ``|``
   (bitwise or) operator.


.. data:: PARSE_COLNAMES

   Pass this flag value to the *detect_types* parameter of
   :func:`connect` to look up a converter function by
   using the type name, parsed from the query column name,
   as the converter dictionary key.
   The type name must be wrapped in square brackets (``[]``).

   .. code-block:: sql

      SELECT p as "p [point]" FROM test;  ! will look up converter "point"

   This flag may be combined with :const:`PARSE_DECLTYPES` using the ``|``
   (bitwise or) operator.


.. function:: connect(database[, timeout, detect_types, isolation_level, check_same_thread, factory, cached_statements, uri])

   Opens a connection to the SQLite database file *database*. By default returns a
   :class:`Connection` object, unless a custom *factory* is given.

   *database* is a :term:`path-like object` giving the pathname (absolute or
   relative to the current  working directory) of the database file to be opened.
   You can use ``":memory:"`` to open a database connection to a database that
   resides in RAM instead of on disk.

   When a database is accessed by multiple connections, and one of the processes
   modifies the database, the SQLite database is locked until that transaction is
   committed. The *timeout* parameter specifies how long the connection should wait
   for the lock to go away until raising an exception. The default for the timeout
   parameter is 5.0 (five seconds).

   For the *isolation_level* parameter, please see the
   :attr:`~Connection.isolation_level` property of :class:`Connection` objects.

   SQLite natively supports only the types TEXT, INTEGER, REAL, BLOB and NULL. If
   you want to use other types you must add support for them yourself. The
   *detect_types* parameter and using custom **converters** registered with the
   module-level :func:`register_converter` function allow you to easily do that.

   *detect_types* defaults to 0 (type detection disabled).
   Set it to any combination (using ``|``, bitwise or) of
   :const:`PARSE_DECLTYPES` and :const:`PARSE_COLNAMES`
   to enable type detection.
   Column names takes precedence over declared types if both flags are set.
   Types cannot be detected for generated fields (for example ``max(data)``),
   even when the *detect_types* parameter is set.
   In such cases, the returned type is :class:`str`.

   By default, *check_same_thread* is :const:`True` and only the creating thread may
   use the connection. If set :const:`False`, the returned connection may be shared
   across multiple threads. When using multiple threads with the same connection
   writing operations should be serialized by the user to avoid data corruption.

   By default, the :mod:`sqlite3` module uses its :class:`Connection` class for the
   connect call.  You can, however, subclass the :class:`Connection` class and make
   :func:`connect` use your class instead by providing your class for the *factory*
   parameter.

   Consult the section :ref:`sqlite3-types` of this manual for details.

   The :mod:`sqlite3` module internally uses a statement cache to avoid SQL parsing
   overhead. If you want to explicitly set the number of statements that are cached
   for the connection, you can set the *cached_statements* parameter. The currently
   implemented default is to cache 128 statements.

   If *uri* is :const:`True`, *database* is interpreted as a
   :abbr:`URI (Uniform Resource Identifier)` with a file path and an optional
   query string.  The scheme part *must* be ``"file:"``.  The path can be a
   relative or absolute file path.  The query string allows us to pass
   parameters to SQLite. Some useful URI tricks include::

       # Open a database in read-only mode.
       con = sqlite3.connect("file:template.db?mode=ro", uri=True)

       # Don't implicitly create a new database file if it does not already exist.
       # Will raise sqlite3.OperationalError if unable to open a database file.
       con = sqlite3.connect("file:nosuchdb.db?mode=rw", uri=True)

       # Create a shared named in-memory database.
       con1 = sqlite3.connect("file:mem1?mode=memory&cache=shared", uri=True)
       con2 = sqlite3.connect("file:mem1?mode=memory&cache=shared", uri=True)
       con1.executescript("create table t(t); insert into t values(28);")
       rows = con2.execute("select * from t").fetchall()

   More information about this feature, including a list of recognized
   parameters, can be found in the
   `SQLite URI documentation <https://www.sqlite.org/uri.html>`_.

   .. audit-event:: sqlite3.connect database sqlite3.connect
   .. audit-event:: sqlite3.connect/handle connection_handle sqlite3.connect

   .. versionchanged:: 3.4
      Added the *uri* parameter.

   .. versionchanged:: 3.7
      *database* can now also be a :term:`path-like object`, not only a string.

   .. versionchanged:: 3.10
      Added the ``sqlite3.connect/handle`` auditing event.


.. function:: register_converter(typename, converter)

   Register the *converter* callable to convert SQLite objects of type
   *typename* into a Python object of a specific type.
   The converter is invoked for all SQLite values of type *typename*;
   it is passed a :class:`bytes` object and should return an object of the
   desired Python type.
   Consult the parameter *detect_types* of
   :func:`connect` for information regarding how type detection works.

   Note: *typename* and the name of the type in your query are matched
   case-insensitively.


.. function:: register_adapter(type, adapter)

   Register an *adapter* callable to adapt the Python type *type* into an
   SQLite type.
   The adapter is called with a Python object of type *type* as its sole
   argument, and must return a value of a
   :ref:`type that SQLite natively understands<sqlite3-types>`.


.. function:: complete_statement(statement)

   Returns :const:`True` if the string *statement* contains one or more complete SQL
   statements terminated by semicolons. It does not verify that the SQL is
   syntactically correct, only that there are no unclosed string literals and the
   statement is terminated by a semicolon.

   This can be used to build a shell for SQLite, as in the following example:


   .. literalinclude:: ../includes/sqlite3/complete_statement.py


.. function:: enable_callback_tracebacks(flag)

   By default you will not get any tracebacks in user-defined functions,
   aggregates, converters, authorizer callbacks etc. If you want to debug them,
   you can call this function with *flag* set to :const:`True`. Afterwards, you
   will get tracebacks from callbacks on :data:`sys.stderr`. Use :const:`False`
   to disable the feature again.

   Register an :func:`unraisable hook handler <sys.unraisablehook>` for an
   improved debug experience::

      >>> import sqlite3
      >>> sqlite3.enable_callback_tracebacks(True)
      >>> cx = sqlite3.connect(":memory:")
      >>> cx.set_trace_callback(lambda stmt: 5/0)
      >>> cx.execute("select 1")
      Exception ignored in: <function <lambda> at 0x10b4e3ee0>
      Traceback (most recent call last):
        File "<stdin>", line 1, in <lambda>
      ZeroDivisionError: division by zero
      >>> import sys
      >>> sys.unraisablehook = lambda unraisable: print(unraisable)
      >>> cx.execute("select 1")
      UnraisableHookArgs(exc_type=<class 'ZeroDivisionError'>, exc_value=ZeroDivisionError('division by zero'), exc_traceback=<traceback object at 0x10b559900>, err_msg=None, object=<function <lambda> at 0x10b4e3ee0>)
      <sqlite3.Cursor object at 0x10b1fe840>


.. _sqlite3-connection-objects:

Connection Objects
------------------

.. class:: Connection

   An SQLite database connection has the following attributes and methods:

   .. attribute:: isolation_level

      Get or set the current default isolation level. :const:`None` for autocommit mode or
      one of "DEFERRED", "IMMEDIATE" or "EXCLUSIVE". See section
      :ref:`sqlite3-controlling-transactions` for a more detailed explanation.

   .. attribute:: in_transaction

      :const:`True` if a transaction is active (there are uncommitted changes),
      :const:`False` otherwise.  Read-only attribute.

      .. versionadded:: 3.2

   .. method:: cursor(factory=Cursor)

      The cursor method accepts a single optional parameter *factory*. If
      supplied, this must be a callable returning an instance of :class:`Cursor`
      or its subclasses.

   .. method:: blobopen(table, column, row, /, *, readonly=False, name="main")

      Open a :class:`Blob` handle to the :abbr:`BLOB (Binary Large OBject)`
      located in table name *table*, column name *column*, and row index *row*
      of database *name*.
      When *readonly* is :const:`True` the blob is opened without write
      permissions.
      Trying to open a blob in a ``WITHOUT ROWID`` table will raise
      :exc:`OperationalError`.

      .. note::

         The blob size cannot be changed using the :class:`Blob` class.
         Use the SQL function ``zeroblob`` to create a blob with a fixed size.

      .. versionadded:: 3.11

   .. method:: commit()

      Commit any pending transaction to the database.
      If there is no open transaction, this method is a no-op.

   .. method:: rollback()

      Roll back to the start of any pending transaction.
      If there is no open transaction, this method is a no-op.

   .. method:: close()

      Close the database connection.
      Any pending transaction is not committed implicitly;
      make sure to :meth:`commit` before closing
      to avoid losing pending changes.

   .. method:: execute(sql[, parameters])

      Create a new :class:`Cursor` object and call
      :meth:`~Cursor.execute` on it with the given *sql* and *parameters*.
      Return the new cursor object.

   .. method:: executemany(sql[, parameters])

      Create a new :class:`Cursor` object and call
      :meth:`~Cursor.executemany` on it with the given *sql* and *parameters*.
      Return the new cursor object.

   .. method:: executescript(sql_script)

      Create a new :class:`Cursor` object and call
      :meth:`~Cursor.executescript` on it with the given *sql_script*.
      Return the new cursor object.

   .. method:: create_function(name, narg, func, *, deterministic=False)

      Creates a user-defined function that you can later use from within SQL
      statements under the function name *name*. *narg* is the number of
      parameters the function accepts (if *narg* is -1, the function may
      take any number of arguments), and *func* is a Python callable that is
      called as the SQL function. If *deterministic* is true, the created function
      is marked as `deterministic <https://sqlite.org/deterministic.html>`_, which
      allows SQLite to perform additional optimizations. This flag is supported by
      SQLite 3.8.3 or higher, :exc:`NotSupportedError` will be raised if used
      with older versions.

      The function can return any of the types supported by SQLite: bytes, str, int,
      float and ``None``.

      .. versionchanged:: 3.8
         The *deterministic* parameter was added.

      Example:

      .. literalinclude:: ../includes/sqlite3/md5func.py


   .. method:: create_aggregate(name, n_arg, aggregate_class)

      Creates a user-defined aggregate function.

      The aggregate class must implement a ``step`` method, which accepts the number
      of parameters *n_arg* (if *n_arg* is -1, the function may take
      any number of arguments), and a ``finalize`` method which will return the
      final result of the aggregate.

      The ``finalize`` method can return any of the types supported by SQLite:
      bytes, str, int, float and ``None``.

      Example:

      .. literalinclude:: ../includes/sqlite3/mysumaggr.py


   .. method:: create_window_function(name, num_params, aggregate_class, /)

      Creates user-defined aggregate window function *name*.

      *aggregate_class* must implement the following methods:

      * ``step``: adds a row to the current window
      * ``value``: returns the current value of the aggregate
      * ``inverse``: removes a row from the current window
      * ``finalize``: returns the final value of the aggregate

      ``step`` and ``value`` accept *num_params* number of parameters,
      unless *num_params* is ``-1``, in which case they may take any number of
      arguments.  ``finalize`` and ``value`` can return any of the types
      supported by SQLite:
      :class:`bytes`, :class:`str`, :class:`int`, :class:`float`, and
      :const:`None`.  Call :meth:`create_window_function` with
      *aggregate_class* set to :const:`None` to clear window function *name*.

      Aggregate window functions are supported by SQLite 3.25.0 and higher.
      :exc:`NotSupportedError` will be raised if used with older versions.

      .. versionadded:: 3.11

      Example:

      .. literalinclude:: ../includes/sqlite3/sumintwindow.py


   .. method:: create_collation(name, callable)

      Create a collation named *name* using the collating function *callable*.
      *callable* is passed two :class:`string <str>` arguments,
      and it should return an :class:`integer <int>`:

      * ``1`` if the first is ordered higher than the second
      * ``-1`` if the first is ordered lower than the second
      * ``0`` if they are ordered equal

      The following example shows a reverse sorting collation:

      .. literalinclude:: ../includes/sqlite3/collation_reverse.py

      Remove a collation function by setting *callable* to :const:`None`.

      .. versionchanged:: 3.11
         The collation name can contain any Unicode character.  Earlier, only
         ASCII characters were allowed.


   .. method:: interrupt()

      You can call this method from a different thread to abort any queries that might
      be executing on the connection. The query will then abort and the caller will
      get an exception.


   .. method:: set_authorizer(authorizer_callback)

      This routine registers a callback. The callback is invoked for each attempt to
      access a column of a table in the database. The callback should return
      :const:`SQLITE_OK` if access is allowed, :const:`SQLITE_DENY` if the entire SQL
      statement should be aborted with an error and :const:`SQLITE_IGNORE` if the
      column should be treated as a NULL value. These constants are available in the
      :mod:`sqlite3` module.

      The first argument to the callback signifies what kind of operation is to be
      authorized. The second and third argument will be arguments or :const:`None`
      depending on the first argument. The 4th argument is the name of the database
      ("main", "temp", etc.) if applicable. The 5th argument is the name of the
      inner-most trigger or view that is responsible for the access attempt or
      :const:`None` if this access attempt is directly from input SQL code.

      Please consult the SQLite documentation about the possible values for the first
      argument and the meaning of the second and third argument depending on the first
      one. All necessary constants are available in the :mod:`sqlite3` module.

      Passing :const:`None` as *authorizer_callback* will disable the authorizer.

      .. versionchanged:: 3.11
         Added support for disabling the authorizer using :const:`None`.


   .. method:: set_progress_handler(progress_handler, n)

      This routine registers a callback. The callback is invoked for every *n*
      instructions of the SQLite virtual machine. This is useful if you want to
      get called from SQLite during long-running operations, for example to update
      a GUI.

      If you want to clear any previously installed progress handler, call the
      method with :const:`None` for *progress_handler*.

      Returning a non-zero value from the handler function will terminate the
      currently executing query and cause it to raise a :exc:`DatabaseError`
      exception.


   .. method:: set_trace_callback(trace_callback)

      Registers *trace_callback* to be called for each SQL statement that is
      actually executed by the SQLite backend.

      The only argument passed to the callback is the statement (as
      :class:`str`) that is being executed. The return value of the callback is
      ignored. Note that the backend does not only run statements passed to the
      :meth:`Cursor.execute` methods.  Other sources include the
      :ref:`transaction management <sqlite3-controlling-transactions>` of the
      sqlite3 module and the execution of triggers defined in the current
      database.

      Passing :const:`None` as *trace_callback* will disable the trace callback.

      .. note::
         Exceptions raised in the trace callback are not propagated. As a
         development and debugging aid, use
         :meth:`~sqlite3.enable_callback_tracebacks` to enable printing
         tracebacks from exceptions raised in the trace callback.

      .. versionadded:: 3.3


   .. method:: enable_load_extension(enabled)

      This routine allows/disallows the SQLite engine to load SQLite extensions
      from shared libraries.  SQLite extensions can define new functions,
      aggregates or whole new virtual table implementations.  One well-known
      extension is the fulltext-search extension distributed with SQLite.

      Loadable extensions are disabled by default. See [#f1]_.

      .. audit-event:: sqlite3.enable_load_extension connection,enabled sqlite3.Connection.enable_load_extension

      .. versionadded:: 3.2

      .. versionchanged:: 3.10
         Added the ``sqlite3.enable_load_extension`` auditing event.

      .. literalinclude:: ../includes/sqlite3/load_extension.py

   .. method:: load_extension(path)

      This routine loads an SQLite extension from a shared library.  You have to
      enable extension loading with :meth:`enable_load_extension` before you can
      use this routine.

      Loadable extensions are disabled by default. See [#f1]_.

      .. audit-event:: sqlite3.load_extension connection,path sqlite3.Connection.load_extension

      .. versionadded:: 3.2

      .. versionchanged:: 3.10
         Added the ``sqlite3.load_extension`` auditing event.

   .. attribute:: row_factory

      You can change this attribute to a callable that accepts the cursor and the
      original row as a tuple and will return the real result row.  This way, you can
      implement more advanced ways of returning results, such  as returning an object
      that can also access columns by name.

      Example:

      .. literalinclude:: ../includes/sqlite3/row_factory.py

      If returning a tuple doesn't suffice and you want name-based access to
      columns, you should consider setting :attr:`row_factory` to the
      highly-optimized :class:`sqlite3.Row` type. :class:`Row` provides both
      index-based and case-insensitive name-based access to columns with almost no
      memory overhead. It will probably be better than your own custom
      dictionary-based approach or even a db_row based solution.

      .. XXX what's a db_row-based solution?


   .. attribute:: text_factory

      Using this attribute you can control what objects are returned for the ``TEXT``
      data type. By default, this attribute is set to :class:`str` and the
      :mod:`sqlite3` module will return :class:`str` objects for ``TEXT``.
      If you want to return :class:`bytes` instead, you can set it to :class:`bytes`.

      You can also set it to any other callable that accepts a single bytestring
      parameter and returns the resulting object.

      See the following example code for illustration:

      .. literalinclude:: ../includes/sqlite3/text_factory.py


   .. attribute:: total_changes

      Returns the total number of database rows that have been modified, inserted, or
      deleted since the database connection was opened.


   .. method:: iterdump

      Returns an iterator to dump the database in an SQL text format.  Useful when
      saving an in-memory database for later restoration.  This function provides
      the same capabilities as the :kbd:`.dump` command in the :program:`sqlite3`
      shell.

      Example::

         # Convert file existing_db.db to SQL dump file dump.sql
         import sqlite3

         con = sqlite3.connect('existing_db.db')
         with open('dump.sql', 'w') as f:
             for line in con.iterdump():
                 f.write('%s\n' % line)
         con.close()


   .. method:: backup(target, *, pages=-1, progress=None, name="main", sleep=0.250)

      This method makes a backup of an SQLite database even while it's being accessed
      by other clients, or concurrently by the same connection.  The copy will be
      written into the mandatory argument *target*, that must be another
      :class:`Connection` instance.

      By default, or when *pages* is either ``0`` or a negative integer, the entire
      database is copied in a single step; otherwise the method performs a loop
      copying up to *pages* pages at a time.

      If *progress* is specified, it must either be ``None`` or a callable object that
      will be executed at each iteration with three integer arguments, respectively
      the *status* of the last iteration, the *remaining* number of pages still to be
      copied and the *total* number of pages.

      The *name* argument specifies the database name that will be copied: it must be
      a string containing either ``"main"``, the default, to indicate the main
      database, ``"temp"`` to indicate the temporary database or the name specified
      after the ``AS`` keyword in an ``ATTACH DATABASE`` statement for an attached
      database.

      The *sleep* argument specifies the number of seconds to sleep by between
      successive attempts to backup remaining pages, can be specified either as an
      integer or a floating point value.

      Example 1, copy an existing database into another::

         import sqlite3

         def progress(status, remaining, total):
             print(f'Copied {total-remaining} of {total} pages...')

         con = sqlite3.connect('existing_db.db')
         bck = sqlite3.connect('backup.db')
         with bck:
             con.backup(bck, pages=1, progress=progress)
         bck.close()
         con.close()

      Example 2, copy an existing database into a transient copy::

         import sqlite3

         source = sqlite3.connect('existing_db.db')
         dest = sqlite3.connect(':memory:')
         source.backup(dest)

      .. versionadded:: 3.7


   .. method:: getlimit(category, /)

      Get a connection run-time limit. *category* is the limit category to be
      queried.

      Example, query the maximum length of an SQL statement::

         import sqlite3
         con = sqlite3.connect(":memory:")
         lim = con.getlimit(sqlite3.SQLITE_LIMIT_SQL_LENGTH)
         print(f"SQLITE_LIMIT_SQL_LENGTH={lim}")

      .. versionadded:: 3.11


   .. method:: setlimit(category, limit, /)

      Set a connection run-time limit. *category* is the limit category to be
      set. *limit* is the new limit. If the new limit is a negative number, the
      limit is unchanged.

      Attempts to increase a limit above its hard upper bound are silently
      truncated to the hard upper bound. Regardless of whether or not the limit
      was changed, the prior value of the limit is returned.

      Example, limit the number of attached databases to 1::

         import sqlite3
         con = sqlite3.connect(":memory:")
         con.setlimit(sqlite3.SQLITE_LIMIT_ATTACHED, 1)

      .. versionadded:: 3.11


   .. method:: serialize(*, name="main")

      This method serializes a database into a :class:`bytes` object.  For an
      ordinary on-disk database file, the serialization is just a copy of the
      disk file.  For an in-memory database or a "temp" database, the
      serialization is the same sequence of bytes which would be written to
      disk if that database were backed up to disk.

      *name* is the database to be serialized, and defaults to the main
      database.

      .. note::

         This method is only available if the underlying SQLite library has the
         serialize API.

      .. versionadded:: 3.11


   .. method:: deserialize(data, /, *, name="main")

      This method causes the database connection to disconnect from database
      *name*, and reopen *name* as an in-memory database based on the
      serialization contained in *data*.  Deserialization will raise
      :exc:`OperationalError` if the database connection is currently involved
      in a read transaction or a backup operation.  :exc:`OverflowError` will be
      raised if ``len(data)`` is larger than ``2**63 - 1``, and
      :exc:`DatabaseError` will be raised if *data* does not contain a valid
      SQLite database.

      .. note::

         This method is only available if the underlying SQLite library has the
         deserialize API.

      .. versionadded:: 3.11


.. _sqlite3-cursor-objects:

Cursor Objects
--------------

.. class:: Cursor

   A :class:`Cursor` instance has the following attributes and methods.

   .. index:: single: ? (question mark); in SQL statements
   .. index:: single: : (colon); in SQL statements

   .. method:: execute(sql[, parameters])

      Executes an SQL statement. Values may be bound to the statement using
      :ref:`placeholders <sqlite3-placeholders>`.

      :meth:`execute` will only execute a single SQL statement. If you try to execute
      more than one statement with it, it will raise a :exc:`ProgrammingError`. Use
      :meth:`executescript` if you want to execute multiple SQL statements with one
      call.


   .. method:: executemany(sql, seq_of_parameters)

      Executes a :ref:`parameterized <sqlite3-placeholders>` SQL command
      against all parameter sequences or mappings found in the sequence
      *seq_of_parameters*. The :mod:`sqlite3` module also allows using an
      :term:`iterator` yielding parameters instead of a sequence.

      .. literalinclude:: ../includes/sqlite3/executemany_1.py

      Here's a shorter example using a :term:`generator`:

      .. literalinclude:: ../includes/sqlite3/executemany_2.py


   .. method:: executescript(sql_script)

      This is a nonstandard convenience method for executing multiple SQL statements
      at once. It issues a ``COMMIT`` statement first, then executes the SQL script it
      gets as a parameter.  This method disregards :attr:`isolation_level`; any
      transaction control must be added to *sql_script*.

      *sql_script* can be an instance of :class:`str`.

      Example:

      .. literalinclude:: ../includes/sqlite3/executescript.py


   .. method:: fetchone()

      Fetches the next row of a query result set, returning a single sequence,
      or :const:`None` when no more data is available.


   .. method:: fetchmany(size=cursor.arraysize)

      Fetches the next set of rows of a query result, returning a list.  An empty
      list is returned when no more rows are available.

      The number of rows to fetch per call is specified by the *size* parameter.
      If it is not given, the cursor's arraysize determines the number of rows
      to be fetched. The method should try to fetch as many rows as indicated by
      the size parameter. If this is not possible due to the specified number of
      rows not being available, fewer rows may be returned.

      Note there are performance considerations involved with the *size* parameter.
      For optimal performance, it is usually best to use the arraysize attribute.
      If the *size* parameter is used, then it is best for it to retain the same
      value from one :meth:`fetchmany` call to the next.

   .. method:: fetchall()

      Fetches all (remaining) rows of a query result, returning a list.  Note that
      the cursor's arraysize attribute can affect the performance of this operation.
      An empty list is returned when no rows are available.

   .. method:: close()

      Close the cursor now (rather than whenever ``__del__`` is called).

      The cursor will be unusable from this point forward; a :exc:`ProgrammingError`
      exception will be raised if any operation is attempted with the cursor.

   .. method:: setinputsizes(sizes)

      Required by the DB-API. Does nothing in :mod:`sqlite3`.

   .. method:: setoutputsize(size [, column])

      Required by the DB-API. Does nothing in :mod:`sqlite3`.

   .. attribute:: rowcount

      Although the :class:`Cursor` class of the :mod:`sqlite3` module implements this
      attribute, the database engine's own support for the determination of "rows
      affected"/"rows selected" is quirky.

      For :meth:`executemany` statements, the number of modifications are summed up
      into :attr:`rowcount`.

      As required by the Python DB API Spec, the :attr:`rowcount` attribute "is -1 in
      case no ``executeXX()`` has been performed on the cursor or the rowcount of the
      last operation is not determinable by the interface". This includes ``SELECT``
      statements because we cannot determine the number of rows a query produced
      until all rows were fetched.

   .. attribute:: lastrowid

      This read-only attribute provides the row id of the last inserted row. It
      is only updated after successful ``INSERT`` or ``REPLACE`` statements
      using the :meth:`execute` method.  For other statements, after
      :meth:`executemany` or :meth:`executescript`, or if the insertion failed,
      the value of ``lastrowid`` is left unchanged.  The initial value of
      ``lastrowid`` is :const:`None`.

      .. note::
         Inserts into ``WITHOUT ROWID`` tables are not recorded.

      .. versionchanged:: 3.6
         Added support for the ``REPLACE`` statement.

   .. attribute:: arraysize

      Read/write attribute that controls the number of rows returned by :meth:`fetchmany`.
      The default value is 1 which means a single row would be fetched per call.

   .. attribute:: description

      This read-only attribute provides the column names of the last query. To
      remain compatible with the Python DB API, it returns a 7-tuple for each
      column where the last six items of each tuple are :const:`None`.

      It is set for ``SELECT`` statements without any matching rows as well.

   .. attribute:: connection

      This read-only attribute provides the SQLite database :class:`Connection`
      used by the :class:`Cursor` object.  A :class:`Cursor` object created by
      calling :meth:`con.cursor() <Connection.cursor>` will have a
      :attr:`connection` attribute that refers to *con*::

         >>> con = sqlite3.connect(":memory:")
         >>> cur = con.cursor()
         >>> cur.connection == con
         True

.. _sqlite3-row-objects:

Row Objects
-----------

.. class:: Row

   A :class:`Row` instance serves as a highly optimized
   :attr:`~Connection.row_factory` for :class:`Connection` objects.
   It tries to mimic a tuple in most of its features.

   It supports mapping access by column name and index, iteration,
   representation, equality testing and :func:`len`.

   If two :class:`Row` objects have exactly the same columns and their
   members are equal, they compare equal.

   .. method:: keys

      This method returns a list of column names. Immediately after a query,
      it is the first member of each tuple in :attr:`Cursor.description`.

   .. versionchanged:: 3.5
      Added support of slicing.

Let's assume we initialize a table as in the example given above::

   con = sqlite3.connect(":memory:")
   cur = con.cursor()
   cur.execute('''create table stocks
   (date text, trans text, symbol text,
    qty real, price real)''')
   cur.execute("""insert into stocks
               values ('2006-01-05','BUY','RHAT',100,35.14)""")
   con.commit()
   cur.close()

Now we plug :class:`Row` in::

   >>> con.row_factory = sqlite3.Row
   >>> cur = con.cursor()
   >>> cur.execute('select * from stocks')
   <sqlite3.Cursor object at 0x7f4e7dd8fa80>
   >>> r = cur.fetchone()
   >>> type(r)
   <class 'sqlite3.Row'>
   >>> tuple(r)
   ('2006-01-05', 'BUY', 'RHAT', 100.0, 35.14)
   >>> len(r)
   5
   >>> r[2]
   'RHAT'
   >>> r.keys()
   ['date', 'trans', 'symbol', 'qty', 'price']
   >>> r['qty']
   100.0
   >>> for member in r:
   ...     print(member)
   ...
   2006-01-05
   BUY
   RHAT
   100.0
   35.14


.. _sqlite3-blob-objects:

Blob Objects
------------

.. versionadded:: 3.11

.. class:: Blob

   A :class:`Blob` instance is a :term:`file-like object`
   that can read and write data in an SQLite :abbr:`BLOB (Binary Large OBject)`.
   Call :func:`len(blob) <len>` to get the size (number of bytes) of the blob.
   Use indices and :term:`slices <slice>` for direct access to the blob data.

   Use the :class:`Blob` as a :term:`context manager` to ensure that the blob
   handle is closed after use.

   .. literalinclude:: ../includes/sqlite3/blob.py

   .. method:: close()

      Close the blob.

      The blob will be unusable from this point onward.  An
      :class:`~sqlite3.Error` (or subclass) exception will be raised if any
      further operation is attempted with the blob.

   .. method:: read(length=-1, /)

      Read *length* bytes of data from the blob at the current offset position.
      If the end of the blob is reached, the data up to
      :abbr:`EOF (End of File)` will be returned.  When *length* is not
      specified, or is negative, :meth:`~Blob.read` will read until the end of
      the blob.

   .. method:: write(data, /)

      Write *data* to the blob at the current offset.  This function cannot
      change the blob length.  Writing beyond the end of the blob will raise
      :exc:`ValueError`.

   .. method:: tell()

      Return the current access position of the blob.

   .. method:: seek(offset, origin=os.SEEK_SET, /)

      Set the current access position of the blob to *offset*.  The *origin*
      argument defaults to :data:`os.SEEK_SET` (absolute blob positioning).
      Other values for *origin* are :data:`os.SEEK_CUR` (seek relative to the
      current position) and :data:`os.SEEK_END` (seek relative to the blob’s
      end).


.. _sqlite3-exceptions:

Exceptions
----------

The exception hierarchy is defined by the DB-API 2.0 (:pep:`249`).

.. exception:: Warning

   This exception is not currently raised by the ``sqlite3`` module,
   but may be raised by applications using ``sqlite3``,
   for example if a user-defined function truncates data while inserting.
   ``Warning`` is a subclass of :exc:`Exception`.

.. exception:: Error

   The base class of the other exceptions in this module.
   Use this to catch all errors with one single :keyword:`except` statement.
   ``Error`` is a subclass of :exc:`Exception`.

   .. attribute:: sqlite_errorcode

      The numeric error code from the
      `SQLite API <https://sqlite.org/rescode.html>`_

      .. versionadded:: 3.11

   .. attribute:: sqlite_errorname

      The symbolic name of the numeric error code
      from the `SQLite API <https://sqlite.org/rescode.html>`_

      .. versionadded:: 3.11

.. exception:: InterfaceError

   Exception raised for misuse of the low-level SQLite C API.
   In other words, if this exception is raised, it probably indicates a bug in the
   ``sqlite3`` module.
   ``InterfaceError`` is a subclass of :exc:`Error`.

.. exception:: DatabaseError

   Exception raised for errors that are related to the database.
   This serves as the base exception for several types of database errors.
   It is only raised implicitly through the specialised subclasses.
   ``DatabaseError`` is a subclass of :exc:`Error`.

.. exception:: DataError

   Exception raised for errors caused by problems with the processed data,
   like numeric values out of range, and strings which are too long.
   ``DataError`` is a subclass of :exc:`DatabaseError`.

.. exception:: OperationalError

   Exception raised for errors that are related to the database's operation,
   and not necessarily under the control of the programmer.
   For example, the database path is not found,
   or a transaction could not be processed.
   ``OperationalError`` is a subclass of :exc:`DatabaseError`.

.. exception:: IntegrityError

   Exception raised when the relational integrity of the database is affected,
   e.g. a foreign key check fails.  It is a subclass of :exc:`DatabaseError`.

.. exception:: InternalError

   Exception raised when SQLite encounters an internal error.
   If this is raised, it may indicate that there is a problem with the runtime
   SQLite library.
   ``InternalError`` is a subclass of :exc:`DatabaseError`.

.. exception:: ProgrammingError

   Exception raised for ``sqlite3`` API programming errors,
   for example supplying the wrong number of bindings to a query,
   or trying to operate on a closed :class:`Connection`.
   ``ProgrammingError`` is a subclass of :exc:`DatabaseError`.

.. exception:: NotSupportedError

   Exception raised in case a method or database API is not supported by the
   underlying SQLite library. For example, setting *deterministic* to
   :const:`True` in :meth:`~Connection.create_function`, if the underlying SQLite library
   does not support deterministic functions.
   ``NotSupportedError`` is a subclass of :exc:`DatabaseError`.


.. _sqlite3-types:

SQLite and Python types
-----------------------


Introduction
^^^^^^^^^^^^

SQLite natively supports the following types: ``NULL``, ``INTEGER``,
``REAL``, ``TEXT``, ``BLOB``.

The following Python types can thus be sent to SQLite without any problem:

+-------------------------------+-------------+
| Python type                   | SQLite type |
+===============================+=============+
| :const:`None`                 | ``NULL``    |
+-------------------------------+-------------+
| :class:`int`                  | ``INTEGER`` |
+-------------------------------+-------------+
| :class:`float`                | ``REAL``    |
+-------------------------------+-------------+
| :class:`str`                  | ``TEXT``    |
+-------------------------------+-------------+
| :class:`bytes`                | ``BLOB``    |
+-------------------------------+-------------+


This is how SQLite types are converted to Python types by default:

+-------------+----------------------------------------------+
| SQLite type | Python type                                  |
+=============+==============================================+
| ``NULL``    | :const:`None`                                |
+-------------+----------------------------------------------+
| ``INTEGER`` | :class:`int`                                 |
+-------------+----------------------------------------------+
| ``REAL``    | :class:`float`                               |
+-------------+----------------------------------------------+
| ``TEXT``    | depends on :attr:`~Connection.text_factory`, |
|             | :class:`str` by default                      |
+-------------+----------------------------------------------+
| ``BLOB``    | :class:`bytes`                               |
+-------------+----------------------------------------------+

The type system of the :mod:`sqlite3` module is extensible in two ways: you can
store additional Python types in an SQLite database via object adaptation, and
you can let the :mod:`sqlite3` module convert SQLite types to different Python
types via converters.


Using adapters to store custom Python types in SQLite databases
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SQLite supports only a limited set of data types natively.
To store custom Python types in SQLite databases, *adapt* them to one of the
:ref:`Python types SQLite natively understands<sqlite3-types>`.

There are two ways to adapt Python objects to SQLite types:
letting your object adapt itself, or using an *adapter callable*.
The latter will take precedence above the former.
For a library that exports a custom type,
it may make sense to enable that type to adapt itself.
As an application developer, it may make more sense to take direct control by
registering custom adapter functions.


Letting your object adapt itself
""""""""""""""""""""""""""""""""

Suppose we have a ``Point`` class that represents a pair of coordinates,
``x`` and ``y``, in a Cartesian coordinate system.
The coordinate pair will be stored as a text string in the database,
using a semicolon to separate the coordinates.
This can be implemented by adding a ``__conform__(self, protocol)``
method which returns the adapted value.
The object passed to *protocol* will be of type :class:`PrepareProtocol`.

.. literalinclude:: ../includes/sqlite3/adapter_point_1.py


Registering an adapter callable
"""""""""""""""""""""""""""""""

The other possibility is to create a function that converts the Python object
to an SQLite-compatible type.
This function can then be registered using :func:`register_adapter`.

.. literalinclude:: ../includes/sqlite3/adapter_point_2.py


Converting SQLite values to custom Python types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Writing an adapter lets you convert *from* custom Python types *to* SQLite
values.
To be able to convert *from* SQLite values *to* custom Python types,
we use *converters*.

Let's go back to the :class:`Point` class. We stored the x and y coordinates
separated via semicolons as strings in SQLite.

First, we'll define a converter function that accepts the string as a parameter
and constructs a :class:`Point` object from it.

.. note::

   Converter functions are **always** passed a :class:`bytes` object,
   no matter the underlying SQLite data type.

::

   def convert_point(s):
       x, y = map(float, s.split(b";"))
       return Point(x, y)

We now need to tell ``sqlite3`` when it should convert a given SQLite value.
This is done when connecting to a database, using the *detect_types* parameter
of :func:`connect`. There are three options:

* Implicit: set *detect_types* to :const:`PARSE_DECLTYPES`
* Explicit: set *detect_types* to :const:`PARSE_COLNAMES`
* Both: set *detect_types* to
  ``sqlite3.PARSE_DECLTYPES | sqlite3.PARSE_COLNAMES``.
  Colum names take precedence over declared types.

The following example illustrates the implicit and explicit approaches:

.. literalinclude:: ../includes/sqlite3/converter_point.py


Default adapters and converters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are default adapters for the date and datetime types in the datetime
module. They will be sent as ISO dates/ISO timestamps to SQLite.

The default converters are registered under the name "date" for
:class:`datetime.date` and under the name "timestamp" for
:class:`datetime.datetime`.

This way, you can use date/timestamps from Python without any additional
fiddling in most cases. The format of the adapters is also compatible with the
experimental SQLite date/time functions.

The following example demonstrates this.

.. literalinclude:: ../includes/sqlite3/pysqlite_datetime.py

If a timestamp stored in SQLite has a fractional part longer than 6
numbers, its value will be truncated to microsecond precision by the
timestamp converter.

.. note::

   The default "timestamp" converter ignores UTC offsets in the database and
   always returns a naive :class:`datetime.datetime` object. To preserve UTC
   offsets in timestamps, either leave converters disabled, or register an
   offset-aware converter with :func:`register_converter`.


.. _sqlite3-adapter-converter-recipes:

Adapter and Converter Recipes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This section shows recipes for common adapters and converters.

.. code-block::

   import datetime
   import sqlite3

   def adapt_date_iso(val):
       """Adapt datetime.date to ISO 8601 date."""
       return val.isoformat()

   def adapt_datetime_iso(val):
       """Adapt datetime.datetime to timezone-naive ISO 8601 date."""
       return val.isoformat()

   def adapt_datetime_epoch(val)
       """Adapt datetime.datetime to Unix timestamp."""
       return int(val.timestamp())

   sqlite3.register_adapter(datetime.date, adapt_date_iso)
   sqlite3.register_adapter(datetime.datetime, adapt_datetime_iso)
   sqlite3.register_adapter(datetime.datetime, adapt_datetime_epoch)

   def convert_date(val):
       """Convert ISO 8601 date to datetime.date object."""
       return datetime.date.fromisoformat(val)

   def convert_datetime(val):
       """Convert ISO 8601 datetime to datetime.datetime object."""
       return datetime.datetime.fromisoformat(val)

   def convert_timestamp(val):
       """Convert Unix epoch timestamp to datetime.datetime object."""
       return datetime.datetime.fromtimestamp(val)

   sqlite3.register_converter("date", convert_date)
   sqlite3.register_converter("datetime", convert_datetime)
   sqlite3.register_converter("timestamp", convert_timestamp)


.. _sqlite3-controlling-transactions:

Controlling Transactions
------------------------

The underlying ``sqlite3`` library operates in ``autocommit`` mode by default,
but the Python :mod:`sqlite3` module by default does not.

``autocommit`` mode means that statements that modify the database take effect
immediately.  A ``BEGIN`` or ``SAVEPOINT`` statement disables ``autocommit``
mode, and a ``COMMIT``, a ``ROLLBACK``, or a ``RELEASE`` that ends the
outermost transaction, turns ``autocommit`` mode back on.

The Python :mod:`sqlite3` module by default issues a ``BEGIN`` statement
implicitly before a Data Modification Language (DML) statement (i.e.
``INSERT``/``UPDATE``/``DELETE``/``REPLACE``).

You can control which kind of ``BEGIN`` statements :mod:`sqlite3` implicitly
executes via the *isolation_level* parameter to the :func:`connect`
call, or via the :attr:`isolation_level` property of connections.
If you specify no *isolation_level*, a plain ``BEGIN`` is used, which is
equivalent to specifying ``DEFERRED``.  Other possible values are ``IMMEDIATE``
and ``EXCLUSIVE``.

You can disable the :mod:`sqlite3` module's implicit transaction management by
setting :attr:`isolation_level` to ``None``.  This will leave the underlying
``sqlite3`` library operating in ``autocommit`` mode.  You can then completely
control the transaction state by explicitly issuing ``BEGIN``, ``ROLLBACK``,
``SAVEPOINT``, and ``RELEASE`` statements in your code.

Note that :meth:`~Cursor.executescript` disregards
:attr:`isolation_level`; any transaction control must be added explicitly.

.. versionchanged:: 3.6
   :mod:`sqlite3` used to implicitly commit an open transaction before DDL
   statements.  This is no longer the case.


Using :mod:`sqlite3` efficiently
--------------------------------


Using shortcut methods
^^^^^^^^^^^^^^^^^^^^^^

Using the nonstandard :meth:`execute`, :meth:`executemany` and
:meth:`executescript` methods of the :class:`Connection` object, your code can
be written more concisely because you don't have to create the (often
superfluous) :class:`Cursor` objects explicitly. Instead, the :class:`Cursor`
objects are created implicitly and these shortcut methods return the cursor
objects. This way, you can execute a ``SELECT`` statement and iterate over it
directly using only a single call on the :class:`Connection` object.

.. literalinclude:: ../includes/sqlite3/shortcut_methods.py


Accessing columns by name instead of by index
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One useful feature of the :mod:`sqlite3` module is the built-in
:class:`sqlite3.Row` class designed to be used as a row factory.

Rows wrapped with this class can be accessed both by index (like tuples) and
case-insensitively by name:

.. literalinclude:: ../includes/sqlite3/rowclass.py


.. _sqlite3-connection-context-manager:

Using the connection as a context manager
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A :class:`Connection` object can be used as a context manager that
automatically commits or rolls back open transactions when leaving the body of
the context manager.
If the body of the :keyword:`with` statement finishes without exceptions,
the transaction is committed.
If this commit fails,
or if the body of the ``with`` statement raises an uncaught exception,
the transaction is rolled back.

If there is no open transaction upon leaving the body of the ``with`` statement,
the context manager is a no-op.

.. note::

   The context manager neither implicitly opens a new transaction
   nor closes the connection.

.. literalinclude:: ../includes/sqlite3/ctx_manager.py


.. rubric:: Footnotes

.. [#f1] The sqlite3 module is not built with loadable extension support by
   default, because some platforms (notably macOS) have SQLite
   libraries which are compiled without this feature. To get loadable
   extension support, you must pass the
   :option:`--enable-loadable-sqlite-extensions` option to configure.
