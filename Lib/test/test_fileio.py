# Adapted from test_file.py by Daniel Stutzbach
#from __future__ import unicode_literals

import sys
import os
import unittest
from array import array
from weakref import proxy

from test.test_support import (TESTFN, findfile, check_warnings, run_unittest,
                               make_bad_fd)
from UserList import UserList

import _fileio

class AutoFileTests(unittest.TestCase):
    # file tests for which a test file is automatically set up

    def setUp(self):
        self.f = _fileio._FileIO(TESTFN, 'w')

    def tearDown(self):
        if self.f:
            self.f.close()
        os.remove(TESTFN)

    def testWeakRefs(self):
        # verify weak references
        p = proxy(self.f)
        p.write(bytes(range(10)))
        self.assertEquals(self.f.tell(), p.tell())
        self.f.close()
        self.f = None
        self.assertRaises(ReferenceError, getattr, p, 'tell')

    def testSeekTell(self):
        self.f.write(bytes(bytearray(range(20))))
        self.assertEquals(self.f.tell(), 20)
        self.f.seek(0)
        self.assertEquals(self.f.tell(), 0)
        self.f.seek(10)
        self.assertEquals(self.f.tell(), 10)
        self.f.seek(5, 1)
        self.assertEquals(self.f.tell(), 15)
        self.f.seek(-5, 1)
        self.assertEquals(self.f.tell(), 10)
        self.f.seek(-5, 2)
        self.assertEquals(self.f.tell(), 15)

    def testAttributes(self):
        # verify expected attributes exist
        f = self.f

        self.assertEquals(f.mode, "wb")
        self.assertEquals(f.closed, False)

        # verify the attributes are readonly
        for attr in 'mode', 'closed':
            self.assertRaises((AttributeError, TypeError),
                              setattr, f, attr, 'oops')

    def testReadinto(self):
        # verify readinto
        self.f.write(bytes(bytearray([1, 2])))
        self.f.close()
        a = array('b', b'x'*10)
        self.f = _fileio._FileIO(TESTFN, 'r')
        n = self.f.readinto(a)
        self.assertEquals(array('b', [1, 2]), a[:n])

    def testRepr(self):
        self.assertEquals(repr(self.f),
                          "_fileio._FileIO(%d, %s)" % (self.f.fileno(),
                                                       repr(self.f.mode)))

    def testErrors(self):
        f = self.f
        self.assert_(not f.isatty())
        self.assert_(not f.closed)
        #self.assertEquals(f.name, TESTFN)
        self.assertRaises(ValueError, f.read, 10) # Open for reading
        f.close()
        self.assert_(f.closed)
        f = _fileio._FileIO(TESTFN, 'r')
        self.assertRaises(TypeError, f.readinto, "")
        self.assert_(not f.closed)
        f.close()
        self.assert_(f.closed)

    def testMethods(self):
        methods = ['fileno', 'isatty', 'read', 'readinto',
                   'seek', 'tell', 'truncate', 'write', 'seekable',
                   'readable', 'writable']
        if sys.platform.startswith('atheos'):
            methods.remove('truncate')

        self.f.close()
        self.assert_(self.f.closed)

        for methodname in methods:
            method = getattr(self.f, methodname)
            # should raise on closed file
            self.assertRaises(ValueError, method)

    def testOpendir(self):
        # Issue 3703: opening a directory should fill the errno
        # Windows always returns "[Errno 13]: Permission denied
        # Unix calls dircheck() and returns "[Errno 21]: Is a directory"
        try:
            _fileio._FileIO('.', 'r')
        except IOError as e:
            self.assertNotEqual(e.errno, 0)
            self.assertEqual(e.filename, ".")
        else:
            self.fail("Should have raised IOError")


