from test import test_support
import unittest
import os
import random
import dbm
from dbm import error

class DbmTestCase(unittest.TestCase):

    def setUp(self):
        self.filename = test_support.TESTFN
        self.d = dbm.open(self.filename, 'c')
        self.d.close()

    def tearDown(self):
        for suffix in ['', '.pag', '.dir', '.db']:
            test_support.unlink(self.filename + suffix)

    def test_keys(self):
        self.d = dbm.open(self.filename, 'c')
        self.assert_(self.d.keys() == [])
        self.d['a'] = 'b'
        self.d['12345678910'] = '019237410982340912840198242'
        self.d.keys()
        self.assert_(b'a' in self.d)
        self.d.close()

    def test_modes(self):
        for mode in ['r', 'rw', 'w', 'n']:
            try:
                self.d = dbm.open(self.filename, mode)
                self.d.close()
            except dbm.error:
                self.fail()

def test_main():
    test_support.run_unittest(DbmTestCase)

if __name__ == '__main__':
    test_main()
