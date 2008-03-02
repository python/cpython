"""TestCases for exercising a Recno DB.
"""

import os
import shutil
import sys
import errno
import tempfile
from pprint import pprint
import unittest

from bsddb.test.test_all import verbose

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

letters = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'


#----------------------------------------------------------------------

class SimpleRecnoTestCase(unittest.TestCase):
    def setUp(self):
        self.filename = tempfile.mktemp()
        self.homeDir = None

    def tearDown(self):
        test_support.unlink(self.filename)
        if self.homeDir:
            test_support.rmtree(self.homeDir)

    def test01_basic(self):
        d = db.DB()

        get_returns_none = d.set_get_returns_none(2)
        d.set_get_returns_none(get_returns_none)

        d.open(self.filename, db.DB_RECNO, db.DB_CREATE)

        for x in letters:
            recno = d.append(x.encode('ascii') * 60)
            assert type(recno) == type(0)
            assert recno >= 1
            if verbose:
                print(recno, end=' ')

        if verbose: print()

        stat = d.stat()
        if verbose:
            pprint(stat)

        for recno in range(1, len(d)+1):
            data = d[recno]
            if verbose:
                print(data)

            assert type(data) == bytes
            assert data == d.get(recno)

        try:
            data = d[0]  # This should raise a KeyError!?!?!
        except db.DBInvalidArgError as val:
            assert val.args[0] == db.EINVAL
            if verbose: print(val)
        else:
            self.fail("expected exception")

        # test that has_key raises DB exceptions (fixed in pybsddb 4.3.2)
        try:
            d.has_key(0)
        except db.DBError as val:
            pass
        else:
            self.fail("has_key did not raise a proper exception")

        try:
            data = d[100]
        except KeyError:
            pass
        else:
            self.fail("expected exception")

        try:
            data = d.get(100)
        except db.DBNotFoundError as val:
            if get_returns_none:
                self.fail("unexpected exception")
        else:
            assert data == None

        keys = d.keys()
        if verbose:
            print(keys)
        assert type(keys) == type([])
        assert type(keys[0]) == type(123)
        assert len(keys) == len(d)

        items = d.items()
        if verbose:
            pprint(items)
        assert type(items) == type([])
        assert type(items[0]) == type(())
        assert len(items[0]) == 2
        assert type(items[0][0]) == type(123)
        assert type(items[0][1]) == bytes
        assert len(items) == len(d)

        assert d.has_key(25)

        del d[25]
        assert not d.has_key(25)

        d.delete(13)
        assert not d.has_key(13)

        data = d.get_both(26, b"z" * 60)
        assert data == b"z" * 60, 'was %r' % data
        if verbose:
            print(data)

        fd = d.fd()
        if verbose:
            print(fd)

        c = d.cursor()
        rec = c.first()
        while rec:
            if verbose:
                print(rec)
            rec = c.next()

        c.set(50)
        rec = c.current()
        if verbose:
            print(rec)

        c.put(-1, b"a replacement record", db.DB_CURRENT)

        c.set(50)
        rec = c.current()
        assert rec == (50, b"a replacement record")
        if verbose:
            print(rec)

        rec = c.set_range(30)
        if verbose:
            print(rec)

        # test that non-existant key lookups work (and that
        # DBC_set_range doesn't have a memleak under valgrind)
        rec = c.set_range(999999)
        assert rec == None
        if verbose:
            print(rec)

        c.close()
        d.close()

        d = db.DB()
        d.open(self.filename)
        c = d.cursor()

        # put a record beyond the consecutive end of the recno's
        d[100] = b"way out there"
        assert d[100] == b"way out there"

        try:
            data = d[99]
        except KeyError:
            pass
        else:
            self.fail("expected exception")

        try:
            d.get(99)
        except db.DBKeyEmptyError as val:
            if get_returns_none:
                self.fail("unexpected DBKeyEmptyError exception")
            else:
                assert val.args[0] == db.DB_KEYEMPTY
                if verbose: print(val)
        else:
            if not get_returns_none:
                self.fail("expected exception")

        rec = c.set(40)
        while rec:
            if verbose:
                print(rec)
            rec = c.next()

        c.close()
        d.close()

    def test02_WithSource(self):
        """
        A Recno file that is given a "backing source file" is essentially a
        simple ASCII file.  Normally each record is delimited by \n and so is
        just a line in the file, but you can set a different record delimiter
        if needed.
        """
        homeDir = os.path.join(tempfile.gettempdir(), 'db_home%d'%os.getpid())
        self.homeDir = homeDir
        source = os.path.join(homeDir, 'test_recno.txt')
        if not os.path.isdir(homeDir):
            os.mkdir(homeDir)
        f = open(source, 'w') # create the file
        f.close()

        d = db.DB()
        # This is the default value, just checking if both int
        d.set_re_delim(0x0A)
        d.set_re_delim('\n')  # and char can be used...
        d.set_re_source(source)
        d.open(self.filename, db.DB_RECNO, db.DB_CREATE)

        data = "The quick brown fox jumped over the lazy dog".split()
        for datum in data:
            d.append(datum.encode('ascii'))
        d.sync()
        d.close()

        # get the text from the backing source
        text = open(source, 'r').read()
        text = text.strip()
        if verbose:
            print(text)
            print(data)
            print(text.split('\n'))

        assert text.split('\n') == data

        # open as a DB again
        d = db.DB()
        d.set_re_source(source)
        d.open(self.filename, db.DB_RECNO)

        d[3] = b'reddish-brown'
        d[8] = b'comatose'

        d.sync()
        d.close()

        text = open(source, 'r').read()
        text = text.strip()
        if verbose:
            print(text)
            print(text.split('\n'))

        assert text.split('\n') == \
             "The quick reddish-brown fox jumped over the comatose dog".split()

    def test03_FixedLength(self):
        d = db.DB()
        d.set_re_len(40)  # fixed length records, 40 bytes long
        d.set_re_pad('-') # sets the pad character...
        d.set_re_pad(45)  # ...test both int and char
        d.open(self.filename, db.DB_RECNO, db.DB_CREATE)

        for x in letters:
            d.append(x.encode('ascii') * 35)    # These will be padded

        d.append(b'.' * 40)      # this one will be exact

        try:                    # this one will fail
            d.append(b'bad' * 20)
        except db.DBInvalidArgError as val:
            assert val.args[0] == db.EINVAL
            if verbose: print(val)
        else:
            self.fail("expected exception")

        c = d.cursor()
        rec = c.first()
        while rec:
            if verbose:
                print(rec)
            rec = c.next()

        c.close()
        d.close()


#----------------------------------------------------------------------


def test_suite():
    return unittest.makeSuite(SimpleRecnoTestCase)


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
