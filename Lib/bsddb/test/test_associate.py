"""
TestCases for DB.associate.
"""

import sys, os, string
import tempfile
import time
from pprint import pprint

try:
    from threading import Thread, currentThread
    have_threads = 1
except ImportError:
    have_threads = 0

import unittest
from test_all import verbose

try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db, dbshelve
except ImportError:
    # For Python 2.3
    from bsddb import db, dbshelve


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


class AssociateTestCase(unittest.TestCase):
    keytype = ''

    def setUp(self):
        self.filename = self.__class__.__name__ + '.db'
        homeDir = os.path.join(os.path.dirname(sys.argv[0]), 'db_home')
        self.homeDir = homeDir
        try: os.mkdir(homeDir)
        except os.error: pass
        self.env = db.DBEnv()
        self.env.open(homeDir, db.DB_CREATE | db.DB_INIT_MPOOL |
                               db.DB_INIT_LOCK | db.DB_THREAD)

    def tearDown(self):
        self.closeDB()
        self.env.close()
        import glob
        files = glob.glob(os.path.join(self.homeDir, '*'))
        for file in files:
            os.remove(file)

    def addDataToDB(self, d):
        for key, value in musicdata.items():
            if type(self.keytype) == type(''):
                key = "%02d" % key
            d.put(key, string.join(value, '|'))

    def createDB(self):
        self.primary = db.DB(self.env)
        self.primary.set_get_returns_none(2)
        self.primary.open(self.filename, "primary", self.dbtype,
                          db.DB_CREATE | db.DB_THREAD)

    def closeDB(self):
        self.primary.close()

    def getDB(self):
        return self.primary

    def test01_associateWithDB(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test01_associateWithDB..." % \
                  self.__class__.__name__

        self.createDB()

        secDB = db.DB(self.env)
        secDB.set_flags(db.DB_DUP)
        secDB.set_get_returns_none(2)
        secDB.open(self.filename, "secondary", db.DB_BTREE,
                   db.DB_CREATE | db.DB_THREAD)
        self.getDB().associate(secDB, self.getGenre)

        self.addDataToDB(self.getDB())

        self.finish_test(secDB)


    def test02_associateAfterDB(self):
        if verbose:
            print '\n', '-=' * 30
            print "Running %s.test02_associateAfterDB..." % \
                  self.__class__.__name__

        self.createDB()
        self.addDataToDB(self.getDB())

        secDB = db.DB(self.env)
        secDB.set_flags(db.DB_DUP)
        secDB.open(self.filename, "secondary", db.DB_BTREE,
                   db.DB_CREATE | db.DB_THREAD)

        # adding the DB_CREATE flag will cause it to index existing records
        self.getDB().associate(secDB, self.getGenre, db.DB_CREATE)

        self.finish_test(secDB)


    def finish_test(self, secDB):
        # 'Blues' should not be in the secondary database
        vals = secDB.pget('Blues')
        assert vals == None, vals

        vals = secDB.pget('Unknown')
        assert vals[0] == 99 or vals[0] == '99', vals
        vals[1].index('Unknown')
        vals[1].index('Unnamed')
        vals[1].index('unknown')

        if verbose:
            print "Primary key traversal:"
        c = self.getDB().cursor()
        count = 0
        rec = c.first()
        while rec is not None:
            if type(self.keytype) == type(''):
                assert string.atoi(rec[0])  # for primary db, key is a number
            else:
                assert rec[0] and type(rec[0]) == type(0)
            count = count + 1
            if verbose:
                print rec
            rec = c.next()
        assert count == len(musicdata) # all items accounted for


        if verbose:
            print "Secondary key traversal:"
        c = secDB.cursor()
        count = 0

        # test cursor pget
        vals = c.pget('Unknown', flags=db.DB_LAST)
        assert vals[1] == 99 or vals[1] == '99', vals
        assert vals[0] == 'Unknown'
        vals[2].index('Unknown')
        vals[2].index('Unnamed')
        vals[2].index('unknown')

        vals = c.pget('Unknown', data='wrong value', flags=db.DB_GET_BOTH)
        assert vals == None, vals

        rec = c.first()
        assert rec[0] == "Jazz"
        while rec is not None:
            count = count + 1
            if verbose:
                print rec
            rec = c.next()
        # all items accounted for EXCEPT for 1 with "Blues" genre
        assert count == len(musicdata)-1

    def getGenre(self, priKey, priData):
        assert type(priData) == type("")
        if verbose:
            print 'getGenre key: %r data: %r' % (priKey, priData)
        genre = string.split(priData, '|')[2]
        if genre == 'Blues':
            return db.DB_DONOTINDEX
        else:
            return genre


#----------------------------------------------------------------------


class AssociateHashTestCase(AssociateTestCase):
    dbtype = db.DB_HASH

class AssociateBTreeTestCase(AssociateTestCase):
    dbtype = db.DB_BTREE

class AssociateRecnoTestCase(AssociateTestCase):
    dbtype = db.DB_RECNO
    keytype = 0


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
        assert type(priData) == type(())
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

        t1.start()
        t2.start()
        t1.join()
        t2.join()

    def writer1(self, d):
        for key, value in musicdata.items():
            if type(self.keytype) == type(''):
                key = "%02d" % key
            d.put(key, string.join(value, '|'))

    def writer2(self, d):
        for x in range(100, 600):
            key = 'z%2d' % x
            value = [key] * 4
            d.put(key, string.join(value, '|'))


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

    if db.version() >= (3, 3, 11):
        suite.addTest(unittest.makeSuite(AssociateHashTestCase))
        suite.addTest(unittest.makeSuite(AssociateBTreeTestCase))
        suite.addTest(unittest.makeSuite(AssociateRecnoTestCase))

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
