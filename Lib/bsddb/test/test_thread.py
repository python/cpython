"""TestCases for multi-threaded access to a DB.
"""

import os
import sys
import time
import errno
import shutil
import tempfile
from pprint import pprint
from random import random

try:
    True, False
except NameError:
    True = 1
    False = 0

DASH = '-'

try:
    from threading import Thread, currentThread
    have_threads = True
except ImportError:
    have_threads = False

try:
    WindowsError
except NameError:
    class WindowsError(Exception):
        pass

import unittest
from test_all import verbose

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db, dbutils
except ImportError:
    # For Python 2.3
    from bsddb import db, dbutils


#----------------------------------------------------------------------

class BaseThreadedTestCase(unittest.TestCase):
    dbtype       = db.DB_UNKNOWN  # must be set in derived class
    dbopenflags  = 0
    dbsetflags   = 0
    envflags     = 0

    def setUp(self):
        if verbose:
            dbutils._deadlock_VerboseFile = sys.stdout

        homeDir = os.path.join(os.path.dirname(sys.argv[0]), 'db_home')
        self.homeDir = homeDir
        try:
            os.mkdir(homeDir)
        except OSError, e:
            if e.errno <> errno.EEXIST: raise
        self.env = db.DBEnv()
        self.setEnvOpts()
        self.env.open(homeDir, self.envflags | db.DB_CREATE)

        self.filename = self.__class__.__name__ + '.db'
        self.d = db.DB(self.env)
        if self.dbsetflags:
            self.d.set_flags(self.dbsetflags)
        self.d.open(self.filename, self.dbtype, self.dbopenflags|db.DB_CREATE)

    def tearDown(self):
        self.d.close()
        self.env.close()
        shutil.rmtree(self.homeDir)

    def setEnvOpts(self):
        pass

    def makeData(self, key):
        return DASH.join([key] * 5)


#----------------------------------------------------------------------


