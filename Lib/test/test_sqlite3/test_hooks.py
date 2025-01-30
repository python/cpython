# pysqlite2/test/hooks.py: tests for various SQLite-specific hooks
#
# Copyright (C) 2006-2007 Gerhard Häring <gh@ghaering.de>
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

import contextlib
import sqlite3 as sqlite
import unittest

from test.support.os_helper import TESTFN, unlink

from .util import memory_database, cx_limit, with_tracebacks
from .util import MemoryDatabaseMixin


class CollationTests(MemoryDatabaseMixin, unittest.TestCase):

    def test_create_collation_not_string(self):
        with self.assertRaises(TypeError):
            self.con.create_collation(None, lambda x, y: (x > y) - (x < y))

    def test_create_collation_not_callable(self):
        with self.assertRaises(TypeError) as cm:
            self.con.create_collation("X", 42)
        self.assertEqual(str(cm.exception), 'parameter must be callable')

    def test_create_collation_not_ascii(self):
        self.con.create_collation("collä", lambda x, y: (x > y) - (x < y))

    def test_create_collation_bad_upper(self):
        class BadUpperStr(str):
            def upper(self):
                return None
        mycoll = lambda x, y: -((x > y) - (x < y))
        self.con.create_collation(BadUpperStr("mycoll"), mycoll)
        result = self.con.execute("""
            select x from (
            select 'a' as x
            union
            select 'b' as x
            ) order by x collate mycoll
            """).fetchall()
        self.assertEqual(result[0][0], 'b')
        self.assertEqual(result[1][0], 'a')

    def test_collation_is_used(self):
        def mycoll(x, y):
            # reverse order
            return -((x > y) - (x < y))

        self.con.create_collation("mycoll", mycoll)
        sql = """
            select x from (
            select 'a' as x
            union
            select 'b' as x
            union
            select 'c' as x
            ) order by x collate mycoll
            """
        result = self.con.execute(sql).fetchall()
        self.assertEqual(result, [('c',), ('b',), ('a',)],
                         msg='the expected order was not returned')

        self.con.create_collation("mycoll", None)
        with self.assertRaises(sqlite.OperationalError) as cm:
            result = self.con.execute(sql).fetchall()
        self.assertEqual(str(cm.exception), 'no such collation sequence: mycoll')

    def test_collation_returns_large_integer(self):
        def mycoll(x, y):
            # reverse order
            return -((x > y) - (x < y)) * 2**32
        self.con.create_collation("mycoll", mycoll)
        sql = """
            select x from (
            select 'a' as x
            union
            select 'b' as x
            union
            select 'c' as x
            ) order by x collate mycoll
            """
        result = self.con.execute(sql).fetchall()
        self.assertEqual(result, [('c',), ('b',), ('a',)],
                         msg="the expected order was not returned")

    def test_collation_register_twice(self):
        """
        Register two different collation functions under the same name.
        Verify that the last one is actually used.
        """
        con = self.con
        con.create_collation("mycoll", lambda x, y: (x > y) - (x < y))
        con.create_collation("mycoll", lambda x, y: -((x > y) - (x < y)))
        result = con.execute("""
            select x from (select 'a' as x union select 'b' as x) order by x collate mycoll
            """).fetchall()
        self.assertEqual(result[0][0], 'b')
        self.assertEqual(result[1][0], 'a')

    def test_deregister_collation(self):
        """
        Register a collation, then deregister it. Make sure an error is raised if we try
        to use it.
        """
        con = self.con
        con.create_collation("mycoll", lambda x, y: (x > y) - (x < y))
        con.create_collation("mycoll", None)
        with self.assertRaises(sqlite.OperationalError) as cm:
            con.execute("select 'a' as x union select 'b' as x order by x collate mycoll")
        self.assertEqual(str(cm.exception), 'no such collation sequence: mycoll')


class ProgressTests(MemoryDatabaseMixin, unittest.TestCase):

    def test_progress_handler_used(self):
        """
        Test that the progress handler is invoked once it is set.
        """
        progress_calls = []
        def progress():
            progress_calls.append(None)
            return 0
        self.con.set_progress_handler(progress, 1)
        self.con.execute("""
            create table foo(a, b)
            """)
        self.assertTrue(progress_calls)

    def test_opcode_count(self):
        """
        Test that the opcode argument is respected.
        """
        con = self.con
        progress_calls = []
        def progress():
            progress_calls.append(None)
            return 0
        con.set_progress_handler(progress, 1)
        curs = con.cursor()
        curs.execute("""
            create table foo (a, b)
            """)
        first_count = len(progress_calls)
        progress_calls = []
        con.set_progress_handler(progress, 2)
        curs.execute("""
            create table bar (a, b)
            """)
        second_count = len(progress_calls)
        self.assertGreaterEqual(first_count, second_count)

    def test_cancel_operation(self):
        """
        Test that returning a non-zero value stops the operation in progress.
        """
        def progress():
            return 1
        self.con.set_progress_handler(progress, 1)
        curs = self.con.cursor()
        self.assertRaises(
            sqlite.OperationalError,
            curs.execute,
            "create table bar (a, b)")

    def test_clear_handler(self):
        """
        Test that setting the progress handler to None clears the previously set handler.
        """
        con = self.con
        action = 0
        def progress():
            nonlocal action
            action = 1
            return 0
        con.set_progress_handler(progress, 1)
        con.set_progress_handler(None, 1)
        con.execute("select 1 union select 2 union select 3").fetchall()
        self.assertEqual(action, 0, "progress handler was not cleared")

    @with_tracebacks(ZeroDivisionError, msg_regex="bad_progress")
    def test_error_in_progress_handler(self):
        def bad_progress():
            1 / 0
        self.con.set_progress_handler(bad_progress, 1)
        with self.assertRaises(sqlite.OperationalError):
            self.con.execute("""
                create table foo(a, b)
                """)

    @with_tracebacks(ZeroDivisionError, msg_regex="bad_progress")
    def test_error_in_progress_handler_result(self):
        class BadBool:
            def __bool__(self):
                1 / 0
        def bad_progress():
            return BadBool()
        self.con.set_progress_handler(bad_progress, 1)
        with self.assertRaises(sqlite.OperationalError):
            self.con.execute("""
                create table foo(a, b)
                """)

    def test_progress_handler_keyword_args(self):
        regex = (
            r"Passing keyword argument 'progress_handler' to "
            r"_sqlite3.Connection.set_progress_handler\(\) is deprecated. "
            r"Parameter 'progress_handler' will become positional-only in "
            r"Python 3.15."
        )

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.set_progress_handler(progress_handler=lambda: None, n=1)
        self.assertEqual(cm.filename, __file__)


