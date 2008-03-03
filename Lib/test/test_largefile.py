"""Test largefile support on system where this makes sense.
"""

import os
import stat
import sys
import unittest
from test.test_support import run_unittest, TESTFN, verbose, requires, \
                              TestSkipped, unlink

try:
    import signal
    # The default handler for SIGXFSZ is to abort the process.
    # By ignoring it, system calls exceeding the file size resource
    # limit will raise IOError instead of crashing the interpreter.
    oldhandler = signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
except (ImportError, AttributeError):
    pass

# create >2GB file (2GB = 2147483648 bytes)
size = 2500000000


class TestCase(unittest.TestCase):
    """Test that each file function works as expected for a large
    (i.e. > 2GB, do  we have to check > 4GB) files.
    """

    def test_seek(self):
        if verbose:
            print('create large file via seek (may be sparse file) ...')
        f = open(TESTFN, 'wb')
        try:
            f.write(b'z')
            f.seek(0)
            f.seek(size)
            f.write(b'a')
            f.flush()
            if verbose:
                print('check file size with os.fstat')
            self.assertEqual(os.fstat(f.fileno())[stat.ST_SIZE], size+1)
        finally:
            f.close()

    def test_osstat(self):
        if verbose:
            print('check file size with os.stat')
        self.assertEqual(os.stat(TESTFN)[stat.ST_SIZE], size+1)

    def test_seek_read(self):
        if verbose:
            print('play around with seek() and read() with the built largefile')
        f = open(TESTFN, 'rb')
        try:
            self.assertEqual(f.tell(), 0)
            self.assertEqual(f.read(1), b'z')
            self.assertEqual(f.tell(), 1)
            f.seek(0)
            self.assertEqual(f.tell(), 0)
            f.seek(0, 0)
            self.assertEqual(f.tell(), 0)
            f.seek(42)
            self.assertEqual(f.tell(), 42)
            f.seek(42, 0)
            self.assertEqual(f.tell(), 42)
            f.seek(42, 1)
            self.assertEqual(f.tell(), 84)
            f.seek(0, 1)
            self.assertEqual(f.tell(), 84)
            f.seek(0, 2)  # seek from the end
            self.assertEqual(f.tell(), size + 1 + 0)
            f.seek(-10, 2)
            self.assertEqual(f.tell(), size + 1 - 10)
            f.seek(-size-1, 2)
            self.assertEqual(f.tell(), 0)
            f.seek(size)
            self.assertEqual(f.tell(), size)
            # the 'a' that was written at the end of file above
            self.assertEqual(f.read(1), b'a')
            f.seek(-size-1, 1)
            self.assertEqual(f.read(1), b'z')
            self.assertEqual(f.tell(), 1)
        finally:
            f.close()

    def test_lseek(self):
        if verbose:
            print('play around with os.lseek() with the built largefile')
        f = open(TESTFN, 'rb')
        try:
            self.assertEqual(os.lseek(f.fileno(), 0, 0), 0)
            self.assertEqual(os.lseek(f.fileno(), 42, 0), 42)
            self.assertEqual(os.lseek(f.fileno(), 42, 1), 84)
            self.assertEqual(os.lseek(f.fileno(), 0, 1), 84)
            self.assertEqual(os.lseek(f.fileno(), 0, 2), size+1+0)
            self.assertEqual(os.lseek(f.fileno(), -10, 2), size+1-10)
            self.assertEqual(os.lseek(f.fileno(), -size-1, 2), 0)
            self.assertEqual(os.lseek(f.fileno(), size, 0), size)
            # the 'a' that was written at the end of file above
            self.assertEqual(f.read(1), b'a')
        finally:
            f.close()

    def test_truncate(self):
        if verbose:
            print('try truncate')
        f = open(TESTFN, 'r+b')
        # this is already decided before start running the test suite
        # but we do it anyway for extra protection
        if not hasattr(f, 'truncate'):
            raise TestSkipped("open().truncate() not available on this system")
        try:
            f.seek(0, 2)
            # else we've lost track of the true size
            self.assertEqual(f.tell(), size+1)
            # Cut it back via seek + truncate with no argument.
            newsize = size - 10
            f.seek(newsize)
            f.truncate()
            self.assertEqual(f.tell(), newsize)  # else pointer moved
            f.seek(0, 2)
            self.assertEqual(f.tell(), newsize)  # else wasn't truncated
            # Ensure that truncate(smaller than true size) shrinks
            # the file.
            newsize -= 1
            f.seek(42)
            f.truncate(newsize)
            self.assertEqual(f.tell(), 42)       # else pointer moved
            f.seek(0, 2)
            self.assertEqual(f.tell(), newsize)  # else wasn't truncated
            # XXX truncate(larger than true size) is ill-defined
            # across platform; cut it waaaaay back
            f.seek(0)
            f.truncate(1)
            self.assertEqual(f.tell(), 0)       # else pointer moved
            self.assertEqual(len(f.read()), 1)  # else wasn't truncated
        finally:
            f.close()

def main_test():
    # On Windows and Mac OSX this test comsumes large resources; It
    # takes a long time to build the >2GB file and takes >2GB of disk
    # space therefore the resource must be enabled to run this test.
    # If not, nothing after this line stanza will be executed.
    if sys.platform[:3] == 'win' or sys.platform == 'darwin':
        requires('largefile',
                 'test requires %s bytes and a long time to run' % str(size))
    else:
        # Only run if the current filesystem supports large files.
        # (Skip this test on Windows, since we now always support
        # large files.)
        f = open(TESTFN, 'wb')
        try:
            # 2**31 == 2147483648
            f.seek(2147483649)
            # Seeking is not enough of a test: you must write and
            # flush, too!
            f.write(b'x')
            f.flush()
        except (IOError, OverflowError):
            f.close()
            unlink(TESTFN)
            raise TestSkipped("filesystem does not have largefile support")
        else:
            f.close()
    suite = unittest.TestSuite()
    suite.addTest(TestCase('test_seek'))
    suite.addTest(TestCase('test_osstat'))
    suite.addTest(TestCase('test_seek_read'))
    suite.addTest(TestCase('test_lseek'))
    f = open(TESTFN, 'w')
    if hasattr(f, 'truncate'):
        suite.addTest(TestCase('test_truncate'))
    f.close()
    unlink(TESTFN)
    run_unittest(suite)
    unlink(TESTFN)

if __name__ == '__main__':
    main_test()
