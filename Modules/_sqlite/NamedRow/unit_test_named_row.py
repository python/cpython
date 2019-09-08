import sqlite3
import unittest


class NamedRowSetup(unittest.TestCase):

    expected_list = [("a", 1), ("dash-name", 2), ("space name", 3)]

    def setUp(self):
        """Database ('a', 'dash-name', 'space name')
        Filled with a 100 of (1, 2)
        """
        conn = sqlite3.connect(":memory:")
        conn.execute('CREATE TABLE t(a, "dash-name", "space name")')
        for i in range(100):
            conn.execute("INSERT INTO t VALUES(?,?,?)", (1, 2, 3))
        conn.row_factory = sqlite3.NamedRow
        self.conn = conn

    def row(self):
        """An instance of NamedRow."""
        return self.conn.execute("SELECT * FROM t LIMIT 1").fetchone()


class Test01_sqlite3_connection(NamedRowSetup):
    def test_database(self):
        self.assertIn("NamedRow", repr(self.conn.row_factory))
        cursor = self.conn.execute("SELECT * FROM t LIMIT 1")
        self.assertSequenceEqual(
            ("a", "dash-name", "space name"), tuple(x[0] for x in cursor.description)
        )


class Test02_NamedRow_class(NamedRowSetup):
    def test1_is_NamedRow(self):
        row = self.row()
        self.assertIn("NamedRow", row.__class__.__name__)
        self.assertIn("NamedRow", repr(row))

    def test2_len(self):
        self.assertEqual(3, len(self.row()))


class Test03_NamedRow_access(NamedRowSetup):
    def test1_getitem_by_number(self):
        row = self.row()
        self.assertEqual(1, row[0])
        self.assertEqual(2, row[1])
        self.assertEqual(3, row[2])

    def test2_getitem_by_string(self):
        row = self.row()
        self.assertEqual(1, row["a"])
        self.assertEqual(2, row["dash-name"])
        self.assertEqual(3, row["space name"])

    def test3_getitem_by_string_acceptable_replacement(self):
        row = self.row()
        # underscore is acceptable for space and dash
        self.assertEqual(2, row["dash_name"])
        self.assertEqual(3, row["space_name"])

    def test4_getitem_by_string_case_insensitive(self):
        row = self.row()
        self.assertEqual(1, row["A"])
        self.assertEqual(2, row["DaSh-nAmE"])
        self.assertEqual(3, row["Space Name"])

        self.assertEqual(2, row["dAsH_NaMe"])
        self.assertEqual(3, row["Space_Name"])

    def test5_access_by_attribute(self):
        row = self.row()
        self.assertEqual(1, row.a)
        self.assertEqual(2, row.dash_name)
        self.assertEqual(3, row.SPACE_Name)


class Test_NamedRow_iterator:
    def test1_iterable_keyValue_pairs(self):
        row = self.row()
        self.assertSequenceEqual(self.expected_list, [x for x in row])
        self.assertSequenceEqual(self.expected_list, tuple(row))
        self.assertSequenceEqual(self.expected_list, list(row))

    def test2_iterable_into_dict(self):
        row = self.row()
        self.assertDictEqual(dict(self.expected_list), dict(row))


class Test_NamedRow_exceptions(NamedRowSetup):
    def test_IndexError(self):
        row = self.row()
        with self.assertRaises(IndexError):
            row[9]
        with self.assertRaises(IndexError):
            row["no_such_field"]

    def test_AttributeError(self):
        row = self.row()
        with self.assertRaises(AttributeError):
            row.alphabet

    def test_not_assignable(self):
        row = self.row()
        with self.assertRaises(TypeError):
            row["a"] = 100
        with self.assertRaises(TypeError):
            row.a = 100


if __name__ == "__main__":
    unittest.main()
