#! /usr/bin/env python
"""Test script for the dbm.open function based on testdumbdbm.py"""

import os
import unittest
import glob
import test.support

# Skip tests if dbm module doesn't exist.
dbm = test.support.import_module('dbm')

_fname = test.support.TESTFN

#
# Iterates over every database module supported by dbm currently available,
# setting dbm to use each in turn, and yielding that module
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
def delete_files():
    # we don't know the precise name the underlying database uses
    # so we use glob to locate all names
    for f in glob.glob(_fname + "*"):
        test.support.unlink(f)


class AnyDBMTestCase(unittest.TestCase):
    _dict = {'0': b'',
             'a': b'Python:',
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
        self.assertTrue(issubclass(self.module.error, IOError))

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
        with open(_fname, "w") as w:
            pass # create an empty file

        f = dbm.open(_fname, 'n')
        self.addCleanup(f.close)
        self.assertEqual(len(f), 0)

    def test_anydbm_modification(self):
        self.init_db()
        f = dbm.open(_fname, 'c')
        self._dict['g'] = f[b'g'] = b"indented"
        self.read_helper(f)
        f.close()

    def test_anydbm_read(self):
        self.init_db()
        f = dbm.open(_fname, 'r')
        self.read_helper(f)
        f.close()

    def test_anydbm_keys(self):
        self.init_db()
        f = dbm.open(_fname, 'r')
        keys = self.keys_helper(f)
        f.close()

    def test_anydbm_access(self):
        self.init_db()
        f = dbm.open(_fname, 'r')
        key = "a".encode("ascii")
        assert(key in f)
        assert(f[key] == b"Python:")
        f.close()

    def read_helper(self, f):
        keys = self.keys_helper(f)
        for key in self._dict:
            self.assertEqual(self._dict[key], f[key.encode("ascii")])

    def tearDown(self):
        delete_files()

    def setUp(self):
        dbm._defaultmod = self.module
        delete_files()


class WhichDBTestCase(unittest.TestCase):
    # Actual test methods are added to namespace after class definition.
    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def test_whichdb(self):
        for module in dbm_iterator():
            # Check whether whichdb correctly guesses module name
            # for databases opened with "module" module.
            # Try with empty files first
            name = module.__name__
            if name == 'dbm.dumb':
                continue   # whichdb can't support dbm.dumb
            test.support.unlink(_fname)
            f = module.open(_fname, 'c')
            f.close()
            self.assertEqual(name, dbm.whichdb(_fname))
            # Now add a key
            f = module.open(_fname, 'w')
            f[b"1"] = b"1"
            # and test that we can find it
            self.assertTrue(b"1" in f)
            # and read it
            self.assertTrue(f[b"1"] == b"1")
            f.close()
            self.assertEqual(name, dbm.whichdb(_fname))

    def tearDown(self):
        delete_files()

    def setUp(self):
        delete_files()
        self.filename = test.support.TESTFN
        self.d = dbm.open(self.filename, 'c')
        self.d.close()

    def test_keys(self):
        self.d = dbm.open(self.filename, 'c')
        self.assertEqual(self.d.keys(), [])
        a = [(b'a', b'b'), (b'12345678910', b'019237410982340912840198242')]
        for k, v in a:
            self.d[k] = v
        self.assertEqual(sorted(self.d.keys()), sorted(k for (k, v) in a))
        for k, v in a:
            self.assertTrue(k in self.d)
            self.assertEqual(self.d[k], v)
        self.assertTrue(b'xxx' not in self.d)
        self.assertRaises(KeyError, lambda: self.d[b'xxx'])
        self.d.close()


def test_main():
    classes = [WhichDBTestCase]
    for mod in dbm_iterator():
        classes.append(type("TestCase-" + mod.__name__, (AnyDBMTestCase,),
                            {'module': mod}))
    test.support.run_unittest(*classes)

if __name__ == "__main__":
    test_main()
