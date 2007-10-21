
:mod:`sqlite3` --- DB-API 2.0 interface for SQLite databases
============================================================

.. module:: sqlite3
   :synopsis: A DB-API 2.0 implementation using SQLite 3.x.
.. sectionauthor:: Gerhard HÃ¤ring <gh@ghaering.de>


.. versionadded:: 2.5

SQLite is a C library that provides a lightweight disk-based database that
doesn't require a separate server process and allows accessing the database
using a nonstandard variant of the SQL query language. Some applications can use
SQLite for internal data storage.  It's also possible to prototype an
application using SQLite and then port the code to a larger database such as
PostgreSQL or Oracle.

pysqlite was written by Gerhard Häring and provides a SQL interface compliant
with the DB-API 2.0 specification described by :pep:`249`.

To use the module, you must first create a :class:`Connection` object that
represents the database.  Here the data will be stored in the
:file:`/tmp/example` file::

   conn = sqlite3.connect('/tmp/example')

You can also supply the special name ``:memory:`` to create a database in RAM.

Once you have a :class:`Connection`, you can create a :class:`Cursor`  object
and call its :meth:`execute` method to perform SQL commands::

   c = conn.cursor()

   # Create table
   c.execute('''create table stocks
   (date text, trans text, symbol text,
    qty real, price real)''')

   # Insert a row of data
   c.execute("""insert into stocks
             values ('2006-01-05','BUY','RHAT',100,35.14)""")

   # Save (commit) the changes
   conn.commit()

   # We can also close the cursor if we are done with it
   c.close()

Usually your SQL operations will need to use values from Python variables.  You
shouldn't assemble your query using Python's string operations because doing so
is insecure; it makes your program vulnerable to an SQL injection attack.

Instead, use the DB-API's parameter substitution.  Put ``?`` as a placeholder
wherever you want to use a value, and then provide a tuple of values as the
second argument to the cursor's :meth:`execute` method.  (Other database modules
may use a different placeholder, such as ``%s`` or ``:1``.) For example::

   # Never do this -- insecure!
   symbol = 'IBM'
   c.execute("... where symbol = '%s'" % symbol)

   # Do this instead
   t = (symbol,)
   c.execute('select * from stocks where symbol=?', t)

   # Larger example
   for t in (('2006-03-28', 'BUY', 'IBM', 1000, 45.00),
             ('2006-04-05', 'BUY', 'MSOFT', 1000, 72.00),
             ('2006-04-06', 'SELL', 'IBM', 500, 53.00),
            ):
       c.execute('insert into stocks values (?,?,?,?,?)', t)

To retrieve data after executing a SELECT statement, you can either treat the
cursor as an :term:`iterator`, call the cursor's :meth:`fetchone` method to
retrieve a single matching row, or call :meth:`fetchall` to get a list of the
matching rows.

This example uses the iterator form::

   >>> c = conn.cursor()
   >>> c.execute('select * from stocks order by price')
   >>> for row in c:
   ...    print row
   ...
   (u'2006-01-05', u'BUY', u'RHAT', 100, 35.140000000000001)
   (u'2006-03-28', u'BUY', u'IBM', 1000, 45.0)
   (u'2006-04-06', u'SELL', u'IBM', 500, 53.0)
   (u'2006-04-05', u'BUY', u'MSOFT', 1000, 72.0)
   >>>


.. seealso::

   http://www.pysqlite.org
      The pysqlite web page.

   http://www.sqlite.org
      The SQLite web page; the documentation describes the syntax and the available
      data types for the supported SQL dialect.

   :pep:`249` - Database API Specification 2.0
      PEP written by Marc-André Lemburg.


.. _sqlite3-module-contents:

Module functions and constants
------------------------------


.. data:: PARSE_DECLTYPES

   This constant is meant to be used with the *detect_types* parameter of the
   :func:`connect` function.

   Setting it makes the :mod:`sqlite3` module parse the declared type for each
   column it returns.  It will parse out the first word of the declared type, i. e.
   for "integer primary key", it will parse out "integer". Then for that column, it
   will look into the converters dictionary and use the converter function
   registered for that type there.  Converter names are case-sensitive!


