#-*- coding: iso-8859-1 -*-
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

import unittest
import sqlite3 as sqlite

from test.support import TESTFN, unlink

class CollationTests(unittest.TestCase):
    def CheckCreateCollationNotString(self):
        con = sqlite.connect(":memory:")
        with self.assertRaises(TypeError):
            con.create_collation(None, lambda x, y: (x > y) - (x < y))

    def CheckCreateCollationNotCallable(self):
        con = sqlite.connect(":memory:")
        with self.assertRaises(TypeError) as cm:
            con.create_collation("X", 42)
        self.assertEqual(str(cm.exception), 'parameter must be callable')

    def CheckCreateCollationNotAscii(self):
        con = sqlite.connect(":memory:")
        with self.assertRaises(sqlite.ProgrammingError):
            con.create_collation("collä", lambda x, y: (x > y) - (x < y))

    def CheckCreateCollationBadUpper(self):
        class BadUpperStr(str):
            def upper(self):
                return None
        con = sqlite.connect(":memory:")
        mycoll = lambda x, y: -((x > y) - (x < y))
        con.create_collation(BadUpperStr("mycoll"), mycoll)
        result = con.execute("""
            select x from (
            select 'a' as x
            union
            select 'b' as x
            ) order by x collate mycoll
            """).fetchall()
        self.assertEqual(result[0][0], 'b')
        self.assertEqual(result[1][0], 'a')

    @unittest.skipIf(sqlite.sqlite_version_info < (3, 2, 1),
                     'old SQLite versions crash on this test')
    def CheckCollationIsUsed(self):
        def mycoll(x, y):
            # reverse order
            return -((x > y) - (x < y))

        con = sqlite.connect(":memory:")
        con.create_collation("mycoll", mycoll)
        sql = """
            select x from (
            select 'a' as x
            union
            select 'b' as x
            union
            select 'c' as x
            ) order by x collate mycoll
            """
        result = con.execute(sql).fetchall()
        self.assertEqual(result, [('c',), ('b',), ('a',)],
                         msg='the expected order was not returned')

        con.create_collation("mycoll", None)
        with self.assertRaises(sqlite.OperationalError) as cm:
            result = con.execute(sql).fetchall()
        self.assertEqual(str(cm.exception), 'no such collation sequence: mycoll')

    def CheckCollationReturnsLargeInteger(self):
        def mycoll(x, y):
            # reverse order
            return -((x > y) - (x < y)) * 2**32
        con = sqlite.connect(":memory:")
        con.create_collation("mycoll", mycoll)
        sql = """
            select x from (
            select 'a' as x
            union
            select 'b' as x
            union
            select 'c' as x
            ) order by x collate mycoll
            """
        result = con.execute(sql).fetchall()
        self.assertEqual(result, [('c',), ('b',), ('a',)],
                         msg="the expected order was not returned")

    def CheckCollationRegisterTwice(self):
        """
        Register two different collation functions under the same name.
        Verify that the last one is actually used.
        """
        con = sqlite.connect(":memory:")
        con.create_collation("mycoll", lambda x, y: (x > y) - (x < y))
        con.create_collation("mycoll", lambda x, y: -((x > y) - (x < y)))
        result = con.execute("""
            select x from (select 'a' as x union select 'b' as x) order by x collate mycoll
            """).fetchall()
        self.assertEqual(result[0][0], 'b')
        self.assertEqual(result[1][0], 'a')

    def CheckDeregisterCollation(self):
        """
        Register a collation, then deregister it. Make sure an error is raised if we try
        to use it.
        """
        con = sqlite.connect(":memory:")
        con.create_collation("mycoll", lambda x, y: (x > y) - (x < y))
        con.create_collation("mycoll", None)
        with self.assertRaises(sqlite.OperationalError) as cm:
            con.execute("select 'a' as x union select 'b' as x order by x collate mycoll")
        self.assertEqual(str(cm.exception), 'no such collation sequence: mycoll')

