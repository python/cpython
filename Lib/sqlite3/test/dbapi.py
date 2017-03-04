#-*- coding: iso-8859-1 -*-
# pysqlite2/test/dbapi.py: tests for DB-API compliance
#
# Copyright (C) 2004-2010 Gerhard Häring <gh@ghaering.de>
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

import threading
import unittest
import sqlite3 as sqlite

from test.support import TESTFN, unlink


class ModuleTests(unittest.TestCase):
    def CheckAPILevel(self):
        self.assertEqual(sqlite.apilevel, "2.0",
                         "apilevel is %s, should be 2.0" % sqlite.apilevel)

    def CheckThreadSafety(self):
        self.assertEqual(sqlite.threadsafety, 1,
                         "threadsafety is %d, should be 1" % sqlite.threadsafety)

    def CheckParamStyle(self):
        self.assertEqual(sqlite.paramstyle, "qmark",
                         "paramstyle is '%s', should be 'qmark'" %
                         sqlite.paramstyle)

    def CheckWarning(self):
        self.assertTrue(issubclass(sqlite.Warning, Exception),
                     "Warning is not a subclass of Exception")

    def CheckError(self):
        self.assertTrue(issubclass(sqlite.Error, Exception),
                        "Error is not a subclass of Exception")

    def CheckInterfaceError(self):
        self.assertTrue(issubclass(sqlite.InterfaceError, sqlite.Error),
                        "InterfaceError is not a subclass of Error")

    def CheckDatabaseError(self):
        self.assertTrue(issubclass(sqlite.DatabaseError, sqlite.Error),
                        "DatabaseError is not a subclass of Error")

    def CheckDataError(self):
        self.assertTrue(issubclass(sqlite.DataError, sqlite.DatabaseError),
                        "DataError is not a subclass of DatabaseError")

    def CheckOperationalError(self):
        self.assertTrue(issubclass(sqlite.OperationalError, sqlite.DatabaseError),
                        "OperationalError is not a subclass of DatabaseError")

    def CheckIntegrityError(self):
        self.assertTrue(issubclass(sqlite.IntegrityError, sqlite.DatabaseError),
                        "IntegrityError is not a subclass of DatabaseError")

    def CheckInternalError(self):
        self.assertTrue(issubclass(sqlite.InternalError, sqlite.DatabaseError),
                        "InternalError is not a subclass of DatabaseError")

    def CheckProgrammingError(self):
        self.assertTrue(issubclass(sqlite.ProgrammingError, sqlite.DatabaseError),
                        "ProgrammingError is not a subclass of DatabaseError")

    def CheckNotSupportedError(self):
        self.assertTrue(issubclass(sqlite.NotSupportedError,
                                   sqlite.DatabaseError),
                        "NotSupportedError is not a subclass of DatabaseError")

class ConnectionTests(unittest.TestCase):

    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        cu = self.cx.cursor()
        cu.execute("create table test(id integer primary key, name text)")
        cu.execute("insert into test(name) values (?)", ("foo",))

    def tearDown(self):
        self.cx.close()

    def CheckCommit(self):
        self.cx.commit()

    def CheckCommitAfterNoChanges(self):
        """
        A commit should also work when no changes were made to the database.
        """
        self.cx.commit()
        self.cx.commit()

    def CheckRollback(self):
        self.cx.rollback()

    def CheckRollbackAfterNoChanges(self):
        """
        A rollback should also work when no changes were made to the database.
        """
        self.cx.rollback()
        self.cx.rollback()

    def CheckCursor(self):
        cu = self.cx.cursor()

    def CheckFailedOpen(self):
        YOU_CANNOT_OPEN_THIS = "/foo/bar/bla/23534/mydb.db"
        with self.assertRaises(sqlite.OperationalError):
            con = sqlite.connect(YOU_CANNOT_OPEN_THIS)

    def CheckClose(self):
        self.cx.close()

    def CheckExceptions(self):
        # Optional DB-API extension.
        self.assertEqual(self.cx.Warning, sqlite.Warning)
        self.assertEqual(self.cx.Error, sqlite.Error)
        self.assertEqual(self.cx.InterfaceError, sqlite.InterfaceError)
        self.assertEqual(self.cx.DatabaseError, sqlite.DatabaseError)
        self.assertEqual(self.cx.DataError, sqlite.DataError)
        self.assertEqual(self.cx.OperationalError, sqlite.OperationalError)
        self.assertEqual(self.cx.IntegrityError, sqlite.IntegrityError)
        self.assertEqual(self.cx.InternalError, sqlite.InternalError)
        self.assertEqual(self.cx.ProgrammingError, sqlite.ProgrammingError)
        self.assertEqual(self.cx.NotSupportedError, sqlite.NotSupportedError)

    def CheckInTransaction(self):
        # Can't use db from setUp because we want to test initial state.
        cx = sqlite.connect(":memory:")
        cu = cx.cursor()
        self.assertEqual(cx.in_transaction, False)
        cu.execute("create table transactiontest(id integer primary key, name text)")
        self.assertEqual(cx.in_transaction, False)
        cu.execute("insert into transactiontest(name) values (?)", ("foo",))
        self.assertEqual(cx.in_transaction, True)
        cu.execute("select name from transactiontest where name=?", ["foo"])
        row = cu.fetchone()
        self.assertEqual(cx.in_transaction, True)
        cx.commit()
        self.assertEqual(cx.in_transaction, False)
        cu.execute("select name from transactiontest where name=?", ["foo"])
        row = cu.fetchone()
        self.assertEqual(cx.in_transaction, False)

    def CheckInTransactionRO(self):
        with self.assertRaises(AttributeError):
            self.cx.in_transaction = True

    def CheckOpenWithPathLikeObject(self):
        """ Checks that we can successfully connect to a database using an object that
            is PathLike, i.e. has __fspath__(). """
        self.addCleanup(unlink, TESTFN)
        class Path:
            def __fspath__(self):
                return TESTFN
        path = Path()
        with sqlite.connect(path) as cx:
            cx.execute('create table test(id integer)')

    def CheckOpenUri(self):
        if sqlite.sqlite_version_info < (3, 7, 7):
            with self.assertRaises(sqlite.NotSupportedError):
                sqlite.connect(':memory:', uri=True)
            return
        self.addCleanup(unlink, TESTFN)
        with sqlite.connect(TESTFN) as cx:
            cx.execute('create table test(id integer)')
        with sqlite.connect('file:' + TESTFN, uri=True) as cx:
            cx.execute('insert into test(id) values(0)')
        with sqlite.connect('file:' + TESTFN + '?mode=ro', uri=True) as cx:
            with self.assertRaises(sqlite.OperationalError):
                cx.execute('insert into test(id) values(1)')

    @unittest.skipIf(sqlite.sqlite_version_info >= (3, 3, 1),
                     'needs sqlite versions older than 3.3.1')
    def CheckSameThreadErrorOnOldVersion(self):
        with self.assertRaises(sqlite.NotSupportedError) as cm:
            sqlite.connect(':memory:', check_same_thread=False)
        self.assertEqual(str(cm.exception), 'shared connections not available')

class CursorTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cu = self.cx.cursor()
        self.cu.execute(
            "create table test(id integer primary key, name text, "
            "income number, unique_test text unique)"
        )
        self.cu.execute("insert into test(name) values (?)", ("foo",))

    def tearDown(self):
        self.cu.close()
        self.cx.close()

    def CheckExecuteNoArgs(self):
        self.cu.execute("delete from test")

    def CheckExecuteIllegalSql(self):
        with self.assertRaises(sqlite.OperationalError):
            self.cu.execute("select asdf")

    def CheckExecuteTooMuchSql(self):
        with self.assertRaises(sqlite.Warning):
            self.cu.execute("select 5+4; select 4+5")

    def CheckExecuteTooMuchSql2(self):
        self.cu.execute("select 5+4; -- foo bar")

    def CheckExecuteTooMuchSql3(self):
        self.cu.execute("""
            select 5+4;

            /*
            foo
            */
            """)

    def CheckExecuteWrongSqlArg(self):
        with self.assertRaises(TypeError):
            self.cu.execute(42)

    def CheckExecuteArgInt(self):
        self.cu.execute("insert into test(id) values (?)", (42,))

    def CheckExecuteArgFloat(self):
        self.cu.execute("insert into test(income) values (?)", (2500.32,))

    def CheckExecuteArgString(self):
        self.cu.execute("insert into test(name) values (?)", ("Hugo",))

    def CheckExecuteArgStringWithZeroByte(self):
        self.cu.execute("insert into test(name) values (?)", ("Hu\x00go",))

        self.cu.execute("select name from test where id=?", (self.cu.lastrowid,))
        row = self.cu.fetchone()
        self.assertEqual(row[0], "Hu\x00go")

    def CheckExecuteNonIterable(self):
        with self.assertRaises(ValueError) as cm:
            self.cu.execute("insert into test(id) values (?)", 42)
        self.assertEqual(str(cm.exception), 'parameters are of unsupported type')

    def CheckExecuteWrongNoOfArgs1(self):
        # too many parameters
        with self.assertRaises(sqlite.ProgrammingError):
            self.cu.execute("insert into test(id) values (?)", (17, "Egon"))

    def CheckExecuteWrongNoOfArgs2(self):
        # too little parameters
        with self.assertRaises(sqlite.ProgrammingError):
            self.cu.execute("insert into test(id) values (?)")

    def CheckExecuteWrongNoOfArgs3(self):
        # no parameters, parameters are needed
        with self.assertRaises(sqlite.ProgrammingError):
            self.cu.execute("insert into test(id) values (?)")

    def CheckExecuteParamList(self):
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("select name from test where name=?", ["foo"])
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")

    def CheckExecuteParamSequence(self):
        class L(object):
            def __len__(self):
                return 1
            def __getitem__(self, x):
                assert x == 0
                return "foo"

        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("select name from test where name=?", L())
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")

    def CheckExecuteDictMapping(self):
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("select name from test where name=:name", {"name": "foo"})
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")

    def CheckExecuteDictMapping_Mapping(self):
        class D(dict):
            def __missing__(self, key):
                return "foo"

        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("select name from test where name=:name", D())
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")

    def CheckExecuteDictMappingTooLittleArgs(self):
        self.cu.execute("insert into test(name) values ('foo')")
        with self.assertRaises(sqlite.ProgrammingError):
            self.cu.execute("select name from test where name=:name and id=:id", {"name": "foo"})

    def CheckExecuteDictMappingNoArgs(self):
        self.cu.execute("insert into test(name) values ('foo')")
        with self.assertRaises(sqlite.ProgrammingError):
            self.cu.execute("select name from test where name=:name")

    def CheckExecuteDictMappingUnnamed(self):
        self.cu.execute("insert into test(name) values ('foo')")
        with self.assertRaises(sqlite.ProgrammingError):
            self.cu.execute("select name from test where name=?", {"name": "foo"})

    def CheckClose(self):
        self.cu.close()

    def CheckRowcountExecute(self):
        self.cu.execute("delete from test")
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("update test set name='bar'")
        self.assertEqual(self.cu.rowcount, 2)

    def CheckRowcountSelect(self):
        """
        pysqlite does not know the rowcount of SELECT statements, because we
        don't fetch all rows after executing the select statement. The rowcount
        has thus to be -1.
        """
        self.cu.execute("select 5 union select 6")
        self.assertEqual(self.cu.rowcount, -1)

    def CheckRowcountExecutemany(self):
        self.cu.execute("delete from test")
        self.cu.executemany("insert into test(name) values (?)", [(1,), (2,), (3,)])
        self.assertEqual(self.cu.rowcount, 3)

    def CheckTotalChanges(self):
        self.cu.execute("insert into test(name) values ('foo')")
        self.cu.execute("insert into test(name) values ('foo')")
        self.assertLess(2, self.cx.total_changes, msg='total changes reported wrong value')

    # Checks for executemany:
    # Sequences are required by the DB-API, iterators
    # enhancements in pysqlite.

    def CheckExecuteManySequence(self):
        self.cu.executemany("insert into test(income) values (?)", [(x,) for x in range(100, 110)])

    def CheckExecuteManyIterator(self):
        class MyIter:
            def __init__(self):
                self.value = 5

            def __next__(self):
                if self.value == 10:
                    raise StopIteration
                else:
                    self.value += 1
                    return (self.value,)

        self.cu.executemany("insert into test(income) values (?)", MyIter())

    def CheckExecuteManyGenerator(self):
        def mygen():
            for i in range(5):
                yield (i,)

        self.cu.executemany("insert into test(income) values (?)", mygen())

    def CheckExecuteManyWrongSqlArg(self):
        with self.assertRaises(TypeError):
            self.cu.executemany(42, [(3,)])

    def CheckExecuteManySelect(self):
        with self.assertRaises(sqlite.ProgrammingError):
            self.cu.executemany("select ?", [(3,)])

    def CheckExecuteManyNotIterable(self):
        with self.assertRaises(TypeError):
            self.cu.executemany("insert into test(income) values (?)", 42)

    def CheckFetchIter(self):
        # Optional DB-API extension.
        self.cu.execute("delete from test")
        self.cu.execute("insert into test(id) values (?)", (5,))
        self.cu.execute("insert into test(id) values (?)", (6,))
        self.cu.execute("select id from test order by id")
        lst = []
        for row in self.cu:
            lst.append(row[0])
        self.assertEqual(lst[0], 5)
        self.assertEqual(lst[1], 6)

    def CheckFetchone(self):
        self.cu.execute("select name from test")
        row = self.cu.fetchone()
        self.assertEqual(row[0], "foo")
        row = self.cu.fetchone()
        self.assertEqual(row, None)

    def CheckFetchoneNoStatement(self):
        cur = self.cx.cursor()
        row = cur.fetchone()
        self.assertEqual(row, None)

    def CheckArraySize(self):
        # must default ot 1
        self.assertEqual(self.cu.arraysize, 1)

        # now set to 2
        self.cu.arraysize = 2

        # now make the query return 3 rows
        self.cu.execute("delete from test")
        self.cu.execute("insert into test(name) values ('A')")
        self.cu.execute("insert into test(name) values ('B')")
        self.cu.execute("insert into test(name) values ('C')")
        self.cu.execute("select name from test")
        res = self.cu.fetchmany()

        self.assertEqual(len(res), 2)

    def CheckFetchmany(self):
        self.cu.execute("select name from test")
        res = self.cu.fetchmany(100)
        self.assertEqual(len(res), 1)
        res = self.cu.fetchmany(100)
        self.assertEqual(res, [])

    def CheckFetchmanyKwArg(self):
        """Checks if fetchmany works with keyword arguments"""
        self.cu.execute("select name from test")
        res = self.cu.fetchmany(size=100)
        self.assertEqual(len(res), 1)

    def CheckFetchall(self):
        self.cu.execute("select name from test")
        res = self.cu.fetchall()
        self.assertEqual(len(res), 1)
        res = self.cu.fetchall()
        self.assertEqual(res, [])

    def CheckSetinputsizes(self):
        self.cu.setinputsizes([3, 4, 5])

    def CheckSetoutputsize(self):
        self.cu.setoutputsize(5, 0)

    def CheckSetoutputsizeNoColumn(self):
        self.cu.setoutputsize(42)

    def CheckCursorConnection(self):
        # Optional DB-API extension.
        self.assertEqual(self.cu.connection, self.cx)

    def CheckWrongCursorCallable(self):
        with self.assertRaises(TypeError):
            def f(): pass
            cur = self.cx.cursor(f)

    def CheckCursorWrongClass(self):
        class Foo: pass
        foo = Foo()
        with self.assertRaises(TypeError):
            cur = sqlite.Cursor(foo)

    def CheckLastRowIDOnReplace(self):
        """
        INSERT OR REPLACE and REPLACE INTO should produce the same behavior.
        """
        sql = '{} INTO test(id, unique_test) VALUES (?, ?)'
        for statement in ('INSERT OR REPLACE', 'REPLACE'):
            with self.subTest(statement=statement):
                self.cu.execute(sql.format(statement), (1, 'foo'))
                self.assertEqual(self.cu.lastrowid, 1)

    def CheckLastRowIDOnIgnore(self):
        self.cu.execute(
            "insert or ignore into test(unique_test) values (?)",
            ('test',))
        self.assertEqual(self.cu.lastrowid, 2)
        self.cu.execute(
            "insert or ignore into test(unique_test) values (?)",
            ('test',))
        self.assertEqual(self.cu.lastrowid, 2)

    def CheckLastRowIDInsertOR(self):
        results = []
        for statement in ('FAIL', 'ABORT', 'ROLLBACK'):
            sql = 'INSERT OR {} INTO test(unique_test) VALUES (?)'
            with self.subTest(statement='INSERT OR {}'.format(statement)):
                self.cu.execute(sql.format(statement), (statement,))
                results.append((statement, self.cu.lastrowid))
                with self.assertRaises(sqlite.IntegrityError):
                    self.cu.execute(sql.format(statement), (statement,))
                results.append((statement, self.cu.lastrowid))
        expected = [
            ('FAIL', 2), ('FAIL', 2),
            ('ABORT', 3), ('ABORT', 3),
            ('ROLLBACK', 4), ('ROLLBACK', 4),
        ]
        self.assertEqual(results, expected)


class BlobTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cx.execute("create table test(id integer primary key, blob_col blob)")
        self.blob_data = b"a" * 100
        self.cx.execute("insert into test(blob_col) values (?)", (self.blob_data, ))
        self.blob = self.cx.open_blob("test", "blob_col", 1)
        self.second_data = b"b" * 100

    def tearDown(self):
        self.blob.close()
        self.cx.close()

    def CheckLength(self):
        self.assertEqual(len(self.blob), 100)

    def CheckTell(self):
        self.assertEqual(self.blob.tell(), 0)

    def CheckSeekFromBlobStart(self):
        self.blob.seek(10)
        self.assertEqual(self.blob.tell(), 10)
        self.blob.seek(10, 0)
        self.assertEqual(self.blob.tell(), 10)

    def CheckSeekFromCurrentPosition(self):
        self.blob.seek(10, 1)
        self.blob.seek(10, 1)
        self.assertEqual(self.blob.tell(), 20)

    def CheckSeekFromBlobEnd(self):
        self.blob.seek(-10, 2)
        self.assertEqual(self.blob.tell(), 90)

    def CheckBlobSeekOverBlobSize(self):
        with self.assertRaises(ValueError):
            self.blob.seek(1000)

    def CheckBlobSeekUnderBlobSize(self):
        with self.assertRaises(ValueError):
            self.blob.seek(-10)

    def CheckBlobRead(self):
        self.assertEqual(self.blob.read(), self.blob_data)

    def CheckBlobReadSize(self):
        self.assertEqual(len(self.blob.read(10)), 10)

    def CheckBlobReadAdvanceOffset(self):
        self.blob.read(10)
        self.assertEqual(self.blob.tell(), 10)

    def CheckBlobReadStartAtOffset(self):
        self.blob.seek(10)
        self.blob.write(self.second_data[:10])
        self.blob.seek(10)
        self.assertEqual(self.blob.read(10), self.second_data[:10])

    def CheckBlobWrite(self):
        self.blob.write(self.second_data)
        self.assertEqual(self.cx.execute("select blob_col from test").fetchone()[0], self.second_data)

    def CheckBlobWriteAtOffset(self):
        self.blob.seek(50)
        self.blob.write(self.second_data[:50])
        self.assertEqual(self.cx.execute("select blob_col from test").fetchone()[0],
                         self.blob_data[:50] + self.second_data[:50])

    def CheckBlobWriteAdvanceOffset(self):
        self.blob.write(self.second_data[:50])
        self.assertEqual(self.blob.tell(), 50)

    def CheckBlobWriteMoreThenBlobSize(self):
        with self.assertRaises(sqlite.OperationalError):
            self.blob.write(b"a" * 1000)

    def CheckBlobReadAfterRowChange(self):
        self.cx.execute("UPDATE test SET blob_col='aaaa' where id=1")
        with self.assertRaises(sqlite.OperationalError):
            self.blob.read()

    def CheckBlobWriteAfterRowChange(self):
        self.cx.execute("UPDATE test SET blob_col='aaaa' where id=1")
        with self.assertRaises(sqlite.OperationalError):
            self.blob.write(b"aaa")

    def CheckBlobWriteWhenReadOnly(self):
        read_only_blob = \
            self.cx.open_blob("test", "blob_col", 1, readonly=True)
        with self.assertRaises(sqlite.OperationalError):
            read_only_blob.write(b"aaa")
        read_only_blob.close()

    def CheckBlobOpenWithBadDb(self):
        with self.assertRaises(sqlite.OperationalError):
            self.cx.open_blob("test", "blob_col", 1, dbname="notexisintg")

    def CheckBlobOpenWithBadTable(self):
        with self.assertRaises(sqlite.OperationalError):
            self.cx.open_blob("notexisintg", "blob_col", 1)

    def CheckBlobOpenWithBadColumn(self):
        with self.assertRaises(sqlite.OperationalError):
            self.cx.open_blob("test", "notexisting", 1)

    def CheckBlobOpenWithBadRow(self):
        with self.assertRaises(sqlite.OperationalError):
            self.cx.open_blob("test", "blob_col", 2)

    def CheckBlobGetItem(self):
        self.assertEqual(self.blob[5], b"a")

    def CheckBlobGetItemIndexOutOfRange(self):
        with self.assertRaises(IndexError):
            self.blob[105]
        with self.assertRaises(IndexError):
            self.blob[-105]

    def CheckBlobGetItemNegativeIndex(self):
        self.assertEqual(self.blob[-5], b"a")

    def CheckBlobGetItemInvalidIndex(self):
        with self.assertRaises(TypeError):
            self.blob[b"a"]

    def CheckBlobGetSlice(self):
        self.assertEqual(self.blob[5:10], b"aaaaa")

    def CheckBlobGetSliceNegativeIndex(self):
        self.assertEqual(self.blob[5:-5], self.blob_data[5:-5])

    def CheckBlobGetSliceInvalidIndex(self):
        with self.assertRaises(TypeError):
            self.blob[5:b"a"]

    def CheckBlobGetSliceWithSkip(self):
        self.blob.write(b"abcdefghij")
        self.assertEqual(self.blob[0:10:2], b"acegi")

    def CheckBlobSetItem(self):
        self.blob[0] = b"b"
        self.assertEqual(self.cx.execute("select blob_col from test").fetchone()[0], b"b" + self.blob_data[1:])

    def CheckBlobSetSlice(self):
        self.blob[0:5] = b"bbbbb"
        self.assertEqual(self.cx.execute("select blob_col from test").fetchone()[0], b"bbbbb" + self.blob_data[5:])

    def CheckBlobSetSliceWithSkip(self):
        self.blob[0:10:2] = b"bbbbb"
        self.assertEqual(self.cx.execute("select blob_col from test").fetchone()[0], b"bababababa" + self.blob_data[10:])

    def CheckBlobGetEmptySlice(self):
        self.assertEqual(self.blob[5:5], b"")

    def CheckBlobSetSliceWrongLength(self):
        with self.assertRaises(IndexError):
            self.blob[5:10] = b"a"

    def CheckBlobConcatNotSupported(self):
        with self.assertRaises(SystemError):
            self.blob + self.blob

    def CheckBlobRepeateNotSupported(self):
        with self.assertRaises(SystemError):
            self.blob * 5

