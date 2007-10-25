import sys
import os
import unittest
from array import array
from weakref import proxy

from test.test_support import TESTFN, findfile, run_unittest
from UserList import UserList

class AutoFileTests(unittest.TestCase):
    # file tests for which a test file is automatically set up

    def setUp(self):
        self.f = open(TESTFN, 'wb')

    def tearDown(self):
        if self.f:
            self.f.close()
        os.remove(TESTFN)

    def testWeakRefs(self):
        # verify weak references
        p = proxy(self.f)
        p.write(b'teststring')
        self.assertEquals(self.f.tell(), p.tell())
        self.f.close()
        self.f = None
        self.assertRaises(ReferenceError, getattr, p, 'tell')

    def testAttributes(self):
        # verify expected attributes exist
        f = self.f
        f.name     # merely shouldn't blow up
        f.mode     # ditto
        f.closed   # ditto

    def testReadinto(self):
        # verify readinto
        self.f.write(b'12')
        self.f.close()
        a = array('b', b'x'*10)
        self.f = open(TESTFN, 'rb')
        n = self.f.readinto(a)
        self.assertEquals(b'12', a.tostring()[:n])

    def testReadinto_text(self):
        # verify readinto refuses text files
        a = array('b', b'x'*10)
        self.f.close()
        self.f = open(TESTFN, 'r')
        if hasattr(self.f, "readinto"):
            self.assertRaises(TypeError, self.f.readinto, a)

    def testWritelinesUserList(self):
        # verify writelines with instance sequence
        l = UserList([b'1', b'2'])
        self.f.writelines(l)
        self.f.close()
        self.f = open(TESTFN, 'rb')
        buf = self.f.read()
        self.assertEquals(buf, b'12')

    def testWritelinesIntegers(self):
        # verify writelines with integers
        self.assertRaises(TypeError, self.f.writelines, [1, 2, 3])

    def testWritelinesIntegersUserList(self):
        # verify writelines with integers in UserList
        l = UserList([1,2,3])
        self.assertRaises(TypeError, self.f.writelines, l)

    def testWritelinesNonString(self):
        # verify writelines with non-string object
        class NonString:
            pass

        self.assertRaises(TypeError, self.f.writelines,
                          [NonString(), NonString()])

    def testErrors(self):
        f = self.f
        self.assertEquals(f.name, TESTFN)
        self.assert_(not f.isatty())
        self.assert_(not f.closed)

        if hasattr(f, "readinto"):
            self.assertRaises((IOError, TypeError), f.readinto, "")
        f.close()
        self.assert_(f.closed)

    def testMethods(self):
        methods = [('fileno', ()),
                   ('flush', ()),
                   ('isatty', ()),
                   ('__next__', ()),
                   ('read', ()),
                   ('write', (b"",)),
                   ('readline', ()),
                   ('readlines', ()),
                   ('seek', (0,)),
                   ('tell', ()),
                   ('write', (b"",)),
                   ('writelines', ([],)),
                   ('__iter__', ()),
                   ]
        if not sys.platform.startswith('atheos'):
            methods.append(('truncate', ()))

        # __exit__ should close the file
        self.f.__exit__(None, None, None)
        self.assert_(self.f.closed)

        for methodname, args in methods:
            method = getattr(self.f, methodname)
            # should raise on closed file
            self.assertRaises(ValueError, method, *args)

        # file is closed, __exit__ shouldn't do anything
        self.assertEquals(self.f.__exit__(None, None, None), None)
        # it must also return None if an exception was given
        try:
            1/0
        except:
            self.assertEquals(self.f.__exit__(*sys.exc_info()), None)


