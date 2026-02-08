# Tests of the full ZIP64 functionality of zipfile
# The support.requires call is the only reason for keeping this separate
# from test_zipfile
from test import support

# XXX(nnorwitz): disable this test by looking for extralargefile resource,
# which doesn't exist.  This test takes over 30 minutes to run in general
# and requires more disk space than most of the buildbots.
support.requires(
        'extralargefile',
        'test requires loads of disk-space bytes and a long time to run'
    )

import zipfile, unittest
import time
import tracemalloc
import sys
import unittest.mock as mock

from tempfile import TemporaryFile

from test.support import os_helper
from test.support import requires_zlib
from test.test_zipfile.test_core import Unseekable
from test.test_zipfile.test_core import struct_pack_no_dd_sig

TESTFN = os_helper.TESTFN
TESTFN2 = TESTFN + "2"

# How much time in seconds can pass before we print a 'Still working' message.
_PRINT_WORKING_MSG_INTERVAL = 60

class TestsWithSourceFile(unittest.TestCase):
    def setUp(self):
        # Create test data.
        line_gen = ("Test of zipfile line %d." % i for i in range(1000000))
        self.data = '\n'.join(line_gen).encode('ascii')

    def zipTest(self, f, compression):
        # Create the ZIP archive.
        with zipfile.ZipFile(f, "w", compression) as zipfp:

            # It will contain enough copies of self.data to reach about 6 GiB of
            # raw data to store.
            filecount = 6*1024**3 // len(self.data)

            next_time = time.monotonic() + _PRINT_WORKING_MSG_INTERVAL
            for num in range(filecount):
                zipfp.writestr("testfn%d" % num, self.data)
                # Print still working message since this test can be really slow
                if next_time <= time.monotonic():
                    next_time = time.monotonic() + _PRINT_WORKING_MSG_INTERVAL
                    print((
                    '  zipTest still writing %d of %d, be patient...' %
                    (num, filecount)), file=sys.__stdout__)
                    sys.__stdout__.flush()

        # Read the ZIP archive
        with zipfile.ZipFile(f, "r", compression) as zipfp:
            for num in range(filecount):
                self.assertEqual(zipfp.read("testfn%d" % num), self.data)
                # Print still working message since this test can be really slow
                if next_time <= time.monotonic():
                    next_time = time.monotonic() + _PRINT_WORKING_MSG_INTERVAL
                    print((
                    '  zipTest still reading %d of %d, be patient...' %
                    (num, filecount)), file=sys.__stdout__)
                    sys.__stdout__.flush()

            # Check that testzip thinks the archive is valid
            self.assertIsNone(zipfp.testzip())

    def testStored(self):
        # Try the temp file first.  If we do TESTFN2 first, then it hogs
        # gigabytes of disk space for the duration of the test.
        with TemporaryFile() as f:
            self.zipTest(f, zipfile.ZIP_STORED)
            self.assertFalse(f.closed)
        self.zipTest(TESTFN2, zipfile.ZIP_STORED)

    @requires_zlib()
    def testDeflated(self):
        # Try the temp file first.  If we do TESTFN2 first, then it hogs
        # gigabytes of disk space for the duration of the test.
        with TemporaryFile() as f:
            self.zipTest(f, zipfile.ZIP_DEFLATED)
            self.assertFalse(f.closed)
        self.zipTest(TESTFN2, zipfile.ZIP_DEFLATED)

    def tearDown(self):
        os_helper.unlink(TESTFN2)


