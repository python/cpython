"""
Basic TestCases for BTree and hash DBs, with and without a DBEnv, with
various DB flags, etc.
"""

import os
import sys
import errno
import string
import tempfile
from pprint import pprint
from test import test_support
import unittest
import time

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db
except ImportError:
    # For Python 2.3
    from bsddb import db

from bsddb.test.test_all import verbose

DASH = b'-'
letters = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'

#----------------------------------------------------------------------

class VersionTestCase(unittest.TestCase):
    def test00_version(self):
        info = db.version()
        if verbose:
            print('\n', '-=' * 20)
            print('bsddb.db.version(): %s' % (info, ))
            print(db.DB_VERSION_STRING)
            print('-=' * 20)
        assert info == (db.DB_VERSION_MAJOR, db.DB_VERSION_MINOR,
                        db.DB_VERSION_PATCH)

#----------------------------------------------------------------------

class BasicTestCase(unittest.TestCase):
    dbtype       = db.DB_UNKNOWN  # must be set in derived class
    dbopenflags  = 0
    dbsetflags   = 0
    dbmode       = 0o660
    dbname       = None
    useEnv       = 0
    envflags     = 0
    envsetflags  = 0

    _numKeys      = 1002    # PRIVATE.  NOTE: must be an even value

    def setUp(self):
        if self.useEnv:
            homeDir = os.path.join(tempfile.gettempdir(), 'db_home%d'%os.getpid())
            self.homeDir = homeDir
            test_support.rmtree(homeDir)
            os.mkdir(homeDir)
            try:
                self.env = db.DBEnv()
                self.env.set_lg_max(1024*1024)
                self.env.set_tx_max(30)
                self.env.set_tx_timestamp(int(time.time()))
                self.env.set_flags(self.envsetflags, 1)
                self.env.open(self.homeDir, self.envflags | db.DB_CREATE)
                old_tempfile_tempdir = tempfile.tempdir
                tempfile.tempdir = self.homeDir
                self.filename = os.path.split(tempfile.mktemp())[1]
                tempfile.tempdir = old_tempfile_tempdir
            # Yes, a bare except is intended, since we're re-raising the exc.
            except:
                test_support.rmtree(homeDir)
                raise
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
                        dbtype = self.dbtype,
                        flags = self.dbopenflags|db.DB_CREATE)

        self.populateDB()


    def tearDown(self):
        self.d.close()
        if self.env is not None:
            test_support.rmtree(self.homeDir)
            self.env.close()
            ## Make a new DBEnv to remove the env files from the home dir.
            ## (It can't be done while the env is open, nor after it has been
            ## closed, so we make a new one to do it.)
            #e = db.DBEnv()
            #e.remove(self.homeDir)
            #os.remove(os.path.join(self.homeDir, self.filename))
        else:
            os.remove(self.filename)



    def populateDB(self, _txn=None):
        d = self.d

        for x in range(self._numKeys//2):
            key = '%04d' % (self._numKeys - x)  # insert keys in reverse order
            key = key.encode("utf-8")
            data = self.makeData(key)
            d.put(key, data, _txn)

        d.put(b'empty value', b'', _txn)

        for x in range(self._numKeys//2-1):
            key = '%04d' % x  # and now some in forward order
            key = key.encode("utf-8")
            data = self.makeData(key)
            d.put(key, data, _txn)

        if _txn:
            _txn.commit()

        num = len(d)
        if verbose:
            print("created %d records" % num)


    def makeData(self, key):
        return DASH.join([key] * 5)



    #----------------------------------------

    def test01_GetsAndPuts(self):
        d = self.d

        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test01_GetsAndPuts..." % self.__class__.__name__)

        for key in [b'0001', b'0100', b'0400', b'0700', b'0999']:
            data = d.get(key)
            if verbose:
                print(data)

        assert d.get(b'0321') == b'0321-0321-0321-0321-0321'

        # By default non-existant keys return None...
        assert d.get(b'abcd') == None

        # ...but they raise exceptions in other situations.  Call
        # set_get_returns_none() to change it.
        try:
            d.delete(b'abcd')
        except db.DBNotFoundError as val:
            assert val.args[0] == db.DB_NOTFOUND
            if verbose: print(val)
        else:
            self.fail("expected exception")


        d.put(b'abcd', b'a new record')
        assert d.get(b'abcd') == b'a new record'

        d.put(b'abcd', b'same key')
        if self.dbsetflags & db.DB_DUP:
            assert d.get(b'abcd') == b'a new record'
        else:
            assert d.get(b'abcd') == b'same key'


        try:
            d.put(b'abcd', b'this should fail', flags=db.DB_NOOVERWRITE)
        except db.DBKeyExistError as val:
            assert val.args[0] == db.DB_KEYEXIST
            if verbose: print(val)
        else:
            self.fail("expected exception")

        if self.dbsetflags & db.DB_DUP:
            assert d.get(b'abcd') == b'a new record'
        else:
            assert d.get(b'abcd') == b'same key'


        d.sync()
        d.close()
        del d

        self.d = db.DB(self.env)
        if self.dbname:
            self.d.open(self.filename, self.dbname)
        else:
            self.d.open(self.filename)
        d = self.d

        assert d.get(b'0321') == b'0321-0321-0321-0321-0321'
        if self.dbsetflags & db.DB_DUP:
            assert d.get(b'abcd') == b'a new record'
        else:
            assert d.get(b'abcd') == b'same key'

        rec = d.get_both(b'0555', b'0555-0555-0555-0555-0555')
        if verbose:
            print(rec)

        assert d.get_both(b'0555', b'bad data') == None

        # test default value
        data = d.get(b'bad key', b'bad data')
        assert data == b'bad data'

        # any object can pass through
        data = d.get(b'bad key', self)
        assert data == self

        s = d.stat()
        assert type(s) == type({})
        if verbose:
            print('d.stat() returned this dictionary:')
            pprint(s)


    #----------------------------------------

    def test02_DictionaryMethods(self):
        d = self.d

        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test02_DictionaryMethods..." % \
                  self.__class__.__name__)

        for key in [b'0002', b'0101', b'0401', b'0701', b'0998']:
            data = d[key]
            assert data == self.makeData(key)
            if verbose:
                print(data)

        assert len(d) == self._numKeys
        keys = d.keys()
        assert len(keys) == self._numKeys
        assert type(keys) == type([])

        d[b'new record'] = b'a new record'
        assert len(d) == self._numKeys+1
        keys = d.keys()
        assert len(keys) == self._numKeys+1

        d[b'new record'] = b'a replacement record'
        assert len(d) == self._numKeys+1
        keys = d.keys()
        assert len(keys) == self._numKeys+1

        if verbose:
            print("the first 10 keys are:")
            pprint(keys[:10])

        assert d[b'new record'] == b'a replacement record'

        assert d.has_key(b'0001') == 1
        assert d.has_key(b'spam') == 0

        items = d.items()
        assert len(items) == self._numKeys+1
        assert type(items) == type([])
        assert type(items[0]) == type(())
        assert len(items[0]) == 2

        if verbose:
            print("the first 10 items are:")
            pprint(items[:10])

        values = d.values()
        assert len(values) == self._numKeys+1
        assert type(values) == type([])

        if verbose:
            print("the first 10 values are:")
            pprint(values[:10])



    #----------------------------------------

    def test03_SimpleCursorStuff(self, get_raises_error=0, set_raises_error=0):
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test03_SimpleCursorStuff (get_error %s, set_error %s)..." % \
                  (self.__class__.__name__, get_raises_error, set_raises_error))

        if self.env and self.dbopenflags & db.DB_AUTO_COMMIT:
            txn = self.env.txn_begin()
        else:
            txn = None
        c = self.d.cursor(txn=txn)

        rec = c.first()
        count = 0
        while rec is not None:
            count = count + 1
            if verbose and count % 100 == 0:
                print(rec)
            try:
                rec = c.next()
            except db.DBNotFoundError as val:
                if get_raises_error:
                    assert val.args[0] == db.DB_NOTFOUND
                    if verbose: print(val)
                    rec = None
                else:
                    self.fail("unexpected DBNotFoundError")
            assert c.get_current_size() == len(c.current()[1]), "%s != len(%r)" % (c.get_current_size(), c.current()[1])

        assert count == self._numKeys


        rec = c.last()
        count = 0
        while rec is not None:
            count = count + 1
            if verbose and count % 100 == 0:
                print(rec)
            try:
                rec = c.prev()
            except db.DBNotFoundError as val:
                if get_raises_error:
                    assert val.args[0] == db.DB_NOTFOUND
                    if verbose: print(val)
                    rec = None
                else:
                    self.fail("unexpected DBNotFoundError")

        assert count == self._numKeys

        rec = c.set(b'0505')
        rec2 = c.current()
        assert rec == rec2, (repr(rec),repr(rec2))
        assert rec[0] == b'0505'
        assert rec[1] == self.makeData(b'0505')
        assert c.get_current_size() == len(rec[1])

        # make sure we get empty values properly
        rec = c.set(b'empty value')
        assert rec[1] == b''
        assert c.get_current_size() == 0

        try:
            n = c.set(b'bad key')
        except db.DBNotFoundError as val:
            assert val.args[0] == db.DB_NOTFOUND
            if verbose: print(val)
        else:
            if set_raises_error:
                self.fail("expected exception")
            if n != None:
                self.fail("expected None: %r" % (n,))

        rec = c.get_both(b'0404', self.makeData(b'0404'))
        assert rec == (b'0404', self.makeData(b'0404'))

        try:
            n = c.get_both(b'0404', b'bad data')
        except db.DBNotFoundError as val:
            assert val.args[0] == db.DB_NOTFOUND
            if verbose: print(val)
        else:
            if get_raises_error:
                self.fail("expected exception")
            if n != None:
                self.fail("expected None: %r" % (n,))

        if self.d.get_type() == db.DB_BTREE:
            rec = c.set_range(b'011')
            if verbose:
                print("searched for '011', found: ", rec)

            rec = c.set_range(b'011',dlen=0,doff=0)
            if verbose:
                print("searched (partial) for '011', found: ", rec)
            if rec[1] != b'': self.fail('expected empty data portion')

            ev = c.set_range(b'empty value')
            if verbose:
                print("search for 'empty value' returned", ev)
            if ev[1] != b'': self.fail('empty value lookup failed')

        c.set(b'0499')
        c.delete()
        try:
            rec = c.current()
        except db.DBKeyEmptyError as val:
            if get_raises_error:
                assert val.args[0] == db.DB_KEYEMPTY
                if verbose: print(val)
            else:
                self.fail("unexpected DBKeyEmptyError")
        else:
            if get_raises_error:
                self.fail('DBKeyEmptyError exception expected')

        c.next()
        c2 = c.dup(db.DB_POSITION)
        assert c.current() == c2.current()

        c2.put(b'', b'a new value', db.DB_CURRENT)
        assert c.current() == c2.current()
        assert c.current()[1] == b'a new value'

        c2.put(b'', b'er', db.DB_CURRENT, dlen=0, doff=5)
        assert c2.current()[1] == b'a newer value'

        c.close()
        c2.close()
        if txn:
            txn.commit()

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
                    print("attempting to use a closed cursor's %s method" % \
                          method)
                # a bug may cause a NULL pointer dereference...
                getattr(c, method)(*args)
            except db.DBError as val:
                assert val.args[0] == 0
                if verbose: print(val)
            else:
                self.fail("no exception raised when using a buggy cursor's"
                          "%s method" % method)

        #
        # free cursor referencing a closed database, it should not barf:
        #
        oldcursor = self.d.cursor(txn=txn)
        self.d.close()

        # this would originally cause a segfault when the cursor for a
        # closed database was cleaned up.  it should not anymore.
        # SF pybsddb bug id 667343
        del oldcursor

    def test03b_SimpleCursorWithoutGetReturnsNone0(self):
        # same test but raise exceptions instead of returning None
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test03b_SimpleCursorStuffWithoutGetReturnsNone..." % \
                  self.__class__.__name__)

        old = self.d.set_get_returns_none(0)
        assert old == 2
        self.test03_SimpleCursorStuff(get_raises_error=1, set_raises_error=1)

    def test03b_SimpleCursorWithGetReturnsNone1(self):
        # same test but raise exceptions instead of returning None
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test03b_SimpleCursorStuffWithoutGetReturnsNone..." % \
                  self.__class__.__name__)

        old = self.d.set_get_returns_none(1)
        self.test03_SimpleCursorStuff(get_raises_error=0, set_raises_error=1)


    def test03c_SimpleCursorGetReturnsNone2(self):
        # same test but raise exceptions instead of returning None
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test03c_SimpleCursorStuffWithoutSetReturnsNone..." % \
                  self.__class__.__name__)

        old = self.d.set_get_returns_none(1)
        assert old == 2
        old = self.d.set_get_returns_none(2)
        assert old == 1
        self.test03_SimpleCursorStuff(get_raises_error=0, set_raises_error=0)

    #----------------------------------------

    def test04_PartialGetAndPut(self):
        d = self.d
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test04_PartialGetAndPut..." % \
                  self.__class__.__name__)

        key = b"partialTest"
        data = b"1" * 1000 + b"2" * 1000
        d.put(key, data)
        assert d.get(key) == data
        assert d.get(key, dlen=20, doff=990) == (b"1" * 10) + (b"2" * 10)

        d.put(b"partialtest2", (b"1" * 30000) + b"robin" )
        assert d.get(b"partialtest2", dlen=5, doff=30000) == b"robin"

        # There seems to be a bug in DB here...  Commented out the test for
        # now.
        ##assert d.get("partialtest2", dlen=5, doff=30010) == ""

        if self.dbsetflags != db.DB_DUP:
            # Partial put with duplicate records requires a cursor
            d.put(key, b"0000", dlen=2000, doff=0)
            assert d.get(key) == b"0000"

            d.put(key, b"1111", dlen=1, doff=2)
            assert d.get(key) == b"0011110"

    #----------------------------------------

    def test05_GetSize(self):
        d = self.d
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test05_GetSize..." % self.__class__.__name__)

        for i in range(1, 50000, 500):
            key = ("size%s" % i).encode("utf-8")
            #print "before ", i,
            d.put(key, b"1" * i)
            #print "after",
            assert d.get_size(key) == i
            #print "done"

    #----------------------------------------

    def test06_Truncate(self):
        if db.version() < (3,3):
            # truncate is a feature of BerkeleyDB 3.3 and above
            return

        d = self.d
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test99_Truncate..." % self.__class__.__name__)

        d.put(b"abcde", b"ABCDE");
        num = d.truncate()
        assert num >= 1, "truncate returned <= 0 on non-empty database"
        num = d.truncate()
        assert num == 0, "truncate on empty DB returned nonzero (%r)" % (num,)

    #----------------------------------------


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


class BasicWithEnvTestCase(BasicTestCase):
    dbopenflags = db.DB_THREAD
    useEnv = 1
    envflags = db.DB_THREAD | db.DB_INIT_MPOOL | db.DB_INIT_LOCK

    #----------------------------------------

    def test07_EnvRemoveAndRename(self):
        if not self.env:
            return

        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test07_EnvRemoveAndRename..." % self.__class__.__name__)

        # can't rename or remove an open DB
        self.d.close()

        newname = self.filename + '.renamed'
        self.env.dbrename(self.filename, None, newname)
        self.env.dbremove(newname)

    # dbremove and dbrename are in 4.1 and later
    if db.version() < (4,1):
        del test07_EnvRemoveAndRename

    #----------------------------------------

class BasicBTreeWithEnvTestCase(BasicWithEnvTestCase):
    dbtype = db.DB_BTREE


class BasicHashWithEnvTestCase(BasicWithEnvTestCase):
    dbtype = db.DB_HASH


#----------------------------------------------------------------------

class BasicTransactionTestCase(BasicTestCase):
    dbopenflags = db.DB_THREAD | db.DB_AUTO_COMMIT
    useEnv = 1
    envflags = (db.DB_THREAD | db.DB_INIT_MPOOL | db.DB_INIT_LOCK |
                db.DB_INIT_TXN)
    envsetflags = db.DB_AUTO_COMMIT


    def tearDown(self):
        self.txn.commit()
        BasicTestCase.tearDown(self)


    def populateDB(self):
        txn = self.env.txn_begin()
        BasicTestCase.populateDB(self, _txn=txn)

        self.txn = self.env.txn_begin()


    def test06_Transactions(self):
        d = self.d
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test06_Transactions..." % self.__class__.__name__)

        assert d.get(b'new rec', txn=self.txn) == None
        d.put(b'new rec', b'this is a new record', self.txn)
        assert d.get(b'new rec', txn=self.txn) == b'this is a new record'
        self.txn.abort()
        assert d.get(b'new rec') == None

        self.txn = self.env.txn_begin()

        assert d.get(b'new rec', txn=self.txn) == None
        d.put(b'new rec', b'this is a new record', self.txn)
        assert d.get(b'new rec', txn=self.txn) == b'this is a new record'
        self.txn.commit()
        assert d.get(b'new rec') == b'this is a new record'

        self.txn = self.env.txn_begin()
        c = d.cursor(self.txn)
        rec = c.first()
        count = 0
        while rec is not None:
            count = count + 1
            if verbose and count % 100 == 0:
                print(rec)
            rec = c.next()
        assert count == self._numKeys+1

        c.close()                # Cursors *MUST* be closed before commit!
        self.txn.commit()

        # flush pending updates
        try:
            self.env.txn_checkpoint (0, 0, 0)
        except db.DBIncompleteError:
            pass

        if db.version() >= (4,0):
            statDict = self.env.log_stat(0);
            assert 'magic' in statDict
            assert 'version' in statDict
            assert 'cur_file' in statDict
            assert 'region_nowait' in statDict

        # must have at least one log file present:
        logs = self.env.log_archive(db.DB_ARCH_ABS | db.DB_ARCH_LOG)
        assert logs != None
        for log in logs:
            if verbose:
                print('log file: ' + log)
        if db.version() >= (4,2):
            logs = self.env.log_archive(db.DB_ARCH_REMOVE)
            assert not logs

        self.txn = self.env.txn_begin()

    #----------------------------------------

    def test07_TxnTruncate(self):
        if db.version() < (3,3):
            # truncate is a feature of BerkeleyDB 3.3 and above
            return

        d = self.d
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test07_TxnTruncate..." % self.__class__.__name__)

        d.put(b"abcde", b"ABCDE");
        txn = self.env.txn_begin()
        num = d.truncate(txn)
        assert num >= 1, "truncate returned <= 0 on non-empty database"
        num = d.truncate(txn)
        assert num == 0, "truncate on empty DB returned nonzero (%r)" % (num,)
        txn.commit()

    #----------------------------------------

    def test08_TxnLateUse(self):
        txn = self.env.txn_begin()
        txn.abort()
        try:
            txn.abort()
        except db.DBError as e:
            pass
        else:
            raise RuntimeError("DBTxn.abort() called after DB_TXN no longer valid w/o an exception")

        txn = self.env.txn_begin()
        txn.commit()
        try:
            txn.commit()
        except db.DBError as e:
            pass
        else:
            raise RuntimeError("DBTxn.commit() called after DB_TXN no longer valid w/o an exception")


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
            print('\n', '-=' * 30)
            print("Running %s.test07_RecnoInBTree..." % self.__class__.__name__)

        rec = d.get(200)
        assert type(rec) == type(())
        assert len(rec) == 2
        if verbose:
            print("Record #200 is ", rec)

        c = d.cursor()
        c.set(b'0200')
        num = c.get_recno()
        assert type(num) == type(1)
        if verbose:
            print("recno of d['0200'] is ", num)

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
            print('\n', '-=' * 30)
            print("Running %s.test08_DuplicateKeys..." % \
                  self.__class__.__name__)

        d.put(b"dup0", b"before")
        for x in "The quick brown fox jumped over the lazy dog.".split():
            x = x.encode("ascii")
            d.put(b"dup1", x)
        d.put(b"dup2", b"after")

        data = d.get(b"dup1")
        assert data == b"The"
        if verbose:
            print(data)

        c = d.cursor()
        rec = c.set(b"dup1")
        assert rec == (b'dup1', b'The')

        next = c.next()
        assert next == (b'dup1', b'quick')

        rec = c.set(b"dup1")
        count = c.count()
        assert count == 9

        next_dup = c.next_dup()
        assert next_dup == (b'dup1', b'quick')

        rec = c.set(b'dup1')
        while rec is not None:
            if verbose:
                print(rec)
            rec = c.next_dup()

        c.set(b'dup1')
        rec = c.next_nodup()
        assert rec[0] != b'dup1'
        if verbose:
            print(rec)

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
            print('\n', '-=' * 30)
            print("Running %s.test09_MultiDB..." % self.__class__.__name__)

        d2 = db.DB(self.env)
        d2.open(self.filename, "second", self.dbtype,
                self.dbopenflags|db.DB_CREATE)
        d3 = db.DB(self.env)
        d3.open(self.filename, "third", self.otherType(),
                self.dbopenflags|db.DB_CREATE)

        for x in "The quick brown fox jumped over the lazy dog".split():
            x = x.encode("ascii")
            d2.put(x, self.makeData(x))

        for x in letters:
            x = x.encode("ascii")
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
                print(rec)
            rec = c1.next()
        assert count == self._numKeys

        count = 0
        rec = c2.first()
        while rec is not None:
            count = count + 1
            if verbose:
                print(rec)
            rec = c2.next()
        assert count == 9

        count = 0
        rec = c3.first()
        while rec is not None:
            count = count + 1
            if verbose:
                print(rec)
            rec = c3.next()
        assert count == 52


        c1.close()
        c2.close()
        c3.close()

        d1.close()
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

def test_suite():
    suite = unittest.TestSuite()

    suite.addTest(unittest.makeSuite(VersionTestCase))
    suite.addTest(unittest.makeSuite(BasicBTreeTestCase))
    suite.addTest(unittest.makeSuite(BasicHashTestCase))
    suite.addTest(unittest.makeSuite(BasicBTreeWithThreadFlagTestCase))
    suite.addTest(unittest.makeSuite(BasicHashWithThreadFlagTestCase))
    suite.addTest(unittest.makeSuite(BasicBTreeWithEnvTestCase))
    suite.addTest(unittest.makeSuite(BasicHashWithEnvTestCase))
    suite.addTest(unittest.makeSuite(BTreeTransactionTestCase))
    suite.addTest(unittest.makeSuite(HashTransactionTestCase))
    suite.addTest(unittest.makeSuite(BTreeRecnoTestCase))
    suite.addTest(unittest.makeSuite(BTreeRecnoWithThreadFlagTestCase))
    suite.addTest(unittest.makeSuite(BTreeDUPTestCase))
    suite.addTest(unittest.makeSuite(HashDUPTestCase))
    suite.addTest(unittest.makeSuite(BTreeDUPWithThreadTestCase))
    suite.addTest(unittest.makeSuite(HashDUPWithThreadTestCase))
    suite.addTest(unittest.makeSuite(BTreeMultiDBTestCase))
    suite.addTest(unittest.makeSuite(HashMultiDBTestCase))

    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
