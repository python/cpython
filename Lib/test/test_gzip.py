"""Test script for the gzip module.
"""

import array
import functools
import io
import os
import pathlib
import struct
import sys
import unittest
from subprocess import PIPE, Popen
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import _4G, bigmemtest
from test.support.script_helper import assert_python_ok, assert_python_failure

gzip = import_helper.import_module('gzip')

data1 = b"""  int length=DEFAULTALLOC, err = Z_OK;
  PyObject *RetVal;
  int flushmode = Z_FINISH;
  unsigned long start_total_out;

"""

data2 = b"""/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.gzip.org/zlib/
/* See http://www.winimage.com/zLibDll for Windows */
"""


TEMPDIR = os.path.abspath(os_helper.TESTFN) + '-gzdir'


class UnseekableIO(io.BytesIO):
    def seekable(self):
        return False

    def tell(self):
        raise io.UnsupportedOperation

    def seek(self, *args):
        raise io.UnsupportedOperation


class BaseTest(unittest.TestCase):
    filename = os_helper.TESTFN

    def setUp(self):
        os_helper.unlink(self.filename)

    def tearDown(self):
        os_helper.unlink(self.filename)


class TestGzip(BaseTest):
    def write_and_read_back(self, data, mode='b'):
        b_data = bytes(data)
        with gzip.GzipFile(self.filename, 'w'+mode) as f:
            l = f.write(data)
        self.assertEqual(l, len(b_data))
        with gzip.GzipFile(self.filename, 'r'+mode) as f:
            self.assertEqual(f.read(), b_data)

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

    def test_write_read_with_pathlike_file(self):
        filename = pathlib.Path(self.filename)
        with gzip.GzipFile(filename, 'w') as f:
            f.write(data1 * 50)
        self.assertIsInstance(f.name, str)
        with gzip.GzipFile(filename, 'a') as f:
            f.write(data1)
        with gzip.GzipFile(filename) as f:
            d = f.read()
        self.assertEqual(d, data1 * 51)
        self.assertIsInstance(f.name, str)

    # The following test_write_xy methods test that write accepts
    # the corresponding bytes-like object type as input
    # and that the data written equals bytes(xy) in all cases.
    def test_write_memoryview(self):
        self.write_and_read_back(memoryview(data1 * 50))
        m = memoryview(bytes(range(256)))
        data = m.cast('B', shape=[8,8,4])
        self.write_and_read_back(data)

    def test_write_bytearray(self):
        self.write_and_read_back(bytearray(data1 * 50))

    def test_write_array(self):
        self.write_and_read_back(array.array('I', data1 * 40))

    def test_write_incompatible_type(self):
        # Test that non-bytes-like types raise TypeError.
        # Issue #21560: attempts to write incompatible types
        # should not affect the state of the fileobject
        with gzip.GzipFile(self.filename, 'wb') as f:
            with self.assertRaises(TypeError):
                f.write('')
            with self.assertRaises(TypeError):
                f.write([])
            f.write(data1)
        with gzip.GzipFile(self.filename, 'rb') as f:
            self.assertEqual(f.read(), data1)

    def test_read(self):
        self.test_write()
        # Try reading.
        with gzip.GzipFile(self.filename, 'r') as f:
            d = f.read()
        self.assertEqual(d, data1*50)

    def test_read1(self):
        self.test_write()
        blocks = []
        nread = 0
        with gzip.GzipFile(self.filename, 'r') as f:
            while True:
                d = f.read1()
                if not d:
                    break
                blocks.append(d)
                nread += len(d)
                # Check that position was updated correctly (see issue10791).
                self.assertEqual(f.tell(), nread)
        self.assertEqual(b''.join(blocks), data1 * 50)

    @bigmemtest(size=_4G, memuse=1)
    def test_read_large(self, size):
        # Read chunk size over UINT_MAX should be supported, despite zlib's
        # limitation per low-level call
        compressed = gzip.compress(data1, compresslevel=1)
        f = gzip.GzipFile(fileobj=io.BytesIO(compressed), mode='rb')
        self.assertEqual(f.read(size), data1)

    def test_io_on_closed_object(self):
        # Test that I/O operations on closed GzipFile objects raise a
        # ValueError, just like the corresponding functions on file objects.

        # Write to a file, open it for reading, then close it.
        self.test_write()
        f = gzip.GzipFile(self.filename, 'r')
        fileobj = f.fileobj
        self.assertFalse(fileobj.closed)
        f.close()
        self.assertTrue(fileobj.closed)
        with self.assertRaises(ValueError):
            f.read(1)
        with self.assertRaises(ValueError):
            f.seek(0)
        with self.assertRaises(ValueError):
            f.tell()
        # Open the file for writing, then close it.
        f = gzip.GzipFile(self.filename, 'w')
        fileobj = f.fileobj
        self.assertFalse(fileobj.closed)
        f.close()
        self.assertTrue(fileobj.closed)
        with self.assertRaises(ValueError):
            f.write(b'')
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
        with gzip.GzipFile(self.filename, 'wb', 9) as f:
            f.write(b'a')
        for i in range(0, 200):
            with gzip.GzipFile(self.filename, "ab", 9) as f: # append
                f.write(b'a')

        # Try reading the file
        with gzip.GzipFile(self.filename, "rb") as zgfile:
            contents = b""
            while 1:
                ztxt = zgfile.read(8192)
                contents += ztxt
                if not ztxt: break
        self.assertEqual(contents, b'a'*201)

    def test_exclusive_write(self):
        with gzip.GzipFile(self.filename, 'xb') as f:
            f.write(data1 * 50)
        with gzip.GzipFile(self.filename, 'rb') as f:
            self.assertEqual(f.read(), data1 * 50)
        with self.assertRaises(FileExistsError):
            gzip.GzipFile(self.filename, 'xb')

    def test_buffered_reader(self):
        # Issue #7471: a GzipFile can be wrapped in a BufferedReader for
        # performance.
        self.test_write()

        with gzip.GzipFile(self.filename, 'rb') as f:
            with io.BufferedReader(f) as r:
                lines = [line for line in r]

        self.assertEqual(lines, 50 * data1.splitlines(keepends=True))

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
                f.write(b'GZ\n')

    def test_mode(self):
        self.test_write()
        with gzip.GzipFile(self.filename, 'r') as f:
            self.assertEqual(f.myfileobj.mode, 'rb')
        os_helper.unlink(self.filename)
        with gzip.GzipFile(self.filename, 'x') as f:
            self.assertEqual(f.myfileobj.mode, 'xb')

    def test_1647484(self):
        for mode in ('wb', 'rb'):
            with gzip.GzipFile(self.filename, mode) as f:
                self.assertTrue(hasattr(f, "name"))
                self.assertEqual(f.name, self.filename)

    def test_paddedfile_getattr(self):
        self.test_write()
        with gzip.GzipFile(self.filename, 'rb') as f:
            self.assertTrue(hasattr(f.fileobj, "name"))
            self.assertEqual(f.fileobj.name, self.filename)

    def test_mtime(self):
        mtime = 123456789
        with gzip.GzipFile(self.filename, 'w', mtime = mtime) as fWrite:
            fWrite.write(data1)
        with gzip.GzipFile(self.filename) as fRead:
            self.assertTrue(hasattr(fRead, 'mtime'))
            self.assertIsNone(fRead.mtime)
            dataRead = fRead.read()
            self.assertEqual(dataRead, data1)
            self.assertEqual(fRead.mtime, mtime)

    def test_metadata(self):
        mtime = 123456789

        with gzip.GzipFile(self.filename, 'w', mtime = mtime) as fWrite:
            fWrite.write(data1)

        with open(self.filename, 'rb') as fRead:
            # see RFC 1952: http://www.faqs.org/rfcs/rfc1952.html

            idBytes = fRead.read(2)
            self.assertEqual(idBytes, b'\x1f\x8b') # gzip ID

            cmByte = fRead.read(1)
            self.assertEqual(cmByte, b'\x08') # deflate

            try:
                expectedname = self.filename.encode('Latin-1') + b'\x00'
                expectedflags = b'\x08' # only the FNAME flag is set
            except UnicodeEncodeError:
                expectedname = b''
                expectedflags = b'\x00'

            flagsByte = fRead.read(1)
            self.assertEqual(flagsByte, expectedflags)

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
            nameBytes = fRead.read(len(expectedname))
            self.assertEqual(nameBytes, expectedname)

            # Since no other flags were set, the header ends here.
            # Rather than process the compressed data, let's seek to the trailer.
            fRead.seek(os.stat(self.filename).st_size - 8)

            crc32Bytes = fRead.read(4) # CRC32 of uncompressed data [data1]
            self.assertEqual(crc32Bytes, b'\xaf\xd7d\x83')

            isizeBytes = fRead.read(4)
            self.assertEqual(isizeBytes, struct.pack('<i', len(data1)))

    def test_metadata_ascii_name(self):
        self.filename = os_helper.TESTFN_ASCII
        self.test_metadata()

    def test_compresslevel_metadata(self):
        # see RFC 1952: http://www.faqs.org/rfcs/rfc1952.html
        # specifically, discussion of XFL in section 2.3.1
        cases = [
            ('fast', 1, b'\x04'),
            ('best', 9, b'\x02'),
            ('tradeoff', 6, b'\x00'),
        ]
        xflOffset = 8

        for (name, level, expectedXflByte) in cases:
            with self.subTest(name):
                fWrite = gzip.GzipFile(self.filename, 'w', compresslevel=level)
                with fWrite:
                    fWrite.write(data1)
                with open(self.filename, 'rb') as fRead:
                    fRead.seek(xflOffset)
                    xflByte = fRead.read(1)
                    self.assertEqual(xflByte, expectedXflByte)

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

    def test_gzip_BadGzipFile_exception(self):
        self.assertTrue(issubclass(gzip.BadGzipFile, OSError))

    def test_bad_gzip_file(self):
        with open(self.filename, 'wb') as file:
            file.write(data1 * 50)
        with gzip.GzipFile(self.filename, 'r') as file:
            self.assertRaises(gzip.BadGzipFile, file.readlines)

    def test_non_seekable_file(self):
        uncompressed = data1 * 50
        buf = UnseekableIO()
        with gzip.GzipFile(fileobj=buf, mode="wb") as f:
            f.write(uncompressed)
        compressed = buf.getvalue()
        buf = UnseekableIO(compressed)
        with gzip.GzipFile(fileobj=buf, mode="rb") as f:
            self.assertEqual(f.read(), uncompressed)

    def test_peek(self):
        uncompressed = data1 * 200
        with gzip.GzipFile(self.filename, "wb") as f:
            f.write(uncompressed)

        def sizes():
            while True:
                for n in range(5, 50, 10):
                    yield n

        with gzip.GzipFile(self.filename, "rb") as f:
            f.max_read_chunk = 33
            nread = 0
            for n in sizes():
                s = f.peek(n)
                if s == b'':
                    break
                self.assertEqual(f.read(len(s)), s)
                nread += len(s)
            self.assertEqual(f.read(100), b'')
            self.assertEqual(nread, len(uncompressed))

    def test_textio_readlines(self):
        # Issue #10791: TextIOWrapper.readlines() fails when wrapping GzipFile.
        lines = (data1 * 50).decode("ascii").splitlines(keepends=True)
        self.test_write()
        with gzip.GzipFile(self.filename, 'r') as f:
            with io.TextIOWrapper(f, encoding="ascii") as t:
                self.assertEqual(t.readlines(), lines)

    def test_fileobj_from_fdopen(self):
        # Issue #13781: Opening a GzipFile for writing fails when using a
        # fileobj created with os.fdopen().
        fd = os.open(self.filename, os.O_WRONLY | os.O_CREAT)
        with os.fdopen(fd, "wb") as f:
            with gzip.GzipFile(fileobj=f, mode="w") as g:
                pass

    def test_fileobj_mode(self):
        gzip.GzipFile(self.filename, "wb").close()
        with open(self.filename, "r+b") as f:
            with gzip.GzipFile(fileobj=f, mode='r') as g:
                self.assertEqual(g.mode, gzip.READ)
            with gzip.GzipFile(fileobj=f, mode='w') as g:
                self.assertEqual(g.mode, gzip.WRITE)
            with gzip.GzipFile(fileobj=f, mode='a') as g:
                self.assertEqual(g.mode, gzip.WRITE)
            with gzip.GzipFile(fileobj=f, mode='x') as g:
                self.assertEqual(g.mode, gzip.WRITE)
            with self.assertRaises(ValueError):
                gzip.GzipFile(fileobj=f, mode='z')
        for mode in "rb", "r+b":
            with open(self.filename, mode) as f:
                with gzip.GzipFile(fileobj=f) as g:
                    self.assertEqual(g.mode, gzip.READ)
        for mode in "wb", "ab", "xb":
            if "x" in mode:
                os_helper.unlink(self.filename)
            with open(self.filename, mode) as f:
                with self.assertWarns(FutureWarning):
                    g = gzip.GzipFile(fileobj=f)
                with g:
                    self.assertEqual(g.mode, gzip.WRITE)

    def test_bytes_filename(self):
        str_filename = self.filename
        try:
            bytes_filename = str_filename.encode("ascii")
        except UnicodeEncodeError:
            self.skipTest("Temporary file name needs to be ASCII")
        with gzip.GzipFile(bytes_filename, "wb") as f:
            f.write(data1 * 50)
        with gzip.GzipFile(bytes_filename, "rb") as f:
            self.assertEqual(f.read(), data1 * 50)
        # Sanity check that we are actually operating on the right file.
        with gzip.GzipFile(str_filename, "rb") as f:
            self.assertEqual(f.read(), data1 * 50)

    def test_decompress_limited(self):
        """Decompressed data buffering should be limited"""
        bomb = gzip.compress(b'\0' * int(2e6), compresslevel=9)
        self.assertLess(len(bomb), io.DEFAULT_BUFFER_SIZE)

        bomb = io.BytesIO(bomb)
        decomp = gzip.GzipFile(fileobj=bomb)
        self.assertEqual(decomp.read(1), b'\0')
        max_decomp = 1 + io.DEFAULT_BUFFER_SIZE
        self.assertLessEqual(decomp._buffer.raw.tell(), max_decomp,
            "Excessive amount of data was decompressed")

    # Testing compress/decompress shortcut functions

    def test_compress(self):
        for data in [data1, data2]:
            for args in [(), (1,), (6,), (9,)]:
                datac = gzip.compress(data, *args)
                self.assertEqual(type(datac), bytes)
                with gzip.GzipFile(fileobj=io.BytesIO(datac), mode="rb") as f:
                    self.assertEqual(f.read(), data)

    def test_compress_mtime(self):
        mtime = 123456789
        for data in [data1, data2]:
            for args in [(), (1,), (6,), (9,)]:
                with self.subTest(data=data, args=args):
                    datac = gzip.compress(data, *args, mtime=mtime)
                    self.assertEqual(type(datac), bytes)
                    with gzip.GzipFile(fileobj=io.BytesIO(datac), mode="rb") as f:
                        f.read(1) # to set mtime attribute
                        self.assertEqual(f.mtime, mtime)

    def test_decompress(self):
        for data in (data1, data2):
            buf = io.BytesIO()
            with gzip.GzipFile(fileobj=buf, mode="wb") as f:
                f.write(data)
            self.assertEqual(gzip.decompress(buf.getvalue()), data)
            # Roundtrip with compress
            datac = gzip.compress(data)
            self.assertEqual(gzip.decompress(datac), data)

    def test_read_truncated(self):
        data = data1*50
        # Drop the CRC (4 bytes) and file size (4 bytes).
        truncated = gzip.compress(data)[:-8]
        with gzip.GzipFile(fileobj=io.BytesIO(truncated)) as f:
            self.assertRaises(EOFError, f.read)
        with gzip.GzipFile(fileobj=io.BytesIO(truncated)) as f:
            self.assertEqual(f.read(len(data)), data)
            self.assertRaises(EOFError, f.read, 1)
        # Incomplete 10-byte header.
        for i in range(2, 10):
            with gzip.GzipFile(fileobj=io.BytesIO(truncated[:i])) as f:
                self.assertRaises(EOFError, f.read, 1)

    def test_read_with_extra(self):
        # Gzip data with an extra field
        gzdata = (b'\x1f\x8b\x08\x04\xb2\x17cQ\x02\xff'
                  b'\x05\x00Extra'
                  b'\x0bI-.\x01\x002\xd1Mx\x04\x00\x00\x00')
        with gzip.GzipFile(fileobj=io.BytesIO(gzdata)) as f:
            self.assertEqual(f.read(), b'Test')

    def test_prepend_error(self):
        # See issue #20875
        with gzip.open(self.filename, "wb") as f:
            f.write(data1)
        with gzip.open(self.filename, "rb") as f:
            f._buffer.raw._fp.prepend()

