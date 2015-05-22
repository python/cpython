#-*- coding: ISO-8859-1 -*-
# pysqlite2/test/factory.py: tests for the various factories in pysqlite
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

import unittest
import sqlite3 as sqlite
from collections import Sequence

class MyConnection(sqlite.Connection):
    def __init__(self, *args, **kwargs):
        sqlite.Connection.__init__(self, *args, **kwargs)

def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
    return d

class MyCursor(sqlite.Cursor):
    def __init__(self, *args, **kwargs):
        sqlite.Cursor.__init__(self, *args, **kwargs)
        self.row_factory = dict_factory

class ConnectionFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:", factory=MyConnection)

    def tearDown(self):
        self.con.close()

    def CheckIsInstance(self):
        self.assertIsInstance(self.con, MyConnection)

class CursorFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def tearDown(self):
        self.con.close()

    def CheckIsInstance(self):
        cur = self.con.cursor(factory=MyCursor)
        self.assertIsInstance(cur, MyCursor)

class RowFactoryTestsBackwardsCompat(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def CheckIsProducedByFactory(self):
        cur = self.con.cursor(factory=MyCursor)
        cur.execute("select 4+5 as foo")
        row = cur.fetchone()
        self.assertIsInstance(row, dict)
        cur.close()

    def tearDown(self):
        self.con.close()

class RowFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def CheckCustomFactory(self):
        self.con.row_factory = lambda cur, row: list(row)
        row = self.con.execute("select 1, 2").fetchone()
        self.assertIsInstance(row, list)

    def CheckSqliteRowIndex(self):
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        self.assertIsInstance(row, sqlite.Row)

        col1, col2 = row["a"], row["b"]
        self.assertEqual(col1, 1, "by name: wrong result for column 'a'")
        self.assertEqual(col2, 2, "by name: wrong result for column 'a'")

        col1, col2 = row["A"], row["B"]
        self.assertEqual(col1, 1, "by name: wrong result for column 'A'")
        self.assertEqual(col2, 2, "by name: wrong result for column 'B'")

        self.assertEqual(row[0], 1, "by index: wrong result for column 0")
        self.assertEqual(row[0L], 1, "by index: wrong result for column 0")
        self.assertEqual(row[1], 2, "by index: wrong result for column 1")
        self.assertEqual(row[1L], 2, "by index: wrong result for column 1")
        self.assertEqual(row[-1], 2, "by index: wrong result for column -1")
        self.assertEqual(row[-1L], 2, "by index: wrong result for column -1")
        self.assertEqual(row[-2], 1, "by index: wrong result for column -2")
        self.assertEqual(row[-2L], 1, "by index: wrong result for column -2")

        with self.assertRaises(IndexError):
            row['c']
        with self.assertRaises(IndexError):
            row[2]
        with self.assertRaises(IndexError):
            row[2L]
        with self.assertRaises(IndexError):
            row[-3]
        with self.assertRaises(IndexError):
            row[-3L]
        with self.assertRaises(IndexError):
            row[2**1000]

    def CheckSqliteRowIter(self):
        """Checks if the row object is iterable"""
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        for col in row:
            pass

    def CheckSqliteRowAsTuple(self):
        """Checks if the row object can be converted to a tuple"""
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        t = tuple(row)
        self.assertEqual(t, (row['a'], row['b']))

    def CheckSqliteRowAsDict(self):
        """Checks if the row object can be correctly converted to a dictionary"""
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        d = dict(row)
        self.assertEqual(d["a"], row["a"])
        self.assertEqual(d["b"], row["b"])

    def CheckSqliteRowHashCmp(self):
        """Checks if the row object compares and hashes correctly"""
        self.con.row_factory = sqlite.Row
        row_1 = self.con.execute("select 1 as a, 2 as b").fetchone()
        row_2 = self.con.execute("select 1 as a, 2 as b").fetchone()
        row_3 = self.con.execute("select 1 as a, 3 as b").fetchone()

        self.assertEqual(row_1, row_1)
        self.assertEqual(row_1, row_2)
        self.assertTrue(row_2 != row_3)

        self.assertFalse(row_1 != row_1)
        self.assertFalse(row_1 != row_2)
        self.assertFalse(row_2 == row_3)

        self.assertEqual(row_1, row_2)
        self.assertEqual(hash(row_1), hash(row_2))
        self.assertNotEqual(row_1, row_3)
        self.assertNotEqual(hash(row_1), hash(row_3))

    def CheckSqliteRowAsSequence(self):
        """ Checks if the row object can act like a sequence """
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()

        as_tuple = tuple(row)
        self.assertEqual(list(reversed(row)), list(reversed(as_tuple)))
        self.assertIsInstance(row, Sequence)

    def CheckFakeCursorClass(self):
        # Issue #24257: Incorrect use of PyObject_IsInstance() caused
        # segmentation fault.
        class FakeCursor(str):
            __class__ = sqlite.Cursor
        cur = self.con.cursor(factory=FakeCursor)
        self.assertRaises(TypeError, sqlite.Row, cur, ())

    def tearDown(self):
        self.con.close()

class TextFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def CheckUnicode(self):
        austria = unicode("Österreich", "latin1")
        row = self.con.execute("select ?", (austria,)).fetchone()
        self.assertEqual(type(row[0]), unicode, "type of row[0] must be unicode")

    def CheckString(self):
        self.con.text_factory = str
        austria = unicode("Österreich", "latin1")
        row = self.con.execute("select ?", (austria,)).fetchone()
        self.assertEqual(type(row[0]), str, "type of row[0] must be str")
        self.assertEqual(row[0], austria.encode("utf-8"), "column must equal original data in UTF-8")

    def CheckCustom(self):
        self.con.text_factory = lambda x: unicode(x, "utf-8", "ignore")
        austria = unicode("Österreich", "latin1")
        row = self.con.execute("select ?", (austria.encode("latin1"),)).fetchone()
        self.assertEqual(type(row[0]), unicode, "type of row[0] must be unicode")
        self.assertTrue(row[0].endswith(u"reich"), "column must contain original data")

    def CheckOptimizedUnicode(self):
        self.con.text_factory = sqlite.OptimizedUnicode
        austria = unicode("Österreich", "latin1")
        germany = unicode("Deutchland")
        a_row = self.con.execute("select ?", (austria,)).fetchone()
        d_row = self.con.execute("select ?", (germany,)).fetchone()
        self.assertEqual(type(a_row[0]), unicode, "type of non-ASCII row must be unicode")
        self.assertEqual(type(d_row[0]), str, "type of ASCII-only row must be str")

    def tearDown(self):
        self.con.close()

class TextFactoryTestsWithEmbeddedZeroBytes(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.con.execute("create table test (value text)")
        self.con.execute("insert into test (value) values (?)", ("a\x00b",))

    def CheckString(self):
        # text_factory defaults to unicode
        row = self.con.execute("select value from test").fetchone()
        self.assertIs(type(row[0]), unicode)
        self.assertEqual(row[0], "a\x00b")

    def CheckCustom(self):
        # A custom factory should receive an str argument
        self.con.text_factory = lambda x: x
        row = self.con.execute("select value from test").fetchone()
        self.assertIs(type(row[0]), str)
        self.assertEqual(row[0], "a\x00b")

    def CheckOptimizedUnicodeAsString(self):
        # ASCII -> str argument
        self.con.text_factory = sqlite.OptimizedUnicode
        row = self.con.execute("select value from test").fetchone()
        self.assertIs(type(row[0]), str)
        self.assertEqual(row[0], "a\x00b")

    def CheckOptimizedUnicodeAsUnicode(self):
        # Non-ASCII -> unicode argument
        self.con.text_factory = sqlite.OptimizedUnicode
        self.con.execute("delete from test")
        self.con.execute("insert into test (value) values (?)", (u'ä\0ö',))
        row = self.con.execute("select value from test").fetchone()
        self.assertIs(type(row[0]), unicode)
        self.assertEqual(row[0], u"ä\x00ö")

    def tearDown(self):
        self.con.close()

def suite():
    connection_suite = unittest.makeSuite(ConnectionFactoryTests, "Check")
    cursor_suite = unittest.makeSuite(CursorFactoryTests, "Check")
    row_suite_compat = unittest.makeSuite(RowFactoryTestsBackwardsCompat, "Check")
    row_suite = unittest.makeSuite(RowFactoryTests, "Check")
    text_suite = unittest.makeSuite(TextFactoryTests, "Check")
    text_zero_bytes_suite = unittest.makeSuite(TextFactoryTestsWithEmbeddedZeroBytes, "Check")
    return unittest.TestSuite((connection_suite, cursor_suite, row_suite_compat, row_suite, text_suite, text_zero_bytes_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
