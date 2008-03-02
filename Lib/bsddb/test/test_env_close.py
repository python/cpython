"""TestCases for checking that it does not segfault when a DBEnv object
is closed before its DB objects.
"""

import os
import tempfile
import unittest

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db
except ImportError:
    # For Python 2.3
    from bsddb import db

try:
    from bsddb3 import test_support
except ImportError:
    from test import test_support

from test_all import verbose

# We're going to get warnings in this module about trying to close the db when
# its env is already closed.  Let's just ignore those.
try:
    import warnings
except ImportError:
    pass
else:
    warnings.filterwarnings('ignore',
                            message='DB could not be closed in',
                            category=RuntimeWarning)


#----------------------------------------------------------------------

class DBEnvClosedEarlyCrash(unittest.TestCase):
    def setUp(self):
        self.homeDir = os.path.join(tempfile.gettempdir(), 'db_home%d'%os.getpid())
        try: os.mkdir(self.homeDir)
        except os.error: pass
        tempfile.tempdir = self.homeDir
        self.filename = os.path.split(tempfile.mktemp())[1]
        tempfile.tempdir = None

    def tearDown(self):
        test_support.rmtree(self.homeDir)

    def test01_close_dbenv_before_db(self):
        dbenv = db.DBEnv()
        dbenv.open(self.homeDir,
                   db.DB_INIT_CDB| db.DB_CREATE |db.DB_THREAD|db.DB_INIT_MPOOL,
                   0666)

        d = db.DB(dbenv)
        d.open(self.filename, db.DB_BTREE, db.DB_CREATE | db.DB_THREAD, 0666)

        try:
            dbenv.close()
        except db.DBError:
            try:
                d.close()
            except db.DBError:
                return
            assert 0, \
                   "DB close did not raise an exception about its "\
                   "DBEnv being trashed"

        # XXX This may fail when using older versions of BerkeleyDB.
        # E.g. 3.2.9 never raised the exception.
        assert 0, "dbenv did not raise an exception about its DB being open"


    def test02_close_dbenv_delete_db_success(self):
        dbenv = db.DBEnv()
        dbenv.open(self.homeDir,
                   db.DB_INIT_CDB| db.DB_CREATE |db.DB_THREAD|db.DB_INIT_MPOOL,
                   0666)

        d = db.DB(dbenv)
        d.open(self.filename, db.DB_BTREE, db.DB_CREATE | db.DB_THREAD, 0666)

        try:
            dbenv.close()
        except db.DBError:
            pass  # good, it should raise an exception

        del d
        try:
            import gc
        except ImportError:
            gc = None
        if gc:
            # force d.__del__ [DB_dealloc] to be called
            gc.collect()


#----------------------------------------------------------------------

def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(DBEnvClosedEarlyCrash))
    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