class TraceCallbackTests(MemoryDatabaseMixin, unittest.TestCase):

    @contextlib.contextmanager
    def check_stmt_trace(self, cx, expected):
        try:
            traced = []
            cx.set_trace_callback(lambda stmt: traced.append(stmt))
            yield
        finally:
            self.assertEqual(traced, expected)
            cx.set_trace_callback(None)

    def test_trace_callback_used(self):
        """
        Test that the trace callback is invoked once it is set.
        """
        traced_statements = []
        def trace(statement):
            traced_statements.append(statement)
        self.con.set_trace_callback(trace)
        self.con.execute("create table foo(a, b)")
        self.assertTrue(traced_statements)
        self.assertTrue(any("create table foo" in stmt for stmt in traced_statements))

    def test_clear_trace_callback(self):
        """
        Test that setting the trace callback to None clears the previously set callback.
        """
        con = self.con
        traced_statements = []
        def trace(statement):
            traced_statements.append(statement)
        con.set_trace_callback(trace)
        con.set_trace_callback(None)
        con.execute("create table foo(a, b)")
        self.assertFalse(traced_statements, "trace callback was not cleared")

    def test_unicode_content(self):
        """
        Test that the statement can contain unicode literals.
        """
        unicode_value = '\xf6\xe4\xfc\xd6\xc4\xdc\xdf\u20ac'
        con = self.con
        traced_statements = []
        def trace(statement):
            traced_statements.append(statement)
        con.set_trace_callback(trace)
        con.execute("create table foo(x)")
        con.execute("insert into foo(x) values ('%s')" % unicode_value)
        con.commit()
        self.assertTrue(any(unicode_value in stmt for stmt in traced_statements),
                        "Unicode data %s garbled in trace callback: %s"
                        % (ascii(unicode_value), ', '.join(map(ascii, traced_statements))))

    def test_trace_callback_content(self):
        # set_trace_callback() shouldn't produce duplicate content (bpo-26187)
        traced_statements = []
        def trace(statement):
            traced_statements.append(statement)

        queries = ["create table foo(x)",
                   "insert into foo(x) values(1)"]
        self.addCleanup(unlink, TESTFN)
        con1 = sqlite.connect(TESTFN, isolation_level=None)
        con2 = sqlite.connect(TESTFN)
        try:
            con1.set_trace_callback(trace)
            cur = con1.cursor()
            cur.execute(queries[0])
            con2.execute("create table bar(x)")
            cur.execute(queries[1])
        finally:
            con1.close()
            con2.close()
        self.assertEqual(traced_statements, queries)

    def test_trace_expanded_sql(self):
        expected = [
            "create table t(t)",
            "BEGIN ",
            "insert into t values(0)",
            "insert into t values(1)",
            "insert into t values(2)",
            "COMMIT",
        ]
        with memory_database() as cx, self.check_stmt_trace(cx, expected):
            with cx:
                cx.execute("create table t(t)")
                cx.executemany("insert into t values(?)", ((v,) for v in range(3)))

    @with_tracebacks(
        sqlite.DataError,
        regex="Expanded SQL string exceeds the maximum string length"
    )
    def test_trace_too_much_expanded_sql(self):
        # If the expanded string is too large, we'll fall back to the
        # unexpanded SQL statement.
        # The resulting string length is limited by the runtime limit
        # SQLITE_LIMIT_LENGTH.
        template = "select 1 as a where a="
        category = sqlite.SQLITE_LIMIT_LENGTH
        with memory_database() as cx, cx_limit(cx, category=category) as lim:
            ok_param = "a"
            bad_param = "a" * lim

            unexpanded_query = template + "?"
            expected = [unexpanded_query]
            with self.check_stmt_trace(cx, expected):
                cx.execute(unexpanded_query, (bad_param,))

            expanded_query = f"{template}'{ok_param}'"
            with self.check_stmt_trace(cx, [expanded_query]):
                cx.execute(unexpanded_query, (ok_param,))

    @with_tracebacks(ZeroDivisionError, regex="division by zero")
    def test_trace_bad_handler(self):
        with memory_database() as cx:
            cx.set_trace_callback(lambda stmt: 5/0)
            cx.execute("select 1")

    def test_trace_keyword_args(self):
        regex = (
            r"Passing keyword argument 'trace_callback' to "
            r"_sqlite3.Connection.set_trace_callback\(\) is deprecated. "
            r"Parameter 'trace_callback' will become positional-only in "
            r"Python 3.15."
        )

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.set_trace_callback(trace_callback=lambda: None)
        self.assertEqual(cm.filename, __file__)


if __name__ == "__main__":
    unittest.main()
