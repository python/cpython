#-*- coding: ISO-8859-1 -*-
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

class RegressionTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def tearDown(self):
        self.con.close()

    def CheckPragmaUserVersion(self):
        # This used to crash pysqlite because this pragma command returns NULL for the column name
        cur = self.con.cursor()
        cur.execute("pragma user_version")

    def CheckPragmaSchemaVersion(self):
        # This still crashed pysqlite <= 2.2.1
        con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_COLNAMES)
        try:
            cur = self.con.cursor()
            cur.execute("pragma schema_version")
        finally:
            cur.close()
            con.close()

    def CheckStatementReset(self):
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

    def CheckColumnNameWithSpaces(self):
        cur = self.con.cursor()
        cur.execute('select 1 as "foo bar [datetime]"')
        self.assertEqual(cur.description[0][0], "foo bar")

        cur.execute('select 1 as "foo baz"')
        self.assertEqual(cur.description[0][0], "foo baz")

    def CheckStatementFinalizationOnCloseDb(self):
        # pysqlite versions <= 2.3.3 only finalized statements in the statement
        # cache when closing the database. statements that were still
        # referenced in cursors weren't closed an could provoke "
        # "OperationalError: Unable to close due to unfinalised statements".
        con = sqlite.connect(":memory:")
        cursors = []
        # default statement cache size is 100
        for i in range(105):
            cur = con.cursor()
            cursors.append(cur)
            cur.execute("select 1 x union select " + str(i))
        con.close()

    def CheckOnConflictRollback(self):
        if sqlite.sqlite_version_info < (3, 2, 2):
            return
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

    def CheckWorkaroundForBuggySqliteTransferBindings(self):
        """
        pysqlite would crash with older SQLite versions unless
        a workaround is implemented.
        """
        self.con.execute("create table foo(bar)")
        self.con.execute("drop table foo")
        self.con.execute("create table foo(bar)")

    def CheckEmptyStatement(self):
        """
        pysqlite used to segfault with SQLite versions 3.5.x. These return NULL
        for "no-operation" statements
        """
        self.con.execute("")

    def CheckTypeMapUsage(self):
        """
        pysqlite until 2.4.1 did not rebuild the row_cast_map when recompiling
        a statement. This test exhibits the problem.
        """
        SELECT = "select * from foo"
        con = sqlite.connect(":memory:",detect_types=sqlite.PARSE_DECLTYPES)
        con.execute("create table foo(bar timestamp)")
        con.execute("insert into foo(bar) values (?)", (datetime.datetime.now(),))
        con.execute(SELECT)
        con.execute("drop table foo")
        con.execute("create table foo(bar integer)")
        con.execute("insert into foo(bar) values (5)")
        con.execute(SELECT)

    def CheckErrorMsgDecodeError(self):
        # When porting the module to Python 3.0, the error message about
        # decoding errors disappeared. This verifies they're back again.
        failure = None
        try:
            self.con.execute("select 'xxx' || ? || 'yyy' colname",
                             (bytes(bytearray([250])),)).fetchone()
            failure = "should have raised an OperationalError with detailed description"
        except sqlite.OperationalError as e:
            msg = e.args[0]
            if not msg.startswith("Could not decode to UTF-8 column 'colname' with text 'xxx"):
                failure = "OperationalError did not have expected description text"
        if failure:
            self.fail(failure)

    def CheckRegisterAdapter(self):
        """
        See issue 3312.
        """
        self.assertRaises(TypeError, sqlite.register_adapter, {}, None)

    def CheckSetIsolationLevel(self):
        """
        See issue 3312.
        """
        con = sqlite.connect(":memory:")
        setattr(con, "isolation_level", "\xe9")

    def CheckCursorConstructorCallCheck(self):
        """
        Verifies that cursor methods check wether base class __init__ was called.
        """
        class Cursor(sqlite.Cursor):
            def __init__(self, con):
                pass

        con = sqlite.connect(":memory:")
        cur = Cursor(con)
        try:
            cur.execute("select 4+5").fetchall()
            self.fail("should have raised ProgrammingError")
        except sqlite.ProgrammingError:
            pass
        except:
            self.fail("should have raised ProgrammingError")


    def CheckStrSubclass(self):
        """
        The Python 3.0 port of the module didn't cope with values of subclasses of str.
        """
        class MyStr(str): pass
        self.con.execute("select ?", (MyStr("abc"),))

    def CheckConnectionConstructorCallCheck(self):
        """
        Verifies that connection methods check wether base class __init__ was called.
        """
        class Connection(sqlite.Connection):
            def __init__(self, name):
                pass

        con = Connection(":memory:")
        try:
            cur = con.cursor()
            self.fail("should have raised ProgrammingError")
        except sqlite.ProgrammingError:
            pass
        except:
            self.fail("should have raised ProgrammingError")

    def CheckCursorRegistration(self):
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
        try:
            cur.fetchall()
            self.fail("should have raised InterfaceError")
        except sqlite.InterfaceError:
            pass
        except:
            self.fail("should have raised InterfaceError")

    def CheckAutoCommit(self):
        """
        Verifies that creating a connection in autocommit mode works.
        2.5.3 introduced a regression so that these could no longer
        be created.
        """
        con = sqlite.connect(":memory:", isolation_level=None)

    def CheckPragmaAutocommit(self):
        """
        Verifies that running a PRAGMA statement that does an autocommit does
        work. This did not work in 2.5.3/2.5.4.
        """
        cur = self.con.cursor()
        cur.execute("create table foo(bar)")
        cur.execute("insert into foo(bar) values (5)")

        cur.execute("pragma page_size")
        row = cur.fetchone()

    def CheckSetDict(self):
        """
        See http://bugs.python.org/issue7478

        It was possible to successfully register callbacks that could not be
        hashed. Return codes of PyDict_SetItem were not checked properly.
        """
        class NotHashable:
            def __call__(self, *args, **kw):
                pass
            def __hash__(self):
                raise TypeError()
        var = NotHashable()
        self.assertRaises(TypeError, self.con.create_function, var)
        self.assertRaises(TypeError, self.con.create_aggregate, var)
        self.assertRaises(TypeError, self.con.set_authorizer, var)
        self.assertRaises(TypeError, self.con.set_progress_handler, var)

    def CheckConnectionCall(self):
        """
        Call a connection with a non-string SQL request: check error handling
        of the statement constructor.
        """
        self.assertRaises(sqlite.Warning, self.con, 1)

    def CheckCollation(self):
        def collation_cb(a, b):
            return 1
        self.assertRaises(sqlite.ProgrammingError, self.con.create_collation,
            # Lone surrogate cannot be encoded to the default encoding (utf8)
            "\uDC80", collation_cb)

    def CheckRecursiveCursorUse(self):
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

        try:
            cur.executemany("insert into b (baz) values (?)", ((i,) for i in foo()))
            self.fail("should have raised ProgrammingError")
        except sqlite.ProgrammingError:
            pass

def suite():
    regression_suite = unittest.makeSuite(RegressionTests, "Check")
    return unittest.TestSuite((regression_suite,))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
