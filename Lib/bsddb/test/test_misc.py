"""Miscellaneous bsddb module test cases
"""

import os
import shutil
import sys
import unittest
import tempfile

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db, dbshelve, hashopen
except ImportError:
    # For Python 2.3
    from bsddb import db, dbshelve, hashopen

#----------------------------------------------------------------------

class MiscTestCase(unittest.TestCase):
    def setUp(self):
        self.filename = self.__class__.__name__ + '.db'
        self.homeDir = tempfile.mkdtemp()

    def tearDown(self):
        try:
            os.remove(self.filename)
        except OSError:
            pass
        shutil.rmtree(self.homeDir)

    def test01_badpointer(self):
        dbs = dbshelve.open(self.filename)
        dbs.close()
        self.assertRaises(db.DBError, dbs.get, "foo")

    def test02_db_home(self):
        env = db.DBEnv()
        # check for crash fixed when db_home is used before open()
        assert env.db_home is None
        env.open(self.homeDir, db.DB_CREATE)
        assert self.homeDir == env.db_home

    def test03_repr_closed_db(self):
        db = hashopen(self.filename)
        db.close()
        rp = repr(db)
        self.assertEquals(rp, "{}")


#----------------------------------------------------------------------


def test_suite():
    return unittest.makeSuite(MiscTestCase)


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