@unittest.skipUnless(threading, 'This test requires threading.')
class ThreadTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.cur = self.con.cursor()
        self.cur.execute("create table test(id integer primary key, name text, bin binary, ratio number, ts timestamp)")

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def CheckConCursor(self):
        def run(con, errors):
            try:
                cur = con.cursor()
                errors.append("did not raise ProgrammingError")
                return
            except sqlite.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"con": self.con, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def CheckConCommit(self):
        def run(con, errors):
            try:
                con.commit()
                errors.append("did not raise ProgrammingError")
                return
            except sqlite.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"con": self.con, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def CheckConRollback(self):
        def run(con, errors):
            try:
                con.rollback()
                errors.append("did not raise ProgrammingError")
                return
            except sqlite.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"con": self.con, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def CheckConClose(self):
        def run(con, errors):
            try:
                con.close()
                errors.append("did not raise ProgrammingError")
                return
            except sqlite.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"con": self.con, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def CheckCurImplicitBegin(self):
        def run(cur, errors):
            try:
                cur.execute("insert into test(name) values ('a')")
                errors.append("did not raise ProgrammingError")
                return
            except sqlite.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"cur": self.cur, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def CheckCurClose(self):
        def run(cur, errors):
            try:
                cur.close()
                errors.append("did not raise ProgrammingError")
                return
            except sqlite.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        t = threading.Thread(target=run, kwargs={"cur": self.cur, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def CheckCurExecute(self):
        def run(cur, errors):
            try:
                cur.execute("select name from test")
                errors.append("did not raise ProgrammingError")
                return
            except sqlite.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        self.cur.execute("insert into test(name) values ('a')")
        t = threading.Thread(target=run, kwargs={"cur": self.cur, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

    def CheckCurIterNext(self):
        def run(cur, errors):
            try:
                row = cur.fetchone()
                errors.append("did not raise ProgrammingError")
                return
            except sqlite.ProgrammingError:
                return
            except:
                errors.append("raised wrong exception")

        errors = []
        self.cur.execute("insert into test(name) values ('a')")
        self.cur.execute("select name from test")
        t = threading.Thread(target=run, kwargs={"cur": self.cur, "errors": errors})
        t.start()
        t.join()
        if len(errors) > 0:
            self.fail("\n".join(errors))

class ConstructorTests(unittest.TestCase):
    def CheckDate(self):
        d = sqlite.Date(2004, 10, 28)

    def CheckTime(self):
        t = sqlite.Time(12, 39, 35)

    def CheckTimestamp(self):
        ts = sqlite.Timestamp(2004, 10, 28, 12, 39, 35)

    def CheckDateFromTicks(self):
        d = sqlite.DateFromTicks(42)

    def CheckTimeFromTicks(self):
        t = sqlite.TimeFromTicks(42)

    def CheckTimestampFromTicks(self):
        ts = sqlite.TimestampFromTicks(42)

    def CheckBinary(self):
        b = sqlite.Binary(b"\0'")

class ExtensionTests(unittest.TestCase):
    def CheckScriptStringSql(self):
        con = sqlite.connect(":memory:")
        cur = con.cursor()
        cur.executescript("""
            -- bla bla
            /* a stupid comment */
            create table a(i);
            insert into a(i) values (5);
            """)
        cur.execute("select i from a")
        res = cur.fetchone()[0]
        self.assertEqual(res, 5)

    def CheckScriptSyntaxError(self):
        con = sqlite.connect(":memory:")
        cur = con.cursor()
        with self.assertRaises(sqlite.OperationalError):
            cur.executescript("create table test(x); asdf; create table test2(x)")

    def CheckScriptErrorNormal(self):
        con = sqlite.connect(":memory:")
        cur = con.cursor()
        with self.assertRaises(sqlite.OperationalError):
            cur.executescript("create table test(sadfsadfdsa); select foo from hurz;")

    def CheckCursorExecutescriptAsBytes(self):
        con = sqlite.connect(":memory:")
        cur = con.cursor()
        with self.assertRaises(ValueError) as cm:
            cur.executescript(b"create table test(foo); insert into test(foo) values (5);")
        self.assertEqual(str(cm.exception), 'script argument must be unicode.')

    def CheckConnectionExecute(self):
        con = sqlite.connect(":memory:")
        result = con.execute("select 5").fetchone()[0]
        self.assertEqual(result, 5, "Basic test of Connection.execute")

    def CheckConnectionExecutemany(self):
        con = sqlite.connect(":memory:")
        con.execute("create table test(foo)")
        con.executemany("insert into test(foo) values (?)", [(3,), (4,)])
        result = con.execute("select foo from test order by foo").fetchall()
        self.assertEqual(result[0][0], 3, "Basic test of Connection.executemany")
        self.assertEqual(result[1][0], 4, "Basic test of Connection.executemany")

    def CheckConnectionExecutescript(self):
        con = sqlite.connect(":memory:")
        con.executescript("create table test(foo); insert into test(foo) values (5);")
        result = con.execute("select foo from test").fetchone()[0]
        self.assertEqual(result, 5, "Basic test of Connection.executescript")

class ClosedConTests(unittest.TestCase):
    def CheckClosedConCursor(self):
        con = sqlite.connect(":memory:")
        con.close()
        with self.assertRaises(sqlite.ProgrammingError):
            cur = con.cursor()

    def CheckClosedConCommit(self):
        con = sqlite.connect(":memory:")
        con.close()
        with self.assertRaises(sqlite.ProgrammingError):
            con.commit()

    def CheckClosedConRollback(self):
        con = sqlite.connect(":memory:")
        con.close()
        with self.assertRaises(sqlite.ProgrammingError):
            con.rollback()

    def CheckClosedCurExecute(self):
        con = sqlite.connect(":memory:")
        cur = con.cursor()
        con.close()
        with self.assertRaises(sqlite.ProgrammingError):
            cur.execute("select 4")

    def CheckClosedBlobRead(self):
        con = sqlite.connect(":memory:")
        con.execute("create table test(id integer primary key, blob_col blob)")
        con.execute("insert into test(blob_col) values (zeroblob(100))")
        blob = con.open_blob("test", "blob_col", 1)
        con.close()
        with self.assertRaises(sqlite.ProgrammingError):
            blob.read()

    def CheckClosedCreateFunction(self):
        con = sqlite.connect(":memory:")
        con.close()
        def f(x): return 17
        with self.assertRaises(sqlite.ProgrammingError):
            con.create_function("foo", 1, f)

    def CheckClosedCreateAggregate(self):
        con = sqlite.connect(":memory:")
        con.close()
        class Agg:
            def __init__(self):
                pass
            def step(self, x):
                pass
            def finalize(self):
                return 17
        with self.assertRaises(sqlite.ProgrammingError):
            con.create_aggregate("foo", 1, Agg)

    def CheckClosedSetAuthorizer(self):
        con = sqlite.connect(":memory:")
        con.close()
        def authorizer(*args):
            return sqlite.DENY
        with self.assertRaises(sqlite.ProgrammingError):
            con.set_authorizer(authorizer)

    def CheckClosedSetProgressCallback(self):
        con = sqlite.connect(":memory:")
        con.close()
        def progress(): pass
        with self.assertRaises(sqlite.ProgrammingError):
            con.set_progress_handler(progress, 100)

    def CheckClosedCall(self):
        con = sqlite.connect(":memory:")
        con.close()
        with self.assertRaises(sqlite.ProgrammingError):
            con()

class ClosedCurTests(unittest.TestCase):
    def CheckClosed(self):
        con = sqlite.connect(":memory:")
        cur = con.cursor()
        cur.close()

        for method_name in ("execute", "executemany", "executescript", "fetchall", "fetchmany", "fetchone"):
            if method_name in ("execute", "executescript"):
                params = ("select 4 union select 5",)
            elif method_name == "executemany":
                params = ("insert into foo(bar) values (?)", [(3,), (4,)])
            else:
                params = []

            with self.assertRaises(sqlite.ProgrammingError):
                method = getattr(cur, method_name)
                method(*params)


class SqliteOnConflictTests(unittest.TestCase):
    """
    Tests for SQLite's "insert on conflict" feature.

    See https://www.sqlite.org/lang_conflict.html for details.
    """

    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cu = self.cx.cursor()
        self.cu.execute("""
          CREATE TABLE test(
            id INTEGER PRIMARY KEY, name TEXT, unique_name TEXT UNIQUE
          );
        """)

    def tearDown(self):
        self.cu.close()
        self.cx.close()

    def CheckOnConflictRollbackWithExplicitTransaction(self):
        self.cx.isolation_level = None  # autocommit mode
        self.cu = self.cx.cursor()
        # Start an explicit transaction.
        self.cu.execute("BEGIN")
        self.cu.execute("INSERT INTO test(name) VALUES ('abort_test')")
        self.cu.execute("INSERT OR ROLLBACK INTO test(unique_name) VALUES ('foo')")
        with self.assertRaises(sqlite.IntegrityError):
            self.cu.execute("INSERT OR ROLLBACK INTO test(unique_name) VALUES ('foo')")
        # Use connection to commit.
        self.cx.commit()
        self.cu.execute("SELECT name, unique_name from test")
        # Transaction should have rolled back and nothing should be in table.
        self.assertEqual(self.cu.fetchall(), [])

    def CheckOnConflictAbortRaisesWithExplicitTransactions(self):
        # Abort cancels the current sql statement but doesn't change anything
        # about the current transaction.
        self.cx.isolation_level = None  # autocommit mode
        self.cu = self.cx.cursor()
        # Start an explicit transaction.
        self.cu.execute("BEGIN")
        self.cu.execute("INSERT INTO test(name) VALUES ('abort_test')")
        self.cu.execute("INSERT OR ABORT INTO test(unique_name) VALUES ('foo')")
        with self.assertRaises(sqlite.IntegrityError):
            self.cu.execute("INSERT OR ABORT INTO test(unique_name) VALUES ('foo')")
        self.cx.commit()
        self.cu.execute("SELECT name, unique_name FROM test")
        # Expect the first two inserts to work, third to do nothing.
        self.assertEqual(self.cu.fetchall(), [('abort_test', None), (None, 'foo',)])

    def CheckOnConflictRollbackWithoutTransaction(self):
        # Start of implicit transaction
        self.cu.execute("INSERT INTO test(name) VALUES ('abort_test')")
        self.cu.execute("INSERT OR ROLLBACK INTO test(unique_name) VALUES ('foo')")
        with self.assertRaises(sqlite.IntegrityError):
            self.cu.execute("INSERT OR ROLLBACK INTO test(unique_name) VALUES ('foo')")
        self.cu.execute("SELECT name, unique_name FROM test")
        # Implicit transaction is rolled back on error.
        self.assertEqual(self.cu.fetchall(), [])

    def CheckOnConflictAbortRaisesWithoutTransactions(self):
        # Abort cancels the current sql statement but doesn't change anything
        # about the current transaction.
        self.cu.execute("INSERT INTO test(name) VALUES ('abort_test')")
        self.cu.execute("INSERT OR ABORT INTO test(unique_name) VALUES ('foo')")
        with self.assertRaises(sqlite.IntegrityError):
            self.cu.execute("INSERT OR ABORT INTO test(unique_name) VALUES ('foo')")
        # Make sure all other values were inserted.
        self.cu.execute("SELECT name, unique_name FROM test")
        self.assertEqual(self.cu.fetchall(), [('abort_test', None), (None, 'foo',)])

    def CheckOnConflictFail(self):
        self.cu.execute("INSERT OR FAIL INTO test(unique_name) VALUES ('foo')")
        with self.assertRaises(sqlite.IntegrityError):
            self.cu.execute("INSERT OR FAIL INTO test(unique_name) VALUES ('foo')")
        self.assertEqual(self.cu.fetchall(), [])

    def CheckOnConflictIgnore(self):
        self.cu.execute("INSERT OR IGNORE INTO test(unique_name) VALUES ('foo')")
        # Nothing should happen.
        self.cu.execute("INSERT OR IGNORE INTO test(unique_name) VALUES ('foo')")
        self.cu.execute("SELECT unique_name FROM test")
        self.assertEqual(self.cu.fetchall(), [('foo',)])

    def CheckOnConflictReplace(self):
        self.cu.execute("INSERT OR REPLACE INTO test(name, unique_name) VALUES ('Data!', 'foo')")
        # There shouldn't be an IntegrityError exception.
        self.cu.execute("INSERT OR REPLACE INTO test(name, unique_name) VALUES ('Very different data!', 'foo')")
        self.cu.execute("SELECT name, unique_name FROM test")
        self.assertEqual(self.cu.fetchall(), [('Very different data!', 'foo')])



class ClosedBlobTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cx.execute("create table test(id integer primary key, blob_col blob)")
        self.cx.execute("insert into test(blob_col) values (zeroblob(100))")

    def tearDown(self):
        self.cx.close()

    def CheckClosedRead(self):
        self.blob = self.cx.open_blob("test", "blob_col", 1)
        self.blob.close()
        with self.assertRaises(sqlite.ProgrammingError):
            self.blob.read()

    def CheckClosedWrite(self):
        self.blob = self.cx.open_blob("test", "blob_col", 1)
        self.blob.close()
        with self.assertRaises(sqlite.ProgrammingError):
            self.blob.write(b"aaaaaaaaa")

    def CheckClosedSeek(self):
        self.blob = self.cx.open_blob("test", "blob_col", 1)
        self.blob.close()
        with self.assertRaises(sqlite.ProgrammingError):
            self.blob.seek(10)

    def CheckClosedTell(self):
        self.blob = self.cx.open_blob("test", "blob_col", 1)
        self.blob.close()
        with self.assertRaises(sqlite.ProgrammingError):
            self.blob.tell()

    def CheckClosedClose(self):
        self.blob = self.cx.open_blob("test", "blob_col", 1)
        self.blob.close()
        with self.assertRaises(sqlite.ProgrammingError):
            self.blob.close()


class BlobContextManagerTests(unittest.TestCase):
    def setUp(self):
        self.cx = sqlite.connect(":memory:")
        self.cx.execute("create table test(id integer primary key, blob_col blob)")
        self.cx.execute("insert into test(blob_col) values (zeroblob(100))")

    def tearDown(self):
        self.cx.close()

    def CheckContextExecute(self):
        data = b"a" * 100
        with self.cx.open_blob("test", "blob_col", 1) as blob:
            blob.write(data)
        self.assertEqual(self.cx.execute("select blob_col from test").fetchone()[0], data)

    def CheckContextCloseBlob(self):
        with self.cx.open_blob("test", "blob_col", 1) as blob:
            blob.seek(10)
        with self.assertRaises(sqlite.ProgrammingError):
            blob.close()


def suite():
    module_suite = unittest.makeSuite(ModuleTests, "Check")
    connection_suite = unittest.makeSuite(ConnectionTests, "Check")
    cursor_suite = unittest.makeSuite(CursorTests, "Check")
    thread_suite = unittest.makeSuite(ThreadTests, "Check")
    constructor_suite = unittest.makeSuite(ConstructorTests, "Check")
    ext_suite = unittest.makeSuite(ExtensionTests, "Check")
    closed_con_suite = unittest.makeSuite(ClosedConTests, "Check")
    closed_cur_suite = unittest.makeSuite(ClosedCurTests, "Check")
    on_conflict_suite = unittest.makeSuite(SqliteOnConflictTests, "Check")
    blob_suite = unittest.makeSuite(BlobTests, "Check")
    closed_blob_suite = unittest.makeSuite(ClosedBlobTests, "Check")
    blob_context_manager_suite = unittest.makeSuite(BlobContextManagerTests, "Check")
    return unittest.TestSuite((module_suite, connection_suite, cursor_suite, thread_suite, constructor_suite,
                               ext_suite, closed_con_suite, closed_cur_suite, on_conflict_suite,
                               blob_suite, closed_blob_suite, blob_context_manager_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
