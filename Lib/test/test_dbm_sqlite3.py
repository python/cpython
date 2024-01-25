import sqlite3
import sys
import test.support
import unittest
from contextlib import closing
from functools import partial
from pathlib import Path
from test.support import cpython_only, import_helper, os_helper


dbm_sqlite3 = import_helper.import_module("dbm.sqlite3")
from dbm.sqlite3 import _normalize_uri


class _SQLiteDbmTests(unittest.TestCase):

    def setUp(self):
        self.filename = os_helper.TESTFN
        db = dbm_sqlite3.open(self.filename, "c")
        db.close()

    def tearDown(self):
        for suffix in "", "-wal", "-shm":
            os_helper.unlink(self.filename + suffix)


class URI(unittest.TestCase):

    def test_uri_substitutions(self):
        dataset = (
            ("/absolute/////b/c", "file:/absolute/b/c"),
            ("/PRE#MID##END", "file:/PRE%23MID%23%23END"),
            ("/%#?%%#", "file:/%25%23%3F%25%25%23"),
            ("/localhost", "file:/localhost"),
        )
        for path, normalized in dataset:
            with self.subTest(path=path, normalized=normalized):
                self.assertEqual(_normalize_uri(path), normalized)

    @unittest.skipUnless(sys.platform == "win32", "requires Windows")
    def test_uri_windows(self):
        dataset = (
            # Relative subdir.
            (r"2018\January.xlsx",
             "file:/2018/January.xlsx"),
            # Relative CWD.
            (r"..\Publications\TravelBrochure.pdf",
             "file:/../Publications/TravelBrochure.pdf"),
            # Absolute with drive letter.
            (r"C:\Projects\apilibrary\apilibrary.sln",
             "file:/C:/Projects/apilibrary/apilibrary.sln"),
            # Relative with drive letter.
            (r"C:Projects\apilibrary\apilibrary.sln",
             "file:/C:Projects/apilibrary/apilibrary.sln"),
        )
        for path, normalized in dataset:
            with self.subTest(path=path, normalized=normalized):
                if not Path(path).is_absolute():
                    self.skipTest(f"skipping relative path: {path!r}")
                self.assertEqual(_normalize_uri(path), normalized)


class ReadOnly(_SQLiteDbmTests):

    def setUp(self):
        super().setUp()
        with dbm_sqlite3.open(self.filename, "w") as db:
            db["key1"] = "value1"
            db["key2"] = "value2"
        self.db = dbm_sqlite3.open(self.filename, "r")

    def tearDown(self):
        self.db.close()
        super().tearDown()

    def test_readonly_read(self):
        self.assertEqual(self.db["key1"], "value1")
        self.assertEqual(self.db["key2"], "value2")

    def test_readonly_write(self):
        with self.assertRaises(dbm_sqlite3.error):
            self.db["new"] = "value"

    def test_readonly_delete(self):
        with self.assertRaises(dbm_sqlite3.error):
            del self.db["key1"]

    def test_readonly_keys(self):
        self.assertEqual(self.db.keys(), ["key1", "key2"])

    def test_readonly_iter(self):
        self.assertEqual([k for k in self.db], ["key1", "key2"])


class ReadWrite(_SQLiteDbmTests):

    def setUp(self):
        super().setUp()
        self.db = dbm_sqlite3.open(self.filename, "w")

    def tearDown(self):
        self.db.close()
        super().tearDown()

    def db_content(self):
        with closing(sqlite3.connect(self.filename)) as cx:
            keys = [r[0] for r in cx.execute("SELECT key FROM Dict")]
            vals = [r[0] for r in cx.execute("SELECT value FROM Dict")]
        return keys, vals

    def test_readwrite_unique_key(self):
        self.db["key"] = "value"
        self.db["key"] = "other"
        keys, vals = self.db_content()
        self.assertEqual(keys, ["key"])
        self.assertEqual(vals, ["other"])

    def test_readwrite_delete(self):
        self.db["key"] = "value"
        self.db["new"] = "other"

        del self.db["new"]
        keys, vals = self.db_content()
        self.assertEqual(keys, ["key"])
        self.assertEqual(vals, ["value"])

        del self.db["key"]
        keys, vals = self.db_content()
        self.assertEqual(keys, [])
        self.assertEqual(vals, [])

    def test_readwrite_null_key(self):
        with self.assertRaises(dbm_sqlite3.error):
            self.db[None] = "value"

    def test_readwrite_null_value(self):
        with self.assertRaises(dbm_sqlite3.error):
            self.db["key"] = None


