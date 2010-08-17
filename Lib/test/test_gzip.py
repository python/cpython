#! /usr/bin/env python3
"""Test script for the gzip module.
"""

import unittest
from test import support
import os
import io
import struct
gzip = support.import_module('gzip')

data1 = b"""  int length=DEFAULTALLOC, err = Z_OK;
  PyObject *RetVal;
  int flushmode = Z_FINISH;
  unsigned long start_total_out;

"""

data2 = b"""/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.gzip.org/zlib/
/* See http://www.winimage.com/zLibDll for Windows */
"""


class TestGzip(unittest.TestCase):
    filename = support.TESTFN

    def setUp(self):
        support.unlink(self.filename)

    def tearDown(self):
        support.unlink(self.filename)


    def test_write(self):
        f = gzip.GzipFile(self.filename, 'wb') ; f.write(data1 * 50)

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
        f = gzip.GzipFile(self.filename, 'r') ; d = f.read() ; f.close()
        self.assertEqual(d, data1*50)

    def test_append(self):
        self.test_write()
        # Append to the previous file
        f = gzip.GzipFile(self.filename, 'ab') ; f.write(data2 * 15) ; f.close()

        f = gzip.GzipFile(self.filename, 'rb') ; d = f.read() ; f.close()
        self.assertEqual(d, (data1*50) + (data2*15))

    def test_many_append(self):
        # Bug #1074261 was triggered when reading a file that contained
        # many, many members.  Create such a file and verify that reading it
        # works.
        f = gzip.open(self.filename, 'wb', 9)
        f.write(b'a')
        f.close()
        for i in range(0, 200):
            f = gzip.open(self.filename, "ab", 9) # append
            f.write(b'a')
            f.close()

        # Try reading the file
        zgfile = gzip.open(self.filename, "rb")
        contents = b""
        while 1:
            ztxt = zgfile.read(8192)
            contents += ztxt
            if not ztxt: break
        zgfile.close()
        self.assertEquals(contents, b'a'*201)

    def test_buffered_reader(self):
        # Issue #7471: a GzipFile can be wrapped in a BufferedReader for
        # performance.
        self.test_write()

        f = gzip.GzipFile(self.filename, 'rb')
        with io.BufferedReader(f) as r:
            lines = [line for line in r]

        self.assertEqual(lines, 50 * data1.splitlines(True))

    def test_readline(self):
        self.test_write()
        # Try .readline() with varying line lengths

        f = gzip.GzipFile(self.filename, 'rb')
        line_length = 0
        while 1:
            L = f.readline(line_length)
            if not L and line_length != 0: break
            self.assertTrue(len(L) <= line_length)
            line_length = (line_length + 1) % 50
        f.close()

    def test_readlines(self):
        self.test_write()
        # Try .readlines()

        f = gzip.GzipFile(self.filename, 'rb')
        L = f.readlines()
        f.close()

        f = gzip.GzipFile(self.filename, 'rb')
        while 1:
            L = f.readlines(150)
            if L == []: break
        f.close()

    def test_seek_read(self):
        self.test_write()
        # Try seek, read test

        f = gzip.GzipFile(self.filename)
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
        f.close()

    def test_seek_whence(self):
        self.test_write()
        # Try seek(whence=1), read test

        f = gzip.GzipFile(self.filename)
        f.read(10)
        f.seek(10, whence=1)
        y = f.read(10)
        f.close()
        self.assertEquals(y, data1[20:30])

    def test_seek_write(self):
        # Try seek, write test
        f = gzip.GzipFile(self.filename, 'w')
        for pos in range(0, 256, 16):
            f.seek(pos)
            f.write(b'GZ\n')
        f.close()

    def test_mode(self):
        self.test_write()
        f = gzip.GzipFile(self.filename, 'r')
        self.assertEqual(f.myfileobj.mode, 'rb')
        f.close()

    def test_1647484(self):
        for mode in ('wb', 'rb'):
            f = gzip.GzipFile(self.filename, mode)
            self.assertTrue(hasattr(f, "name"))
            self.assertEqual(f.name, self.filename)
            f.close()

    def test_mtime(self):
        mtime = 123456789
        fWrite = gzip.GzipFile(self.filename, 'w', mtime = mtime)
        fWrite.write(data1)
        fWrite.close()
        fRead = gzip.GzipFile(self.filename)
        dataRead = fRead.read()
        self.assertEqual(dataRead, data1)
        self.assertTrue(hasattr(fRead, 'mtime'))
        self.assertEqual(fRead.mtime, mtime)
        fRead.close()

    def test_metadata(self):
        mtime = 123456789

        fWrite = gzip.GzipFile(self.filename, 'w', mtime = mtime)
        fWrite.write(data1)
        fWrite.close()

        fRead = open(self.filename, 'rb')

        # see RFC 1952: http://www.faqs.org/rfcs/rfc1952.html

        idBytes = fRead.read(2)
        self.assertEqual(idBytes, b'\x1f\x8b') # gzip ID

        cmByte = fRead.read(1)
        self.assertEqual(cmByte, b'\x08') # deflate

        flagsByte = fRead.read(1)
        self.assertEqual(flagsByte, b'\x08') # only the FNAME flag is set

        mtimeBytes = fRead.read(4)
        self.assertEqual(mtimeBytes, struct.pack('<i', mtime)) # little-endian

        xflByte = fRead.read(1)
        self.assertEqual(xflByte, b'\x02') # maximum compression

        osByte = fRead.read(1)
        self.assertEqual(osByte, b'\xff') # OS "unknown" (OS-independent)

        # Since the FNAME flag is set, the zero-terminated filename follows.
        # RFC 1952 specifies that this is the name of the input file, if any.
        # However, the gzip module defaults to storing the name of the output
        # file in this field.
        expected = self.filename.encode('Latin-1') + b'\x00'
        nameBytes = fRead.read(len(expected))
        self.assertEqual(nameBytes, expected)

        # Since no other flags were set, the header ends here.
        # Rather than process the compressed data, let's seek to the trailer.
        fRead.seek(os.stat(self.filename).st_size - 8)

        crc32Bytes = fRead.read(4) # CRC32 of uncompressed data [data1]
        self.assertEqual(crc32Bytes, b'\xaf\xd7d\x83')

        isizeBytes = fRead.read(4)
        self.assertEqual(isizeBytes, struct.pack('<i', len(data1)))

        fRead.close()

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
                1/0
        except ZeroDivisionError:
            pass
        else:
            self.fail("1/0 didn't raise an exception")

    def test_zero_padded_file(self):
        with gzip.GzipFile(self.filename, "wb") as f:
            f.write(data1 * 50)

        # Pad the file with zeroes
        with open(self.filename, "ab") as f:
            f.write(b"\x00" * 50)

        with gzip.GzipFile(self.filename, "rb") as f:
            d = f.read()
            self.assertEqual(d, data1 * 50, "Incorrect data in file")

    # Testing compress/decompress shortcut functions

    def test_compress(self):
        for data in [data1, data2]:
            for args in [(), (1,), (6,), (9,)]:
                datac = gzip.compress(data, *args)
                self.assertEqual(type(datac), bytes)
                with gzip.GzipFile(fileobj=io.BytesIO(datac), mode="rb") as f:
                    self.assertEqual(f.read(), data)

    def test_decompress(self):
        for data in (data1, data2):
            buf = io.BytesIO()
            with gzip.GzipFile(fileobj=buf, mode="wb") as f:
                f.write(data)
            self.assertEqual(gzip.decompress(buf.getvalue()), data)
            # Roundtrip with compress
            datac = gzip.compress(data)
            self.assertEqual(gzip.decompress(datac), data)

def test_main(verbose=None):
    support.run_unittest(TestGzip)

if __name__ == "__main__":
    test_main(verbose=True)
