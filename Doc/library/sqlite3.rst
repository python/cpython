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

The sqlite3 module was written by Gerhard Häring.  It provides a SQL interface
compliant with the DB-API 2.0 specification described by :pep:`249`, and
requires SQLite 3.7.15 or newer.

To use the module, you must first create a :class:`Connection` object that
represents the database.  Here the data will be stored in the
:file:`example.db` file::

   import sqlite3
   con = sqlite3.connect('example.db')

You can also supply the special name ``:memory:`` to create a database in RAM.

Once you have a :class:`Connection`, you can create a :class:`Cursor`  object
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

The data you've saved is persistent and is available in subsequent sessions::

   import sqlite3
   con = sqlite3.connect('example.db')
   cur = con.cursor()

To retrieve data after executing a SELECT statement, you can either treat the
cursor as an :term:`iterator`, call the cursor's :meth:`~Cursor.fetchone` method to
retrieve a single matching row, or call :meth:`~Cursor.fetchall` to get a list of the
matching rows.

This example uses the iterator form::

   >>> for row in cur.execute('SELECT * FROM stocks ORDER BY price'):
           print(row)

   ('2006-01-05', 'BUY', 'RHAT', 100, 35.14)
   ('2006-03-28', 'BUY', 'IBM', 1000, 45.0)
   ('2006-04-06', 'SELL', 'IBM', 500, 53.0)
   ('2006-04-05', 'BUY', 'MSFT', 1000, 72.0)


.. _sqlite3-placeholders:

Usually your SQL operations will need to use values from Python variables.  You
shouldn't assemble your query using Python's string operations because doing so
is insecure; it makes your program vulnerable to an SQL injection attack
(see the `xkcd webcomic <https://xkcd.com/327/>`_ for a humorous example of
what can go wrong)::

   # Never do this -- insecure!
   symbol = 'RHAT'
   cur.execute("SELECT * FROM stocks WHERE symbol = '%s'" % symbol)

Instead, use the DB-API's parameter substitution. Put a placeholder wherever
you want to use a value, and then provide a tuple of values as the second
argument to the cursor's :meth:`~Cursor.execute` method. An SQL statement may
use one of two kinds of placeholders: question marks (qmark style) or named
placeholders (named style). For the qmark style, ``parameters`` must be a
:term:`sequence <sequence>`. For the named style, it can be either a
:term:`sequence <sequence>` or :class:`dict` instance. The length of the
:term:`sequence <sequence>` must match the number of placeholders, or a
:exc:`ProgrammingError` is raised. If a :class:`dict` is given, it must contain
keys for all named parameters. Any extra items are ignored. Here's an example
of both styles:

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


.. data:: version

   The version number of this module, as a string. This is not the version of
   the SQLite library.


.. data:: version_info

   The version number of this module, as a tuple of integers. This is not the
   version of the SQLite library.


.. data:: sqlite_version

   The version number of the run-time SQLite library, as a string.


.. data:: sqlite_version_info

   The version number of the run-time SQLite library, as a tuple of integers.


.. data:: PARSE_DECLTYPES

   This constant is meant to be used with the *detect_types* parameter of the
   :func:`connect` function.

   Setting it makes the :mod:`sqlite3` module parse the declared type for each
   column it returns.  It will parse out the first word of the declared type,
   i. e.  for "integer primary key", it will parse out "integer", or for
   "number(10)" it will parse out "number". Then for that column, it will look
   into the converters dictionary and use the converter function registered for
   that type there.


