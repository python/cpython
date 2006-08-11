# We can test part of the module without zlib.
try:
    import zlib
except ImportError:
    zlib = None

import zipfile, os, unittest, sys, shutil

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
        zipfp.writestr("strfile", self.data)
        zipfp.close()

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        self.assertEqual(zipfp.read(TESTFN), self.data)
        self.assertEqual(zipfp.read("another"+os.extsep+"name"), self.data)
        self.assertEqual(zipfp.read("strfile"), self.data)

        # Print the ZIP directory
        fp = StringIO()
        stdout = sys.stdout
        try:
            sys.stdout = fp

            zipfp.printdir()
        finally:
            sys.stdout = stdout

        directory = fp.getvalue()
        lines = directory.splitlines()
        self.assertEquals(len(lines), 4) # Number of files + header

        self.assert_('File Name' in lines[0])
        self.assert_('Modified' in lines[0])
        self.assert_('Size' in lines[0])

        fn, date, time, size = lines[1].split()
        self.assertEquals(fn, 'another.name')
        # XXX: timestamp is not tested
        self.assertEquals(size, str(len(self.data)))

        # Check the namelist
        names = zipfp.namelist()
        self.assertEquals(len(names), 3)
        self.assert_(TESTFN in names)
        self.assert_("another"+os.extsep+"name" in names)
        self.assert_("strfile" in names)

        # Check infolist
        infos = zipfp.infolist()
        names = [ i.filename for i in infos ]
        self.assertEquals(len(names), 3)
        self.assert_(TESTFN in names)
        self.assert_("another"+os.extsep+"name" in names)
        self.assert_("strfile" in names)
        for i in infos:
            self.assertEquals(i.file_size, len(self.data))

        # check getinfo
        for nm in (TESTFN, "another"+os.extsep+"name", "strfile"):
            info = zipfp.getinfo(nm)
            self.assertEquals(info.filename, nm)
            self.assertEquals(info.file_size, len(self.data))

        # Check that testzip doesn't raise an exception
        zipfp.testzip()


        zipfp.close()




    def testStored(self):
        for f in (TESTFN2, TemporaryFile(), StringIO()):
            self.zipTest(f, zipfile.ZIP_STORED)

    if zlib:
        def testDeflated(self):
            for f in (TESTFN2, TemporaryFile(), StringIO()):
                self.zipTest(f, zipfile.ZIP_DEFLATED)

    def testAbsoluteArcnames(self):
        zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED)
        zipfp.write(TESTFN, "/absolute")
        zipfp.close()

        zipfp = zipfile.ZipFile(TESTFN2, "r", zipfile.ZIP_STORED)
        self.assertEqual(zipfp.namelist(), ["absolute"])
        zipfp.close()


    def tearDown(self):
        os.remove(TESTFN)
        os.remove(TESTFN2)

