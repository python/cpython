import sqlite3
import unittest


class NamedRowFactoryTests(unittest.TestCase):
    def setUp(self):
        self.conn = sqlite3.connect(":memory:")
        self.conn.row_factory = sqlite3.NamedRow

    def test_instance(self):
        """row_factory creates NamedRow instances"""
        row = self.conn.execute("SELECT 1, 2").fetchone()
        self.assertIsInstance(row, sqlite3.NamedRow)
        self.assertIn("NamedRow", row.__class__.__name__)
        self.assertIn("NamedRow", repr(row))
        self.assertIn("optimized row_factory with namedtuple", sqlite3.NamedRow.__doc__)

    def test_by_number_index(self):
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

    def test_by_string_index(self):
        """Get values by string index ['field']"""
        row = self.conn.execute(
            'SELECT 1 AS a, 2 AS "dash-name", ' '3 AS "space name", 4 AS "四"'
        ).fetchone()
        self.assertEqual(1, row["a"])
        self.assertEqual(2, row["dash-name"])
        self.assertEqual(3, row["space name"])
        self.assertEqual(4, row["四"])

        # Case insensitive
        self.assertEqual(1, row["A"])
        self.assertEqual(2, row["DaSh-NaMe"])
        self.assertEqual(3, row["SPACE name"])

        # Underscore as acceptable replacement for dash and space
        self.assertEqual(2, row["dash_NAME"])
        self.assertEqual(3, row["space_name"])

    def test_by_attribute(self):
        """Get values by attribute row.field"""
        row = self.conn.execute(
            'SELECT 1 AS a, 2 AS "dash-name", ' '3 AS "space name", 4 AS "四"'
        ).fetchone()
        self.assertEqual(1, row.a)
        self.assertEqual(2, row.dash_name)
        self.assertEqual(3, row.space_name)
        self.assertEqual(4, row.四)
        self.assertEqual(2, row.DASH_NAME)

    def test_contains_field(self):
        row = self.conn.execute("SELECT 1 AS a, 2 AS b").fetchone()
        self.assertIn("a", row)
        self.assertIn("A", row)

    def test_slice(self):
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

    def test_hash_comparison(self):
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

    def test_fake_cursor_class(self):
        # Issue #24257: Incorrect use of PyObject_IsInstance() caused
        # segmentation fault.
        # Issue #27861: Also applies for cursor factory.
        class FakeCursor(str):
            __class__ = sqlite3.Cursor

        self.assertRaises(TypeError, self.conn.cursor, FakeCursor)
        self.assertRaises(TypeError, sqlite3.NamedRow, FakeCursor(), ())

    def test_iterable(self):
        """Is NamedRow iterable."""
        row = self.conn.execute("SELECT 1 AS a, 2 AS b").fetchone()
        for col in row:
            pass

    def test_iterable_pairs(self):
        expected = (("a", 1), ("b", 2))
        row = self.conn.execute("SELECT 1 AS a, 2 AS b").fetchone()
        self.assertSequenceEqual(expected, [x for x in row])
        self.assertSequenceEqual(expected, tuple(row))
        self.assertSequenceEqual(expected, list(row))
        self.assertSequenceEqual(dict(expected), dict(row))

    def test_expected_exceptions(self):
        row = self.conn.execute('SELECT 1 AS a, 2 AS "dash-name"').fetchone()
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


if __name__ == "__main__":
    unittest.main()
