from test import support
gdbm = support.import_module("dbm.gnu") #skip if not supported
import unittest
import os
from test.support import TESTFN, TESTFN_NONASCII, unlink


class TestGdbm(unittest.TestCase):
    def setUp(self):
        self.filename = TESTFN
        self.g = None
        self.assertFalse(os.path.exists(self.filename))

    def tearDown(self):
        if self.g is not None:
            self.g.close()
        unlink(self.filename)

    def test_key_methods(self):
        self.g = gdbm.open(self.filename, 'c')
        self.assertEqual(self.g.keys(), [])
        self.g['a'] = 'b'
        self.g['12345678910'] = '019237410982340912840198242'
        self.g[b'bytes'] = b'data'
        key_set = set(self.g.keys())
        self.assertEqual(key_set, set([b'a', b'bytes', b'12345678910']))
        self.assertIn('a', self.g)
        self.assertIn(b'a', self.g)
        self.assertEqual(self.g[b'bytes'], b'data')
        key = self.g.firstkey()
        while key:
            self.assertIn(key, key_set)
            key_set.remove(key)
            key = self.g.nextkey(key)
        # get() and setdefault() work as in the dict interface
        self.assertEqual(self.g.get(b'a'), b'b')
        self.assertIsNone(self.g.get(b'xxx'))
        self.assertEqual(self.g.get(b'xxx', b'foo'), b'foo')
        with self.assertRaises(KeyError):
            self.g['xxx']
        self.assertEqual(self.g.setdefault(b'xxx', b'foo'), b'foo')
        self.assertEqual(self.g[b'xxx'], b'foo')

    def test_error_conditions(self):
        # setUp() ensures that the filename doesn't exist
        with self.assertRaises(gdbm.error):
            gdbm.open(self.filename, 'r')
        # Try to access a closed database.
        self.g = gdbm.open(self.filename, 'c')
        self.g.close()
        with self.assertRaises(gdbm.error):
            self.g['a']
        # try pass an invalid open flag
        with self.assertRaises(gdbm.error):
            gdbm.open(self.filename, 'rx')

    def test_flags(self):
        # Test the flag parameter open() by trying all supported flag modes.
        all = set(gdbm.open_flags)
        # Test standard flags (presumably "crwn").
        modes = all - set('fsu')
        for mode in sorted(modes):  # put "c" mode first
            self.g = gdbm.open(self.filename, mode)
            self.g.close()

        # Test additional flags (presumably "fsu").
        flags = all - set('crwn')
        for mode in modes:
            for flag in flags:
                self.g = gdbm.open(self.filename, mode + flag)
                self.g.close()

    def test_reorganize(self):
        self.g = gdbm.open(self.filename, 'c')
        size0 = os.path.getsize(self.filename)

        self.g['x'] = 'x' * 10000
        size1 = os.path.getsize(self.filename)
        self.assertGreater(size1, size0)

        del self.g['x']
        # 'size' is supposed to be the same even after deleting an entry.
        self.assertEqual(os.path.getsize(self.filename), size1)

        self.g.reorganize()
        size2 = os.path.getsize(self.filename)
        self.assertLess(size2, size1)
        self.assertGreaterEqual(size2, size0)

    def test_context_manager(self):
        with gdbm.open(self.filename, 'c') as db:
            db["gdbm context manager"] = "context manager"

        with gdbm.open(self.filename, 'r') as db:
            self.assertEqual(list(db.keys()), [b"gdbm context manager"])

        with self.assertRaises(gdbm.error) as cm:
            db.keys()
        self.assertEqual(str(cm.exception),
                         "GDBM object has already been closed")

    def test_bytes(self):
        with gdbm.open(self.filename, 'c') as db:
            db[b'bytes key \xbd'] = b'bytes value \xbd'

        with gdbm.open(self.filename, 'r') as db:
            self.assertEqual(list(db.keys()), [b'bytes key \xbd'])
            self.assertTrue(b'bytes key \xbd' in db)
            self.assertEqual(db[b'bytes key \xbd'], b'bytes value \xbd')

    def test_unicode(self):
        with gdbm.open(self.filename, 'c') as db:
            db['Unicode key \U0001f40d'] = 'Unicode value \U0001f40d'

        with gdbm.open(self.filename, 'r') as db:
            self.assertEqual(list(db.keys()), ['Unicode key \U0001f40d'.encode()])
            self.assertTrue('Unicode key \U0001f40d'.encode() in db)
            self.assertTrue('Unicode key \U0001f40d' in db)
            self.assertEqual(db['Unicode key \U0001f40d'.encode()],
                             'Unicode value \U0001f40d'.encode())
            self.assertEqual(db['Unicode key \U0001f40d'],
                             'Unicode value \U0001f40d'.encode())

    @unittest.skipUnless(TESTFN_NONASCII,
                         'requires OS support of non-ASCII encodings')
    def test_nonascii_filename(self):
        self.filename = TESTFN_NONASCII
        self.assertFalse(os.path.exists(self.filename))

        with gdbm.open(self.filename, 'c') as db:
            db[b'key'] = b'value'

        self.assertTrue(os.path.exists(self.filename))
        with gdbm.open(self.filename, 'r') as db:
            self.assertEqual(list(db.keys()), [b'key'])
            self.assertTrue(b'key' in db)
            self.assertEqual(db[b'key'], b'value')


if __name__ == '__main__':
    unittest.main()