class TestZip64InSmallFiles(unittest.TestCase):
    # These tests test the ZIP64 functionality without using large files,
    # see test_zipfile64 for proper tests.

    def setUp(self):
        self._limit = zipfile.ZIP64_LIMIT
        zipfile.ZIP64_LIMIT = 5

        line_gen = ("Test of zipfile line %d." % i for i in range(0, 1000))
        self.data = '\n'.join(line_gen)

        # Make a source file with some lines
        fp = open(TESTFN, "wb")
        fp.write(self.data)
        fp.close()

    def largeFileExceptionTest(self, f, compression):
        zipfp = zipfile.ZipFile(f, "w", compression)
        self.assertRaises(zipfile.LargeZipFile,
                zipfp.write, TESTFN, "another"+os.extsep+"name")
        zipfp.close()

    def largeFileExceptionTest2(self, f, compression):
        zipfp = zipfile.ZipFile(f, "w", compression)
        self.assertRaises(zipfile.LargeZipFile,
                zipfp.writestr, "another"+os.extsep+"name", self.data)
        zipfp.close()

    def testLargeFileException(self):
        for f in (TESTFN2, TemporaryFile(), StringIO()):
            self.largeFileExceptionTest(f, zipfile.ZIP_STORED)
            self.largeFileExceptionTest2(f, zipfile.ZIP_STORED)

    def zipTest(self, f, compression):
        # Create the ZIP archive
        zipfp = zipfile.ZipFile(f, "w", compression, allowZip64=True)
        zipfp.write(TESTFN, "another"+os.extsep+"name")
        zipfp.write(TESTFN, TESTFN)
        zipfp.writestr("strfile", self.data)
        zipfp.close()

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        self.assertEqual(zipfp.read(TESTFN), self.data)
        self.assertEqual(zipfp.read("another"+os.extsep+"name"), self.data)
        self.assertEqual(zipfp.read("strfile"), self.data)

        # Print the ZIP directory
        fp = StringIO()
        stdout = sys.stdout
        try:
            sys.stdout = fp

            zipfp.printdir()
        finally:
            sys.stdout = stdout

        directory = fp.getvalue()
        lines = directory.splitlines()
        self.assertEquals(len(lines), 4) # Number of files + header

        self.assert_('File Name' in lines[0])
        self.assert_('Modified' in lines[0])
        self.assert_('Size' in lines[0])

        fn, date, time, size = lines[1].split()
        self.assertEquals(fn, 'another.name')
        # XXX: timestamp is not tested
        self.assertEquals(size, str(len(self.data)))

        # Check the namelist
        names = zipfp.namelist()
        self.assertEquals(len(names), 3)
        self.assert_(TESTFN in names)
        self.assert_("another"+os.extsep+"name" in names)
        self.assert_("strfile" in names)

        # Check infolist
        infos = zipfp.infolist()
        names = [ i.filename for i in infos ]
        self.assertEquals(len(names), 3)
        self.assert_(TESTFN in names)
        self.assert_("another"+os.extsep+"name" in names)
        self.assert_("strfile" in names)
        for i in infos:
            self.assertEquals(i.file_size, len(self.data))

        # check getinfo
        for nm in (TESTFN, "another"+os.extsep+"name", "strfile"):
            info = zipfp.getinfo(nm)
            self.assertEquals(info.filename, nm)
            self.assertEquals(info.file_size, len(self.data))

        # Check that testzip doesn't raise an exception
        zipfp.testzip()


        zipfp.close()

    def testStored(self):
        for f in (TESTFN2, TemporaryFile(), StringIO()):
            self.zipTest(f, zipfile.ZIP_STORED)


    if zlib:
        def testDeflated(self):
            for f in (TESTFN2, TemporaryFile(), StringIO()):
                self.zipTest(f, zipfile.ZIP_DEFLATED)

    def testAbsoluteArcnames(self):
        zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED, allowZip64=True)
        zipfp.write(TESTFN, "/absolute")
        zipfp.close()

        zipfp = zipfile.ZipFile(TESTFN2, "r", zipfile.ZIP_STORED)
        self.assertEqual(zipfp.namelist(), ["absolute"])
        zipfp.close()


    def tearDown(self):
        zipfile.ZIP64_LIMIT = self._limit
        os.remove(TESTFN)
        os.remove(TESTFN2)

class PyZipFileTests(unittest.TestCase):
    def testWritePyfile(self):
        zipfp  = zipfile.PyZipFile(TemporaryFile(), "w")
        fn = __file__
        if fn.endswith('.pyc') or fn.endswith('.pyo'):
            fn = fn[:-1]

        zipfp.writepy(fn)

        bn = os.path.basename(fn)
        self.assert_(bn not in zipfp.namelist())
        self.assert_(bn + 'o' in zipfp.namelist() or bn + 'c' in zipfp.namelist())
        zipfp.close()


        zipfp  = zipfile.PyZipFile(TemporaryFile(), "w")
        fn = __file__
        if fn.endswith('.pyc') or fn.endswith('.pyo'):
            fn = fn[:-1]

        zipfp.writepy(fn, "testpackage")

        bn = "%s/%s"%("testpackage", os.path.basename(fn))
        self.assert_(bn not in zipfp.namelist())
        self.assert_(bn + 'o' in zipfp.namelist() or bn + 'c' in zipfp.namelist())
        zipfp.close()

    def testWritePythonPackage(self):
        import email
        packagedir = os.path.dirname(email.__file__)

        zipfp  = zipfile.PyZipFile(TemporaryFile(), "w")
        zipfp.writepy(packagedir)

        # Check for a couple of modules at different levels of the hieararchy
        names = zipfp.namelist()
        self.assert_('email/__init__.pyo' in names or 'email/__init__.pyc' in names)
        self.assert_('email/mime/text.pyo' in names or 'email/mime/text.pyc' in names)

    def testWritePythonDirectory(self):
        os.mkdir(TESTFN2)
        try:
            fp = open(os.path.join(TESTFN2, "mod1.py"), "w")
            fp.write("print 42\n")
            fp.close()

            fp = open(os.path.join(TESTFN2, "mod2.py"), "w")
            fp.write("print 42 * 42\n")
            fp.close()

            fp = open(os.path.join(TESTFN2, "mod2.txt"), "w")
            fp.write("bla bla bla\n")
            fp.close()

            zipfp  = zipfile.PyZipFile(TemporaryFile(), "w")
            zipfp.writepy(TESTFN2)

            names = zipfp.namelist()
            self.assert_('mod1.pyc' in names or 'mod1.pyo' in names)
            self.assert_('mod2.pyc' in names or 'mod2.pyo' in names)
            self.assert_('mod2.txt' not in names)

        finally:
            shutil.rmtree(TESTFN2)



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
    run_unittest(TestsWithSourceFile, TestZip64InSmallFiles, OtherTests, PyZipFileTests)
    #run_unittest(TestZip64InSmallFiles)

if __name__ == "__main__":
    test_main()
