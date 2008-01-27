import unittest
import os
import shutil
import sys
import tempfile
import glob

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db
except ImportError:
    from bsddb import db

from bsddb.test.test_all import verbose


class DBSequenceTest(unittest.TestCase):
    def setUp(self):
        self.int_32_max = 0x100000000
        self.homeDir = os.path.join(tempfile.gettempdir(), 'db_home')
        try:
            os.mkdir(self.homeDir)
        except os.error:
            pass
        tempfile.tempdir = self.homeDir
        self.filename = os.path.split(tempfile.mktemp())[1]
        tempfile.tempdir = old_tempfile_tempdir

        self.dbenv = db.DBEnv()
        self.dbenv.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL, 0o666)
        self.d = db.DB(self.dbenv)
        self.d.open(self.filename, db.DB_BTREE, db.DB_CREATE, 0o666)

    def tearDown(self):
        if hasattr(self, 'seq'):
            self.seq.close()
            del self.seq
        if hasattr(self, 'd'):
            self.d.close()
            del self.d
        if hasattr(self, 'dbenv'):
            self.dbenv.close()
            del self.dbenv

        shutil.rmtree(self.homeDir)

    def test_get(self):
        self.seq = db.DBSequence(self.d, flags=0)
        start_value = 10 * self.int_32_max
        self.assertEqual(0xA00000000, start_value)
        self.assertEquals(None, self.seq.init_value(start_value))
        self.assertEquals(None, self.seq.open(key=b'id', txn=None, flags=db.DB_CREATE))
        self.assertEquals(start_value, self.seq.get(5))
        self.assertEquals(start_value + 5, self.seq.get())

    def test_remove(self):
        self.seq = db.DBSequence(self.d, flags=0)
        self.assertEquals(None, self.seq.open(key=b'foo', txn=None, flags=db.DB_CREATE))
        self.assertEquals(None, self.seq.remove(txn=None, flags=0))
        del self.seq

    def test_get_key(self):
        self.seq = db.DBSequence(self.d, flags=0)
        key = b'foo'
        self.assertEquals(None, self.seq.open(key=key, txn=None, flags=db.DB_CREATE))
        self.assertEquals(key, self.seq.get_key())

    def test_get_dbp(self):
        self.seq = db.DBSequence(self.d, flags=0)
        self.assertEquals(None, self.seq.open(key=b'foo', txn=None, flags=db.DB_CREATE))
        self.assertEquals(self.d, self.seq.get_dbp())

    def test_cachesize(self):
        self.seq = db.DBSequence(self.d, flags=0)
        cashe_size = 10
        self.assertEquals(None, self.seq.set_cachesize(cashe_size))
        self.assertEquals(None, self.seq.open(key=b'foo', txn=None, flags=db.DB_CREATE))
        self.assertEquals(cashe_size, self.seq.get_cachesize())

    def test_flags(self):
        self.seq = db.DBSequence(self.d, flags=0)
        flag = db.DB_SEQ_WRAP;
        self.assertEquals(None, self.seq.set_flags(flag))
        self.assertEquals(None, self.seq.open(key=b'foo', txn=None, flags=db.DB_CREATE))
        self.assertEquals(flag, self.seq.get_flags() & flag)

    def test_range(self):
        self.seq = db.DBSequence(self.d, flags=0)
        seq_range = (10 * self.int_32_max, 11 * self.int_32_max - 1)
        self.assertEquals(None, self.seq.set_range(seq_range))
        self.seq.init_value(seq_range[0])
        self.assertEquals(None, self.seq.open(key=b'foo', txn=None, flags=db.DB_CREATE))
        self.assertEquals(seq_range, self.seq.get_range())

    def test_stat(self):
        self.seq = db.DBSequence(self.d, flags=0)
        self.assertEquals(None, self.seq.open(key=b'foo', txn=None, flags=db.DB_CREATE))
        stat = self.seq.stat()
        for param in ('nowait', 'min', 'max', 'value', 'current',
                      'flags', 'cache_size', 'last_value', 'wait'):
            self.assertTrue(param in stat, "parameter %s isn't in stat info" % param)

def test_suite():
    suite = unittest.TestSuite()
    if db.version() >= (4,3):
        suite.addTest(unittest.makeSuite(DBSequenceTest))
    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
