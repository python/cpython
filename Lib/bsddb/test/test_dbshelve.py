"""
TestCases for checking dbShelve objects.
"""

import os, string
import random
import unittest
import warnings


from test_all import db, dbshelve, test_support, verbose, \
        get_new_environment_path, get_new_database_path



#----------------------------------------------------------------------

# We want the objects to be comparable so we can test dbshelve.values
# later on.
class DataClass:
    def __init__(self):
        self.value = random.random()

    def __repr__(self) :  # For Python 3.0 comparison
        return "DataClass %f" %self.value

    def __cmp__(self, other):  # For Python 2.x comparison
        return cmp(self.value, other)


class DBShelveTestCase(unittest.TestCase):
    def setUp(self):
        import sys
        if sys.version_info[0] >= 3 :
            from test_all import do_proxy_db_py3k
            self._flag_proxy_db_py3k = do_proxy_db_py3k(False)
        self.filename = get_new_database_path()
        self.do_open()

    def tearDown(self):
        import sys
        if sys.version_info[0] >= 3 :
            from test_all import do_proxy_db_py3k
            do_proxy_db_py3k(self._flag_proxy_db_py3k)
        self.do_close()
        test_support.unlink(self.filename)

    def mk(self, key):
        """Turn key into an appropriate key type for this db"""
        # override in child class for RECNO
        import sys
        if sys.version_info[0] < 3 :
            return key
        else :
            return bytes(key, "iso8859-1")  # 8 bits

    def populateDB(self, d):
        for x in string.letters:
            d[self.mk('S' + x)] = 10 * x           # add a string
            d[self.mk('I' + x)] = ord(x)           # add an integer
            d[self.mk('L' + x)] = [x] * 10         # add a list

            inst = DataClass()            # add an instance
            inst.S = 10 * x
            inst.I = ord(x)
            inst.L = [x] * 10
            d[self.mk('O' + x)] = inst


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

        self.assertEqual(0, d.has_key(self.mk('bad key')))
        self.assertEqual(1, d.has_key(self.mk('IA')))
        self.assertEqual(1, d.has_key(self.mk('OA')))

        d.delete(self.mk('IA'))
        del d[self.mk('OA')]
        self.assertEqual(0, d.has_key(self.mk('IA')))
        self.assertEqual(0, d.has_key(self.mk('OA')))
        self.assertEqual(len(d), l-2)

        values = []
        for key in d.keys():
            value = d[key]
            values.append(value)
            if verbose:
                print "%s: %s" % (key, value)
            self.checkrec(key, value)

        dbvalues = d.values()
        self.assertEqual(len(dbvalues), len(d.keys()))
        with warnings.catch_warnings():
            warnings.filterwarnings('ignore',
                                    'comparing unequal types not supported',
                                    DeprecationWarning)
            self.assertEqual(sorted(values), sorted(dbvalues))

        items = d.items()
        self.assertEqual(len(items), len(values))

        for key, value in items:
            self.checkrec(key, value)

        self.assertEqual(d.get(self.mk('bad key')), None)
        self.assertEqual(d.get(self.mk('bad key'), None), None)
        self.assertEqual(d.get(self.mk('bad key'), 'a string'), 'a string')
        self.assertEqual(d.get(self.mk('bad key'), [1, 2, 3]), [1, 2, 3])

        d.set_get_returns_none(0)
        self.assertRaises(db.DBNotFoundError, d.get, self.mk('bad key'))
        d.set_get_returns_none(1)

        d.put(self.mk('new key'), 'new data')
        self.assertEqual(d.get(self.mk('new key')), 'new data')
        self.assertEqual(d[self.mk('new key')], 'new data')



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
            # Hack to avoid conversion by 2to3 tool
            rec = getattr(c, "next")()
        del c

        self.assertEqual(count, len(d))

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

        self.assertEqual(count, len(d))

        c.set(self.mk('SS'))
        key, value = c.current()
        self.checkrec(key, value)
        del c


    def test03_append(self):
        # NOTE: this is overridden in RECNO subclass, don't change its name.
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test03_append..." % self.__class__.__name__

        self.assertRaises(dbshelve.DBShelveError,
                          self.d.append, 'unit test was here')


    def checkrec(self, key, value):
        # override this in a subclass if the key type is different

        import sys
        if sys.version_info[0] >= 3 :
            if isinstance(key, bytes) :
                key = key.decode("iso8859-1")  # 8 bits

        x = key[1]
        if key[0] == 'S':
            self.assertEqual(type(value), str)
            self.assertEqual(value, 10 * x)

        elif key[0] == 'I':
            self.assertEqual(type(value), int)
            self.assertEqual(value, ord(x))

        elif key[0] == 'L':
            self.assertEqual(type(value), list)
            self.assertEqual(value, [x] * 10)

        elif key[0] == 'O':
            import sys
            if sys.version_info[0] < 3 :
                from types import InstanceType
                self.assertEqual(type(value), InstanceType)
            else :
                self.assertEqual(type(value), DataClass)

            self.assertEqual(value.S, 10 * x)
            self.assertEqual(value.I, ord(x))
            self.assertEqual(value.L, [x] * 10)

        else:
            self.assert_(0, 'Unknown key type, fix the test')

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
        self.env = db.DBEnv()
        self.env.open(self.homeDir,
                self.envflags | db.DB_INIT_MPOOL | db.DB_CREATE)

        self.filename = os.path.split(self.filename)[1]
        self.d = dbshelve.DBShelf(self.env)
        self.d.open(self.filename, self.dbtype, self.dbflags)


    def do_close(self):
        self.d.close()
        self.env.close()


    def setUp(self) :
        self.homeDir = get_new_environment_path()
        DBShelveTestCase.setUp(self)

    def tearDown(self):
        import sys
        if sys.version_info[0] >= 3 :
            from test_all import do_proxy_db_py3k
            do_proxy_db_py3k(self._flag_proxy_db_py3k)
        self.do_close()
        test_support.rmtree(self.homeDir)


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
# test cases for a DBShelf in a RECNO DB.

class RecNoShelveTestCase(BasicShelveTestCase):
    dbtype = db.DB_RECNO
    dbflags = db.DB_CREATE

    def setUp(self):
        BasicShelveTestCase.setUp(self)

        # pool to assign integer key values out of
        self.key_pool = list(range(1, 5000))
        self.key_map = {}     # map string keys to the number we gave them
        self.intkey_map = {}  # reverse map of above

    def mk(self, key):
        if key not in self.key_map:
            self.key_map[key] = self.key_pool.pop(0)
            self.intkey_map[self.key_map[key]] = key
        return self.key_map[key]

    def checkrec(self, intkey, value):
        key = self.intkey_map[intkey]
        BasicShelveTestCase.checkrec(self, key, value)

    def test03_append(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test03_append..." % self.__class__.__name__

        self.d[1] = 'spam'
        self.d[5] = 'eggs'
        self.assertEqual(6, self.d.append('spam'))
        self.assertEqual(7, self.d.append('baked beans'))
        self.assertEqual('spam', self.d.get(6))
        self.assertEqual('spam', self.d.get(1))
        self.assertEqual('baked beans', self.d.get(7))
        self.assertEqual('eggs', self.d.get(5))


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
    suite.addTest(unittest.makeSuite(RecNoShelveTestCase))

    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
