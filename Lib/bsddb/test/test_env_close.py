"""
TestCases for checking that it does not segfault when a DBEnv object
is closed before its DB objects.
"""

import sys, os, string
from pprint import pprint
import tempfile
import glob
import unittest

from bsddb import db

from test.test_support import verbose


#----------------------------------------------------------------------

class DBEnvClosedEarlyCrash(unittest.TestCase):
    def setUp(self):
        self.homeDir = os.path.join(os.path.dirname(sys.argv[0]), 'db_home')
        try: os.mkdir(self.homeDir)
        except os.error: pass
        tempfile.tempdir = self.homeDir
        self.filename = os.path.split(tempfile.mktemp())[1]
        tempfile.tempdir = None

    def tearDown(self):
        files = glob.glob(os.path.join(self.homeDir, '*'))
        for file in files:
            os.remove(file)


    def test01_close_dbenv_before_db(self):
        dbenv = db.DBEnv()
        dbenv.open(self.homeDir,db.DB_INIT_CDB| db.DB_CREATE |db.DB_THREAD|db.DB_INIT_MPOOL, 0666)

        d = db.DB(dbenv)
        d.open(self.filename, db.DB_BTREE, db.DB_CREATE | db.DB_THREAD, 0666)

        try:
            dbenv.close()
        except db.DBError:
            try:
                d.close()
            except db.DBError:
                return
            assert 0, "DB close did not raise an exception about its DBEnv being trashed"

        assert 0, "dbenv did not raise an exception about its DB being open"


    def test02_close_dbenv_delete_db_success(self):
        dbenv = db.DBEnv()
        dbenv.open(self.homeDir,db.DB_INIT_CDB| db.DB_CREATE |db.DB_THREAD|db.DB_INIT_MPOOL, 0666)

        d = db.DB(dbenv)
        d.open(self.filename, db.DB_BTREE, db.DB_CREATE | db.DB_THREAD, 0666)

        try:
            dbenv.close()
        except db.DBError:
            pass  # good, it should raise an exception

        # this should not raise an exception, it should silently skip
        # the db->close() call as it can't be done safely.
        del d
        try:
            import gc
        except ImportError:
            gc = None
        if gc:
            # force d.__del__ [DB_dealloc] to be called
            gc.collect()


#----------------------------------------------------------------------

def suite():
    return unittest.makeSuite(DBEnvClosedEarlyCrash)


if __name__ == '__main__':
    unittest.main( defaultTest='suite' )
