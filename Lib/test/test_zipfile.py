# We can test part of the module without zlib.
try:
    import zlib
except ImportError:
    zlib = None
import zipfile, os, unittest, sys, shutil, struct, io

from tempfile import TemporaryFile
from random import randint, random

import test.support as support
from test.support import TESTFN, run_unittest, findfile

TESTFN2 = TESTFN + "2"
TESTFNDIR = TESTFN + "d"
FIXEDTEST_SIZE = 1000

SMALL_TEST_DATA = [('_ziptest1', '1q2w3e4r5t'),
                   ('ziptest2dir/_ziptest2', 'qawsedrftg'),
                   ('/ziptest2dir/ziptest3dir/_ziptest3', 'azsxdcfvgb'),
                   ('ziptest2dir/ziptest3dir/ziptest4dir/_ziptest3', '6y7u8i9o0p')]

class TestsWithSourceFile(unittest.TestCase):
    def setUp(self):
        self.line_gen = (bytes("Zipfile test line %d. random float: %f" %
                               (i, random()), "ascii")
                         for i in range(FIXEDTEST_SIZE))
        self.data = b'\n'.join(self.line_gen) + b'\n'

        # Make a source file with some lines
        fp = open(TESTFN, "wb")
        fp.write(self.data)
        fp.close()

    def makeTestArchive(self, f, compression):
        # Create the ZIP archive
        zipfp = zipfile.ZipFile(f, "w", compression)
        zipfp.write(TESTFN, "another.name")
        zipfp.write(TESTFN, TESTFN)
        zipfp.writestr("strfile", self.data)
        zipfp.close()

    def zipTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        self.assertEqual(zipfp.read(TESTFN), self.data)
        self.assertEqual(zipfp.read("another.name"), self.data)
        self.assertEqual(zipfp.read("strfile"), self.data)

        # Print the ZIP directory
        fp = io.StringIO()
        zipfp.printdir(file=fp)

        directory = fp.getvalue()
        lines = directory.splitlines()
        self.assertEqual(len(lines), 4) # Number of files + header

        self.assertTrue('File Name' in lines[0])
        self.assertTrue('Modified' in lines[0])
        self.assertTrue('Size' in lines[0])

        fn, date, time, size = lines[1].split()
        self.assertEqual(fn, 'another.name')
        # XXX: timestamp is not tested
        self.assertEqual(size, str(len(self.data)))

        # Check the namelist
        names = zipfp.namelist()
        self.assertEqual(len(names), 3)
        self.assertTrue(TESTFN in names)
        self.assertTrue("another.name" in names)
        self.assertTrue("strfile" in names)

        # Check infolist
        infos = zipfp.infolist()
        names = [ i.filename for i in infos ]
        self.assertEqual(len(names), 3)
        self.assertTrue(TESTFN in names)
        self.assertTrue("another.name" in names)
        self.assertTrue("strfile" in names)
        for i in infos:
            self.assertEqual(i.file_size, len(self.data))

        # check getinfo
        for nm in (TESTFN, "another.name", "strfile"):
            info = zipfp.getinfo(nm)
            self.assertEqual(info.filename, nm)
            self.assertEqual(info.file_size, len(self.data))

        # Check that testzip doesn't raise an exception
        zipfp.testzip()
        zipfp.close()

    def testStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipTest(f, zipfile.ZIP_STORED)

    def zipOpenTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        zipdata1 = []
        zipopen1 = zipfp.open(TESTFN)
        while 1:
            read_data = zipopen1.read(256)
            if not read_data:
                break
            zipdata1.append(read_data)

        zipdata2 = []
        zipopen2 = zipfp.open("another.name")
        while 1:
            read_data = zipopen2.read(256)
            if not read_data:
                break
            zipdata2.append(read_data)

        self.assertEqual(b''.join(zipdata1), self.data)
        self.assertEqual(b''.join(zipdata2), self.data)
        zipfp.close()

    def testOpenStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipOpenTest(f, zipfile.ZIP_STORED)

    def testOpenViaZipInfo(self):
        # Create the ZIP archive
        zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED)
        zipfp.writestr("name", "foo")
        zipfp.writestr("name", "bar")
        zipfp.close()

        zipfp = zipfile.ZipFile(TESTFN2, "r")
        infos = zipfp.infolist()
        data = b""
        for info in infos:
            data += zipfp.open(info).read()
        self.assertTrue(data == b"foobar" or data == b"barfoo")
        data = b""
        for info in infos:
            data += zipfp.read(info)
        self.assertTrue(data == b"foobar" or data == b"barfoo")
        zipfp.close()

    def zipRandomOpenTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        zipdata1 = []
        zipopen1 = zipfp.open(TESTFN)
        while 1:
            read_data = zipopen1.read(randint(1, 1024))
            if not read_data:
                break
            zipdata1.append(read_data)

        self.assertEqual(b''.join(zipdata1), self.data)
        zipfp.close()

    def testRandomOpenStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipRandomOpenTest(f, zipfile.ZIP_STORED)

    def zipReadlineTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r")
        zipopen = zipfp.open(TESTFN)
        for line in self.line_gen:
            linedata = zipopen.readline()
            self.assertEqual(linedata, line + '\n')

        zipfp.close()

    def zipReadlinesTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r")
        ziplines = zipfp.open(TESTFN).readlines()
        for line, zipline in zip(self.line_gen, ziplines):
            self.assertEqual(zipline, line + '\n')

        zipfp.close()

    def zipIterlinesTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r")
        for line, zipline in zip(self.line_gen, zipfp.open(TESTFN)):
            self.assertEqual(zipline, line + '\n')

        zipfp.close()

    def testReadlineStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipReadlineTest(f, zipfile.ZIP_STORED)

    def testReadlinesStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipReadlinesTest(f, zipfile.ZIP_STORED)

    def testIterlinesStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipIterlinesTest(f, zipfile.ZIP_STORED)

    if zlib:
        def testDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.zipTest(f, zipfile.ZIP_DEFLATED)

        def testOpenDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.zipOpenTest(f, zipfile.ZIP_DEFLATED)

        def testRandomOpenDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.zipRandomOpenTest(f, zipfile.ZIP_DEFLATED)

        def testReadlineDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.zipReadlineTest(f, zipfile.ZIP_DEFLATED)

        def testReadlinesDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.zipReadlinesTest(f, zipfile.ZIP_DEFLATED)

        def testIterlinesDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.zipIterlinesTest(f, zipfile.ZIP_DEFLATED)

        def testLowCompression(self):
            # Checks for cases where compressed data is larger than original
            # Create the ZIP archive
            zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_DEFLATED)
            zipfp.writestr("strfile", '12')
            zipfp.close()

            # Get an open object for strfile
            zipfp = zipfile.ZipFile(TESTFN2, "r", zipfile.ZIP_DEFLATED)
            openobj = zipfp.open("strfile")
            self.assertEqual(openobj.read(1), b'1')
            self.assertEqual(openobj.read(1), b'2')

    def testAbsoluteArcnames(self):
        zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED)
        zipfp.write(TESTFN, "/absolute")
        zipfp.close()

        zipfp = zipfile.ZipFile(TESTFN2, "r", zipfile.ZIP_STORED)
        self.assertEqual(zipfp.namelist(), ["absolute"])
        zipfp.close()

    def testAppendToZipFile(self):
        # Test appending to an existing zipfile
        zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED)
        zipfp.write(TESTFN, TESTFN)
        zipfp.close()
        zipfp = zipfile.ZipFile(TESTFN2, "a", zipfile.ZIP_STORED)
        zipfp.writestr("strfile", self.data)
        self.assertEqual(zipfp.namelist(), [TESTFN, "strfile"])
        zipfp.close()

    def testAppendToNonZipFile(self):
        # Test appending to an existing file that is not a zipfile
        # NOTE: this test fails if len(d) < 22 because of the first
        # line "fpin.seek(-22, 2)" in _EndRecData
        d = b'I am not a ZipFile!'*10
        f = open(TESTFN2, 'wb')
        f.write(d)
        f.close()
        zipfp = zipfile.ZipFile(TESTFN2, "a", zipfile.ZIP_STORED)
        zipfp.write(TESTFN, TESTFN)
        zipfp.close()

        f = open(TESTFN2, 'rb')
        f.seek(len(d))
        zipfp = zipfile.ZipFile(f, "r")
        self.assertEqual(zipfp.namelist(), [TESTFN])
        zipfp.close()
        f.close()

    def test_WriteDefaultName(self):
        # Check that calling ZipFile.write without arcname specified produces the expected result
        zipfp = zipfile.ZipFile(TESTFN2, "w")
        zipfp.write(TESTFN)
        self.assertEqual(zipfp.read(TESTFN), open(TESTFN, "rb").read())
        zipfp.close()

    def test_PerFileCompression(self):
        # Check that files within a Zip archive can have different compression options
        zipfp = zipfile.ZipFile(TESTFN2, "w")
        zipfp.write(TESTFN, 'storeme', zipfile.ZIP_STORED)
        zipfp.write(TESTFN, 'deflateme', zipfile.ZIP_DEFLATED)
        sinfo = zipfp.getinfo('storeme')
        dinfo = zipfp.getinfo('deflateme')
        self.assertEqual(sinfo.compress_type, zipfile.ZIP_STORED)
        self.assertEqual(dinfo.compress_type, zipfile.ZIP_DEFLATED)
        zipfp.close()

    def test_WriteToReadonly(self):
        # Check that trying to call write() on a readonly ZipFile object
        # raises a RuntimeError
        zipf = zipfile.ZipFile(TESTFN2, mode="w")
        zipf.writestr("somefile.txt", "bogus")
        zipf.close()
        zipf = zipfile.ZipFile(TESTFN2, mode="r")
        self.assertRaises(RuntimeError, zipf.write, TESTFN)
        zipf.close()

    def testExtract(self):
        zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED)
        for fpath, fdata in SMALL_TEST_DATA:
            zipfp.writestr(fpath, fdata)
        zipfp.close()

        zipfp = zipfile.ZipFile(TESTFN2, "r")
        for fpath, fdata in SMALL_TEST_DATA:
            writtenfile = zipfp.extract(fpath)

            # make sure it was written to the right place
            if os.path.isabs(fpath):
                correctfile = os.path.join(os.getcwd(), fpath[1:])
            else:
                correctfile = os.path.join(os.getcwd(), fpath)
            correctfile = os.path.normpath(correctfile)

            self.assertEqual(writtenfile, correctfile)

            # make sure correct data is in correct file
            self.assertEqual(fdata.encode(), open(writtenfile, "rb").read())

            os.remove(writtenfile)

        zipfp.close()

        # remove the test file subdirectories
        shutil.rmtree(os.path.join(os.getcwd(), 'ziptest2dir'))

    def testExtractAll(self):
        zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED)
        for fpath, fdata in SMALL_TEST_DATA:
            zipfp.writestr(fpath, fdata)
        zipfp.close()

        zipfp = zipfile.ZipFile(TESTFN2, "r")
        zipfp.extractall()
        for fpath, fdata in SMALL_TEST_DATA:
            if os.path.isabs(fpath):
                outfile = os.path.join(os.getcwd(), fpath[1:])
            else:
                outfile = os.path.join(os.getcwd(), fpath)

            self.assertEqual(fdata.encode(), open(outfile, "rb").read())

            os.remove(outfile)

        zipfp.close()

        # remove the test file subdirectories
        shutil.rmtree(os.path.join(os.getcwd(), 'ziptest2dir'))

    def zip_test_writestr_permissions(self, f, compression):
        # Make sure that writestr creates files with mode 0600,
        # when it is passed a name rather than a ZipInfo instance.

        self.makeTestArchive(f, compression)
        zipfp = zipfile.ZipFile(f, "r")
        zinfo = zipfp.getinfo('strfile')
        self.assertEqual(zinfo.external_attr, 0o600 << 16)

    def test_writestr_permissions(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zip_test_writestr_permissions(f, zipfile.ZIP_STORED)

    def test_writestr_extended_local_header_issue1202(self):
        orig_zip = zipfile.ZipFile(TESTFN2, 'w')
        for data in 'abcdefghijklmnop':
            zinfo = zipfile.ZipInfo(data)
            zinfo.flag_bits |= 0x08  # Include an extended local header.
            orig_zip.writestr(zinfo, data)
        orig_zip.close()

    def tearDown(self):
        os.remove(TESTFN)
        os.remove(TESTFN2)

class TestZip64InSmallFiles(unittest.TestCase):
    # These tests test the ZIP64 functionality without using large files,
    # see test_zipfile64 for proper tests.

    def setUp(self):
        self._limit = zipfile.ZIP64_LIMIT
        zipfile.ZIP64_LIMIT = 5

        line_gen = (bytes("Test of zipfile line %d." % i, "ascii")
                    for i in range(0, FIXEDTEST_SIZE))
        self.data = b'\n'.join(line_gen)

        # Make a source file with some lines
        fp = open(TESTFN, "wb")
        fp.write(self.data)
        fp.close()

    def largeFileExceptionTest(self, f, compression):
        zipfp = zipfile.ZipFile(f, "w", compression)
        self.assertRaises(zipfile.LargeZipFile,
                zipfp.write, TESTFN, "another.name")
        zipfp.close()

    def largeFileExceptionTest2(self, f, compression):
        zipfp = zipfile.ZipFile(f, "w", compression)
        self.assertRaises(zipfile.LargeZipFile,
                zipfp.writestr, "another.name", self.data)
        zipfp.close()

    def testLargeFileException(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.largeFileExceptionTest(f, zipfile.ZIP_STORED)
            self.largeFileExceptionTest2(f, zipfile.ZIP_STORED)

    def zipTest(self, f, compression):
        # Create the ZIP archive
        zipfp = zipfile.ZipFile(f, "w", compression, allowZip64=True)
        zipfp.write(TESTFN, "another.name")
        zipfp.write(TESTFN, TESTFN)
        zipfp.writestr("strfile", self.data)
        zipfp.close()

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        self.assertEqual(zipfp.read(TESTFN), self.data)
        self.assertEqual(zipfp.read("another.name"), self.data)
        self.assertEqual(zipfp.read("strfile"), self.data)

        # Print the ZIP directory
        fp = io.StringIO()
        zipfp.printdir(fp)

        directory = fp.getvalue()
        lines = directory.splitlines()
        self.assertEqual(len(lines), 4) # Number of files + header

        self.assertTrue('File Name' in lines[0])
        self.assertTrue('Modified' in lines[0])
        self.assertTrue('Size' in lines[0])

        fn, date, time, size = lines[1].split()
        self.assertEqual(fn, 'another.name')
        # XXX: timestamp is not tested
        self.assertEqual(size, str(len(self.data)))

        # Check the namelist
        names = zipfp.namelist()
        self.assertEqual(len(names), 3)
        self.assertTrue(TESTFN in names)
        self.assertTrue("another.name" in names)
        self.assertTrue("strfile" in names)

        # Check infolist
        infos = zipfp.infolist()
        names = [ i.filename for i in infos ]
        self.assertEqual(len(names), 3)
        self.assertTrue(TESTFN in names)
        self.assertTrue("another.name" in names)
        self.assertTrue("strfile" in names)
        for i in infos:
            self.assertEqual(i.file_size, len(self.data))

        # check getinfo
        for nm in (TESTFN, "another.name", "strfile"):
            info = zipfp.getinfo(nm)
            self.assertEqual(info.filename, nm)
            self.assertEqual(info.file_size, len(self.data))

        # Check that testzip doesn't raise an exception
        zipfp.testzip()


        zipfp.close()

    def testStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipTest(f, zipfile.ZIP_STORED)


    if zlib:
        def testDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
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
        self.assertTrue(bn not in zipfp.namelist())
        self.assertTrue(bn + 'o' in zipfp.namelist() or bn + 'c' in zipfp.namelist())
        zipfp.close()


        zipfp  = zipfile.PyZipFile(TemporaryFile(), "w")
        fn = __file__
        if fn.endswith('.pyc') or fn.endswith('.pyo'):
            fn = fn[:-1]

        zipfp.writepy(fn, "testpackage")

        bn = "%s/%s"%("testpackage", os.path.basename(fn))
        self.assertTrue(bn not in zipfp.namelist())
        self.assertTrue(bn + 'o' in zipfp.namelist() or bn + 'c' in zipfp.namelist())
        zipfp.close()

    def testWritePythonPackage(self):
        import email
        packagedir = os.path.dirname(email.__file__)

        zipfp  = zipfile.PyZipFile(TemporaryFile(), "w")
        zipfp.writepy(packagedir)

        # Check for a couple of modules at different levels of the hieararchy
        names = zipfp.namelist()
        self.assertTrue('email/__init__.pyo' in names or 'email/__init__.pyc' in names)
        self.assertTrue('email/mime/text.pyo' in names or 'email/mime/text.pyc' in names)

    def testWritePythonDirectory(self):
        os.mkdir(TESTFN2)
        try:
            fp = open(os.path.join(TESTFN2, "mod1.py"), "w")
            fp.write("print(42)\n")
            fp.close()

            fp = open(os.path.join(TESTFN2, "mod2.py"), "w")
            fp.write("print(42 * 42)\n")
            fp.close()

            fp = open(os.path.join(TESTFN2, "mod2.txt"), "w")
            fp.write("bla bla bla\n")
            fp.close()

            zipfp  = zipfile.PyZipFile(TemporaryFile(), "w")
            zipfp.writepy(TESTFN2)

            names = zipfp.namelist()
            self.assertTrue('mod1.pyc' in names or 'mod1.pyo' in names)
            self.assertTrue('mod2.pyc' in names or 'mod2.pyo' in names)
            self.assertTrue('mod2.txt' not in names)

        finally:
            shutil.rmtree(TESTFN2)

    def testWriteNonPyfile(self):
        zipfp  = zipfile.PyZipFile(TemporaryFile(), "w")
        open(TESTFN, 'w').write('most definitely not a python file')
        self.assertRaises(RuntimeError, zipfp.writepy, TESTFN)
        os.remove(TESTFN)


class OtherTests(unittest.TestCase):
    zips_with_bad_crc = {
        zipfile.ZIP_STORED: (
            b'PK\003\004\024\0\0\0\0\0 \213\212;:r'
            b'\253\377\f\0\0\0\f\0\0\0\005\0\0\000af'
            b'ilehello,AworldP'
            b'K\001\002\024\003\024\0\0\0\0\0 \213\212;:'
            b'r\253\377\f\0\0\0\f\0\0\0\005\0\0\0\0'
            b'\0\0\0\0\0\0\0\200\001\0\0\0\000afi'
            b'lePK\005\006\0\0\0\0\001\0\001\0003\000'
            b'\0\0/\0\0\0\0\0'),
        zipfile.ZIP_DEFLATED: (
            b'PK\x03\x04\x14\x00\x00\x00\x08\x00n}\x0c=FA'
            b'KE\x10\x00\x00\x00n\x00\x00\x00\x05\x00\x00\x00af'
            b'ile\xcbH\xcd\xc9\xc9W(\xcf/\xcaI\xc9\xa0'
            b'=\x13\x00PK\x01\x02\x14\x03\x14\x00\x00\x00\x08\x00n'
            b'}\x0c=FAKE\x10\x00\x00\x00n\x00\x00\x00\x05'
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x01\x00\x00\x00'
            b'\x00afilePK\x05\x06\x00\x00\x00\x00\x01\x00'
            b'\x01\x003\x00\x00\x003\x00\x00\x00\x00\x00'),
    }

    def testUnicodeFilenames(self):
        zf = zipfile.ZipFile(TESTFN, "w")
        zf.writestr("foo.txt", "Test for unicode filename")
        zf.writestr("\xf6.txt", "Test for unicode filename")
        zf.close()
        zf = zipfile.ZipFile(TESTFN, "r")
        self.assertEqual(zf.filelist[0].filename, "foo.txt")
        self.assertEqual(zf.filelist[1].filename, "\xf6.txt")
        zf.close()

    def testCreateNonExistentFileForAppend(self):
        if os.path.exists(TESTFN):
            os.unlink(TESTFN)

        filename = 'testfile.txt'
        content = b'hello, world. this is some content.'

        try:
            zf = zipfile.ZipFile(TESTFN, 'a')
            zf.writestr(filename, content)
            zf.close()
        except IOError:
            self.fail('Could not append data to a non-existent zip file.')

        self.assertTrue(os.path.exists(TESTFN))

        zf = zipfile.ZipFile(TESTFN, 'r')
        self.assertEqual(zf.read(filename), content)
        zf.close()

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
            pass

    def testIsZipErroneousFile(self):
        # This test checks that the is_zipfile function correctly identifies
        # a file that is not a zip file

        # - passing a filename
        with open(TESTFN, "w") as fp:
            fp.write("this is not a legal zip file\n")
        chk = zipfile.is_zipfile(TESTFN)
        self.assertTrue(not chk)
        # - passing a file object
        with open(TESTFN, "rb") as fp:
            chk = zipfile.is_zipfile(fp)
            self.assertTrue(not chk)
        # - passing a file-like object
        fp = io.BytesIO()
        fp.write(b"this is not a legal zip file\n")
        chk = zipfile.is_zipfile(fp)
        self.assertTrue(not chk)
        fp.seek(0,0)
        chk = zipfile.is_zipfile(fp)
        self.assertTrue(not chk)

    def testIsZipValidFile(self):
        # This test checks that the is_zipfile function correctly identifies
        # a file that is a zip file

        # - passing a filename
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        zipf.writestr("foo.txt", b"O, for a Muse of Fire!")
        zipf.close()
        chk = zipfile.is_zipfile(TESTFN)
        self.assertTrue(chk)
        # - passing a file object
        with open(TESTFN, "rb") as fp:
            chk = zipfile.is_zipfile(fp)
            self.assertTrue(chk)
            fp.seek(0,0)
            zip_contents = fp.read()
        # - passing a file-like object
        fp = io.BytesIO()
        fp.write(zip_contents)
        chk = zipfile.is_zipfile(fp)
        self.assertTrue(chk)
        fp.seek(0,0)
        chk = zipfile.is_zipfile(fp)
        self.assertTrue(chk)

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

    def test_empty_file_raises_BadZipFile(self):
        f = open(TESTFN, 'w')
        f.close()
        self.assertRaises(zipfile.BadZipfile, zipfile.ZipFile, TESTFN)

        f = open(TESTFN, 'w')
        f.write("short file")
        f.close()
        self.assertRaises(zipfile.BadZipfile, zipfile.ZipFile, TESTFN)

    def testClosedZipRaisesRuntimeError(self):
        # Verify that testzip() doesn't swallow inappropriate exceptions.
        data = io.BytesIO()
        zipf = zipfile.ZipFile(data, mode="w")
        zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        zipf.close()

        # This is correct; calling .read on a closed ZipFile should throw
        # a RuntimeError, and so should calling .testzip.  An earlier
        # version of .testzip would swallow this exception (and any other)
        # and report that the first file in the archive was corrupt.
        self.assertRaises(RuntimeError, zipf.read, "foo.txt")
        self.assertRaises(RuntimeError, zipf.open, "foo.txt")
        self.assertRaises(RuntimeError, zipf.testzip)
        self.assertRaises(RuntimeError, zipf.writestr, "bogus.txt", "bogus")
        open(TESTFN, 'w').write('zipfile test data')
        self.assertRaises(RuntimeError, zipf.write, TESTFN)

    def test_BadConstructorMode(self):
        # Check that bad modes passed to ZipFile constructor are caught
        self.assertRaises(RuntimeError, zipfile.ZipFile, TESTFN, "q")

    def test_BadOpenMode(self):
        # Check that bad modes passed to ZipFile.open are caught
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        zipf.close()
        zipf = zipfile.ZipFile(TESTFN, mode="r")
        # read the data to make sure the file is there
        zipf.read("foo.txt")
        self.assertRaises(RuntimeError, zipf.open, "foo.txt", "q")
        zipf.close()

    def test_Read0(self):
        # Check that calling read(0) on a ZipExtFile object returns an empty
        # string and doesn't advance file pointer
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        # read the data to make sure the file is there
        f = zipf.open("foo.txt")
        for i in range(FIXEDTEST_SIZE):
            self.assertEqual(f.read(0), b'')

        self.assertEqual(f.read(), b"O, for a Muse of Fire!")
        zipf.close()

    def test_OpenNonexistentItem(self):
        # Check that attempting to call open() for an item that doesn't
        # exist in the archive raises a RuntimeError
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        self.assertRaises(KeyError, zipf.open, "foo.txt", "r")

    def test_BadCompressionMode(self):
        # Check that bad compression methods passed to ZipFile.open are caught
        self.assertRaises(RuntimeError, zipfile.ZipFile, TESTFN, "w", -1)

    def test_NullByteInFilename(self):
        # Check that a filename containing a null byte is properly terminated
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        zipf.writestr("foo.txt\x00qqq", b"O, for a Muse of Fire!")
        self.assertEqual(zipf.namelist(), ['foo.txt'])

    def test_StructSizes(self):
        # check that ZIP internal structure sizes are calculated correctly
        self.assertEqual(zipfile.sizeEndCentDir, 22)
        self.assertEqual(zipfile.sizeCentralDir, 46)
        self.assertEqual(zipfile.sizeEndCentDir64, 56)
        self.assertEqual(zipfile.sizeEndCentDir64Locator, 20)

    def testComments(self):
        # This test checks that comments on the archive are handled properly

        # check default comment is empty
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        self.assertEqual(zipf.comment, b'')
        zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        zipf.close()
        zipfr = zipfile.ZipFile(TESTFN, mode="r")
        self.assertEqual(zipfr.comment, b'')
        zipfr.close()

        # check a simple short comment
        comment = b'Bravely taking to his feet, he beat a very brave retreat.'
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        zipf.comment = comment
        zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        zipf.close()
        zipfr = zipfile.ZipFile(TESTFN, mode="r")
        self.assertEqual(zipfr.comment, comment)
        zipfr.close()

        # check a comment of max length
        comment2 = ''.join(['%d' % (i**3 % 10) for i in range((1 << 16)-1)])
        comment2 = comment2.encode("ascii")
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        zipf.comment = comment2
        zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        zipf.close()
        zipfr = zipfile.ZipFile(TESTFN, mode="r")
        self.assertEqual(zipfr.comment, comment2)
        zipfr.close()

        # check a comment that is too long is truncated
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        zipf.comment = comment2 + b'oops'
        zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        zipf.close()
        zipfr = zipfile.ZipFile(TESTFN, mode="r")
        self.assertEqual(zipfr.comment, comment2)
        zipfr.close()

    def check_testzip_with_bad_crc(self, compression):
        """Tests that files with bad CRCs return their name from testzip."""
        zipdata = self.zips_with_bad_crc[compression]

        zipf = zipfile.ZipFile(io.BytesIO(zipdata), mode="r")
        # testzip returns the name of the first corrupt file, or None
        self.assertEqual('afile', zipf.testzip())
        zipf.close()

    def test_testzip_with_bad_crc_stored(self):
        self.check_testzip_with_bad_crc(zipfile.ZIP_STORED)

    if zlib:
        def test_testzip_with_bad_crc_deflated(self):
            self.check_testzip_with_bad_crc(zipfile.ZIP_DEFLATED)

    def check_read_with_bad_crc(self, compression):
        """Tests that files with bad CRCs raise a BadZipfile exception when read."""
        zipdata = self.zips_with_bad_crc[compression]

        # Using ZipFile.read()
        zipf = zipfile.ZipFile(io.BytesIO(zipdata), mode="r")
        self.assertRaises(zipfile.BadZipfile, zipf.read, 'afile')
        zipf.close()

        # Using ZipExtFile.read()
        zipf = zipfile.ZipFile(io.BytesIO(zipdata), mode="r")
        corrupt_file = zipf.open('afile', 'r')
        self.assertRaises(zipfile.BadZipfile, corrupt_file.read)
        corrupt_file.close()
        zipf.close()

        # Same with small reads (in order to exercise the buffering logic)
        zipf = zipfile.ZipFile(io.BytesIO(zipdata), mode="r")
        corrupt_file = zipf.open('afile', 'r')
        corrupt_file.MIN_READ_SIZE = 2
        with self.assertRaises(zipfile.BadZipfile):
            while corrupt_file.read(2):
                pass
        corrupt_file.close()
        zipf.close()

    def test_read_with_bad_crc_stored(self):
        self.check_read_with_bad_crc(zipfile.ZIP_STORED)

    if zlib:
        def test_read_with_bad_crc_deflated(self):
            self.check_read_with_bad_crc(zipfile.ZIP_DEFLATED)

    def check_read_return_size(self, compression):
        # Issue #9837: ZipExtFile.read() shouldn't return more bytes
        # than requested.
        for test_size in (1, 4095, 4096, 4097, 16384):
            file_size = test_size + 1
            junk = b''.join(struct.pack('B', randint(0, 255))
                            for x in range(file_size))
            zipf = zipfile.ZipFile(io.BytesIO(), "w", compression)
            try:
                zipf.writestr('foo', junk)
                fp = zipf.open('foo', 'r')
                buf = fp.read(test_size)
                self.assertEqual(len(buf), test_size)
            finally:
                zipf.close()

    def test_read_return_size_stored(self):
        self.check_read_return_size(zipfile.ZIP_STORED)

    if zlib:
        def test_read_return_size_deflated(self):
            self.check_read_return_size(zipfile.ZIP_DEFLATED)

    def test_empty_zipfile(self):
        # Check that creating a file in 'w' or 'a' mode and closing without
        # adding any files to the archives creates a valid empty ZIP file
        zipf = zipfile.ZipFile(TESTFN, mode="w")
        zipf.close()
        try:
            zipf = zipfile.ZipFile(TESTFN, mode="r")
        except zipfile.BadZipFile:
            self.fail("Unable to create empty ZIP file in 'w' mode")

        zipf = zipfile.ZipFile(TESTFN, mode="a")
        zipf.close()
        try:
            zipf = zipfile.ZipFile(TESTFN, mode="r")
        except:
            self.fail("Unable to create empty ZIP file in 'a' mode")

    def test_open_empty_file(self):
        # Issue 1710703: Check that opening a file with less than 22 bytes
        # raises a BadZipfile exception (rather than the previously unhelpful
        # IOError)
        f = open(TESTFN, 'w')
        f.close()
        self.assertRaises(zipfile.BadZipfile, zipfile.ZipFile, TESTFN, 'r')

    def tearDown(self):
        support.unlink(TESTFN)
        support.unlink(TESTFN2)

class DecryptionTests(unittest.TestCase):
    # This test checks that ZIP decryption works. Since the library does not
    # support encryption at the moment, we use a pre-generated encrypted
    # ZIP file

    data = (
    b'PK\x03\x04\x14\x00\x01\x00\x00\x00n\x92i.#y\xef?&\x00\x00\x00\x1a\x00'
    b'\x00\x00\x08\x00\x00\x00test.txt\xfa\x10\xa0gly|\xfa-\xc5\xc0=\xf9y'
    b'\x18\xe0\xa8r\xb3Z}Lg\xbc\xae\xf9|\x9b\x19\xe4\x8b\xba\xbb)\x8c\xb0\xdbl'
    b'PK\x01\x02\x14\x00\x14\x00\x01\x00\x00\x00n\x92i.#y\xef?&\x00\x00\x00'
    b'\x1a\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x01\x00 \x00\xb6\x81'
    b'\x00\x00\x00\x00test.txtPK\x05\x06\x00\x00\x00\x00\x01\x00\x01\x006\x00'
    b'\x00\x00L\x00\x00\x00\x00\x00' )
    data2 = (
    b'PK\x03\x04\x14\x00\t\x00\x08\x00\xcf}38xu\xaa\xb2\x14\x00\x00\x00\x00\x02'
    b'\x00\x00\x04\x00\x15\x00zeroUT\t\x00\x03\xd6\x8b\x92G\xda\x8b\x92GUx\x04'
    b'\x00\xe8\x03\xe8\x03\xc7<M\xb5a\xceX\xa3Y&\x8b{oE\xd7\x9d\x8c\x98\x02\xc0'
    b'PK\x07\x08xu\xaa\xb2\x14\x00\x00\x00\x00\x02\x00\x00PK\x01\x02\x17\x03'
    b'\x14\x00\t\x00\x08\x00\xcf}38xu\xaa\xb2\x14\x00\x00\x00\x00\x02\x00\x00'
    b'\x04\x00\r\x00\x00\x00\x00\x00\x00\x00\x00\x00\xa4\x81\x00\x00\x00\x00ze'
    b'roUT\x05\x00\x03\xd6\x8b\x92GUx\x00\x00PK\x05\x06\x00\x00\x00\x00\x01'
    b'\x00\x01\x00?\x00\x00\x00[\x00\x00\x00\x00\x00' )

    plain = b'zipfile.py encryption test'
    plain2 = b'\x00'*512

    def setUp(self):
        fp = open(TESTFN, "wb")
        fp.write(self.data)
        fp.close()
        self.zip = zipfile.ZipFile(TESTFN, "r")
        fp = open(TESTFN2, "wb")
        fp.write(self.data2)
        fp.close()
        self.zip2 = zipfile.ZipFile(TESTFN2, "r")

    def tearDown(self):
        self.zip.close()
        os.unlink(TESTFN)
        self.zip2.close()
        os.unlink(TESTFN2)

    def testNoPassword(self):
        # Reading the encrypted file without password
        # must generate a RunTime exception
        self.assertRaises(RuntimeError, self.zip.read, "test.txt")
        self.assertRaises(RuntimeError, self.zip2.read, "zero")

    def testBadPassword(self):
        self.zip.setpassword(b"perl")
        self.assertRaises(RuntimeError, self.zip.read, "test.txt")
        self.zip2.setpassword(b"perl")
        self.assertRaises(RuntimeError, self.zip2.read, "zero")

    def testGoodPassword(self):
        self.zip.setpassword(b"python")
        self.assertEqual(self.zip.read("test.txt"), self.plain)
        self.zip2.setpassword(b"12345")
        self.assertEqual(self.zip2.read("zero"), self.plain2)

    def test_unicode_password(self):
        self.assertRaises(TypeError, self.zip.setpassword, "unicode")
        self.assertRaises(TypeError, self.zip.read, "test.txt", "python")
        self.assertRaises(TypeError, self.zip.open, "test.txt", pwd="python")
        self.assertRaises(TypeError, self.zip.extract, "test.txt", pwd="python")


class TestsWithRandomBinaryFiles(unittest.TestCase):
    def setUp(self):
        datacount = randint(16, 64)*1024 + randint(1, 1024)
        self.data = b''.join(struct.pack('<f', random()*randint(-1000, 1000))
                             for i in range(datacount))

        # Make a source file with some lines
        fp = open(TESTFN, "wb")
        fp.write(self.data)
        fp.close()

    def tearDown(self):
        support.unlink(TESTFN)
        support.unlink(TESTFN2)

    def makeTestArchive(self, f, compression):
        # Create the ZIP archive
        zipfp = zipfile.ZipFile(f, "w", compression)
        zipfp.write(TESTFN, "another.name")
        zipfp.write(TESTFN, TESTFN)
        zipfp.close()

    def zipTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        testdata = zipfp.read(TESTFN)
        self.assertEqual(len(testdata), len(self.data))
        self.assertEqual(testdata, self.data)
        self.assertEqual(zipfp.read("another.name"), self.data)
        zipfp.close()

    def testStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipTest(f, zipfile.ZIP_STORED)

    def zipOpenTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        zipdata1 = []
        zipopen1 = zipfp.open(TESTFN)
        while 1:
            read_data = zipopen1.read(256)
            if not read_data:
                break
            zipdata1.append(read_data)

        zipdata2 = []
        zipopen2 = zipfp.open("another.name")
        while 1:
            read_data = zipopen2.read(256)
            if not read_data:
                break
            zipdata2.append(read_data)

        testdata1 = b''.join(zipdata1)
        self.assertEqual(len(testdata1), len(self.data))
        self.assertEqual(testdata1, self.data)

        testdata2 = b''.join(zipdata2)
        self.assertEqual(len(testdata1), len(self.data))
        self.assertEqual(testdata1, self.data)
        zipfp.close()

    def testOpenStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipOpenTest(f, zipfile.ZIP_STORED)

    def zipRandomOpenTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r", compression)
        zipdata1 = []
        zipopen1 = zipfp.open(TESTFN)
        while 1:
            read_data = zipopen1.read(randint(1, 1024))
            if not read_data:
                break
            zipdata1.append(read_data)

        testdata = b''.join(zipdata1)
        self.assertEqual(len(testdata), len(self.data))
        self.assertEqual(testdata, self.data)
        zipfp.close()

    def testRandomOpenStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.zipRandomOpenTest(f, zipfile.ZIP_STORED)

class TestsWithMultipleOpens(unittest.TestCase):
    def setUp(self):
        # Create the ZIP archive
        zipfp = zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_DEFLATED)
        zipfp.writestr('ones', '1'*FIXEDTEST_SIZE)
        zipfp.writestr('twos', '2'*FIXEDTEST_SIZE)
        zipfp.close()

    def testSameFile(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        zipf = zipfile.ZipFile(TESTFN2, mode="r")
        zopen1 = zipf.open('ones')
        zopen2 = zipf.open('ones')
        data1 = zopen1.read(500)
        data2 = zopen2.read(500)
        data1 += zopen1.read(500)
        data2 += zopen2.read(500)
        self.assertEqual(data1, data2)
        zipf.close()

    def testDifferentFile(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        zipf = zipfile.ZipFile(TESTFN2, mode="r")
        zopen1 = zipf.open('ones')
        zopen2 = zipf.open('twos')
        data1 = zopen1.read(500)
        data2 = zopen2.read(500)
        data1 += zopen1.read(500)
        data2 += zopen2.read(500)
        self.assertEqual(data1, b'1'*FIXEDTEST_SIZE)
        self.assertEqual(data2, b'2'*FIXEDTEST_SIZE)
        zipf.close()

    def testInterleaved(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        zipf = zipfile.ZipFile(TESTFN2, mode="r")
        zopen1 = zipf.open('ones')
        data1 = zopen1.read(500)
        zopen2 = zipf.open('twos')
        data2 = zopen2.read(500)
        data1 += zopen1.read(500)
        data2 += zopen2.read(500)
        self.assertEqual(data1, b'1'*FIXEDTEST_SIZE)
        self.assertEqual(data2, b'2'*FIXEDTEST_SIZE)
        zipf.close()

    def tearDown(self):
        os.remove(TESTFN2)

class TestWithDirectory(unittest.TestCase):
    def setUp(self):
        os.mkdir(TESTFN2)

    def testExtractDir(self):
        zipf = zipfile.ZipFile(findfile("zipdir.zip"))
        zipf.extractall(TESTFN2)
        self.assertTrue(os.path.isdir(os.path.join(TESTFN2, "a")))
        self.assertTrue(os.path.isdir(os.path.join(TESTFN2, "a", "b")))
        self.assertTrue(os.path.exists(os.path.join(TESTFN2, "a", "b", "c")))

    def test_bug_6050(self):
        # Extraction should succeed if directories already exist
        os.mkdir(os.path.join(TESTFN2, "a"))
        self.testExtractDir()

    def testStoreDir(self):
        os.mkdir(os.path.join(TESTFN2, "x"))
        zipf = zipfile.ZipFile(TESTFN, "w")
        zipf.write(os.path.join(TESTFN2, "x"), "x")
        self.assertTrue(zipf.filelist[0].filename.endswith("x/"))

    def tearDown(self):
        shutil.rmtree(TESTFN2)
        if os.path.exists(TESTFN):
            os.remove(TESTFN)


class UniversalNewlineTests(unittest.TestCase):
    def setUp(self):
        self.line_gen = [bytes("Test of zipfile line %d." % i, "ascii")
                         for i in range(FIXEDTEST_SIZE)]
        self.seps = ('\r', '\r\n', '\n')
        self.arcdata, self.arcfiles = {}, {}
        for n, s in enumerate(self.seps):
            b = s.encode("ascii")
            self.arcdata[s] = b.join(self.line_gen) + b
            self.arcfiles[s] = '%s-%d' % (TESTFN, n)
            f = open(self.arcfiles[s], "wb")
            try:
                f.write(self.arcdata[s])
            finally:
                f.close()

    def makeTestArchive(self, f, compression):
        # Create the ZIP archive
        zipfp = zipfile.ZipFile(f, "w", compression)
        for fn in self.arcfiles.values():
            zipfp.write(fn, fn)
        zipfp.close()

    def readTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r")
        for sep, fn in self.arcfiles.items():
            zipdata = zipfp.open(fn, "rU").read()
            self.assertEqual(self.arcdata[sep], zipdata)

        zipfp.close()

    def readlineTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r")
        for sep, fn in self.arcfiles.items():
            zipopen = zipfp.open(fn, "rU")
            for line in self.line_gen:
                linedata = zipopen.readline()
                self.assertEqual(linedata, line + b'\n')

        zipfp.close()

    def readlinesTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r")
        for sep, fn in self.arcfiles.items():
            ziplines = zipfp.open(fn, "rU").readlines()
            for line, zipline in zip(self.line_gen, ziplines):
                self.assertEqual(zipline, line + b'\n')

        zipfp.close()

    def iterlinesTest(self, f, compression):
        self.makeTestArchive(f, compression)

        # Read the ZIP archive
        zipfp = zipfile.ZipFile(f, "r")
        for sep, fn in self.arcfiles.items():
            for line, zipline in zip(self.line_gen, zipfp.open(fn, "rU")):
                self.assertEqual(zipline, line + b'\n')

        zipfp.close()

    def testReadStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.readTest(f, zipfile.ZIP_STORED)

    def testReadlineStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.readlineTest(f, zipfile.ZIP_STORED)

    def testReadlinesStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.readlinesTest(f, zipfile.ZIP_STORED)

    def testIterlinesStored(self):
        for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
            self.iterlinesTest(f, zipfile.ZIP_STORED)

    if zlib:
        def testReadDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.readTest(f, zipfile.ZIP_DEFLATED)

        def testReadlineDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.readlineTest(f, zipfile.ZIP_DEFLATED)

        def testReadlinesDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.readlinesTest(f, zipfile.ZIP_DEFLATED)

        def testIterlinesDeflated(self):
            for f in (TESTFN2, TemporaryFile(), io.BytesIO()):
                self.iterlinesTest(f, zipfile.ZIP_DEFLATED)

    def tearDown(self):
        for sep, fn in self.arcfiles.items():
            os.remove(fn)
        support.unlink(TESTFN)
        support.unlink(TESTFN2)


def test_main():
    run_unittest(TestsWithSourceFile, TestZip64InSmallFiles, OtherTests,
                 PyZipFileTests, DecryptionTests, TestsWithMultipleOpens,
                 TestWithDirectory,
                 UniversalNewlineTests, TestsWithRandomBinaryFiles)

if __name__ == "__main__":
    test_main()