class ProgressTests(unittest.TestCase):
    def CheckProgressHandlerUsed(self):
        """
        Test that the progress handler is invoked once it is set.
        """
        con = sqlite.connect(":memory:")
        progress_calls = []
        def progress():
            progress_calls.append(None)
            return 0
        con.set_progress_handler(progress, 1)
        con.execute("""
            create table foo(a, b)
            """)
        self.assertTrue(progress_calls)


    def CheckOpcodeCount(self):
        """
        Test that the opcode argument is respected.
        """
        con = sqlite.connect(":memory:")
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

    def CheckCancelOperation(self):
        """
        Test that returning a non-zero value stops the operation in progress.
        """
        con = sqlite.connect(":memory:")
        def progress():
            return 1
        con.set_progress_handler(progress, 1)
        curs = con.cursor()
        self.assertRaises(
            sqlite.OperationalError,
            curs.execute,
            "create table bar (a, b)")

    def CheckClearHandler(self):
        """
        Test that setting the progress handler to None clears the previously set handler.
        """
        con = sqlite.connect(":memory:")
        action = 0
        def progress():
            nonlocal action
            action = 1
            return 0
        con.set_progress_handler(progress, 1)
        con.set_progress_handler(None, 1)
        con.execute("select 1 union select 2 union select 3").fetchall()
        self.assertEqual(action, 0, "progress handler was not cleared")

class TraceCallbackTests(unittest.TestCase):
    def CheckTraceCallbackUsed(self):
        """
        Test that the trace callback is invoked once it is set.
        """
        con = sqlite.connect(":memory:")
        traced_statements = []
        def trace(statement):
            traced_statements.append(statement)
        con.set_trace_callback(trace)
        con.execute("create table foo(a, b)")
        self.assertTrue(traced_statements)
        self.assertTrue(any("create table foo" in stmt for stmt in traced_statements))

    def CheckClearTraceCallback(self):
        """
        Test that setting the trace callback to None clears the previously set callback.
        """
        con = sqlite.connect(":memory:")
        traced_statements = []
        def trace(statement):
            traced_statements.append(statement)
        con.set_trace_callback(trace)
        con.set_trace_callback(None)
        con.execute("create table foo(a, b)")
        self.assertFalse(traced_statements, "trace callback was not cleared")

    def CheckUnicodeContent(self):
        """
        Test that the statement can contain unicode literals.
        """
        unicode_value = '\xf6\xe4\xfc\xd6\xc4\xdc\xdf\u20ac'
        con = sqlite.connect(":memory:")
        traced_statements = []
        def trace(statement):
            traced_statements.append(statement)
        con.set_trace_callback(trace)
        con.execute("create table foo(x)")
        # Can't execute bound parameters as their values don't appear
        # in traced statements before SQLite 3.6.21
        # (cf. http://www.sqlite.org/draft/releaselog/3_6_21.html)
        con.execute("insert into foo(x) values ('%s')" % unicode_value)
        con.commit()
        self.assertTrue(any(unicode_value in stmt for stmt in traced_statements),
                        "Unicode data %s garbled in trace callback: %s"
                        % (ascii(unicode_value), ', '.join(map(ascii, traced_statements))))

    @unittest.skipIf(sqlite.sqlite_version_info < (3, 3, 9), "sqlite3_prepare_v2 is not available")
    def CheckTraceCallbackContent(self):
        # set_trace_callback() shouldn't produce duplicate content (bpo-26187)
        traced_statements = []
        def trace(statement):
            traced_statements.append(statement)

        queries = ["create table foo(x)",
                   "insert into foo(x) values(1)"]
        self.addCleanup(unlink, TESTFN)
        con1 = sqlite.connect(TESTFN, isolation_level=None)
        con2 = sqlite.connect(TESTFN)
        con1.set_trace_callback(trace)
        cur = con1.cursor()
        cur.execute(queries[0])
        con2.execute("create table bar(x)")
        cur.execute(queries[1])

        # Extract from SQLite 3.7.15 changelog:
        # Avoid invoking the sqlite3_trace() callback multiple times when a
        # statement is automatically reprepared due to SQLITE_SCHEMA errors.
        #
        # See bpo-40810
        if sqlite.sqlite_version_info < (3, 7, 15):
            queries.append(queries[-1])
        self.assertEqual(traced_statements, queries)


def suite():
    collation_suite = unittest.makeSuite(CollationTests, "Check")
    progress_suite = unittest.makeSuite(ProgressTests, "Check")
    trace_suite = unittest.makeSuite(TraceCallbackTests, "Check")
    return unittest.TestSuite((collation_suite, progress_suite, trace_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
