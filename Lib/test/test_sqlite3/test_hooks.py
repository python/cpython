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

from test.support import import_helper
from test.support.os_helper import TESTFN, unlink

from .util import memory_database, cx_limit, with_tracebacks
from .util import MemoryDatabaseMixin

# TODO(picnixz): increase test coverage for other callbacks
# such as 'func', 'step', 'finalize', and 'collation'.


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

    def test_collation_register_when_busy(self):
        # See https://github.com/python/cpython/issues/146090.
        con = self.con
        con.create_collation("mycoll", lambda x, y: (x > y) - (x < y))
        con.execute("CREATE TABLE t(x TEXT)")
        con.execute("INSERT INTO t VALUES (?)", ("a",))
        con.execute("INSERT INTO t VALUES (?)", ("b",))
        con.commit()

        cursor = self.con.execute("SELECT x FROM t ORDER BY x COLLATE mycoll")
        next(cursor)
        # Replace the collation while the statement is active -> SQLITE_BUSY.
        with self.assertRaises(sqlite.OperationalError) as cm:
            self.con.create_collation("mycoll", lambda a, b: 0)

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


class CollationErrorTests(MemoryDatabaseMixin, unittest.TestCase):
    # gh-161837: A collation callback cannot report an error to SQLite, so
    # the exception stays set while SQLite continues the step. The module
    # must propagate that exception, and it must not call other Python code
    # while the exception is set.

    # When the collation raises, it returns 0 ("equal"). Thus the comparison
    # is true, and abs() of the smallest 64-bit integer makes the step fail.
    step_fails = """
        select case when y = 'q' collate mycoll then abs(x) else 0 end
        from t order by rowid
        """
    step_succeeds = "select x, y = 'q' collate mycoll from t order by rowid"

    def setUp(self):
        super().setUp()
        self.events = []
        self.setUp_collation()
        self.con.execute("create table t(x, y)")
        self.con.executemany("insert into t values (?, ?)",
                             [(1, "a"), (-2**63, "boom")])

    def setUp_collation(self):
        def mycoll(x, y):
            if "boom" in (x, y):
                self.events.append("collation raised")
                raise KeyError("collation raised")
            return (x > y) - (x < y)
        self.con.create_collation("mycoll", mycoll)

    def assertCollationRaises(self):
        return self.assertRaisesRegex(KeyError, "collation raised")

    def events_after_exception(self):
        return self.events[self.events.index("collation raised") + 1:]

    def test_execute(self):
        for sql in self.step_fails, self.step_succeeds:
            with self.subTest(sql=sql), self.assertCollationRaises():
                self.con.execute(sql + " desc")

    def test_fetchone(self):
        # The cursor reads one row ahead, so the first fetch steps to the
        # row that makes the collation raise.
        for sql in self.step_fails, self.step_succeeds:
            with self.subTest(sql=sql):
                cur = self.con.execute(sql)
                with self.assertCollationRaises():
                    cur.fetchone()
                self.assertIsNone(cur.fetchone())
                self.assertEqual(cur.execute("select 1").fetchone(), (1,))

    def test_iteration(self):
        for sql in self.step_fails, self.step_succeeds:
            with self.subTest(sql=sql):
                rows = []
                with self.assertCollationRaises():
                    for row in self.con.execute(sql):
                        rows.append(row)
                self.assertEqual(rows, [])

    def test_executemany(self):
        def params():
            yield ("boom",)
            self.fail("executemany() continued after the exception")
        with self.assertCollationRaises():
            self.con.executemany(
                "insert into t(y) values (? = 'q' collate mycoll)", params())

    def test_executescript(self):
        for sql in self.step_fails, self.step_succeeds:
            with self.subTest(sql=sql):
                with self.assertCollationRaises():
                    self.con.executescript(sql + "; create table u(x)")
                # The script stops after the statement with the exception.
                with self.assertRaisesRegex(sqlite.OperationalError,
                                            "no such table"):
                    self.con.execute("select * from u")

    def test_converter_not_called(self):
        calls = []
        sqlite.register_converter("COLLTEST", calls.append)
        self.addCleanup(sqlite.converters.pop, "COLLTEST", None)
        with memory_database(detect_types=sqlite.PARSE_COLNAMES) as con:
            con.create_collation("mycoll", lambda x, y: 1 / 0)
            with self.assertRaises(ZeroDivisionError):
                con.execute("""
                    select 'a' = 'b' collate mycoll as "x [colltest]"
                    """)
        self.assertEqual(calls, [])

    def test_function_not_called(self):
        def f(value):
            self.events.append("function")
        self.con.create_function("f", 1, f)
        with self.assertCollationRaises():
            self.con.execute("select f(y = 'q' collate mycoll) from t "
                             "order by rowid desc")
        self.assertEqual(self.events, ["collation raised"])

    def test_aggregate_step_not_called(self):
        events = self.events
        class Agg:
            def step(self, value):
                events.append("step")
            def finalize(self):
                events.append("finalize")
        self.con.create_aggregate("agg", 1, Agg)
        with self.assertCollationRaises():
            self.con.execute("select agg(y = 'q' collate mycoll) from t")
        # SQLite calls finalize() to release the aggregate.
        self.assertEqual(self.events_after_exception(), ["finalize"])

    @with_tracebacks(ZeroDivisionError)
    def test_aggregate_finalize_error(self):
        class Agg:
            def step(self, value):
                pass
            def finalize(self):
                1 / 0
        self.con.create_aggregate("agg", 1, Agg)
        with self.assertCollationRaises():
            self.con.execute("select agg(y = 'q' collate mycoll) from t")

    def test_window_function_value_not_called(self):
        events = self.events
        class Window:
            def step(self, value):
                events.append("step")
            def inverse(self, value):
                events.append("inverse")
            def value(self):
                events.append("value")
                return 0
            def finalize(self):
                return 0
        self.con.create_window_function("win", 1, Window)
        # For this query, SQLite evaluates the filter for all rows before it
        # calls the window function. The filter is false for the first row,
        # so the first call after the exception is value(), before step().
        with self.assertCollationRaises():
            self.con.execute("""
                select win(x) filter (where y = 'q' collate mycoll)
                over (order by rowid) from t
                """)
        self.assertEqual(self.events_after_exception(), [])

    def test_window_function_inverse_not_called(self):
        events = self.events
        class Window:
            def step(self, value):
                events.append("step")
            def inverse(self, value):
                events.append("inverse")
            def value(self):
                events.append("value")
                return 0
            def finalize(self):
                return 0
        self.con.create_window_function("win", 1, Window)
        self.con.executemany("insert into t values (?, ?)",
                             [(3, "c"), (4, "d")])
        self.con.execute("create table u(w, y)")
        # SQLite uses the collation when it adds a row to the index. For the
        # next row, the first call is inverse().
        self.con.execute("create index u_y on u(y collate mycoll)")
        with self.assertCollationRaises():
            self.con.execute("""
                insert into u
                select win(x) over (order by rowid
                                    rows between 1 preceding and current row),
                       y
                from t order by rowid
                """)
        self.assertEqual(self.events_after_exception(), [])

    def test_authorizer_not_called(self):
        def authorizer(action, *args):
            self.events.append(action)
            return sqlite.SQLITE_OK
        sql = """
            select t.x, p.name from t join pragma_table_info('t') p
            where t.y = 'q' collate mycoll
            """
        self.con.executemany("insert into t values (?, ?)", [(3, "c")])
        self.con.set_authorizer(authorizer)
        # Control: if the comparison is true, SQLite prepares a statement for
        # pragma_table_info() during the step, and it calls the authorizer.
        self.con.create_collation("mycoll", lambda x, y: 0)
        self.con.execute(sql).fetchall()
        self.assertIn(sqlite.SQLITE_PRAGMA, self.events)
        self.events.clear()
        self.setUp_collation()
        with self.assertCollationRaises():
            self.con.execute(sql)
        self.con.set_authorizer(None)
        self.assertEqual(self.events_after_exception(), [])

    def test_prepare(self):
        # With STAT4, the query planner calls the collation when SQLite
        # prepares the statement.
        options = [o for (o,) in self.con.execute("pragma compile_options")]
        if "ENABLE_STAT4" not in options:
            self.skipTest("requires SQLite with SQLITE_ENABLE_STAT4")
        fail = False
        def statcoll(x, y):
            if fail:
                raise KeyError("collation raised")
            return (x > y) - (x < y)
        self.con.create_collation("statcoll", statcoll)
        self.con.execute("create table s(v text collate statcoll)")
        self.con.execute("create index s_v on s(v)")
        self.con.executemany("insert into s values (?)",
                             [(str(i % 7),) for i in range(200)])
        self.con.execute("analyze")
        fail = True
        with self.assertCollationRaises():
            self.con.execute("select * from s where v = '3'")

    def test_progress_handler_not_called(self):
        def progress():
            self.events.append("progress")
            return 0
        self.con.set_progress_handler(progress, 1)
        with self.assertCollationRaises():
            self.con.execute(self.step_succeeds + " desc")
        self.con.set_progress_handler(None, 1)
        self.assertIn("progress", self.events)
        self.assertEqual(self.events_after_exception(), [])

    def test_trace_callback_not_called(self):
        self.con.execute("create table u(x)")
        # SQLite calls the trace callback again for each trigger program.
        self.con.execute("create trigger u_tr after insert on u "
                         "begin select 1; end")
        traced = []
        sql = ("insert into u select y = 'q' collate mycoll from t "
               "order by rowid desc")
        self.con.set_trace_callback(traced.append)
        with self.assertCollationRaises():
            self.con.execute(sql)
        self.con.set_trace_callback(None)
        self.assertEqual(traced, [sql])


