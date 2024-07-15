"""Test script for the dbm.open function based on testdumbdbm.py"""

import unittest
import dbm
import os
from test.support import import_helper
from test.support import os_helper


try:
    from dbm import sqlite3 as dbm_sqlite3
except ImportError:
    dbm_sqlite3 = None


try:
    from dbm import ndbm
except ImportError:
    ndbm = None

dirname = os_helper.TESTFN
_fname = os.path.join(dirname, os_helper.TESTFN)

#
# Iterates over every database module supported by dbm currently available.
#
def dbm_iterator():
    for name in dbm._names:
        try:
            mod = __import__(name, fromlist=['open'])
        except ImportError:
            continue
        dbm._modules[name] = mod
        yield mod

#
# Clean up all scratch databases we might have created during testing
#
def cleaunup_test_dir():
    os_helper.rmtree(dirname)

def setup_test_dir():
    cleaunup_test_dir()
    os.mkdir(dirname)


class AnyDBMTestCase:
    _dict = {'a': b'Python:',
             'b': b'Programming',
             'c': b'the',
             'd': b'way',
             'f': b'Guido',
             'g': b'intended',
             }

    def init_db(self):
        f = dbm.open(_fname, 'n')
        for k in self._dict:
            f[k.encode("ascii")] = self._dict[k]
        f.close()

    def keys_helper(self, f):
        keys = sorted(k.decode("ascii") for k in f.keys())
        dkeys = sorted(self._dict.keys())
        self.assertEqual(keys, dkeys)
        return keys

    def test_error(self):
        self.assertTrue(issubclass(self.module.error, OSError))

    def test_anydbm_not_existing(self):
        self.assertRaises(dbm.error, dbm.open, _fname)

    def test_anydbm_creation(self):
        f = dbm.open(_fname, 'c')
        self.assertEqual(list(f.keys()), [])
        for key in self._dict:
            f[key.encode("ascii")] = self._dict[key]
        self.read_helper(f)
        f.close()

    def test_anydbm_creation_n_file_exists_with_invalid_contents(self):
        # create an empty file
        os_helper.create_empty_file(_fname)
        with dbm.open(_fname, 'n') as f:
            self.assertEqual(len(f), 0)

    def test_anydbm_modification(self):
        self.init_db()
        f = dbm.open(_fname, 'c')
        self._dict['g'] = f[b'g'] = b"indented"
        self.read_helper(f)
        # setdefault() works as in the dict interface
        self.assertEqual(f.setdefault(b'xxx', b'foo'), b'foo')
        self.assertEqual(f[b'xxx'], b'foo')
        f.close()

    def test_anydbm_read(self):
        self.init_db()
        f = dbm.open(_fname, 'r')
        self.read_helper(f)
        # get() works as in the dict interface
        self.assertEqual(f.get(b'a'), self._dict['a'])
        self.assertEqual(f.get(b'xxx', b'foo'), b'foo')
        self.assertIsNone(f.get(b'xxx'))
        with self.assertRaises(KeyError):
            f[b'xxx']
        f.close()

    def test_anydbm_keys(self):
        self.init_db()
        f = dbm.open(_fname, 'r')
        keys = self.keys_helper(f)
        f.close()

    def test_empty_value(self):
        if getattr(dbm._defaultmod, 'library', None) == 'Berkeley DB':
            self.skipTest("Berkeley DB doesn't distinguish the empty value "
                          "from the absent one")
        f = dbm.open(_fname, 'c')
        self.assertEqual(f.keys(), [])
        f[b'empty'] = b''
        self.assertEqual(f.keys(), [b'empty'])
        self.assertIn(b'empty', f)
        self.assertEqual(f[b'empty'], b'')
        self.assertEqual(f.get(b'empty'), b'')
        self.assertEqual(f.setdefault(b'empty'), b'')
        f.close()

    def test_anydbm_access(self):
        self.init_db()
        f = dbm.open(_fname, 'r')
        key = "a".encode("ascii")
        self.assertIn(key, f)
        assert(f[key] == b"Python:")
        f.close()

    def test_open_with_bytes(self):
        dbm.open(os.fsencode(_fname), "c").close()

    def test_open_with_pathlib_path(self):
        dbm.open(os_helper.FakePath(_fname), "c").close()

    def test_open_with_pathlib_path_bytes(self):
        dbm.open(os_helper.FakePath(os.fsencode(_fname)), "c").close()

    def read_helper(self, f):
        keys = self.keys_helper(f)
        for key in self._dict:
            self.assertEqual(self._dict[key], f[key.encode("ascii")])

    def test_keys(self):
        with dbm.open(_fname, 'c') as d:
            self.assertEqual(d.keys(), [])
            a = [(b'a', b'b'), (b'12345678910', b'019237410982340912840198242')]
            for k, v in a:
                d[k] = v
            self.assertEqual(sorted(d.keys()), sorted(k for (k, v) in a))
            for k, v in a:
                self.assertIn(k, d)
                self.assertEqual(d[k], v)
            self.assertNotIn(b'xxx', d)
            self.assertRaises(KeyError, lambda: d[b'xxx'])

    def test_clear(self):
        with dbm.open(_fname, 'c') as d:
            self.assertEqual(d.keys(), [])
            a = [(b'a', b'b'), (b'12345678910', b'019237410982340912840198242')]
            for k, v in a:
                d[k] = v
            for k, _ in a:
                self.assertIn(k, d)
            self.assertEqual(len(d), len(a))

            d.clear()
            self.assertEqual(len(d), 0)
            for k, _ in a:
                self.assertNotIn(k, d)

    def setUp(self):
        self.addCleanup(setattr, dbm, '_defaultmod', dbm._defaultmod)
        dbm._defaultmod = self.module
        self.addCleanup(cleaunup_test_dir)
        setup_test_dir()


