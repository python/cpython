"""Miscellaneous bsddb module test cases
"""

import os, sys
import unittest

from test_all import db, dbshelve, hashopen, test_support, get_new_environment_path, get_new_database_path

#----------------------------------------------------------------------

class MiscTestCase(unittest.TestCase):
    def setUp(self):
        self.filename = get_new_database_path()
        self.homeDir = get_new_environment_path()

    def tearDown(self):
        test_support.unlink(self.filename)
        test_support.rmtree(self.homeDir)

    def test01_badpointer(self):
        dbs = dbshelve.open(self.filename)
        dbs.close()
        self.assertRaises(db.DBError, dbs.get, "foo")

    def test02_db_home(self):
        env = db.DBEnv()
        # check for crash fixed when db_home is used before open()
        self.assertTrue(env.db_home is None)
        env.open(self.homeDir, db.DB_CREATE)
        if sys.version_info[0] < 3 :
            self.assertEqual(self.homeDir, env.db_home)
        else :
            self.assertEqual(bytes(self.homeDir, "ascii"), env.db_home)

    def test03_repr_closed_db(self):
        db = hashopen(self.filename)
        db.close()
        rp = repr(db)
        self.assertEqual(rp, "{}")

    def test04_repr_db(self) :
        db = hashopen(self.filename)
        d = {}
        for i in xrange(100) :
            db[repr(i)] = repr(100*i)
            d[repr(i)] = repr(100*i)
        db.close()
        db = hashopen(self.filename)
        rp = repr(db)
        self.assertEqual(rp, repr(d))
        db.close()

    # http://sourceforge.net/tracker/index.php?func=detail&aid=1708868&group_id=13900&atid=313900
    #
    # See the bug report for details.
    #
    # The problem was that make_key_dbt() was not allocating a copy of
    # string keys but FREE_DBT() was always being told to free it when the
    # database was opened with DB_THREAD.
    def test05_double_free_make_key_dbt(self):
        try:
            db1 = db.DB()
            db1.open(self.filename, None, db.DB_BTREE,
                     db.DB_CREATE | db.DB_THREAD)

            curs = db1.cursor()
            t = curs.get("/foo", db.DB_SET)
            # double free happened during exit from DBC_get
        finally:
            db1.close()
            test_support.unlink(self.filename)

    def test06_key_with_null_bytes(self):
        try:
            db1 = db.DB()
            db1.open(self.filename, None, db.DB_HASH, db.DB_CREATE)
            db1['a'] = 'eh?'
            db1['a\x00'] = 'eh zed.'
            db1['a\x00a'] = 'eh zed eh?'
            db1['aaa'] = 'eh eh eh!'
            keys = db1.keys()
            keys.sort()
            self.assertEqual(['a', 'a\x00', 'a\x00a', 'aaa'], keys)
            self.assertEqual(db1['a'], 'eh?')
            self.assertEqual(db1['a\x00'], 'eh zed.')
            self.assertEqual(db1['a\x00a'], 'eh zed eh?')
            self.assertEqual(db1['aaa'], 'eh eh eh!')
        finally:
            db1.close()
            test_support.unlink(self.filename)

    def test07_DB_set_flags_persists(self):
        try:
            db1 = db.DB()
            db1.set_flags(db.DB_DUPSORT)
            db1.open(self.filename, db.DB_HASH, db.DB_CREATE)
            db1['a'] = 'eh'
            db1['a'] = 'A'
            self.assertEqual([('a', 'A')], db1.items())
            db1.put('a', 'Aa')
            self.assertEqual([('a', 'A'), ('a', 'Aa')], db1.items())
            db1.close()
            db1 = db.DB()
            # no set_flags call, we're testing that it reads and obeys
            # the flags on open.
            db1.open(self.filename, db.DB_HASH)
            self.assertEqual([('a', 'A'), ('a', 'Aa')], db1.items())
            # if it read the flags right this will replace all values
            # for key 'a' instead of adding a new one.  (as a dict should)
            db1['a'] = 'new A'
            self.assertEqual([('a', 'new A')], db1.items())
        finally:
            db1.close()
            test_support.unlink(self.filename)


    def test08_ExceptionTypes(self) :
        self.assertTrue(issubclass(db.DBError, Exception))
        for i, j in db.__dict__.items() :
            if i.startswith("DB") and i.endswith("Error") :
                self.assertTrue(issubclass(j, db.DBError), msg=i)
                if i not in ("DBKeyEmptyError", "DBNotFoundError") :
                    self.assertFalse(issubclass(j, KeyError), msg=i)

        # This two exceptions have two bases
        self.assertTrue(issubclass(db.DBKeyEmptyError, KeyError))
        self.assertTrue(issubclass(db.DBNotFoundError, KeyError))


#----------------------------------------------------------------------


def test_suite():
    return unittest.makeSuite(MiscTestCase)


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
