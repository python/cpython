"""
Test cases adapted from the test_bsddb.py module in Python's
regression test suite.
"""

import sys, os, string
import unittest
import tempfile

from test_all import verbose

try:
    # For Python 2.3
    from bsddb import db, hashopen, btopen, rnopen
except ImportError:
    # For earlier Pythons w/distutils pybsddb
    from bsddb3 import db, hashopen, btopen, rnopen


class CompatibilityTestCase(unittest.TestCase):
    def setUp(self):
        self.filename = tempfile.mktemp()

    def tearDown(self):
        try:
            os.remove(self.filename)
        except os.error:
            pass


    def test01_btopen(self):
        self.do_bthash_test(btopen, 'btopen')

    def test02_hashopen(self):
        self.do_bthash_test(hashopen, 'hashopen')

    def test03_rnopen(self):
        data = string.split("The quick brown fox jumped over the lazy dog.")
        if verbose:
            print "\nTesting: rnopen"

        f = rnopen(self.filename, 'c')
        for x in range(len(data)):
            f[x+1] = data[x]

        getTest = (f[1], f[2], f[3])
        if verbose:
            print '%s %s %s' % getTest

        assert getTest[1] == 'quick', 'data mismatch!'

        f[25] = 'twenty-five'
        f.close()
        del f

        f = rnopen(self.filename, 'w')
        f[20] = 'twenty'

        def noRec(f):
            rec = f[15]
        self.assertRaises(KeyError, noRec, f)

        def badKey(f):
            rec = f['a string']
        self.assertRaises(TypeError, badKey, f)

        del f[3]

        rec = f.first()
        while rec:
            if verbose:
                print rec
            try:
                rec = f.next()
            except KeyError:
                break

        f.close()


    def test04_n_flag(self):
        f = hashopen(self.filename, 'n')
        f.close()



    def do_bthash_test(self, factory, what):
        if verbose:
            print '\nTesting: ', what

        f = factory(self.filename, 'c')
        if verbose:
            print 'creation...'

        # truth test
        if f:
            if verbose: print "truth test: true"
        else:
            if verbose: print "truth test: false"

        f['0'] = ''
        f['a'] = 'Guido'
        f['b'] = 'van'
        f['c'] = 'Rossum'
        f['d'] = 'invented'
        f['f'] = 'Python'
        if verbose:
            print '%s %s %s' % (f['a'], f['b'], f['c'])

        if verbose:
            print 'key ordering...'
        f.set_location(f.first()[0])
        while 1:
            try:
                rec = f.next()
            except KeyError:
                assert rec == f.last(), 'Error, last <> last!'
                f.previous()
                break
            if verbose:
                print rec

        assert f.has_key('f'), 'Error, missing key!'

        f.sync()
        f.close()
        # truth test
        try:
            if f:
                if verbose: print "truth test: true"
            else:
                if verbose: print "truth test: false"
        except db.DBError:
            pass
        else:
            self.fail("Exception expected")

        del f

        if verbose:
            print 'modification...'
        f = factory(self.filename, 'w')
        f['d'] = 'discovered'

        if verbose:
            print 'access...'
        for key in f.keys():
            word = f[key]
            if verbose:
                print word

        def noRec(f):
            rec = f['no such key']
        self.assertRaises(KeyError, noRec, f)

        def badKey(f):
            rec = f[15]
        self.assertRaises(TypeError, badKey, f)

        f.close()


#----------------------------------------------------------------------


def test_suite():
    return unittest.makeSuite(CompatibilityTestCase)


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