class Misuse(_SQLiteDbmTests):

    def setUp(self):
        super().setUp()
        self.db = dbm_sqlite3.open(self.filename, "w")

    def tearDown(self):
        self.db.close()
        super().tearDown()

    def test_misuse_double_create(self):
        self.db["key"] = "value"
        with dbm_sqlite3.open(self.filename, "c") as db:
            self.assertEqual(db["key"], "value")

    def test_misuse_double_close(self):
        self.db.close()

    def test_misuse_invalid_flag(self):
        regex = "must be.*'r'.*'w'.*'c'.*'n', not 'invalid'"
        with self.assertRaisesRegex(ValueError, regex):
            dbm_sqlite3.open(self.filename, flag="invalid")

    def test_misuse_double_delete(self):
        self.db["key"] = "value"
        del self.db["key"]
        with self.assertRaises(KeyError):
            del self.db["key"]

    def test_misuse_invalid_key(self):
        with self.assertRaises(KeyError):
            self.db["key"]

    def test_misuse_iter_close1(self):
        self.db["1"] = 1
        it = iter(self.db)
        self.db.close()
        with self.assertRaises(dbm_sqlite3.error):
            next(it)

    def test_misuse_iter_close2(self):
        self.db["1"] = 1
        self.db["2"] = 2
        it = iter(self.db)
        next(it)
        self.db.close()
        with self.assertRaises(dbm_sqlite3.error):
            next(it)

    def test_misuse_use_after_close(self):
        self.db.close()
        with self.assertRaises(dbm_sqlite3.error):
            self.db["read"]
        with self.assertRaises(dbm_sqlite3.error):
            self.db["write"] = "value"
        with self.assertRaises(dbm_sqlite3.error):
            del self.db["del"]
        with self.assertRaises(dbm_sqlite3.error):
            len(self.db)
        with self.assertRaises(dbm_sqlite3.error):
            self.db.keys()

    def test_misuse_reinit(self):
        with self.assertRaises(dbm_sqlite3.error):
            self.db.__init__("new.db", flag="rw")

    def test_misuse_empty_filename(self):
        for flag in "r", "w", "c", "n":
            with self.assertRaises(dbm_sqlite3.error):
                db = dbm_sqlite3.open("", flag="c")


class DataTypes(_SQLiteDbmTests):

    dataset = 10, 2.5, "string", b"bytes"

    def setUp(self):
        super().setUp()
        self.db = dbm_sqlite3.open(self.filename, "w")

    def tearDown(self):
        self.db.close()
        super().tearDown()

    def test_datatypes_values(self):
        for value in self.dataset:
            with self.subTest(value=value):
                self.db["key"] = value
                self.assertEqual(self.db["key"], value)

    def test_datatypes_keys(self):
        for key in self.dataset:
            with self.subTest(key=key):
                self.db[key] = "value"
                self.assertEqual(self.db[key], "value")


class CorruptDatabase(_SQLiteDbmTests):
    """Verify that database exceptions are raised as dbm.sqlite3.error."""

    def setUp(self):
        super().setUp()
        with closing(sqlite3.connect(self.filename)) as cx:
            with cx:
                cx.execute("DROP TABLE IF EXISTS Dict")
                cx.execute("CREATE TABLE Dict (invalid_schema)")

    def check(self, flag, fn, should_succeed=False):
        with closing(dbm_sqlite3.open(self.filename, flag)) as db:
            with self.assertRaises(dbm_sqlite3.error):
                fn(db)

    @staticmethod
    def read(db):
        return db["key"]

    @staticmethod
    def write(db):
        db["key"] = "value"

    @staticmethod
    def iter(db):
        next(iter(db))

    @staticmethod
    def keys(db):
        db.keys()

    @staticmethod
    def del_(db):
        del db["key"]

    @staticmethod
    def len_(db):
        len(db)

    def test_corrupt_readwrite(self):
        for flag in "r", "w", "c":
            with self.subTest(flag=flag):
                check = partial(self.check, flag=flag)
                check(fn=self.read)
                check(fn=self.write)
                check(fn=self.iter)
                check(fn=self.keys)
                check(fn=self.del_)
                check(fn=self.len_)

    def test_corrupt_force_new(self):
        with closing(dbm_sqlite3.open(self.filename, "n")) as db:
            db["foo"] = "write"
            _ = db["foo"]
            next(iter(db))
            del db["foo"]


if __name__ == "__main__":
    unittest.main()
