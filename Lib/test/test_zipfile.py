import io
import os
import sys
import imp
import time
import shutil
import struct
import zipfile
import unittest


from tempfile import TemporaryFile
from random import randint, random, getrandbits

from test.support import (TESTFN, findfile, unlink,
                          requires_zlib, requires_bz2, requires_lzma,
                          captured_stdout)

TESTFN2 = TESTFN + "2"
TESTFNDIR = TESTFN + "d"
FIXEDTEST_SIZE = 1000
DATAFILES_DIR = 'zipfile_datafiles'

SMALL_TEST_DATA = [('_ziptest1', '1q2w3e4r5t'),
                   ('ziptest2dir/_ziptest2', 'qawsedrftg'),
                   ('ziptest2dir/ziptest3dir/_ziptest3', 'azsxdcfvgb'),
                   ('ziptest2dir/ziptest3dir/ziptest4dir/_ziptest3', '6y7u8i9o0p')]

def get_files(test):
    yield TESTFN2
    with TemporaryFile() as f:
        yield f
        test.assertFalse(f.closed)
    with io.BytesIO() as f:
        yield f
        test.assertFalse(f.closed)

class AbstractTestsWithSourceFile:
    @classmethod
    def setUpClass(cls):
        cls.line_gen = [bytes("Zipfile test line %d. random float: %f\n" %
                              (i, random()), "ascii")
                        for i in range(FIXEDTEST_SIZE)]
        cls.data = b''.join(cls.line_gen)

    def setUp(self):
        # Make a source file with some lines
        with open(TESTFN, "wb") as fp:
            fp.write(self.data)

    def make_test_archive(self, f, compression):
        # Create the ZIP archive
        with zipfile.ZipFile(f, "w", compression) as zipfp:
            zipfp.write(TESTFN, "another.name")
            zipfp.write(TESTFN, TESTFN)
            zipfp.writestr("strfile", self.data)

    def zip_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r", compression) as zipfp:
            self.assertEqual(zipfp.read(TESTFN), self.data)
            self.assertEqual(zipfp.read("another.name"), self.data)
            self.assertEqual(zipfp.read("strfile"), self.data)

            # Print the ZIP directory
            fp = io.StringIO()
            zipfp.printdir(file=fp)
            directory = fp.getvalue()
            lines = directory.splitlines()
            self.assertEqual(len(lines), 4) # Number of files + header

            self.assertIn('File Name', lines[0])
            self.assertIn('Modified', lines[0])
            self.assertIn('Size', lines[0])

            fn, date, time_, size = lines[1].split()
            self.assertEqual(fn, 'another.name')
            self.assertTrue(time.strptime(date, '%Y-%m-%d'))
            self.assertTrue(time.strptime(time_, '%H:%M:%S'))
            self.assertEqual(size, str(len(self.data)))

            # Check the namelist
            names = zipfp.namelist()
            self.assertEqual(len(names), 3)
            self.assertIn(TESTFN, names)
            self.assertIn("another.name", names)
            self.assertIn("strfile", names)

            # Check infolist
            infos = zipfp.infolist()
            names = [i.filename for i in infos]
            self.assertEqual(len(names), 3)
            self.assertIn(TESTFN, names)
            self.assertIn("another.name", names)
            self.assertIn("strfile", names)
            for i in infos:
                self.assertEqual(i.file_size, len(self.data))

            # check getinfo
            for nm in (TESTFN, "another.name", "strfile"):
                info = zipfp.getinfo(nm)
                self.assertEqual(info.filename, nm)
                self.assertEqual(info.file_size, len(self.data))

            # Check that testzip doesn't raise an exception
            zipfp.testzip()

    def test_basic(self):
        for f in get_files(self):
            self.zip_test(f, self.compression)

    def zip_open_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r", compression) as zipfp:
            zipdata1 = []
            with zipfp.open(TESTFN) as zipopen1:
                while True:
                    read_data = zipopen1.read(256)
                    if not read_data:
                        break
                    zipdata1.append(read_data)

            zipdata2 = []
            with zipfp.open("another.name") as zipopen2:
                while True:
                    read_data = zipopen2.read(256)
                    if not read_data:
                        break
                    zipdata2.append(read_data)

            self.assertEqual(b''.join(zipdata1), self.data)
            self.assertEqual(b''.join(zipdata2), self.data)

    def test_open(self):
        for f in get_files(self):
            self.zip_open_test(f, self.compression)

    def zip_random_open_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r", compression) as zipfp:
            zipdata1 = []
            with zipfp.open(TESTFN) as zipopen1:
                while True:
                    read_data = zipopen1.read(randint(1, 1024))
                    if not read_data:
                        break
                    zipdata1.append(read_data)

            self.assertEqual(b''.join(zipdata1), self.data)

    def test_random_open(self):
        for f in get_files(self):
            self.zip_random_open_test(f, self.compression)

    def zip_read1_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp, \
             zipfp.open(TESTFN) as zipopen:
            zipdata = []
            while True:
                read_data = zipopen.read1(-1)
                if not read_data:
                    break
                zipdata.append(read_data)

        self.assertEqual(b''.join(zipdata), self.data)

    def test_read1(self):
        for f in get_files(self):
            self.zip_read1_test(f, self.compression)

    def zip_read1_10_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp, \
             zipfp.open(TESTFN) as zipopen:
            zipdata = []
            while True:
                read_data = zipopen.read1(10)
                self.assertLessEqual(len(read_data), 10)
                if not read_data:
                    break
                zipdata.append(read_data)

        self.assertEqual(b''.join(zipdata), self.data)

    def test_read1_10(self):
        for f in get_files(self):
            self.zip_read1_10_test(f, self.compression)

    def zip_readline_read_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp, \
             zipfp.open(TESTFN) as zipopen:
            data = b''
            while True:
                read = zipopen.readline()
                if not read:
                    break
                data += read

                read = zipopen.read(100)
                if not read:
                    break
                data += read

        self.assertEqual(data, self.data)

    def test_readline_read(self):
        # Issue #7610: calls to readline() interleaved with calls to read().
        for f in get_files(self):
            self.zip_readline_read_test(f, self.compression)

    def zip_readline_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp:
            with zipfp.open(TESTFN) as zipopen:
                for line in self.line_gen:
                    linedata = zipopen.readline()
                    self.assertEqual(linedata, line)

    def test_readline(self):
        for f in get_files(self):
            self.zip_readline_test(f, self.compression)

    def zip_readlines_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp:
            with zipfp.open(TESTFN) as zipopen:
                ziplines = zipopen.readlines()
            for line, zipline in zip(self.line_gen, ziplines):
                self.assertEqual(zipline, line)

    def test_readlines(self):
        for f in get_files(self):
            self.zip_readlines_test(f, self.compression)

    def zip_iterlines_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp:
            with zipfp.open(TESTFN) as zipopen:
                for line, zipline in zip(self.line_gen, zipopen):
                    self.assertEqual(zipline, line)

    def test_iterlines(self):
        for f in get_files(self):
            self.zip_iterlines_test(f, self.compression)

    def test_low_compression(self):
        """Check for cases where compressed data is larger than original."""
        # Create the ZIP archive
        with zipfile.ZipFile(TESTFN2, "w", self.compression) as zipfp:
            zipfp.writestr("strfile", '12')

        # Get an open object for strfile
        with zipfile.ZipFile(TESTFN2, "r", self.compression) as zipfp:
            with zipfp.open("strfile") as openobj:
                self.assertEqual(openobj.read(1), b'1')
                self.assertEqual(openobj.read(1), b'2')

    def test_writestr_compression(self):
        zipfp = zipfile.ZipFile(TESTFN2, "w")
        zipfp.writestr("b.txt", "hello world", compress_type=self.compression)
        info = zipfp.getinfo('b.txt')
        self.assertEqual(info.compress_type, self.compression)

    def test_read_return_size(self):
        # Issue #9837: ZipExtFile.read() shouldn't return more bytes
        # than requested.
        for test_size in (1, 4095, 4096, 4097, 16384):
            file_size = test_size + 1
            junk = getrandbits(8 * file_size).to_bytes(file_size, 'little')
            with zipfile.ZipFile(io.BytesIO(), "w", self.compression) as zipf:
                zipf.writestr('foo', junk)
                with zipf.open('foo', 'r') as fp:
                    buf = fp.read(test_size)
                    self.assertEqual(len(buf), test_size)

    def tearDown(self):
        unlink(TESTFN)
        unlink(TESTFN2)