.. data:: PARSE_COLNAMES

   This constant is meant to be used with the *detect_types* parameter of the
   :func:`connect` function.

   Setting this makes the SQLite interface parse the column name for each column it
   returns.  It will look for a string formed [mytype] in there, and then decide
   that 'mytype' is the type of the column. It will try to find an entry of
   'mytype' in the converters dictionary and then use the converter function found
   there to return the value. The column name found in :attr:`Cursor.description`
   does not include the type, i. e. if you use something like
   ``'as "Expiration date [datetime]"'`` in your SQL, then we will parse out
   everything until the first ``'['`` for the column name and strip
   the preceeding space: the column name would simply be "Expiration date".


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
   *detect_types* parameter and the using custom **converters** registered with the
   module-level :func:`register_converter` function allow you to easily do that.

   *detect_types* defaults to 0 (i. e. off, no type detection), you can set it to
   any combination of :const:`PARSE_DECLTYPES` and :const:`PARSE_COLNAMES` to turn
   type detection on. Due to SQLite behaviour, types can't be detected for generated
   fields (for example ``max(data)``), even when *detect_types* parameter is set. In
   such case, the returned type is :class:`str`.

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

   If *uri* is true, *database* is interpreted as a URI. This allows you
   to specify options. For example, to open a database in read-only mode
   you can use::

       db = sqlite3.connect('file:path/to/database?mode=ro', uri=True)

   More information about this feature, including a list of recognized options, can
   be found in the `SQLite URI documentation <https://www.sqlite.org/uri.html>`_.

   .. audit-event:: sqlite3.connect database sqlite3.connect
   .. audit-event:: sqlite3.connect/handle connection_handle sqlite3.connect

   .. versionchanged:: 3.4
      Added the *uri* parameter.

   .. versionchanged:: 3.7
      *database* can now also be a :term:`path-like object`, not only a string.

   .. versionchanged:: 3.10
      Added the ``sqlite3.connect/handle`` auditing event.


.. function:: register_converter(typename, callable)

   Registers a callable to convert a bytestring from the database into a custom
   Python type. The callable will be invoked for all database values that are of
   the type *typename*. Confer the parameter *detect_types* of the :func:`connect`
   function for how the type detection works. Note that *typename* and the name of
   the type in your query are matched in case-insensitive manner.


.. function:: register_adapter(type, callable)

   Registers a callable to convert the custom Python type *type* into one of
   SQLite's supported types. The callable *callable* accepts as single parameter
   the Python value, and must return a value of the following types: int,
   float, str or bytes.


.. function:: complete_statement(sql)

   Returns :const:`True` if the string *sql* contains one or more complete SQL
   statements terminated by semicolons. It does not verify that the SQL is
   syntactically correct, only that there are no unclosed string literals and the
   statement is terminated by a semicolon.

   This can be used to build a shell for SQLite, as in the following example:


   .. literalinclude:: ../includes/sqlite3/complete_statement.py


.. function:: enable_callback_tracebacks(flag)

   By default you will not get any tracebacks in user-defined functions,
   aggregates, converters, authorizer callbacks etc. If you want to debug them,
   you can call this function with *flag* set to ``True``. Afterwards, you will
   get tracebacks from callbacks on ``sys.stderr``. Use :const:`False` to
   disable the feature again.


.. _sqlite3-connection-objects:

Connection Objects
------------------

