#-*- coding: ISO-8859-1 -*-
# pysqlite2/test/factory.py: tests for the various factories in pysqlite
#
# Copyright (C) 2005 Gerhard Häring <gh@ghaering.de>
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
        self.failUnless(isinstance(self.con,
                                   MyConnection),
                        "connection is not instance of MyConnection")

class CursorFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def tearDown(self):
        self.con.close()

    def CheckIsInstance(self):
        cur = self.con.cursor(factory=MyCursor)
        self.failUnless(isinstance(cur,
                                   MyCursor),
                        "cursor is not instance of MyCursor")

class RowFactoryTestsBackwardsCompat(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def CheckIsProducedByFactory(self):
        cur = self.con.cursor(factory=MyCursor)
        cur.execute("select 4+5 as foo")
        row = cur.fetchone()
        self.failUnless(isinstance(row,
                                   dict),
                        "row is not instance of dict")
        cur.close()

    def tearDown(self):
        self.con.close()

class RowFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def CheckCustomFactory(self):
        self.con.row_factory = lambda cur, row: list(row)
        row = self.con.execute("select 1, 2").fetchone()
        self.failUnless(isinstance(row,
                                   list),
                        "row is not instance of list")

    def CheckSqliteRowIndex(self):
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        self.failUnless(isinstance(row,
                                   sqlite.Row),
                        "row is not instance of sqlite.Row")

        col1, col2 = row["a"], row["b"]
        self.failUnless(col1 == 1, "by name: wrong result for column 'a'")
        self.failUnless(col2 == 2, "by name: wrong result for column 'a'")

        col1, col2 = row["A"], row["B"]
        self.failUnless(col1 == 1, "by name: wrong result for column 'A'")
        self.failUnless(col2 == 2, "by name: wrong result for column 'B'")

        col1, col2 = row[0], row[1]
        self.failUnless(col1 == 1, "by index: wrong result for column 0")
        self.failUnless(col2 == 2, "by index: wrong result for column 1")

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

    def CheckSqliteRowAsDict(self):
        """Checks if the row object can be correctly converted to a dictionary"""
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        d = dict(row)
        self.failUnlessEqual(d["a"], row["a"])
        self.failUnlessEqual(d["b"], row["b"])

    def tearDown(self):
        self.con.close()

class TextFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def CheckUnicode(self):
        austria = "Österreich"
        row = self.con.execute("select ?", (austria,)).fetchone()
        self.failUnless(type(row[0]) == str, "type of row[0] must be unicode")

    def CheckString(self):
        self.con.text_factory = bytes
        austria = "Österreich"
        row = self.con.execute("select ?", (austria,)).fetchone()
        self.failUnless(type(row[0]) == bytes, "type of row[0] must be bytes")
        self.failUnless(row[0] == austria.encode("utf-8"), "column must equal original data in UTF-8")

    def CheckCustom(self):
        self.con.text_factory = lambda x: str(x, "utf-8", "ignore")
        austria = "Österreich"
        row = self.con.execute("select ?", (austria,)).fetchone()
        self.failUnless(type(row[0]) == str, "type of row[0] must be unicode")
        self.failUnless(row[0].endswith("reich"), "column must contain original data")

    def CheckOptimizedUnicode(self):
        self.con.text_factory = sqlite.OptimizedUnicode
        austria = "Österreich"
        germany = "Deutchland"
        a_row = self.con.execute("select ?", (austria,)).fetchone()
        d_row = self.con.execute("select ?", (germany,)).fetchone()
        self.failUnless(type(a_row[0]) == str, "type of non-ASCII row must be unicode")
        self.failUnless(type(d_row[0]) == str8, "type of ASCII-only row must be str8")

    def tearDown(self):
        self.con.close()

def suite():
    connection_suite = unittest.makeSuite(ConnectionFactoryTests, "Check")
    cursor_suite = unittest.makeSuite(CursorFactoryTests, "Check")
    row_suite_compat = unittest.makeSuite(RowFactoryTestsBackwardsCompat, "Check")
    row_suite = unittest.makeSuite(RowFactoryTests, "Check")
    text_suite = unittest.makeSuite(TextFactoryTests, "Check")
    return unittest.TestSuite((connection_suite, cursor_suite, row_suite_compat, row_suite, text_suite))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