class TestOpen(BaseTest):
    def test_binary_modes(self):
        uncompressed = data1 * 50

        with gzip.open(self.filename, "wb") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read())
            self.assertEqual(file_data, uncompressed)

        with gzip.open(self.filename, "rb") as f:
            self.assertEqual(f.read(), uncompressed)

        with gzip.open(self.filename, "ab") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read())
            self.assertEqual(file_data, uncompressed * 2)

        with self.assertRaises(FileExistsError):
            gzip.open(self.filename, "xb")
        os_helper.unlink(self.filename)
        with gzip.open(self.filename, "xb") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read())
            self.assertEqual(file_data, uncompressed)

    def test_pathlike_file(self):
        filename = pathlib.Path(self.filename)
        with gzip.open(filename, "wb") as f:
            f.write(data1 * 50)
        with gzip.open(filename, "ab") as f:
            f.write(data1)
        with gzip.open(filename) as f:
            self.assertEqual(f.read(), data1 * 51)

    def test_implicit_binary_modes(self):
        # Test implicit binary modes (no "b" or "t" in mode string).
        uncompressed = data1 * 50

        with gzip.open(self.filename, "w") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read())
            self.assertEqual(file_data, uncompressed)

        with gzip.open(self.filename, "r") as f:
            self.assertEqual(f.read(), uncompressed)

        with gzip.open(self.filename, "a") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read())
            self.assertEqual(file_data, uncompressed * 2)

        with self.assertRaises(FileExistsError):
            gzip.open(self.filename, "x")
        os_helper.unlink(self.filename)
        with gzip.open(self.filename, "x") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read())
            self.assertEqual(file_data, uncompressed)

    def test_text_modes(self):
        uncompressed = data1.decode("ascii") * 50
        uncompressed_raw = uncompressed.replace("\n", os.linesep)
        with gzip.open(self.filename, "wt") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read()).decode("ascii")
            self.assertEqual(file_data, uncompressed_raw)
        with gzip.open(self.filename, "rt") as f:
            self.assertEqual(f.read(), uncompressed)
        with gzip.open(self.filename, "at") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read()).decode("ascii")
            self.assertEqual(file_data, uncompressed_raw * 2)

    def test_fileobj(self):
        uncompressed_bytes = data1 * 50
        uncompressed_str = uncompressed_bytes.decode("ascii")
        compressed = gzip.compress(uncompressed_bytes)
        with gzip.open(io.BytesIO(compressed), "r") as f:
            self.assertEqual(f.read(), uncompressed_bytes)
        with gzip.open(io.BytesIO(compressed), "rb") as f:
            self.assertEqual(f.read(), uncompressed_bytes)
        with gzip.open(io.BytesIO(compressed), "rt") as f:
            self.assertEqual(f.read(), uncompressed_str)

    def test_bad_params(self):
        # Test invalid parameter combinations.
        with self.assertRaises(TypeError):
            gzip.open(123.456)
        with self.assertRaises(ValueError):
            gzip.open(self.filename, "wbt")
        with self.assertRaises(ValueError):
            gzip.open(self.filename, "xbt")
        with self.assertRaises(ValueError):
            gzip.open(self.filename, "rb", encoding="utf-8")
        with self.assertRaises(ValueError):
            gzip.open(self.filename, "rb", errors="ignore")
        with self.assertRaises(ValueError):
            gzip.open(self.filename, "rb", newline="\n")

    def test_encoding(self):
        # Test non-default encoding.
        uncompressed = data1.decode("ascii") * 50
        uncompressed_raw = uncompressed.replace("\n", os.linesep)
        with gzip.open(self.filename, "wt", encoding="utf-16") as f:
            f.write(uncompressed)
        with open(self.filename, "rb") as f:
            file_data = gzip.decompress(f.read()).decode("utf-16")
            self.assertEqual(file_data, uncompressed_raw)
        with gzip.open(self.filename, "rt", encoding="utf-16") as f:
            self.assertEqual(f.read(), uncompressed)

    def test_encoding_error_handler(self):
        # Test with non-default encoding error handler.
        with gzip.open(self.filename, "wb") as f:
            f.write(b"foo\xffbar")
        with gzip.open(self.filename, "rt", encoding="ascii", errors="ignore") \
                as f:
            self.assertEqual(f.read(), "foobar")

    def test_newline(self):
        # Test with explicit newline (universal newline mode disabled).
        uncompressed = data1.decode("ascii") * 50
        with gzip.open(self.filename, "wt", newline="\n") as f:
            f.write(uncompressed)
        with gzip.open(self.filename, "rt", newline="\r") as f:
            self.assertEqual(f.readlines(), [uncompressed])


