from test import support
from test.support import import_helper
from test.support import os_helper
import_helper.import_module("dbm.ndbm") #skip if not supported
import os
import unittest
import dbm.ndbm
from dbm.ndbm import error

class DbmTestCase(unittest.TestCase):

    def setUp(self):
        self.filename = os_helper.TESTFN
        self.d = dbm.ndbm.open(self.filename, 'c')
        self.d.close()

    def tearDown(self):
        for suffix in ['', '.pag', '.dir', '.db']:
            support.unlink(self.filename + suffix)

    def test_keys(self):
        self.d = dbm.ndbm.open(self.filename, 'c')
        self.assertEqual(self.d.keys(), [])
        self.d['a'] = 'b'
        self.d[b'bytes'] = b'data'
        self.d['12345678910'] = '019237410982340912840198242'
        self.d.keys()
        self.assertIn('a', self.d)
        self.assertIn(b'a', self.d)
        self.assertEqual(self.d[b'bytes'], b'data')
        # get() and setdefault() work as in the dict interface
        self.assertEqual(self.d.get(b'a'), b'b')
        self.assertIsNone(self.d.get(b'xxx'))
        self.assertEqual(self.d.get(b'xxx', b'foo'), b'foo')
        with self.assertRaises(KeyError):
            self.d['xxx']
        self.assertEqual(self.d.setdefault(b'xxx', b'foo'), b'foo')
        self.assertEqual(self.d[b'xxx'], b'foo')
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

    def test_bytes(self):
        with dbm.ndbm.open(self.filename, 'c') as db:
            db[b'bytes key \xbd'] = b'bytes value \xbd'
        with dbm.ndbm.open(self.filename, 'r') as db:
            self.assertEqual(list(db.keys()), [b'bytes key \xbd'])
            self.assertTrue(b'bytes key \xbd' in db)
            self.assertEqual(db[b'bytes key \xbd'], b'bytes value \xbd')

    def test_unicode(self):
        with dbm.ndbm.open(self.filename, 'c') as db:
            db['Unicode key \U0001f40d'] = 'Unicode value \U0001f40d'
        with dbm.ndbm.open(self.filename, 'r') as db:
            self.assertEqual(list(db.keys()), ['Unicode key \U0001f40d'.encode()])
            self.assertTrue('Unicode key \U0001f40d'.encode() in db)
            self.assertTrue('Unicode key \U0001f40d' in db)
            self.assertEqual(db['Unicode key \U0001f40d'.encode()],
                             'Unicode value \U0001f40d'.encode())
            self.assertEqual(db['Unicode key \U0001f40d'],
                             'Unicode value \U0001f40d'.encode())

    def test_write_readonly_file(self):
        with dbm.ndbm.open(self.filename, 'c') as db:
            db[b'bytes key'] = b'bytes value'
        with dbm.ndbm.open(self.filename, 'r') as db:
            with self.assertRaises(error):
                del db[b'not exist key']
            with self.assertRaises(error):
                del db[b'bytes key']
            with self.assertRaises(error):
                db[b'not exist key'] = b'not exist value'

    @unittest.skipUnless(os_helper.TESTFN_NONASCII,
                         'requires OS support of non-ASCII encodings')
    def test_nonascii_filename(self):
        filename = os_helper.TESTFN_NONASCII
        for suffix in ['', '.pag', '.dir', '.db']:
            self.addCleanup(support.unlink, filename + suffix)
        with dbm.ndbm.open(filename, 'c') as db:
            db[b'key'] = b'value'
        self.assertTrue(any(os.path.exists(filename + suffix)
                            for suffix in ['', '.pag', '.dir', '.db']))
        with dbm.ndbm.open(filename, 'r') as db:
            self.assertEqual(list(db.keys()), [b'key'])
            self.assertTrue(b'key' in db)
            self.assertEqual(db[b'key'], b'value')

    def test_nonexisting_file(self):
        nonexisting_file = 'nonexisting-file'
        with self.assertRaises(dbm.ndbm.error) as cm:
            dbm.ndbm.open(nonexisting_file)
        self.assertIn(nonexisting_file, str(cm.exception))
        self.assertEqual(cm.exception.filename, nonexisting_file)


if __name__ == '__main__':
    unittest.main()
