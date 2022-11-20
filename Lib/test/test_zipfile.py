import array
import contextlib
import importlib.util
import io
import itertools
import os
import pathlib
import posixpath
import string
import struct
import subprocess
import sys
import time
import unittest
import unittest.mock as mock
import zipfile
import functools


from tempfile import TemporaryFile
from random import randint, random, randbytes

from test.support import script_helper
from test.support import (
    findfile, requires_zlib, requires_bz2, requires_lzma,
    captured_stdout, captured_stderr, requires_subprocess
)
from test.support.os_helper import (
    TESTFN, unlink, rmtree, temp_dir, temp_cwd, fd_count
)


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

    def make_test_archive(self, f, compression, compresslevel=None):
        kwargs = {'compression': compression, 'compresslevel': compresslevel}
        # Create the ZIP archive
        with zipfile.ZipFile(f, "w", **kwargs) as zipfp:
            zipfp.write(TESTFN, "another.name")
            zipfp.write(TESTFN, TESTFN)
            zipfp.writestr("strfile", self.data)
            with zipfp.open('written-open-w', mode='w') as f:
                for line in self.line_gen:
                    f.write(line)

    def zip_test(self, f, compression, compresslevel=None):
        self.make_test_archive(f, compression, compresslevel)

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
            self.assertEqual(len(lines), 5) # Number of files + header

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
            self.assertEqual(len(names), 4)
            self.assertIn(TESTFN, names)
            self.assertIn("another.name", names)
            self.assertIn("strfile", names)
            self.assertIn("written-open-w", names)

            # Check infolist
            infos = zipfp.infolist()
            names = [i.filename for i in infos]
            self.assertEqual(len(names), 4)
            self.assertIn(TESTFN, names)
            self.assertIn("another.name", names)
            self.assertIn("strfile", names)
            self.assertIn("written-open-w", names)
            for i in infos:
                self.assertEqual(i.file_size, len(self.data))

            # check getinfo
            for nm in (TESTFN, "another.name", "strfile", "written-open-w"):
                info = zipfp.getinfo(nm)
                self.assertEqual(info.filename, nm)
                self.assertEqual(info.file_size, len(self.data))

            # Check that testzip thinks the archive is ok
            # (it returns None if all contents could be read properly)
            self.assertIsNone(zipfp.testzip())

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

    def test_open_with_pathlike(self):
        path = pathlib.Path(TESTFN2)
        self.zip_open_test(path, self.compression)
        with zipfile.ZipFile(path, "r", self.compression) as zipfp:
            self.assertIsInstance(zipfp.filename, str)

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

    def test_writestr_compresslevel(self):
        zipfp = zipfile.ZipFile(TESTFN2, "w", compresslevel=1)
        zipfp.writestr("a.txt", "hello world", compress_type=self.compression)
        zipfp.writestr("b.txt", "hello world", compress_type=self.compression,
                       compresslevel=2)

        # Compression level follows the constructor.
        a_info = zipfp.getinfo('a.txt')
        self.assertEqual(a_info.compress_type, self.compression)
        self.assertEqual(a_info._compresslevel, 1)

        # Compression level is overridden.
        b_info = zipfp.getinfo('b.txt')
        self.assertEqual(b_info.compress_type, self.compression)
        self.assertEqual(b_info._compresslevel, 2)

    def test_read_return_size(self):
        # Issue #9837: ZipExtFile.read() shouldn't return more bytes
        # than requested.
        for test_size in (1, 4095, 4096, 4097, 16384):
            file_size = test_size + 1
            junk = randbytes(file_size)
            with zipfile.ZipFile(io.BytesIO(), "w", self.compression) as zipf:
                zipf.writestr('foo', junk)
                with zipf.open('foo', 'r') as fp:
                    buf = fp.read(test_size)
                    self.assertEqual(len(buf), test_size)

    def test_truncated_zipfile(self):
        fp = io.BytesIO()
        with zipfile.ZipFile(fp, mode='w') as zipf:
            zipf.writestr('strfile', self.data, compress_type=self.compression)
            end_offset = fp.tell()
        zipfiledata = fp.getvalue()

        fp = io.BytesIO(zipfiledata)
        with zipfile.ZipFile(fp) as zipf:
            with zipf.open('strfile') as zipopen:
                fp.truncate(end_offset - 20)
                with self.assertRaises(EOFError):
                    zipopen.read()

        fp = io.BytesIO(zipfiledata)
        with zipfile.ZipFile(fp) as zipf:
            with zipf.open('strfile') as zipopen:
                fp.truncate(end_offset - 20)
                with self.assertRaises(EOFError):
                    while zipopen.read(100):
                        pass

        fp = io.BytesIO(zipfiledata)
        with zipfile.ZipFile(fp) as zipf:
            with zipf.open('strfile') as zipopen:
                fp.truncate(end_offset - 20)
                with self.assertRaises(EOFError):
                    while zipopen.read1(100):
                        pass

    def test_repr(self):
        fname = 'file.name'
        for f in get_files(self):
            with zipfile.ZipFile(f, 'w', self.compression) as zipfp:
                zipfp.write(TESTFN, fname)
                r = repr(zipfp)
                self.assertIn("mode='w'", r)

            with zipfile.ZipFile(f, 'r') as zipfp:
                r = repr(zipfp)
                if isinstance(f, str):
                    self.assertIn('filename=%r' % f, r)
                else:
                    self.assertIn('file=%r' % f, r)
                self.assertIn("mode='r'", r)
                r = repr(zipfp.getinfo(fname))
                self.assertIn('filename=%r' % fname, r)
                self.assertIn('filemode=', r)
                self.assertIn('file_size=', r)
                if self.compression != zipfile.ZIP_STORED:
                    self.assertIn('compress_type=', r)
                    self.assertIn('compress_size=', r)
                with zipfp.open(fname) as zipopen:
                    r = repr(zipopen)
                    self.assertIn('name=%r' % fname, r)
                    self.assertIn("mode='r'", r)
                    if self.compression != zipfile.ZIP_STORED:
                        self.assertIn('compress_type=', r)
                self.assertIn('[closed]', repr(zipopen))
            self.assertIn('[closed]', repr(zipfp))

    def test_compresslevel_basic(self):
        for f in get_files(self):
            self.zip_test(f, self.compression, compresslevel=9)

    def test_per_file_compresslevel(self):
        """Check that files within a Zip archive can have different
        compression levels."""
        with zipfile.ZipFile(TESTFN2, "w", compresslevel=1) as zipfp:
            zipfp.write(TESTFN, 'compress_1')
            zipfp.write(TESTFN, 'compress_9', compresslevel=9)
            one_info = zipfp.getinfo('compress_1')
            nine_info = zipfp.getinfo('compress_9')
            self.assertEqual(one_info._compresslevel, 1)
            self.assertEqual(nine_info._compresslevel, 9)

    def test_writing_errors(self):
        class BrokenFile(io.BytesIO):
            def write(self, data):
                nonlocal count
                if count is not None:
                    if count == stop:
                        raise OSError
                    count += 1
                super().write(data)

        stop = 0
        while True:
            testfile = BrokenFile()
            count = None
            with zipfile.ZipFile(testfile, 'w', self.compression) as zipfp:
                with zipfp.open('file1', 'w') as f:
                    f.write(b'data1')
                count = 0
                try:
                    with zipfp.open('file2', 'w') as f:
                        f.write(b'data2')
                except OSError:
                    stop += 1
                else:
                    break
                finally:
                    count = None
            with zipfile.ZipFile(io.BytesIO(testfile.getvalue())) as zipfp:
                self.assertEqual(zipfp.namelist(), ['file1'])
                self.assertEqual(zipfp.read('file1'), b'data1')

        with zipfile.ZipFile(io.BytesIO(testfile.getvalue())) as zipfp:
            self.assertEqual(zipfp.namelist(), ['file1', 'file2'])
            self.assertEqual(zipfp.read('file1'), b'data1')
            self.assertEqual(zipfp.read('file2'), b'data2')


    def tearDown(self):
        unlink(TESTFN)
        unlink(TESTFN2)


