import sqlite3
import test.support
import unittest
from contextlib import closing
from functools import partial
from test.support import cpython_only, import_helper, os_helper


dbm_sqlite3 = import_helper.import_module("dbm.sqlite3")


class _SQLiteDbmTests(unittest.TestCase):

    def setUp(self):
        self.filename = os_helper.TESTFN
        db = dbm_sqlite3.open(self.filename, "c")
        db.close()

    def tearDown(self):
        for suffix in "", "-wal", "-shm":
            os_helper.unlink(self.filename + suffix)


class DataTypes(_SQLiteDbmTests):

    dataset = 10, 2.5, "string", b"bytes"

    def setUp(self):
        super().setUp()
        self.db = dbm_sqlite3.open(self.filename, "w")

    def tearDown(self):
        super().tearDown()
        self.db.close()

    def test_values(self):
        for value in self.dataset:
            with self.subTest(value=value):
                self.db["key"] = value
                self.assertEqual(self.db["key"], value)

    def test_keys(self):
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

    def test_readonly(self):
        self.check(flag="r", fn=self.read)

    def test_readwrite(self):
        check = partial(self.check, flag="w")
        check(fn=self.read)
        check(fn=self.write)

    def test_create(self):
        check = partial(self.check, flag="c")
        check(fn=self.read)
        check(fn=self.write)

    def test_new(self):
        with closing(dbm_sqlite3.open(self.filename, "n")) as db:
            db["foo"] = "write"
            _ = db["foo"]


if __name__ == "__main__":
    unittest.main()
