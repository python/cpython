import unittest
import os

from test_all import db, test_support, get_new_environment_path, get_new_database_path


class DBSequenceTest(unittest.TestCase):
    import sys
    if sys.version_info < (2, 4) :
        def assertTrue(self, expr, msg=None):
            self.failUnless(expr,msg=msg)

    def setUp(self):
        self.int_32_max = 0x100000000
        self.homeDir = get_new_environment_path()
        self.filename = "test"

        self.dbenv = db.DBEnv()
        self.dbenv.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL, 0666)
        self.d = db.DB(self.dbenv)
        self.d.open(self.filename, db.DB_BTREE, db.DB_CREATE, 0666)

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

        test_support.rmtree(self.homeDir)

    def test_get(self):
        self.seq = db.DBSequence(self.d, flags=0)
        start_value = 10 * self.int_32_max
        self.assertEqual(0xA00000000, start_value)
        self.assertEqual(None, self.seq.initial_value(start_value))
        self.assertEqual(None, self.seq.open(key='id', txn=None, flags=db.DB_CREATE))
        self.assertEqual(start_value, self.seq.get(5))
        self.assertEqual(start_value + 5, self.seq.get())

    def test_remove(self):
        self.seq = db.DBSequence(self.d, flags=0)
        self.assertEqual(None, self.seq.open(key='foo', txn=None, flags=db.DB_CREATE))
        self.assertEqual(None, self.seq.remove(txn=None, flags=0))
        del self.seq

    def test_get_key(self):
        self.seq = db.DBSequence(self.d, flags=0)
        key = 'foo'
        self.assertEqual(None, self.seq.open(key=key, txn=None, flags=db.DB_CREATE))
        self.assertEqual(key, self.seq.get_key())

    def test_get_dbp(self):
        self.seq = db.DBSequence(self.d, flags=0)
        self.assertEqual(None, self.seq.open(key='foo', txn=None, flags=db.DB_CREATE))
        self.assertEqual(self.d, self.seq.get_dbp())

    def test_cachesize(self):
        self.seq = db.DBSequence(self.d, flags=0)
        cashe_size = 10
        self.assertEqual(None, self.seq.set_cachesize(cashe_size))
        self.assertEqual(None, self.seq.open(key='foo', txn=None, flags=db.DB_CREATE))
        self.assertEqual(cashe_size, self.seq.get_cachesize())

    def test_flags(self):
        self.seq = db.DBSequence(self.d, flags=0)
        flag = db.DB_SEQ_WRAP;
        self.assertEqual(None, self.seq.set_flags(flag))
        self.assertEqual(None, self.seq.open(key='foo', txn=None, flags=db.DB_CREATE))
        self.assertEqual(flag, self.seq.get_flags() & flag)

    def test_range(self):
        self.seq = db.DBSequence(self.d, flags=0)
        seq_range = (10 * self.int_32_max, 11 * self.int_32_max - 1)
        self.assertEqual(None, self.seq.set_range(seq_range))
        self.seq.initial_value(seq_range[0])
        self.assertEqual(None, self.seq.open(key='foo', txn=None, flags=db.DB_CREATE))
        self.assertEqual(seq_range, self.seq.get_range())

    def test_stat(self):
        self.seq = db.DBSequence(self.d, flags=0)
        self.assertEqual(None, self.seq.open(key='foo', txn=None, flags=db.DB_CREATE))
        stat = self.seq.stat()
        for param in ('nowait', 'min', 'max', 'value', 'current',
                      'flags', 'cache_size', 'last_value', 'wait'):
            self.assertTrue(param in stat, "parameter %s isn't in stat info" % param)

    if db.version() >= (4,7) :
        # This code checks a crash solved in Berkeley DB 4.7
        def test_stat_crash(self) :
            d=db.DB()
            d.open(None,dbtype=db.DB_HASH,flags=db.DB_CREATE)  # In RAM
            seq = db.DBSequence(d, flags=0)

            self.assertRaises(db.DBNotFoundError, seq.open,
                    key='id', txn=None, flags=0)

            self.assertRaises(db.DBInvalidArgError, seq.stat)

            d.close()

    def test_64bits(self) :
        # We don't use both extremes because they are problematic
        value_plus=(1L<<63)-2
        self.assertEqual(9223372036854775806L,value_plus)
        value_minus=(-1L<<63)+1  # Two complement
        self.assertEqual(-9223372036854775807L,value_minus)
        self.seq = db.DBSequence(self.d, flags=0)
        self.assertEqual(None, self.seq.initial_value(value_plus-1))
        self.assertEqual(None, self.seq.open(key='id', txn=None,
            flags=db.DB_CREATE))
        self.assertEqual(value_plus-1, self.seq.get(1))
        self.assertEqual(value_plus, self.seq.get(1))

        self.seq.remove(txn=None, flags=0)

        self.seq = db.DBSequence(self.d, flags=0)
        self.assertEqual(None, self.seq.initial_value(value_minus))
        self.assertEqual(None, self.seq.open(key='id', txn=None,
            flags=db.DB_CREATE))
        self.assertEqual(value_minus, self.seq.get(1))
        self.assertEqual(value_minus+1, self.seq.get(1))

    def test_multiple_close(self):
        self.seq = db.DBSequence(self.d)
        self.seq.close()  # You can close a Sequence multiple times
        self.seq.close()
        self.seq.close()

def test_suite():
    suite = unittest.TestSuite()
    if db.version() >= (4,3):
        suite.addTest(unittest.makeSuite(DBSequenceTest))
    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
