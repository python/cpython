# pysqlite2/test/transactions.py: tests transactions
#
# Copyright (C) 2005-2007 Gerhard Häring <gh@ghaering.de>
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

import os, unittest
import sqlite3 as sqlite

from test.support.os_helper import TESTFN, unlink

from test.test_sqlite3.test_dbapi import memory_database


class TransactionTests(unittest.TestCase):
    def setUp(self):
        # We can disable the busy handlers, since we control
        # the order of SQLite C API operations.
        self.con1 = sqlite.connect(TESTFN, timeout=0)
        self.cur1 = self.con1.cursor()

        self.con2 = sqlite.connect(TESTFN, timeout=0)
        self.cur2 = self.con2.cursor()

    def tearDown(self):
        try:
            self.cur1.close()
            self.con1.close()

            self.cur2.close()
            self.con2.close()

        finally:
            unlink(TESTFN)

    def test_dml_does_not_auto_commit_before(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.cur1.execute("create table test2(j)")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 0)

    def test_insert_starts_transaction(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 0)

    def test_update_starts_transaction(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.con1.commit()
        self.cur1.execute("update test set i=6")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchone()[0]
        self.assertEqual(res, 5)

    def test_delete_starts_transaction(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.con1.commit()
        self.cur1.execute("delete from test")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 1)

    def test_replace_starts_transaction(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.con1.commit()
        self.cur1.execute("replace into test(i) values (6)")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 1)
        self.assertEqual(res[0][0], 5)

    def test_toggle_auto_commit(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.con1.isolation_level = None
        self.assertEqual(self.con1.isolation_level, None)
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 1)

        self.con1.isolation_level = "DEFERRED"
        self.assertEqual(self.con1.isolation_level , "DEFERRED")
        self.cur1.execute("insert into test(i) values (5)")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 1)

    def test_raise_timeout(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        with self.assertRaises(sqlite.OperationalError):
            self.cur2.execute("insert into test(i) values (5)")

    def test_locking(self):
        # This tests the improved concurrency with pysqlite 2.3.4. You needed
        # to roll back con2 before you could commit con1.
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        with self.assertRaises(sqlite.OperationalError):
            self.cur2.execute("insert into test(i) values (5)")
        # NO self.con2.rollback() HERE!!!
        self.con1.commit()

    def test_rollback_cursor_consistency(self):
        """Check that cursors behave correctly after rollback."""
        con = sqlite.connect(":memory:")
        cur = con.cursor()
        cur.execute("create table test(x)")
        cur.execute("insert into test(x) values (5)")
        cur.execute("select 1 union select 2 union select 3")

        con.rollback()
        self.assertEqual(cur.fetchall(), [(1,), (2,), (3,)])

    def test_multiple_cursors_and_iternext(self):
        # gh-94028: statements are cleared and reset in cursor iternext.

        # Provoke the gh-94028 by using a cursor cache.
        CURSORS = {}
        def sql(cx, sql, *args):
            cu = cx.cursor()
            cu.execute(sql, args)
            CURSORS[id(sql)] = cu
            return cu

        self.con1.execute("create table t(t)")
        sql(self.con1, "insert into t values (?), (?), (?)", "u1", "u2", "u3")
        self.con1.commit()

        # On second connection, verify rows are visible, then delete them.
        count = sql(self.con2, "select count(*) from t").fetchone()[0]
        self.assertEqual(count, 3)
        changes = sql(self.con2, "delete from t").rowcount
        self.assertEqual(changes, 3)
        self.con2.commit()

        # Back in original connection, create 2 new users.
        sql(self.con1, "insert into t values (?)", "u4")
        sql(self.con1, "insert into t values (?)", "u5")

        # The second connection cannot see uncommitted changes.
        count = sql(self.con2, "select count(*) from t").fetchone()[0]
        self.assertEqual(count, 0)

        # First connection can see its own changes.
        count = sql(self.con1, "select count(*) from t").fetchone()[0]
        self.assertEqual(count, 2)

        # The second connection can now see the changes.
        self.con1.commit()
        count = sql(self.con2, "select count(*) from t").fetchone()[0]
        self.assertEqual(count, 2)


class RollbackTests(unittest.TestCase):
    """bpo-44092: sqlite3 now leaves it to SQLite to resolve rollback issues"""

    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.cur1 = self.con.cursor()
        self.cur2 = self.con.cursor()
        with self.con:
            self.con.execute("create table t(c)");
            self.con.executemany("insert into t values(?)", [(0,), (1,), (2,)])
        self.cur1.execute("begin transaction")
        select = "select c from t"
        self.cur1.execute(select)
        self.con.rollback()
        self.res = self.cur2.execute(select)  # Reusing stmt from cache

    def tearDown(self):
        self.con.close()

    def _check_rows(self):
        for i, row in enumerate(self.res):
            self.assertEqual(row[0], i)

    def test_no_duplicate_rows_after_rollback_del_cursor(self):
        del self.cur1
        self._check_rows()

    def test_no_duplicate_rows_after_rollback_close_cursor(self):
        self.cur1.close()
        self._check_rows()

    def test_no_duplicate_rows_after_rollback_new_query(self):
        self.cur1.execute("select c from t where c = 1")
        self._check_rows()



class SpecialCommandTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.cur = self.con.cursor()

    def test_drop_table(self):
        self.cur.execute("create table test(i)")
        self.cur.execute("insert into test(i) values (5)")
        self.cur.execute("drop table test")

    def test_pragma(self):
        self.cur.execute("create table test(i)")
        self.cur.execute("insert into test(i) values (5)")
        self.cur.execute("pragma count_changes=1")

    def tearDown(self):
        self.cur.close()
        self.con.close()


class TransactionalDDL(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def test_ddl_does_not_autostart_transaction(self):
        # For backwards compatibility reasons, DDL statements should not
        # implicitly start a transaction.
        self.con.execute("create table test(i)")
        self.con.rollback()
        result = self.con.execute("select * from test").fetchall()
        self.assertEqual(result, [])

    def test_immediate_transactional_ddl(self):
        # You can achieve transactional DDL by issuing a BEGIN
        # statement manually.
        self.con.execute("begin immediate")
        self.con.execute("create table test(i)")
        self.con.rollback()
        with self.assertRaises(sqlite.OperationalError):
            self.con.execute("select * from test")

    def test_transactional_ddl(self):
        # You can achieve transactional DDL by issuing a BEGIN
        # statement manually.
        self.con.execute("begin")
        self.con.execute("create table test(i)")
        self.con.rollback()
        with self.assertRaises(sqlite.OperationalError):
            self.con.execute("select * from test")

    def tearDown(self):
        self.con.close()


class IsolationLevelFromInit(unittest.TestCase):
    CREATE = "create table t(t)"
    INSERT = "insert into t values(1)"

    def setUp(self):
        self.traced = []

    def _run_test(self, cx):
        cx.execute(self.CREATE)
        cx.set_trace_callback(lambda stmt: self.traced.append(stmt))
        with cx:
            cx.execute(self.INSERT)

    def test_isolation_level_default(self):
        with memory_database() as cx:
            self._run_test(cx)
            self.assertEqual(self.traced, ["BEGIN ", self.INSERT, "COMMIT"])

    def test_isolation_level_begin(self):
        with memory_database(isolation_level="") as cx:
            self._run_test(cx)
            self.assertEqual(self.traced, ["BEGIN ", self.INSERT, "COMMIT"])

    def test_isolation_level_deferred(self):
        with memory_database(isolation_level="DEFERRED") as cx:
            self._run_test(cx)
            self.assertEqual(self.traced, ["BEGIN DEFERRED", self.INSERT, "COMMIT"])

    def test_isolation_level_immediate(self):
        with memory_database(isolation_level="IMMEDIATE") as cx:
            self._run_test(cx)
            self.assertEqual(self.traced,
                             ["BEGIN IMMEDIATE", self.INSERT, "COMMIT"])

    def test_isolation_level_exclusive(self):
        with memory_database(isolation_level="EXCLUSIVE") as cx:
            self._run_test(cx)
            self.assertEqual(self.traced,
                             ["BEGIN EXCLUSIVE", self.INSERT, "COMMIT"])

    def test_isolation_level_none(self):
        with memory_database(isolation_level=None) as cx:
            self._run_test(cx)
            self.assertEqual(self.traced, [self.INSERT])


class IsolationLevelPostInit(unittest.TestCase):
    QUERY = "insert into t values(1)"

    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cx.execute("create table t(t)")
        self.traced = []
        self.cx.set_trace_callback(lambda stmt: self.traced.append(stmt))

    def tearDown(self):
        self.cx.close()

    def test_isolation_level_default(self):
        with self.cx:
            self.cx.execute(self.QUERY)
        self.assertEqual(self.traced, ["BEGIN ", self.QUERY, "COMMIT"])

    def test_isolation_level_begin(self):
        self.cx.isolation_level = ""
        with self.cx:
            self.cx.execute(self.QUERY)
        self.assertEqual(self.traced, ["BEGIN ", self.QUERY, "COMMIT"])

    def test_isolation_level_deferrred(self):
        self.cx.isolation_level = "DEFERRED"
        with self.cx:
            self.cx.execute(self.QUERY)
        self.assertEqual(self.traced, ["BEGIN DEFERRED", self.QUERY, "COMMIT"])

    def test_isolation_level_immediate(self):
        self.cx.isolation_level = "IMMEDIATE"
        with self.cx:
            self.cx.execute(self.QUERY)
        self.assertEqual(self.traced,
                         ["BEGIN IMMEDIATE", self.QUERY, "COMMIT"])

    def test_isolation_level_exclusive(self):
        self.cx.isolation_level = "EXCLUSIVE"
        with self.cx:
            self.cx.execute(self.QUERY)
        self.assertEqual(self.traced,
                         ["BEGIN EXCLUSIVE", self.QUERY, "COMMIT"])

    def test_isolation_level_none(self):
        self.cx.isolation_level = None
        with self.cx:
            self.cx.execute(self.QUERY)
        self.assertEqual(self.traced, [self.QUERY])


if __name__ == "__main__":
    unittest.main()