def create_and_remove_directory(directory):
    def decorator(function):
        @functools.wraps(function)
        def wrapper(*args, **kwargs):
            os.makedirs(directory)
            try:
                return function(*args, **kwargs)
            finally:
                os_helper.rmtree(directory)
        return wrapper
    return decorator


class TestCommandLine(unittest.TestCase):
    data = b'This is a simple test with gzip'

    def test_decompress_stdin_stdout(self):
        with io.BytesIO() as bytes_io:
            with gzip.GzipFile(fileobj=bytes_io, mode='wb') as gzip_file:
                gzip_file.write(self.data)

            args = sys.executable, '-m', 'gzip', '-d'
            with Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
                out, err = proc.communicate(bytes_io.getvalue())

        self.assertEqual(err, b'')
        self.assertEqual(out, self.data)

    @create_and_remove_directory(TEMPDIR)
    def test_decompress_infile_outfile(self):
        gzipname = os.path.join(TEMPDIR, 'testgzip.gz')
        self.assertFalse(os.path.exists(gzipname))

        with gzip.open(gzipname, mode='wb') as fp:
            fp.write(self.data)
        rc, out, err = assert_python_ok('-m', 'gzip', '-d', gzipname)

        with open(os.path.join(TEMPDIR, "testgzip"), "rb") as gunziped:
            self.assertEqual(gunziped.read(), self.data)

        self.assertTrue(os.path.exists(gzipname))
        self.assertEqual(rc, 0)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

    def test_decompress_infile_outfile_error(self):
        rc, out, err = assert_python_failure('-m', 'gzip', '-d', 'thisisatest.out')
        self.assertIn(b"filename doesn't end in .gz:", err)
        self.assertEqual(rc, 1)
        self.assertEqual(out, b'')

    @create_and_remove_directory(TEMPDIR)
    def test_compress_stdin_outfile(self):
        args = sys.executable, '-m', 'gzip'
        with Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
            out, err = proc.communicate(self.data)

        self.assertEqual(err, b'')
        self.assertEqual(out[:2], b"\x1f\x8b")

    @create_and_remove_directory(TEMPDIR)
    def test_compress_infile_outfile_default(self):
        local_testgzip = os.path.join(TEMPDIR, 'testgzip')
        gzipname = local_testgzip + '.gz'
        self.assertFalse(os.path.exists(gzipname))

        with open(local_testgzip, 'wb') as fp:
            fp.write(self.data)

        rc, out, err = assert_python_ok('-m', 'gzip', local_testgzip)

        self.assertTrue(os.path.exists(gzipname))
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

    @create_and_remove_directory(TEMPDIR)
    def test_compress_infile_outfile(self):
        for compress_level in ('--fast', '--best'):
            with self.subTest(compress_level=compress_level):
                local_testgzip = os.path.join(TEMPDIR, 'testgzip')
                gzipname = local_testgzip + '.gz'
                self.assertFalse(os.path.exists(gzipname))

                with open(local_testgzip, 'wb') as fp:
                    fp.write(self.data)

                rc, out, err = assert_python_ok('-m', 'gzip', compress_level, local_testgzip)

                self.assertTrue(os.path.exists(gzipname))
                self.assertEqual(out, b'')
                self.assertEqual(err, b'')
                os.remove(gzipname)
                self.assertFalse(os.path.exists(gzipname))

    def test_compress_fast_best_are_exclusive(self):
        rc, out, err = assert_python_failure('-m', 'gzip', '--fast', '--best')
        self.assertIn(b"error: argument --best: not allowed with argument --fast", err)
        self.assertEqual(out, b'')

    def test_decompress_cannot_have_flags_compression(self):
        rc, out, err = assert_python_failure('-m', 'gzip', '--fast', '-d')
        self.assertIn(b'error: argument -d/--decompress: not allowed with argument --fast', err)
        self.assertEqual(out, b'')


def test_main(verbose=None):
    support.run_unittest(TestGzip, TestOpen, TestCommandLine)


if __name__ == "__main__":
    test_main(verbose=True)