.. class:: Connection

   A SQLite database connection has the following attributes and methods:

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

   .. method:: commit()

      This method commits the current transaction. If you don't call this method,
      anything you did since the last call to ``commit()`` is not visible from
      other database connections. If you wonder why you don't see the data you've
      written to the database, please check you didn't forget to call this method.

   .. method:: rollback()

      This method rolls back any changes to the database since the last call to
      :meth:`commit`.

   .. method:: close()

      This closes the database connection. Note that this does not automatically
      call :meth:`commit`. If you just close your database connection without
      calling :meth:`commit` first, your changes will be lost!

   .. method:: execute(sql[, parameters])

      This is a nonstandard shortcut that creates a cursor object by calling
      the :meth:`~Connection.cursor` method, calls the cursor's
      :meth:`~Cursor.execute` method with the *parameters* given, and returns
      the cursor.

   .. method:: executemany(sql[, parameters])

      This is a nonstandard shortcut that creates a cursor object by
      calling the :meth:`~Connection.cursor` method, calls the cursor's
      :meth:`~Cursor.executemany` method with the *parameters* given, and
      returns the cursor.

   .. method:: executescript(sql_script)

      This is a nonstandard shortcut that creates a cursor object by
      calling the :meth:`~Connection.cursor` method, calls the cursor's
      :meth:`~Cursor.executescript` method with the given *sql_script*, and
      returns the cursor.

   .. method:: create_function(name, num_params, func, *, deterministic=False)

      Creates a user-defined function that you can later use from within SQL
      statements under the function name *name*. *num_params* is the number of
      parameters the function accepts (if *num_params* is -1, the function may
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


   .. method:: create_aggregate(name, num_params, aggregate_class)

      Creates a user-defined aggregate function.

      The aggregate class must implement a ``step`` method, which accepts the number
      of parameters *num_params* (if *num_params* is -1, the function may take
      any number of arguments), and a ``finalize`` method which will return the
      final result of the aggregate.

      The ``finalize`` method can return any of the types supported by SQLite:
      bytes, str, int, float and ``None``.

      Example:

      .. literalinclude:: ../includes/sqlite3/mysumaggr.py


   .. method:: create_collation(name, callable)

      Creates a collation with the specified *name* and *callable*. The callable will
      be passed two string arguments. It should return -1 if the first is ordered
      lower than the second, 0 if they are ordered equal and 1 if the first is ordered
      higher than the second.  Note that this controls sorting (ORDER BY in SQL) so
      your comparisons don't affect other SQL operations.

      Note that the callable will get its parameters as Python bytestrings, which will
      normally be encoded in UTF-8.

      The following example shows a custom collation that sorts "the wrong way":

      .. literalinclude:: ../includes/sqlite3/collation_reverse.py

      To remove a collation, call ``create_collation`` with ``None`` as callable::

         con.create_collation("reverse", None)


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


   .. method:: set_progress_handler(handler, n)

      This routine registers a callback. The callback is invoked for every *n*
      instructions of the SQLite virtual machine. This is useful if you want to
      get called from SQLite during long-running operations, for example to update
      a GUI.

      If you want to clear any previously installed progress handler, call the
      method with :const:`None` for *handler*.

      Returning a non-zero value from the handler function will terminate the
      currently executing query and cause it to raise an :exc:`OperationalError`
      exception.


   .. method:: set_trace_callback(trace_callback)

      Registers *trace_callback* to be called for each SQL statement that is
      actually executed by the SQLite backend.

      The only argument passed to the callback is the statement (as string) that
      is being executed. The return value of the callback is ignored. Note that
      the backend does not only run statements passed to the :meth:`Cursor.execute`
      methods.  Other sources include the transaction management of the Python
      module and the execution of triggers defined in the current database.

      Passing :const:`None` as *trace_callback* will disable the trace callback.

      .. versionadded:: 3.3


   .. method:: enable_load_extension(enabled)

      This routine allows/disallows the SQLite engine to load SQLite extensions
      from shared libraries.  SQLite extensions can define new functions,
      aggregates or whole new virtual table implementations.  One well-known
      extension is the fulltext-search extension distributed with SQLite.

      Loadable extensions are disabled by default. See [#f1]_.

      .. audit-event:: sqlite3.enable_load_extension connection,enabled sqlite3.enable_load_extension

      .. versionadded:: 3.2

      .. versionchanged:: 3.10
         Added the ``sqlite3.enable_load_extension`` auditing event.

      .. literalinclude:: ../includes/sqlite3/load_extension.py

   .. method:: load_extension(path)

      This routine loads a SQLite extension from a shared library.  You have to
      enable extension loading with :meth:`enable_load_extension` before you can
      use this routine.

      Loadable extensions are disabled by default. See [#f1]_.

      .. audit-event:: sqlite3.load_extension connection,path sqlite3.load_extension

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
      :mod:`sqlite3` module will return Unicode objects for ``TEXT``. If you want to
      return bytestrings instead, you can set it to :class:`bytes`.

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

      This method makes a backup of a SQLite database even while it's being accessed
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
      more than one statement with it, it will raise a :exc:`.Warning`. Use
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

      This read-only attribute provides the rowid of the last modified row. It is
      only set if you issued an ``INSERT`` or a ``REPLACE`` statement using the
      :meth:`execute` method.  For operations other than ``INSERT`` or
      ``REPLACE`` or when :meth:`executemany` is called, :attr:`lastrowid` is
      set to :const:`None`.

      If the ``INSERT`` or ``REPLACE`` statement failed to insert the previous
      successful rowid is returned.

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


.. _sqlite3-exceptions:

Exceptions
----------

.. exception:: Warning

   A subclass of :exc:`Exception`.

.. exception:: Error

   The base class of the other exceptions in this module.  It is a subclass
   of :exc:`Exception`.

.. exception:: DatabaseError

   Exception raised for errors that are related to the database.

.. exception:: IntegrityError

   Exception raised when the relational integrity of the database is affected,
   e.g. a foreign key check fails.  It is a subclass of :exc:`DatabaseError`.

.. exception:: ProgrammingError

   Exception raised for programming errors, e.g. table not found or already
   exists, syntax error in the SQL statement, wrong number of parameters
   specified, etc.  It is a subclass of :exc:`DatabaseError`.

.. exception:: OperationalError

   Exception raised for errors that are related to the database's operation
   and not necessarily under the control of the programmer, e.g. an unexpected
   disconnect occurs, the data source name is not found, a transaction could
   not be processed, etc.  It is a subclass of :exc:`DatabaseError`.

.. exception:: NotSupportedError

   Exception raised in case a method or database API was used which is not
   supported by the database, e.g. calling the :meth:`~Connection.rollback`
   method on a connection that does not support transaction or has
   transactions turned off.  It is a subclass of :exc:`DatabaseError`.


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
store additional Python types in a SQLite database via object adaptation, and
you can let the :mod:`sqlite3` module convert SQLite types to different Python
types via converters.


Using adapters to store additional Python types in SQLite databases
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As described before, SQLite supports only a limited set of types natively. To
use other Python types with SQLite, you must **adapt** them to one of the
sqlite3 module's supported types for SQLite: one of NoneType, int, float,
str, bytes.

There are two ways to enable the :mod:`sqlite3` module to adapt a custom Python
type to one of the supported ones.


Letting your object adapt itself
""""""""""""""""""""""""""""""""

This is a good approach if you write the class yourself. Let's suppose you have
a class like this::

   class Point:
       def __init__(self, x, y):
           self.x, self.y = x, y

Now you want to store the point in a single SQLite column.  First you'll have to
choose one of the supported types to be used for representing the point.
Let's just use str and separate the coordinates using a semicolon. Then you need
to give your class a method ``__conform__(self, protocol)`` which must return
the converted value. The parameter *protocol* will be :class:`PrepareProtocol`.

.. literalinclude:: ../includes/sqlite3/adapter_point_1.py


Registering an adapter callable
"""""""""""""""""""""""""""""""

The other possibility is to create a function that converts the type to the
string representation and register the function with :meth:`register_adapter`.

.. literalinclude:: ../includes/sqlite3/adapter_point_2.py

The :mod:`sqlite3` module has two default adapters for Python's built-in
:class:`datetime.date` and :class:`datetime.datetime` types.  Now let's suppose
we want to store :class:`datetime.datetime` objects not in ISO representation,
but as a Unix timestamp.

.. literalinclude:: ../includes/sqlite3/adapter_datetime.py


Converting SQLite values to custom Python types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Writing an adapter lets you send custom Python types to SQLite. But to make it
really useful we need to make the Python to SQLite to Python roundtrip work.

Enter converters.

Let's go back to the :class:`Point` class. We stored the x and y coordinates
separated via semicolons as strings in SQLite.

First, we'll define a converter function that accepts the string as a parameter
and constructs a :class:`Point` object from it.

.. note::

   Converter functions **always** get called with a :class:`bytes` object, no
   matter under which data type you sent the value to SQLite.

::

   def convert_point(s):
       x, y = map(float, s.split(b";"))
       return Point(x, y)

Now you need to make the :mod:`sqlite3` module know that what you select from
the database is actually a point. There are two ways of doing this:

* Implicitly via the declared type

* Explicitly via the column name

Both ways are described in section :ref:`sqlite3-module-contents`, in the entries
for the constants :const:`PARSE_DECLTYPES` and :const:`PARSE_COLNAMES`.

The following example illustrates both approaches.

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


Using the connection as a context manager
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Connection objects can be used as context managers
that automatically commit or rollback transactions.  In the event of an
exception, the transaction is rolled back; otherwise, the transaction is
committed:

.. literalinclude:: ../includes/sqlite3/ctx_manager.py


.. rubric:: Footnotes

.. [#f1] The sqlite3 module is not built with loadable extension support by
   default, because some platforms (notably Mac OS X) have SQLite
   libraries which are compiled without this feature. To get loadable
   extension support, you must pass the
   :option:`--enable-loadable-sqlite-extensions` option to configure.
