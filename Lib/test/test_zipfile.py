# We can test part of the module without zlib.
try:
    import zlib
except ImportError:
    zlib = None

import zipfile, os, unittest

from StringIO import StringIO
from tempfile import TemporaryFile

from test.test_support import TESTFN, run_unittest

TESTFN2 = TESTFN + "2"

class TestsWithSourceFile(unittest.TestCase):
    def setUp(self):
        line_gen = ("Test of zipfile line %d." % i for i in range(0, 1000))
        self.data = '\n'.join(line_gen)

        # Make a source file with some lines
        fp = open(TESTFN, "wb")
        fp.write(self.data)
        fp.close()

    def zipTest(self, f, compression):
        # Create the ZIP archive
        zipfp = zipfile.ZipFile(f, "w", compression)
        zipfp.write(TESTFN, "another"+os.extsep+"name")
        zipfp.write(TESTFN, TESTFN)
        zipfp.close()

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        self.assertEqual(zipfp.read(TESTFN), self.data)
        self.assertEqual(zipfp.read("another"+os.extsep+"name"), self.data)
        zipfp.close()

    def testStored(self):
        for f in (TESTFN2, TemporaryFile(), StringIO()):
            self.zipTest(f, zipfile.ZIP_STORED)

    if zlib:
        def testDeflated(self):
            for f in (TESTFN2, TemporaryFile(), StringIO()):
                self.zipTest(f, zipfile.ZIP_DEFLATED)

    def tearDown(self):
        os.remove(TESTFN)
        os.remove(TESTFN2)

class OtherTests(unittest.TestCase):
    def testCloseErroneousFile(self):
        # This test checks that the ZipFile constructor closes the file object
        # it opens if there's an error in the file.  If it doesn't, the traceback
        # holds a reference to the ZipFile object and, indirectly, the file object.
        # On Windows, this causes the os.unlink() call to fail because the
        # underlying file is still open.  This is SF bug #412214.
        #
        fp = open(TESTFN, "w")
        fp.write("this is not a legal zip file\n")
        fp.close()
        try:
            zf = zipfile.ZipFile(TESTFN)
        except zipfile.BadZipfile:
            os.unlink(TESTFN)

    def testNonExistentFileRaisesIOError(self):
        # make sure we don't raise an AttributeError when a partially-constructed
        # ZipFile instance is finalized; this tests for regression on SF tracker
        # bug #403871.

        # The bug we're testing for caused an AttributeError to be raised
        # when a ZipFile instance was created for a file that did not
        # exist; the .fp member was not initialized but was needed by the
        # __del__() method.  Since the AttributeError is in the __del__(),
        # it is ignored, but the user should be sufficiently annoyed by
        # the message on the output that regression will be noticed
        # quickly.
        self.assertRaises(IOError, zipfile.ZipFile, TESTFN)

    def testClosedZipRaisesRuntimeError(self):
        # Verify that testzip() doesn't swallow inappropriate exceptions.
        data = StringIO()
        zipf = zipfile.ZipFile(data, mode="w")
        zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        zipf.close()

        # This is correct; calling .read on a closed ZipFile should throw
        # a RuntimeError, and so should calling .testzip.  An earlier
        # version of .testzip would swallow this exception (and any other)
        # and report that the first file in the archive was corrupt.
        self.assertRaises(RuntimeError, zipf.testzip)

def test_main():
    run_unittest(TestsWithSourceFile, OtherTests)

if __name__ == "__main__":
    test_main()
