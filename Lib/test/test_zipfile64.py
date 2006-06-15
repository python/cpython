# Tests of the full ZIP64 functionality of zipfile
# The test_support.requires call is the only reason for keeping this separate
# from test_zipfile
from test import test_support
test_support.requires(
        'largefile', 
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

from StringIO import StringIO
from tempfile import TemporaryFile

from test.test_support import TESTFN, run_unittest

TESTFN2 = TESTFN + "2"

# How much time in seconds can pass before we print a 'Still working' message.
_PRINT_WORKING_MSG_INTERVAL = 5 * 60

class TestsWithSourceFile(unittest.TestCase):
    def setUp(self):
        line_gen = ("Test of zipfile line %d." % i for i in range(0, 1000000))
        self.data = '\n'.join(line_gen)

        # Make a source file with some lines
        fp = open(TESTFN, "wb")
        fp.write(self.data)
        fp.close()

    def zipTest(self, f, compression):
        # Create the ZIP archive
        filecount = int(((1 << 32) / len(self.data)) * 1.5)
        zipfp = zipfile.ZipFile(f, "w", compression, allowZip64=True)

        next_time = time.time() + _PRINT_WORKING_MSG_INTERVAL
        for num in range(filecount):
            zipfp.writestr("testfn%d"%(num,), self.data)
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
            self.assertEqual(zipfp.read("testfn%d"%(num,)), self.data)
            # Print still working message since this test can be really slow
            if next_time <= time.time():
                next_time = time.time() + _PRINT_WORKING_MSG_INTERVAL
                print >>sys.__stdout__, (
                   '  zipTest still reading %d of %d, be patient...' %
                   (num, filecount))
                sys.__stdout__.flush()
        zipfp.close()

    def testStored(self):
        for f in (TESTFN2, TemporaryFile()):
            self.zipTest(f, zipfile.ZIP_STORED)

    if zlib:
        def testDeflated(self):
            for f in (TESTFN2, TemporaryFile()):
                self.zipTest(f, zipfile.ZIP_DEFLATED)

    def tearDown(self):
        os.remove(TESTFN)
        os.remove(TESTFN2)

def test_main():
    run_unittest(TestsWithSourceFile)

if __name__ == "__main__":
    test_main()
