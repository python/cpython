"""
TestCases for checking dbShelve objects.
"""

import sys, os, string
import tempfile, random
from pprint import pprint
from types import *
import unittest

from bsddb import db, dbshelve

from .test_all import verbose


#----------------------------------------------------------------------

# We want the objects to be comparable so we can test dbshelve.values
# later on.
class DataClass:

    def __init__(self):
        self.value = random.random()

    def __repr__(self):
        return "DataClass(%r)" % self.value

    def __eq__(self, other):
        value = self.value
        if isinstance(other, DataClass):
            other = other.value
        return value == other

    def __lt__(self, other):
        value = self.value
        if isinstance(other, DataClass):
            other = other.value
        return value < other

letters = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'

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
        for x in letters:
            d[('S' + x).encode("ascii")] = 10 * x           # add a string
            d[('I' + x).encode("ascii")] = ord(x)           # add an integer
            d[('L' + x).encode("ascii")] = [x] * 10         # add a list

            inst = DataClass()            # add an instance
            inst.S = 10 * x
            inst.I = ord(x)
            inst.L = [x] * 10
            d[('O' + x).encode("ascii")] = inst


    # overridable in derived classes to affect how the shelf is created/opened
    def do_open(self):
        self.d = dbshelve.open(self.filename)

    # and closed...
    def do_close(self):
        self.d.close()



    def test01_basics(self):
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test01_basics..." % self.__class__.__name__)

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
            print("length:", l)
            print("keys:", k)
            print("stats:", s)

        assert 0 == d.has_key(b'bad key')
        assert 1 == d.has_key(b'IA')
        assert 1 == d.has_key(b'OA')

        d.delete(b'IA')
        del d[b'OA']
        assert 0 == d.has_key(b'IA')
        assert 0 == d.has_key(b'OA')
        assert len(d) == l-2

        values = []
        for key in d.keys():
            value = d[key]
            values.append(value)
            if verbose:
                print("%s: %s" % (key, value))
            self.checkrec(key, value)

        dbvalues = sorted(d.values(), key=lambda x: (str(type(x)), x))
        assert len(dbvalues) == len(d.keys())
        values.sort(key=lambda x: (str(type(x)), x))
        assert values == dbvalues, "%r != %r" % (values, dbvalues)

        items = d.items()
        assert len(items) == len(values)

        for key, value in items:
            self.checkrec(key, value)

        assert d.get(b'bad key') == None
        assert d.get(b'bad key', None) == None
        assert d.get(b'bad key', b'a string') == b'a string'
        assert d.get(b'bad key', [1, 2, 3]) == [1, 2, 3]

        d.set_get_returns_none(0)
        self.assertRaises(db.DBNotFoundError, d.get, b'bad key')
        d.set_get_returns_none(1)

        d.put(b'new key', b'new data')
        assert d.get(b'new key') == b'new data'
        assert d[b'new key'] == b'new data'



    def test02_cursors(self):
        if verbose:
            print('\n', '-=' * 30)
            print("Running %s.test02_cursors..." % self.__class__.__name__)

        self.populateDB(self.d)
        d = self.d

        count = 0
        c = d.cursor()
        rec = c.first()
        while rec is not None:
            count = count + 1
            if verbose:
                print(repr(rec))
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
                print(rec)
            key, value = rec
            self.checkrec(key, value)
            rec = c.prev()

        assert count == len(d)

        c.set(b'SS')
        key, value = c.current()
        self.checkrec(key, value)
        del c

    def checkrec(self, key, value):
        x = key[1:]
        if key[0:1] == b'S':
            self.assertEquals(type(value), str)
            self.assertEquals(value, 10 * x.decode("ascii"))

        elif key[0:1] == b'I':
            self.assertEquals(type(value), int)
            self.assertEquals(value, ord(x))

        elif key[0:1] == b'L':
            self.assertEquals(type(value), list)
            self.assertEquals(value, [x.decode("ascii")] * 10)

        elif key[0:1] == b'O':
            self.assertEquals(value.S, 10 * x.decode("ascii"))
            self.assertEquals(value.I, ord(x))
            self.assertEquals(value.L, [x.decode("ascii")] * 10)

        else:
            self.fail('Unknown key type, fix the test')

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
            tempfile.gettempdir(), 'db_home')
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
