"""TestCases for using the DB.join and DBCursor.join_item methods.
"""

import os

import unittest

from test_all import db, dbshelve, test_support, verbose, \
        get_new_environment_path, get_new_database_path

#----------------------------------------------------------------------

ProductIndex = [
    ('apple', "Convenience Store"),
    ('blueberry', "Farmer's Market"),
    ('shotgun', "S-Mart"),              # Aisle 12
    ('pear', "Farmer's Market"),
    ('chainsaw', "S-Mart"),             # "Shop smart.  Shop S-Mart!"
    ('strawberry', "Farmer's Market"),
]

ColorIndex = [
    ('blue', "blueberry"),
    ('red', "apple"),
    ('red', "chainsaw"),
    ('red', "strawberry"),
    ('yellow', "peach"),
    ('yellow', "pear"),
    ('black', "shotgun"),
]

class JoinTestCase(unittest.TestCase):
    keytype = ''

    def setUp(self):
        self.filename = self.__class__.__name__ + '.db'
        self.homeDir = get_new_environment_path()
        self.env = db.DBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL | db.DB_INIT_LOCK )

    def tearDown(self):
        self.env.close()
        test_support.rmtree(self.homeDir)

    def test01_join(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test01_join..." % \
                  self.__class__.__name__

        # create and populate primary index
        priDB = db.DB(self.env)
        priDB.open(self.filename, "primary", db.DB_BTREE, db.DB_CREATE)
        map(lambda t, priDB=priDB: priDB.put(*t), ProductIndex)

        # create and populate secondary index
        secDB = db.DB(self.env)
        secDB.set_flags(db.DB_DUP | db.DB_DUPSORT)
        secDB.open(self.filename, "secondary", db.DB_BTREE, db.DB_CREATE)
        map(lambda t, secDB=secDB: secDB.put(*t), ColorIndex)

        sCursor = None
        jCursor = None
        try:
            # lets look up all of the red Products
            sCursor = secDB.cursor()
            # Don't do the .set() in an assert, or you can get a bogus failure
            # when running python -O
            tmp = sCursor.set('red')
            self.assertTrue(tmp)

            # FIXME: jCursor doesn't properly hold a reference to its
            # cursors, if they are closed before jcursor is used it
            # can cause a crash.
            jCursor = priDB.join([sCursor])

            if jCursor.get(0) != ('apple', "Convenience Store"):
                self.fail("join cursor positioned wrong")
            if jCursor.join_item() != 'chainsaw':
                self.fail("DBCursor.join_item returned wrong item")
            if jCursor.get(0)[0] != 'strawberry':
                self.fail("join cursor returned wrong thing")
            if jCursor.get(0):  # there were only three red items to return
                self.fail("join cursor returned too many items")
        finally:
            if jCursor:
                jCursor.close()
            if sCursor:
                sCursor.close()
            priDB.close()
            secDB.close()


def test_suite():
    suite = unittest.TestSuite()

    suite.addTest(unittest.makeSuite(JoinTestCase))

    return suite
