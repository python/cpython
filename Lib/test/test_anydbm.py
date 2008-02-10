#! /usr/bin/env python
"""Test script for the anydbm module
   based on testdumbdbm.py
"""

import os
import unittest
import anydbm
import glob
from test import test_support

_fname = test_support.TESTFN

def _delete_files():
    # we don't know the precise name the underlying database uses
    # so we use glob to locate all names
    for f in glob.glob(_fname + "*"):
        try:
            os.unlink(f)
        except OSError:
            pass

class AnyDBMTestCase(unittest.TestCase):
    _dict = {'0': '',
             'a': 'Python:',
             'b': 'Programming',
             'c': 'the',
             'd': 'way',
             'f': 'Guido',
             'g': 'indented'
             }

    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def test_anydbm_creation(self):
        f = anydbm.open(_fname, 'c')
        self.assertEqual(f.keys(), [])
        for key in self._dict:
            f[key] = self._dict[key]
        self.read_helper(f)
        f.close()

    def test_anydbm_modification(self):
        self.init_db()
        f = anydbm.open(_fname, 'c')
        self._dict['g'] = f['g'] = "indented"
        self.read_helper(f)
        f.close()

    def test_anydbm_read(self):
        self.init_db()
        f = anydbm.open(_fname, 'r')
        self.read_helper(f)
        f.close()

    def test_anydbm_keys(self):
        self.init_db()
        f = anydbm.open(_fname, 'r')
        keys = self.keys_helper(f)
        f.close()

    def read_helper(self, f):
        keys = self.keys_helper(f)
        for key in self._dict:
            self.assertEqual(self._dict[key], f[key])

    def init_db(self):
        f = anydbm.open(_fname, 'n')
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
        test_support.run_unittest(AnyDBMTestCase)
    finally:
        _delete_files()

if __name__ == "__main__":
    test_main()