class AuthorizerTests(MemoryDatabaseMixin, unittest.TestCase):

    def assert_not_authorized(self, func, /, *args, **kwargs):
        with self.assertRaisesRegex(sqlite.DatabaseError, "not authorized"):
            func(*args, **kwargs)

    # When a handler has an invalid signature, the exception raised is
    # the same that would be raised if the handler "negatively" replied.

    def test_authorizer_invalid_signature(self):
        self.cx.execute("create table if not exists test(a number)")
        self.cx.set_authorizer(lambda: None)
        self.assert_not_authorized(self.cx.execute, "select * from test")

    # Tests for checking that callback context mutations do not crash.
    # Regression tests for https://github.com/python/cpython/issues/142830.

    @with_tracebacks(ZeroDivisionError, regex="hello world")
    def test_authorizer_concurrent_mutation_in_call(self):
        self.cx.execute("create table if not exists test(a number)")

        def handler(*a, **kw):
            self.cx.set_authorizer(None)
            raise ZeroDivisionError("hello world")

        self.cx.set_authorizer(handler)
        self.assert_not_authorized(self.cx.execute, "select * from test")

    @with_tracebacks(OverflowError)
    def test_authorizer_concurrent_mutation_with_overflown_value(self):
        _testcapi = import_helper.import_module("_testcapi")
        self.cx.execute("create table if not exists test(a number)")

        def handler(*a, **kw):
            self.cx.set_authorizer(None)
            # We expect 'int' at the C level, so this one will raise
            # when converting via PyLong_Int().
            return _testcapi.INT_MAX + 1

        self.cx.set_authorizer(handler)
        self.assert_not_authorized(self.cx.execute, "select * from test")


