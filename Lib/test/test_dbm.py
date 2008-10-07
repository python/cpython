from test import test_support
import unittest
import dbm

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
        self.assertEqual(self.d.keys(), [])
        a = [('a', 'b'), ('12345678910', '019237410982340912840198242')]
        for k, v in a:
            self.d[k] = v
        self.assertEqual(sorted(self.d.keys()), sorted(k for (k, v) in a))
        for k, v in a:
            self.assert_(k in self.d)
            self.assertEqual(self.d[k], v)
        self.assert_('xxx' not in self.d)
        self.assertRaises(KeyError, lambda: self.d['xxx'])
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
