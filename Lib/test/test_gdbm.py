import unittest
import os
from test.test_support import TESTFN, run_unittest, unlink, import_module
gdbm = import_module('gdbm')


filename = TESTFN

class TestGdbm(unittest.TestCase):

    def setUp(self):
        self.g = None

    def tearDown(self):
        if self.g is not None:
            self.g.close()
        unlink(filename)

    def test_key_methods(self):
        self.g = gdbm.open(filename, 'c')
        self.assertEqual(self.g.keys(), [])
        self.g['a'] = 'b'
        self.g['12345678910'] = '019237410982340912840198242'
        key_set = set(self.g.keys())
        self.assertEqual(key_set, frozenset(['a', '12345678910']))
        self.assertTrue(self.g.has_key('a'))
        key = self.g.firstkey()
        while key:
            self.assertIn(key, key_set)
            key_set.remove(key)
            key = self.g.nextkey(key)
        self.assertRaises(KeyError, lambda: self.g['xxx'])

    def test_error_conditions(self):
        # Try to open a non-existent database.
        unlink(filename)
        self.assertRaises(gdbm.error, gdbm.open, filename, 'r')
        # Try to access a closed database.
        self.g = gdbm.open(filename, 'c')
        self.g.close()
        self.assertRaises(gdbm.error, lambda: self.g['a'])
        # try pass an invalid open flag
        self.assertRaises(gdbm.error, lambda: gdbm.open(filename, 'rx').close())

    def test_flags(self):
        # Test the flag parameter open() by trying all supported flag modes.
        all = set(gdbm.open_flags)
        # Test standard flags (presumably "crwn").
        modes = all - set('fsu')
        for mode in modes:
            self.g = gdbm.open(filename, mode)
            self.g.close()

        # Test additional flags (presumably "fsu").
        flags = all - set('crwn')
        for mode in modes:
            for flag in flags:
                self.g = gdbm.open(filename, mode + flag)
                self.g.close()

    def test_reorganize(self):
        self.g = gdbm.open(filename, 'c')
        size0 = os.path.getsize(filename)

        self.g['x'] = 'x' * 10000
        size1 = os.path.getsize(filename)
        self.assertTrue(size0 < size1)

        del self.g['x']
        # 'size' is supposed to be the same even after deleting an entry.
        self.assertEqual(os.path.getsize(filename), size1)

        self.g.reorganize()
        size2 = os.path.getsize(filename)
        self.assertTrue(size1 > size2 >= size0)


def test_main():
    run_unittest(TestGdbm)

if __name__ == '__main__':
    test_main()
