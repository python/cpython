#! /usr/bin/env python
"""Test script for the dumbdbm module
   Original by Roger E. Masse
"""

import os
import test_support
import unittest
import dumbdbm
import tempfile

_fname = tempfile.mktemp()

def _delete_files():
    for ext in [".dir", ".dat", ".bak"]:
        try:
            os.unlink(_fname + ext)
        except OSError:
            pass

class DumbDBMTestCase(unittest.TestCase):
    _dict = {'0': '',
             'a': 'Python:',
             'b': 'Programming',
             'c': 'the',
             'd': 'way',
             'f': 'Guido',
             'g': 'intended'
             }

    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def test_dumbdbm_creation(self):
        f = dumbdbm.open(_fname, 'c')
        self.assertEqual(f.keys(), [])
        for key in self._dict:
            f[key] = self._dict[key]
        self.read_helper(f)
        f.close()

    def test_dumbdbm_modification(self):
        self.init_db()
        f = dumbdbm.open(_fname, 'w')
        self._dict['g'] = f['g'] = "indented"
        self.read_helper(f)
        f.close()

    def test_dumbdbm_read(self):
        self.init_db()
        f = dumbdbm.open(_fname, 'r')
        self.read_helper(f)
        f.close()

    def test_dumbdbm_keys(self):
        self.init_db()
        f = dumbdbm.open(_fname)
        keys = self.keys_helper(f)
        f.close()

    def test_write_write_read(self):
        # test for bug #482460
        f = dumbdbm.open(_fname)
        f['1'] = 'hello'
        f['1'] = 'hello2'
        f.close()
        f = dumbdbm.open(_fname)
        self.assertEqual(f['1'], 'hello2')
        f.close()

    def read_helper(self, f):
        keys = self.keys_helper(f)
        for key in self._dict:
            self.assertEqual(self._dict[key], f[key])

    def init_db(self):
        f = dumbdbm.open(_fname, 'w')
        for k in self._dict:
            f[k] = self._dict[k]
        f.close()

    def keys_helper(self, f):
        keys = f.keys()
        keys.sort()
        dkeys = self._dict.keys()
        dkeys.sort()
        self.assertEqual(keys, dkeys)
        return keys

    def tearDown(self):
        _delete_files()

    def setUp(self):
        _delete_files()

def test_main():
    try:
        test_support.run_unittest(DumbDBMTestCase)
    finally:
        _delete_files()

if __name__ == "__main__":
    test_main()