.. data:: PARSE_COLNAMES

   This constant is meant to be used with the *detect_types* parameter of the
   :func:`connect` function.

   Setting this makes the SQLite interface parse the column name for each column it
   returns.  It will look for a string formed [mytype] in there, and then decide
   that 'mytype' is the type of the column. It will try to find an entry of
   'mytype' in the converters dictionary and then use the converter function found
   there to return the value. The column name found in :attr:`cursor.description`
   is only the first word of the column name, i.  e. if you use something like
   ``'as "x [datetime]"'`` in your SQL, then we will parse out everything until the
   first blank for the column name: the column name would simply be "x".


.. function:: connect(database[, timeout, isolation_level, detect_types, factory])

   Opens a connection to the SQLite database file *database*. You can use
   ``":memory:"`` to open a database connection to a database that resides in RAM
   instead of on disk.

   When a database is accessed by multiple connections, and one of the processes
   modifies the database, the SQLite database is locked until that transaction is
   committed. The *timeout* parameter specifies how long the connection should wait
   for the lock to go away until raising an exception. The default for the timeout
   parameter is 5.0 (five seconds).

   For the *isolation_level* parameter, please see the
   :attr:`Connection.isolation_level` property of :class:`Connection` objects.

   SQLite natively supports only the types TEXT, INTEGER, FLOAT, BLOB and NULL. If
   you want to use other types you must add support for them yourself. The
   *detect_types* parameter and the using custom **converters** registered with the
   module-level :func:`register_converter` function allow you to easily do that.

   *detect_types* defaults to 0 (i. e. off, no type detection), you can set it to
   any combination of :const:`PARSE_DECLTYPES` and :const:`PARSE_COLNAMES` to turn
   type detection on.

   By default, the :mod:`sqlite3` module uses its :class:`Connection` class for the
   connect call.  You can, however, subclass the :class:`Connection` class and make
   :func:`connect` use your class instead by providing your class for the *factory*
   parameter.

   Consult the section :ref:`sqlite3-types` of this manual for details.

   The :mod:`sqlite3` module internally uses a statement cache to avoid SQL parsing
   overhead. If you want to explicitly set the number of statements that are cached
   for the connection, you can set the *cached_statements* parameter. The currently
   implemented default is to cache 100 statements.


.. function:: register_converter(typename, callable)

   Registers a callable to convert a bytestring from the database into a custom
   Python type. The callable will be invoked for all database values that are of
   the type *typename*. Confer the parameter *detect_types* of the :func:`connect`
   function for how the type detection works. Note that the case of *typename* and
   the name of the type in your query must match!


.. function:: register_adapter(type, callable)

   Registers a callable to convert the custom Python type *type* into one of
   SQLite's supported types. The callable *callable* accepts as single parameter
   the Python value, and must return a value of the following types: int, long,
   float, str (UTF-8 encoded), unicode or buffer.


.. function:: complete_statement(sql)

   Returns :const:`True` if the string *sql* contains one or more complete SQL
   statements terminated by semicolons. It does not verify that the SQL is
   syntactically correct, only that there are no unclosed string literals and the
   statement is terminated by a semicolon.

   This can be used to build a shell for SQLite, as in the following example:


   .. literalinclude:: ../includes/sqlite3/complete_statement.py


.. function:: enable_callback_tracebacks(flag)

   By default you will not get any tracebacks in user-defined functions,
   aggregates, converters, authorizer callbacks etc. If you want to debug them, you
   can call this function with *flag* as True. Afterwards, you will get tracebacks
   from callbacks on ``sys.stderr``. Use :const:`False` to disable the feature
   again.


.. _sqlite3-connection-objects:

Connection Objects
------------------

A :class:`Connection` instance has the following attributes and methods:

