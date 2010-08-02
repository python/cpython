
import os, string
import unittest

from test_all import db, dbobj, test_support, get_new_environment_path, \
        get_new_database_path

#----------------------------------------------------------------------

class dbobjTestCase(unittest.TestCase):
    """Verify that dbobj.DB and dbobj.DBEnv work properly"""
    db_name = 'test-dbobj.db'

    def setUp(self):
        self.homeDir = get_new_environment_path()

    def tearDown(self):
        if hasattr(self, 'db'):
            del self.db
        if hasattr(self, 'env'):
            del self.env
        test_support.rmtree(self.homeDir)

    def test01_both(self):
        class TestDBEnv(dbobj.DBEnv): pass
        class TestDB(dbobj.DB):
            def put(self, key, *args, **kwargs):
                key = key.upper()
                # call our parent classes put method with an upper case key
                return dbobj.DB.put(self, key, *args, **kwargs)
        self.env = TestDBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)
        self.db = TestDB(self.env)
        self.db.open(self.db_name, db.DB_HASH, db.DB_CREATE)
        self.db.put('spam', 'eggs')
        self.assertEqual(self.db.get('spam'), None,
               "overridden dbobj.DB.put() method failed [1]")
        self.assertEqual(self.db.get('SPAM'), 'eggs',
               "overridden dbobj.DB.put() method failed [2]")
        self.db.close()
        self.env.close()

    def test02_dbobj_dict_interface(self):
        self.env = dbobj.DBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)
        self.db = dbobj.DB(self.env)
        self.db.open(self.db_name+'02', db.DB_HASH, db.DB_CREATE)
        # __setitem__
        self.db['spam'] = 'eggs'
        # __len__
        self.assertEqual(len(self.db), 1)
        # __getitem__
        self.assertEqual(self.db['spam'], 'eggs')
        # __del__
        del self.db['spam']
        self.assertEqual(self.db.get('spam'), None, "dbobj __del__ failed")
        self.db.close()
        self.env.close()

    def test03_dbobj_type_before_open(self):
        # Ensure this doesn't cause a segfault.
        self.assertRaises(db.DBInvalidArgError, db.DB().type)

#----------------------------------------------------------------------

def test_suite():
    return unittest.makeSuite(dbobjTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
