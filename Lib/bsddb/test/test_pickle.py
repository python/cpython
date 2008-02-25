
import shutil
import sys, os
import pickle
import tempfile
import unittest
import tempfile

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db
except ImportError as e:
    # For Python 2.3
    from bsddb import db


#----------------------------------------------------------------------

class pickleTestCase(unittest.TestCase):
    """Verify that DBError can be pickled and unpickled"""
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

    def _base_test_pickle_DBError(self, pickle):
        self.env = db.DBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)
        self.db = db.DB(self.env)
        self.db.open(self.db_name, db.DB_HASH, db.DB_CREATE)
        self.db.put(b'spam', b'eggs')
        self.assertEqual(self.db[b'spam'], b'eggs')
        try:
            self.db.put(b'spam', b'ham', flags=db.DB_NOOVERWRITE)
        except db.DBError as egg:
            pickledEgg = pickle.dumps(egg)
            #print repr(pickledEgg)
            rottenEgg = pickle.loads(pickledEgg)
            if rottenEgg.args != egg.args or type(rottenEgg) != type(egg):
                raise Exception(rottenEgg, '!=', egg)
        else:
            self.fail("where's my DBError exception?!?")

        self.db.close()
        self.env.close()

    def test01_pickle_DBError(self):
        self._base_test_pickle_DBError(pickle=pickle)

#----------------------------------------------------------------------

def test_suite():
    return unittest.makeSuite(pickleTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
