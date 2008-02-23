"""
TestCases for testing the locking sub-system.
"""

import shutil
import tempfile
import time

try:
    from threading import Thread, currentThread
    have_threads = 1
except ImportError:
    have_threads = 0


import unittest
from test_all import verbose

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db
except ImportError:
    # For Python 2.3
    from bsddb import db


#----------------------------------------------------------------------

class LockingTestCase(unittest.TestCase):

    def setUp(self):
        self.homeDir = tempfile.mkdtemp('.test_lock')
        self.env = db.DBEnv()
        self.env.open(self.homeDir, db.DB_THREAD | db.DB_INIT_MPOOL |
                                    db.DB_INIT_LOCK | db.DB_CREATE)


    def tearDown(self):
        self.env.close()
        shutil.rmtree(self.homeDir)


    def test01_simple(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test01_simple..." % self.__class__.__name__

        anID = self.env.lock_id()
        if verbose:
            print "locker ID: %s" % anID
        lock = self.env.lock_get(anID, "some locked thing", db.DB_LOCK_WRITE)
        if verbose:
            print "Aquired lock: %s" % lock
        time.sleep(1)
        self.env.lock_put(lock)
        if verbose:
            print "Released lock: %s" % lock
        if db.version() >= (4,0):
            self.env.lock_id_free(anID)


    def test02_threaded(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test02_threaded..." % self.__class__.__name__

        threads = []
        threads.append(Thread(target = self.theThread,
                              args=(5, db.DB_LOCK_WRITE)))
        threads.append(Thread(target = self.theThread,
                              args=(1, db.DB_LOCK_READ)))
        threads.append(Thread(target = self.theThread,
                              args=(1, db.DB_LOCK_READ)))
        threads.append(Thread(target = self.theThread,
                              args=(1, db.DB_LOCK_WRITE)))
        threads.append(Thread(target = self.theThread,
                              args=(1, db.DB_LOCK_READ)))
        threads.append(Thread(target = self.theThread,
                              args=(1, db.DB_LOCK_READ)))
        threads.append(Thread(target = self.theThread,
                              args=(1, db.DB_LOCK_WRITE)))
        threads.append(Thread(target = self.theThread,
                              args=(1, db.DB_LOCK_WRITE)))
        threads.append(Thread(target = self.theThread,
                              args=(1, db.DB_LOCK_WRITE)))

        for t in threads:
            t.start()
        for t in threads:
            t.join()

    def test03_set_timeout(self):
        # test that the set_timeout call works
        if hasattr(self.env, 'set_timeout'):
            self.env.set_timeout(0, db.DB_SET_LOCK_TIMEOUT)
            self.env.set_timeout(0, db.DB_SET_TXN_TIMEOUT)
            self.env.set_timeout(123456, db.DB_SET_LOCK_TIMEOUT)
            self.env.set_timeout(7890123, db.DB_SET_TXN_TIMEOUT)

    def theThread(self, sleepTime, lockType):
        name = currentThread().getName()
        if lockType ==  db.DB_LOCK_WRITE:
            lt = "write"
        else:
            lt = "read"

        anID = self.env.lock_id()
        if verbose:
            print "%s: locker ID: %s" % (name, anID)

        lock = self.env.lock_get(anID, "some locked thing", lockType)
        if verbose:
            print "%s: Aquired %s lock: %s" % (name, lt, lock)

        time.sleep(sleepTime)

        self.env.lock_put(lock)
        if verbose:
            print "%s: Released %s lock: %s" % (name, lt, lock)
        if db.version() >= (4,0):
            self.env.lock_id_free(anID)


#----------------------------------------------------------------------

def test_suite():
    suite = unittest.TestSuite()

    if have_threads:
        suite.addTest(unittest.makeSuite(LockingTestCase))
    else:
        suite.addTest(unittest.makeSuite(LockingTestCase, 'test01'))

    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
