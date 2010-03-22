
import os
import pickle
import sys

if sys.version_info[0] < 3 :
    try:
        import cPickle
    except ImportError:
        cPickle = None
else :
    cPickle = None

import unittest

from test_all import db, test_support, get_new_environment_path, get_new_database_path

#----------------------------------------------------------------------

class pickleTestCase(unittest.TestCase):
    """Verify that DBError can be pickled and unpickled"""
    db_name = 'test-dbobj.db'

    def setUp(self):
        self.homeDir = get_new_environment_path()

    def tearDown(self):
        if hasattr(self, 'db'):
            del self.db
        if hasattr(self, 'env'):
            del self.env
        test_support.rmtree(self.homeDir)

    def _base_test_pickle_DBError(self, pickle):
        self.env = db.DBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)
        self.db = db.DB(self.env)
        self.db.open(self.db_name, db.DB_HASH, db.DB_CREATE)
        self.db.put('spam', 'eggs')
        self.assertEqual(self.db['spam'], 'eggs')
        try:
            self.db.put('spam', 'ham', flags=db.DB_NOOVERWRITE)
        except db.DBError, egg:
            pickledEgg = pickle.dumps(egg)
            #print repr(pickledEgg)
            rottenEgg = pickle.loads(pickledEgg)
            if rottenEgg.args != egg.args or type(rottenEgg) != type(egg):
                raise Exception, (rottenEgg, '!=', egg)
        else:
            raise Exception, "where's my DBError exception?!?"

        self.db.close()
        self.env.close()

    def test01_pickle_DBError(self):
        self._base_test_pickle_DBError(pickle=pickle)

    if cPickle:
        def test02_cPickle_DBError(self):
            self._base_test_pickle_DBError(pickle=cPickle)

#----------------------------------------------------------------------

def test_suite():
    return unittest.makeSuite(pickleTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