class WhichDBTestCase(unittest.TestCase):
    def test_whichdb(self):
        self.addCleanup(setattr, dbm, '_defaultmod', dbm._defaultmod)
        _bytes_fname = os.fsencode(_fname)
        fnames = [_fname, os_helper.FakePath(_fname),
                  _bytes_fname, os_helper.FakePath(_bytes_fname)]
        for module in dbm_iterator():
            # Check whether whichdb correctly guesses module name
            # for databases opened with "module" module.
            name = module.__name__
            setup_test_dir()
            dbm._defaultmod = module
            # Try with empty files first
            with module.open(_fname, 'c'): pass
            for path in fnames:
                self.assertEqual(name, self.dbm.whichdb(path))
            # Now add a key
            with module.open(_fname, 'w') as f:
                f[b"1"] = b"1"
                # and test that we can find it
                self.assertIn(b"1", f)
                # and read it
                self.assertEqual(f[b"1"], b"1")
            for path in fnames:
                self.assertEqual(name, self.dbm.whichdb(path))

    @unittest.skipUnless(ndbm, reason='Test requires ndbm')
    def test_whichdb_ndbm(self):
        # Issue 17198: check that ndbm which is referenced in whichdb is defined
        with open(_fname + '.db', 'wb'): pass
        _bytes_fname = os.fsencode(_fname)
        fnames = [_fname, os_helper.FakePath(_fname),
                  _bytes_fname, os_helper.FakePath(_bytes_fname)]
        for path in fnames:
            self.assertIsNone(self.dbm.whichdb(path))

    @unittest.skipUnless(dbm_sqlite3, reason='Test requires dbm.sqlite3')
    def test_whichdb_sqlite3(self):
        # Databases created by dbm.sqlite3 are detected correctly.
        with dbm_sqlite3.open(_fname, "c") as db:
            db["key"] = "value"
        self.assertEqual(self.dbm.whichdb(_fname), "dbm.sqlite3")

    @unittest.skipUnless(dbm_sqlite3, reason='Test requires dbm.sqlite3')
    def test_whichdb_sqlite3_existing_db(self):
        # Existing sqlite3 databases are detected correctly.
        sqlite3 = import_helper.import_module("sqlite3")
        try:
            # Create an empty database.
            with sqlite3.connect(_fname) as cx:
                cx.execute("CREATE TABLE dummy(database)")
                cx.commit()
        finally:
            cx.close()
        self.assertEqual(self.dbm.whichdb(_fname), "dbm.sqlite3")


    def setUp(self):
        self.addCleanup(cleaunup_test_dir)
        setup_test_dir()
        self.dbm = import_helper.import_fresh_module('dbm')


for mod in dbm_iterator():
    assert mod.__name__.startswith('dbm.')
    suffix = mod.__name__[4:]
    testname = f'TestCase_{suffix}'
    globals()[testname] = type(testname,
                               (AnyDBMTestCase, unittest.TestCase),
                               {'module': mod})


if __name__ == "__main__":
    unittest.main()