class OtherFileTests(unittest.TestCase):

    def testAbles(self):
        try:
            f = _fileio._FileIO(TESTFN, "w")
            self.assertEquals(f.readable(), False)
            self.assertEquals(f.writable(), True)
            self.assertEquals(f.seekable(), True)
            f.close()

            f = _fileio._FileIO(TESTFN, "r")
            self.assertEquals(f.readable(), True)
            self.assertEquals(f.writable(), False)
            self.assertEquals(f.seekable(), True)
            f.close()

            f = _fileio._FileIO(TESTFN, "a+")
            self.assertEquals(f.readable(), True)
            self.assertEquals(f.writable(), True)
            self.assertEquals(f.seekable(), True)
            self.assertEquals(f.isatty(), False)
            f.close()

            if sys.platform != "win32":
                try:
                    f = _fileio._FileIO("/dev/tty", "a")
                except EnvironmentError:
                    # When run in a cron job there just aren't any
                    # ttys, so skip the test.  This also handles other
                    # OS'es that don't support /dev/tty.
                    pass
                else:
                    f = _fileio._FileIO("/dev/tty", "a")
                    self.assertEquals(f.readable(), False)
                    self.assertEquals(f.writable(), True)
                    if sys.platform != "darwin" and \
                       'bsd' not in sys.platform and \
                       not sys.platform.startswith('sunos'):
                        # Somehow /dev/tty appears seekable on some BSDs
                        self.assertEquals(f.seekable(), False)
                    self.assertEquals(f.isatty(), True)
                    f.close()
        finally:
            os.unlink(TESTFN)

    def testModeStrings(self):
        # check invalid mode strings
        for mode in ("", "aU", "wU+", "rw", "rt"):
            try:
                f = _fileio._FileIO(TESTFN, mode)
            except ValueError:
                pass
            else:
                f.close()
                self.fail('%r is an invalid file mode' % mode)

    def testUnicodeOpen(self):
        # verify repr works for unicode too
        f = _fileio._FileIO(str(TESTFN), "w")
        f.close()
        os.unlink(TESTFN)

    def testInvalidFd(self):
        self.assertRaises(ValueError, _fileio._FileIO, -10)
        self.assertRaises(OSError, _fileio._FileIO, make_bad_fd())

    def testBadModeArgument(self):
        # verify that we get a sensible error message for bad mode argument
        bad_mode = "qwerty"
        try:
            f = _fileio._FileIO(TESTFN, bad_mode)
        except ValueError as msg:
            if msg.args[0] != 0:
                s = str(msg)
                if TESTFN in s or bad_mode not in s:
                    self.fail("bad error message for invalid mode: %s" % s)
            # if msg.args[0] == 0, we're probably on Windows where there may be
            # no obvious way to discover why open() failed.
        else:
            f.close()
            self.fail("no error for invalid mode: %s" % bad_mode)

    def testTruncate(self):
        f = _fileio._FileIO(TESTFN, 'w')
        f.write(bytes(bytearray(range(10))))
        self.assertEqual(f.tell(), 10)
        f.truncate(5)
        self.assertEqual(f.tell(), 10)
        self.assertEqual(f.seek(0, os.SEEK_END), 5)
        f.truncate(15)
        self.assertEqual(f.tell(), 5)
        self.assertEqual(f.seek(0, os.SEEK_END), 15)

    def testTruncateOnWindows(self):
        def bug801631():
            # SF bug <http://www.python.org/sf/801631>
            # "file.truncate fault on windows"
            f = _fileio._FileIO(TESTFN, 'w')
            f.write(bytes(bytearray(range(11))))
            f.close()

            f = _fileio._FileIO(TESTFN,'r+')
            data = f.read(5)
            if data != bytes(bytearray(range(5))):
                self.fail("Read on file opened for update failed %r" % data)
            if f.tell() != 5:
                self.fail("File pos after read wrong %d" % f.tell())

            f.truncate()
            if f.tell() != 5:
                self.fail("File pos after ftruncate wrong %d" % f.tell())

            f.close()
            size = os.path.getsize(TESTFN)
            if size != 5:
                self.fail("File size after ftruncate wrong %d" % size)

        try:
            bug801631()
        finally:
            os.unlink(TESTFN)

    def testAppend(self):
        try:
            f = open(TESTFN, 'wb')
            f.write(b'spam')
            f.close()
            f = open(TESTFN, 'ab')
            f.write(b'eggs')
            f.close()
            f = open(TESTFN, 'rb')
            d = f.read()
            f.close()
            self.assertEqual(d, b'spameggs')
        finally:
            try:
                os.unlink(TESTFN)
            except:
                pass

    def testInvalidInit(self):
        self.assertRaises(TypeError, _fileio._FileIO, "1", 0, 0)

    def testWarnings(self):
        with check_warnings(quiet=True) as w:
            self.assertEqual(w.warnings, [])
            self.assertRaises(TypeError, _fileio._FileIO, [])
            self.assertEqual(w.warnings, [])
            self.assertRaises(ValueError, _fileio._FileIO, "/some/invalid/name", "rt")
            self.assertEqual(w.warnings, [])


def test_main():
    # Historically, these tests have been sloppy about removing TESTFN.
    # So get rid of it no matter what.
    try:
        run_unittest(AutoFileTests, OtherFileTests)
    finally:
        if os.path.exists(TESTFN):
            os.unlink(TESTFN)

if __name__ == '__main__':
    test_main()
