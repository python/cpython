#! /usr/bin/env python
"""Test script for the gzip module.
"""

import unittest
from test import test_support
import os
import io
import struct
gzip = test_support.import_module('gzip')

data1 = """  int length=DEFAULTALLOC, err = Z_OK;
  PyObject *RetVal;
  int flushmode = Z_FINISH;
  unsigned long start_total_out;

"""

data2 = """/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.gzip.org/zlib/
/* See http://www.winimage.com/zLibDll for Windows */
"""


class TestGzip(unittest.TestCase):
    filename = test_support.TESTFN

    def setUp(self):
        test_support.unlink(self.filename)

    def tearDown(self):
        test_support.unlink(self.filename)


    def test_write(self):
        with gzip.GzipFile(self.filename, 'wb') as f:
            f.write(data1 * 50)

            # Try flush and fileno.
            f.flush()
            f.fileno()
            if hasattr(os, 'fsync'):
                os.fsync(f.fileno())
            f.close()

        # Test multiple close() calls.
        f.close()

    def test_read(self):
        self.test_write()
        # Try reading.
        with gzip.GzipFile(self.filename, 'r') as f:
            d = f.read()
        self.assertEqual(d, data1*50)

    def test_io_on_closed_object(self):
        # Test that I/O operations on closed GzipFile objects raise a
        # ValueError, just like the corresponding functions on file objects.

        # Write to a file, open it for reading, then close it.
        self.test_write()
        f = gzip.GzipFile(self.filename, 'r')
        f.close()
        with self.assertRaises(ValueError):
            f.read(1)
        with self.assertRaises(ValueError):
            f.seek(0)
        with self.assertRaises(ValueError):
            f.tell()
        # Open the file for writing, then close it.
        f = gzip.GzipFile(self.filename, 'w')
        f.close()
        with self.assertRaises(ValueError):
            f.write('')
        with self.assertRaises(ValueError):
            f.flush()

    def test_append(self):
        self.test_write()
        # Append to the previous file
        with gzip.GzipFile(self.filename, 'ab') as f:
            f.write(data2 * 15)

        with gzip.GzipFile(self.filename, 'rb') as f:
            d = f.read()
        self.assertEqual(d, (data1*50) + (data2*15))

    def test_many_append(self):
        # Bug #1074261 was triggered when reading a file that contained
        # many, many members.  Create such a file and verify that reading it
        # works.
        with gzip.open(self.filename, 'wb', 9) as f:
            f.write('a')
        for i in range(0, 200):
            with gzip.open(self.filename, "ab", 9) as f: # append
                f.write('a')

        # Try reading the file
        with gzip.open(self.filename, "rb") as zgfile:
            contents = ""
            while 1:
                ztxt = zgfile.read(8192)
                contents += ztxt
                if not ztxt: break
        self.assertEqual(contents, 'a'*201)

    def test_buffered_reader(self):
        # Issue #7471: a GzipFile can be wrapped in a BufferedReader for
        # performance.
        self.test_write()

        with gzip.GzipFile(self.filename, 'rb') as f:
            with io.BufferedReader(f) as r:
                lines = [line for line in r]

        self.assertEqual(lines, 50 * data1.splitlines(True))

    def test_readline(self):
        self.test_write()
        # Try .readline() with varying line lengths

        with gzip.GzipFile(self.filename, 'rb') as f:
            line_length = 0
            while 1:
                L = f.readline(line_length)
                if not L and line_length != 0: break
                self.assertTrue(len(L) <= line_length)
                line_length = (line_length + 1) % 50

    def test_readlines(self):
        self.test_write()
        # Try .readlines()

        with gzip.GzipFile(self.filename, 'rb') as f:
            L = f.readlines()

        with gzip.GzipFile(self.filename, 'rb') as f:
            while 1:
                L = f.readlines(150)
                if L == []: break

    def test_seek_read(self):
        self.test_write()
        # Try seek, read test

        with gzip.GzipFile(self.filename) as f:
            while 1:
                oldpos = f.tell()
                line1 = f.readline()
                if not line1: break
                newpos = f.tell()
                f.seek(oldpos)  # negative seek
                if len(line1)>10:
                    amount = 10
                else:
                    amount = len(line1)
                line2 = f.read(amount)
                self.assertEqual(line1[:amount], line2)
                f.seek(newpos)  # positive seek

    def test_seek_whence(self):
        self.test_write()
        # Try seek(whence=1), read test

        with gzip.GzipFile(self.filename) as f:
            f.read(10)
            f.seek(10, whence=1)
            y = f.read(10)
        self.assertEqual(y, data1[20:30])

    def test_seek_write(self):
        # Try seek, write test
        with gzip.GzipFile(self.filename, 'w') as f:
            for pos in range(0, 256, 16):
                f.seek(pos)
                f.write('GZ\n')

    def test_mode(self):
        self.test_write()
        with gzip.GzipFile(self.filename, 'r') as f:
            self.assertEqual(f.myfileobj.mode, 'rb')

    def test_1647484(self):
        for mode in ('wb', 'rb'):
            with gzip.GzipFile(self.filename, mode) as f:
                self.assertTrue(hasattr(f, "name"))
                self.assertEqual(f.name, self.filename)

    def test_mtime(self):
        mtime = 123456789
        with gzip.GzipFile(self.filename, 'w', mtime = mtime) as fWrite:
            fWrite.write(data1)
        with gzip.GzipFile(self.filename) as fRead:
            dataRead = fRead.read()
            self.assertEqual(dataRead, data1)
            self.assertTrue(hasattr(fRead, 'mtime'))
            self.assertEqual(fRead.mtime, mtime)

    def test_metadata(self):
        mtime = 123456789

        with gzip.GzipFile(self.filename, 'w', mtime = mtime) as fWrite:
            fWrite.write(data1)

        with open(self.filename, 'rb') as fRead:
            # see RFC 1952: http://www.faqs.org/rfcs/rfc1952.html

            idBytes = fRead.read(2)
            self.assertEqual(idBytes, '\x1f\x8b') # gzip ID

            cmByte = fRead.read(1)
            self.assertEqual(cmByte, '\x08') # deflate

            flagsByte = fRead.read(1)
            self.assertEqual(flagsByte, '\x08') # only the FNAME flag is set

            mtimeBytes = fRead.read(4)
            self.assertEqual(mtimeBytes, struct.pack('<i', mtime)) # little-endian

            xflByte = fRead.read(1)
            self.assertEqual(xflByte, '\x02') # maximum compression

            osByte = fRead.read(1)
            self.assertEqual(osByte, '\xff') # OS "unknown" (OS-independent)

            # Since the FNAME flag is set, the zero-terminated filename follows.
            # RFC 1952 specifies that this is the name of the input file, if any.
            # However, the gzip module defaults to storing the name of the output
            # file in this field.
            expected = self.filename.encode('Latin-1') + '\x00'
            nameBytes = fRead.read(len(expected))
            self.assertEqual(nameBytes, expected)

            # Since no other flags were set, the header ends here.
            # Rather than process the compressed data, let's seek to the trailer.
            fRead.seek(os.stat(self.filename).st_size - 8)

            crc32Bytes = fRead.read(4) # CRC32 of uncompressed data [data1]
            self.assertEqual(crc32Bytes, '\xaf\xd7d\x83')

            isizeBytes = fRead.read(4)
            self.assertEqual(isizeBytes, struct.pack('<i', len(data1)))

    def test_with_open(self):
        # GzipFile supports the context management protocol
        with gzip.GzipFile(self.filename, "wb") as f:
            f.write(b"xxx")
        f = gzip.GzipFile(self.filename, "rb")
        f.close()
        try:
            with f:
                pass
        except ValueError:
            pass
        else:
            self.fail("__enter__ on a closed file didn't raise an exception")
        try:
            with gzip.GzipFile(self.filename, "wb") as f:
                1 // 0
        except ZeroDivisionError:
            pass
        else:
            self.fail("1 // 0 didn't raise an exception")

    def test_zero_padded_file(self):
        with gzip.GzipFile(self.filename, "wb") as f:
            f.write(data1 * 50)

        # Pad the file with zeroes
        with open(self.filename, "ab") as f:
            f.write("\x00" * 50)

        with gzip.GzipFile(self.filename, "rb") as f:
            d = f.read()
            self.assertEqual(d, data1 * 50, "Incorrect data in file")

def test_main(verbose=None):
    test_support.run_unittest(TestGzip)

if __name__ == "__main__":
    test_main(verbose=True)