.. attribute:: Connection.isolation_level

   Get or set the current isolation level. None for autocommit mode or one of
   "DEFERRED", "IMMEDIATE" or "EXLUSIVE". See section
   :ref:`sqlite3-controlling-transactions` for a more detailed explanation.


.. method:: Connection.cursor([cursorClass])

   The cursor method accepts a single optional parameter *cursorClass*. If
   supplied, this must be a custom cursor class that extends
   :class:`sqlite3.Cursor`.


.. method:: Connection.execute(sql, [parameters])

   This is a nonstandard shortcut that creates an intermediate cursor object by
   calling the cursor method, then calls the cursor's :meth:`execute` method with
   the parameters given.


.. method:: Connection.executemany(sql, [parameters])

   This is a nonstandard shortcut that creates an intermediate cursor object by
   calling the cursor method, then calls the cursor's :meth:`executemany` method
   with the parameters given.


.. method:: Connection.executescript(sql_script)

   This is a nonstandard shortcut that creates an intermediate cursor object by
   calling the cursor method, then calls the cursor's :meth:`executescript` method
   with the parameters given.


.. method:: Connection.create_function(name, num_params, func)

   Creates a user-defined function that you can later use from within SQL
   statements under the function name *name*. *num_params* is the number of
   parameters the function accepts, and *func* is a Python callable that is called
   as the SQL function.

   The function can return any of the types supported by SQLite: unicode, str, int,
   long, float, buffer and None.

   Example:

   .. literalinclude:: ../includes/sqlite3/md5func.py


.. method:: Connection.create_aggregate(name, num_params, aggregate_class)

   Creates a user-defined aggregate function.

   The aggregate class must implement a ``step`` method, which accepts the number
   of parameters *num_params*, and a ``finalize`` method which will return the
   final result of the aggregate.

   The ``finalize`` method can return any of the types supported by SQLite:
   unicode, str, int, long, float, buffer and None.

   Example:

   .. literalinclude:: ../includes/sqlite3/mysumaggr.py


.. method:: Connection.create_collation(name, callable)

   Creates a collation with the specified *name* and *callable*. The callable will
   be passed two string arguments. It should return -1 if the first is ordered
   lower than the second, 0 if they are ordered equal and 1 if the first is ordered
   higher than the second.  Note that this controls sorting (ORDER BY in SQL) so
   your comparisons don't affect other SQL operations.

   Note that the callable will get its parameters as Python bytestrings, which will
   normally be encoded in UTF-8.

   The following example shows a custom collation that sorts "the wrong way":

   .. literalinclude:: ../includes/sqlite3/collation_reverse.py

   To remove a collation, call ``create_collation`` with None as callable::

      con.create_collation("reverse", None)


.. method:: Connection.interrupt()

   You can call this method from a different thread to abort any queries that might
   be executing on the connection. The query will then abort and the caller will
   get an exception.


.. method:: Connection.set_authorizer(authorizer_callback)

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


.. attribute:: Connection.row_factory

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

   .. % XXX what's a db_row-based solution?


.. attribute:: Connection.text_factory

   Using this attribute you can control what objects are returned for the TEXT data
   type. By default, this attribute is set to :class:`unicode` and the
   :mod:`sqlite3` module will return Unicode objects for TEXT. If you want to
   return bytestrings instead, you can set it to :class:`str`.

   For efficiency reasons, there's also a way to return Unicode objects only for
   non-ASCII data, and bytestrings otherwise. To activate it, set this attribute to
   :const:`sqlite3.OptimizedUnicode`.

   You can also set it to any other callable that accepts a single bytestring
   parameter and returns the resulting object.

   See the following example code for illustration:

   .. literalinclude:: ../includes/sqlite3/text_factory.py


.. attribute:: Connection.total_changes

   Returns the total number of database rows that have been modified, inserted, or
   deleted since the database connection was opened.


.. _sqlite3-cursor-objects:

Cursor Objects
--------------

A :class:`Cursor` instance has the following attributes and methods:


