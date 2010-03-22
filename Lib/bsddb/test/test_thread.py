"""TestCases for multi-threaded access to a DB.
"""

import os
import sys
import time
import errno
from random import random

DASH = '-'

try:
    WindowsError
except NameError:
    class WindowsError(Exception):
        pass

import unittest
from test_all import db, dbutils, test_support, verbose, have_threads, \
        get_new_environment_path, get_new_database_path

if have_threads :
    from threading import Thread
    if sys.version_info[0] < 3 :
        from threading import currentThread
    else :
        from threading import current_thread as currentThread


#----------------------------------------------------------------------

class BaseThreadedTestCase(unittest.TestCase):
    dbtype       = db.DB_UNKNOWN  # must be set in derived class
    dbopenflags  = 0
    dbsetflags   = 0
    envflags     = 0

    if sys.version_info < (2, 4) :
        def assertTrue(self, expr, msg=None):
            self.failUnless(expr,msg=msg)

    def setUp(self):
        if verbose:
            dbutils._deadlock_VerboseFile = sys.stdout

        self.homeDir = get_new_environment_path()
        self.env = db.DBEnv()
        self.setEnvOpts()
        self.env.open(self.homeDir, self.envflags | db.DB_CREATE)

        self.filename = self.__class__.__name__ + '.db'
        self.d = db.DB(self.env)
        if self.dbsetflags:
            self.d.set_flags(self.dbsetflags)
        self.d.open(self.filename, self.dbtype, self.dbopenflags|db.DB_CREATE)

    def tearDown(self):
        self.d.close()
        self.env.close()
        test_support.rmtree(self.homeDir)

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

        keys=range(self.records)
        import random
        random.shuffle(keys)
        records_per_writer=self.records//self.writers
        readers_per_writer=self.readers//self.writers
        self.assertEqual(self.records,self.writers*records_per_writer)
        self.assertEqual(self.readers,self.writers*readers_per_writer)
        self.assertTrue((records_per_writer%readers_per_writer)==0)
        readers = []

        for x in xrange(self.readers):
            rt = Thread(target = self.readerThread,
                        args = (self.d, x),
                        name = 'reader %d' % x,
                        )#verbose = verbose)
            if sys.version_info[0] < 3 :
                rt.setDaemon(True)
            else :
                rt.daemon = True
            readers.append(rt)

        writers=[]
        for x in xrange(self.writers):
            a=keys[records_per_writer*x:records_per_writer*(x+1)]
            a.sort()  # Generate conflicts
            b=readers[readers_per_writer*x:readers_per_writer*(x+1)]
            wt = Thread(target = self.writerThread,
                        args = (self.d, a, b),
                        name = 'writer %d' % x,
                        )#verbose = verbose)
            writers.append(wt)

        for t in writers:
            if sys.version_info[0] < 3 :
                t.setDaemon(True)
            else :
                t.daemon = True
            t.start()

        for t in writers:
            t.join()
        for t in readers:
            t.join()

    def writerThread(self, d, keys, readers):
        if sys.version_info[0] < 3 :
            name = currentThread().getName()
        else :
            name = currentThread().name

        if verbose:
            print "%s: creating records %d - %d" % (name, start, stop)

        count=len(keys)//len(readers)
        count2=count
        for x in keys :
            key = '%04d' % x
            dbutils.DeadlockWrap(d.put, key, self.makeData(key),
                                 max_retries=12)
            if verbose and x % 100 == 0:
                print "%s: records %d - %d finished" % (name, start, x)

            count2-=1
            if not count2 :
                readers.pop().start()
                count2=count

        if verbose:
            print "%s: finished creating records" % name

        if verbose:
            print "%s: thread finished" % name

    def readerThread(self, d, readerNum):
        if sys.version_info[0] < 3 :
            name = currentThread().getName()
        else :
            name = currentThread().name

        for i in xrange(5) :
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
    readers = 10
    writers = 2
    records = 1000

    def setEnvOpts(self):
        self.env.set_lk_detect(db.DB_LOCK_DEFAULT)

    def test02_SimpleLocks(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test02_SimpleLocks..." % self.__class__.__name__


        keys=range(self.records)
        import random
        random.shuffle(keys)
        records_per_writer=self.records//self.writers
        readers_per_writer=self.readers//self.writers
        self.assertEqual(self.records,self.writers*records_per_writer)
        self.assertEqual(self.readers,self.writers*readers_per_writer)
        self.assertTrue((records_per_writer%readers_per_writer)==0)

        readers = []
        for x in xrange(self.readers):
            rt = Thread(target = self.readerThread,
                        args = (self.d, x),
                        name = 'reader %d' % x,
                        )#verbose = verbose)
            if sys.version_info[0] < 3 :
                rt.setDaemon(True)
            else :
                rt.daemon = True
            readers.append(rt)

        writers = []
        for x in xrange(self.writers):
            a=keys[records_per_writer*x:records_per_writer*(x+1)]
            a.sort()  # Generate conflicts
            b=readers[readers_per_writer*x:readers_per_writer*(x+1)]
            wt = Thread(target = self.writerThread,
                        args = (self.d, a, b),
                        name = 'writer %d' % x,
                        )#verbose = verbose)
            writers.append(wt)

        for t in writers:
            if sys.version_info[0] < 3 :
                t.setDaemon(True)
            else :
                t.daemon = True
            t.start()

        for t in writers:
            t.join()
        for t in readers:
            t.join()

    def writerThread(self, d, keys, readers):
        if sys.version_info[0] < 3 :
            name = currentThread().getName()
        else :
            name = currentThread().name
        if verbose:
            print "%s: creating records %d - %d" % (name, start, stop)

        count=len(keys)//len(readers)
        count2=count
        for x in keys :
            key = '%04d' % x
            dbutils.DeadlockWrap(d.put, key, self.makeData(key),
                                 max_retries=12)

            if verbose and x % 100 == 0:
                print "%s: records %d - %d finished" % (name, start, x)

            count2-=1
            if not count2 :
                readers.pop().start()
                count2=count

        if verbose:
            print "%s: thread finished" % name

    def readerThread(self, d, readerNum):
        if sys.version_info[0] < 3 :
            name = currentThread().getName()
        else :
            name = currentThread().name

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

        keys=range(self.records)
        import random
        random.shuffle(keys)
        records_per_writer=self.records//self.writers
        readers_per_writer=self.readers//self.writers
        self.assertEqual(self.records,self.writers*records_per_writer)
        self.assertEqual(self.readers,self.writers*readers_per_writer)
        self.assertTrue((records_per_writer%readers_per_writer)==0)

        readers=[]
        for x in xrange(self.readers):
            rt = Thread(target = self.readerThread,
                        args = (self.d, x),
                        name = 'reader %d' % x,
                        )#verbose = verbose)
            if sys.version_info[0] < 3 :
                rt.setDaemon(True)
            else :
                rt.daemon = True
            readers.append(rt)

        writers = []
        for x in xrange(self.writers):
            a=keys[records_per_writer*x:records_per_writer*(x+1)]
            b=readers[readers_per_writer*x:readers_per_writer*(x+1)]
            wt = Thread(target = self.writerThread,
                        args = (self.d, a, b),
                        name = 'writer %d' % x,
                        )#verbose = verbose)
            writers.append(wt)

        dt = Thread(target = self.deadlockThread)
        if sys.version_info[0] < 3 :
            dt.setDaemon(True)
        else :
            dt.daemon = True
        dt.start()

        for t in writers:
            if sys.version_info[0] < 3 :
                t.setDaemon(True)
            else :
                t.daemon = True
            t.start()

        for t in writers:
            t.join()
        for t in readers:
            t.join()

        self.doLockDetect = False
        dt.join()

    def writerThread(self, d, keys, readers):
        if sys.version_info[0] < 3 :
            name = currentThread().getName()
        else :
            name = currentThread().name

        count=len(keys)//len(readers)
        while len(keys):
            try:
                txn = self.env.txn_begin(None, self.txnFlag)
                keys2=keys[:count]
                for x in keys2 :
                    key = '%04d' % x
                    d.put(key, self.makeData(key), txn)
                    if verbose and x % 100 == 0:
                        print "%s: records %d - %d finished" % (name, start, x)
                txn.commit()
                keys=keys[count:]
                readers.pop().start()
            except (db.DBLockDeadlockError, db.DBLockNotGrantedError), val:
                if verbose:
                    if sys.version_info < (2, 6) :
                        print "%s: Aborting transaction (%s)" % (name, val[1])
                    else :
                        print "%s: Aborting transaction (%s)" % (name,
                                val.args[1])
                txn.abort()

        if verbose:
            print "%s: thread finished" % name

    def readerThread(self, d, readerNum):
        if sys.version_info[0] < 3 :
            name = currentThread().getName()
        else :
            name = currentThread().name

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
                    if sys.version_info < (2, 6) :
                        print "%s: Aborting transaction (%s)" % (name, val[1])
                    else :
                        print "%s: Aborting transaction (%s)" % (name,
                                val.args[1])
                c.close()
                txn.abort()

        if verbose:
            print "%s: thread finished" % name

    def deadlockThread(self):
        self.doLockDetect = True
        while self.doLockDetect:
            time.sleep(0.05)
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
    writers = 2
    readers = 10
    records = 1000

class HashThreadedTransactions(ThreadedTransactionsBase):
    dbtype = db.DB_HASH
    writers = 2
    readers = 10
    records = 1000

class BTreeThreadedNoWaitTransactions(ThreadedTransactionsBase):
    dbtype = db.DB_BTREE
    writers = 2
    readers = 10
    records = 1000
    txnFlag = db.DB_TXN_NOWAIT

class HashThreadedNoWaitTransactions(ThreadedTransactionsBase):
    dbtype = db.DB_HASH
    writers = 2
    readers = 10
    records = 1000
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
