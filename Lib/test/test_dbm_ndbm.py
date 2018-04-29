from test import support
support.import_module("dbm.ndbm") #skip if not supported
import unittest
import os
import random
import dbm.ndbm
from dbm.ndbm import error

class DbmTestCase(unittest.TestCase):

    def setUp(self):
        self.filename = support.TESTFN
        self.d = dbm.ndbm.open(self.filename, 'c')
        self.d.close()

    def tearDown(self):
        for suffix in ['', '.pag', '.dir', '.db']:
            support.unlink(self.filename + suffix)

    def test_keys(self):
        self.d = dbm.ndbm.open(self.filename, 'c')
        self.assertTrue(self.d.keys() == [])
        self.d['a'] = 'b'
        self.d[b'bytes'] = b'data'
        self.d['12345678910'] = '019237410982340912840198242'
        self.d.keys()
        self.assertIn('a', self.d)
        self.assertIn(b'a', self.d)
        self.assertEqual(self.d[b'bytes'], b'data')
        self.d.close()

    def test_empty_value(self):
        if dbm.ndbm.library == 'Berkeley DB':
            self.skipTest("Berkeley DB doesn't distinguish the empty value "
                          "from the absent one")
        self.d = dbm.ndbm.open(self.filename, 'c')
        self.assertEqual(self.d.keys(), [])
        self.d['empty'] = ''
        self.assertEqual(self.d.keys(), [b'empty'])
        self.assertIn(b'empty', self.d)
        self.assertEqual(self.d[b'empty'], b'')
        self.assertEqual(self.d.get(b'empty'), b'')
        self.assertEqual(self.d.setdefault(b'empty'), b'')
        self.d.close()

    def test_modes(self):
        for mode in ['r', 'rw', 'w', 'n']:
            try:
                self.d = dbm.ndbm.open(self.filename, mode)
                self.d.close()
            except error:
                self.fail()

    def test_context_manager(self):
        with dbm.ndbm.open(self.filename, 'c') as db:
            db["ndbm context manager"] = "context manager"

        with dbm.ndbm.open(self.filename, 'r') as db:
            self.assertEqual(list(db.keys()), [b"ndbm context manager"])

        with self.assertRaises(dbm.ndbm.error) as cm:
            db.keys()
        self.assertEqual(str(cm.exception),
                         "DBM object has already been closed")


if __name__ == '__main__':
    unittest.main()
