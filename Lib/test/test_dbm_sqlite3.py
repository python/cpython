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


class Misuse(_SQLiteDbmTests):

    def setUp(self):
        super().setUp()
        self.db = dbm_sqlite3.open(self.filename, "w")

    def tearDown(self):
        super().tearDown()
        self.db.close()

    def test_double_close(self):
        self.db.close()

    def test_invalid_flag(self):
        with self.assertRaises(ValueError):
            dbm_sqlite3.open(self.filename, flag="invalid")

    def test_double_delete(self):
        self.db["key"] = "value"
        del self.db["key"]
        with self.assertRaises(KeyError):
            del self.db["key"]

    def test_invalid_key(self):
        with self.assertRaises(KeyError):
            self.db["key"]


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

    def test_readonly(self):
        check = partial(self.check, flag="r")
        check(fn=self.read)
        check(fn=self.iter)
        check(fn=self.keys)
        check(fn=self.del_)

    def test_readwrite(self):
        for flag in "w", "c":
            with self.subTest(flag=flag):
                check = partial(self.check, flag=flag)
                check(fn=self.read)
                check(fn=self.write)
                check(fn=self.iter)
                check(fn=self.keys)
                check(fn=self.del_)
                check(fn=self.len_)

    def test_new(self):
        with closing(dbm_sqlite3.open(self.filename, "n")) as db:
            db["foo"] = "write"
            _ = db["foo"]
            next(iter(db))
            del db["foo"]


if __name__ == "__main__":
    unittest.main()
