# Tests of the full ZIP64 functionality of zipfile
# The test_support.requires call is the only reason for keeping this separate
# from test_zipfile
from test import test_support
# XXX(nnorwitz): disable this test by looking for extra largfile resource
# which doesn't exist.  This test takes over 30 minutes to run in general
# and requires more disk space than most of the buildbots.
test_support.requires(
        'extralargefile',
        'test requires loads of disk-space bytes and a long time to run'
    )

# We can test part of the module without zlib.
try:
    import zlib
except ImportError:
    zlib = None

import zipfile, os, unittest
import time
import sys

from tempfile import TemporaryFile

from test.test_support import TESTFN, run_unittest

TESTFN2 = TESTFN + "2"

# How much time in seconds can pass before we print a 'Still working' message.
_PRINT_WORKING_MSG_INTERVAL = 5 * 60

class TestsWithSourceFile(unittest.TestCase):
    def setUp(self):
        # Create test data.
        # xrange() is important here -- don't want to create immortal space
        # for a million ints.
        line_gen = ("Test of zipfile line %d." % i for i in xrange(1000000))
        self.data = '\n'.join(line_gen)

        # And write it to a file.
        fp = open(TESTFN, "wb")
        fp.write(self.data)
        fp.close()

    def zipTest(self, f, compression):
        # Create the ZIP archive.
        zipfp = zipfile.ZipFile(f, "w", compression, allowZip64=True)

        # It will contain enough copies of self.data to reach about 6GB of
        # raw data to store.
        filecount = 6*1024**3 // len(self.data)

        next_time = time.time() + _PRINT_WORKING_MSG_INTERVAL
        for num in range(filecount):
            zipfp.writestr("testfn%d" % num, self.data)
            # Print still working message since this test can be really slow
            if next_time <= time.time():
                next_time = time.time() + _PRINT_WORKING_MSG_INTERVAL
                print >>sys.__stdout__, (
                   '  zipTest still writing %d of %d, be patient...' %
                   (num, filecount))
                sys.__stdout__.flush()
        zipfp.close()

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        for num in range(filecount):
            self.assertEqual(zipfp.read("testfn%d" % num), self.data)
            # Print still working message since this test can be really slow
            if next_time <= time.time():
                next_time = time.time() + _PRINT_WORKING_MSG_INTERVAL
                print >>sys.__stdout__, (
                   '  zipTest still reading %d of %d, be patient...' %
                   (num, filecount))
                sys.__stdout__.flush()
        zipfp.close()

    def testStored(self):
        # Try the temp file first.  If we do TESTFN2 first, then it hogs
        # gigabytes of disk space for the duration of the test.
        for f in TemporaryFile(), TESTFN2:
            self.zipTest(f, zipfile.ZIP_STORED)

    if zlib:
        def testDeflated(self):
            # Try the temp file first.  If we do TESTFN2 first, then it hogs
            # gigabytes of disk space for the duration of the test.
            for f in TemporaryFile(), TESTFN2:
                self.zipTest(f, zipfile.ZIP_DEFLATED)

    def tearDown(self):
        for fname in TESTFN, TESTFN2:
            if os.path.exists(fname):
                os.remove(fname)

def test_main():
    run_unittest(TestsWithSourceFile)

if __name__ == "__main__":
    test_main()