class ProgressTests(MemoryDatabaseMixin, unittest.TestCase):

    def assert_interrupted(self, func, /, *args, **kwargs):
        with self.assertRaisesRegex(sqlite.OperationalError, "interrupted"):
            func(*args, **kwargs)

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

    def test_set_progress_handler_keyword_args(self):
        with self.assertRaisesRegex(TypeError,
                'takes at least 1 positional argument'):
            self.con.set_progress_handler(progress_handler=lambda: None, n=1)

    # When a handler has an invalid signature, the exception raised is
    # the same that would be raised if the handler "negatively" replied.

    def test_progress_handler_invalid_signature(self):
        self.cx.execute("create table if not exists test(a number)")
        self.cx.set_progress_handler(lambda x: None, 1)
        self.assert_interrupted(self.cx.execute, "select * from test")

    # Tests for checking that callback context mutations do not crash.
    # Regression tests for https://github.com/python/cpython/issues/142830.

    @with_tracebacks(ZeroDivisionError, regex="hello world")
    def test_progress_handler_concurrent_mutation_in_call(self):
        self.cx.execute("create table if not exists test(a number)")

        def handler(*a, **kw):
            self.cx.set_progress_handler(None, 1)
            raise ZeroDivisionError("hello world")

        self.cx.set_progress_handler(handler, 1)
        self.assert_interrupted(self.cx.execute, "select * from test")

    def test_progress_handler_concurrent_mutation_in_conversion(self):
        self.cx.execute("create table if not exists test(a number)")

        class Handler:
            def __bool__(_):
                # clear the progress handler
                self.cx.set_progress_handler(None, 1)
                raise ValueError  # force PyObject_True() to fail

        self.cx.set_progress_handler(Handler.__init__, 1)
        self.assert_interrupted(self.cx.execute, "select * from test")

        # Running with tracebacks makes the second execution of this
        # function raise another exception because of a database change.


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

    def test_set_trace_callback_keyword_args(self):
        with self.assertRaisesRegex(TypeError,
                'takes no keyword arguments'):
            self.con.set_trace_callback(trace_callback=lambda: None)

    # When a handler has an invalid signature, the exception raised is
    # the same that would be raised if the handler "negatively" replied,
    # but for the trace handler, exceptions are never re-raised (only
    # printed when needed).

    @with_tracebacks(
        TypeError,
        regex=r".*<lambda>\(\) missing 6 required positional arguments",
    )
    def test_trace_handler_invalid_signature(self):
        self.cx.execute("create table if not exists test(a number)")
        self.cx.set_trace_callback(lambda x, y, z, t, a, b, c: None)
        self.cx.execute("select * from test")

    # Tests for checking that callback context mutations do not crash.
    # Regression tests for https://github.com/python/cpython/issues/142830.

    @with_tracebacks(ZeroDivisionError, regex="hello world")
    def test_trace_callback_concurrent_mutation_in_call(self):
        self.cx.execute("create table if not exists test(a number)")

        def handler(statement):
            # clear the progress handler
            self.cx.set_trace_callback(None)
            raise ZeroDivisionError("hello world")

        self.cx.set_trace_callback(handler)
        self.cx.execute("select * from test")


if __name__ == "__main__":
    unittest.main()
