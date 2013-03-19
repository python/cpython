import unittest
import os, glob

from test_all import db, test_support, get_new_environment_path, \
        get_new_database_path

#----------------------------------------------------------------------

class DBEnv(unittest.TestCase):
    def setUp(self):
        self.homeDir = get_new_environment_path()
        self.env = db.DBEnv()

    def tearDown(self):
        self.env.close()
        del self.env
        test_support.rmtree(self.homeDir)

class DBEnv_general(DBEnv) :
    def test_get_open_flags(self) :
        flags = db.DB_CREATE | db.DB_INIT_MPOOL
        self.env.open(self.homeDir, flags)
        self.assertEqual(flags, self.env.get_open_flags())

    def test_get_open_flags2(self) :
        flags = db.DB_CREATE | db.DB_INIT_MPOOL | \
                db.DB_INIT_LOCK | db.DB_THREAD
        self.env.open(self.homeDir, flags)
        self.assertEqual(flags, self.env.get_open_flags())

    if db.version() >= (4, 7) :
        def test_lk_partitions(self) :
            for i in [10, 20, 40] :
                self.env.set_lk_partitions(i)
                self.assertEqual(i, self.env.get_lk_partitions())

        def test_getset_intermediate_dir_mode(self) :
            self.assertEqual(None, self.env.get_intermediate_dir_mode())
            for mode in ["rwx------", "rw-rw-rw-", "rw-r--r--"] :
                self.env.set_intermediate_dir_mode(mode)
                self.assertEqual(mode, self.env.get_intermediate_dir_mode())
            self.assertRaises(db.DBInvalidArgError,
                    self.env.set_intermediate_dir_mode, "abcde")

    if db.version() >= (4, 6) :
        def test_thread(self) :
            for i in [16, 100, 1000] :
                self.env.set_thread_count(i)
                self.assertEqual(i, self.env.get_thread_count())

        def test_cache_max(self) :
            for size in [64, 128] :
                size = size*1024*1024  # Megabytes
                self.env.set_cache_max(0, size)
                size2 = self.env.get_cache_max()
                self.assertEqual(0, size2[0])
                self.assertTrue(size <= size2[1])
                self.assertTrue(2*size > size2[1])

    if db.version() >= (4, 4) :
        def test_mutex_stat(self) :
            self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL |
                    db.DB_INIT_LOCK)
            stat = self.env.mutex_stat()
            self.assertTrue("mutex_inuse_max" in stat)

        def test_lg_filemode(self) :
            for i in [0600, 0660, 0666] :
                self.env.set_lg_filemode(i)
                self.assertEqual(i, self.env.get_lg_filemode())

    def test_mp_max_openfd(self) :
        for i in [17, 31, 42] :
            self.env.set_mp_max_openfd(i)
            self.assertEqual(i, self.env.get_mp_max_openfd())

    def test_mp_max_write(self) :
        for i in [100, 200, 300] :
            for j in [1, 2, 3] :
                j *= 1000000
                self.env.set_mp_max_write(i, j)
                v=self.env.get_mp_max_write()
                self.assertEqual((i, j), v)

        def test_invalid_txn(self) :
            # This environment doesn't support transactions
            self.assertRaises(db.DBInvalidArgError, self.env.txn_begin)

        def test_mp_mmapsize(self) :
            for i in [16, 32, 64] :
                i *= 1024*1024
                self.env.set_mp_mmapsize(i)
                self.assertEqual(i, self.env.get_mp_mmapsize())

        def test_tmp_dir(self) :
            for i in ["a", "bb", "ccc"] :
                self.env.set_tmp_dir(i)
                self.assertEqual(i, self.env.get_tmp_dir())

        def test_flags(self) :
            self.env.set_flags(db.DB_AUTO_COMMIT, 1)
            self.assertEqual(db.DB_AUTO_COMMIT, self.env.get_flags())
            self.env.set_flags(db.DB_TXN_NOSYNC, 1)
            self.assertEqual(db.DB_AUTO_COMMIT | db.DB_TXN_NOSYNC,
                    self.env.get_flags())
            self.env.set_flags(db.DB_AUTO_COMMIT, 0)
            self.assertEqual(db.DB_TXN_NOSYNC, self.env.get_flags())
            self.env.set_flags(db.DB_TXN_NOSYNC, 0)
            self.assertEqual(0, self.env.get_flags())

        def test_lk_max_objects(self) :
            for i in [1000, 2000, 3000] :
                self.env.set_lk_max_objects(i)
                self.assertEqual(i, self.env.get_lk_max_objects())

        def test_lk_max_locks(self) :
            for i in [1000, 2000, 3000] :
                self.env.set_lk_max_locks(i)
                self.assertEqual(i, self.env.get_lk_max_locks())

        def test_lk_max_lockers(self) :
            for i in [1000, 2000, 3000] :
                self.env.set_lk_max_lockers(i)
                self.assertEqual(i, self.env.get_lk_max_lockers())

        def test_lg_regionmax(self) :
            for i in [128, 256, 1000] :
                i = i*1024*1024
                self.env.set_lg_regionmax(i)
                j = self.env.get_lg_regionmax()
                self.assertTrue(i <= j)
                self.assertTrue(2*i > j)

        def test_lk_detect(self) :
            flags= [db.DB_LOCK_DEFAULT, db.DB_LOCK_EXPIRE, db.DB_LOCK_MAXLOCKS,
                    db.DB_LOCK_MINLOCKS, db.DB_LOCK_MINWRITE,
                    db.DB_LOCK_OLDEST, db.DB_LOCK_RANDOM, db.DB_LOCK_YOUNGEST]

            flags.append(db.DB_LOCK_MAXWRITE)

            for i in flags :
                self.env.set_lk_detect(i)
                self.assertEqual(i, self.env.get_lk_detect())

        def test_lg_dir(self) :
            for i in ["a", "bb", "ccc", "dddd"] :
                self.env.set_lg_dir(i)
                self.assertEqual(i, self.env.get_lg_dir())

        def test_lg_bsize(self) :
            log_size = 70*1024
            self.env.set_lg_bsize(log_size)
            self.assertTrue(self.env.get_lg_bsize() >= log_size)
            self.assertTrue(self.env.get_lg_bsize() < 4*log_size)
            self.env.set_lg_bsize(4*log_size)
            self.assertTrue(self.env.get_lg_bsize() >= 4*log_size)

        def test_setget_data_dirs(self) :
            dirs = ("a", "b", "c", "d")
            for i in dirs :
                self.env.set_data_dir(i)
            self.assertEqual(dirs, self.env.get_data_dirs())

        def test_setget_cachesize(self) :
            cachesize = (0, 512*1024*1024, 3)
            self.env.set_cachesize(*cachesize)
            self.assertEqual(cachesize, self.env.get_cachesize())

            cachesize = (0, 1*1024*1024, 5)
            self.env.set_cachesize(*cachesize)
            cachesize2 = self.env.get_cachesize()
            self.assertEqual(cachesize[0], cachesize2[0])
            self.assertEqual(cachesize[2], cachesize2[2])
            # Berkeley DB expands the cache 25% accounting overhead,
            # if the cache is small.
            self.assertEqual(125, int(100.0*cachesize2[1]/cachesize[1]))

            # You can not change configuration after opening
            # the environment.
            self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)
            cachesize = (0, 2*1024*1024, 1)
            self.assertRaises(db.DBInvalidArgError,
                              self.env.set_cachesize, *cachesize)
            cachesize3 = self.env.get_cachesize()
            self.assertEqual(cachesize2[0], cachesize3[0])
            self.assertEqual(cachesize2[2], cachesize3[2])
            # In Berkeley DB 5.1, the cachesize can change when opening the Env
            self.assertTrue(cachesize2[1] <= cachesize3[1])

        def test_set_cachesize_dbenv_db(self) :
            # You can not configure the cachesize using
            # the database handle, if you are using an environment.
            d = db.DB(self.env)
            self.assertRaises(db.DBInvalidArgError,
                d.set_cachesize, 0, 1024*1024, 1)

        def test_setget_shm_key(self) :
            shm_key=137
            self.env.set_shm_key(shm_key)
            self.assertEqual(shm_key, self.env.get_shm_key())
            self.env.set_shm_key(shm_key+1)
            self.assertEqual(shm_key+1, self.env.get_shm_key())

            # You can not change configuration after opening
            # the environment.
            self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)
            # If we try to reconfigure cache after opening the
            # environment, core dump.
            self.assertRaises(db.DBInvalidArgError,
                self.env.set_shm_key, shm_key)
            self.assertEqual(shm_key+1, self.env.get_shm_key())

    if db.version() >= (4, 4) :
        def test_mutex_setget_max(self) :
            v = self.env.mutex_get_max()
            v2 = v*2+1

            self.env.mutex_set_max(v2)
            self.assertEqual(v2, self.env.mutex_get_max())

            self.env.mutex_set_max(v)
            self.assertEqual(v, self.env.mutex_get_max())

            # You can not change configuration after opening
            # the environment.
            self.env.open(self.homeDir, db.DB_CREATE)
            self.assertRaises(db.DBInvalidArgError,
                    self.env.mutex_set_max, v2)

        def test_mutex_setget_increment(self) :
            v = self.env.mutex_get_increment()
            v2 = 127

            self.env.mutex_set_increment(v2)
            self.assertEqual(v2, self.env.mutex_get_increment())

            self.env.mutex_set_increment(v)
            self.assertEqual(v, self.env.mutex_get_increment())

            # You can not change configuration after opening
            # the environment.
            self.env.open(self.homeDir, db.DB_CREATE)
            self.assertRaises(db.DBInvalidArgError,
                    self.env.mutex_set_increment, v2)

        def test_mutex_setget_tas_spins(self) :
            self.env.mutex_set_tas_spins(0)  # Default = BDB decides
            v = self.env.mutex_get_tas_spins()
            v2 = v*2+1

            self.env.mutex_set_tas_spins(v2)
            self.assertEqual(v2, self.env.mutex_get_tas_spins())

            self.env.mutex_set_tas_spins(v)
            self.assertEqual(v, self.env.mutex_get_tas_spins())

            # In this case, you can change configuration
            # after opening the environment.
            self.env.open(self.homeDir, db.DB_CREATE)
            self.env.mutex_set_tas_spins(v2)

        def test_mutex_setget_align(self) :
            v = self.env.mutex_get_align()
            v2 = 64
            if v == 64 :
                v2 = 128

            self.env.mutex_set_align(v2)
            self.assertEqual(v2, self.env.mutex_get_align())

            # Requires a nonzero power of two
            self.assertRaises(db.DBInvalidArgError,
                    self.env.mutex_set_align, 0)
            self.assertRaises(db.DBInvalidArgError,
                    self.env.mutex_set_align, 17)

            self.env.mutex_set_align(2*v2)
            self.assertEqual(2*v2, self.env.mutex_get_align())

            # You can not change configuration after opening
            # the environment.
            self.env.open(self.homeDir, db.DB_CREATE)
            self.assertRaises(db.DBInvalidArgError,
                    self.env.mutex_set_align, v2)


