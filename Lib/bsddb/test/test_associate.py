"""
TestCases for DB.associate.
"""

import sys, os, string
import time
from pprint import pprint

import unittest
from test_all import db, dbshelve, test_support, verbose, have_threads, \
        get_new_environment_path


#----------------------------------------------------------------------


musicdata = {
1 : ("Bad English", "The Price Of Love", "Rock"),
2 : ("DNA featuring Suzanne Vega", "Tom's Diner", "Rock"),
3 : ("George Michael", "Praying For Time", "Rock"),
4 : ("Gloria Estefan", "Here We Are", "Rock"),
5 : ("Linda Ronstadt", "Don't Know Much", "Rock"),
6 : ("Michael Bolton", "How Am I Supposed To Live Without You", "Blues"),
7 : ("Paul Young", "Oh Girl", "Rock"),
8 : ("Paula Abdul", "Opposites Attract", "Rock"),
9 : ("Richard Marx", "Should've Known Better", "Rock"),
10: ("Rod Stewart", "Forever Young", "Rock"),
11: ("Roxette", "Dangerous", "Rock"),
12: ("Sheena Easton", "The Lover In Me", "Rock"),
13: ("Sinead O'Connor", "Nothing Compares 2 U", "Rock"),
14: ("Stevie B.", "Because I Love You", "Rock"),
15: ("Taylor Dayne", "Love Will Lead You Back", "Rock"),
16: ("The Bangles", "Eternal Flame", "Rock"),
17: ("Wilson Phillips", "Release Me", "Rock"),
18: ("Billy Joel", "Blonde Over Blue", "Rock"),
19: ("Billy Joel", "Famous Last Words", "Rock"),
20: ("Billy Joel", "Lullabye (Goodnight, My Angel)", "Rock"),
21: ("Billy Joel", "The River Of Dreams", "Rock"),
22: ("Billy Joel", "Two Thousand Years", "Rock"),
23: ("Janet Jackson", "Alright", "Rock"),
24: ("Janet Jackson", "Black Cat", "Rock"),
25: ("Janet Jackson", "Come Back To Me", "Rock"),
26: ("Janet Jackson", "Escapade", "Rock"),
27: ("Janet Jackson", "Love Will Never Do (Without You)", "Rock"),
28: ("Janet Jackson", "Miss You Much", "Rock"),
29: ("Janet Jackson", "Rhythm Nation", "Rock"),
30: ("Janet Jackson", "State Of The World", "Rock"),
31: ("Janet Jackson", "The Knowledge", "Rock"),
32: ("Spyro Gyra", "End of Romanticism", "Jazz"),
33: ("Spyro Gyra", "Heliopolis", "Jazz"),
34: ("Spyro Gyra", "Jubilee", "Jazz"),
35: ("Spyro Gyra", "Little Linda", "Jazz"),
36: ("Spyro Gyra", "Morning Dance", "Jazz"),
37: ("Spyro Gyra", "Song for Lorraine", "Jazz"),
38: ("Yes", "Owner Of A Lonely Heart", "Rock"),
39: ("Yes", "Rhythm Of Love", "Rock"),
40: ("Cusco", "Dream Catcher", "New Age"),
41: ("Cusco", "Geronimos Laughter", "New Age"),
42: ("Cusco", "Ghost Dance", "New Age"),
43: ("Blue Man Group", "Drumbone", "New Age"),
44: ("Blue Man Group", "Endless Column", "New Age"),
45: ("Blue Man Group", "Klein Mandelbrot", "New Age"),
46: ("Kenny G", "Silhouette", "Jazz"),
47: ("Sade", "Smooth Operator", "Jazz"),
48: ("David Arkenstone", "Papillon (On The Wings Of The Butterfly)",
     "New Age"),
49: ("David Arkenstone", "Stepping Stars", "New Age"),
50: ("David Arkenstone", "Carnation Lily Lily Rose", "New Age"),
51: ("David Lanz", "Behind The Waterfall", "New Age"),
52: ("David Lanz", "Cristofori's Dream", "New Age"),
53: ("David Lanz", "Heartsounds", "New Age"),
54: ("David Lanz", "Leaves on the Seine", "New Age"),
99: ("unknown artist", "Unnamed song", "Unknown"),
}