class TestRepack(unittest.TestCase):
    def setUp(self):
        # Create test data.
        line_gen = ("Test of zipfile line %d." % i for i in range(1000000))
        self.data = '\n'.join(line_gen).encode('ascii')

        # It will contain enough copies of self.data to reach about 8 GiB.
        self.datacount = 8*1024**3 // len(self.data)

        # memory usage should not exceed 10 MiB
        self.allowed_memory = 10*1024**2

    def _write_large_file(self, fh):
        next_time = time.monotonic() + _PRINT_WORKING_MSG_INTERVAL
        for num in range(self.datacount):
            fh.write(self.data)
            # Print still working message since this test can be really slow
            if next_time <= time.monotonic():
                next_time = time.monotonic() + _PRINT_WORKING_MSG_INTERVAL
                print((
                '  writing %d of %d, be patient...' %
                (num, self.datacount)), file=sys.__stdout__)
                sys.__stdout__.flush()

    def test_strip_removed_large_file(self):
        """Should move the physical data of a file positioned after a large
        removed file without causing a memory issue."""
        # Try the temp file.  If we do TESTFN2, then it hogs
        # gigabytes of disk space for the duration of the test.
        with TemporaryFile() as f:
            tracemalloc.start()
            self._test_strip_removed_large_file(f)
            self.assertFalse(f.closed)
            current, peak = tracemalloc.get_traced_memory()
            tracemalloc.stop()
            self.assertLess(peak, self.allowed_memory)

    def _test_strip_removed_large_file(self, f):
        file = 'file.txt'
        file1 = 'largefile.txt'
        data = b'Sed ut perspiciatis unde omnis iste natus error sit voluptatem'
        with zipfile.ZipFile(f, 'w') as zh:
            with zh.open(file1, 'w', force_zip64=True) as fh:
                self._write_large_file(fh)
            zh.writestr(file, data)

        with zipfile.ZipFile(f, 'a') as zh:
            zh.remove(file1)
            zh.repack()
            self.assertIsNone(zh.testzip())

    def test_strip_removed_file_before_large_file(self):
        """Should move the physical data of a large file positioned after a
        removed file without causing a memory issue."""
        # Try the temp file.  If we do TESTFN2, then it hogs
        # gigabytes of disk space for the duration of the test.
        with TemporaryFile() as f:
            tracemalloc.start()
            self._test_strip_removed_file_before_large_file(f)
            self.assertFalse(f.closed)
            current, peak = tracemalloc.get_traced_memory()
            tracemalloc.stop()
            self.assertLess(peak, self.allowed_memory)

    def _test_strip_removed_file_before_large_file(self, f):
        file = 'file.txt'
        file1 = 'largefile.txt'
        data = b'Sed ut perspiciatis unde omnis iste natus error sit voluptatem'
        with zipfile.ZipFile(f, 'w') as zh:
            zh.writestr(file, data)
            with zh.open(file1, 'w', force_zip64=True) as fh:
                self._write_large_file(fh)

        with zipfile.ZipFile(f, 'a') as zh:
            zh.remove(file)
            zh.repack()
            self.assertIsNone(zh.testzip())

    def test_strip_removed_large_file_with_dd(self):
        """Should scan for the data descriptor of a removed large file without
        causing a memory issue."""
        # Try the temp file.  If we do TESTFN2, then it hogs
        # gigabytes of disk space for the duration of the test.
        with TemporaryFile() as f:
            tracemalloc.start()
            self._test_strip_removed_large_file_with_dd(f)
            self.assertFalse(f.closed)
            current, peak = tracemalloc.get_traced_memory()
            tracemalloc.stop()
            self.assertLess(peak, self.allowed_memory)

    def _test_strip_removed_large_file_with_dd(self, f):
        file = 'file.txt'
        file1 = 'largefile.txt'
        data = b'Sed ut perspiciatis unde omnis iste natus error sit voluptatem'
        with zipfile.ZipFile(Unseekable(f), 'w') as zh:
            with zh.open(file1, 'w', force_zip64=True) as fh:
                self._write_large_file(fh)
            zh.writestr(file, data)

        with zipfile.ZipFile(f, 'a') as zh:
            zh.remove(file1)
            zh.repack()
            self.assertIsNone(zh.testzip())

    def test_strip_removed_large_file_with_dd_no_sig(self):
        """Should scan for the data descriptor (without signature) of a removed
        large file without causing a memory issue."""
        # Reduce data scale for this test, as it's especially slow...
        self.datacount = 30*1024**2 // len(self.data)
        self.allowed_memory = 200*1024

        # Try the temp file.  If we do TESTFN2, then it hogs
        # gigabytes of disk space for the duration of the test.
        with TemporaryFile() as f:
            tracemalloc.start()
            self._test_strip_removed_large_file_with_dd_no_sig(f)
            self.assertFalse(f.closed)
            current, peak = tracemalloc.get_traced_memory()
            tracemalloc.stop()
            self.assertLess(peak, self.allowed_memory)

    def _test_strip_removed_large_file_with_dd_no_sig(self, f):
        file = 'file.txt'
        file1 = 'largefile.txt'
        data = b'Sed ut perspiciatis unde omnis iste natus error sit voluptatem'
        with mock.patch('zipfile.struct.pack', side_effect=struct_pack_no_dd_sig):
            with zipfile.ZipFile(Unseekable(f), 'w') as zh:
                with zh.open(file1, 'w', force_zip64=True) as fh:
                    self._write_large_file(fh)
                zh.writestr(file, data)

        with zipfile.ZipFile(f, 'a') as zh:
            zh.remove(file1)
            zh.repack()
            self.assertIsNone(zh.testzip())

    @requires_zlib()
    def test_strip_removed_large_file_with_dd_no_sig_by_decompression(self):
        """Should scan for the data descriptor (without signature) of a removed
        large file without causing a memory issue."""
        # Try the temp file.  If we do TESTFN2, then it hogs
        # gigabytes of disk space for the duration of the test.
        with TemporaryFile() as f:
            tracemalloc.start()
            self._test_strip_removed_large_file_with_dd_no_sig_by_decompression(
                f, zipfile.ZIP_DEFLATED)
            self.assertFalse(f.closed)
            current, peak = tracemalloc.get_traced_memory()
            tracemalloc.stop()
            self.assertLess(peak, self.allowed_memory)

    def _test_strip_removed_large_file_with_dd_no_sig_by_decompression(self, f, method):
        file = 'file.txt'
        file1 = 'largefile.txt'
        data = b'Sed ut perspiciatis unde omnis iste natus error sit voluptatem'
        with mock.patch('zipfile.struct.pack', side_effect=struct_pack_no_dd_sig):
            with zipfile.ZipFile(Unseekable(f), 'w', compression=method) as zh:
                with zh.open(file1, 'w', force_zip64=True) as fh:
                    self._write_large_file(fh)
                zh.writestr(file, data)

        with zipfile.ZipFile(f, 'a') as zh:
            zh.remove(file1)
            zh.repack()
            self.assertIsNone(zh.testzip())


