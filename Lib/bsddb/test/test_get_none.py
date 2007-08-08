"""
TestCases for checking set_get_returns_none.
"""

import sys, os, string
import tempfile
from pprint import pprint
import unittest

from bsddb import db

from .test_all import verbose


#----------------------------------------------------------------------

class GetReturnsNoneTestCase(unittest.TestCase):
    def setUp(self):
        self.filename = tempfile.mktemp()

    def tearDown(self):
        try:
            os.remove(self.filename)
        except os.error:
            pass


    def test01_get_returns_none(self):
        d = db.DB()
        d.open(self.filename, db.DB_BTREE, db.DB_CREATE)
        d.set_get_returns_none(1)

        for x in string.letters:
            x = x.encode("ascii")
            d.put(x, x * 40)

        data = d.get(b'bad key')
        assert data == None

        data = d.get(b'a')
        assert data == b'a'*40

        count = 0
        c = d.cursor()
        rec = c.first()
        while rec:
            count = count + 1
            rec = c.next()

        assert rec == None
        assert count == 52

        c.close()
        d.close()


    def test02_get_raises_exception(self):
        d = db.DB()
        d.open(self.filename, db.DB_BTREE, db.DB_CREATE)
        d.set_get_returns_none(0)

        for x in string.letters:
            x = x.encode("ascii")
            d.put(x, x * 40)

        self.assertRaises(db.DBNotFoundError, d.get, b'bad key')
        self.assertRaises(KeyError, d.get, b'bad key')

        data = d.get(b'a')
        assert data == b'a'*40

        count = 0
        exceptionHappened = 0
        c = d.cursor()
        rec = c.first()
        while rec:
            count = count + 1
            try:
                rec = c.next()
            except db.DBNotFoundError:  # end of the records
                exceptionHappened = 1
                break

        assert rec != None
        assert exceptionHappened
        assert count == 52

        c.close()
        d.close()

#----------------------------------------------------------------------

def test_suite():
    return unittest.makeSuite(GetReturnsNoneTestCase)


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
