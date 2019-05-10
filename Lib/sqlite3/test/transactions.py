#-*- coding: iso-8859-1 -*-
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

def get_db_path():
    return "sqlite_testdb"

class TransactionTests(unittest.TestCase):
    def setUp(self):
        try:
            os.remove(get_db_path())
        except OSError:
            pass

        self.con1 = sqlite.connect(get_db_path(), timeout=0.1)
        self.cur1 = self.con1.cursor()

        self.con2 = sqlite.connect(get_db_path(), timeout=0.1)
        self.cur2 = self.con2.cursor()

    def tearDown(self):
        self.cur1.close()
        self.con1.close()

        self.cur2.close()
        self.con2.close()

        try:
            os.unlink(get_db_path())
        except OSError:
            pass

    def CheckDMLDoesNotAutoCommitBefore(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.cur1.execute("create table test2(j)")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 0)

    def CheckInsertStartsTransaction(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 0)

    def CheckUpdateStartsTransaction(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.con1.commit()
        self.cur1.execute("update test set i=6")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchone()[0]
        self.assertEqual(res, 5)

    def CheckDeleteStartsTransaction(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.con1.commit()
        self.cur1.execute("delete from test")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 1)

    def CheckReplaceStartsTransaction(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        self.con1.commit()
        self.cur1.execute("replace into test(i) values (6)")
        self.cur2.execute("select i from test")
        res = self.cur2.fetchall()
        self.assertEqual(len(res), 1)
        self.assertEqual(res[0][0], 5)

    def CheckToggleAutoCommit(self):
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

    @unittest.skipIf(sqlite.sqlite_version_info < (3, 2, 2),
                     'test hangs on sqlite versions older than 3.2.2')
    def CheckRaiseTimeout(self):
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        with self.assertRaises(sqlite.OperationalError):
            self.cur2.execute("insert into test(i) values (5)")

    @unittest.skipIf(sqlite.sqlite_version_info < (3, 2, 2),
                     'test hangs on sqlite versions older than 3.2.2')
    def CheckLocking(self):
        """
        This tests the improved concurrency with pysqlite 2.3.4. You needed
        to roll back con2 before you could commit con1.
        """
        self.cur1.execute("create table test(i)")
        self.cur1.execute("insert into test(i) values (5)")
        with self.assertRaises(sqlite.OperationalError):
            self.cur2.execute("insert into test(i) values (5)")
        # NO self.con2.rollback() HERE!!!
        self.con1.commit()

    def CheckRollbackCursorConsistency(self):
        """
        Checks if cursors on the connection are set into a "reset" state
        when a rollback is done on the connection.
        """
        con = sqlite.connect(":memory:")
        cur = con.cursor()
        cur.execute("create table test(x)")
        cur.execute("insert into test(x) values (5)")
        cur.execute("select 1 union select 2 union select 3")

        con.rollback()
        with self.assertRaises(sqlite.InterfaceError):
            cur.fetchall()

class SpecialCommandTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.cur = self.con.cursor()

    def CheckDropTable(self):
        self.cur.execute("create table test(i)")
        self.cur.execute("insert into test(i) values (5)")
        self.cur.execute("drop table test")

    def CheckPragma(self):
        self.cur.execute("create table test(i)")
        self.cur.execute("insert into test(i) values (5)")
        self.cur.execute("pragma count_changes=1")

    def tearDown(self):
        self.cur.close()
        self.con.close()

class TransactionalDDL(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def CheckDdlDoesNotAutostartTransaction(self):
        # For backwards compatibility reasons, DDL statements should not
        # implicitly start a transaction.
        self.con.execute("create table test(i)")
        self.con.rollback()
        result = self.con.execute("select * from test").fetchall()
        self.assertEqual(result, [])

        self.con.execute("alter table test rename to test2")
        self.con.rollback()
        result = self.con.execute("select * from test2").fetchall()
        self.assertEqual(result, [])

    def CheckImmediateTransactionalDDL(self):
        # You can achieve transactional DDL by issuing a BEGIN
        # statement manually.
        self.con.execute("begin immediate")
        self.con.execute("create table test(i)")
        self.con.rollback()
        with self.assertRaises(sqlite.OperationalError):
            self.con.execute("select * from test")

    def CheckTransactionalDDL(self):
        # You can achieve transactional DDL by issuing a BEGIN
        # statement manually.
        self.con.execute("begin")
        self.con.execute("create table test(i)")
        self.con.rollback()
        with self.assertRaises(sqlite.OperationalError):
            self.con.execute("select * from test")

    def tearDown(self):
        self.con.close()


class DMLStatementDetectionTestCase(unittest.TestCase):
    """
    Test behavior of sqlite3_stmt_readonly() in determining if a statement is
    DML or not.
    """
    @unittest.skipIf(sqlite.sqlite_version_info < (3, 8, 3), 'needs sqlite 3.8.3 or newer')
    def test_dml_detection_cte(self):
        conn = sqlite.connect(':memory:')
        conn.execute('CREATE TABLE kv ("key" TEXT, "val" INTEGER)')
        self.assertFalse(conn.in_transaction)
        conn.execute('INSERT INTO kv (key, val) VALUES (?, ?), (?, ?)',
                     ('k1', 1, 'k2', 2))
        self.assertTrue(conn.in_transaction)
        conn.commit()
        self.assertFalse(conn.in_transaction)

        rc = conn.execute('UPDATE kv SET val=val + ?', (10,))
        self.assertEqual(rc.rowcount, 2)
        self.assertTrue(conn.in_transaction)
        conn.commit()
        self.assertFalse(conn.in_transaction)

        rc = conn.execute(
            'WITH c(k, v) AS (SELECT key, val + ? FROM kv) '
            'UPDATE kv SET val=(SELECT v FROM c WHERE k=kv.key)',
            (100,)
        )
        self.assertEqual(rc.rowcount, 2)
        self.assertTrue(conn.in_transaction)

        curs = conn.execute('SELECT key, val FROM kv ORDER BY key')
        self.assertEqual(curs.fetchall(), [('k1', 111), ('k2', 112)])

    @unittest.skipIf(sqlite.sqlite_version_info < (3, 7, 11), 'needs sqlite 3.7.11 or newer')
    def test_dml_detection_sql_comment(self):
        conn = sqlite.connect(':memory:')
        conn.execute('CREATE TABLE kv ("key" TEXT, "val" INTEGER)')
        self.assertFalse(conn.in_transaction)
        conn.execute('INSERT INTO kv (key, val) VALUES (?, ?), (?, ?)',
                     ('k1', 1, 'k2', 2))
        conn.commit()
        self.assertFalse(conn.in_transaction)

        rc = conn.execute('-- a comment\nUPDATE kv SET val=val + ?', (10,))
        self.assertEqual(rc.rowcount, 2)
        self.assertTrue(conn.in_transaction)

        curs = conn.execute('SELECT key, val FROM kv ORDER BY key')
        self.assertEqual(curs.fetchall(), [('k1', 11), ('k2', 12)])
        conn.rollback()
        self.assertFalse(conn.in_transaction)
        # Fetch again after rollback.
        curs = conn.execute('SELECT key, val FROM kv ORDER BY key')
        self.assertEqual(curs.fetchall(), [('k1', 1), ('k2', 2)])

    def test_dml_detection_begin_exclusive(self):
        # sqlite3_stmt_readonly() reports BEGIN EXCLUSIVE as being a
        # non-read-only statement. To retain compatibility with the
        # transactional behavior, we add a special exclusion for these
        # statements.
        conn = sqlite.connect(':memory:')
        conn.execute('BEGIN EXCLUSIVE')
        self.assertTrue(conn.in_transaction)
        conn.execute('ROLLBACK')
        self.assertFalse(conn.in_transaction)

    def test_dml_detection_vacuum(self):
        conn = sqlite.connect(':memory:')
        conn.execute('vacuum')
        self.assertFalse(conn.in_transaction)


def suite():
    default_suite = unittest.makeSuite(TransactionTests, "Check")
    special_command_suite = unittest.makeSuite(SpecialCommandTests, "Check")
    ddl_suite = unittest.makeSuite(TransactionalDDL, "Check")
    dml_suite = unittest.makeSuite(DMLStatementDetectionTestCase)
    return unittest.TestSuite((default_suite, special_command_suite, ddl_suite, dml_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
