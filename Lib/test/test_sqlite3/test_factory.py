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
from collections.abc import Sequence


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
    def test_connection_factories(self):
        class DefectFactory(sqlite.Connection):
            def __init__(self, *args, **kwargs):
                return None
        class OkFactory(sqlite.Connection):
            def __init__(self, *args, **kwargs):
                sqlite.Connection.__init__(self, *args, **kwargs)

        for factory in DefectFactory, OkFactory:
            with self.subTest(factory=factory):
                con = sqlite.connect(":memory:", factory=factory)
                self.assertIsInstance(con, factory)

    def test_connection_factory_relayed_call(self):
        # gh-95132: keyword args must not be passed as positional args
        class Factory(sqlite.Connection):
            def __init__(self, *args, **kwargs):
                kwargs["isolation_level"] = None
                super(Factory, self).__init__(*args, **kwargs)

        con = sqlite.connect(":memory:", factory=Factory)
        self.assertIsNone(con.isolation_level)
        self.assertIsInstance(con, Factory)

    def test_connection_factory_as_positional_arg(self):
        class Factory(sqlite.Connection):
            def __init__(self, *args, **kwargs):
                super(Factory, self).__init__(*args, **kwargs)

        con = sqlite.connect(":memory:", 5.0, 0, None, True, Factory)
        self.assertIsNone(con.isolation_level)
        self.assertIsInstance(con, Factory)


class CursorFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def tearDown(self):
        self.con.close()

    def test_is_instance(self):
        cur = self.con.cursor()
        self.assertIsInstance(cur, sqlite.Cursor)
        cur = self.con.cursor(MyCursor)
        self.assertIsInstance(cur, MyCursor)
        cur = self.con.cursor(factory=lambda con: MyCursor(con))
        self.assertIsInstance(cur, MyCursor)

    def test_invalid_factory(self):
        # not a callable at all
        self.assertRaises(TypeError, self.con.cursor, None)
        # invalid callable with not exact one argument
        self.assertRaises(TypeError, self.con.cursor, lambda: None)
        # invalid callable returning non-cursor
        self.assertRaises(TypeError, self.con.cursor, lambda con: None)