class DBEnv_log(DBEnv) :
    def setUp(self):
        DBEnv.setUp(self)
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL | db.DB_INIT_LOG)

    def test_log_file(self) :
        log_file = self.env.log_file((1, 1))
        self.assertEqual("log.0000000001", log_file[-14:])

    if db.version() >= (4, 4) :
        # The version with transactions is checked in other test object
        def test_log_printf(self) :
            msg = "This is a test..."
            self.env.log_printf(msg)
            logc = self.env.log_cursor()
            self.assertTrue(msg in (logc.last()[1]))

    if db.version() >= (4, 7) :
        def test_log_config(self) :
            self.env.log_set_config(db.DB_LOG_DSYNC | db.DB_LOG_ZERO, 1)
            self.assertTrue(self.env.log_get_config(db.DB_LOG_DSYNC))
            self.assertTrue(self.env.log_get_config(db.DB_LOG_ZERO))
            self.env.log_set_config(db.DB_LOG_ZERO, 0)
            self.assertTrue(self.env.log_get_config(db.DB_LOG_DSYNC))
            self.assertFalse(self.env.log_get_config(db.DB_LOG_ZERO))


class DBEnv_log_txn(DBEnv) :
    def setUp(self):
        DBEnv.setUp(self)
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL |
                db.DB_INIT_LOG | db.DB_INIT_TXN)

    if (db.version() >= (4, 5)) and (db.version() < (5, 2)) :
        def test_tx_max(self) :
            txns=[]
            def tx() :
                for i in xrange(self.env.get_tx_max()) :
                    txns.append(self.env.txn_begin())

            tx()
            self.assertRaises(MemoryError, tx)

            # Abort the transactions before garbage collection,
            # to avoid "warnings".
            for i in txns :
                i.abort()

    if db.version() >= (4, 4) :
        # The version without transactions is checked in other test object
        def test_log_printf(self) :
            msg = "This is a test..."
            txn = self.env.txn_begin()
            self.env.log_printf(msg, txn=txn)
            txn.commit()
            logc = self.env.log_cursor()
            logc.last()  # Skip the commit
            self.assertTrue(msg in (logc.prev()[1]))

            msg = "This is another test..."
            txn = self.env.txn_begin()
            self.env.log_printf(msg, txn=txn)
            txn.abort()  # Do not store the new message
            logc.last()  # Skip the abort
            self.assertTrue(msg not in (logc.prev()[1]))

            msg = "This is a third test..."
            txn = self.env.txn_begin()
            self.env.log_printf(msg, txn=txn)
            txn.commit()  # Do not store the new message
            logc.last()  # Skip the commit
            self.assertTrue(msg in (logc.prev()[1]))