class ConcurrentDataStoreBase(BaseThreadedTestCase):
    dbopenflags = db.DB_THREAD
    envflags    = db.DB_THREAD | db.DB_INIT_CDB | db.DB_INIT_MPOOL
    readers     = 0 # derived class should set
    writers     = 0
    records     = 1000

    def test01_1WriterMultiReaders(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test01_1WriterMultiReaders..." % \
                  self.__class__.__name__

        threads = []
        for x in range(self.writers):
            wt = Thread(target = self.writerThread,
                        args = (self.d, self.records, x),
                        name = 'writer %d' % x,
                        )#verbose = verbose)
            threads.append(wt)

        for x in range(self.readers):
            rt = Thread(target = self.readerThread,
                        args = (self.d, x),
                        name = 'reader %d' % x,
                        )#verbose = verbose)
            threads.append(rt)

        for t in threads:
            t.start()
        for t in threads:
            t.join()

    def writerThread(self, d, howMany, writerNum):
        #time.sleep(0.01 * writerNum + 0.01)
        name = currentThread().getName()
        start = howMany * writerNum
        stop = howMany * (writerNum + 1) - 1
        if verbose:
            print "%s: creating records %d - %d" % (name, start, stop)

        for x in range(start, stop):
            key = '%04d' % x
            dbutils.DeadlockWrap(d.put, key, self.makeData(key),
                                 max_retries=12)
            if verbose and x % 100 == 0:
                print "%s: records %d - %d finished" % (name, start, x)

        if verbose:
            print "%s: finished creating records" % name

##         # Each write-cursor will be exclusive, the only one that can update the DB...
##         if verbose: print "%s: deleting a few records" % name
##         c = d.cursor(flags = db.DB_WRITECURSOR)
##         for x in range(10):
##             key = int(random() * howMany) + start
##             key = '%04d' % key
##             if d.has_key(key):
##                 c.set(key)
##                 c.delete()

##         c.close()
        if verbose:
            print "%s: thread finished" % name

    def readerThread(self, d, readerNum):
        time.sleep(0.01 * readerNum)
        name = currentThread().getName()

        for loop in range(5):
            c = d.cursor()
            count = 0
            rec = c.first()
            while rec:
                count += 1
                key, data = rec
                self.assertEqual(self.makeData(key), data)
                rec = c.next()
            if verbose:
                print "%s: found %d records" % (name, count)
            c.close()
            time.sleep(0.05)

        if verbose:
            print "%s: thread finished" % name


class BTreeConcurrentDataStore(ConcurrentDataStoreBase):
    dbtype  = db.DB_BTREE
    writers = 2
    readers = 10
    records = 1000


class HashConcurrentDataStore(ConcurrentDataStoreBase):
    dbtype  = db.DB_HASH
    writers = 2
    readers = 10
    records = 1000


#----------------------------------------------------------------------

class SimpleThreadedBase(BaseThreadedTestCase):
    dbopenflags = db.DB_THREAD
    envflags    = db.DB_THREAD | db.DB_INIT_MPOOL | db.DB_INIT_LOCK
    readers = 5
    writers = 3
    records = 1000

    def setEnvOpts(self):
        self.env.set_lk_detect(db.DB_LOCK_DEFAULT)

    def test02_SimpleLocks(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test02_SimpleLocks..." % self.__class__.__name__

        threads = []
        for x in range(self.writers):
            wt = Thread(target = self.writerThread,
                        args = (self.d, self.records, x),
                        name = 'writer %d' % x,
                        )#verbose = verbose)
            threads.append(wt)
        for x in range(self.readers):
            rt = Thread(target = self.readerThread,
                        args = (self.d, x),
                        name = 'reader %d' % x,
                        )#verbose = verbose)
            threads.append(rt)

        for t in threads:
            t.start()
        for t in threads:
            t.join()

    def writerThread(self, d, howMany, writerNum):
        name = currentThread().getName()
        start = howMany * writerNum
        stop = howMany * (writerNum + 1) - 1
        if verbose:
            print "%s: creating records %d - %d" % (name, start, stop)

        # create a bunch of records
        for x in xrange(start, stop):
            key = '%04d' % x
            dbutils.DeadlockWrap(d.put, key, self.makeData(key),
                                 max_retries=12)

            if verbose and x % 100 == 0:
                print "%s: records %d - %d finished" % (name, start, x)

            # do a bit or reading too
            if random() <= 0.05:
                for y in xrange(start, x):
                    key = '%04d' % x
                    data = dbutils.DeadlockWrap(d.get, key, max_retries=12)
                    self.assertEqual(data, self.makeData(key))

        # flush them
        try:
            dbutils.DeadlockWrap(d.sync, max_retries=12)
        except db.DBIncompleteError, val:
            if verbose:
                print "could not complete sync()..."

        # read them back, deleting a few
        for x in xrange(start, stop):
            key = '%04d' % x
            data = dbutils.DeadlockWrap(d.get, key, max_retries=12)
            if verbose and x % 100 == 0:
                print "%s: fetched record (%s, %s)" % (name, key, data)
            self.assertEqual(data, self.makeData(key))
            if random() <= 0.10:
                dbutils.DeadlockWrap(d.delete, key, max_retries=12)
                if verbose:
                    print "%s: deleted record %s" % (name, key)

        if verbose:
            print "%s: thread finished" % name

    def readerThread(self, d, readerNum):
        time.sleep(0.01 * readerNum)
        name = currentThread().getName()

        for loop in range(5):
            c = d.cursor()
            count = 0
            rec = dbutils.DeadlockWrap(c.first, max_retries=10)
            while rec:
                count += 1
                key, data = rec
                self.assertEqual(self.makeData(key), data)
                rec = dbutils.DeadlockWrap(c.next, max_retries=10)
            if verbose:
                print "%s: found %d records" % (name, count)
            c.close()
            time.sleep(0.05)

        if verbose:
            print "%s: thread finished" % name


class BTreeSimpleThreaded(SimpleThreadedBase):
    dbtype = db.DB_BTREE


class HashSimpleThreaded(SimpleThreadedBase):
    dbtype = db.DB_HASH


#----------------------------------------------------------------------


class ThreadedTransactionsBase(BaseThreadedTestCase):
    dbopenflags = db.DB_THREAD | db.DB_AUTO_COMMIT
    envflags    = (db.DB_THREAD |
                   db.DB_INIT_MPOOL |
                   db.DB_INIT_LOCK |
                   db.DB_INIT_LOG |
                   db.DB_INIT_TXN
                   )
    readers = 0
    writers = 0
    records = 2000
    txnFlag = 0

    def setEnvOpts(self):
        #self.env.set_lk_detect(db.DB_LOCK_DEFAULT)
        pass

    def test03_ThreadedTransactions(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test03_ThreadedTransactions..." % \
                  self.__class__.__name__

        threads = []
        for x in range(self.writers):
            wt = Thread(target = self.writerThread,
                        args = (self.d, self.records, x),
                        name = 'writer %d' % x,
                        )#verbose = verbose)
            threads.append(wt)

        for x in range(self.readers):
            rt = Thread(target = self.readerThread,
                        args = (self.d, x),
                        name = 'reader %d' % x,
                        )#verbose = verbose)
            threads.append(rt)

        dt = Thread(target = self.deadlockThread)
        dt.start()

        for t in threads:
            t.start()
        for t in threads:
            t.join()

        self.doLockDetect = False
        dt.join()

    def doWrite(self, d, name, start, stop):
        finished = False
        while not finished:
            try:
                txn = self.env.txn_begin(None, self.txnFlag)
                for x in range(start, stop):
                    key = '%04d' % x
                    d.put(key, self.makeData(key), txn)
                    if verbose and x % 100 == 0:
                        print "%s: records %d - %d finished" % (name, start, x)
                txn.commit()
                finished = True
            except (db.DBLockDeadlockError, db.DBLockNotGrantedError), val:
                if verbose:
                    print "%s: Aborting transaction (%s)" % (name, val[1])
                txn.abort()
                time.sleep(0.05)

    def writerThread(self, d, howMany, writerNum):
        name = currentThread().getName()
        start = howMany * writerNum
        stop = howMany * (writerNum + 1) - 1
        if verbose:
            print "%s: creating records %d - %d" % (name, start, stop)

        step = 100
        for x in range(start, stop, step):
            self.doWrite(d, name, x, min(stop, x+step))

        if verbose:
            print "%s: finished creating records" % name
        if verbose:
            print "%s: deleting a few records" % name

        finished = False
        while not finished:
            try:
                recs = []
                txn = self.env.txn_begin(None, self.txnFlag)
                for x in range(10):
                    key = int(random() * howMany) + start
                    key = '%04d' % key
                    data = d.get(key, None, txn, db.DB_RMW)
                    if data is not None:
                        d.delete(key, txn)
                        recs.append(key)
                txn.commit()
                finished = True
                if verbose:
                    print "%s: deleted records %s" % (name, recs)
            except (db.DBLockDeadlockError, db.DBLockNotGrantedError), val:
                if verbose:
                    print "%s: Aborting transaction (%s)" % (name, val[1])
                txn.abort()
                time.sleep(0.05)

        if verbose:
            print "%s: thread finished" % name

    def readerThread(self, d, readerNum):
        time.sleep(0.01 * readerNum + 0.05)
        name = currentThread().getName()

        for loop in range(5):
            finished = False
            while not finished:
                try:
                    txn = self.env.txn_begin(None, self.txnFlag)
                    c = d.cursor(txn)
                    count = 0
                    rec = c.first()
                    while rec:
                        count += 1
                        key, data = rec
                        self.assertEqual(self.makeData(key), data)
                        rec = c.next()
                    if verbose: print "%s: found %d records" % (name, count)
                    c.close()
                    txn.commit()
                    finished = True
                except (db.DBLockDeadlockError, db.DBLockNotGrantedError), val:
                    if verbose:
                        print "%s: Aborting transaction (%s)" % (name, val[1])
                    c.close()
                    txn.abort()
                    time.sleep(0.05)

            time.sleep(0.05)

        if verbose:
            print "%s: thread finished" % name

    def deadlockThread(self):
        self.doLockDetect = True
        while self.doLockDetect:
            time.sleep(0.5)
            try:
                aborted = self.env.lock_detect(
                    db.DB_LOCK_RANDOM, db.DB_LOCK_CONFLICT)
                if verbose and aborted:
                    print "deadlock: Aborted %d deadlocked transaction(s)" \
                          % aborted
            except db.DBError:
                pass


class BTreeThreadedTransactions(ThreadedTransactionsBase):
    dbtype = db.DB_BTREE
    writers = 3
    readers = 5
    records = 2000

class HashThreadedTransactions(ThreadedTransactionsBase):
    dbtype = db.DB_HASH
    writers = 1
    readers = 5
    records = 2000

class BTreeThreadedNoWaitTransactions(ThreadedTransactionsBase):
    dbtype = db.DB_BTREE
    writers = 3
    readers = 5
    records = 2000
    txnFlag = db.DB_TXN_NOWAIT

class HashThreadedNoWaitTransactions(ThreadedTransactionsBase):
    dbtype = db.DB_HASH
    writers = 1
    readers = 5
    records = 2000
    txnFlag = db.DB_TXN_NOWAIT


#----------------------------------------------------------------------

def test_suite():
    suite = unittest.TestSuite()

    if have_threads:
        suite.addTest(unittest.makeSuite(BTreeConcurrentDataStore))
        suite.addTest(unittest.makeSuite(HashConcurrentDataStore))
        suite.addTest(unittest.makeSuite(BTreeSimpleThreaded))
        suite.addTest(unittest.makeSuite(HashSimpleThreaded))
        suite.addTest(unittest.makeSuite(BTreeThreadedTransactions))
        suite.addTest(unittest.makeSuite(HashThreadedTransactions))
        suite.addTest(unittest.makeSuite(BTreeThreadedNoWaitTransactions))
        suite.addTest(unittest.makeSuite(HashThreadedNoWaitTransactions))

    else:
        print "Threads not available, skipping thread tests."

    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