class RowFactoryTestsBackwardsCompat(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def test_is_produced_by_factory(self):
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

    def test_custom_factory(self):
        self.con.row_factory = lambda cur, row: list(row)
        row = self.con.execute("select 1, 2").fetchone()
        self.assertIsInstance(row, list)

    def test_sqlite_row_index(self):
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a_1, 2 as b").fetchone()
        self.assertIsInstance(row, sqlite.Row)

        self.assertEqual(row["a_1"], 1, "by name: wrong result for column 'a_1'")
        self.assertEqual(row["b"], 2, "by name: wrong result for column 'b'")

        self.assertEqual(row["A_1"], 1, "by name: wrong result for column 'A_1'")
        self.assertEqual(row["B"], 2, "by name: wrong result for column 'B'")

        self.assertEqual(row[0], 1, "by index: wrong result for column 0")
        self.assertEqual(row[1], 2, "by index: wrong result for column 1")
        self.assertEqual(row[-1], 2, "by index: wrong result for column -1")
        self.assertEqual(row[-2], 1, "by index: wrong result for column -2")

        with self.assertRaises(IndexError):
            row['c']
        with self.assertRaises(IndexError):
            row['a_\x11']
        with self.assertRaises(IndexError):
            row['a\x7f1']
        with self.assertRaises(IndexError):
            row[2]
        with self.assertRaises(IndexError):
            row[-3]
        with self.assertRaises(IndexError):
            row[2**1000]
        with self.assertRaises(IndexError):
            row[complex()]  # index must be int or string

    def test_sqlite_row_index_unicode(self):
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as \xff").fetchone()
        self.assertEqual(row["\xff"], 1)
        with self.assertRaises(IndexError):
            row['\u0178']
        with self.assertRaises(IndexError):
            row['\xdf']

    def test_sqlite_row_slice(self):
        # A sqlite.Row can be sliced like a list.
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1, 2, 3, 4").fetchone()
        self.assertEqual(row[0:0], ())
        self.assertEqual(row[0:1], (1,))
        self.assertEqual(row[1:3], (2, 3))
        self.assertEqual(row[3:1], ())
        # Explicit bounds are optional.
        self.assertEqual(row[1:], (2, 3, 4))
        self.assertEqual(row[:3], (1, 2, 3))
        # Slices can use negative indices.
        self.assertEqual(row[-2:-1], (3,))
        self.assertEqual(row[-2:], (3, 4))
        # Slicing supports steps.
        self.assertEqual(row[0:4:2], (1, 3))
        self.assertEqual(row[3:0:-2], (4, 2))

    def test_sqlite_row_iter(self):
        """Checks if the row object is iterable"""
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        for col in row:
            pass

    def test_sqlite_row_as_tuple(self):
        """Checks if the row object can be converted to a tuple"""
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        t = tuple(row)
        self.assertEqual(t, (row['a'], row['b']))

    def test_sqlite_row_as_dict(self):
        """Checks if the row object can be correctly converted to a dictionary"""
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()
        d = dict(row)
        self.assertEqual(d["a"], row["a"])
        self.assertEqual(d["b"], row["b"])

    def test_sqlite_row_hash_cmp(self):
        """Checks if the row object compares and hashes correctly"""
        self.con.row_factory = sqlite.Row
        row_1 = self.con.execute("select 1 as a, 2 as b").fetchone()
        row_2 = self.con.execute("select 1 as a, 2 as b").fetchone()
        row_3 = self.con.execute("select 1 as a, 3 as b").fetchone()
        row_4 = self.con.execute("select 1 as b, 2 as a").fetchone()
        row_5 = self.con.execute("select 2 as b, 1 as a").fetchone()

        self.assertTrue(row_1 == row_1)
        self.assertTrue(row_1 == row_2)
        self.assertFalse(row_1 == row_3)
        self.assertFalse(row_1 == row_4)
        self.assertFalse(row_1 == row_5)
        self.assertFalse(row_1 == object())

        self.assertFalse(row_1 != row_1)
        self.assertFalse(row_1 != row_2)
        self.assertTrue(row_1 != row_3)
        self.assertTrue(row_1 != row_4)
        self.assertTrue(row_1 != row_5)
        self.assertTrue(row_1 != object())

        with self.assertRaises(TypeError):
            row_1 > row_2
        with self.assertRaises(TypeError):
            row_1 < row_2
        with self.assertRaises(TypeError):
            row_1 >= row_2
        with self.assertRaises(TypeError):
            row_1 <= row_2

        self.assertEqual(hash(row_1), hash(row_2))

    def test_sqlite_row_as_sequence(self):
        """ Checks if the row object can act like a sequence """
        self.con.row_factory = sqlite.Row
        row = self.con.execute("select 1 as a, 2 as b").fetchone()

        as_tuple = tuple(row)
        self.assertEqual(list(reversed(row)), list(reversed(as_tuple)))
        self.assertIsInstance(row, Sequence)

    def test_fake_cursor_class(self):
        # Issue #24257: Incorrect use of PyObject_IsInstance() caused
        # segmentation fault.
        # Issue #27861: Also applies for cursor factory.
        class FakeCursor(str):
            __class__ = sqlite.Cursor
        self.con.row_factory = sqlite.Row
        self.assertRaises(TypeError, self.con.cursor, FakeCursor)
        self.assertRaises(TypeError, sqlite.Row, FakeCursor(), ())

    def tearDown(self):
        self.con.close()

class TextFactoryTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

    def test_unicode(self):
        austria = "Österreich"
        row = self.con.execute("select ?", (austria,)).fetchone()
        self.assertEqual(type(row[0]), str, "type of row[0] must be unicode")

    def test_string(self):
        self.con.text_factory = bytes
        austria = "Österreich"
        row = self.con.execute("select ?", (austria,)).fetchone()
        self.assertEqual(type(row[0]), bytes, "type of row[0] must be bytes")
        self.assertEqual(row[0], austria.encode("utf-8"), "column must equal original data in UTF-8")

    def test_custom(self):
        self.con.text_factory = lambda x: str(x, "utf-8", "ignore")
        austria = "Österreich"
        row = self.con.execute("select ?", (austria,)).fetchone()
        self.assertEqual(type(row[0]), str, "type of row[0] must be unicode")
        self.assertTrue(row[0].endswith("reich"), "column must contain original data")

    def test_optimized_unicode(self):
        # OptimizedUnicode is deprecated as of Python 3.10
        with self.assertWarns(DeprecationWarning) as cm:
            self.con.text_factory = sqlite.OptimizedUnicode
        self.assertIn("factory.py", cm.filename)
        austria = "Österreich"
        germany = "Deutchland"
        a_row = self.con.execute("select ?", (austria,)).fetchone()
        d_row = self.con.execute("select ?", (germany,)).fetchone()
        self.assertEqual(type(a_row[0]), str, "type of non-ASCII row must be str")
        self.assertEqual(type(d_row[0]), str, "type of ASCII-only row must be str")

    def tearDown(self):
        self.con.close()

class TextFactoryTestsWithEmbeddedZeroBytes(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.con.execute("create table test (value text)")
        self.con.execute("insert into test (value) values (?)", ("a\x00b",))

    def test_string(self):
        # text_factory defaults to str
        row = self.con.execute("select value from test").fetchone()
        self.assertIs(type(row[0]), str)
        self.assertEqual(row[0], "a\x00b")

    def test_bytes(self):
        self.con.text_factory = bytes
        row = self.con.execute("select value from test").fetchone()
        self.assertIs(type(row[0]), bytes)
        self.assertEqual(row[0], b"a\x00b")

    def test_bytearray(self):
        self.con.text_factory = bytearray
        row = self.con.execute("select value from test").fetchone()
        self.assertIs(type(row[0]), bytearray)
        self.assertEqual(row[0], b"a\x00b")

    def test_custom(self):
        # A custom factory should receive a bytes argument
        self.con.text_factory = lambda x: x
        row = self.con.execute("select value from test").fetchone()
        self.assertIs(type(row[0]), bytes)
        self.assertEqual(row[0], b"a\x00b")

    def tearDown(self):
        self.con.close()


if __name__ == "__main__":
    unittest.main()
