# pysqlite2/test/regression.py: pysqlite regression tests
#
# Copyright (C) 2006-2010 Gerhard Häring <gh@ghaering.de>
#
# This file is part of pysqlite.
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

import datetime
import unittest
import sqlite3 as sqlite
import weakref
import functools
from test import support

class RegressionTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def tearDown(self):
        self.con.close()

    def test_pragma_user_version(self):
        # This used to crash pysqlite because this pragma command returns NULL for the column name
        cur = self.con.cursor()
        cur.execute("pragma user_version")

    def test_pragma_schema_version(self):
        # This still crashed pysqlite <= 2.2.1
        con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_COLNAMES)
        try:
            cur = self.con.cursor()
            cur.execute("pragma schema_version")
        finally:
            cur.close()
            con.close()

    def test_statement_reset(self):
        # pysqlite 2.1.0 to 2.2.0 have the problem that not all statements are
        # reset before a rollback, but only those that are still in the
        # statement cache. The others are not accessible from the connection object.
        con = sqlite.connect(":memory:", cached_statements=5)
        cursors = [con.cursor() for x in range(5)]
        cursors[0].execute("create table test(x)")
        for i in range(10):
            cursors[0].executemany("insert into test(x) values (?)", [(x,) for x in range(10)])

        for i in range(5):
            cursors[i].execute(" " * i + "select x from test")

        con.rollback()

    def test_column_name_with_spaces(self):
        cur = self.con.cursor()
        cur.execute('select 1 as "foo bar [datetime]"')
        self.assertEqual(cur.description[0][0], "foo bar [datetime]")

        cur.execute('select 1 as "foo baz"')
        self.assertEqual(cur.description[0][0], "foo baz")

    def test_statement_finalization_on_close_db(self):
        # pysqlite versions <= 2.3.3 only finalized statements in the statement
        # cache when closing the database. statements that were still
        # referenced in cursors weren't closed and could provoke "
        # "OperationalError: Unable to close due to unfinalised statements".
        con = sqlite.connect(":memory:")
        cursors = []
        # default statement cache size is 100
        for i in range(105):
            cur = con.cursor()
            cursors.append(cur)
            cur.execute("select 1 x union select " + str(i))
        con.close()

    def test_on_conflict_rollback(self):
        con = sqlite.connect(":memory:")
        con.execute("create table foo(x, unique(x) on conflict rollback)")
        con.execute("insert into foo(x) values (1)")
        try:
            con.execute("insert into foo(x) values (1)")
        except sqlite.DatabaseError:
            pass
        con.execute("insert into foo(x) values (2)")
        try:
            con.commit()
        except sqlite.OperationalError:
            self.fail("pysqlite knew nothing about the implicit ROLLBACK")

    def test_workaround_for_buggy_sqlite_transfer_bindings(self):
        """
        pysqlite would crash with older SQLite versions unless
        a workaround is implemented.
        """
        self.con.execute("create table foo(bar)")
        self.con.execute("drop table foo")
        self.con.execute("create table foo(bar)")

    def test_empty_statement(self):
        """
        pysqlite used to segfault with SQLite versions 3.5.x. These return NULL
        for "no-operation" statements
        """
        self.con.execute("")

    def test_type_map_usage(self):
        """
        pysqlite until 2.4.1 did not rebuild the row_cast_map when recompiling
        a statement. This test exhibits the problem.
        """
        SELECT = "select * from foo"
        con = sqlite.connect(":memory:",detect_types=sqlite.PARSE_DECLTYPES)
        con.execute("create table foo(bar timestamp)")
        con.execute("insert into foo(bar) values (?)", (datetime.datetime.now(),))
        con.execute(SELECT).close()
        con.execute("drop table foo")
        con.execute("create table foo(bar integer)")
        con.execute("insert into foo(bar) values (5)")
        con.execute(SELECT).close()

    def test_bind_mutating_list(self):
        # Issue41662: Crash when mutate a list of parameters during iteration.
        class X:
            def __conform__(self, protocol):
                parameters.clear()
                return "..."
        parameters = [X(), 0]
        con = sqlite.connect(":memory:",detect_types=sqlite.PARSE_DECLTYPES)
        con.execute("create table foo(bar X, baz integer)")
        # Should not crash
        with self.assertRaises(IndexError):
            con.execute("insert into foo(bar, baz) values (?, ?)", parameters)

    def test_error_msg_decode_error(self):
        # When porting the module to Python 3.0, the error message about
        # decoding errors disappeared. This verifies they're back again.
        with self.assertRaises(sqlite.OperationalError) as cm:
            self.con.execute("select 'xxx' || ? || 'yyy' colname",
                             (bytes(bytearray([250])),)).fetchone()
        msg = "Could not decode to UTF-8 column 'colname' with text 'xxx"
        self.assertIn(msg, str(cm.exception))

    def test_register_adapter(self):
        """
        See issue 3312.
        """
        self.assertRaises(TypeError, sqlite.register_adapter, {}, None)

    def test_set_isolation_level(self):
        # See issue 27881.
        class CustomStr(str):
            def upper(self):
                return None
            def __del__(self):
                con.isolation_level = ""

        con = sqlite.connect(":memory:")
        con.isolation_level = None
        for level in "", "DEFERRED", "IMMEDIATE", "EXCLUSIVE":
            with self.subTest(level=level):
                con.isolation_level = level
                con.isolation_level = level.lower()
                con.isolation_level = level.capitalize()
                con.isolation_level = CustomStr(level)

        # setting isolation_level failure should not alter previous state
        con.isolation_level = None
        con.isolation_level = "DEFERRED"
        pairs = [
            (1, TypeError), (b'', TypeError), ("abc", ValueError),
            ("IMMEDIATE\0EXCLUSIVE", ValueError), ("\xe9", ValueError),
        ]
        for value, exc in pairs:
            with self.subTest(level=value):
                with self.assertRaises(exc):
                    con.isolation_level = value
                self.assertEqual(con.isolation_level, "DEFERRED")

    def test_cursor_constructor_call_check(self):
        """
        Verifies that cursor methods check whether base class __init__ was
        called.
        """
        class Cursor(sqlite.Cursor):
            def __init__(self, con):
                pass

        con = sqlite.connect(":memory:")
        cur = Cursor(con)
        with self.assertRaises(sqlite.ProgrammingError):
            cur.execute("select 4+5").fetchall()
        with self.assertRaisesRegex(sqlite.ProgrammingError,
                                    r'^Base Cursor\.__init__ not called\.$'):
            cur.close()

    def test_str_subclass(self):
        """
        The Python 3.0 port of the module didn't cope with values of subclasses of str.
        """
        class MyStr(str): pass
        self.con.execute("select ?", (MyStr("abc"),))

    def test_connection_constructor_call_check(self):
        """
        Verifies that connection methods check whether base class __init__ was
        called.
        """
        class Connection(sqlite.Connection):
            def __init__(self, name):
                pass

        con = Connection(":memory:")
        with self.assertRaises(sqlite.ProgrammingError):
            cur = con.cursor()

    def test_cursor_registration(self):
        """
        Verifies that subclassed cursor classes are correctly registered with
        the connection object, too.  (fetch-across-rollback problem)
        """
        class Connection(sqlite.Connection):
            def cursor(self):
                return Cursor(self)

        class Cursor(sqlite.Cursor):
            def __init__(self, con):
                sqlite.Cursor.__init__(self, con)

        con = Connection(":memory:")
        cur = con.cursor()
        cur.execute("create table foo(x)")
        cur.executemany("insert into foo(x) values (?)", [(3,), (4,), (5,)])
        cur.execute("select x from foo")
        con.rollback()
        with self.assertRaises(sqlite.InterfaceError):
            cur.fetchall()

    def test_auto_commit(self):
        """
        Verifies that creating a connection in autocommit mode works.
        2.5.3 introduced a regression so that these could no longer
        be created.
        """
        con = sqlite.connect(":memory:", isolation_level=None)

    def test_pragma_autocommit(self):
        """
        Verifies that running a PRAGMA statement that does an autocommit does
        work. This did not work in 2.5.3/2.5.4.
        """
        cur = self.con.cursor()
        cur.execute("create table foo(bar)")
        cur.execute("insert into foo(bar) values (5)")

        cur.execute("pragma page_size")
        row = cur.fetchone()

    def test_connection_call(self):
        """
        Call a connection with a non-string SQL request: check error handling
        of the statement constructor.
        """
        self.assertRaises(TypeError, self.con, 1)

    def test_collation(self):
        def collation_cb(a, b):
            return 1
        self.assertRaises(sqlite.ProgrammingError, self.con.create_collation,
            # Lone surrogate cannot be encoded to the default encoding (utf8)
            "\uDC80", collation_cb)

    def test_recursive_cursor_use(self):
        """
        http://bugs.python.org/issue10811

        Recursively using a cursor, such as when reusing it from a generator led to segfaults.
        Now we catch recursive cursor usage and raise a ProgrammingError.
        """
        con = sqlite.connect(":memory:")

        cur = con.cursor()
        cur.execute("create table a (bar)")
        cur.execute("create table b (baz)")

        def foo():
            cur.execute("insert into a (bar) values (?)", (1,))
            yield 1

        with self.assertRaises(sqlite.ProgrammingError):
            cur.executemany("insert into b (baz) values (?)",
                            ((i,) for i in foo()))

    def test_convert_timestamp_microsecond_padding(self):
        """
        http://bugs.python.org/issue14720

        The microsecond parsing of convert_timestamp() should pad with zeros,
        since the microsecond string "456" actually represents "456000".
        """

        con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_DECLTYPES)
        cur = con.cursor()
        cur.execute("CREATE TABLE t (x TIMESTAMP)")

        # Microseconds should be 456000
        cur.execute("INSERT INTO t (x) VALUES ('2012-04-04 15:06:00.456')")

        # Microseconds should be truncated to 123456
        cur.execute("INSERT INTO t (x) VALUES ('2012-04-04 15:06:00.123456789')")

        cur.execute("SELECT * FROM t")
        values = [x[0] for x in cur.fetchall()]

        self.assertEqual(values, [
            datetime.datetime(2012, 4, 4, 15, 6, 0, 456000),
            datetime.datetime(2012, 4, 4, 15, 6, 0, 123456),
        ])

    def test_invalid_isolation_level_type(self):
        # isolation level is a string, not an integer
        self.assertRaises(TypeError,
                          sqlite.connect, ":memory:", isolation_level=123)


    def test_null_character(self):
        # Issue #21147
        con = sqlite.connect(":memory:")
        self.assertRaises(ValueError, con, "\0select 1")
        self.assertRaises(ValueError, con, "select 1\0")
        cur = con.cursor()
        self.assertRaises(ValueError, cur.execute, " \0select 2")
        self.assertRaises(ValueError, cur.execute, "select 2\0")

    def test_commit_cursor_reset(self):
        """
        Connection.commit() did reset cursors, which made sqlite3
        to return rows multiple times when fetched from cursors
        after commit. See issues 10513 and 23129 for details.
        """
        con = sqlite.connect(":memory:")
        con.executescript("""
        create table t(c);
        create table t2(c);
        insert into t values(0);
        insert into t values(1);
        insert into t values(2);
        """)

        self.assertEqual(con.isolation_level, "")

        counter = 0
        for i, row in enumerate(con.execute("select c from t")):
            with self.subTest(i=i, row=row):
                con.execute("insert into t2(c) values (?)", (i,))
                con.commit()
                if counter == 0:
                    self.assertEqual(row[0], 0)
                elif counter == 1:
                    self.assertEqual(row[0], 1)
                elif counter == 2:
                    self.assertEqual(row[0], 2)
                counter += 1
        self.assertEqual(counter, 3, "should have returned exactly three rows")

    def test_bpo31770(self):
        """
        The interpreter shouldn't crash in case Cursor.__init__() is called
        more than once.
        """
        def callback(*args):
            pass
        con = sqlite.connect(":memory:")
        cur = sqlite.Cursor(con)
        ref = weakref.ref(cur, callback)
        cur.__init__(con)
        del cur
        # The interpreter shouldn't crash when ref is collected.
        del ref
        support.gc_collect()

    def test_del_isolation_level_segfault(self):
        with self.assertRaises(AttributeError):
            del self.con.isolation_level

    def test_bpo37347(self):
        class Printer:
            def log(self, *args):
                return sqlite.SQLITE_OK

        for method in [self.con.set_trace_callback,
                       functools.partial(self.con.set_progress_handler, n=1),
                       self.con.set_authorizer]:
            printer_instance = Printer()
            method(printer_instance.log)
            method(printer_instance.log)
            self.con.execute("select 1")  # trigger seg fault
            method(None)

    def test_return_empty_bytestring(self):
        cur = self.con.execute("select X''")
        val = cur.fetchone()[0]
        self.assertEqual(val, b'')


def suite():
    tests = [
        RegressionTests
    ]
    return unittest.TestSuite(
        [unittest.TestLoader().loadTestsFromTestCase(t) for t in tests]
    )

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