class OtherFileTests(unittest.TestCase):

    def testModeStrings(self):
        # check invalid mode strings
        for mode in ("", "aU", "wU+"):
            try:
                f = open(TESTFN, mode)
            except ValueError:
                pass
            else:
                f.close()
                self.fail('%r is an invalid file mode' % mode)

    def testStdin(self):
        # This causes the interpreter to exit on OSF1 v5.1.
        if sys.platform != 'osf1V5':
            self.assertRaises((IOError, ValueError), sys.stdin.seek, -1)
        else:
            print((
                '  Skipping sys.stdin.seek(-1), it may crash the interpreter.'
                ' Test manually.'), file=sys.__stdout__)
        self.assertRaises((IOError, ValueError), sys.stdin.truncate)

    def testBadModeArgument(self):
        # verify that we get a sensible error message for bad mode argument
        bad_mode = "qwerty"
        try:
            f = open(TESTFN, bad_mode)
        except ValueError as msg:
            if msg.args[0] != 0:
                s = str(msg)
                if s.find(TESTFN) != -1 or s.find(bad_mode) == -1:
                    self.fail("bad error message for invalid mode: %s" % s)
            # if msg[0] == 0, we're probably on Windows where there may be
            # no obvious way to discover why open() failed.
        else:
            f.close()
            self.fail("no error for invalid mode: %s" % bad_mode)

    def testSetBufferSize(self):
        # make sure that explicitly setting the buffer size doesn't cause
        # misbehaviour especially with repeated close() calls
        for s in (-1, 0, 1, 512):
            try:
                f = open(TESTFN, 'wb', s)
                f.write(str(s).encode("ascii"))
                f.close()
                f.close()
                f = open(TESTFN, 'rb', s)
                d = int(f.read().decode("ascii"))
                f.close()
                f.close()
            except IOError as msg:
                self.fail('error setting buffer size %d: %s' % (s, str(msg)))
            self.assertEquals(d, s)

    def testTruncateOnWindows(self):
        # SF bug <http://www.python.org/sf/801631>
        # "file.truncate fault on windows"

        os.unlink(TESTFN)
        f = open(TESTFN, 'wb')

        try:
            f.write(b'12345678901')   # 11 bytes
            f.close()

            f = open(TESTFN,'rb+')
            data = f.read(5)
            if data != b'12345':
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
        finally:
            f.close()
            os.unlink(TESTFN)

    def testIteration(self):
        # Test the complex interaction when mixing file-iteration and the
        # various read* methods.
        dataoffset = 16384
        filler = b"ham\n"
        assert not dataoffset % len(filler), \
            "dataoffset must be multiple of len(filler)"
        nchunks = dataoffset // len(filler)
        testlines = [
            b"spam, spam and eggs\n",
            b"eggs, spam, ham and spam\n",
            b"saussages, spam, spam and eggs\n",
            b"spam, ham, spam and eggs\n",
            b"spam, spam, spam, spam, spam, ham, spam\n",
            b"wonderful spaaaaaam.\n"
        ]
        methods = [("readline", ()), ("read", ()), ("readlines", ()),
                   ("readinto", (array("b", b" "*100),))]

        try:
            # Prepare the testfile
            bag = open(TESTFN, "wb")
            bag.write(filler * nchunks)
            bag.writelines(testlines)
            bag.close()
            # Test for appropriate errors mixing read* and iteration
            for methodname, args in methods:
                f = open(TESTFN, 'rb')
                if next(f) != filler:
                    self.fail, "Broken testfile"
                meth = getattr(f, methodname)
                meth(*args)  # This simply shouldn't fail
                f.close()

            # Test to see if harmless (by accident) mixing of read* and
            # iteration still works. This depends on the size of the internal
            # iteration buffer (currently 8192,) but we can test it in a
            # flexible manner.  Each line in the bag o' ham is 4 bytes
            # ("h", "a", "m", "\n"), so 4096 lines of that should get us
            # exactly on the buffer boundary for any power-of-2 buffersize
            # between 4 and 16384 (inclusive).
            f = open(TESTFN, 'rb')
            for i in range(nchunks):
                next(f)
            testline = testlines.pop(0)
            try:
                line = f.readline()
            except ValueError:
                self.fail("readline() after next() with supposedly empty "
                          "iteration-buffer failed anyway")
            if line != testline:
                self.fail("readline() after next() with empty buffer "
                          "failed. Got %r, expected %r" % (line, testline))
            testline = testlines.pop(0)
            buf = array("b", b"\x00" * len(testline))
            try:
                f.readinto(buf)
            except ValueError:
                self.fail("readinto() after next() with supposedly empty "
                          "iteration-buffer failed anyway")
            line = buf.tostring()
            if line != testline:
                self.fail("readinto() after next() with empty buffer "
                          "failed. Got %r, expected %r" % (line, testline))

            testline = testlines.pop(0)
            try:
                line = f.read(len(testline))
            except ValueError:
                self.fail("read() after next() with supposedly empty "
                          "iteration-buffer failed anyway")
            if line != testline:
                self.fail("read() after next() with empty buffer "
                          "failed. Got %r, expected %r" % (line, testline))
            try:
                lines = f.readlines()
            except ValueError:
                self.fail("readlines() after next() with supposedly empty "
                          "iteration-buffer failed anyway")
            if lines != testlines:
                self.fail("readlines() after next() with empty buffer "
                          "failed. Got %r, expected %r" % (line, testline))
            # Reading after iteration hit EOF shouldn't hurt either
            f = open(TESTFN, 'rb')
            try:
                for line in f:
                    pass
                try:
                    f.readline()
                    f.readinto(buf)
                    f.read()
                    f.readlines()
                except ValueError:
                    self.fail("read* failed after next() consumed file")
            finally:
                f.close()
        finally:
            os.unlink(TESTFN)


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