class OtherTests(unittest.TestCase):
    def testMoreThan64kFiles(self):
        # This test checks that more than 64k files can be added to an archive,
        # and that the resulting archive can be read properly by ZipFile
        with zipfile.ZipFile(TESTFN, mode="w", allowZip64=True) as zipf:
            zipf.debug = 100
            numfiles = (1 << 16) * 3//2
            for i in range(numfiles):
                zipf.writestr("foo%08d" % i, "%d" % (i**3 % 57))
            self.assertEqual(len(zipf.namelist()), numfiles)

        with zipfile.ZipFile(TESTFN, mode="r") as zipf2:
            self.assertEqual(len(zipf2.namelist()), numfiles)
            for i in range(numfiles):
                content = zipf2.read("foo%08d" % i).decode('ascii')
                self.assertEqual(content, "%d" % (i**3 % 57))

    def testMoreThan64kFilesAppend(self):
        with zipfile.ZipFile(TESTFN, mode="w", allowZip64=False) as zipf:
            zipf.debug = 100
            numfiles = (1 << 16) - 1
            for i in range(numfiles):
                zipf.writestr("foo%08d" % i, "%d" % (i**3 % 57))
            self.assertEqual(len(zipf.namelist()), numfiles)
            with self.assertRaises(zipfile.LargeZipFile):
                zipf.writestr("foo%08d" % numfiles, b'')
            self.assertEqual(len(zipf.namelist()), numfiles)

        with zipfile.ZipFile(TESTFN, mode="a", allowZip64=False) as zipf:
            zipf.debug = 100
            self.assertEqual(len(zipf.namelist()), numfiles)
            with self.assertRaises(zipfile.LargeZipFile):
                zipf.writestr("foo%08d" % numfiles, b'')
            self.assertEqual(len(zipf.namelist()), numfiles)

        with zipfile.ZipFile(TESTFN, mode="a", allowZip64=True) as zipf:
            zipf.debug = 100
            self.assertEqual(len(zipf.namelist()), numfiles)
            numfiles2 = (1 << 16) * 3//2
            for i in range(numfiles, numfiles2):
                zipf.writestr("foo%08d" % i, "%d" % (i**3 % 57))
            self.assertEqual(len(zipf.namelist()), numfiles2)

        with zipfile.ZipFile(TESTFN, mode="r") as zipf2:
            self.assertEqual(len(zipf2.namelist()), numfiles2)
            for i in range(numfiles2):
                content = zipf2.read("foo%08d" % i).decode('ascii')
                self.assertEqual(content, "%d" % (i**3 % 57))

    def tearDown(self):
        os_helper.unlink(TESTFN)
        os_helper.unlink(TESTFN2)

if __name__ == "__main__":
    unittest.main()
