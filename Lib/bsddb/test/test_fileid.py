"""TestCase for reseting File ID.
"""

import os
import shutil
import unittest

from test_all import db, test_support, get_new_environment_path, get_new_database_path

class FileidResetTestCase(unittest.TestCase):
    def setUp(self):
        self.db_path_1 = get_new_database_path()
        self.db_path_2 = get_new_database_path()
        self.db_env_path = get_new_environment_path()

    def test_fileid_reset(self):
        # create DB 1
        self.db1 = db.DB()
        self.db1.open(self.db_path_1, dbtype=db.DB_HASH, flags=(db.DB_CREATE|db.DB_EXCL))
        self.db1.put('spam', 'eggs')
        self.db1.close()

        shutil.copy(self.db_path_1, self.db_path_2)

        self.db2 = db.DB()
        self.db2.open(self.db_path_2, dbtype=db.DB_HASH)
        self.db2.put('spam', 'spam')
        self.db2.close()

        self.db_env = db.DBEnv()
        self.db_env.open(self.db_env_path, db.DB_CREATE|db.DB_INIT_MPOOL)

        # use fileid_reset() here
        self.db_env.fileid_reset(self.db_path_2)

        self.db1 = db.DB(self.db_env)
        self.db1.open(self.db_path_1, dbtype=db.DB_HASH, flags=db.DB_RDONLY)
        self.assertEqual(self.db1.get('spam'), 'eggs')

        self.db2 = db.DB(self.db_env)
        self.db2.open(self.db_path_2, dbtype=db.DB_HASH, flags=db.DB_RDONLY)
        self.assertEqual(self.db2.get('spam'), 'spam')

        self.db1.close()
        self.db2.close()

        self.db_env.close()

    def tearDown(self):
        test_support.unlink(self.db_path_1)
        test_support.unlink(self.db_path_2)
        test_support.rmtree(self.db_env_path)

def test_suite():
    suite = unittest.TestSuite()
    if db.version() >= (4, 4):
        suite.addTest(unittest.makeSuite(FileidResetTestCase))
    return suite

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
