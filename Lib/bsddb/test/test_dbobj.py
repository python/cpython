
import shutil
import sys, os
import unittest
import tempfile

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db, dbobj
except ImportError:
    # For Python 2.3
    from bsddb import db, dbobj


#----------------------------------------------------------------------

class dbobjTestCase(unittest.TestCase):
    """Verify that dbobj.DB and dbobj.DBEnv work properly"""
    db_name = 'test-dbobj.db'

    def setUp(self):
        homeDir = os.path.join(tempfile.gettempdir(), 'db_home%d'%os.getpid())
        self.homeDir = homeDir
        try: os.mkdir(homeDir)
        except os.error: pass

    def tearDown(self):
        if hasattr(self, 'db'):
            del self.db
        if hasattr(self, 'env'):
            del self.env
        from test import test_support
        test_support.rmtree(self.homeDir)

    def test01_both(self):
        class TestDBEnv(dbobj.DBEnv): pass
        class TestDB(dbobj.DB):
            def put(self, key, *args, **kwargs):
                key = key.decode("ascii").upper().encode("ascii")
                # call our parent classes put method with an upper case key
                return dbobj.DB.put(self, key, *args, **kwargs)
        self.env = TestDBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)
        self.db = TestDB(self.env)
        self.db.open(self.db_name, db.DB_HASH, db.DB_CREATE)
        self.db.put(b'spam', b'eggs')
        assert self.db.get(b'spam') == None, \
               "overridden dbobj.DB.put() method failed [1]"
        assert self.db.get(b'SPAM') == b'eggs', \
               "overridden dbobj.DB.put() method failed [2]"
        self.db.close()
        self.env.close()

    def test02_dbobj_dict_interface(self):
        self.env = dbobj.DBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)
        self.db = dbobj.DB(self.env)
        self.db.open(self.db_name+'02', db.DB_HASH, db.DB_CREATE)
        # __setitem__
        self.db[b'spam'] = b'eggs'
        # __len__
        assert len(self.db) == 1
        # __getitem__
        assert self.db[b'spam'] == b'eggs'
        # __del__
        del self.db[b'spam']
        assert self.db.get(b'spam') == None, "dbobj __del__ failed"
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