class DBEnv_memp(DBEnv):
    def setUp(self):
        DBEnv.setUp(self)
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL | db.DB_INIT_LOG)
        self.db = db.DB(self.env)
        self.db.open("test", db.DB_HASH, db.DB_CREATE, 0660)

    def tearDown(self):
        self.db.close()
        del self.db
        DBEnv.tearDown(self)

    def test_memp_1_trickle(self) :
        self.db.put("hi", "bye")
        self.assertTrue(self.env.memp_trickle(100) > 0)

# Preserve the order, do "memp_trickle" test first
    def test_memp_2_sync(self) :
        self.db.put("hi", "bye")
        self.env.memp_sync()  # Full flush
        # Nothing to do...
        self.assertTrue(self.env.memp_trickle(100) == 0)

        self.db.put("hi", "bye2")
        self.env.memp_sync((1, 0))  # NOP, probably
        # Something to do... or not
        self.assertTrue(self.env.memp_trickle(100) >= 0)

        self.db.put("hi", "bye3")
        self.env.memp_sync((123, 99))  # Full flush
        # Nothing to do...
        self.assertTrue(self.env.memp_trickle(100) == 0)

    def test_memp_stat_1(self) :
        stats = self.env.memp_stat()  # No param
        self.assertTrue(len(stats)==2)
        self.assertTrue("cache_miss" in stats[0])
        stats = self.env.memp_stat(db.DB_STAT_CLEAR)  # Positional param
        self.assertTrue("cache_miss" in stats[0])
        stats = self.env.memp_stat(flags=0)  # Keyword param
        self.assertTrue("cache_miss" in stats[0])

    def test_memp_stat_2(self) :
        stats=self.env.memp_stat()[1]
        self.assertTrue(len(stats))==1
        self.assertTrue("test" in stats)
        self.assertTrue("page_in" in stats["test"])

