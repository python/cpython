"""
TestCases for checking dbShelve objects.
"""

import sys, os, string
import tempfile, random
from pprint import pprint
from types import *
import unittest

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db, dbshelve
except ImportError:
    # For Python 2.3
    from bsddb import db, dbshelve

from test_all import verbose


#----------------------------------------------------------------------

# We want the objects to be comparable so we can test dbshelve.values
# later on.
class DataClass:
    def __init__(self):
        self.value = random.random()

    def __cmp__(self, other):
        return cmp(self.value, other)

class DBShelveTestCase(unittest.TestCase):
    def setUp(self):
        self.filename = tempfile.mktemp()
        self.do_open()

    def tearDown(self):
        self.do_close()
        try:
            os.remove(self.filename)
        except os.error:
            pass

    def populateDB(self, d):
        for x in string.letters:
            d['S' + x] = 10 * x           # add a string
            d['I' + x] = ord(x)           # add an integer
            d['L' + x] = [x] * 10         # add a list

            inst = DataClass()            # add an instance
            inst.S = 10 * x
            inst.I = ord(x)
            inst.L = [x] * 10
            d['O' + x] = inst


    # overridable in derived classes to affect how the shelf is created/opened
    def do_open(self):
        self.d = dbshelve.open(self.filename)

    # and closed...
    def do_close(self):
        self.d.close()



    def test01_basics(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test01_basics..." % self.__class__.__name__

        self.populateDB(self.d)
        self.d.sync()
        self.do_close()
        self.do_open()
        d = self.d

        l = len(d)
        k = d.keys()
        s = d.stat()
        f = d.fd()

        if verbose:
            print "length:", l
            print "keys:", k
            print "stats:", s

        assert 0 == d.has_key('bad key')
        assert 1 == d.has_key('IA')
        assert 1 == d.has_key('OA')

        d.delete('IA')
        del d['OA']
        assert 0 == d.has_key('IA')
        assert 0 == d.has_key('OA')
        assert len(d) == l-2

        values = []
        for key in d.keys():
            value = d[key]
            values.append(value)
            if verbose:
                print "%s: %s" % (key, value)
            self.checkrec(key, value)

        dbvalues = d.values()
        assert len(dbvalues) == len(d.keys())
        values.sort()
        dbvalues.sort()
        assert values == dbvalues

        items = d.items()
        assert len(items) == len(values)

        for key, value in items:
            self.checkrec(key, value)

        assert d.get('bad key') == None
        assert d.get('bad key', None) == None
        assert d.get('bad key', 'a string') == 'a string'
        assert d.get('bad key', [1, 2, 3]) == [1, 2, 3]

        d.set_get_returns_none(0)
        self.assertRaises(db.DBNotFoundError, d.get, 'bad key')
        d.set_get_returns_none(1)

        d.put('new key', 'new data')
        assert d.get('new key') == 'new data'
        assert d['new key'] == 'new data'



    def test02_cursors(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test02_cursors..." % self.__class__.__name__

        self.populateDB(self.d)
        d = self.d

        count = 0
        c = d.cursor()
        rec = c.first()
        while rec is not None:
            count = count + 1
            if verbose:
                print rec
            key, value = rec
            self.checkrec(key, value)
            rec = c.next()
        del c

        assert count == len(d)

        count = 0
        c = d.cursor()
        rec = c.last()
        while rec is not None:
            count = count + 1
            if verbose:
                print rec
            key, value = rec
            self.checkrec(key, value)
            rec = c.prev()

        assert count == len(d)

        c.set('SS')
        key, value = c.current()
        self.checkrec(key, value)
        del c



    def checkrec(self, key, value):
        x = key[1]
        if key[0] == 'S':
            assert type(value) == StringType
            assert value == 10 * x

        elif key[0] == 'I':
            assert type(value) == IntType
            assert value == ord(x)

        elif key[0] == 'L':
            assert type(value) == ListType
            assert value == [x] * 10

        elif key[0] == 'O':
            assert type(value) == InstanceType
            assert value.S == 10 * x
            assert value.I == ord(x)
            assert value.L == [x] * 10

        else:
            raise AssertionError, 'Unknown key type, fix the test'

#----------------------------------------------------------------------

class BasicShelveTestCase(DBShelveTestCase):
    def do_open(self):
        self.d = dbshelve.DBShelf()
        self.d.open(self.filename, self.dbtype, self.dbflags)

    def do_close(self):
        self.d.close()


class BTreeShelveTestCase(BasicShelveTestCase):
    dbtype = db.DB_BTREE
    dbflags = db.DB_CREATE


class HashShelveTestCase(BasicShelveTestCase):
    dbtype = db.DB_HASH
    dbflags = db.DB_CREATE


class ThreadBTreeShelveTestCase(BasicShelveTestCase):
    dbtype = db.DB_BTREE
    dbflags = db.DB_CREATE | db.DB_THREAD


class ThreadHashShelveTestCase(BasicShelveTestCase):
    dbtype = db.DB_HASH
    dbflags = db.DB_CREATE | db.DB_THREAD


#----------------------------------------------------------------------

class BasicEnvShelveTestCase(DBShelveTestCase):
    def do_open(self):
        self.homeDir = homeDir = os.path.join(
            os.path.dirname(sys.argv[0]), 'db_home')
        try: os.mkdir(homeDir)
        except os.error: pass
        self.env = db.DBEnv()
        self.env.open(homeDir, self.envflags | db.DB_INIT_MPOOL | db.DB_CREATE)

        self.filename = os.path.split(self.filename)[1]
        self.d = dbshelve.DBShelf(self.env)
        self.d.open(self.filename, self.dbtype, self.dbflags)


    def do_close(self):
        self.d.close()
        self.env.close()


    def tearDown(self):
        self.do_close()
        import glob
        files = glob.glob(os.path.join(self.homeDir, '*'))
        for file in files:
            os.remove(file)



class EnvBTreeShelveTestCase(BasicEnvShelveTestCase):
    envflags = 0
    dbtype = db.DB_BTREE
    dbflags = db.DB_CREATE


class EnvHashShelveTestCase(BasicEnvShelveTestCase):
    envflags = 0
    dbtype = db.DB_HASH
    dbflags = db.DB_CREATE


class EnvThreadBTreeShelveTestCase(BasicEnvShelveTestCase):
    envflags = db.DB_THREAD
    dbtype = db.DB_BTREE
    dbflags = db.DB_CREATE | db.DB_THREAD


class EnvThreadHashShelveTestCase(BasicEnvShelveTestCase):
    envflags = db.DB_THREAD
    dbtype = db.DB_HASH
    dbflags = db.DB_CREATE | db.DB_THREAD


#----------------------------------------------------------------------
# TODO:  Add test cases for a DBShelf in a RECNO DB.


#----------------------------------------------------------------------

def test_suite():
    suite = unittest.TestSuite()

    suite.addTest(unittest.makeSuite(DBShelveTestCase))
    suite.addTest(unittest.makeSuite(BTreeShelveTestCase))
    suite.addTest(unittest.makeSuite(HashShelveTestCase))
    suite.addTest(unittest.makeSuite(ThreadBTreeShelveTestCase))
    suite.addTest(unittest.makeSuite(ThreadHashShelveTestCase))
    suite.addTest(unittest.makeSuite(EnvBTreeShelveTestCase))
    suite.addTest(unittest.makeSuite(EnvHashShelveTestCase))
    suite.addTest(unittest.makeSuite(EnvThreadBTreeShelveTestCase))
    suite.addTest(unittest.makeSuite(EnvThreadHashShelveTestCase))

    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