.. method:: Cursor.execute(sql, [parameters])

   Executes a SQL statement. The SQL statement may be parametrized (i. e.
   placeholders instead of SQL literals). The :mod:`sqlite3` module supports two
   kinds of placeholders: question marks (qmark style) and named placeholders
   (named style).

   This example shows how to use parameters with qmark style:

   .. literalinclude:: ../includes/sqlite3/execute_1.py

   This example shows how to use the named style:

   .. literalinclude:: ../includes/sqlite3/execute_2.py

   :meth:`execute` will only execute a single SQL statement. If you try to execute
   more than one statement with it, it will raise a Warning. Use
   :meth:`executescript` if you want to execute multiple SQL statements with one
   call.


.. method:: Cursor.executemany(sql, seq_of_parameters)

   Executes a SQL command against all parameter sequences or mappings found in
   the sequence *sql*.  The :mod:`sqlite3` module also allows using an
   :term:`iterator` yielding parameters instead of a sequence.

   .. literalinclude:: ../includes/sqlite3/executemany_1.py

   Here's a shorter example using a :term:`generator`:

   .. literalinclude:: ../includes/sqlite3/executemany_2.py


.. method:: Cursor.executescript(sql_script)

   This is a nonstandard convenience method for executing multiple SQL statements
   at once. It issues a COMMIT statement first, then executes the SQL script it
   gets as a parameter.

   *sql_script* can be a bytestring or a Unicode string.

   Example:

   .. literalinclude:: ../includes/sqlite3/executescript.py


.. attribute:: Cursor.rowcount

   Although the :class:`Cursor` class of the :mod:`sqlite3` module implements this
   attribute, the database engine's own support for the determination of "rows
   affected"/"rows selected" is quirky.

   For ``DELETE`` statements, SQLite reports :attr:`rowcount` as 0 if you make a
   ``DELETE FROM table`` without any condition.

   For :meth:`executemany` statements, the number of modifications are summed up
   into :attr:`rowcount`.

   As required by the Python DB API Spec, the :attr:`rowcount` attribute "is -1 in
   case no executeXX() has been performed on the cursor or the rowcount of the last
   operation is not determinable by the interface".

   This includes ``SELECT`` statements because we cannot determine the number of
   rows a query produced until all rows were fetched.


.. _sqlite3-types:

SQLite and Python types
-----------------------


Introduction
^^^^^^^^^^^^

SQLite natively supports the following types: NULL, INTEGER, REAL, TEXT, BLOB.

The following Python types can thus be sent to SQLite without any problem:

+------------------------+-------------+
| Python type            | SQLite type |
+========================+=============+
| ``None``               | NULL        |
+------------------------+-------------+
| ``int``                | INTEGER     |
+------------------------+-------------+
| ``long``               | INTEGER     |
+------------------------+-------------+
| ``float``              | REAL        |
+------------------------+-------------+
| ``str (UTF8-encoded)`` | TEXT        |
+------------------------+-------------+
| ``unicode``            | TEXT        |
+------------------------+-------------+
| ``buffer``             | BLOB        |
+------------------------+-------------+

This is how SQLite types are converted to Python types by default:

+-------------+---------------------------------------------+
| SQLite type | Python type                                 |
+=============+=============================================+
| ``NULL``    | None                                        |
+-------------+---------------------------------------------+
| ``INTEGER`` | int or long, depending on size              |
+-------------+---------------------------------------------+
| ``REAL``    | float                                       |
+-------------+---------------------------------------------+
| ``TEXT``    | depends on text_factory, unicode by default |
+-------------+---------------------------------------------+
| ``BLOB``    | buffer                                      |
+-------------+---------------------------------------------+

The type system of the :mod:`sqlite3` module is extensible in two ways: you can
store additional Python types in a SQLite database via object adaptation, and
you can let the :mod:`sqlite3` module convert SQLite types to different Python
types via converters.


Using adapters to store additional Python types in SQLite databases
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As described before, SQLite supports only a limited set of types natively. To
use other Python types with SQLite, you must **adapt** them to one of the
sqlite3 module's supported types for SQLite: one of NoneType, int, long, float,
str, unicode, buffer.