class DBEnv_logcursor(DBEnv):
    def setUp(self):
        DBEnv.setUp(self)
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL |
                db.DB_INIT_LOG | db.DB_INIT_TXN)
        txn = self.env.txn_begin()
        self.db = db.DB(self.env)
        self.db.open("test", db.DB_HASH, db.DB_CREATE, 0660, txn=txn)
        txn.commit()
        for i in ["2", "8", "20"] :
            txn = self.env.txn_begin()
            self.db.put(key = i, data = i*int(i), txn=txn)
            txn.commit()

    def tearDown(self):
        self.db.close()
        del self.db
        DBEnv.tearDown(self)

    def _check_return(self, value) :
        self.assertTrue(isinstance(value, tuple))
        self.assertEqual(len(value), 2)
        self.assertTrue(isinstance(value[0], tuple))
        self.assertEqual(len(value[0]), 2)
        self.assertTrue(isinstance(value[0][0], int))
        self.assertTrue(isinstance(value[0][1], int))
        self.assertTrue(isinstance(value[1], str))

    # Preserve test order
    def test_1_first(self) :
        logc = self.env.log_cursor()
        v = logc.first()
        self._check_return(v)
        self.assertTrue((1, 1) < v[0])
        self.assertTrue(len(v[1])>0)

    def test_2_last(self) :
        logc = self.env.log_cursor()
        lsn_first = logc.first()[0]
        v = logc.last()
        self._check_return(v)
        self.assertTrue(lsn_first < v[0])

    def test_3_next(self) :
        logc = self.env.log_cursor()
        lsn_last = logc.last()[0]
        self.assertEqual(logc.next(), None)
        lsn_first = logc.first()[0]
        v = logc.next()
        self._check_return(v)
        self.assertTrue(lsn_first < v[0])
        self.assertTrue(lsn_last > v[0])

        v2 = logc.next()
        self.assertTrue(v2[0] > v[0])
        self.assertTrue(lsn_last > v2[0])

        v3 = logc.next()
        self.assertTrue(v3[0] > v2[0])
        self.assertTrue(lsn_last > v3[0])

    def test_4_prev(self) :
        logc = self.env.log_cursor()
        lsn_first = logc.first()[0]
        self.assertEqual(logc.prev(), None)
        lsn_last = logc.last()[0]
        v = logc.prev()
        self._check_return(v)
        self.assertTrue(lsn_first < v[0])
        self.assertTrue(lsn_last > v[0])

        v2 = logc.prev()
        self.assertTrue(v2[0] < v[0])
        self.assertTrue(lsn_first < v2[0])

        v3 = logc.prev()
        self.assertTrue(v3[0] < v2[0])
        self.assertTrue(lsn_first < v3[0])

    def test_5_current(self) :
        logc = self.env.log_cursor()
        logc.first()
        v = logc.next()
        self.assertEqual(v, logc.current())

    def test_6_set(self) :
        logc = self.env.log_cursor()
        logc.first()
        v = logc.next()
        self.assertNotEqual(v, logc.next())
        self.assertNotEqual(v, logc.next())
        self.assertEqual(v, logc.set(v[0]))

    def test_explicit_close(self) :
        logc = self.env.log_cursor()
        logc.close()
        self.assertRaises(db.DBCursorClosedError, logc.next)

    def test_implicit_close(self) :
        logc =  [self.env.log_cursor() for i in xrange(10)]
        self.env.close()  # This close should close too all its tree
        for i in logc :
            self.assertRaises(db.DBCursorClosedError, i.next)

def test_suite():
    suite = unittest.TestSuite()

    suite.addTest(unittest.makeSuite(DBEnv_general))
    suite.addTest(unittest.makeSuite(DBEnv_memp))
    suite.addTest(unittest.makeSuite(DBEnv_logcursor))
    suite.addTest(unittest.makeSuite(DBEnv_log))
    suite.addTest(unittest.makeSuite(DBEnv_log_txn))

    return suite

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