class StoredTestsWithSourceFile(AbstractTestsWithSourceFile,
                                unittest.TestCase):
    compression = zipfile.ZIP_STORED
    test_low_compression = None

    def zip_test_writestr_permissions(self, f, compression):
        # Make sure that writestr creates files with mode 0600,
        # when it is passed a name rather than a ZipInfo instance.

        self.make_test_archive(f, compression)
        with zipfile.ZipFile(f, "r") as zipfp:
            zinfo = zipfp.getinfo('strfile')
            self.assertEqual(zinfo.external_attr, 0o600 << 16)

    def test_writestr_permissions(self):
        for f in get_files(self):
            self.zip_test_writestr_permissions(f, zipfile.ZIP_STORED)

    def test_absolute_arcnames(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            zipfp.write(TESTFN, "/absolute")

        with zipfile.ZipFile(TESTFN2, "r", zipfile.ZIP_STORED) as zipfp:
            self.assertEqual(zipfp.namelist(), ["absolute"])

    def test_append_to_zip_file(self):
        """Test appending to an existing zipfile."""
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            zipfp.write(TESTFN, TESTFN)

        with zipfile.ZipFile(TESTFN2, "a", zipfile.ZIP_STORED) as zipfp:
            zipfp.writestr("strfile", self.data)
            self.assertEqual(zipfp.namelist(), [TESTFN, "strfile"])

    def test_append_to_non_zip_file(self):
        """Test appending to an existing file that is not a zipfile."""
        # NOTE: this test fails if len(d) < 22 because of the first
        # line "fpin.seek(-22, 2)" in _EndRecData
        data = b'I am not a ZipFile!'*10
        with open(TESTFN2, 'wb') as f:
            f.write(data)

        with zipfile.ZipFile(TESTFN2, "a", zipfile.ZIP_STORED) as zipfp:
            zipfp.write(TESTFN, TESTFN)

        with open(TESTFN2, 'rb') as f:
            f.seek(len(data))
            with zipfile.ZipFile(f, "r") as zipfp:
                self.assertEqual(zipfp.namelist(), [TESTFN])

    def test_ignores_newline_at_end(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            zipfp.write(TESTFN, TESTFN)
        with open(TESTFN2, 'a') as f:
            f.write("\r\n\00\00\00")
        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            self.assertIsInstance(zipfp, zipfile.ZipFile)

    def test_ignores_stuff_appended_past_comments(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            zipfp.comment = b"this is a comment"
            zipfp.write(TESTFN, TESTFN)
        with open(TESTFN2, 'a') as f:
            f.write("abcdef\r\n")
        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            self.assertIsInstance(zipfp, zipfile.ZipFile)
            self.assertEqual(zipfp.comment, b"this is a comment")

    def test_write_default_name(self):
        """Check that calling ZipFile.write without arcname specified
        produces the expected result."""
        with zipfile.ZipFile(TESTFN2, "w") as zipfp:
            zipfp.write(TESTFN)
            with open(TESTFN, "rb") as f:
                self.assertEqual(zipfp.read(TESTFN), f.read())

    def test_write_to_readonly(self):
        """Check that trying to call write() on a readonly ZipFile object
        raises a RuntimeError."""
        with zipfile.ZipFile(TESTFN2, mode="w") as zipfp:
            zipfp.writestr("somefile.txt", "bogus")

        with zipfile.ZipFile(TESTFN2, mode="r") as zipfp:
            self.assertRaises(RuntimeError, zipfp.write, TESTFN)

    def test_add_file_before_1980(self):
        # Set atime and mtime to 1970-01-01
        os.utime(TESTFN, (0, 0))
        with zipfile.ZipFile(TESTFN2, "w") as zipfp:
            self.assertRaises(ValueError, zipfp.write, TESTFN)

@requires_zlib
class DeflateTestsWithSourceFile(AbstractTestsWithSourceFile,
                                 unittest.TestCase):
    compression = zipfile.ZIP_DEFLATED

    def test_per_file_compression(self):
        """Check that files within a Zip archive can have different
        compression options."""
        with zipfile.ZipFile(TESTFN2, "w") as zipfp:
            zipfp.write(TESTFN, 'storeme', zipfile.ZIP_STORED)
            zipfp.write(TESTFN, 'deflateme', zipfile.ZIP_DEFLATED)
            sinfo = zipfp.getinfo('storeme')
            dinfo = zipfp.getinfo('deflateme')
            self.assertEqual(sinfo.compress_type, zipfile.ZIP_STORED)
            self.assertEqual(dinfo.compress_type, zipfile.ZIP_DEFLATED)

@requires_bz2
class Bzip2TestsWithSourceFile(AbstractTestsWithSourceFile,
                               unittest.TestCase):
    compression = zipfile.ZIP_BZIP2

@requires_lzma
class LzmaTestsWithSourceFile(AbstractTestsWithSourceFile,
                              unittest.TestCase):
    compression = zipfile.ZIP_LZMA


class AbstractTestZip64InSmallFiles:
    # These tests test the ZIP64 functionality without using large files,
    # see test_zipfile64 for proper tests.

    @classmethod
    def setUpClass(cls):
        line_gen = (bytes("Test of zipfile line %d." % i, "ascii")
                    for i in range(0, FIXEDTEST_SIZE))
        cls.data = b'\n'.join(line_gen)

    def setUp(self):
        self._limit = zipfile.ZIP64_LIMIT
        zipfile.ZIP64_LIMIT = 5

        # Make a source file with some lines
        with open(TESTFN, "wb") as fp:
            fp.write(self.data)

    def zip_test(self, f, compression):
        # Create the ZIP archive
        with zipfile.ZipFile(f, "w", compression, allowZip64=True) as zipfp:
            zipfp.write(TESTFN, "another.name")
            zipfp.write(TESTFN, TESTFN)
            zipfp.writestr("strfile", self.data)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r", compression) as zipfp:
            self.assertEqual(zipfp.read(TESTFN), self.data)
            self.assertEqual(zipfp.read("another.name"), self.data)
            self.assertEqual(zipfp.read("strfile"), self.data)

            # Print the ZIP directory
            fp = io.StringIO()
            zipfp.printdir(fp)

            directory = fp.getvalue()
            lines = directory.splitlines()
            self.assertEqual(len(lines), 4) # Number of files + header

            self.assertIn('File Name', lines[0])
            self.assertIn('Modified', lines[0])
            self.assertIn('Size', lines[0])

            fn, date, time_, size = lines[1].split()
            self.assertEqual(fn, 'another.name')
            self.assertTrue(time.strptime(date, '%Y-%m-%d'))
            self.assertTrue(time.strptime(time_, '%H:%M:%S'))
            self.assertEqual(size, str(len(self.data)))

            # Check the namelist
            names = zipfp.namelist()
            self.assertEqual(len(names), 3)
            self.assertIn(TESTFN, names)
            self.assertIn("another.name", names)
            self.assertIn("strfile", names)

            # Check infolist
            infos = zipfp.infolist()
            names = [i.filename for i in infos]
            self.assertEqual(len(names), 3)
            self.assertIn(TESTFN, names)
            self.assertIn("another.name", names)
            self.assertIn("strfile", names)
            for i in infos:
                self.assertEqual(i.file_size, len(self.data))

            # check getinfo
            for nm in (TESTFN, "another.name", "strfile"):
                info = zipfp.getinfo(nm)
                self.assertEqual(info.filename, nm)
                self.assertEqual(info.file_size, len(self.data))

            # Check that testzip doesn't raise an exception
            zipfp.testzip()

    def test_basic(self):
        for f in get_files(self):
            self.zip_test(f, self.compression)

    def tearDown(self):
        zipfile.ZIP64_LIMIT = self._limit
        unlink(TESTFN)
        unlink(TESTFN2)


class StoredTestZip64InSmallFiles(AbstractTestZip64InSmallFiles,
                                  unittest.TestCase):
    compression = zipfile.ZIP_STORED

    def large_file_exception_test(self, f, compression):
        with zipfile.ZipFile(f, "w", compression) as zipfp:
            self.assertRaises(zipfile.LargeZipFile,
                              zipfp.write, TESTFN, "another.name")

    def large_file_exception_test2(self, f, compression):
        with zipfile.ZipFile(f, "w", compression) as zipfp:
            self.assertRaises(zipfile.LargeZipFile,
                              zipfp.writestr, "another.name", self.data)

    def test_large_file_exception(self):
        for f in get_files(self):
            self.large_file_exception_test(f, zipfile.ZIP_STORED)
            self.large_file_exception_test2(f, zipfile.ZIP_STORED)

    def test_absolute_arcnames(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED,
                             allowZip64=True) as zipfp:
            zipfp.write(TESTFN, "/absolute")

        with zipfile.ZipFile(TESTFN2, "r", zipfile.ZIP_STORED) as zipfp:
            self.assertEqual(zipfp.namelist(), ["absolute"])

@requires_zlib
class DeflateTestZip64InSmallFiles(AbstractTestZip64InSmallFiles,
                                   unittest.TestCase):
    compression = zipfile.ZIP_DEFLATED

@requires_bz2
class Bzip2TestZip64InSmallFiles(AbstractTestZip64InSmallFiles,
                                 unittest.TestCase):
    compression = zipfile.ZIP_BZIP2

@requires_lzma
class LzmaTestZip64InSmallFiles(AbstractTestZip64InSmallFiles,
                                unittest.TestCase):
    compression = zipfile.ZIP_LZMA


class PyZipFileTests(unittest.TestCase):
    def assertCompiledIn(self, name, namelist):
        if name + 'o' not in namelist:
            self.assertIn(name + 'c', namelist)

    def test_write_pyfile(self):
        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
            fn = __file__
            if fn.endswith('.pyc') or fn.endswith('.pyo'):
                path_split = fn.split(os.sep)
                if os.altsep is not None:
                    path_split.extend(fn.split(os.altsep))
                if '__pycache__' in path_split:
                    fn = imp.source_from_cache(fn)
                else:
                    fn = fn[:-1]

            zipfp.writepy(fn)

            bn = os.path.basename(fn)
            self.assertNotIn(bn, zipfp.namelist())
            self.assertCompiledIn(bn, zipfp.namelist())

        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
            fn = __file__
            if fn.endswith(('.pyc', '.pyo')):
                fn = fn[:-1]

            zipfp.writepy(fn, "testpackage")

            bn = "%s/%s" % ("testpackage", os.path.basename(fn))
            self.assertNotIn(bn, zipfp.namelist())
            self.assertCompiledIn(bn, zipfp.namelist())

    def test_write_python_package(self):
        import email
        packagedir = os.path.dirname(email.__file__)

        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
            zipfp.writepy(packagedir)

            # Check for a couple of modules at different levels of the
            # hierarchy
            names = zipfp.namelist()
            self.assertCompiledIn('email/__init__.py', names)
            self.assertCompiledIn('email/mime/text.py', names)

    def test_write_with_optimization(self):
        import email
        packagedir = os.path.dirname(email.__file__)
        # use .pyc if running test in optimization mode,
        # use .pyo if running test in debug mode
        optlevel = 1 if __debug__ else 0
        ext = '.pyo' if optlevel == 1 else '.pyc'

        with TemporaryFile() as t, \
                 zipfile.PyZipFile(t, "w", optimize=optlevel) as zipfp:
            zipfp.writepy(packagedir)

            names = zipfp.namelist()
            self.assertIn('email/__init__' + ext, names)
            self.assertIn('email/mime/text' + ext, names)

    def test_write_python_directory(self):
        os.mkdir(TESTFN2)
        try:
            with open(os.path.join(TESTFN2, "mod1.py"), "w") as fp:
                fp.write("print(42)\n")

            with open(os.path.join(TESTFN2, "mod2.py"), "w") as fp:
                fp.write("print(42 * 42)\n")

            with open(os.path.join(TESTFN2, "mod2.txt"), "w") as fp:
                fp.write("bla bla bla\n")

            with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
                zipfp.writepy(TESTFN2)

                names = zipfp.namelist()
                self.assertCompiledIn('mod1.py', names)
                self.assertCompiledIn('mod2.py', names)
                self.assertNotIn('mod2.txt', names)

        finally:
            shutil.rmtree(TESTFN2)

    def test_write_non_pyfile(self):
        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
            with open(TESTFN, 'w') as f:
                f.write('most definitely not a python file')
            self.assertRaises(RuntimeError, zipfp.writepy, TESTFN)
            os.remove(TESTFN)

    def test_write_pyfile_bad_syntax(self):
        os.mkdir(TESTFN2)
        try:
            with open(os.path.join(TESTFN2, "mod1.py"), "w") as fp:
                fp.write("Bad syntax in python file\n")

            with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
                # syntax errors are printed to stdout
                with captured_stdout() as s:
                    zipfp.writepy(os.path.join(TESTFN2, "mod1.py"))

                self.assertIn("SyntaxError", s.getvalue())

                # as it will not have compiled the python file, it will
                # include the .py file not .pyc or .pyo
                names = zipfp.namelist()
                self.assertIn('mod1.py', names)
                self.assertNotIn('mod1.pyc', names)
                self.assertNotIn('mod1.pyo', names)

        finally:
            shutil.rmtree(TESTFN2)


class ExtractTests(unittest.TestCase):
    def test_extract(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            for fpath, fdata in SMALL_TEST_DATA:
                zipfp.writestr(fpath, fdata)

        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            for fpath, fdata in SMALL_TEST_DATA:
                writtenfile = zipfp.extract(fpath)

                # make sure it was written to the right place
                correctfile = os.path.join(os.getcwd(), fpath)
                correctfile = os.path.normpath(correctfile)

                self.assertEqual(writtenfile, correctfile)

                # make sure correct data is in correct file
                with open(writtenfile, "rb") as f:
                    self.assertEqual(fdata.encode(), f.read())

                os.remove(writtenfile)

        # remove the test file subdirectories
        shutil.rmtree(os.path.join(os.getcwd(), 'ziptest2dir'))

    def test_extract_all(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            for fpath, fdata in SMALL_TEST_DATA:
                zipfp.writestr(fpath, fdata)

        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            zipfp.extractall()
            for fpath, fdata in SMALL_TEST_DATA:
                outfile = os.path.join(os.getcwd(), fpath)

                with open(outfile, "rb") as f:
                    self.assertEqual(fdata.encode(), f.read())

                os.remove(outfile)

        # remove the test file subdirectories
        shutil.rmtree(os.path.join(os.getcwd(), 'ziptest2dir'))

    def check_file(self, filename, content):
        self.assertTrue(os.path.isfile(filename))
        with open(filename, 'rb') as f:
            self.assertEqual(f.read(), content)

    def test_sanitize_windows_name(self):
        san = zipfile.ZipFile._sanitize_windows_name
        # Passing pathsep in allows this test to work regardless of platform.
        self.assertEqual(san(r',,?,C:,foo,bar/z', ','), r'_,C_,foo,bar/z')
        self.assertEqual(san(r'a\b,c<d>e|f"g?h*i', ','), r'a\b,c_d_e_f_g_h_i')
        self.assertEqual(san('../../foo../../ba..r', '/'), r'foo/ba..r')

    def test_extract_hackers_arcnames_common_cases(self):
        common_hacknames = [
            ('../foo/bar', 'foo/bar'),
            ('foo/../bar', 'foo/bar'),
            ('foo/../../bar', 'foo/bar'),
            ('foo/bar/..', 'foo/bar'),
            ('./../foo/bar', 'foo/bar'),
            ('/foo/bar', 'foo/bar'),
            ('/foo/../bar', 'foo/bar'),
            ('/foo/../../bar', 'foo/bar'),
        ]
        self._test_extract_hackers_arcnames(common_hacknames)

    @unittest.skipIf(os.path.sep != '\\', 'Requires \\ as path separator.')
    def test_extract_hackers_arcnames_windows_only(self):
        """Test combination of path fixing and windows name sanitization."""
        windows_hacknames = [
                (r'..\foo\bar', 'foo/bar'),
                (r'..\/foo\/bar', 'foo/bar'),
                (r'foo/\..\/bar', 'foo/bar'),
                (r'foo\/../\bar', 'foo/bar'),
                (r'C:foo/bar', 'foo/bar'),
                (r'C:/foo/bar', 'foo/bar'),
                (r'C://foo/bar', 'foo/bar'),
                (r'C:\foo\bar', 'foo/bar'),
                (r'//conky/mountpoint/foo/bar', 'foo/bar'),
                (r'\\conky\mountpoint\foo\bar', 'foo/bar'),
                (r'///conky/mountpoint/foo/bar', 'conky/mountpoint/foo/bar'),
                (r'\\\conky\mountpoint\foo\bar', 'conky/mountpoint/foo/bar'),
                (r'//conky//mountpoint/foo/bar', 'conky/mountpoint/foo/bar'),
                (r'\\conky\\mountpoint\foo\bar', 'conky/mountpoint/foo/bar'),
                (r'//?/C:/foo/bar', 'foo/bar'),
                (r'\\?\C:\foo\bar', 'foo/bar'),
                (r'C:/../C:/foo/bar', 'C_/foo/bar'),
                (r'a:b\c<d>e|f"g?h*i', 'b/c_d_e_f_g_h_i'),
                ('../../foo../../ba..r', 'foo/ba..r'),
        ]
        self._test_extract_hackers_arcnames(windows_hacknames)

    @unittest.skipIf(os.path.sep != '/', r'Requires / as path separator.')
    def test_extract_hackers_arcnames_posix_only(self):
        posix_hacknames = [
            ('//foo/bar', 'foo/bar'),
            ('../../foo../../ba..r', 'foo../ba..r'),
            (r'foo/..\bar', r'foo/..\bar'),
        ]
        self._test_extract_hackers_arcnames(posix_hacknames)

    def _test_extract_hackers_arcnames(self, hacknames):
        for arcname, fixedname in hacknames:
            content = b'foobar' + arcname.encode()
            with zipfile.ZipFile(TESTFN2, 'w', zipfile.ZIP_STORED) as zipfp:
                zinfo = zipfile.ZipInfo()
                # preserve backslashes
                zinfo.filename = arcname
                zinfo.external_attr = 0o600 << 16
                zipfp.writestr(zinfo, content)

            arcname = arcname.replace(os.sep, "/")
            targetpath = os.path.join('target', 'subdir', 'subsub')
            correctfile = os.path.join(targetpath, *fixedname.split('/'))

            with zipfile.ZipFile(TESTFN2, 'r') as zipfp:
                writtenfile = zipfp.extract(arcname, targetpath)
                self.assertEqual(writtenfile, correctfile,
                                 msg='extract %r: %r != %r' %
                                 (arcname, writtenfile, correctfile))
            self.check_file(correctfile, content)
            shutil.rmtree('target')

            with zipfile.ZipFile(TESTFN2, 'r') as zipfp:
                zipfp.extractall(targetpath)
            self.check_file(correctfile, content)
            shutil.rmtree('target')

            correctfile = os.path.join(os.getcwd(), *fixedname.split('/'))

            with zipfile.ZipFile(TESTFN2, 'r') as zipfp:
                writtenfile = zipfp.extract(arcname)
                self.assertEqual(writtenfile, correctfile,
                                 msg="extract %r" % arcname)
            self.check_file(correctfile, content)
            shutil.rmtree(fixedname.split('/')[0])

            with zipfile.ZipFile(TESTFN2, 'r') as zipfp:
                zipfp.extractall()
            self.check_file(correctfile, content)
            shutil.rmtree(fixedname.split('/')[0])

            os.remove(TESTFN2)


class OtherTests(unittest.TestCase):
    def test_open_via_zip_info(self):
        # Create the ZIP archive
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            zipfp.writestr("name", "foo")
            zipfp.writestr("name", "bar")

        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            infos = zipfp.infolist()
            data = b""
            for info in infos:
                with zipfp.open(info) as zipopen:
                    data += zipopen.read()
            self.assertIn(data, {b"foobar", b"barfoo"})
            data = b""
            for info in infos:
                data += zipfp.read(info)
            self.assertIn(data, {b"foobar", b"barfoo"})

    def test_universal_readaheads(self):
        f = io.BytesIO()

        data = b'a\r\n' * 16 * 1024
        with zipfile.ZipFile(f, 'w', zipfile.ZIP_STORED) as zipfp:
            zipfp.writestr(TESTFN, data)

        data2 = b''
        with zipfile.ZipFile(f, 'r') as zipfp, \
             zipfp.open(TESTFN, 'rU') as zipopen:
            for line in zipopen:
                data2 += line

        self.assertEqual(data, data2.replace(b'\n', b'\r\n'))

    def test_writestr_extended_local_header_issue1202(self):
        with zipfile.ZipFile(TESTFN2, 'w') as orig_zip:
            for data in 'abcdefghijklmnop':
                zinfo = zipfile.ZipInfo(data)
                zinfo.flag_bits |= 0x08  # Include an extended local header.
                orig_zip.writestr(zinfo, data)

    def test_close(self):
        """Check that the zipfile is closed after the 'with' block."""
        with zipfile.ZipFile(TESTFN2, "w") as zipfp:
            for fpath, fdata in SMALL_TEST_DATA:
                zipfp.writestr(fpath, fdata)
                self.assertIsNotNone(zipfp.fp, 'zipfp is not open')
        self.assertIsNone(zipfp.fp, 'zipfp is not closed')

        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            self.assertIsNotNone(zipfp.fp, 'zipfp is not open')
        self.assertIsNone(zipfp.fp, 'zipfp is not closed')

    def test_close_on_exception(self):
        """Check that the zipfile is closed if an exception is raised in the
        'with' block."""
        with zipfile.ZipFile(TESTFN2, "w") as zipfp:
            for fpath, fdata in SMALL_TEST_DATA:
                zipfp.writestr(fpath, fdata)

        try:
            with zipfile.ZipFile(TESTFN2, "r") as zipfp2:
                raise zipfile.BadZipFile()
        except zipfile.BadZipFile:
            self.assertIsNone(zipfp2.fp, 'zipfp is not closed')

    def test_unsupported_version(self):
        # File has an extract_version of 120
        data = (b'PK\x03\x04x\x00\x00\x00\x00\x00!p\xa1@\x00\x00\x00\x00\x00\x00'
        b'\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00xPK\x01\x02x\x03x\x00\x00\x00\x00'
        b'\x00!p\xa1@\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00'
        b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x01\x00\x00\x00\x00xPK\x05\x06'
        b'\x00\x00\x00\x00\x01\x00\x01\x00/\x00\x00\x00\x1f\x00\x00\x00\x00\x00')

        self.assertRaises(NotImplementedError, zipfile.ZipFile,
                          io.BytesIO(data), 'r')

    @requires_zlib
    def test_read_unicode_filenames(self):
        # bug #10801
        fname = findfile('zip_cp437_header.zip')
        with zipfile.ZipFile(fname) as zipfp:
            for name in zipfp.namelist():
                zipfp.open(name).close()

    def test_write_unicode_filenames(self):
        with zipfile.ZipFile(TESTFN, "w") as zf:
            zf.writestr("foo.txt", "Test for unicode filename")
            zf.writestr("\xf6.txt", "Test for unicode filename")
            self.assertIsInstance(zf.infolist()[0].filename, str)

        with zipfile.ZipFile(TESTFN, "r") as zf:
            self.assertEqual(zf.filelist[0].filename, "foo.txt")
            self.assertEqual(zf.filelist[1].filename, "\xf6.txt")

    def test_create_non_existent_file_for_append(self):
        if os.path.exists(TESTFN):
            os.unlink(TESTFN)

        filename = 'testfile.txt'
        content = b'hello, world. this is some content.'

        try:
            with zipfile.ZipFile(TESTFN, 'a') as zf:
                zf.writestr(filename, content)
        except IOError:
            self.fail('Could not append data to a non-existent zip file.')

        self.assertTrue(os.path.exists(TESTFN))

        with zipfile.ZipFile(TESTFN, 'r') as zf:
            self.assertEqual(zf.read(filename), content)

    def test_close_erroneous_file(self):
        # This test checks that the ZipFile constructor closes the file object
        # it opens if there's an error in the file.  If it doesn't, the
        # traceback holds a reference to the ZipFile object and, indirectly,
        # the file object.
        # On Windows, this causes the os.unlink() call to fail because the
        # underlying file is still open.  This is SF bug #412214.
        #
        with open(TESTFN, "w") as fp:
            fp.write("this is not a legal zip file\n")
        try:
            zf = zipfile.ZipFile(TESTFN)
        except zipfile.BadZipFile:
            pass

    def test_is_zip_erroneous_file(self):
        """Check that is_zipfile() correctly identifies non-zip files."""
        # - passing a filename
        with open(TESTFN, "w") as fp:
            fp.write("this is not a legal zip file\n")
        self.assertFalse(zipfile.is_zipfile(TESTFN))
        # - passing a file object
        with open(TESTFN, "rb") as fp:
            self.assertFalse(zipfile.is_zipfile(fp))
        # - passing a file-like object
        fp = io.BytesIO()
        fp.write(b"this is not a legal zip file\n")
        self.assertFalse(zipfile.is_zipfile(fp))
        fp.seek(0, 0)
        self.assertFalse(zipfile.is_zipfile(fp))

    def test_damaged_zipfile(self):
        """Check that zipfiles with missing bytes at the end raise BadZipFile."""
        # - Create a valid zip file
        fp = io.BytesIO()
        with zipfile.ZipFile(fp, mode="w") as zipf:
            zipf.writestr("foo.txt", b"O, for a Muse of Fire!")
        zipfiledata = fp.getvalue()

        # - Now create copies of it missing the last N bytes and make sure
        #   a BadZipFile exception is raised when we try to open it
        for N in range(len(zipfiledata)):
            fp = io.BytesIO(zipfiledata[:N])
            self.assertRaises(zipfile.BadZipFile, zipfile.ZipFile, fp)

    def test_is_zip_valid_file(self):
        """Check that is_zipfile() correctly identifies zip files."""
        # - passing a filename
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            zipf.writestr("foo.txt", b"O, for a Muse of Fire!")

        self.assertTrue(zipfile.is_zipfile(TESTFN))
        # - passing a file object
        with open(TESTFN, "rb") as fp:
            self.assertTrue(zipfile.is_zipfile(fp))
            fp.seek(0, 0)
            zip_contents = fp.read()
        # - passing a file-like object
        fp = io.BytesIO()
        fp.write(zip_contents)
        self.assertTrue(zipfile.is_zipfile(fp))
        fp.seek(0, 0)
        self.assertTrue(zipfile.is_zipfile(fp))

    def test_non_existent_file_raises_IOError(self):
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
        self.assertRaises(zipfile.BadZipFile, zipfile.ZipFile, TESTFN)

        with open(TESTFN, 'w') as fp:
            fp.write("short file")
        self.assertRaises(zipfile.BadZipFile, zipfile.ZipFile, TESTFN)

    def test_closed_zip_raises_RuntimeError(self):
        """Verify that testzip() doesn't swallow inappropriate exceptions."""
        data = io.BytesIO()
        with zipfile.ZipFile(data, mode="w") as zipf:
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")

        # This is correct; calling .read on a closed ZipFile should raise
        # a RuntimeError, and so should calling .testzip.  An earlier
        # version of .testzip would swallow this exception (and any other)
        # and report that the first file in the archive was corrupt.
        self.assertRaises(RuntimeError, zipf.read, "foo.txt")
        self.assertRaises(RuntimeError, zipf.open, "foo.txt")
        self.assertRaises(RuntimeError, zipf.testzip)
        self.assertRaises(RuntimeError, zipf.writestr, "bogus.txt", "bogus")
        with open(TESTFN, 'w') as f:
            f.write('zipfile test data')
        self.assertRaises(RuntimeError, zipf.write, TESTFN)

    def test_bad_constructor_mode(self):
        """Check that bad modes passed to ZipFile constructor are caught."""
        self.assertRaises(RuntimeError, zipfile.ZipFile, TESTFN, "q")

    def test_bad_open_mode(self):
        """Check that bad modes passed to ZipFile.open are caught."""
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")

        with zipfile.ZipFile(TESTFN, mode="r") as zipf:
        # read the data to make sure the file is there
            zipf.read("foo.txt")
            self.assertRaises(RuntimeError, zipf.open, "foo.txt", "q")

    def test_read0(self):
        """Check that calling read(0) on a ZipExtFile object returns an empty
        string and doesn't advance file pointer."""
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")
            # read the data to make sure the file is there
            with zipf.open("foo.txt") as f:
                for i in range(FIXEDTEST_SIZE):
                    self.assertEqual(f.read(0), b'')

                self.assertEqual(f.read(), b"O, for a Muse of Fire!")

    def test_open_non_existent_item(self):
        """Check that attempting to call open() for an item that doesn't
        exist in the archive raises a RuntimeError."""
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            self.assertRaises(KeyError, zipf.open, "foo.txt", "r")

    def test_bad_compression_mode(self):
        """Check that bad compression methods passed to ZipFile.open are
        caught."""
        self.assertRaises(RuntimeError, zipfile.ZipFile, TESTFN, "w", -1)

    def test_unsupported_compression(self):
        # data is declared as shrunk, but actually deflated
        data = (b'PK\x03\x04.\x00\x00\x00\x01\x00\xe4C\xa1@\x00\x00\x00'
        b'\x00\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00x\x03\x00PK\x01'
        b'\x02.\x03.\x00\x00\x00\x01\x00\xe4C\xa1@\x00\x00\x00\x00\x02\x00\x00'
        b'\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        b'\x80\x01\x00\x00\x00\x00xPK\x05\x06\x00\x00\x00\x00\x01\x00\x01\x00'
        b'/\x00\x00\x00!\x00\x00\x00\x00\x00')
        with zipfile.ZipFile(io.BytesIO(data), 'r') as zipf:
            self.assertRaises(NotImplementedError, zipf.open, 'x')

    def test_null_byte_in_filename(self):
        """Check that a filename containing a null byte is properly
        terminated."""
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            zipf.writestr("foo.txt\x00qqq", b"O, for a Muse of Fire!")
            self.assertEqual(zipf.namelist(), ['foo.txt'])

    def test_struct_sizes(self):
        """Check that ZIP internal structure sizes are calculated correctly."""
        self.assertEqual(zipfile.sizeEndCentDir, 22)
        self.assertEqual(zipfile.sizeCentralDir, 46)
        self.assertEqual(zipfile.sizeEndCentDir64, 56)
        self.assertEqual(zipfile.sizeEndCentDir64Locator, 20)

    def test_comments(self):
        """Check that comments on the archive are handled properly."""

        # check default comment is empty
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            self.assertEqual(zipf.comment, b'')
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")

        with zipfile.ZipFile(TESTFN, mode="r") as zipfr:
            self.assertEqual(zipfr.comment, b'')

        # check a simple short comment
        comment = b'Bravely taking to his feet, he beat a very brave retreat.'
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            zipf.comment = comment
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        with zipfile.ZipFile(TESTFN, mode="r") as zipfr:
            self.assertEqual(zipf.comment, comment)

        # check a comment of max length
        comment2 = ''.join(['%d' % (i**3 % 10) for i in range((1 << 16)-1)])
        comment2 = comment2.encode("ascii")
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            zipf.comment = comment2
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")

        with zipfile.ZipFile(TESTFN, mode="r") as zipfr:
            self.assertEqual(zipfr.comment, comment2)

        # check a comment that is too long is truncated
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            zipf.comment = comment2 + b'oops'
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        with zipfile.ZipFile(TESTFN, mode="r") as zipfr:
            self.assertEqual(zipfr.comment, comment2)

        # check that comments are correctly modified in append mode
        with zipfile.ZipFile(TESTFN,mode="w") as zipf:
            zipf.comment = b"original comment"
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        with zipfile.ZipFile(TESTFN,mode="a") as zipf:
            zipf.comment = b"an updated comment"
        with zipfile.ZipFile(TESTFN,mode="r") as zipf:
            self.assertEqual(zipf.comment, b"an updated comment")

        # check that comments are correctly shortened in append mode
        with zipfile.ZipFile(TESTFN,mode="w") as zipf:
            zipf.comment = b"original comment that's longer"
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        with zipfile.ZipFile(TESTFN,mode="a") as zipf:
            zipf.comment = b"shorter comment"
        with zipfile.ZipFile(TESTFN,mode="r") as zipf:
            self.assertEqual(zipf.comment, b"shorter comment")

    def test_unicode_comment(self):
        with zipfile.ZipFile(TESTFN, "w", zipfile.ZIP_STORED) as zipf:
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")
            with self.assertRaises(TypeError):
                zipf.comment = "this is an error"

    def test_change_comment_in_empty_archive(self):
        with zipfile.ZipFile(TESTFN, "a", zipfile.ZIP_STORED) as zipf:
            self.assertFalse(zipf.filelist)
            zipf.comment = b"this is a comment"
        with zipfile.ZipFile(TESTFN, "r") as zipf:
            self.assertEqual(zipf.comment, b"this is a comment")

    def test_change_comment_in_nonempty_archive(self):
        with zipfile.ZipFile(TESTFN, "w", zipfile.ZIP_STORED) as zipf:
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        with zipfile.ZipFile(TESTFN, "a", zipfile.ZIP_STORED) as zipf:
            self.assertTrue(zipf.filelist)
            zipf.comment = b"this is a comment"
        with zipfile.ZipFile(TESTFN, "r") as zipf:
            self.assertEqual(zipf.comment, b"this is a comment")

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
        # raises a BadZipFile exception (rather than the previously unhelpful
        # IOError)
        f = open(TESTFN, 'w')
        f.close()
        self.assertRaises(zipfile.BadZipFile, zipfile.ZipFile, TESTFN, 'r')

    def test_create_zipinfo_before_1980(self):
        self.assertRaises(ValueError,
                          zipfile.ZipInfo, 'seventies', (1979, 1, 1, 0, 0, 0))

    def tearDown(self):
        unlink(TESTFN)
        unlink(TESTFN2)


class AbstractBadCrcTests:
    def test_testzip_with_bad_crc(self):
        """Tests that files with bad CRCs return their name from testzip."""
        zipdata = self.zip_with_bad_crc

        with zipfile.ZipFile(io.BytesIO(zipdata), mode="r") as zipf:
            # testzip returns the name of the first corrupt file, or None
            self.assertEqual('afile', zipf.testzip())

    def test_read_with_bad_crc(self):
        """Tests that files with bad CRCs raise a BadZipFile exception when read."""
        zipdata = self.zip_with_bad_crc

        # Using ZipFile.read()
        with zipfile.ZipFile(io.BytesIO(zipdata), mode="r") as zipf:
            self.assertRaises(zipfile.BadZipFile, zipf.read, 'afile')

        # Using ZipExtFile.read()
        with zipfile.ZipFile(io.BytesIO(zipdata), mode="r") as zipf:
            with zipf.open('afile', 'r') as corrupt_file:
                self.assertRaises(zipfile.BadZipFile, corrupt_file.read)

        # Same with small reads (in order to exercise the buffering logic)
        with zipfile.ZipFile(io.BytesIO(zipdata), mode="r") as zipf:
            with zipf.open('afile', 'r') as corrupt_file:
                corrupt_file.MIN_READ_SIZE = 2
                with self.assertRaises(zipfile.BadZipFile):
                    while corrupt_file.read(2):
                        pass


class StoredBadCrcTests(AbstractBadCrcTests, unittest.TestCase):
    compression = zipfile.ZIP_STORED
    zip_with_bad_crc = (
            b'PK\003\004\024\0\0\0\0\0 \213\212;:r'
            b'\253\377\f\0\0\0\f\0\0\0\005\0\0\000af'
            b'ilehello,AworldP'
            b'K\001\002\024\003\024\0\0\0\0\0 \213\212;:'
            b'r\253\377\f\0\0\0\f\0\0\0\005\0\0\0\0'
            b'\0\0\0\0\0\0\0\200\001\0\0\0\000afi'
            b'lePK\005\006\0\0\0\0\001\0\001\0003\000'
            b'\0\0/\0\0\0\0\0')

@requires_zlib
class DeflateBadCrcTests(AbstractBadCrcTests, unittest.TestCase):
    compression = zipfile.ZIP_DEFLATED
    zip_with_bad_crc = (
            b'PK\x03\x04\x14\x00\x00\x00\x08\x00n}\x0c=FA'
            b'KE\x10\x00\x00\x00n\x00\x00\x00\x05\x00\x00\x00af'
            b'ile\xcbH\xcd\xc9\xc9W(\xcf/\xcaI\xc9\xa0'
            b'=\x13\x00PK\x01\x02\x14\x03\x14\x00\x00\x00\x08\x00n'
            b'}\x0c=FAKE\x10\x00\x00\x00n\x00\x00\x00\x05'
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x01\x00\x00\x00'
            b'\x00afilePK\x05\x06\x00\x00\x00\x00\x01\x00'
            b'\x01\x003\x00\x00\x003\x00\x00\x00\x00\x00')

@requires_bz2
class Bzip2BadCrcTests(AbstractBadCrcTests, unittest.TestCase):
    compression = zipfile.ZIP_BZIP2
    zip_with_bad_crc = (
            b'PK\x03\x04\x14\x03\x00\x00\x0c\x00nu\x0c=FA'
            b'KE8\x00\x00\x00n\x00\x00\x00\x05\x00\x00\x00af'
            b'ileBZh91AY&SY\xd4\xa8\xca'
            b'\x7f\x00\x00\x0f\x11\x80@\x00\x06D\x90\x80 \x00 \xa5'
            b'P\xd9!\x03\x03\x13\x13\x13\x89\xa9\xa9\xc2u5:\x9f'
            b'\x8b\xb9"\x9c(HjTe?\x80PK\x01\x02\x14'
            b'\x03\x14\x03\x00\x00\x0c\x00nu\x0c=FAKE8'
            b'\x00\x00\x00n\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00'
            b'\x00 \x80\x80\x81\x00\x00\x00\x00afilePK'
            b'\x05\x06\x00\x00\x00\x00\x01\x00\x01\x003\x00\x00\x00[\x00'
            b'\x00\x00\x00\x00')

@requires_lzma
class LzmaBadCrcTests(AbstractBadCrcTests, unittest.TestCase):
    compression = zipfile.ZIP_LZMA
    zip_with_bad_crc = (
            b'PK\x03\x04\x14\x03\x00\x00\x0e\x00nu\x0c=FA'
            b'KE\x1b\x00\x00\x00n\x00\x00\x00\x05\x00\x00\x00af'
            b'ile\t\x04\x05\x00]\x00\x00\x00\x04\x004\x19I'
            b'\xee\x8d\xe9\x17\x89:3`\tq!.8\x00PK'
            b'\x01\x02\x14\x03\x14\x03\x00\x00\x0e\x00nu\x0c=FA'
            b'KE\x1b\x00\x00\x00n\x00\x00\x00\x05\x00\x00\x00\x00\x00'
            b'\x00\x00\x00\x00 \x80\x80\x81\x00\x00\x00\x00afil'
            b'ePK\x05\x06\x00\x00\x00\x00\x01\x00\x01\x003\x00\x00'
            b'\x00>\x00\x00\x00\x00\x00')


class DecryptionTests(unittest.TestCase):
    """Check that ZIP decryption works. Since the library does not
    support encryption at the moment, we use a pre-generated encrypted
    ZIP file."""

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
        with open(TESTFN, "wb") as fp:
            fp.write(self.data)
        self.zip = zipfile.ZipFile(TESTFN, "r")
        with open(TESTFN2, "wb") as fp:
            fp.write(self.data2)
        self.zip2 = zipfile.ZipFile(TESTFN2, "r")

    def tearDown(self):
        self.zip.close()
        os.unlink(TESTFN)
        self.zip2.close()
        os.unlink(TESTFN2)

    def test_no_password(self):
        # Reading the encrypted file without password
        # must generate a RunTime exception
        self.assertRaises(RuntimeError, self.zip.read, "test.txt")
        self.assertRaises(RuntimeError, self.zip2.read, "zero")

    def test_bad_password(self):
        self.zip.setpassword(b"perl")
        self.assertRaises(RuntimeError, self.zip.read, "test.txt")
        self.zip2.setpassword(b"perl")
        self.assertRaises(RuntimeError, self.zip2.read, "zero")

    @requires_zlib
    def test_good_password(self):
        self.zip.setpassword(b"python")
        self.assertEqual(self.zip.read("test.txt"), self.plain)
        self.zip2.setpassword(b"12345")
        self.assertEqual(self.zip2.read("zero"), self.plain2)

    def test_unicode_password(self):
        self.assertRaises(TypeError, self.zip.setpassword, "unicode")
        self.assertRaises(TypeError, self.zip.read, "test.txt", "python")
        self.assertRaises(TypeError, self.zip.open, "test.txt", pwd="python")
        self.assertRaises(TypeError, self.zip.extract, "test.txt", pwd="python")

class AbstractTestsWithRandomBinaryFiles:
    @classmethod
    def setUpClass(cls):
        datacount = randint(16, 64)*1024 + randint(1, 1024)
        cls.data = b''.join(struct.pack('<f', random()*randint(-1000, 1000))
                            for i in range(datacount))

    def setUp(self):
        # Make a source file with some lines
        with open(TESTFN, "wb") as fp:
            fp.write(self.data)

    def tearDown(self):
        unlink(TESTFN)
        unlink(TESTFN2)

    def make_test_archive(self, f, compression):
        # Create the ZIP archive
        with zipfile.ZipFile(f, "w", compression) as zipfp:
            zipfp.write(TESTFN, "another.name")
            zipfp.write(TESTFN, TESTFN)

    def zip_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r", compression) as zipfp:
            testdata = zipfp.read(TESTFN)
            self.assertEqual(len(testdata), len(self.data))
            self.assertEqual(testdata, self.data)
            self.assertEqual(zipfp.read("another.name"), self.data)

    def test_read(self):
        for f in get_files(self):
            self.zip_test(f, self.compression)

    def zip_open_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r", compression) as zipfp:
            zipdata1 = []
            with zipfp.open(TESTFN) as zipopen1:
                while True:
                    read_data = zipopen1.read(256)
                    if not read_data:
                        break
                    zipdata1.append(read_data)

            zipdata2 = []
            with zipfp.open("another.name") as zipopen2:
                while True:
                    read_data = zipopen2.read(256)
                    if not read_data:
                        break
                    zipdata2.append(read_data)

            testdata1 = b''.join(zipdata1)
            self.assertEqual(len(testdata1), len(self.data))
            self.assertEqual(testdata1, self.data)

            testdata2 = b''.join(zipdata2)
            self.assertEqual(len(testdata2), len(self.data))
            self.assertEqual(testdata2, self.data)

    def test_open(self):
        for f in get_files(self):
            self.zip_open_test(f, self.compression)

    def zip_random_open_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r", compression) as zipfp:
            zipdata1 = []
            with zipfp.open(TESTFN) as zipopen1:
                while True:
                    read_data = zipopen1.read(randint(1, 1024))
                    if not read_data:
                        break
                    zipdata1.append(read_data)

            testdata = b''.join(zipdata1)
            self.assertEqual(len(testdata), len(self.data))
            self.assertEqual(testdata, self.data)

    def test_random_open(self):
        for f in get_files(self):
            self.zip_random_open_test(f, self.compression)


class StoredTestsWithRandomBinaryFiles(AbstractTestsWithRandomBinaryFiles,
                                       unittest.TestCase):
    compression = zipfile.ZIP_STORED

@requires_zlib
class DeflateTestsWithRandomBinaryFiles(AbstractTestsWithRandomBinaryFiles,
                                        unittest.TestCase):
    compression = zipfile.ZIP_DEFLATED

@requires_bz2
class Bzip2TestsWithRandomBinaryFiles(AbstractTestsWithRandomBinaryFiles,
                                      unittest.TestCase):
    compression = zipfile.ZIP_BZIP2

@requires_lzma
class LzmaTestsWithRandomBinaryFiles(AbstractTestsWithRandomBinaryFiles,
                                     unittest.TestCase):
    compression = zipfile.ZIP_LZMA


@requires_zlib
class TestsWithMultipleOpens(unittest.TestCase):
    def setUp(self):
        # Create the ZIP archive
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_DEFLATED) as zipfp:
            zipfp.writestr('ones', '1'*FIXEDTEST_SIZE)
            zipfp.writestr('twos', '2'*FIXEDTEST_SIZE)

    def test_same_file(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        with zipfile.ZipFile(TESTFN2, mode="r") as zipf:
            with zipf.open('ones') as zopen1, zipf.open('ones') as zopen2:
                data1 = zopen1.read(500)
                data2 = zopen2.read(500)
                data1 += zopen1.read(500)
                data2 += zopen2.read(500)
            self.assertEqual(data1, data2)

    def test_different_file(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        with zipfile.ZipFile(TESTFN2, mode="r") as zipf:
            with zipf.open('ones') as zopen1, zipf.open('twos') as zopen2:
                data1 = zopen1.read(500)
                data2 = zopen2.read(500)
                data1 += zopen1.read(500)
                data2 += zopen2.read(500)
            self.assertEqual(data1, b'1'*FIXEDTEST_SIZE)
            self.assertEqual(data2, b'2'*FIXEDTEST_SIZE)

    def test_interleaved(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        with zipfile.ZipFile(TESTFN2, mode="r") as zipf:
            with zipf.open('ones') as zopen1, zipf.open('twos') as zopen2:
                data1 = zopen1.read(500)
                data2 = zopen2.read(500)
                data1 += zopen1.read(500)
                data2 += zopen2.read(500)
            self.assertEqual(data1, b'1'*FIXEDTEST_SIZE)
            self.assertEqual(data2, b'2'*FIXEDTEST_SIZE)

    def tearDown(self):
        unlink(TESTFN2)


class TestWithDirectory(unittest.TestCase):
    def setUp(self):
        os.mkdir(TESTFN2)

    def test_extract_dir(self):
        with zipfile.ZipFile(findfile("zipdir.zip")) as zipf:
            zipf.extractall(TESTFN2)
        self.assertTrue(os.path.isdir(os.path.join(TESTFN2, "a")))
        self.assertTrue(os.path.isdir(os.path.join(TESTFN2, "a", "b")))
        self.assertTrue(os.path.exists(os.path.join(TESTFN2, "a", "b", "c")))

    def test_bug_6050(self):
        # Extraction should succeed if directories already exist
        os.mkdir(os.path.join(TESTFN2, "a"))
        self.test_extract_dir()

    def test_store_dir(self):
        os.mkdir(os.path.join(TESTFN2, "x"))
        zipf = zipfile.ZipFile(TESTFN, "w")
        zipf.write(os.path.join(TESTFN2, "x"), "x")
        self.assertTrue(zipf.filelist[0].filename.endswith("x/"))

    def tearDown(self):
        shutil.rmtree(TESTFN2)
        if os.path.exists(TESTFN):
            unlink(TESTFN)


class AbstractUniversalNewlineTests:
    @classmethod
    def setUpClass(cls):
        cls.line_gen = [bytes("Test of zipfile line %d." % i, "ascii")
                        for i in range(FIXEDTEST_SIZE)]
        cls.seps = (b'\r', b'\r\n', b'\n')
        cls.arcdata = {}
        for n, s in enumerate(cls.seps):
            cls.arcdata[s] = s.join(cls.line_gen) + s

    def setUp(self):
        self.arcfiles = {}
        for n, s in enumerate(self.seps):
            self.arcfiles[s] = '%s-%d' % (TESTFN, n)
            with open(self.arcfiles[s], "wb") as f:
                f.write(self.arcdata[s])

    def make_test_archive(self, f, compression):
        # Create the ZIP archive
        with zipfile.ZipFile(f, "w", compression) as zipfp:
            for fn in self.arcfiles.values():
                zipfp.write(fn, fn)

    def read_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp:
            for sep, fn in self.arcfiles.items():
                with zipfp.open(fn, "rU") as fp:
                    zipdata = fp.read()
                self.assertEqual(self.arcdata[sep], zipdata)

    def test_read(self):
        for f in get_files(self):
            self.read_test(f, self.compression)

    def readline_read_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp:
            for sep, fn in self.arcfiles.items():
                with zipfp.open(fn, "rU") as zipopen:
                    data = b''
                    while True:
                        read = zipopen.readline()
                        if not read:
                            break
                        data += read

                        read = zipopen.read(5)
                        if not read:
                            break
                        data += read

            self.assertEqual(data, self.arcdata[b'\n'])

    def test_readline_read(self):
        for f in get_files(self):
            self.readline_read_test(f, self.compression)

    def readline_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp:
            for sep, fn in self.arcfiles.items():
                with zipfp.open(fn, "rU") as zipopen:
                    for line in self.line_gen:
                        linedata = zipopen.readline()
                        self.assertEqual(linedata, line + b'\n')

    def test_readline(self):
        for f in get_files(self):
            self.readline_test(f, self.compression)

    def readlines_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp:
            for sep, fn in self.arcfiles.items():
                with zipfp.open(fn, "rU") as fp:
                    ziplines = fp.readlines()
                for line, zipline in zip(self.line_gen, ziplines):
                    self.assertEqual(zipline, line + b'\n')

    def test_readlines(self):
        for f in get_files(self):
            self.readlines_test(f, self.compression)

    def iterlines_test(self, f, compression):
        self.make_test_archive(f, compression)

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r") as zipfp:
            for sep, fn in self.arcfiles.items():
                with zipfp.open(fn, "rU") as fp:
                    for line, zipline in zip(self.line_gen, fp):
                        self.assertEqual(zipline, line + b'\n')

    def test_iterlines(self):
        for f in get_files(self):
            self.iterlines_test(f, self.compression)

    def tearDown(self):
        for sep, fn in self.arcfiles.items():
            os.remove(fn)
        unlink(TESTFN)
        unlink(TESTFN2)


class StoredUniversalNewlineTests(AbstractUniversalNewlineTests,
                                  unittest.TestCase):
    compression = zipfile.ZIP_STORED

@requires_zlib
class DeflateUniversalNewlineTests(AbstractUniversalNewlineTests,
                                   unittest.TestCase):
    compression = zipfile.ZIP_DEFLATED

@requires_bz2
class Bzip2UniversalNewlineTests(AbstractUniversalNewlineTests,
                                 unittest.TestCase):
    compression = zipfile.ZIP_BZIP2

@requires_lzma
class LzmaUniversalNewlineTests(AbstractUniversalNewlineTests,
                                unittest.TestCase):
    compression = zipfile.ZIP_LZMA


if __name__ == "__main__":
    unittest.main()