class StoredTestsWithSourceFile(AbstractTestsWithSourceFile,
                                unittest.TestCase):
    compression = zipfile.ZIP_STORED
    test_low_compression = None

    def zip_test_writestr_permissions(self, f, compression):
        # Make sure that writestr and open(... mode='w') create files with
        # mode 0600, when they are passed a name rather than a ZipInfo
        # instance.

        self.make_test_archive(f, compression)
        with zipfile.ZipFile(f, "r") as zipfp:
            zinfo = zipfp.getinfo('strfile')
            self.assertEqual(zinfo.external_attr, 0o600 << 16)

            zinfo2 = zipfp.getinfo('written-open-w')
            self.assertEqual(zinfo2.external_attr, 0o600 << 16)

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
                self.assertEqual(zipfp.read(TESTFN), self.data)
        with open(TESTFN2, 'rb') as f:
            self.assertEqual(f.read(len(data)), data)
            zipfiledata = f.read()
        with io.BytesIO(zipfiledata) as bio, zipfile.ZipFile(bio) as zipfp:
            self.assertEqual(zipfp.namelist(), [TESTFN])
            self.assertEqual(zipfp.read(TESTFN), self.data)

    def test_read_concatenated_zip_file(self):
        with io.BytesIO() as bio:
            with zipfile.ZipFile(bio, 'w', zipfile.ZIP_STORED) as zipfp:
                zipfp.write(TESTFN, TESTFN)
            zipfiledata = bio.getvalue()
        data = b'I am not a ZipFile!'*10
        with open(TESTFN2, 'wb') as f:
            f.write(data)
            f.write(zipfiledata)

        with zipfile.ZipFile(TESTFN2) as zipfp:
            self.assertEqual(zipfp.namelist(), [TESTFN])
            self.assertEqual(zipfp.read(TESTFN), self.data)

    def test_append_to_concatenated_zip_file(self):
        with io.BytesIO() as bio:
            with zipfile.ZipFile(bio, 'w', zipfile.ZIP_STORED) as zipfp:
                zipfp.write(TESTFN, TESTFN)
            zipfiledata = bio.getvalue()
        data = b'I am not a ZipFile!'*1000000
        with open(TESTFN2, 'wb') as f:
            f.write(data)
            f.write(zipfiledata)

        with zipfile.ZipFile(TESTFN2, 'a') as zipfp:
            self.assertEqual(zipfp.namelist(), [TESTFN])
            zipfp.writestr('strfile', self.data)

        with open(TESTFN2, 'rb') as f:
            self.assertEqual(f.read(len(data)), data)
            zipfiledata = f.read()
        with io.BytesIO(zipfiledata) as bio, zipfile.ZipFile(bio) as zipfp:
            self.assertEqual(zipfp.namelist(), [TESTFN, 'strfile'])
            self.assertEqual(zipfp.read(TESTFN), self.data)
            self.assertEqual(zipfp.read('strfile'), self.data)

    def test_ignores_newline_at_end(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            zipfp.write(TESTFN, TESTFN)
        with open(TESTFN2, 'a', encoding='utf-8') as f:
            f.write("\r\n\00\00\00")
        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            self.assertIsInstance(zipfp, zipfile.ZipFile)

    def test_ignores_stuff_appended_past_comments(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            zipfp.comment = b"this is a comment"
            zipfp.write(TESTFN, TESTFN)
        with open(TESTFN2, 'a', encoding='utf-8') as f:
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

    def test_io_on_closed_zipextfile(self):
        fname = "somefile.txt"
        with zipfile.ZipFile(TESTFN2, mode="w") as zipfp:
            zipfp.writestr(fname, "bogus")

        with zipfile.ZipFile(TESTFN2, mode="r") as zipfp:
            with zipfp.open(fname) as fid:
                fid.close()
                self.assertRaises(ValueError, fid.read)
                self.assertRaises(ValueError, fid.seek, 0)
                self.assertRaises(ValueError, fid.tell)
                self.assertRaises(ValueError, fid.readable)
                self.assertRaises(ValueError, fid.seekable)

    def test_write_to_readonly(self):
        """Check that trying to call write() on a readonly ZipFile object
        raises a ValueError."""
        with zipfile.ZipFile(TESTFN2, mode="w") as zipfp:
            zipfp.writestr("somefile.txt", "bogus")

        with zipfile.ZipFile(TESTFN2, mode="r") as zipfp:
            self.assertRaises(ValueError, zipfp.write, TESTFN)

        with zipfile.ZipFile(TESTFN2, mode="r") as zipfp:
            with self.assertRaises(ValueError):
                zipfp.open(TESTFN, mode='w')

    def test_add_file_before_1980(self):
        # Set atime and mtime to 1970-01-01
        os.utime(TESTFN, (0, 0))
        with zipfile.ZipFile(TESTFN2, "w") as zipfp:
            self.assertRaises(ValueError, zipfp.write, TESTFN)

        with zipfile.ZipFile(TESTFN2, "w", strict_timestamps=False) as zipfp:
            zipfp.write(TESTFN)
            zinfo = zipfp.getinfo(TESTFN)
            self.assertEqual(zinfo.date_time, (1980, 1, 1, 0, 0, 0))

    def test_add_file_after_2107(self):
        # Set atime and mtime to 2108-12-30
        ts = 4386268800
        try:
            time.localtime(ts)
        except OverflowError:
            self.skipTest(f'time.localtime({ts}) raises OverflowError')
        try:
            os.utime(TESTFN, (ts, ts))
        except OverflowError:
            self.skipTest('Host fs cannot set timestamp to required value.')

        mtime_ns = os.stat(TESTFN).st_mtime_ns
        if mtime_ns != (4386268800 * 10**9):
            # XFS filesystem is limited to 32-bit timestamp, but the syscall
            # didn't fail. Moreover, there is a VFS bug which returns
            # a cached timestamp which is different than the value on disk.
            #
            # Test st_mtime_ns rather than st_mtime to avoid rounding issues.
            #
            # https://bugzilla.redhat.com/show_bug.cgi?id=1795576
            # https://bugs.python.org/issue39460#msg360952
            self.skipTest(f"Linux VFS/XFS kernel bug detected: {mtime_ns=}")

        with zipfile.ZipFile(TESTFN2, "w") as zipfp:
            self.assertRaises(struct.error, zipfp.write, TESTFN)

        with zipfile.ZipFile(TESTFN2, "w", strict_timestamps=False) as zipfp:
            zipfp.write(TESTFN)
            zinfo = zipfp.getinfo(TESTFN)
            self.assertEqual(zinfo.date_time, (2107, 12, 31, 23, 59, 59))


@requires_zlib()
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

@requires_bz2()
class Bzip2TestsWithSourceFile(AbstractTestsWithSourceFile,
                               unittest.TestCase):
    compression = zipfile.ZIP_BZIP2

@requires_lzma()
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
        self._filecount_limit = zipfile.ZIP_FILECOUNT_LIMIT
        zipfile.ZIP64_LIMIT = 1000
        zipfile.ZIP_FILECOUNT_LIMIT = 9

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

            # Check that testzip thinks the archive is valid
            self.assertIsNone(zipfp.testzip())

    def test_basic(self):
        for f in get_files(self):
            self.zip_test(f, self.compression)

    def test_too_many_files(self):
        # This test checks that more than 64k files can be added to an archive,
        # and that the resulting archive can be read properly by ZipFile
        zipf = zipfile.ZipFile(TESTFN, "w", self.compression,
                               allowZip64=True)
        zipf.debug = 100
        numfiles = 15
        for i in range(numfiles):
            zipf.writestr("foo%08d" % i, "%d" % (i**3 % 57))
        self.assertEqual(len(zipf.namelist()), numfiles)
        zipf.close()

        zipf2 = zipfile.ZipFile(TESTFN, "r", self.compression)
        self.assertEqual(len(zipf2.namelist()), numfiles)
        for i in range(numfiles):
            content = zipf2.read("foo%08d" % i).decode('ascii')
            self.assertEqual(content, "%d" % (i**3 % 57))
        zipf2.close()

    def test_too_many_files_append(self):
        zipf = zipfile.ZipFile(TESTFN, "w", self.compression,
                               allowZip64=False)
        zipf.debug = 100
        numfiles = 9
        for i in range(numfiles):
            zipf.writestr("foo%08d" % i, "%d" % (i**3 % 57))
        self.assertEqual(len(zipf.namelist()), numfiles)
        with self.assertRaises(zipfile.LargeZipFile):
            zipf.writestr("foo%08d" % numfiles, b'')
        self.assertEqual(len(zipf.namelist()), numfiles)
        zipf.close()

        zipf = zipfile.ZipFile(TESTFN, "a", self.compression,
                               allowZip64=False)
        zipf.debug = 100
        self.assertEqual(len(zipf.namelist()), numfiles)
        with self.assertRaises(zipfile.LargeZipFile):
            zipf.writestr("foo%08d" % numfiles, b'')
        self.assertEqual(len(zipf.namelist()), numfiles)
        zipf.close()

        zipf = zipfile.ZipFile(TESTFN, "a", self.compression,
                               allowZip64=True)
        zipf.debug = 100
        self.assertEqual(len(zipf.namelist()), numfiles)
        numfiles2 = 15
        for i in range(numfiles, numfiles2):
            zipf.writestr("foo%08d" % i, "%d" % (i**3 % 57))
        self.assertEqual(len(zipf.namelist()), numfiles2)
        zipf.close()

        zipf2 = zipfile.ZipFile(TESTFN, "r", self.compression)
        self.assertEqual(len(zipf2.namelist()), numfiles2)
        for i in range(numfiles2):
            content = zipf2.read("foo%08d" % i).decode('ascii')
            self.assertEqual(content, "%d" % (i**3 % 57))
        zipf2.close()

    def tearDown(self):
        zipfile.ZIP64_LIMIT = self._limit
        zipfile.ZIP_FILECOUNT_LIMIT = self._filecount_limit
        unlink(TESTFN)
        unlink(TESTFN2)


class StoredTestZip64InSmallFiles(AbstractTestZip64InSmallFiles,
                                  unittest.TestCase):
    compression = zipfile.ZIP_STORED

    def large_file_exception_test(self, f, compression):
        with zipfile.ZipFile(f, "w", compression, allowZip64=False) as zipfp:
            self.assertRaises(zipfile.LargeZipFile,
                              zipfp.write, TESTFN, "another.name")

    def large_file_exception_test2(self, f, compression):
        with zipfile.ZipFile(f, "w", compression, allowZip64=False) as zipfp:
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

    def test_append(self):
        # Test that appending to the Zip64 archive doesn't change
        # extra fields of existing entries.
        with zipfile.ZipFile(TESTFN2, "w", allowZip64=True) as zipfp:
            zipfp.writestr("strfile", self.data)
        with zipfile.ZipFile(TESTFN2, "r", allowZip64=True) as zipfp:
            zinfo = zipfp.getinfo("strfile")
            extra = zinfo.extra
        with zipfile.ZipFile(TESTFN2, "a", allowZip64=True) as zipfp:
            zipfp.writestr("strfile2", self.data)
        with zipfile.ZipFile(TESTFN2, "r", allowZip64=True) as zipfp:
            zinfo = zipfp.getinfo("strfile")
            self.assertEqual(zinfo.extra, extra)

    def make_zip64_file(
        self, file_size_64_set=False, file_size_extra=False,
        compress_size_64_set=False, compress_size_extra=False,
        header_offset_64_set=False, header_offset_extra=False,
    ):
        """Generate bytes sequence for a zip with (incomplete) zip64 data.

        The actual values (not the zip 64 0xffffffff values) stored in the file
        are:
        file_size: 8
        compress_size: 8
        header_offset: 0
        """
        actual_size = 8
        actual_header_offset = 0
        local_zip64_fields = []
        central_zip64_fields = []

        file_size = actual_size
        if file_size_64_set:
            file_size = 0xffffffff
            if file_size_extra:
                local_zip64_fields.append(actual_size)
                central_zip64_fields.append(actual_size)
        file_size = struct.pack("<L", file_size)

        compress_size = actual_size
        if compress_size_64_set:
            compress_size = 0xffffffff
            if compress_size_extra:
                local_zip64_fields.append(actual_size)
                central_zip64_fields.append(actual_size)
        compress_size = struct.pack("<L", compress_size)

        header_offset = actual_header_offset
        if header_offset_64_set:
            header_offset = 0xffffffff
            if header_offset_extra:
                central_zip64_fields.append(actual_header_offset)
        header_offset = struct.pack("<L", header_offset)

        local_extra = struct.pack(
            '<HH' + 'Q'*len(local_zip64_fields),
            0x0001,
            8*len(local_zip64_fields),
            *local_zip64_fields
        )

        central_extra = struct.pack(
            '<HH' + 'Q'*len(central_zip64_fields),
            0x0001,
            8*len(central_zip64_fields),
            *central_zip64_fields
        )

        central_dir_size = struct.pack('<Q', 58 + 8 * len(central_zip64_fields))
        offset_to_central_dir = struct.pack('<Q', 50 + 8 * len(local_zip64_fields))

        local_extra_length = struct.pack("<H", 4 + 8 * len(local_zip64_fields))
        central_extra_length = struct.pack("<H", 4 + 8 * len(central_zip64_fields))

        filename = b"test.txt"
        content = b"test1234"
        filename_length = struct.pack("<H", len(filename))
        zip64_contents = (
            # Local file header
            b"PK\x03\x04\x14\x00\x00\x00\x00\x00\x00\x00!\x00\x9e%\xf5\xaf"
            + compress_size
            + file_size
            + filename_length
            + local_extra_length
            + filename
            + local_extra
            + content
            # Central directory:
            + b"PK\x01\x02-\x03-\x00\x00\x00\x00\x00\x00\x00!\x00\x9e%\xf5\xaf"
            + compress_size
            + file_size
            + filename_length
            + central_extra_length
            + b"\x00\x00\x00\x00\x00\x00\x00\x00\x80\x01"
            + header_offset
            + filename
            + central_extra
            # Zip64 end of central directory
            + b"PK\x06\x06,\x00\x00\x00\x00\x00\x00\x00-\x00-"
            + b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00"
            + b"\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00"
            + central_dir_size
            + offset_to_central_dir
            # Zip64 end of central directory locator
            + b"PK\x06\x07\x00\x00\x00\x00l\x00\x00\x00\x00\x00\x00\x00\x01"
            + b"\x00\x00\x00"
            # end of central directory
            + b"PK\x05\x06\x00\x00\x00\x00\x01\x00\x01\x00:\x00\x00\x002\x00"
            + b"\x00\x00\x00\x00"
        )
        return zip64_contents

    def test_bad_zip64_extra(self):
        """Missing zip64 extra records raises an exception.

        There are 4 fields that the zip64 format handles (the disk number is
        not used in this module and so is ignored here). According to the zip
        spec:
              The order of the fields in the zip64 extended
              information record is fixed, but the fields MUST
              only appear if the corresponding Local or Central
              directory record field is set to 0xFFFF or 0xFFFFFFFF.

        If the zip64 extra content doesn't contain enough entries for the
        number of fields marked with 0xFFFF or 0xFFFFFFFF, we raise an error.
        This test mismatches the length of the zip64 extra field and the number
        of fields set to indicate the presence of zip64 data.
        """
        # zip64 file size present, no fields in extra, expecting one, equals
        # missing file size.
        missing_file_size_extra = self.make_zip64_file(
            file_size_64_set=True,
        )
        with self.assertRaises(zipfile.BadZipFile) as e:
            zipfile.ZipFile(io.BytesIO(missing_file_size_extra))
        self.assertIn('file size', str(e.exception).lower())

        # zip64 file size present, zip64 compress size present, one field in
        # extra, expecting two, equals missing compress size.
        missing_compress_size_extra = self.make_zip64_file(
            file_size_64_set=True,
            file_size_extra=True,
            compress_size_64_set=True,
        )
        with self.assertRaises(zipfile.BadZipFile) as e:
            zipfile.ZipFile(io.BytesIO(missing_compress_size_extra))
        self.assertIn('compress size', str(e.exception).lower())

        # zip64 compress size present, no fields in extra, expecting one,
        # equals missing compress size.
        missing_compress_size_extra = self.make_zip64_file(
            compress_size_64_set=True,
        )
        with self.assertRaises(zipfile.BadZipFile) as e:
            zipfile.ZipFile(io.BytesIO(missing_compress_size_extra))
        self.assertIn('compress size', str(e.exception).lower())

        # zip64 file size present, zip64 compress size present, zip64 header
        # offset present, two fields in extra, expecting three, equals missing
        # header offset
        missing_header_offset_extra = self.make_zip64_file(
            file_size_64_set=True,
            file_size_extra=True,
            compress_size_64_set=True,
            compress_size_extra=True,
            header_offset_64_set=True,
        )
        with self.assertRaises(zipfile.BadZipFile) as e:
            zipfile.ZipFile(io.BytesIO(missing_header_offset_extra))
        self.assertIn('header offset', str(e.exception).lower())

        # zip64 compress size present, zip64 header offset present, one field
        # in extra, expecting two, equals missing header offset
        missing_header_offset_extra = self.make_zip64_file(
            file_size_64_set=False,
            compress_size_64_set=True,
            compress_size_extra=True,
            header_offset_64_set=True,
        )
        with self.assertRaises(zipfile.BadZipFile) as e:
            zipfile.ZipFile(io.BytesIO(missing_header_offset_extra))
        self.assertIn('header offset', str(e.exception).lower())

        # zip64 file size present, zip64 header offset present, one field in
        # extra, expecting two, equals missing header offset
        missing_header_offset_extra = self.make_zip64_file(
            file_size_64_set=True,
            file_size_extra=True,
            compress_size_64_set=False,
            header_offset_64_set=True,
        )
        with self.assertRaises(zipfile.BadZipFile) as e:
            zipfile.ZipFile(io.BytesIO(missing_header_offset_extra))
        self.assertIn('header offset', str(e.exception).lower())

        # zip64 header offset present, no fields in extra, expecting one,
        # equals missing header offset
        missing_header_offset_extra = self.make_zip64_file(
            file_size_64_set=False,
            compress_size_64_set=False,
            header_offset_64_set=True,
        )
        with self.assertRaises(zipfile.BadZipFile) as e:
            zipfile.ZipFile(io.BytesIO(missing_header_offset_extra))
        self.assertIn('header offset', str(e.exception).lower())

    def test_generated_valid_zip64_extra(self):
        # These values are what is set in the make_zip64_file method.
        expected_file_size = 8
        expected_compress_size = 8
        expected_header_offset = 0
        expected_content = b"test1234"

        # Loop through the various valid combinations of zip64 masks
        # present and extra fields present.
        params = (
            {"file_size_64_set": True, "file_size_extra": True},
            {"compress_size_64_set": True, "compress_size_extra": True},
            {"header_offset_64_set": True, "header_offset_extra": True},
        )

        for r in range(1, len(params) + 1):
            for combo in itertools.combinations(params, r):
                kwargs = {}
                for c in combo:
                    kwargs.update(c)
                with zipfile.ZipFile(io.BytesIO(self.make_zip64_file(**kwargs))) as zf:
                    zinfo = zf.infolist()[0]
                    self.assertEqual(zinfo.file_size, expected_file_size)
                    self.assertEqual(zinfo.compress_size, expected_compress_size)
                    self.assertEqual(zinfo.header_offset, expected_header_offset)
                    self.assertEqual(zf.read(zinfo), expected_content)


@requires_zlib()
class DeflateTestZip64InSmallFiles(AbstractTestZip64InSmallFiles,
                                   unittest.TestCase):
    compression = zipfile.ZIP_DEFLATED

@requires_bz2()
class Bzip2TestZip64InSmallFiles(AbstractTestZip64InSmallFiles,
                                 unittest.TestCase):
    compression = zipfile.ZIP_BZIP2

@requires_lzma()
class LzmaTestZip64InSmallFiles(AbstractTestZip64InSmallFiles,
                                unittest.TestCase):
    compression = zipfile.ZIP_LZMA


class AbstractWriterTests:

    def tearDown(self):
        unlink(TESTFN2)

    def test_close_after_close(self):
        data = b'content'
        with zipfile.ZipFile(TESTFN2, "w", self.compression) as zipf:
            w = zipf.open('test', 'w')
            w.write(data)
            w.close()
            self.assertTrue(w.closed)
            w.close()
            self.assertTrue(w.closed)
            self.assertEqual(zipf.read('test'), data)

    def test_write_after_close(self):
        data = b'content'
        with zipfile.ZipFile(TESTFN2, "w", self.compression) as zipf:
            w = zipf.open('test', 'w')
            w.write(data)
            w.close()
            self.assertTrue(w.closed)
            self.assertRaises(ValueError, w.write, b'')
            self.assertEqual(zipf.read('test'), data)

    def test_issue44439(self):
        q = array.array('Q', [1, 2, 3, 4, 5])
        LENGTH = len(q) * q.itemsize
        with zipfile.ZipFile(io.BytesIO(), 'w', self.compression) as zip:
            with zip.open('data', 'w') as data:
                self.assertEqual(data.write(q), LENGTH)
            self.assertEqual(zip.getinfo('data').file_size, LENGTH)

class StoredWriterTests(AbstractWriterTests, unittest.TestCase):
    compression = zipfile.ZIP_STORED

@requires_zlib()
class DeflateWriterTests(AbstractWriterTests, unittest.TestCase):
    compression = zipfile.ZIP_DEFLATED

@requires_bz2()
class Bzip2WriterTests(AbstractWriterTests, unittest.TestCase):
    compression = zipfile.ZIP_BZIP2

@requires_lzma()
class LzmaWriterTests(AbstractWriterTests, unittest.TestCase):
    compression = zipfile.ZIP_LZMA


class PyZipFileTests(unittest.TestCase):
    def assertCompiledIn(self, name, namelist):
        if name + 'o' not in namelist:
            self.assertIn(name + 'c', namelist)

    def requiresWriteAccess(self, path):
        # effective_ids unavailable on windows
        if not os.access(path, os.W_OK,
                         effective_ids=os.access in os.supports_effective_ids):
            self.skipTest('requires write access to the installed location')
        filename = os.path.join(path, 'test_zipfile.try')
        try:
            fd = os.open(filename, os.O_WRONLY | os.O_CREAT)
            os.close(fd)
        except Exception:
            self.skipTest('requires write access to the installed location')
        unlink(filename)

    def test_write_pyfile(self):
        self.requiresWriteAccess(os.path.dirname(__file__))
        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
            fn = __file__
            if fn.endswith('.pyc'):
                path_split = fn.split(os.sep)
                if os.altsep is not None:
                    path_split.extend(fn.split(os.altsep))
                if '__pycache__' in path_split:
                    fn = importlib.util.source_from_cache(fn)
                else:
                    fn = fn[:-1]

            zipfp.writepy(fn)

            bn = os.path.basename(fn)
            self.assertNotIn(bn, zipfp.namelist())
            self.assertCompiledIn(bn, zipfp.namelist())

        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
            fn = __file__
            if fn.endswith('.pyc'):
                fn = fn[:-1]

            zipfp.writepy(fn, "testpackage")

            bn = "%s/%s" % ("testpackage", os.path.basename(fn))
            self.assertNotIn(bn, zipfp.namelist())
            self.assertCompiledIn(bn, zipfp.namelist())

    def test_write_python_package(self):
        import email
        packagedir = os.path.dirname(email.__file__)
        self.requiresWriteAccess(packagedir)

        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
            zipfp.writepy(packagedir)

            # Check for a couple of modules at different levels of the
            # hierarchy
            names = zipfp.namelist()
            self.assertCompiledIn('email/__init__.py', names)
            self.assertCompiledIn('email/mime/text.py', names)

    def test_write_filtered_python_package(self):
        import test
        packagedir = os.path.dirname(test.__file__)
        self.requiresWriteAccess(packagedir)

        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:

            # first make sure that the test folder gives error messages
            # (on the badsyntax_... files)
            with captured_stdout() as reportSIO:
                zipfp.writepy(packagedir)
            reportStr = reportSIO.getvalue()
            self.assertTrue('SyntaxError' in reportStr)

            # then check that the filter works on the whole package
            with captured_stdout() as reportSIO:
                zipfp.writepy(packagedir, filterfunc=lambda whatever: False)
            reportStr = reportSIO.getvalue()
            self.assertTrue('SyntaxError' not in reportStr)

            # then check that the filter works on individual files
            def filter(path):
                return not os.path.basename(path).startswith("bad")
            with captured_stdout() as reportSIO, self.assertWarns(UserWarning):
                zipfp.writepy(packagedir, filterfunc=filter)
            reportStr = reportSIO.getvalue()
            if reportStr:
                print(reportStr)
            self.assertTrue('SyntaxError' not in reportStr)

    def test_write_with_optimization(self):
        import email
        packagedir = os.path.dirname(email.__file__)
        self.requiresWriteAccess(packagedir)
        optlevel = 1 if __debug__ else 0
        ext = '.pyc'

        with TemporaryFile() as t, \
             zipfile.PyZipFile(t, "w", optimize=optlevel) as zipfp:
            zipfp.writepy(packagedir)

            names = zipfp.namelist()
            self.assertIn('email/__init__' + ext, names)
            self.assertIn('email/mime/text' + ext, names)

    def test_write_python_directory(self):
        os.mkdir(TESTFN2)
        try:
            with open(os.path.join(TESTFN2, "mod1.py"), "w", encoding='utf-8') as fp:
                fp.write("print(42)\n")

            with open(os.path.join(TESTFN2, "mod2.py"), "w", encoding='utf-8') as fp:
                fp.write("print(42 * 42)\n")

            with open(os.path.join(TESTFN2, "mod2.txt"), "w", encoding='utf-8') as fp:
                fp.write("bla bla bla\n")

            with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
                zipfp.writepy(TESTFN2)

                names = zipfp.namelist()
                self.assertCompiledIn('mod1.py', names)
                self.assertCompiledIn('mod2.py', names)
                self.assertNotIn('mod2.txt', names)

        finally:
            rmtree(TESTFN2)

    def test_write_python_directory_filtered(self):
        os.mkdir(TESTFN2)
        try:
            with open(os.path.join(TESTFN2, "mod1.py"), "w", encoding='utf-8') as fp:
                fp.write("print(42)\n")

            with open(os.path.join(TESTFN2, "mod2.py"), "w", encoding='utf-8') as fp:
                fp.write("print(42 * 42)\n")

            with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
                zipfp.writepy(TESTFN2, filterfunc=lambda fn:
                                                  not fn.endswith('mod2.py'))

                names = zipfp.namelist()
                self.assertCompiledIn('mod1.py', names)
                self.assertNotIn('mod2.py', names)

        finally:
            rmtree(TESTFN2)

    def test_write_non_pyfile(self):
        with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
            with open(TESTFN, 'w', encoding='utf-8') as f:
                f.write('most definitely not a python file')
            self.assertRaises(RuntimeError, zipfp.writepy, TESTFN)
            unlink(TESTFN)

    def test_write_pyfile_bad_syntax(self):
        os.mkdir(TESTFN2)
        try:
            with open(os.path.join(TESTFN2, "mod1.py"), "w", encoding='utf-8') as fp:
                fp.write("Bad syntax in python file\n")

            with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
                # syntax errors are printed to stdout
                with captured_stdout() as s:
                    zipfp.writepy(os.path.join(TESTFN2, "mod1.py"))

                self.assertIn("SyntaxError", s.getvalue())

                # as it will not have compiled the python file, it will
                # include the .py file not .pyc
                names = zipfp.namelist()
                self.assertIn('mod1.py', names)
                self.assertNotIn('mod1.pyc', names)

        finally:
            rmtree(TESTFN2)

    def test_write_pathlike(self):
        os.mkdir(TESTFN2)
        try:
            with open(os.path.join(TESTFN2, "mod1.py"), "w", encoding='utf-8') as fp:
                fp.write("print(42)\n")

            with TemporaryFile() as t, zipfile.PyZipFile(t, "w") as zipfp:
                zipfp.writepy(pathlib.Path(TESTFN2) / "mod1.py")
                names = zipfp.namelist()
                self.assertCompiledIn('mod1.py', names)
        finally:
            rmtree(TESTFN2)


class ExtractTests(unittest.TestCase):

    def make_test_file(self):
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            for fpath, fdata in SMALL_TEST_DATA:
                zipfp.writestr(fpath, fdata)

    def test_extract(self):
        with temp_cwd():
            self.make_test_file()
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

                    unlink(writtenfile)

    def _test_extract_with_target(self, target):
        self.make_test_file()
        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            for fpath, fdata in SMALL_TEST_DATA:
                writtenfile = zipfp.extract(fpath, target)

                # make sure it was written to the right place
                correctfile = os.path.join(target, fpath)
                correctfile = os.path.normpath(correctfile)
                self.assertTrue(os.path.samefile(writtenfile, correctfile), (writtenfile, target))

                # make sure correct data is in correct file
                with open(writtenfile, "rb") as f:
                    self.assertEqual(fdata.encode(), f.read())

                unlink(writtenfile)

        unlink(TESTFN2)

    def test_extract_with_target(self):
        with temp_dir() as extdir:
            self._test_extract_with_target(extdir)

    def test_extract_with_target_pathlike(self):
        with temp_dir() as extdir:
            self._test_extract_with_target(pathlib.Path(extdir))

    def test_extract_all(self):
        with temp_cwd():
            self.make_test_file()
            with zipfile.ZipFile(TESTFN2, "r") as zipfp:
                zipfp.extractall()
                for fpath, fdata in SMALL_TEST_DATA:
                    outfile = os.path.join(os.getcwd(), fpath)

                    with open(outfile, "rb") as f:
                        self.assertEqual(fdata.encode(), f.read())

                    unlink(outfile)

    def _test_extract_all_with_target(self, target):
        self.make_test_file()
        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            zipfp.extractall(target)
            for fpath, fdata in SMALL_TEST_DATA:
                outfile = os.path.join(target, fpath)

                with open(outfile, "rb") as f:
                    self.assertEqual(fdata.encode(), f.read())

                unlink(outfile)

        unlink(TESTFN2)

    def test_extract_all_with_target(self):
        with temp_dir() as extdir:
            self._test_extract_all_with_target(extdir)

    def test_extract_all_with_target_pathlike(self):
        with temp_dir() as extdir:
            self._test_extract_all_with_target(pathlib.Path(extdir))

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
        self.assertEqual(san('  /  /foo  /  /ba  r', '/'), r'foo/ba  r')
        self.assertEqual(san(' . /. /foo ./ . /. ./ba .r', '/'), r'foo/ba .r')

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
            rmtree('target')

            with zipfile.ZipFile(TESTFN2, 'r') as zipfp:
                zipfp.extractall(targetpath)
            self.check_file(correctfile, content)
            rmtree('target')

            correctfile = os.path.join(os.getcwd(), *fixedname.split('/'))

            with zipfile.ZipFile(TESTFN2, 'r') as zipfp:
                writtenfile = zipfp.extract(arcname)
                self.assertEqual(writtenfile, correctfile,
                                 msg="extract %r" % arcname)
            self.check_file(correctfile, content)
            rmtree(fixedname.split('/')[0])

            with zipfile.ZipFile(TESTFN2, 'r') as zipfp:
                zipfp.extractall()
            self.check_file(correctfile, content)
            rmtree(fixedname.split('/')[0])

            unlink(TESTFN2)


class OtherTests(unittest.TestCase):
    def test_open_via_zip_info(self):
        # Create the ZIP archive
        with zipfile.ZipFile(TESTFN2, "w", zipfile.ZIP_STORED) as zipfp:
            zipfp.writestr("name", "foo")
            with self.assertWarns(UserWarning):
                zipfp.writestr("name", "bar")
            self.assertEqual(zipfp.namelist(), ["name"] * 2)

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

    def test_writestr_extended_local_header_issue1202(self):
        with zipfile.ZipFile(TESTFN2, 'w') as orig_zip:
            for data in 'abcdefghijklmnop':
                zinfo = zipfile.ZipInfo(data)
                zinfo.flag_bits |= zipfile._MASK_USE_DATA_DESCRIPTOR  # Include an extended local header.
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

    @requires_zlib()
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

    def test_read_after_write_unicode_filenames(self):
        with zipfile.ZipFile(TESTFN2, 'w') as zipfp:
            zipfp.writestr('приклад', b'sample')
            self.assertEqual(zipfp.read('приклад'), b'sample')

    def test_exclusive_create_zip_file(self):
        """Test exclusive creating a new zipfile."""
        unlink(TESTFN2)
        filename = 'testfile.txt'
        content = b'hello, world. this is some content.'
        with zipfile.ZipFile(TESTFN2, "x", zipfile.ZIP_STORED) as zipfp:
            zipfp.writestr(filename, content)
        with self.assertRaises(FileExistsError):
            zipfile.ZipFile(TESTFN2, "x", zipfile.ZIP_STORED)
        with zipfile.ZipFile(TESTFN2, "r") as zipfp:
            self.assertEqual(zipfp.namelist(), [filename])
            self.assertEqual(zipfp.read(filename), content)

    def test_create_non_existent_file_for_append(self):
        if os.path.exists(TESTFN):
            os.unlink(TESTFN)

        filename = 'testfile.txt'
        content = b'hello, world. this is some content.'

        try:
            with zipfile.ZipFile(TESTFN, 'a') as zf:
                zf.writestr(filename, content)
        except OSError:
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
        with open(TESTFN, "w", encoding="utf-8") as fp:
            fp.write("this is not a legal zip file\n")
        try:
            zf = zipfile.ZipFile(TESTFN)
        except zipfile.BadZipFile:
            pass

    def test_is_zip_erroneous_file(self):
        """Check that is_zipfile() correctly identifies non-zip files."""
        # - passing a filename
        with open(TESTFN, "w", encoding='utf-8') as fp:
            fp.write("this is not a legal zip file\n")
        self.assertFalse(zipfile.is_zipfile(TESTFN))
        # - passing a path-like object
        self.assertFalse(zipfile.is_zipfile(pathlib.Path(TESTFN)))
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

    def test_non_existent_file_raises_OSError(self):
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
        self.assertRaises(OSError, zipfile.ZipFile, TESTFN)

    def test_empty_file_raises_BadZipFile(self):
        f = open(TESTFN, 'w', encoding='utf-8')
        f.close()
        self.assertRaises(zipfile.BadZipFile, zipfile.ZipFile, TESTFN)

        with open(TESTFN, 'w', encoding='utf-8') as fp:
            fp.write("short file")
        self.assertRaises(zipfile.BadZipFile, zipfile.ZipFile, TESTFN)

    def test_negative_central_directory_offset_raises_BadZipFile(self):
        # Zip file containing an empty EOCD record
        buffer = bytearray(b'PK\x05\x06' + b'\0'*18)

        # Set the size of the central directory bytes to become 1,
        # causing the central directory offset to become negative
        for dirsize in 1, 2**32-1:
            buffer[12:16] = struct.pack('<L', dirsize)
            f = io.BytesIO(buffer)
            self.assertRaises(zipfile.BadZipFile, zipfile.ZipFile, f)

    def test_closed_zip_raises_ValueError(self):
        """Verify that testzip() doesn't swallow inappropriate exceptions."""
        data = io.BytesIO()
        with zipfile.ZipFile(data, mode="w") as zipf:
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")

        # This is correct; calling .read on a closed ZipFile should raise
        # a ValueError, and so should calling .testzip.  An earlier
        # version of .testzip would swallow this exception (and any other)
        # and report that the first file in the archive was corrupt.
        self.assertRaises(ValueError, zipf.read, "foo.txt")
        self.assertRaises(ValueError, zipf.open, "foo.txt")
        self.assertRaises(ValueError, zipf.testzip)
        self.assertRaises(ValueError, zipf.writestr, "bogus.txt", "bogus")
        with open(TESTFN, 'w', encoding='utf-8') as f:
            f.write('zipfile test data')
        self.assertRaises(ValueError, zipf.write, TESTFN)

    def test_bad_constructor_mode(self):
        """Check that bad modes passed to ZipFile constructor are caught."""
        self.assertRaises(ValueError, zipfile.ZipFile, TESTFN, "q")

    def test_bad_open_mode(self):
        """Check that bad modes passed to ZipFile.open are caught."""
        with zipfile.ZipFile(TESTFN, mode="w") as zipf:
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")

        with zipfile.ZipFile(TESTFN, mode="r") as zipf:
            # read the data to make sure the file is there
            zipf.read("foo.txt")
            self.assertRaises(ValueError, zipf.open, "foo.txt", "q")
            # universal newlines support is removed
            self.assertRaises(ValueError, zipf.open, "foo.txt", "U")
            self.assertRaises(ValueError, zipf.open, "foo.txt", "rU")

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
        self.assertRaises(NotImplementedError, zipfile.ZipFile, TESTFN, "w", -1)

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
            with self.assertWarns(UserWarning):
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
        # and the file is indeed truncated
        with zipfile.ZipFile(TESTFN,mode="w") as zipf:
            zipf.comment = b"original comment that's longer"
            zipf.writestr("foo.txt", "O, for a Muse of Fire!")
        original_zip_size = os.path.getsize(TESTFN)
        with zipfile.ZipFile(TESTFN,mode="a") as zipf:
            zipf.comment = b"shorter comment"
        self.assertTrue(original_zip_size > os.path.getsize(TESTFN))
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
        # OSError)
        f = open(TESTFN, 'w', encoding='utf-8')
        f.close()
        self.assertRaises(zipfile.BadZipFile, zipfile.ZipFile, TESTFN, 'r')

    def test_create_zipinfo_before_1980(self):
        self.assertRaises(ValueError,
                          zipfile.ZipInfo, 'seventies', (1979, 1, 1, 0, 0, 0))

    def test_create_empty_zipinfo_repr(self):
        """Before bpo-26185, repr() on empty ZipInfo object was failing."""
        zi = zipfile.ZipInfo(filename="empty")
        self.assertEqual(repr(zi), "<ZipInfo filename='empty' file_size=0>")

    def test_create_empty_zipinfo_default_attributes(self):
        """Ensure all required attributes are set."""
        zi = zipfile.ZipInfo()
        self.assertEqual(zi.orig_filename, "NoName")
        self.assertEqual(zi.filename, "NoName")
        self.assertEqual(zi.date_time, (1980, 1, 1, 0, 0, 0))
        self.assertEqual(zi.compress_type, zipfile.ZIP_STORED)
        self.assertEqual(zi.comment, b"")
        self.assertEqual(zi.extra, b"")
        self.assertIn(zi.create_system, (0, 3))
        self.assertEqual(zi.create_version, zipfile.DEFAULT_VERSION)
        self.assertEqual(zi.extract_version, zipfile.DEFAULT_VERSION)
        self.assertEqual(zi.reserved, 0)
        self.assertEqual(zi.flag_bits, 0)
        self.assertEqual(zi.volume, 0)
        self.assertEqual(zi.internal_attr, 0)
        self.assertEqual(zi.external_attr, 0)

        # Before bpo-26185, both were missing
        self.assertEqual(zi.file_size, 0)
        self.assertEqual(zi.compress_size, 0)

    def test_zipfile_with_short_extra_field(self):
        """If an extra field in the header is less than 4 bytes, skip it."""
        zipdata = (
            b'PK\x03\x04\x14\x00\x00\x00\x00\x00\x93\x9b\xad@\x8b\x9e'
            b'\xd9\xd3\x01\x00\x00\x00\x01\x00\x00\x00\x03\x00\x03\x00ab'
            b'c\x00\x00\x00APK\x01\x02\x14\x03\x14\x00\x00\x00\x00'
            b'\x00\x93\x9b\xad@\x8b\x9e\xd9\xd3\x01\x00\x00\x00\x01\x00\x00'
            b'\x00\x03\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\xa4\x81\x00'
            b'\x00\x00\x00abc\x00\x00PK\x05\x06\x00\x00\x00\x00'
            b'\x01\x00\x01\x003\x00\x00\x00%\x00\x00\x00\x00\x00'
        )
        with zipfile.ZipFile(io.BytesIO(zipdata), 'r') as zipf:
            # testzip returns the name of the first corrupt file, or None
            self.assertIsNone(zipf.testzip())

    def test_open_conflicting_handles(self):
        # It's only possible to open one writable file handle at a time
        msg1 = b"It's fun to charter an accountant!"
        msg2 = b"And sail the wide accountant sea"
        msg3 = b"To find, explore the funds offshore"
        with zipfile.ZipFile(TESTFN2, 'w', zipfile.ZIP_STORED) as zipf:
            with zipf.open('foo', mode='w') as w2:
                w2.write(msg1)
            with zipf.open('bar', mode='w') as w1:
                with self.assertRaises(ValueError):
                    zipf.open('handle', mode='w')
                with self.assertRaises(ValueError):
                    zipf.open('foo', mode='r')
                with self.assertRaises(ValueError):
                    zipf.writestr('str', 'abcde')
                with self.assertRaises(ValueError):
                    zipf.write(__file__, 'file')
                with self.assertRaises(ValueError):
                    zipf.close()
                w1.write(msg2)
            with zipf.open('baz', mode='w') as w2:
                w2.write(msg3)

        with zipfile.ZipFile(TESTFN2, 'r') as zipf:
            self.assertEqual(zipf.read('foo'), msg1)
            self.assertEqual(zipf.read('bar'), msg2)
            self.assertEqual(zipf.read('baz'), msg3)
            self.assertEqual(zipf.namelist(), ['foo', 'bar', 'baz'])

    def test_seek_tell(self):
        # Test seek functionality
        txt = b"Where's Bruce?"
        bloc = txt.find(b"Bruce")
        # Check seek on a file
        with zipfile.ZipFile(TESTFN, "w") as zipf:
            zipf.writestr("foo.txt", txt)
        with zipfile.ZipFile(TESTFN, "r") as zipf:
            with zipf.open("foo.txt", "r") as fp:
                fp.seek(bloc, os.SEEK_SET)
                self.assertEqual(fp.tell(), bloc)
                fp.seek(-bloc, os.SEEK_CUR)
                self.assertEqual(fp.tell(), 0)
                fp.seek(bloc, os.SEEK_CUR)
                self.assertEqual(fp.tell(), bloc)
                self.assertEqual(fp.read(5), txt[bloc:bloc+5])
                self.assertEqual(fp.tell(), bloc + 5)
                fp.seek(0, os.SEEK_END)
                self.assertEqual(fp.tell(), len(txt))
                fp.seek(0, os.SEEK_SET)
                self.assertEqual(fp.tell(), 0)
        # Check seek on memory file
        data = io.BytesIO()
        with zipfile.ZipFile(data, mode="w") as zipf:
            zipf.writestr("foo.txt", txt)
        with zipfile.ZipFile(data, mode="r") as zipf:
            with zipf.open("foo.txt", "r") as fp:
                fp.seek(bloc, os.SEEK_SET)
                self.assertEqual(fp.tell(), bloc)
                fp.seek(-bloc, os.SEEK_CUR)
                self.assertEqual(fp.tell(), 0)
                fp.seek(bloc, os.SEEK_CUR)
                self.assertEqual(fp.tell(), bloc)
                self.assertEqual(fp.read(5), txt[bloc:bloc+5])
                self.assertEqual(fp.tell(), bloc + 5)
                fp.seek(0, os.SEEK_END)
                self.assertEqual(fp.tell(), len(txt))
                fp.seek(0, os.SEEK_SET)
                self.assertEqual(fp.tell(), 0)

    @requires_bz2()
    def test_decompress_without_3rd_party_library(self):
        data = b'PK\x05\x06\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        zip_file = io.BytesIO(data)
        with zipfile.ZipFile(zip_file, 'w', compression=zipfile.ZIP_BZIP2) as zf:
            zf.writestr('a.txt', b'a')
        with mock.patch('zipfile.bz2', None):
            with zipfile.ZipFile(zip_file) as zf:
                self.assertRaises(RuntimeError, zf.extract, 'a.txt')

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

@requires_zlib()
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

@requires_bz2()
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

@requires_lzma()
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

    @requires_zlib()
    def test_good_password(self):
        self.zip.setpassword(b"python")
        self.assertEqual(self.zip.read("test.txt"), self.plain)
        self.zip2.setpassword(b"12345")
        self.assertEqual(self.zip2.read("zero"), self.plain2)

    def test_unicode_password(self):
        expected_msg = "pwd: expected bytes, got str"

        with self.assertRaisesRegex(TypeError, expected_msg):
            self.zip.setpassword("unicode")

        with self.assertRaisesRegex(TypeError, expected_msg):
            self.zip.read("test.txt", "python")

        with self.assertRaisesRegex(TypeError, expected_msg):
            self.zip.open("test.txt", pwd="python")

        with self.assertRaisesRegex(TypeError, expected_msg):
            self.zip.extract("test.txt", pwd="python")

        with self.assertRaisesRegex(TypeError, expected_msg):
            self.zip.pwd = "python"
            self.zip.open("test.txt")

    def test_seek_tell(self):
        self.zip.setpassword(b"python")
        txt = self.plain
        test_word = b'encryption'
        bloc = txt.find(test_word)
        bloc_len = len(test_word)
        with self.zip.open("test.txt", "r") as fp:
            fp.seek(bloc, os.SEEK_SET)
            self.assertEqual(fp.tell(), bloc)
            fp.seek(-bloc, os.SEEK_CUR)
            self.assertEqual(fp.tell(), 0)
            fp.seek(bloc, os.SEEK_CUR)
            self.assertEqual(fp.tell(), bloc)
            self.assertEqual(fp.read(bloc_len), txt[bloc:bloc+bloc_len])

            # Make sure that the second read after seeking back beyond
            # _readbuffer returns the same content (ie. rewind to the start of
            # the file to read forward to the required position).
            old_read_size = fp.MIN_READ_SIZE
            fp.MIN_READ_SIZE = 1
            fp._readbuffer = b''
            fp._offset = 0
            fp.seek(0, os.SEEK_SET)
            self.assertEqual(fp.tell(), 0)
            fp.seek(bloc, os.SEEK_CUR)
            self.assertEqual(fp.read(bloc_len), txt[bloc:bloc+bloc_len])
            fp.MIN_READ_SIZE = old_read_size

            fp.seek(0, os.SEEK_END)
            self.assertEqual(fp.tell(), len(txt))
            fp.seek(0, os.SEEK_SET)
            self.assertEqual(fp.tell(), 0)

            # Read the file completely to definitely call any eof integrity
            # checks (crc) and make sure they still pass.
            fp.read()


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

@requires_zlib()
class DeflateTestsWithRandomBinaryFiles(AbstractTestsWithRandomBinaryFiles,
                                        unittest.TestCase):
    compression = zipfile.ZIP_DEFLATED

@requires_bz2()
class Bzip2TestsWithRandomBinaryFiles(AbstractTestsWithRandomBinaryFiles,
                                      unittest.TestCase):
    compression = zipfile.ZIP_BZIP2

@requires_lzma()
class LzmaTestsWithRandomBinaryFiles(AbstractTestsWithRandomBinaryFiles,
                                     unittest.TestCase):
    compression = zipfile.ZIP_LZMA


# Provide the tell() method but not seek()
class Tellable:
    def __init__(self, fp):
        self.fp = fp
        self.offset = 0

    def write(self, data):
        n = self.fp.write(data)
        self.offset += n
        return n

    def tell(self):
        return self.offset

    def flush(self):
        self.fp.flush()

class Unseekable:
    def __init__(self, fp):
        self.fp = fp

    def write(self, data):
        return self.fp.write(data)

    def flush(self):
        self.fp.flush()

class UnseekableTests(unittest.TestCase):
    def test_writestr(self):
        for wrapper in (lambda f: f), Tellable, Unseekable:
            with self.subTest(wrapper=wrapper):
                f = io.BytesIO()
                f.write(b'abc')
                bf = io.BufferedWriter(f)
                with zipfile.ZipFile(wrapper(bf), 'w', zipfile.ZIP_STORED) as zipfp:
                    zipfp.writestr('ones', b'111')
                    zipfp.writestr('twos', b'222')
                self.assertEqual(f.getvalue()[:5], b'abcPK')
                with zipfile.ZipFile(f, mode='r') as zipf:
                    with zipf.open('ones') as zopen:
                        self.assertEqual(zopen.read(), b'111')
                    with zipf.open('twos') as zopen:
                        self.assertEqual(zopen.read(), b'222')

    def test_write(self):
        for wrapper in (lambda f: f), Tellable, Unseekable:
            with self.subTest(wrapper=wrapper):
                f = io.BytesIO()
                f.write(b'abc')
                bf = io.BufferedWriter(f)
                with zipfile.ZipFile(wrapper(bf), 'w', zipfile.ZIP_STORED) as zipfp:
                    self.addCleanup(unlink, TESTFN)
                    with open(TESTFN, 'wb') as f2:
                        f2.write(b'111')
                    zipfp.write(TESTFN, 'ones')
                    with open(TESTFN, 'wb') as f2:
                        f2.write(b'222')
                    zipfp.write(TESTFN, 'twos')
                self.assertEqual(f.getvalue()[:5], b'abcPK')
                with zipfile.ZipFile(f, mode='r') as zipf:
                    with zipf.open('ones') as zopen:
                        self.assertEqual(zopen.read(), b'111')
                    with zipf.open('twos') as zopen:
                        self.assertEqual(zopen.read(), b'222')

    def test_open_write(self):
        for wrapper in (lambda f: f), Tellable, Unseekable:
            with self.subTest(wrapper=wrapper):
                f = io.BytesIO()
                f.write(b'abc')
                bf = io.BufferedWriter(f)
                with zipfile.ZipFile(wrapper(bf), 'w', zipfile.ZIP_STORED) as zipf:
                    with zipf.open('ones', 'w') as zopen:
                        zopen.write(b'111')
                    with zipf.open('twos', 'w') as zopen:
                        zopen.write(b'222')
                self.assertEqual(f.getvalue()[:5], b'abcPK')
                with zipfile.ZipFile(f) as zipf:
                    self.assertEqual(zipf.read('ones'), b'111')
                    self.assertEqual(zipf.read('twos'), b'222')


@requires_zlib()
class TestsWithMultipleOpens(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data1 = b'111' + randbytes(10000)
        cls.data2 = b'222' + randbytes(10000)

    def make_test_archive(self, f):
        # Create the ZIP archive
        with zipfile.ZipFile(f, "w", zipfile.ZIP_DEFLATED) as zipfp:
            zipfp.writestr('ones', self.data1)
            zipfp.writestr('twos', self.data2)

    def test_same_file(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        for f in get_files(self):
            self.make_test_archive(f)
            with zipfile.ZipFile(f, mode="r") as zipf:
                with zipf.open('ones') as zopen1, zipf.open('ones') as zopen2:
                    data1 = zopen1.read(500)
                    data2 = zopen2.read(500)
                    data1 += zopen1.read()
                    data2 += zopen2.read()
                self.assertEqual(data1, data2)
                self.assertEqual(data1, self.data1)

    def test_different_file(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        for f in get_files(self):
            self.make_test_archive(f)
            with zipfile.ZipFile(f, mode="r") as zipf:
                with zipf.open('ones') as zopen1, zipf.open('twos') as zopen2:
                    data1 = zopen1.read(500)
                    data2 = zopen2.read(500)
                    data1 += zopen1.read()
                    data2 += zopen2.read()
                self.assertEqual(data1, self.data1)
                self.assertEqual(data2, self.data2)

    def test_interleaved(self):
        # Verify that (when the ZipFile is in control of creating file objects)
        # multiple open() calls can be made without interfering with each other.
        for f in get_files(self):
            self.make_test_archive(f)
            with zipfile.ZipFile(f, mode="r") as zipf:
                with zipf.open('ones') as zopen1:
                    data1 = zopen1.read(500)
                    with zipf.open('twos') as zopen2:
                        data2 = zopen2.read(500)
                        data1 += zopen1.read()
                        data2 += zopen2.read()
                self.assertEqual(data1, self.data1)
                self.assertEqual(data2, self.data2)

    def test_read_after_close(self):
        for f in get_files(self):
            self.make_test_archive(f)
            with contextlib.ExitStack() as stack:
                with zipfile.ZipFile(f, 'r') as zipf:
                    zopen1 = stack.enter_context(zipf.open('ones'))
                    zopen2 = stack.enter_context(zipf.open('twos'))
                data1 = zopen1.read(500)
                data2 = zopen2.read(500)
                data1 += zopen1.read()
                data2 += zopen2.read()
            self.assertEqual(data1, self.data1)
            self.assertEqual(data2, self.data2)

    def test_read_after_write(self):
        for f in get_files(self):
            with zipfile.ZipFile(f, 'w', zipfile.ZIP_DEFLATED) as zipf:
                zipf.writestr('ones', self.data1)
                zipf.writestr('twos', self.data2)
                with zipf.open('ones') as zopen1:
                    data1 = zopen1.read(500)
            self.assertEqual(data1, self.data1[:500])
            with zipfile.ZipFile(f, 'r') as zipf:
                data1 = zipf.read('ones')
                data2 = zipf.read('twos')
            self.assertEqual(data1, self.data1)
            self.assertEqual(data2, self.data2)

    def test_write_after_read(self):
        for f in get_files(self):
            with zipfile.ZipFile(f, "w", zipfile.ZIP_DEFLATED) as zipf:
                zipf.writestr('ones', self.data1)
                with zipf.open('ones') as zopen1:
                    zopen1.read(500)
                    zipf.writestr('twos', self.data2)
            with zipfile.ZipFile(f, 'r') as zipf:
                data1 = zipf.read('ones')
                data2 = zipf.read('twos')
            self.assertEqual(data1, self.data1)
            self.assertEqual(data2, self.data2)

    def test_many_opens(self):
        # Verify that read() and open() promptly close the file descriptor,
        # and don't rely on the garbage collector to free resources.
        startcount = fd_count()
        self.make_test_archive(TESTFN2)
        with zipfile.ZipFile(TESTFN2, mode="r") as zipf:
            for x in range(100):
                zipf.read('ones')
                with zipf.open('ones') as zopen1:
                    pass
        self.assertEqual(startcount, fd_count())

    def test_write_while_reading(self):
        with zipfile.ZipFile(TESTFN2, 'w', zipfile.ZIP_DEFLATED) as zipf:
            zipf.writestr('ones', self.data1)
        with zipfile.ZipFile(TESTFN2, 'a', zipfile.ZIP_DEFLATED) as zipf:
            with zipf.open('ones', 'r') as r1:
                data1 = r1.read(500)
                with zipf.open('twos', 'w') as w1:
                    w1.write(self.data2)
                data1 += r1.read()
        self.assertEqual(data1, self.data1)
        with zipfile.ZipFile(TESTFN2) as zipf:
            self.assertEqual(zipf.read('twos'), self.data2)

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

    def test_write_dir(self):
        dirpath = os.path.join(TESTFN2, "x")
        os.mkdir(dirpath)
        mode = os.stat(dirpath).st_mode & 0xFFFF
        with zipfile.ZipFile(TESTFN, "w") as zipf:
            zipf.write(dirpath)
            zinfo = zipf.filelist[0]
            self.assertTrue(zinfo.filename.endswith("/x/"))
            self.assertEqual(zinfo.external_attr, (mode << 16) | 0x10)
            zipf.write(dirpath, "y")
            zinfo = zipf.filelist[1]
            self.assertTrue(zinfo.filename, "y/")
            self.assertEqual(zinfo.external_attr, (mode << 16) | 0x10)
        with zipfile.ZipFile(TESTFN, "r") as zipf:
            zinfo = zipf.filelist[0]
            self.assertTrue(zinfo.filename.endswith("/x/"))
            self.assertEqual(zinfo.external_attr, (mode << 16) | 0x10)
            zinfo = zipf.filelist[1]
            self.assertTrue(zinfo.filename, "y/")
            self.assertEqual(zinfo.external_attr, (mode << 16) | 0x10)
            target = os.path.join(TESTFN2, "target")
            os.mkdir(target)
            zipf.extractall(target)
            self.assertTrue(os.path.isdir(os.path.join(target, "y")))
            self.assertEqual(len(os.listdir(target)), 2)

    def test_writestr_dir(self):
        os.mkdir(os.path.join(TESTFN2, "x"))
        with zipfile.ZipFile(TESTFN, "w") as zipf:
            zipf.writestr("x/", b'')
            zinfo = zipf.filelist[0]
            self.assertEqual(zinfo.filename, "x/")
            self.assertEqual(zinfo.external_attr, (0o40775 << 16) | 0x10)
        with zipfile.ZipFile(TESTFN, "r") as zipf:
            zinfo = zipf.filelist[0]
            self.assertTrue(zinfo.filename.endswith("x/"))
            self.assertEqual(zinfo.external_attr, (0o40775 << 16) | 0x10)
            target = os.path.join(TESTFN2, "target")
            os.mkdir(target)
            zipf.extractall(target)
            self.assertTrue(os.path.isdir(os.path.join(target, "x")))
            self.assertEqual(os.listdir(target), ["x"])

    def test_mkdir(self):
        with zipfile.ZipFile(TESTFN, "w") as zf:
            zf.mkdir("directory")
            zinfo = zf.filelist[0]
            self.assertEqual(zinfo.filename, "directory/")
            self.assertEqual(zinfo.external_attr, (0o40777 << 16) | 0x10)

            zf.mkdir("directory2/")
            zinfo = zf.filelist[1]
            self.assertEqual(zinfo.filename, "directory2/")
            self.assertEqual(zinfo.external_attr, (0o40777 << 16) | 0x10)

            zf.mkdir("directory3", mode=0o777)
            zinfo = zf.filelist[2]
            self.assertEqual(zinfo.filename, "directory3/")
            self.assertEqual(zinfo.external_attr, (0o40777 << 16) | 0x10)

            old_zinfo = zipfile.ZipInfo("directory4/")
            old_zinfo.external_attr = (0o40777 << 16) | 0x10
            old_zinfo.CRC = 0
            old_zinfo.file_size = 0
            old_zinfo.compress_size = 0
            zf.mkdir(old_zinfo)
            new_zinfo = zf.filelist[3]
            self.assertEqual(old_zinfo.filename, "directory4/")
            self.assertEqual(old_zinfo.external_attr, new_zinfo.external_attr)

            target = os.path.join(TESTFN2, "target")
            os.mkdir(target)
            zf.extractall(target)
            self.assertEqual(set(os.listdir(target)), {"directory", "directory2", "directory3", "directory4"})

    def test_create_directory_with_write(self):
        with zipfile.ZipFile(TESTFN, "w") as zf:
            zf.writestr(zipfile.ZipInfo('directory/'), '')

            zinfo = zf.filelist[0]
            self.assertEqual(zinfo.filename, "directory/")

            directory = os.path.join(TESTFN2, "directory2")
            os.mkdir(directory)
            mode = os.stat(directory).st_mode
            zf.write(directory, arcname="directory2/")
            zinfo = zf.filelist[1]
            self.assertEqual(zinfo.filename, "directory2/")
            self.assertEqual(zinfo.external_attr, (mode << 16) | 0x10)

            target = os.path.join(TESTFN2, "target")
            os.mkdir(target)
            zf.extractall(target)

            self.assertEqual(set(os.listdir(target)), {"directory", "directory2"})

    def tearDown(self):
        rmtree(TESTFN2)
        if os.path.exists(TESTFN):
            unlink(TESTFN)


class ZipInfoTests(unittest.TestCase):
    def test_from_file(self):
        zi = zipfile.ZipInfo.from_file(__file__)
        self.assertEqual(posixpath.basename(zi.filename), 'test_zipfile.py')
        self.assertFalse(zi.is_dir())
        self.assertEqual(zi.file_size, os.path.getsize(__file__))

    def test_from_file_pathlike(self):
        zi = zipfile.ZipInfo.from_file(pathlib.Path(__file__))
        self.assertEqual(posixpath.basename(zi.filename), 'test_zipfile.py')
        self.assertFalse(zi.is_dir())
        self.assertEqual(zi.file_size, os.path.getsize(__file__))

    def test_from_file_bytes(self):
        zi = zipfile.ZipInfo.from_file(os.fsencode(__file__), 'test')
        self.assertEqual(posixpath.basename(zi.filename), 'test')
        self.assertFalse(zi.is_dir())
        self.assertEqual(zi.file_size, os.path.getsize(__file__))

    def test_from_file_fileno(self):
        with open(__file__, 'rb') as f:
            zi = zipfile.ZipInfo.from_file(f.fileno(), 'test')
            self.assertEqual(posixpath.basename(zi.filename), 'test')
            self.assertFalse(zi.is_dir())
            self.assertEqual(zi.file_size, os.path.getsize(__file__))

    def test_from_dir(self):
        dirpath = os.path.dirname(os.path.abspath(__file__))
        zi = zipfile.ZipInfo.from_file(dirpath, 'stdlib_tests')
        self.assertEqual(zi.filename, 'stdlib_tests/')
        self.assertTrue(zi.is_dir())
        self.assertEqual(zi.compress_type, zipfile.ZIP_STORED)
        self.assertEqual(zi.file_size, 0)


class CommandLineTest(unittest.TestCase):

    def zipfilecmd(self, *args, **kwargs):
        rc, out, err = script_helper.assert_python_ok('-m', 'zipfile', *args,
                                                      **kwargs)
        return out.replace(os.linesep.encode(), b'\n')

    def zipfilecmd_failure(self, *args):
        return script_helper.assert_python_failure('-m', 'zipfile', *args)

    def test_bad_use(self):
        rc, out, err = self.zipfilecmd_failure()
        self.assertEqual(out, b'')
        self.assertIn(b'usage', err.lower())
        self.assertIn(b'error', err.lower())
        self.assertIn(b'required', err.lower())
        rc, out, err = self.zipfilecmd_failure('-l', '')
        self.assertEqual(out, b'')
        self.assertNotEqual(err.strip(), b'')

    def test_test_command(self):
        zip_name = findfile('zipdir.zip')
        for opt in '-t', '--test':
            out = self.zipfilecmd(opt, zip_name)
            self.assertEqual(out.rstrip(), b'Done testing')
        zip_name = findfile('testtar.tar')
        rc, out, err = self.zipfilecmd_failure('-t', zip_name)
        self.assertEqual(out, b'')

    def test_list_command(self):
        zip_name = findfile('zipdir.zip')
        t = io.StringIO()
        with zipfile.ZipFile(zip_name, 'r') as tf:
            tf.printdir(t)
        expected = t.getvalue().encode('ascii', 'backslashreplace')
        for opt in '-l', '--list':
            out = self.zipfilecmd(opt, zip_name,
                                  PYTHONIOENCODING='ascii:backslashreplace')
            self.assertEqual(out, expected)

    @requires_zlib()
    def test_create_command(self):
        self.addCleanup(unlink, TESTFN)
        with open(TESTFN, 'w', encoding='utf-8') as f:
            f.write('test 1')
        os.mkdir(TESTFNDIR)
        self.addCleanup(rmtree, TESTFNDIR)
        with open(os.path.join(TESTFNDIR, 'file.txt'), 'w', encoding='utf-8') as f:
            f.write('test 2')
        files = [TESTFN, TESTFNDIR]
        namelist = [TESTFN, TESTFNDIR + '/', TESTFNDIR + '/file.txt']
        for opt in '-c', '--create':
            try:
                out = self.zipfilecmd(opt, TESTFN2, *files)
                self.assertEqual(out, b'')
                with zipfile.ZipFile(TESTFN2) as zf:
                    self.assertEqual(zf.namelist(), namelist)
                    self.assertEqual(zf.read(namelist[0]), b'test 1')
                    self.assertEqual(zf.read(namelist[2]), b'test 2')
            finally:
                unlink(TESTFN2)

    def test_extract_command(self):
        zip_name = findfile('zipdir.zip')
        for opt in '-e', '--extract':
            with temp_dir() as extdir:
                out = self.zipfilecmd(opt, zip_name, extdir)
                self.assertEqual(out, b'')
                with zipfile.ZipFile(zip_name) as zf:
                    for zi in zf.infolist():
                        path = os.path.join(extdir,
                                    zi.filename.replace('/', os.sep))
                        if zi.is_dir():
                            self.assertTrue(os.path.isdir(path))
                        else:
                            self.assertTrue(os.path.isfile(path))
                            with open(path, 'rb') as f:
                                self.assertEqual(f.read(), zf.read(zi))


class TestExecutablePrependedZip(unittest.TestCase):
    """Test our ability to open zip files with an executable prepended."""

    def setUp(self):
        self.exe_zip = findfile('exe_with_zip', subdir='ziptestdata')
        self.exe_zip64 = findfile('exe_with_z64', subdir='ziptestdata')

    def _test_zip_works(self, name):
        # bpo28494 sanity check: ensure is_zipfile works on these.
        self.assertTrue(zipfile.is_zipfile(name),
                        f'is_zipfile failed on {name}')
        # Ensure we can operate on these via ZipFile.
        with zipfile.ZipFile(name) as zipfp:
            for n in zipfp.namelist():
                data = zipfp.read(n)
                self.assertIn(b'FAVORITE_NUMBER', data)

    def test_read_zip_with_exe_prepended(self):
        self._test_zip_works(self.exe_zip)

    def test_read_zip64_with_exe_prepended(self):
        self._test_zip_works(self.exe_zip64)

    @unittest.skipUnless(sys.executable, 'sys.executable required.')
    @unittest.skipUnless(os.access('/bin/bash', os.X_OK),
                         'Test relies on #!/bin/bash working.')
    @requires_subprocess()
    def test_execute_zip2(self):
        output = subprocess.check_output([self.exe_zip, sys.executable])
        self.assertIn(b'number in executable: 5', output)

    @unittest.skipUnless(sys.executable, 'sys.executable required.')
    @unittest.skipUnless(os.access('/bin/bash', os.X_OK),
                         'Test relies on #!/bin/bash working.')
    @requires_subprocess()
    def test_execute_zip64(self):
        output = subprocess.check_output([self.exe_zip64, sys.executable])
        self.assertIn(b'number in executable: 5', output)


# Poor man's technique to consume a (smallish) iterable.
consume = tuple


# from jaraco.itertools 5.0
class jaraco:
    class itertools:
        class Counter:
            def __init__(self, i):
                self.count = 0
                self._orig_iter = iter(i)

            def __iter__(self):
                return self

            def __next__(self):
                result = next(self._orig_iter)
                self.count += 1
                return result


def add_dirs(zf):
    """
    Given a writable zip file zf, inject directory entries for
    any directories implied by the presence of children.
    """
    for name in zipfile.CompleteDirs._implied_dirs(zf.namelist()):
        zf.writestr(name, b"")
    return zf


def build_alpharep_fixture():
    """
    Create a zip file with this structure:

    .
    ├── a.txt
    ├── b
    │   ├── c.txt
    │   ├── d
    │   │   └── e.txt
    │   └── f.txt
    └── g
        └── h
            └── i.txt

    This fixture has the following key characteristics:

    - a file at the root (a)
    - a file two levels deep (b/d/e)
    - multiple files in a directory (b/c, b/f)
    - a directory containing only a directory (g/h)

    "alpha" because it uses alphabet
    "rep" because it's a representative example
    """
    data = io.BytesIO()
    zf = zipfile.ZipFile(data, "w")
    zf.writestr("a.txt", b"content of a")
    zf.writestr("b/c.txt", b"content of c")
    zf.writestr("b/d/e.txt", b"content of e")
    zf.writestr("b/f.txt", b"content of f")
    zf.writestr("g/h/i.txt", b"content of i")
    zf.filename = "alpharep.zip"
    return zf


def pass_alpharep(meth):
    """
    Given a method, wrap it in a for loop that invokes method
    with each subtest.
    """

    @functools.wraps(meth)
    def wrapper(self):
        for alpharep in self.zipfile_alpharep():
            meth(self, alpharep=alpharep)

    return wrapper


class TestPath(unittest.TestCase):
    def setUp(self):
        self.fixtures = contextlib.ExitStack()
        self.addCleanup(self.fixtures.close)

    def zipfile_alpharep(self):
        with self.subTest():
            yield build_alpharep_fixture()
        with self.subTest():
            yield add_dirs(build_alpharep_fixture())

    def zipfile_ondisk(self, alpharep):
        tmpdir = pathlib.Path(self.fixtures.enter_context(temp_dir()))
        buffer = alpharep.fp
        alpharep.close()
        path = tmpdir / alpharep.filename
        with path.open("wb") as strm:
            strm.write(buffer.getvalue())
        return path

    @pass_alpharep
    def test_iterdir_and_types(self, alpharep):
        root = zipfile.Path(alpharep)
        assert root.is_dir()
        a, b, g = root.iterdir()
        assert a.is_file()
        assert b.is_dir()
        assert g.is_dir()
        c, f, d = b.iterdir()
        assert c.is_file() and f.is_file()
        (e,) = d.iterdir()
        assert e.is_file()
        (h,) = g.iterdir()
        (i,) = h.iterdir()
        assert i.is_file()

    @pass_alpharep
    def test_is_file_missing(self, alpharep):
        root = zipfile.Path(alpharep)
        assert not root.joinpath('missing.txt').is_file()

    @pass_alpharep
    def test_iterdir_on_file(self, alpharep):
        root = zipfile.Path(alpharep)
        a, b, g = root.iterdir()
        with self.assertRaises(ValueError):
            a.iterdir()

    @pass_alpharep
    def test_subdir_is_dir(self, alpharep):
        root = zipfile.Path(alpharep)
        assert (root / 'b').is_dir()
        assert (root / 'b/').is_dir()
        assert (root / 'g').is_dir()
        assert (root / 'g/').is_dir()

    @pass_alpharep
    def test_open(self, alpharep):
        root = zipfile.Path(alpharep)
        a, b, g = root.iterdir()
        with a.open(encoding="utf-8") as strm:
            data = strm.read()
        assert data == "content of a"

    def test_open_write(self):
        """
        If the zipfile is open for write, it should be possible to
        write bytes or text to it.
        """
        zf = zipfile.Path(zipfile.ZipFile(io.BytesIO(), mode='w'))
        with zf.joinpath('file.bin').open('wb') as strm:
            strm.write(b'binary contents')
        with zf.joinpath('file.txt').open('w', encoding="utf-8") as strm:
            strm.write('text file')

    def test_open_extant_directory(self):
        """
        Attempting to open a directory raises IsADirectoryError.
        """
        zf = zipfile.Path(add_dirs(build_alpharep_fixture()))
        with self.assertRaises(IsADirectoryError):
            zf.joinpath('b').open()

    @pass_alpharep
    def test_open_binary_invalid_args(self, alpharep):
        root = zipfile.Path(alpharep)
        with self.assertRaises(ValueError):
            root.joinpath('a.txt').open('rb', encoding='utf-8')
        with self.assertRaises(ValueError):
            root.joinpath('a.txt').open('rb', 'utf-8')

    def test_open_missing_directory(self):
        """
        Attempting to open a missing directory raises FileNotFoundError.
        """
        zf = zipfile.Path(add_dirs(build_alpharep_fixture()))
        with self.assertRaises(FileNotFoundError):
            zf.joinpath('z').open()

    @pass_alpharep
    def test_read(self, alpharep):
        root = zipfile.Path(alpharep)
        a, b, g = root.iterdir()
        assert a.read_text(encoding="utf-8") == "content of a"
        assert a.read_bytes() == b"content of a"

    @pass_alpharep
    def test_joinpath(self, alpharep):
        root = zipfile.Path(alpharep)
        a = root.joinpath("a.txt")
        assert a.is_file()
        e = root.joinpath("b").joinpath("d").joinpath("e.txt")
        assert e.read_text(encoding="utf-8") == "content of e"

    @pass_alpharep
    def test_joinpath_multiple(self, alpharep):
        root = zipfile.Path(alpharep)
        e = root.joinpath("b", "d", "e.txt")
        assert e.read_text(encoding="utf-8") == "content of e"

    @pass_alpharep
    def test_traverse_truediv(self, alpharep):
        root = zipfile.Path(alpharep)
        a = root / "a.txt"
        assert a.is_file()
        e = root / "b" / "d" / "e.txt"
        assert e.read_text(encoding="utf-8") == "content of e"

    @pass_alpharep
    def test_traverse_simplediv(self, alpharep):
        """
        Disable the __future__.division when testing traversal.
        """
        code = compile(
            source="zipfile.Path(alpharep) / 'a'",
            filename="(test)",
            mode="eval",
            dont_inherit=True,
        )
        eval(code)

    @pass_alpharep
    def test_pathlike_construction(self, alpharep):
        """
        zipfile.Path should be constructable from a path-like object
        """
        zipfile_ondisk = self.zipfile_ondisk(alpharep)
        pathlike = pathlib.Path(str(zipfile_ondisk))
        zipfile.Path(pathlike)

    @pass_alpharep
    def test_traverse_pathlike(self, alpharep):
        root = zipfile.Path(alpharep)
        root / pathlib.Path("a")

    @pass_alpharep
    def test_parent(self, alpharep):
        root = zipfile.Path(alpharep)
        assert (root / 'a').parent.at == ''
        assert (root / 'a' / 'b').parent.at == 'a/'

    @pass_alpharep
    def test_dir_parent(self, alpharep):
        root = zipfile.Path(alpharep)
        assert (root / 'b').parent.at == ''
        assert (root / 'b/').parent.at == ''

    @pass_alpharep
    def test_missing_dir_parent(self, alpharep):
        root = zipfile.Path(alpharep)
        assert (root / 'missing dir/').parent.at == ''

    @pass_alpharep
    def test_mutability(self, alpharep):
        """
        If the underlying zipfile is changed, the Path object should
        reflect that change.
        """
        root = zipfile.Path(alpharep)
        a, b, g = root.iterdir()
        alpharep.writestr('foo.txt', 'foo')
        alpharep.writestr('bar/baz.txt', 'baz')
        assert any(child.name == 'foo.txt' for child in root.iterdir())
        assert (root / 'foo.txt').read_text(encoding="utf-8") == 'foo'
        (baz,) = (root / 'bar').iterdir()
        assert baz.read_text(encoding="utf-8") == 'baz'

    HUGE_ZIPFILE_NUM_ENTRIES = 2 ** 13

    def huge_zipfile(self):
        """Create a read-only zipfile with a huge number of entries entries."""
        strm = io.BytesIO()
        zf = zipfile.ZipFile(strm, "w")
        for entry in map(str, range(self.HUGE_ZIPFILE_NUM_ENTRIES)):
            zf.writestr(entry, entry)
        zf.mode = 'r'
        return zf

    def test_joinpath_constant_time(self):
        """
        Ensure joinpath on items in zipfile is linear time.
        """
        root = zipfile.Path(self.huge_zipfile())
        entries = jaraco.itertools.Counter(root.iterdir())
        for entry in entries:
            entry.joinpath('suffix')
        # Check the file iterated all items
        assert entries.count == self.HUGE_ZIPFILE_NUM_ENTRIES

    # @func_timeout.func_set_timeout(3)
    def test_implied_dirs_performance(self):
        data = ['/'.join(string.ascii_lowercase + str(n)) for n in range(10000)]
        zipfile.CompleteDirs._implied_dirs(data)

    @pass_alpharep
    def test_read_does_not_close(self, alpharep):
        alpharep = self.zipfile_ondisk(alpharep)
        with zipfile.ZipFile(alpharep) as file:
            for rep in range(2):
                zipfile.Path(file, 'a.txt').read_text(encoding="utf-8")

    @pass_alpharep
    def test_subclass(self, alpharep):
        class Subclass(zipfile.Path):
            pass

        root = Subclass(alpharep)
        assert isinstance(root / 'b', Subclass)

    @pass_alpharep
    def test_filename(self, alpharep):
        root = zipfile.Path(alpharep)
        assert root.filename == pathlib.Path('alpharep.zip')

    @pass_alpharep
    def test_root_name(self, alpharep):
        """
        The name of the root should be the name of the zipfile
        """
        root = zipfile.Path(alpharep)
        assert root.name == 'alpharep.zip' == root.filename.name

    @pass_alpharep
    def test_suffix(self, alpharep):
        """
        The suffix of the root should be the suffix of the zipfile.
        The suffix of each nested file is the final component's last suffix, if any.
        Includes the leading period, just like pathlib.Path.
        """
        root = zipfile.Path(alpharep)
        assert root.suffix == '.zip' == root.filename.suffix

        b = root / "b.txt"
        assert b.suffix == ".txt"

        c = root / "c" / "filename.tar.gz"
        assert c.suffix == ".gz"

        d = root / "d"
        assert d.suffix == ""

    @pass_alpharep
    def test_suffixes(self, alpharep):
        """
        The suffix of the root should be the suffix of the zipfile.
        The suffix of each nested file is the final component's last suffix, if any.
        Includes the leading period, just like pathlib.Path.
        """
        root = zipfile.Path(alpharep)
        assert root.suffixes == ['.zip'] == root.filename.suffixes

        b = root / 'b.txt'
        assert b.suffixes == ['.txt']

        c = root / 'c' / 'filename.tar.gz'
        assert c.suffixes == ['.tar', '.gz']

        d = root / 'd'
        assert d.suffixes == []

        e = root / '.hgrc'
        assert e.suffixes == []

    @pass_alpharep
    def test_stem(self, alpharep):
        """
        The final path component, without its suffix
        """
        root = zipfile.Path(alpharep)
        assert root.stem == 'alpharep' == root.filename.stem

        b = root / "b.txt"
        assert b.stem == "b"

        c = root / "c" / "filename.tar.gz"
        assert c.stem == "filename.tar"

        d = root / "d"
        assert d.stem == "d"

    @pass_alpharep
    def test_root_parent(self, alpharep):
        root = zipfile.Path(alpharep)
        assert root.parent == pathlib.Path('.')
        root.root.filename = 'foo/bar.zip'
        assert root.parent == pathlib.Path('foo')

    @pass_alpharep
    def test_root_unnamed(self, alpharep):
        """
        It is an error to attempt to get the name
        or parent of an unnamed zipfile.
        """
        alpharep.filename = None
        root = zipfile.Path(alpharep)
        with self.assertRaises(TypeError):
            root.name
        with self.assertRaises(TypeError):
            root.parent

        # .name and .parent should still work on subs
        sub = root / "b"
        assert sub.name == "b"
        assert sub.parent

    @pass_alpharep
    def test_inheritance(self, alpharep):
        cls = type('PathChild', (zipfile.Path,), {})
        for alpharep in self.zipfile_alpharep():
            file = cls(alpharep).joinpath('some dir').parent
            assert isinstance(file, cls)


class EncodedMetadataTests(unittest.TestCase):
    file_names = ['\u4e00', '\u4e8c', '\u4e09']  # Han 'one', 'two', 'three'
    file_content = [
        "This is pure ASCII.\n".encode('ascii'),
        # This is modern Japanese. (UTF-8)
        "\u3053\u308c\u306f\u73fe\u4ee3\u7684\u65e5\u672c\u8a9e\u3067\u3059\u3002\n".encode('utf-8'),
        # This is obsolete Japanese. (Shift JIS)
        "\u3053\u308c\u306f\u53e4\u3044\u65e5\u672c\u8a9e\u3067\u3059\u3002\n".encode('shift_jis'),
    ]

    def setUp(self):
        self.addCleanup(unlink, TESTFN)
        # Create .zip of 3 members with Han names encoded in Shift JIS.
        # Each name is 1 Han character encoding to 2 bytes in Shift JIS.
        # The ASCII names are arbitrary as long as they are length 2 and
        # not otherwise contained in the zip file.
        # Data elements are encoded bytes (ascii, utf-8, shift_jis).
        placeholders = ["n1", "n2"] + self.file_names[2:]
        with zipfile.ZipFile(TESTFN, mode="w") as tf:
            for temp, content in zip(placeholders, self.file_content):
                tf.writestr(temp, content, zipfile.ZIP_STORED)
        # Hack in the Shift JIS names with flag bit 11 (UTF-8) unset.
        with open(TESTFN, "rb") as tf:
            data = tf.read()
        for name, temp in zip(self.file_names, placeholders[:2]):
            data = data.replace(temp.encode('ascii'),
                                name.encode('shift_jis'))
        with open(TESTFN, "wb") as tf:
            tf.write(data)

    def _test_read(self, zipfp, expected_names, expected_content):
        # Check the namelist
        names = zipfp.namelist()
        self.assertEqual(sorted(names), sorted(expected_names))

        # Check infolist
        infos = zipfp.infolist()
        names = [zi.filename for zi in infos]
        self.assertEqual(sorted(names), sorted(expected_names))

        # check getinfo
        for name, content in zip(expected_names, expected_content):
            info = zipfp.getinfo(name)
            self.assertEqual(info.filename, name)
            self.assertEqual(info.file_size, len(content))
            self.assertEqual(zipfp.read(name), content)

    def test_read_with_metadata_encoding(self):
        # Read the ZIP archive with correct metadata_encoding
        with zipfile.ZipFile(TESTFN, "r", metadata_encoding='shift_jis') as zipfp:
            self._test_read(zipfp, self.file_names, self.file_content)

    def test_read_without_metadata_encoding(self):
        # Read the ZIP archive without metadata_encoding
        expected_names = [name.encode('shift_jis').decode('cp437')
                          for name in self.file_names[:2]] + self.file_names[2:]
        with zipfile.ZipFile(TESTFN, "r") as zipfp:
            self._test_read(zipfp, expected_names, self.file_content)

    def test_read_with_incorrect_metadata_encoding(self):
        # Read the ZIP archive with incorrect metadata_encoding
        expected_names = [name.encode('shift_jis').decode('koi8-u')
                          for name in self.file_names[:2]] + self.file_names[2:]
        with zipfile.ZipFile(TESTFN, "r", metadata_encoding='koi8-u') as zipfp:
            self._test_read(zipfp, expected_names, self.file_content)

    def test_read_with_unsuitable_metadata_encoding(self):
        # Read the ZIP archive with metadata_encoding unsuitable for
        # decoding metadata
        with self.assertRaises(UnicodeDecodeError):
            zipfile.ZipFile(TESTFN, "r", metadata_encoding='ascii')
        with self.assertRaises(UnicodeDecodeError):
            zipfile.ZipFile(TESTFN, "r", metadata_encoding='utf-8')

    def test_read_after_append(self):
        newname = '\u56db'  # Han 'four'
        expected_names = [name.encode('shift_jis').decode('cp437')
                          for name in self.file_names[:2]] + self.file_names[2:]
        expected_names.append(newname)
        expected_content = (*self.file_content, b"newcontent")

        with zipfile.ZipFile(TESTFN, "a") as zipfp:
            zipfp.writestr(newname, "newcontent")
            self.assertEqual(sorted(zipfp.namelist()), sorted(expected_names))

        with zipfile.ZipFile(TESTFN, "r") as zipfp:
            self._test_read(zipfp, expected_names, expected_content)

        with zipfile.ZipFile(TESTFN, "r", metadata_encoding='shift_jis') as zipfp:
            self.assertEqual(sorted(zipfp.namelist()), sorted(expected_names))
            for i, (name, content) in enumerate(zip(expected_names, expected_content)):
                info = zipfp.getinfo(name)
                self.assertEqual(info.filename, name)
                self.assertEqual(info.file_size, len(content))
                if i < 2:
                    with self.assertRaises(zipfile.BadZipFile):
                        zipfp.read(name)
                else:
                    self.assertEqual(zipfp.read(name), content)

    def test_write_with_metadata_encoding(self):
        ZF = zipfile.ZipFile
        for mode in ("w", "x", "a"):
            with self.assertRaisesRegex(ValueError,
                                        "^metadata_encoding is only"):
                ZF("nonesuch.zip", mode, metadata_encoding="shift_jis")

    def test_cli_with_metadata_encoding(self):
        errmsg = "Non-conforming encodings not supported with -c."
        args = ["--metadata-encoding=shift_jis", "-c", "nonesuch", "nonesuch"]
        with captured_stdout() as stdout:
            with captured_stderr() as stderr:
                self.assertRaises(SystemExit, zipfile.main, args)
        self.assertEqual(stdout.getvalue(), "")
        self.assertIn(errmsg, stderr.getvalue())

        with captured_stdout() as stdout:
            zipfile.main(["--metadata-encoding=shift_jis", "-t", TESTFN])
        listing = stdout.getvalue()

        with captured_stdout() as stdout:
            zipfile.main(["--metadata-encoding=shift_jis", "-l", TESTFN])
        listing = stdout.getvalue()
        for name in self.file_names:
            self.assertIn(name, listing)

    def test_cli_with_metadata_encoding_extract(self):
        os.mkdir(TESTFN2)
        self.addCleanup(rmtree, TESTFN2)
        # Depending on locale, extracted file names can be not encodable
        # with the filesystem encoding.
        for fn in self.file_names:
            try:
                os.stat(os.path.join(TESTFN2, fn))
            except OSError:
                pass
            except UnicodeEncodeError:
                self.skipTest(f'cannot encode file name {fn!r}')

        zipfile.main(["--metadata-encoding=shift_jis", "-e", TESTFN, TESTFN2])
        listing = os.listdir(TESTFN2)
        for name in self.file_names:
            self.assertIn(name, listing)


if __name__ == "__main__":
    unittest.main()
