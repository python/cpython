# -*- coding: utf-8 -*-
# Tests the NamedRow row_factory with uft-8 for non ascii attribute names
#
# Copyright (C) 2019 Clinton James "JIDN"
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no even will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. Ths origin of this software must to be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source version must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

import unittest
import sqlite3


class NamedRowFactoryTests(unittest.TestCase):
    def setUp(self):
        self.conn = sqlite3.connect(":memory:")
        self.conn.row_factory = sqlite3.NamedRow

    def tearDown(self):
        self.conn.close()

    def CheckInstance(self):
        """row_factory creates NamedRow instances"""
        row = self.conn.execute("SELECT 1, 2").fetchone()
        self.assertIsInstance(row, sqlite3.NamedRow)
        self.assertIn("NamedRow", row.__class__.__name__)
        self.assertIn("NamedRow", repr(row))
        self.assertIn(
            "optimized row_factory for column name access", sqlite3.NamedRow.__doc__
        )

    def CheckByNumberIndex(self):
        """Get values by numbered index [0]"""
        row = self.conn.execute("SELECT 1, 2").fetchone()
        # len(row)
        self.assertEqual(2, len(row))

        self.assertEqual(row[0], 1)
        self.assertEqual(row[1], 2)
        self.assertEqual(row[-1], 2)
        self.assertEqual(row[-2], 1)

        # Check past boundary
        with self.assertRaises(IndexError):
            row[len(row) + 1]
        with self.assertRaises(IndexError):
            row[-1 * (len(row) + 1)]

    def CheckByStringIndex(self):
        """Get values by string index ['field']"""
        row = self.conn.execute('SELECT 1 AS a, 2 AS abcdefg, 4 AS "四"').fetchone()
        self.assertEqual(1, row["a"])
        self.assertEqual(2, row["abcdefg"])
        self.assertEqual(4, row["四"])

    def CheckByAttribute(self):
        """Get values by attribute row.field"""
        row = self.conn.execute('SELECT 1 AS a, 2 AS abcdefg, 4 AS "四"').fetchone()
        self.assertEqual(1, row.a)
        self.assertEqual(2, row.abcdefg)
        self.assertEqual(4, row.四)

    def CheckContainsField(self):
        row = self.conn.execute("SELECT 1 AS a, 2 AS b").fetchone()
        self.assertIn("a", row)
        self.assertNotIn("A", row)

    def CheckSlice(self):
        """Does NamedRow slice like a list."""
        row = self.conn.execute("SELECT 1, 2, 3, 4").fetchone()
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

    def CheckHashComparison(self):
        row1 = self.conn.execute("SELECT 1 AS a, 2 AS b").fetchone()
        row2 = self.conn.execute("SELECT 1 AS a, 2 AS b").fetchone()
        row3 = self.conn.execute("SELECT 1 AS a, 3 AS b").fetchone()

        self.assertEqual(row1, row1)
        self.assertEqual(row1, row2)
        self.assertTrue(row2 != row3)

        self.assertFalse(row1 != row1)
        self.assertFalse(row1 != row2)
        self.assertFalse(row2 == row3)

        self.assertEqual(row1, row2)
        self.assertEqual(hash(row1), hash(row2))
        self.assertNotEqual(row1, row3)
        self.assertNotEqual(hash(row1), hash(row3))

        with self.assertRaises(TypeError):
            row1 > row2
        with self.assertRaises(TypeError):
            row1 < row2
        with self.assertRaises(TypeError):
            row1 >= row2
        with self.assertRaises(TypeError):
            row1 <= row1

    def CheckFakeCursorClass(self):
        # Issue #24257: Incorrect use of PyObject_IsInstance() caused
        # segmentation fault.
        # Issue #27861: Also applies for cursor factory.
        class FakeCursor(str):
            __class__ = sqlite3.Cursor

        self.assertRaises(TypeError, self.conn.cursor, FakeCursor)
        self.assertRaises(TypeError, sqlite3.NamedRow, FakeCursor(), ())

    def CheckIterable(self):
        """Is NamedRow iterable."""
        row = self.conn.execute("SELECT 1 AS a, 2 AS abcdefg").fetchone()
        self.assertEqual(2, len(row))
        for col in row:
            # Its is a key/value pair
            self.assertEqual(2, len(col))

    def CheckIterablePairs(self):
        expected = (("a", 1), ("b", 2))
        row = self.conn.execute("SELECT 1 AS a, 2 AS b").fetchone()
        self.assertSequenceEqual(expected, [x for x in row])
        self.assertSequenceEqual(expected, tuple(row))
        self.assertSequenceEqual(expected, list(row))
        self.assertSequenceEqual(dict(expected), dict(row))

    def CheckExpectedExceptions(self):
        row = self.conn.execute("SELECT 1 AS a").fetchone()
        with self.assertRaises(IndexError):
            row["no_such_field"]
        with self.assertRaises(AttributeError):
            row.no_such_field
        with self.assertRaises(TypeError):
            row[0] = 100
        with self.assertRaises(TypeError):
            row["a"] = 100
        with self.assertRaises(TypeError) as err:
            row.a = 100
        self.assertIn("does not support item assignment", str(err.exception))

        with self.assertRaises(TypeError):
            setattr(row, "a", 100)


def suite():
    return unittest.makeSuite(NamedRowFactoryTests, "Check")


def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())


if __name__ == "__main__":
    unittest.main()