#----------------------------------------------------------------------

class AssociateErrorTestCase(unittest.TestCase):
    def setUp(self):
        self.filename = self.__class__.__name__ + '.db'
        self.homeDir = get_new_environment_path()
        self.env = db.DBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL)

    def tearDown(self):
        self.env.close()
        self.env = None
        test_support.rmtree(self.homeDir)

    def test00_associateDBError(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test00_associateDBError..." % \
                  self.__class__.__name__

        dupDB = db.DB(self.env)
        dupDB.set_flags(db.DB_DUP)
        dupDB.open(self.filename, "primary", db.DB_BTREE, db.DB_CREATE)

        secDB = db.DB(self.env)
        secDB.open(self.filename, "secondary", db.DB_BTREE, db.DB_CREATE)

        # dupDB has been configured to allow duplicates, it can't
        # associate with a secondary.  Berkeley DB will return an error.
        try:
            def f(a,b): return a+b
            dupDB.associate(secDB, f)
        except db.DBError:
            # good
            secDB.close()
            dupDB.close()
        else:
            secDB.close()
            dupDB.close()
            self.fail("DBError exception was expected")

    @unittest.skipUnless(db.version() >= (4, 6), 'Needs 4.6+')
    def test_associateListError(self):
        db1 = db.DB(self.env)
        db1.open('bad.db', "a.db", db.DB_BTREE, db.DB_CREATE)
        db2 = db.DB(self.env)
        db2.open('bad.db', "b.db", db.DB_BTREE, db.DB_CREATE)

        db1.associate(db2, lambda a, b: [0])

        msg = "TypeError: The list returned by DB->associate callback" \
              " should be a list of strings."
        with test_support.captured_output("stderr") as s:
            db1.put("0", "1")
        db1.close()
        db2.close()
        self.assertEquals(s.getvalue().strip(), msg)


#----------------------------------------------------------------------


class AssociateTestCase(unittest.TestCase):
    keytype = ''
    envFlags = 0
    dbFlags = 0

    def setUp(self):
        self.filename = self.__class__.__name__ + '.db'
        self.homeDir = get_new_environment_path()
        self.env = db.DBEnv()
        self.env.open(self.homeDir, db.DB_CREATE | db.DB_INIT_MPOOL |
                               db.DB_INIT_LOCK | db.DB_THREAD | self.envFlags)

    def tearDown(self):
        self.closeDB()
        self.env.close()
        self.env = None
        test_support.rmtree(self.homeDir)

    def addDataToDB(self, d, txn=None):
        for key, value in musicdata.items():
            if type(self.keytype) == type(''):
                key = "%02d" % key
            d.put(key, '|'.join(value), txn=txn)

    def createDB(self, txn=None):
        self.cur = None
        self.secDB = None
        self.primary = db.DB(self.env)
        self.primary.set_get_returns_none(2)
        self.primary.open(self.filename, "primary", self.dbtype,
                      db.DB_CREATE | db.DB_THREAD | self.dbFlags, txn=txn)

    def closeDB(self):
        if self.cur:
            self.cur.close()
            self.cur = None
        if self.secDB:
            self.secDB.close()
            self.secDB = None
        self.primary.close()
        self.primary = None

    def getDB(self):
        return self.primary


    def _associateWithDB(self, getGenre):
        self.createDB()

        self.secDB = db.DB(self.env)
        self.secDB.set_flags(db.DB_DUP)
        self.secDB.set_get_returns_none(2)
        self.secDB.open(self.filename, "secondary", db.DB_BTREE,
                   db.DB_CREATE | db.DB_THREAD | self.dbFlags)
        self.getDB().associate(self.secDB, getGenre)

        self.addDataToDB(self.getDB())

        self.finish_test(self.secDB)

    def test01_associateWithDB(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test01_associateWithDB..." % \
                  self.__class__.__name__

        return self._associateWithDB(self.getGenre)

    def _associateAfterDB(self, getGenre) :
        self.createDB()
        self.addDataToDB(self.getDB())

        self.secDB = db.DB(self.env)
        self.secDB.set_flags(db.DB_DUP)
        self.secDB.open(self.filename, "secondary", db.DB_BTREE,
                   db.DB_CREATE | db.DB_THREAD | self.dbFlags)

        # adding the DB_CREATE flag will cause it to index existing records
        self.getDB().associate(self.secDB, getGenre, db.DB_CREATE)

        self.finish_test(self.secDB)

    def test02_associateAfterDB(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test02_associateAfterDB..." % \
                  self.__class__.__name__

        return self._associateAfterDB(self.getGenre)

    if db.version() >= (4, 6):
        def test03_associateWithDB(self):
            if verbose:
                print '\n', '-=' * 30
                print "Running %s.test03_associateWithDB..." % \
                      self.__class__.__name__

            return self._associateWithDB(self.getGenreList)

        def test04_associateAfterDB(self):
            if verbose:
                print '\n', '-=' * 30
                print "Running %s.test04_associateAfterDB..." % \
                      self.__class__.__name__

            return self._associateAfterDB(self.getGenreList)


    def finish_test(self, secDB, txn=None):
        # 'Blues' should not be in the secondary database
        vals = secDB.pget('Blues', txn=txn)
        self.assertEqual(vals, None, vals)

        vals = secDB.pget('Unknown', txn=txn)
        self.assertIn(vals[0], (99, '99'), vals)
        vals[1].index('Unknown')
        vals[1].index('Unnamed')
        vals[1].index('unknown')

        if verbose:
            print "Primary key traversal:"
        self.cur = self.getDB().cursor(txn)
        count = 0
        rec = self.cur.first()
        while rec is not None:
            if type(self.keytype) == type(''):
                self.assertTrue(int(rec[0]))  # for primary db, key is a number
            else:
                self.assertTrue(rec[0])
                self.assertIs(type(rec[0]), int)
            count = count + 1
            if verbose:
                print rec
            rec = getattr(self.cur, "next")()
        self.assertEqual(count, len(musicdata))  # all items accounted for


        if verbose:
            print "Secondary key traversal:"
        self.cur = secDB.cursor(txn)
        count = 0

        # test cursor pget
        vals = self.cur.pget('Unknown', flags=db.DB_LAST)
        self.assertIn(vals[1], (99, '99'), vals)
        self.assertEqual(vals[0], 'Unknown')
        vals[2].index('Unknown')
        vals[2].index('Unnamed')
        vals[2].index('unknown')

        vals = self.cur.pget('Unknown', data='wrong value', flags=db.DB_GET_BOTH)
        self.assertEqual(vals, None, vals)

        rec = self.cur.first()
        self.assertEqual(rec[0], "Jazz")
        while rec is not None:
            count = count + 1
            if verbose:
                print rec
            rec = getattr(self.cur, "next")()
        # all items accounted for EXCEPT for 1 with "Blues" genre
        self.assertEqual(count, len(musicdata)-1)

        self.cur = None

    def getGenre(self, priKey, priData):
        self.assertEqual(type(priData), type(""))
        genre = priData.split('|')[2]

        if verbose:
            print 'getGenre key: %r data: %r' % (priKey, priData)

        if genre == 'Blues':
            return db.DB_DONOTINDEX
        else:
            return genre

    def getGenreList(self, priKey, PriData) :
        v = self.getGenre(priKey, PriData)
        if type(v) == type("") :
            v = [v]
        return v


#----------------------------------------------------------------------


class AssociateHashTestCase(AssociateTestCase):
    dbtype = db.DB_HASH

class AssociateBTreeTestCase(AssociateTestCase):
    dbtype = db.DB_BTREE

class AssociateRecnoTestCase(AssociateTestCase):
    dbtype = db.DB_RECNO
    keytype = 0

#----------------------------------------------------------------------

class AssociateBTreeTxnTestCase(AssociateBTreeTestCase):
    envFlags = db.DB_INIT_TXN
    dbFlags = 0

    def txn_finish_test(self, sDB, txn):
        try:
            self.finish_test(sDB, txn=txn)
        finally:
            if self.cur:
                self.cur.close()
                self.cur = None
            if txn:
                txn.commit()

    def test13_associate_in_transaction(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test13_associateAutoCommit..." % \
                  self.__class__.__name__

        txn = self.env.txn_begin()
        try:
            self.createDB(txn=txn)

            self.secDB = db.DB(self.env)
            self.secDB.set_flags(db.DB_DUP)
            self.secDB.set_get_returns_none(2)
            self.secDB.open(self.filename, "secondary", db.DB_BTREE,
                       db.DB_CREATE | db.DB_THREAD, txn=txn)
            self.getDB().associate(self.secDB, self.getGenre, txn=txn)

            self.addDataToDB(self.getDB(), txn=txn)
        except:
            txn.abort()
            raise

        self.txn_finish_test(self.secDB, txn=txn)


#----------------------------------------------------------------------

class ShelveAssociateTestCase(AssociateTestCase):

    def createDB(self):
        self.primary = dbshelve.open(self.filename,
                                     dbname="primary",
                                     dbenv=self.env,
                                     filetype=self.dbtype)

    def addDataToDB(self, d):
        for key, value in musicdata.items():
            if type(self.keytype) == type(''):
                key = "%02d" % key
            d.put(key, value)    # save the value as is this time


    def getGenre(self, priKey, priData):
        self.assertEqual(type(priData), type(()))
        if verbose:
            print 'getGenre key: %r data: %r' % (priKey, priData)
        genre = priData[2]
        if genre == 'Blues':
            return db.DB_DONOTINDEX
        else:
            return genre


class ShelveAssociateHashTestCase(ShelveAssociateTestCase):
    dbtype = db.DB_HASH

class ShelveAssociateBTreeTestCase(ShelveAssociateTestCase):
    dbtype = db.DB_BTREE

class ShelveAssociateRecnoTestCase(ShelveAssociateTestCase):
    dbtype = db.DB_RECNO
    keytype = 0


#----------------------------------------------------------------------

class ThreadedAssociateTestCase(AssociateTestCase):

    def addDataToDB(self, d):
        t1 = Thread(target = self.writer1,
                    args = (d, ))
        t2 = Thread(target = self.writer2,
                    args = (d, ))

        t1.setDaemon(True)
        t2.setDaemon(True)
        t1.start()
        t2.start()
        t1.join()
        t2.join()

    def writer1(self, d):
        for key, value in musicdata.items():
            if type(self.keytype) == type(''):
                key = "%02d" % key
            d.put(key, '|'.join(value))

    def writer2(self, d):
        for x in range(100, 600):
            key = 'z%2d' % x
            value = [key] * 4
            d.put(key, '|'.join(value))


class ThreadedAssociateHashTestCase(ShelveAssociateTestCase):
    dbtype = db.DB_HASH

class ThreadedAssociateBTreeTestCase(ShelveAssociateTestCase):
    dbtype = db.DB_BTREE

class ThreadedAssociateRecnoTestCase(ShelveAssociateTestCase):
    dbtype = db.DB_RECNO
    keytype = 0


#----------------------------------------------------------------------

def test_suite():
    suite = unittest.TestSuite()

    suite.addTest(unittest.makeSuite(AssociateErrorTestCase))

    suite.addTest(unittest.makeSuite(AssociateHashTestCase))
    suite.addTest(unittest.makeSuite(AssociateBTreeTestCase))
    suite.addTest(unittest.makeSuite(AssociateRecnoTestCase))

    suite.addTest(unittest.makeSuite(AssociateBTreeTxnTestCase))

    suite.addTest(unittest.makeSuite(ShelveAssociateHashTestCase))
    suite.addTest(unittest.makeSuite(ShelveAssociateBTreeTestCase))
    suite.addTest(unittest.makeSuite(ShelveAssociateRecnoTestCase))

    if have_threads:
        suite.addTest(unittest.makeSuite(ThreadedAssociateHashTestCase))
        suite.addTest(unittest.makeSuite(ThreadedAssociateBTreeTestCase))
        suite.addTest(unittest.makeSuite(ThreadedAssociateRecnoTestCase))

    return suite


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
