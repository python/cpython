#! /usr/bin/env python
"""Test script for the whichdb module
   based on test_anydbm.py
"""

import os
import test.test_support
import unittest
import whichdb
import anydbm
import tempfile
import glob

_fname = test.test_support.TESTFN

def _delete_files():
    # we don't know the precise name the underlying database uses
    # so we use glob to locate all names
    for f in glob.glob(_fname + "*"):
        try:
            os.unlink(f)
        except OSError:
            pass

class WhichDBTestCase(unittest.TestCase):
    # Actual test methods are added to namespace
    # after class definition.
    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def tearDown(self):
        _delete_files()

    def setUp(self):
        _delete_files()

for name in anydbm._names:
    # we define a new test method for each
    # candidate database module.
    try:
        mod = __import__(name)
    except ImportError:
        continue

    def test_whichdb_name(self, name=name, mod=mod):
        # Check whether whichdb correctly guesses module name
        # for databases opened with module mod.
        # Try with empty files first
        f = mod.open(_fname, 'c')
        f.close()
        self.assertEqual(name, whichdb.whichdb(_fname))
        # Now add a key
        f = mod.open(_fname, 'w')
        f[b"1"] = b"1"
        f.close()
        self.assertEqual(name, whichdb.whichdb(_fname))
    setattr(WhichDBTestCase,"test_whichdb_%s" % name, test_whichdb_name)

def test_main():
    try:
        test.test_support.run_unittest(WhichDBTestCase)
    finally:
        _delete_files()

if __name__ == "__main__":
    test_main()
