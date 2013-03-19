import unittest
import os, glob

from test_all import db, test_support, get_new_environment_path, \
        get_new_database_path

#----------------------------------------------------------------------

class DB(unittest.TestCase):
    def setUp(self):
        self.path = get_new_database_path()
        self.db = db.DB()

    def tearDown(self):
        self.db.close()
        del self.db
        test_support.unlink(self.path)

class DB_general(DB) :
    def test_get_open_flags(self) :
        self.db.open(self.path, dbtype=db.DB_HASH, flags = db.DB_CREATE)
        self.assertEqual(db.DB_CREATE, self.db.get_open_flags())

    def test_get_open_flags2(self) :
        self.db.open(self.path, dbtype=db.DB_HASH, flags = db.DB_CREATE |
                db.DB_THREAD)
        self.assertEqual(db.DB_CREATE | db.DB_THREAD, self.db.get_open_flags())

    def test_get_dbname_filename(self) :
        self.db.open(self.path, dbtype=db.DB_HASH, flags = db.DB_CREATE)
        self.assertEqual((self.path, None), self.db.get_dbname())

    def test_get_dbname_filename_database(self) :
        name = "jcea-random-name"
        self.db.open(self.path, dbname=name, dbtype=db.DB_HASH,
                flags = db.DB_CREATE)
        self.assertEqual((self.path, name), self.db.get_dbname())

        def test_bt_minkey(self) :
            for i in [17, 108, 1030] :
                self.db.set_bt_minkey(i)
                self.assertEqual(i, self.db.get_bt_minkey())

        def test_lorder(self) :
            self.db.set_lorder(1234)
            self.assertEqual(1234, self.db.get_lorder())
            self.db.set_lorder(4321)
            self.assertEqual(4321, self.db.get_lorder())
            self.assertRaises(db.DBInvalidArgError, self.db.set_lorder, 9182)

    if db.version() >= (4, 6) :
        def test_priority(self) :
            flags = [db.DB_PRIORITY_VERY_LOW, db.DB_PRIORITY_LOW,
                    db.DB_PRIORITY_DEFAULT, db.DB_PRIORITY_HIGH,
                    db.DB_PRIORITY_VERY_HIGH]
            for flag in flags :
                self.db.set_priority(flag)
                self.assertEqual(flag, self.db.get_priority())

    def test_get_transactional(self) :
        self.assertFalse(self.db.get_transactional())
        self.db.open(self.path, dbtype=db.DB_HASH, flags = db.DB_CREATE)
        self.assertFalse(self.db.get_transactional())

class DB_hash(DB) :
        def test_h_ffactor(self) :
            for ffactor in [4, 16, 256] :
                self.db.set_h_ffactor(ffactor)
                self.assertEqual(ffactor, self.db.get_h_ffactor())

        def test_h_nelem(self) :
            for nelem in [1, 2, 4] :
                nelem = nelem*1024*1024  # Millions
                self.db.set_h_nelem(nelem)
                self.assertEqual(nelem, self.db.get_h_nelem())

        def test_pagesize(self) :
            for i in xrange(9, 17) :  # From 512 to 65536
                i = 1<<i
                self.db.set_pagesize(i)
                self.assertEqual(i, self.db.get_pagesize())

            # The valid values goes from 512 to 65536
            # Test 131072 bytes...
            self.assertRaises(db.DBInvalidArgError, self.db.set_pagesize, 1<<17)
            # Test 256 bytes...
            self.assertRaises(db.DBInvalidArgError, self.db.set_pagesize, 1<<8)

class DB_txn(DB) :
    def setUp(self) :
        self.homeDir = get_new_environment_path()
        self.env = db.DBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL |
                db.DB_INIT_LOG | db.DB_INIT_TXN)
        self.db = db.DB(self.env)

    def tearDown(self) :
        self.db.close()
        del self.db
        self.env.close()
        del self.env
        test_support.rmtree(self.homeDir)

        def test_flags(self) :
            self.db.set_flags(db.DB_CHKSUM)
            self.assertEqual(db.DB_CHKSUM, self.db.get_flags())
            self.db.set_flags(db.DB_TXN_NOT_DURABLE)
            self.assertEqual(db.DB_TXN_NOT_DURABLE | db.DB_CHKSUM,
                    self.db.get_flags())

    def test_get_transactional(self) :
        self.assertFalse(self.db.get_transactional())
        # DB_AUTO_COMMIT = Implicit transaction
        self.db.open("XXX", dbtype=db.DB_HASH,
                flags = db.DB_CREATE | db.DB_AUTO_COMMIT)
        self.assertTrue(self.db.get_transactional())

class DB_recno(DB) :
        def test_re_pad(self) :
            for i in [' ', '*'] :  # Check chars
                self.db.set_re_pad(i)
                self.assertEqual(ord(i), self.db.get_re_pad())
            for i in [97, 65] :  # Check integers
                self.db.set_re_pad(i)
                self.assertEqual(i, self.db.get_re_pad())

        def test_re_delim(self) :
            for i in [' ', '*'] :  # Check chars
                self.db.set_re_delim(i)
                self.assertEqual(ord(i), self.db.get_re_delim())
            for i in [97, 65] :  # Check integers
                self.db.set_re_delim(i)
                self.assertEqual(i, self.db.get_re_delim())

        def test_re_source(self) :
            for i in ["test", "test2", "test3"] :
                self.db.set_re_source(i)
                self.assertEqual(i, self.db.get_re_source())

class DB_queue(DB) :
        def test_re_len(self) :
            for i in [33, 65, 300, 2000] :
                self.db.set_re_len(i)
                self.assertEqual(i, self.db.get_re_len())

        def test_q_extentsize(self) :
            for i in [1, 60, 100] :
                self.db.set_q_extentsize(i)
                self.assertEqual(i, self.db.get_q_extentsize())

def test_suite():
    suite = unittest.TestSuite()

    suite.addTest(unittest.makeSuite(DB_general))
    suite.addTest(unittest.makeSuite(DB_txn))
    suite.addTest(unittest.makeSuite(DB_hash))
    suite.addTest(unittest.makeSuite(DB_recno))
    suite.addTest(unittest.makeSuite(DB_queue))

    return suite

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