The :mod:`sqlite3` module uses Python object adaptation, as described in
:pep:`246` for this.  The protocol to use is :class:`PrepareProtocol`.

There are two ways to enable the :mod:`sqlite3` module to adapt a custom Python
type to one of the supported ones.


Letting your object adapt itself
""""""""""""""""""""""""""""""""

This is a good approach if you write the class yourself. Let's suppose you have
a class like this::

   class Point(object):
       def __init__(self, x, y):
           self.x, self.y = x, y

Now you want to store the point in a single SQLite column.  First you'll have to
choose one of the supported types first to be used for representing the point.
Let's just use str and separate the coordinates using a semicolon. Then you need
to give your class a method ``__conform__(self, protocol)`` which must return
the converted value. The parameter *protocol* will be :class:`PrepareProtocol`.

.. literalinclude:: ../includes/sqlite3/adapter_point_1.py


Registering an adapter callable
"""""""""""""""""""""""""""""""

The other possibility is to create a function that converts the type to the
string representation and register the function with :meth:`register_adapter`.

.. note::

   The type/class to adapt must be a new-style class, i. e. it must have
   :class:`object` as one of its bases.

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

   Converter functions **always** get called with a string, no matter under which
   data type you sent the value to SQLite.

.. note::

   Converter names are looked up in a case-sensitive manner.

::

   def convert_point(s):
       x, y = map(float, s.split(";"))
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


.. _sqlite3-controlling-transactions:

Controlling Transactions
------------------------

By default, the :mod:`sqlite3` module opens transactions implicitly before a
Data Modification Language (DML)  statement (i.e. INSERT/UPDATE/DELETE/REPLACE),
and commits transactions implicitly before a non-DML, non-query statement (i. e.
anything other than SELECT/INSERT/UPDATE/DELETE/REPLACE).

So if you are within a transaction and issue a command like ``CREATE TABLE
...``, ``VACUUM``, ``PRAGMA``, the :mod:`sqlite3` module will commit implicitly
before executing that command. There are two reasons for doing that. The first
is that some of these commands don't work within transactions. The other reason
is that pysqlite needs to keep track of the transaction state (if a transaction
is active or not).

You can control which kind of "BEGIN" statements pysqlite implicitly executes
(or none at all) via the *isolation_level* parameter to the :func:`connect`
call, or via the :attr:`isolation_level` property of connections.

If you want **autocommit mode**, then set :attr:`isolation_level` to None.

Otherwise leave it at its default, which will result in a plain "BEGIN"
statement, or set it to one of SQLite's supported isolation levels: DEFERRED,
IMMEDIATE or EXCLUSIVE.

As the :mod:`sqlite3` module needs to keep track of the transaction state, you
should not use ``OR ROLLBACK`` or ``ON CONFLICT ROLLBACK`` in your SQL. Instead,
catch the :exc:`IntegrityError` and call the :meth:`rollback` method of the
connection yourself.


Using pysqlite efficiently
--------------------------


Using shortcut methods
^^^^^^^^^^^^^^^^^^^^^^

Using the nonstandard :meth:`execute`, :meth:`executemany` and
:meth:`executescript` methods of the :class:`Connection` object, your code can
be written more concisely because you don't have to create the (often
superfluous) :class:`Cursor` objects explicitly. Instead, the :class:`Cursor`
objects are created implicitly and these shortcut methods return the cursor
objects. This way, you can execute a SELECT statement and iterate over it
directly using only a single call on the :class:`Connection` object.

.. literalinclude:: ../includes/sqlite3/shortcut_methods.py


Accessing columns by name instead of by index
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One useful feature of the :mod:`sqlite3` module is the builtin
:class:`sqlite3.Row` class designed to be used as a row factory.

Rows wrapped with this class can be accessed both by index (like tuples) and
case-insensitively by name:

.. literalinclude:: ../includes/sqlite3/rowclass.py

