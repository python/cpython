"""
Basic TestCases for BTree and hash DBs, with and without a DBEnv, with
various DB flags, etc.
"""

import sys, os, string
import tempfile
from pprint import pprint
import unittest

from bsddb import db

from test.test_support import verbose


#----------------------------------------------------------------------

class VersionTestCase(unittest.TestCase):
    def test00_version(self):
        info = db.version()
        if verbose:
            print '\n', '-=' * 20
            print 'bsddb.db.version(): %s' % (info, )
            print db.DB_VERSION_STRING
            print '-=' * 20
        assert info == (db.DB_VERSION_MAJOR, db.DB_VERSION_MINOR, db.DB_VERSION_PATCH)

#----------------------------------------------------------------------

class BasicTestCase(unittest.TestCase):
    dbtype       = db.DB_UNKNOWN  # must be set in derived class
    dbopenflags  = 0
    dbsetflags   = 0
    dbmode       = 0660
    dbname       = None
    useEnv       = 0
    envflags     = 0

    def setUp(self):
        if self.useEnv:
            homeDir = os.path.join(os.path.dirname(sys.argv[0]), 'db_home')
            try: os.mkdir(homeDir)
            except os.error: pass
            self.env = db.DBEnv()
            self.env.set_lg_max(1024*1024)
            self.env.open(homeDir, self.envflags | db.DB_CREATE)
            tempfile.tempdir = homeDir
            self.filename = os.path.split(tempfile.mktemp())[1]
            tempfile.tempdir = None
            self.homeDir = homeDir
        else:
            self.env = None
            self.filename = tempfile.mktemp()

        # create and open the DB
        self.d = db.DB(self.env)
        self.d.set_flags(self.dbsetflags)
        if self.dbname:
            self.d.open(self.filename, self.dbname, self.dbtype,
                        self.dbopenflags|db.DB_CREATE, self.dbmode)
        else:
            self.d.open(self.filename,   # try out keyword args
                        mode = self.dbmode,
                        dbtype = self.dbtype, flags = self.dbopenflags|db.DB_CREATE)

        self.populateDB()


    def tearDown(self):
        self.d.close()
        if self.env is not None:
            self.env.close()

            import glob
            files = glob.glob(os.path.join(self.homeDir, '*'))
            for file in files:
                os.remove(file)

            ## Make a new DBEnv to remove the env files from the home dir.
            ## (It can't be done while the env is open, nor after it has been
            ## closed, so we make a new one to do it.)
            #e = db.DBEnv()
            #e.remove(self.homeDir)
            #os.remove(os.path.join(self.homeDir, self.filename))

        else:
            os.remove(self.filename)



    def populateDB(self):
        d = self.d
        for x in range(500):
            key = '%04d' % (1000 - x)  # insert keys in reverse order
            data = self.makeData(key)
            d.put(key, data)

        for x in range(500):
            key = '%04d' % x  # and now some in forward order
            data = self.makeData(key)
            d.put(key, data)

        num = len(d)
        if verbose:
            print "created %d records" % num


    def makeData(self, key):
        return string.join([key] * 5, '-')



    #----------------------------------------

    def test01_GetsAndPuts(self):
        d = self.d

        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test01_GetsAndPuts..." % self.__class__.__name__

        for key in ['0001', '0100', '0400', '0700', '0999']:
            data = d.get(key)
            if verbose:
                print data

        assert d.get('0321') == '0321-0321-0321-0321-0321'

        # By default non-existant keys return None...
        assert d.get('abcd') == None

        # ...but they raise exceptions in other situations.  Call
        # set_get_returns_none() to change it.
        try:
            d.delete('abcd')
        except db.DBNotFoundError, val:
            assert val[0] == db.DB_NOTFOUND
            if verbose: print val
        else:
            self.fail("expected exception")


        d.put('abcd', 'a new record')
        assert d.get('abcd') == 'a new record'

        d.put('abcd', 'same key')
        if self.dbsetflags & db.DB_DUP:
            assert d.get('abcd') == 'a new record'
        else:
            assert d.get('abcd') == 'same key'


        try:
            d.put('abcd', 'this should fail', flags=db.DB_NOOVERWRITE)
        except db.DBKeyExistError, val:
            assert val[0] == db.DB_KEYEXIST
            if verbose: print val
        else:
            self.fail("expected exception")

        if self.dbsetflags & db.DB_DUP:
            assert d.get('abcd') == 'a new record'
        else:
            assert d.get('abcd') == 'same key'


        d.sync()
        d.close()
        del d

        self.d = db.DB(self.env)
        if self.dbname:
            self.d.open(self.filename, self.dbname)
        else:
            self.d.open(self.filename)
        d = self.d

        assert d.get('0321') == '0321-0321-0321-0321-0321'
        if self.dbsetflags & db.DB_DUP:
            assert d.get('abcd') == 'a new record'
        else:
            assert d.get('abcd') == 'same key'

        rec = d.get_both('0555', '0555-0555-0555-0555-0555')
        if verbose:
            print rec

        assert d.get_both('0555', 'bad data') == None

        # test default value
        data = d.get('bad key', 'bad data')
        assert data == 'bad data'

        # any object can pass through
        data = d.get('bad key', self)
        assert data == self

        s = d.stat()
        assert type(s) == type({})
        if verbose:
            print 'd.stat() returned this dictionary:'
            pprint(s)


    #----------------------------------------

    def test02_DictionaryMethods(self):
        d = self.d

        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test02_DictionaryMethods..." % self.__class__.__name__

        for key in ['0002', '0101', '0401', '0701', '0998']:
            data = d[key]
            assert data == self.makeData(key)
            if verbose:
                print data

        assert len(d) == 1000
        keys = d.keys()
        assert len(keys) == 1000
        assert type(keys) == type([])

        d['new record'] = 'a new record'
        assert len(d) == 1001
        keys = d.keys()
        assert len(keys) == 1001

        d['new record'] = 'a replacement record'
        assert len(d) == 1001
        keys = d.keys()
        assert len(keys) == 1001

        if verbose:
            print "the first 10 keys are:"
            pprint(keys[:10])

        assert d['new record'] == 'a replacement record'

        assert d.has_key('0001') == 1
        assert d.has_key('spam') == 0

        items = d.items()
        assert len(items) == 1001
        assert type(items) == type([])
        assert type(items[0]) == type(())
        assert len(items[0]) == 2

        if verbose:
            print "the first 10 items are:"
            pprint(items[:10])

        values = d.values()
        assert len(values) == 1001
        assert type(values) == type([])

        if verbose:
            print "the first 10 values are:"
            pprint(values[:10])



    #----------------------------------------

    def test03_SimpleCursorStuff(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test03_SimpleCursorStuff..." % self.__class__.__name__

        c = self.d.cursor()


        rec = c.first()
        count = 0
        while rec is not None:
            count = count + 1
            if verbose and count % 100 == 0:
                print rec
            rec = c.next()

        assert count == 1000


        rec = c.last()
        count = 0
        while rec is not None:
            count = count + 1
            if verbose and count % 100 == 0:
                print rec
            rec = c.prev()

        assert count == 1000

        rec = c.set('0505')
        rec2 = c.current()
        assert rec == rec2
        assert rec[0] == '0505'
        assert rec[1] == self.makeData('0505')

        try:
            c.set('bad key')
        except db.DBNotFoundError, val:
            assert val[0] == db.DB_NOTFOUND
            if verbose: print val
        else:
            self.fail("expected exception")

        rec = c.get_both('0404', self.makeData('0404'))
        assert rec == ('0404', self.makeData('0404'))

        try:
            c.get_both('0404', 'bad data')
        except db.DBNotFoundError, val:
            assert val[0] == db.DB_NOTFOUND
            if verbose: print val
        else:
            self.fail("expected exception")

        if self.d.get_type() == db.DB_BTREE:
            rec = c.set_range('011')
            if verbose:
                print "searched for '011', found: ", rec

            rec = c.set_range('011',dlen=0,doff=0)
            if verbose:
                print "searched (partial) for '011', found: ", rec
            if rec[1] != '': set.fail('expected empty data portion')

        c.set('0499')
        c.delete()
        try:
            rec = c.current()
        except db.DBKeyEmptyError, val:
            assert val[0] == db.DB_KEYEMPTY
            if verbose: print val
        else:
            self.fail('exception expected')

        c.next()
        c2 = c.dup(db.DB_POSITION)
        assert c.current() == c2.current()

        c2.put('', 'a new value', db.DB_CURRENT)
        assert c.current() == c2.current()
        assert c.current()[1] == 'a new value'

        c2.put('', 'er', db.DB_CURRENT, dlen=0, doff=5)
        assert c2.current()[1] == 'a newer value'

        c.close()
        c2.close()

        # time to abuse the closed cursors and hope we don't crash
        methods_to_test = {
            'current': (),
            'delete': (),
            'dup': (db.DB_POSITION,),
            'first': (),
            'get': (0,),
            'next': (),
            'prev': (),
            'last': (),
            'put':('', 'spam', db.DB_CURRENT),
            'set': ("0505",),
        }
        for method, args in methods_to_test.items():
            try:
                if verbose:
                    print "attempting to use a closed cursor's %s method" % method
                # a bug may cause a NULL pointer dereference...
                apply(getattr(c, method), args)
            except db.DBError, val:
                assert val[0] == 0
                if verbose: print val
            else:
                self.fail("no exception raised when using a buggy cursor's %s method" % method)

    #----------------------------------------

    def test04_PartialGetAndPut(self):
        d = self.d
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test04_PartialGetAndPut..." % self.__class__.__name__

        key = "partialTest"
        data = "1" * 1000 + "2" * 1000
        d.put(key, data)
        assert d.get(key) == data
        assert d.get(key, dlen=20, doff=990) == ("1" * 10) + ("2" * 10)

        d.put("partialtest2", ("1" * 30000) + "robin" )
        assert d.get("partialtest2", dlen=5, doff=30000) == "robin"

        # There seems to be a bug in DB here...  Commented out the test for now.
        ##assert d.get("partialtest2", dlen=5, doff=30010) == ""

        if self.dbsetflags != db.DB_DUP:
            # Partial put with duplicate records requires a cursor
            d.put(key, "0000", dlen=2000, doff=0)
            assert d.get(key) == "0000"

            d.put(key, "1111", dlen=1, doff=2)
            assert d.get(key) == "0011110"

    #----------------------------------------

    def test05_GetSize(self):
        d = self.d
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test05_GetSize..." % self.__class__.__name__

        for i in range(1, 50000, 500):
            key = "size%s" % i
            #print "before ", i,
            d.put(key, "1" * i)
            #print "after",
            assert d.get_size(key) == i
            #print "done"

    #----------------------------------------

    def test06_Truncate(self):
        d = self.d
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test99_Truncate..." % self.__class__.__name__

        d.put("abcde", "ABCDE");
        num = d.truncate()
        assert num >= 1, "truncate returned <= 0 on non-empty database"
        num = d.truncate()
        assert num == 0, "truncate on empty DB returned nonzero (%s)" % `num`

#----------------------------------------------------------------------


class BasicBTreeTestCase(BasicTestCase):
    dbtype = db.DB_BTREE


class BasicHashTestCase(BasicTestCase):
    dbtype = db.DB_HASH


class BasicBTreeWithThreadFlagTestCase(BasicTestCase):
    dbtype = db.DB_BTREE
    dbopenflags = db.DB_THREAD


class BasicHashWithThreadFlagTestCase(BasicTestCase):
    dbtype = db.DB_HASH
    dbopenflags = db.DB_THREAD


class BasicBTreeWithEnvTestCase(BasicTestCase):
    dbtype = db.DB_BTREE
    dbopenflags = db.DB_THREAD
    useEnv = 1
    envflags = db.DB_THREAD | db.DB_INIT_MPOOL | db.DB_INIT_LOCK


class BasicHashWithEnvTestCase(BasicTestCase):
    dbtype = db.DB_HASH
    dbopenflags = db.DB_THREAD
    useEnv = 1
    envflags = db.DB_THREAD | db.DB_INIT_MPOOL | db.DB_INIT_LOCK


#----------------------------------------------------------------------

class BasicTransactionTestCase(BasicTestCase):
    dbopenflags = db.DB_THREAD
    useEnv = 1
    envflags = db.DB_THREAD | db.DB_INIT_MPOOL | db.DB_INIT_LOCK | db.DB_INIT_TXN


    def tearDown(self):
        self.txn.commit()
        BasicTestCase.tearDown(self)


    def populateDB(self):
        d = self.d
        txn = self.env.txn_begin()
        for x in range(500):
            key = '%04d' % (1000 - x)  # insert keys in reverse order
            data = self.makeData(key)
            d.put(key, data, txn)

        for x in range(500):
            key = '%04d' % x  # and now some in forward order
            data = self.makeData(key)
            d.put(key, data, txn)

        txn.commit()

        num = len(d)
        if verbose:
            print "created %d records" % num

        self.txn = self.env.txn_begin()



    def test06_Transactions(self):
        d = self.d
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test06_Transactions..." % self.__class__.__name__

        assert d.get('new rec', txn=self.txn) == None
        d.put('new rec', 'this is a new record', self.txn)
        assert d.get('new rec', txn=self.txn) == 'this is a new record'
        self.txn.abort()
        assert d.get('new rec') == None

        self.txn = self.env.txn_begin()

        assert d.get('new rec', txn=self.txn) == None
        d.put('new rec', 'this is a new record', self.txn)
        assert d.get('new rec', txn=self.txn) == 'this is a new record'
        self.txn.commit()
        assert d.get('new rec') == 'this is a new record'

        self.txn = self.env.txn_begin()
        c = d.cursor(self.txn)
        rec = c.first()
        count = 0
        while rec is not None:
            count = count + 1
            if verbose and count % 100 == 0:
                print rec
            rec = c.next()
        assert count == 1001

        c.close()                # Cursors *MUST* be closed before commit!
        self.txn.commit()

        # flush pending updates
        try:
            self.env.txn_checkpoint (0, 0, 0)
        except db.DBIncompleteError:
            pass

        # must have at least one log file present:
        logs = self.env.log_archive(db.DB_ARCH_ABS | db.DB_ARCH_LOG)
        assert logs != None
        for log in logs:
            if verbose:
                print 'log file: ' + log

        self.txn = self.env.txn_begin()

    #----------------------------------------

    def test07_TxnTruncate(self):
        d = self.d
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test07_TxnTruncate..." % self.__class__.__name__

        d.put("abcde", "ABCDE");
        txn = self.env.txn_begin()
        num = d.truncate(txn)
        assert num >= 1, "truncate returned <= 0 on non-empty database"
        num = d.truncate(txn)
        assert num == 0, "truncate on empty DB returned nonzero (%s)" % `num`
        txn.commit()



class BTreeTransactionTestCase(BasicTransactionTestCase):
    dbtype = db.DB_BTREE

class HashTransactionTestCase(BasicTransactionTestCase):
    dbtype = db.DB_HASH



#----------------------------------------------------------------------

class BTreeRecnoTestCase(BasicTestCase):
    dbtype     = db.DB_BTREE
    dbsetflags = db.DB_RECNUM

    def test07_RecnoInBTree(self):
        d = self.d
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test07_RecnoInBTree..." % self.__class__.__name__

        rec = d.get(200)
        assert type(rec) == type(())
        assert len(rec) == 2
        if verbose:
            print "Record #200 is ", rec

        c = d.cursor()
        c.set('0200')
        num = c.get_recno()
        assert type(num) == type(1)
        if verbose:
            print "recno of d['0200'] is ", num

        rec = c.current()
        assert c.set_recno(num) == rec

        c.close()



class BTreeRecnoWithThreadFlagTestCase(BTreeRecnoTestCase):
    dbopenflags = db.DB_THREAD

#----------------------------------------------------------------------

class BasicDUPTestCase(BasicTestCase):
    dbsetflags = db.DB_DUP

    def test08_DuplicateKeys(self):
        d = self.d
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test08_DuplicateKeys..." % self.__class__.__name__

        d.put("dup0", "before")
        for x in string.split("The quick brown fox jumped over the lazy dog."):
            d.put("dup1", x)
        d.put("dup2", "after")

        data = d.get("dup1")
        assert data == "The"
        if verbose:
            print data

        c = d.cursor()
        rec = c.set("dup1")
        assert rec == ('dup1', 'The')

        next = c.next()
        assert next == ('dup1', 'quick')

        rec = c.set("dup1")
        count = c.count()
        assert count == 9

        next_dup = c.next_dup()
        assert next_dup == ('dup1', 'quick')

        rec = c.set('dup1')
        while rec is not None:
            if verbose:
                print rec
            rec = c.next_dup()

        c.set('dup1')
        rec = c.next_nodup()
        assert rec[0] != 'dup1'
        if verbose:
            print rec

        c.close()



class BTreeDUPTestCase(BasicDUPTestCase):
    dbtype = db.DB_BTREE

class HashDUPTestCase(BasicDUPTestCase):
    dbtype = db.DB_HASH

class BTreeDUPWithThreadTestCase(BasicDUPTestCase):
    dbtype = db.DB_BTREE
    dbopenflags = db.DB_THREAD

class HashDUPWithThreadTestCase(BasicDUPTestCase):
    dbtype = db.DB_HASH
    dbopenflags = db.DB_THREAD


#----------------------------------------------------------------------

class BasicMultiDBTestCase(BasicTestCase):
    dbname = 'first'

    def otherType(self):
        if self.dbtype == db.DB_BTREE:
            return db.DB_HASH
        else:
            return db.DB_BTREE

    def test09_MultiDB(self):
        d1 = self.d
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test09_MultiDB..." % self.__class__.__name__

        d2 = db.DB(self.env)
        d2.open(self.filename, "second", self.dbtype, self.dbopenflags|db.DB_CREATE)
        d3 = db.DB(self.env)
        d3.open(self.filename, "third", self.otherType(), self.dbopenflags|db.DB_CREATE)

        for x in string.split("The quick brown fox jumped over the lazy dog"):
            d2.put(x, self.makeData(x))

        for x in string.letters:
            d3.put(x, x*70)

        d1.sync()
        d2.sync()
        d3.sync()
        d1.close()
        d2.close()
        d3.close()

        self.d = d1 = d2 = d3 = None

        self.d = d1 = db.DB(self.env)
        d1.open(self.filename, self.dbname, flags = self.dbopenflags)
        d2 = db.DB(self.env)
        d2.open(self.filename, "second",  flags = self.dbopenflags)
        d3 = db.DB(self.env)
        d3.open(self.filename, "third", flags = self.dbopenflags)

        c1 = d1.cursor()
        c2 = d2.cursor()
        c3 = d3.cursor()

        count = 0
        rec = c1.first()
        while rec is not None:
            count = count + 1
            if verbose and (count % 50) == 0:
                print rec
            rec = c1.next()
        assert count == 1000

        count = 0
        rec = c2.first()
        while rec is not None:
            count = count + 1
            if verbose:
                print rec
            rec = c2.next()
        assert count == 9

        count = 0
        rec = c3.first()
        while rec is not None:
            count = count + 1
            if verbose:
                print rec
            rec = c3.next()
        assert count == 52


        c1.close()
        c2.close()
        c3.close()

        d2.close()
        d3.close()



# Strange things happen if you try to use Multiple DBs per file without a
# DBEnv with MPOOL and LOCKing...

class BTreeMultiDBTestCase(BasicMultiDBTestCase):
    dbtype = db.DB_BTREE
    dbopenflags = db.DB_THREAD
    useEnv = 1
    envflags = db.DB_THREAD | db.DB_INIT_MPOOL | db.DB_INIT_LOCK

class HashMultiDBTestCase(BasicMultiDBTestCase):
    dbtype = db.DB_HASH
    dbopenflags = db.DB_THREAD
    useEnv = 1
    envflags = db.DB_THREAD | db.DB_INIT_MPOOL | db.DB_INIT_LOCK


#----------------------------------------------------------------------
#----------------------------------------------------------------------

def suite():
    theSuite = unittest.TestSuite()

    theSuite.addTest(unittest.makeSuite(VersionTestCase))
    theSuite.addTest(unittest.makeSuite(BasicBTreeTestCase))
    theSuite.addTest(unittest.makeSuite(BasicHashTestCase))
    theSuite.addTest(unittest.makeSuite(BasicBTreeWithThreadFlagTestCase))
    theSuite.addTest(unittest.makeSuite(BasicHashWithThreadFlagTestCase))
    theSuite.addTest(unittest.makeSuite(BasicBTreeWithEnvTestCase))
    theSuite.addTest(unittest.makeSuite(BasicHashWithEnvTestCase))
    theSuite.addTest(unittest.makeSuite(BTreeTransactionTestCase))
    theSuite.addTest(unittest.makeSuite(HashTransactionTestCase))
    theSuite.addTest(unittest.makeSuite(BTreeRecnoTestCase))
    theSuite.addTest(unittest.makeSuite(BTreeRecnoWithThreadFlagTestCase))
    theSuite.addTest(unittest.makeSuite(BTreeDUPTestCase))
    theSuite.addTest(unittest.makeSuite(HashDUPTestCase))
    theSuite.addTest(unittest.makeSuite(BTreeDUPWithThreadTestCase))
    theSuite.addTest(unittest.makeSuite(HashDUPWithThreadTestCase))
    theSuite.addTest(unittest.makeSuite(BTreeMultiDBTestCase))
    theSuite.addTest(unittest.makeSuite(HashMultiDBTestCase))

    return theSuite


if __name__ == '__main__':
    unittest.main( defaultTest='suite' )
