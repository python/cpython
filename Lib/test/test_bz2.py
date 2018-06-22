from test import support
from test.support import bigmemtest, _4G

import unittest
from io import BytesIO, DEFAULT_BUFFER_SIZE
import os
import pickle
import glob
import pathlib
import random
import shutil
import subprocess
import threading
from test.support import unlink
import _compression
import sys


# Skip tests if the bz2 module doesn't exist.
bz2 = support.import_module('bz2')
from bz2 import BZ2File, BZ2Compressor, BZ2Decompressor

has_cmdline_bunzip2 = None

def ext_decompress(data):
    global has_cmdline_bunzip2
    if has_cmdline_bunzip2 is None:
        has_cmdline_bunzip2 = bool(shutil.which('bunzip2'))
    if has_cmdline_bunzip2:
        return subprocess.check_output(['bunzip2'], input=data)
    else:
        return bz2.decompress(data)

class BaseTest(unittest.TestCase):
    "Base for other testcases."

    TEXT_LINES = [
        b'root:x:0:0:root:/root:/bin/bash\n',
        b'bin:x:1:1:bin:/bin:\n',
        b'daemon:x:2:2:daemon:/sbin:\n',
        b'adm:x:3:4:adm:/var/adm:\n',
        b'lp:x:4:7:lp:/var/spool/lpd:\n',
        b'sync:x:5:0:sync:/sbin:/bin/sync\n',
        b'shutdown:x:6:0:shutdown:/sbin:/sbin/shutdown\n',
        b'halt:x:7:0:halt:/sbin:/sbin/halt\n',
        b'mail:x:8:12:mail:/var/spool/mail:\n',
        b'news:x:9:13:news:/var/spool/news:\n',
        b'uucp:x:10:14:uucp:/var/spool/uucp:\n',
        b'operator:x:11:0:operator:/root:\n',
        b'games:x:12:100:games:/usr/games:\n',
        b'gopher:x:13:30:gopher:/usr/lib/gopher-data:\n',
        b'ftp:x:14:50:FTP User:/var/ftp:/bin/bash\n',
        b'nobody:x:65534:65534:Nobody:/home:\n',
        b'postfix:x:100:101:postfix:/var/spool/postfix:\n',
        b'niemeyer:x:500:500::/home/niemeyer:/bin/bash\n',
        b'postgres:x:101:102:PostgreSQL Server:/var/lib/pgsql:/bin/bash\n',
        b'mysql:x:102:103:MySQL server:/var/lib/mysql:/bin/bash\n',
        b'www:x:103:104::/var/www:/bin/false\n',
        ]
    TEXT = b''.join(TEXT_LINES)
    DATA = b'BZh91AY&SY.\xc8N\x18\x00\x01>_\x80\x00\x10@\x02\xff\xf0\x01\x07n\x00?\xe7\xff\xe00\x01\x99\xaa\x00\xc0\x03F\x86\x8c#&\x83F\x9a\x03\x06\xa6\xd0\xa6\x93M\x0fQ\xa7\xa8\x06\x804hh\x12$\x11\xa4i4\xf14S\xd2<Q\xb5\x0fH\xd3\xd4\xdd\xd5\x87\xbb\xf8\x94\r\x8f\xafI\x12\xe1\xc9\xf8/E\x00pu\x89\x12]\xc9\xbbDL\nQ\x0e\t1\x12\xdf\xa0\xc0\x97\xac2O9\x89\x13\x94\x0e\x1c7\x0ed\x95I\x0c\xaaJ\xa4\x18L\x10\x05#\x9c\xaf\xba\xbc/\x97\x8a#C\xc8\xe1\x8cW\xf9\xe2\xd0\xd6M\xa7\x8bXa<e\x84t\xcbL\xb3\xa7\xd9\xcd\xd1\xcb\x84.\xaf\xb3\xab\xab\xad`n}\xa0lh\tE,\x8eZ\x15\x17VH>\x88\xe5\xcd9gd6\x0b\n\xe9\x9b\xd5\x8a\x99\xf7\x08.K\x8ev\xfb\xf7xw\xbb\xdf\xa1\x92\xf1\xdd|/";\xa2\xba\x9f\xd5\xb1#A\xb6\xf6\xb3o\xc9\xc5y\\\xebO\xe7\x85\x9a\xbc\xb6f8\x952\xd5\xd7"%\x89>V,\xf7\xa6z\xe2\x9f\xa3\xdf\x11\x11"\xd6E)I\xa9\x13^\xca\xf3r\xd0\x03U\x922\xf26\xec\xb6\xed\x8b\xc3U\x13\x9d\xc5\x170\xa4\xfa^\x92\xacDF\x8a\x97\xd6\x19\xfe\xdd\xb8\xbd\x1a\x9a\x19\xa3\x80ankR\x8b\xe5\xd83]\xa9\xc6\x08\x82f\xf6\xb9"6l$\xb8j@\xc0\x8a\xb0l1..\xbak\x83ls\x15\xbc\xf4\xc1\x13\xbe\xf8E\xb8\x9d\r\xa8\x9dk\x84\xd3n\xfa\xacQ\x07\xb1%y\xaav\xb4\x08\xe0z\x1b\x16\xf5\x04\xe9\xcc\xb9\x08z\x1en7.G\xfc]\xc9\x14\xe1B@\xbb!8`'
    EMPTY_DATA = b'BZh9\x17rE8P\x90\x00\x00\x00\x00'
    BAD_DATA = b'this is not a valid bzip2 file'

    # Some tests need more than one block of uncompressed data. Since one block
    # is at least 100,000 bytes, we gather some data dynamically and compress it.
    # Note that this assumes that compression works correctly, so we cannot
    # simply use the bigger test data for all tests.
    test_size = 0
    BIG_TEXT = bytearray(128*1024)
    for fname in glob.glob(os.path.join(os.path.dirname(__file__), '*.py')):
        with open(fname, 'rb') as fh:
            test_size += fh.readinto(memoryview(BIG_TEXT)[test_size:])
        if test_size > 128*1024:
            break
    BIG_DATA = bz2.compress(BIG_TEXT, compresslevel=1)

    def setUp(self):
        self.filename = support.TESTFN

    def tearDown(self):
        if os.path.isfile(self.filename):
            os.unlink(self.filename)


class BZ2FileTest(BaseTest):
    "Test the BZ2File class."

    def createTempFile(self, streams=1, suffix=b""):
        with open(self.filename, "wb") as f:
            f.write(self.DATA * streams)
            f.write(suffix)

    def testBadArgs(self):
        self.assertRaises(TypeError, BZ2File, 123.456)
        self.assertRaises(ValueError, BZ2File, os.devnull, "z")
        self.assertRaises(ValueError, BZ2File, os.devnull, "rx")
        self.assertRaises(ValueError, BZ2File, os.devnull, "rbt")
        self.assertRaises(ValueError, BZ2File, os.devnull, compresslevel=0)
        self.assertRaises(ValueError, BZ2File, os.devnull, compresslevel=10)

    def testRead(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.read, float())
            self.assertEqual(bz2f.read(), self.TEXT)

    def testReadBadFile(self):
        self.createTempFile(streams=0, suffix=self.BAD_DATA)
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(OSError, bz2f.read)

    def testReadMultiStream(self):
        self.createTempFile(streams=5)
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.read, float())
            self.assertEqual(bz2f.read(), self.TEXT * 5)

    def testReadMonkeyMultiStream(self):
        # Test BZ2File.read() on a multi-stream archive where a stream
        # boundary coincides with the end of the raw read buffer.
        buffer_size = _compression.BUFFER_SIZE
        _compression.BUFFER_SIZE = len(self.DATA)
        try:
            self.createTempFile(streams=5)
            with BZ2File(self.filename) as bz2f:
                self.assertRaises(TypeError, bz2f.read, float())
                self.assertEqual(bz2f.read(), self.TEXT * 5)
        finally:
            _compression.BUFFER_SIZE = buffer_size

    def testReadTrailingJunk(self):
        self.createTempFile(suffix=self.BAD_DATA)
        with BZ2File(self.filename) as bz2f:
            self.assertEqual(bz2f.read(), self.TEXT)

    def testReadMultiStreamTrailingJunk(self):
        self.createTempFile(streams=5, suffix=self.BAD_DATA)
        with BZ2File(self.filename) as bz2f:
            self.assertEqual(bz2f.read(), self.TEXT * 5)

    def testRead0(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.read, float())
            self.assertEqual(bz2f.read(0), b"")

    def testReadChunk10(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            text = b''
            while True:
                str = bz2f.read(10)
                if not str:
                    break
                text += str
            self.assertEqual(text, self.TEXT)

    def testReadChunk10MultiStream(self):
        self.createTempFile(streams=5)
        with BZ2File(self.filename) as bz2f:
            text = b''
            while True:
                str = bz2f.read(10)
                if not str:
                    break
                text += str
            self.assertEqual(text, self.TEXT * 5)

    def testRead100(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertEqual(bz2f.read(100), self.TEXT[:100])

    def testPeek(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            pdata = bz2f.peek()
            self.assertNotEqual(len(pdata), 0)
            self.assertTrue(self.TEXT.startswith(pdata))
            self.assertEqual(bz2f.read(), self.TEXT)

    def testReadInto(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            n = 128
            b = bytearray(n)
            self.assertEqual(bz2f.readinto(b), n)
            self.assertEqual(b, self.TEXT[:n])
            n = len(self.TEXT) - n
            b = bytearray(len(self.TEXT))
            self.assertEqual(bz2f.readinto(b), n)
            self.assertEqual(b[:n], self.TEXT[-n:])

    def testReadLine(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.readline, None)
            for line in self.TEXT_LINES:
                self.assertEqual(bz2f.readline(), line)

    def testReadLineMultiStream(self):
        self.createTempFile(streams=5)
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.readline, None)
            for line in self.TEXT_LINES * 5:
                self.assertEqual(bz2f.readline(), line)

    def testReadLines(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.readlines, None)
            self.assertEqual(bz2f.readlines(), self.TEXT_LINES)

    def testReadLinesMultiStream(self):
        self.createTempFile(streams=5)
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.readlines, None)
            self.assertEqual(bz2f.readlines(), self.TEXT_LINES * 5)

    def testIterator(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertEqual(list(iter(bz2f)), self.TEXT_LINES)

    def testIteratorMultiStream(self):
        self.createTempFile(streams=5)
        with BZ2File(self.filename) as bz2f:
            self.assertEqual(list(iter(bz2f)), self.TEXT_LINES * 5)

    def testClosedIteratorDeadlock(self):
        # Issue #3309: Iteration on a closed BZ2File should release the lock.
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        bz2f.close()
        self.assertRaises(ValueError, next, bz2f)
        # This call will deadlock if the above call failed to release the lock.
        self.assertRaises(ValueError, bz2f.readlines)

    def testWrite(self):
        with BZ2File(self.filename, "w") as bz2f:
            self.assertRaises(TypeError, bz2f.write)
            bz2f.write(self.TEXT)
        with open(self.filename, 'rb') as f:
            self.assertEqual(ext_decompress(f.read()), self.TEXT)

    def testWriteChunks10(self):
        with BZ2File(self.filename, "w") as bz2f:
            n = 0
            while True:
                str = self.TEXT[n*10:(n+1)*10]
                if not str:
                    break
                bz2f.write(str)
                n += 1
        with open(self.filename, 'rb') as f:
            self.assertEqual(ext_decompress(f.read()), self.TEXT)

    def testWriteNonDefaultCompressLevel(self):
        expected = bz2.compress(self.TEXT, compresslevel=5)
        with BZ2File(self.filename, "w", compresslevel=5) as bz2f:
            bz2f.write(self.TEXT)
        with open(self.filename, "rb") as f:
            self.assertEqual(f.read(), expected)

    def testWriteLines(self):
        with BZ2File(self.filename, "w") as bz2f:
            self.assertRaises(TypeError, bz2f.writelines)
            bz2f.writelines(self.TEXT_LINES)
        # Issue #1535500: Calling writelines() on a closed BZ2File
        # should raise an exception.
        self.assertRaises(ValueError, bz2f.writelines, ["a"])
        with open(self.filename, 'rb') as f:
            self.assertEqual(ext_decompress(f.read()), self.TEXT)

    def testWriteMethodsOnReadOnlyFile(self):
        with BZ2File(self.filename, "w") as bz2f:
            bz2f.write(b"abc")

        with BZ2File(self.filename, "r") as bz2f:
            self.assertRaises(OSError, bz2f.write, b"a")
            self.assertRaises(OSError, bz2f.writelines, [b"a"])

    def testAppend(self):
        with BZ2File(self.filename, "w") as bz2f:
            self.assertRaises(TypeError, bz2f.write)
            bz2f.write(self.TEXT)
        with BZ2File(self.filename, "a") as bz2f:
            self.assertRaises(TypeError, bz2f.write)
            bz2f.write(self.TEXT)
        with open(self.filename, 'rb') as f:
            self.assertEqual(ext_decompress(f.read()), self.TEXT * 2)

    def testSeekForward(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.seek)
            bz2f.seek(150)
            self.assertEqual(bz2f.read(), self.TEXT[150:])

    def testSeekForwardAcrossStreams(self):
        self.createTempFile(streams=2)
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.seek)
            bz2f.seek(len(self.TEXT) + 150)
            self.assertEqual(bz2f.read(), self.TEXT[150:])

    def testSeekBackwards(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.read(500)
            bz2f.seek(-150, 1)
            self.assertEqual(bz2f.read(), self.TEXT[500-150:])

    def testSeekBackwardsAcrossStreams(self):
        self.createTempFile(streams=2)
        with BZ2File(self.filename) as bz2f:
            readto = len(self.TEXT) + 100
            while readto > 0:
                readto -= len(bz2f.read(readto))
            bz2f.seek(-150, 1)
            self.assertEqual(bz2f.read(), self.TEXT[100-150:] + self.TEXT)

    def testSeekBackwardsFromEnd(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(-150, 2)
            self.assertEqual(bz2f.read(), self.TEXT[len(self.TEXT)-150:])

    def testSeekBackwardsFromEndAcrossStreams(self):
        self.createTempFile(streams=2)
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(-1000, 2)
            self.assertEqual(bz2f.read(), (self.TEXT * 2)[-1000:])

    def testSeekPostEnd(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(150000)
            self.assertEqual(bz2f.tell(), len(self.TEXT))
            self.assertEqual(bz2f.read(), b"")

    def testSeekPostEndMultiStream(self):
        self.createTempFile(streams=5)
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(150000)
            self.assertEqual(bz2f.tell(), len(self.TEXT) * 5)
            self.assertEqual(bz2f.read(), b"")

    def testSeekPostEndTwice(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(150000)
            bz2f.seek(150000)
            self.assertEqual(bz2f.tell(), len(self.TEXT))
            self.assertEqual(bz2f.read(), b"")

    def testSeekPostEndTwiceMultiStream(self):
        self.createTempFile(streams=5)
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(150000)
            bz2f.seek(150000)
            self.assertEqual(bz2f.tell(), len(self.TEXT) * 5)
            self.assertEqual(bz2f.read(), b"")

    def testSeekPreStart(self):
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(-150)
            self.assertEqual(bz2f.tell(), 0)
            self.assertEqual(bz2f.read(), self.TEXT)

    def testSeekPreStartMultiStream(self):
        self.createTempFile(streams=2)
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(-150)
            self.assertEqual(bz2f.tell(), 0)
            self.assertEqual(bz2f.read(), self.TEXT * 2)

    def testFileno(self):
        self.createTempFile()
        with open(self.filename, 'rb') as rawf:
            bz2f = BZ2File(rawf)
            try:
                self.assertEqual(bz2f.fileno(), rawf.fileno())
            finally:
                bz2f.close()
        self.assertRaises(ValueError, bz2f.fileno)

    def testSeekable(self):
        bz2f = BZ2File(BytesIO(self.DATA))
        try:
            self.assertTrue(bz2f.seekable())
            bz2f.read()
            self.assertTrue(bz2f.seekable())
        finally:
            bz2f.close()
        self.assertRaises(ValueError, bz2f.seekable)

        bz2f = BZ2File(BytesIO(), "w")
        try:
            self.assertFalse(bz2f.seekable())
        finally:
            bz2f.close()
        self.assertRaises(ValueError, bz2f.seekable)

        src = BytesIO(self.DATA)
        src.seekable = lambda: False
        bz2f = BZ2File(src)
        try:
            self.assertFalse(bz2f.seekable())
        finally:
            bz2f.close()
        self.assertRaises(ValueError, bz2f.seekable)

    def testReadable(self):
        bz2f = BZ2File(BytesIO(self.DATA))
        try:
            self.assertTrue(bz2f.readable())
            bz2f.read()
            self.assertTrue(bz2f.readable())
        finally:
            bz2f.close()
        self.assertRaises(ValueError, bz2f.readable)

        bz2f = BZ2File(BytesIO(), "w")
        try:
            self.assertFalse(bz2f.readable())
        finally:
            bz2f.close()
        self.assertRaises(ValueError, bz2f.readable)

    def testWritable(self):
        bz2f = BZ2File(BytesIO(self.DATA))
        try:
            self.assertFalse(bz2f.writable())
            bz2f.read()
            self.assertFalse(bz2f.writable())
        finally:
            bz2f.close()
        self.assertRaises(ValueError, bz2f.writable)

        bz2f = BZ2File(BytesIO(), "w")
        try:
            self.assertTrue(bz2f.writable())
        finally:
            bz2f.close()
        self.assertRaises(ValueError, bz2f.writable)

    def testOpenDel(self):
        self.createTempFile()
        for i in range(10000):
            o = BZ2File(self.filename)
            del o

    def testOpenNonexistent(self):
        self.assertRaises(OSError, BZ2File, "/non/existent")

    def testReadlinesNoNewline(self):
        # Issue #1191043: readlines() fails on a file containing no newline.
        data = b'BZh91AY&SY\xd9b\x89]\x00\x00\x00\x03\x80\x04\x00\x02\x00\x0c\x00 \x00!\x9ah3M\x13<]\xc9\x14\xe1BCe\x8a%t'
        with open(self.filename, "wb") as f:
            f.write(data)
        with BZ2File(self.filename) as bz2f:
            lines = bz2f.readlines()
        self.assertEqual(lines, [b'Test'])
        with BZ2File(self.filename) as bz2f:
            xlines = list(bz2f.readlines())
        self.assertEqual(xlines, [b'Test'])

    def testContextProtocol(self):
        f = None
        with BZ2File(self.filename, "wb") as f:
            f.write(b"xxx")
        f = BZ2File(self.filename, "rb")
        f.close()
        try:
            with f:
                pass
        except ValueError:
            pass
        else:
            self.fail("__enter__ on a closed file didn't raise an exception")
        try:
            with BZ2File(self.filename, "wb") as f:
                1/0
        except ZeroDivisionError:
            pass
        else:
            self.fail("1/0 didn't raise an exception")

    def testThreading(self):
        # Issue #7205: Using a BZ2File from several threads shouldn't deadlock.
        data = b"1" * 2**20
        nthreads = 10
        with BZ2File(self.filename, 'wb') as f:
            def comp():
                for i in range(5):
                    f.write(data)
            threads = [threading.Thread(target=comp) for i in range(nthreads)]
            with support.start_threads(threads):
                pass

    def testMixedIterationAndReads(self):
        self.createTempFile()
        linelen = len(self.TEXT_LINES[0])
        halflen = linelen // 2
        with BZ2File(self.filename) as bz2f:
            bz2f.read(halflen)
            self.assertEqual(next(bz2f), self.TEXT_LINES[0][halflen:])
            self.assertEqual(bz2f.read(), self.TEXT[linelen:])
        with BZ2File(self.filename) as bz2f:
            bz2f.readline()
            self.assertEqual(next(bz2f), self.TEXT_LINES[1])
            self.assertEqual(bz2f.readline(), self.TEXT_LINES[2])
        with BZ2File(self.filename) as bz2f:
            bz2f.readlines()
            self.assertRaises(StopIteration, next, bz2f)
            self.assertEqual(bz2f.readlines(), [])

    def testMultiStreamOrdering(self):
        # Test the ordering of streams when reading a multi-stream archive.
        data1 = b"foo" * 1000
        data2 = b"bar" * 1000
        with BZ2File(self.filename, "w") as bz2f:
            bz2f.write(data1)
        with BZ2File(self.filename, "a") as bz2f:
            bz2f.write(data2)
        with BZ2File(self.filename) as bz2f:
            self.assertEqual(bz2f.read(), data1 + data2)

    def testOpenBytesFilename(self):
        str_filename = self.filename
        try:
            bytes_filename = str_filename.encode("ascii")
        except UnicodeEncodeError:
            self.skipTest("Temporary file name needs to be ASCII")
        with BZ2File(bytes_filename, "wb") as f:
            f.write(self.DATA)
        with BZ2File(bytes_filename, "rb") as f:
            self.assertEqual(f.read(), self.DATA)
        # Sanity check that we are actually operating on the right file.
        with BZ2File(str_filename, "rb") as f:
            self.assertEqual(f.read(), self.DATA)

    def testOpenPathLikeFilename(self):
        filename = pathlib.Path(self.filename)
        with BZ2File(filename, "wb") as f:
            f.write(self.DATA)
        with BZ2File(filename, "rb") as f:
            self.assertEqual(f.read(), self.DATA)

    def testDecompressLimited(self):
        """Decompressed data buffering should be limited"""
        bomb = bz2.compress(b'\0' * int(2e6), compresslevel=9)
        self.assertLess(len(bomb), _compression.BUFFER_SIZE)

        decomp = BZ2File(BytesIO(bomb))
        self.assertEqual(decomp.read(1), b'\0')
        max_decomp = 1 + DEFAULT_BUFFER_SIZE
        self.assertLessEqual(decomp._buffer.raw.tell(), max_decomp,
            "Excessive amount of data was decompressed")


    # Tests for a BZ2File wrapping another file object:

    def testReadBytesIO(self):
        with BytesIO(self.DATA) as bio:
            with BZ2File(bio) as bz2f:
                self.assertRaises(TypeError, bz2f.read, float())
                self.assertEqual(bz2f.read(), self.TEXT)
            self.assertFalse(bio.closed)

    def testPeekBytesIO(self):
        with BytesIO(self.DATA) as bio:
            with BZ2File(bio) as bz2f:
                pdata = bz2f.peek()
                self.assertNotEqual(len(pdata), 0)
                self.assertTrue(self.TEXT.startswith(pdata))
                self.assertEqual(bz2f.read(), self.TEXT)

    def testWriteBytesIO(self):
        with BytesIO() as bio:
            with BZ2File(bio, "w") as bz2f:
                self.assertRaises(TypeError, bz2f.write)
                bz2f.write(self.TEXT)
            self.assertEqual(ext_decompress(bio.getvalue()), self.TEXT)
            self.assertFalse(bio.closed)

    def testSeekForwardBytesIO(self):
        with BytesIO(self.DATA) as bio:
            with BZ2File(bio) as bz2f:
                self.assertRaises(TypeError, bz2f.seek)
                bz2f.seek(150)
                self.assertEqual(bz2f.read(), self.TEXT[150:])

    def testSeekBackwardsBytesIO(self):
        with BytesIO(self.DATA) as bio:
            with BZ2File(bio) as bz2f:
                bz2f.read(500)
                bz2f.seek(-150, 1)
                self.assertEqual(bz2f.read(), self.TEXT[500-150:])

    def test_read_truncated(self):
        # Drop the eos_magic field (6 bytes) and CRC (4 bytes).
        truncated = self.DATA[:-10]
        with BZ2File(BytesIO(truncated)) as f:
            self.assertRaises(EOFError, f.read)
        with BZ2File(BytesIO(truncated)) as f:
            self.assertEqual(f.read(len(self.TEXT)), self.TEXT)
            self.assertRaises(EOFError, f.read, 1)
        # Incomplete 4-byte file header, and block header of at least 146 bits.
        for i in range(22):
            with BZ2File(BytesIO(truncated[:i])) as f:
                self.assertRaises(EOFError, f.read, 1)


class BZ2CompressorTest(BaseTest):
    def testCompress(self):
        bz2c = BZ2Compressor()
        self.assertRaises(TypeError, bz2c.compress)
        data = bz2c.compress(self.TEXT)
        data += bz2c.flush()
        self.assertEqual(ext_decompress(data), self.TEXT)

    def testCompressEmptyString(self):
        bz2c = BZ2Compressor()
        data = bz2c.compress(b'')
        data += bz2c.flush()
        self.assertEqual(data, self.EMPTY_DATA)

    def testCompressChunks10(self):
        bz2c = BZ2Compressor()
        n = 0
        data = b''
        while True:
            str = self.TEXT[n*10:(n+1)*10]
            if not str:
                break
            data += bz2c.compress(str)
            n += 1
        data += bz2c.flush()
        self.assertEqual(ext_decompress(data), self.TEXT)

    @bigmemtest(size=_4G + 100, memuse=2)
    def testCompress4G(self, size):
        # "Test BZ2Compressor.compress()/flush() with >4GiB input"
        bz2c = BZ2Compressor()
        data = b"x" * size
        try:
            compressed = bz2c.compress(data)
            compressed += bz2c.flush()
        finally:
            data = None  # Release memory
        data = bz2.decompress(compressed)
        try:
            self.assertEqual(len(data), size)
            self.assertEqual(len(data.strip(b"x")), 0)
        finally:
            data = None

    def testPickle(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises(TypeError):
                pickle.dumps(BZ2Compressor(), proto)


class BZ2DecompressorTest(BaseTest):
    def test_Constructor(self):
        self.assertRaises(TypeError, BZ2Decompressor, 42)

    def testDecompress(self):
        bz2d = BZ2Decompressor()
        self.assertRaises(TypeError, bz2d.decompress)
        text = bz2d.decompress(self.DATA)
        self.assertEqual(text, self.TEXT)

    def testDecompressChunks10(self):
        bz2d = BZ2Decompressor()
        text = b''
        n = 0
        while True:
            str = self.DATA[n*10:(n+1)*10]
            if not str:
                break
            text += bz2d.decompress(str)
            n += 1
        self.assertEqual(text, self.TEXT)

    def testDecompressUnusedData(self):
        bz2d = BZ2Decompressor()
        unused_data = b"this is unused data"
        text = bz2d.decompress(self.DATA+unused_data)
        self.assertEqual(text, self.TEXT)
        self.assertEqual(bz2d.unused_data, unused_data)

    def testEOFError(self):
        bz2d = BZ2Decompressor()
        text = bz2d.decompress(self.DATA)
        self.assertRaises(EOFError, bz2d.decompress, b"anything")
        self.assertRaises(EOFError, bz2d.decompress, b"")

    @bigmemtest(size=_4G + 100, memuse=3.3)
    def testDecompress4G(self, size):
        # "Test BZ2Decompressor.decompress() with >4GiB input"
        blocksize = 10 * 1024 * 1024
        block = random.getrandbits(blocksize * 8).to_bytes(blocksize, 'little')
        try:
            data = block * (size // blocksize + 1)
            compressed = bz2.compress(data)
            bz2d = BZ2Decompressor()
            decompressed = bz2d.decompress(compressed)
            self.assertTrue(decompressed == data)
        finally:
            data = None
            compressed = None
            decompressed = None

    def testPickle(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises(TypeError):
                pickle.dumps(BZ2Decompressor(), proto)

    def testDecompressorChunksMaxsize(self):
        bzd = BZ2Decompressor()
        max_length = 100
        out = []

        # Feed some input
        len_ = len(self.BIG_DATA) - 64
        out.append(bzd.decompress(self.BIG_DATA[:len_],
                                  max_length=max_length))
        self.assertFalse(bzd.needs_input)
        self.assertEqual(len(out[-1]), max_length)

        # Retrieve more data without providing more input
        out.append(bzd.decompress(b'', max_length=max_length))
        self.assertFalse(bzd.needs_input)
        self.assertEqual(len(out[-1]), max_length)

        # Retrieve more data while providing more input
        out.append(bzd.decompress(self.BIG_DATA[len_:],
                                  max_length=max_length))
        self.assertLessEqual(len(out[-1]), max_length)

        # Retrieve remaining uncompressed data
        while not bzd.eof:
            out.append(bzd.decompress(b'', max_length=max_length))
            self.assertLessEqual(len(out[-1]), max_length)

        out = b"".join(out)
        self.assertEqual(out, self.BIG_TEXT)
        self.assertEqual(bzd.unused_data, b"")

    def test_decompressor_inputbuf_1(self):
        # Test reusing input buffer after moving existing
        # contents to beginning
        bzd = BZ2Decompressor()
        out = []

        # Create input buffer and fill it
        self.assertEqual(bzd.decompress(self.DATA[:100],
                                        max_length=0), b'')

        # Retrieve some results, freeing capacity at beginning
        # of input buffer
        out.append(bzd.decompress(b'', 2))

        # Add more data that fits into input buffer after
        # moving existing data to beginning
        out.append(bzd.decompress(self.DATA[100:105], 15))

        # Decompress rest of data
        out.append(bzd.decompress(self.DATA[105:]))
        self.assertEqual(b''.join(out), self.TEXT)

    def test_decompressor_inputbuf_2(self):
        # Test reusing input buffer by appending data at the
        # end right away
        bzd = BZ2Decompressor()
        out = []

        # Create input buffer and empty it
        self.assertEqual(bzd.decompress(self.DATA[:200],
                                        max_length=0), b'')
        out.append(bzd.decompress(b''))

        # Fill buffer with new data
        out.append(bzd.decompress(self.DATA[200:280], 2))

        # Append some more data, not enough to require resize
        out.append(bzd.decompress(self.DATA[280:300], 2))

        # Decompress rest of data
        out.append(bzd.decompress(self.DATA[300:]))
        self.assertEqual(b''.join(out), self.TEXT)

    def test_decompressor_inputbuf_3(self):
        # Test reusing input buffer after extending it

        bzd = BZ2Decompressor()
        out = []

        # Create almost full input buffer
        out.append(bzd.decompress(self.DATA[:200], 5))

        # Add even more data to it, requiring resize
        out.append(bzd.decompress(self.DATA[200:300], 5))

        # Decompress rest of data
        out.append(bzd.decompress(self.DATA[300:]))
        self.assertEqual(b''.join(out), self.TEXT)

    def test_failure(self):
        bzd = BZ2Decompressor()
        self.assertRaises(Exception, bzd.decompress, self.BAD_DATA * 30)
        # Previously, a second call could crash due to internal inconsistency
        self.assertRaises(Exception, bzd.decompress, self.BAD_DATA * 30)

    @support.refcount_test
    def test_refleaks_in___init__(self):
        gettotalrefcount = support.get_attribute(sys, 'gettotalrefcount')
        bzd = BZ2Decompressor()
        refs_before = gettotalrefcount()
        for i in range(100):
            bzd.__init__()
        self.assertAlmostEqual(gettotalrefcount() - refs_before, 0, delta=10)


class CompressDecompressTest(BaseTest):
    def testCompress(self):
        data = bz2.compress(self.TEXT)
        self.assertEqual(ext_decompress(data), self.TEXT)

    def testCompressEmptyString(self):
        text = bz2.compress(b'')
        self.assertEqual(text, self.EMPTY_DATA)

    def testDecompress(self):
        text = bz2.decompress(self.DATA)
        self.assertEqual(text, self.TEXT)

    def testDecompressEmpty(self):
        text = bz2.decompress(b"")
        self.assertEqual(text, b"")

    def testDecompressToEmptyString(self):
        text = bz2.decompress(self.EMPTY_DATA)
        self.assertEqual(text, b'')

    def testDecompressIncomplete(self):
        self.assertRaises(ValueError, bz2.decompress, self.DATA[:-10])

    def testDecompressBadData(self):
        self.assertRaises(OSError, bz2.decompress, self.BAD_DATA)

    def testDecompressMultiStream(self):
        text = bz2.decompress(self.DATA * 5)
        self.assertEqual(text, self.TEXT * 5)

    def testDecompressTrailingJunk(self):
        text = bz2.decompress(self.DATA + self.BAD_DATA)
        self.assertEqual(text, self.TEXT)

    def testDecompressMultiStreamTrailingJunk(self):
        text = bz2.decompress(self.DATA * 5 + self.BAD_DATA)
        self.assertEqual(text, self.TEXT * 5)


class OpenTest(BaseTest):
    "Test the open function."

    def open(self, *args, **kwargs):
        return bz2.open(*args, **kwargs)

    def test_binary_modes(self):
        for mode in ("wb", "xb"):
            if mode == "xb":
                unlink(self.filename)
            with self.open(self.filename, mode) as f:
                f.write(self.TEXT)
            with open(self.filename, "rb") as f:
                file_data = ext_decompress(f.read())
                self.assertEqual(file_data, self.TEXT)
            with self.open(self.filename, "rb") as f:
                self.assertEqual(f.read(), self.TEXT)
            with self.open(self.filename, "ab") as f:
                f.write(self.TEXT)
            with open(self.filename, "rb") as f:
                file_data = ext_decompress(f.read())
                self.assertEqual(file_data, self.TEXT * 2)

    def test_implicit_binary_modes(self):
        # Test implicit binary modes (no "b" or "t" in mode string).
        for mode in ("w", "x"):
            if mode == "x":
                unlink(self.filename)
            with self.open(self.filename, mode) as f:
                f.write(self.TEXT)
            with open(self.filename, "rb") as f:
                file_data = ext_decompress(f.read())
                self.assertEqual(file_data, self.TEXT)
            with self.open(self.filename, "r") as f:
                self.assertEqual(f.read(), self.TEXT)
            with self.open(self.filename, "a") as f:
                f.write(self.TEXT)
            with open(self.filename, "rb") as f:
                file_data = ext_decompress(f.read())
                self.assertEqual(file_data, self.TEXT * 2)

    def test_text_modes(self):
        text = self.TEXT.decode("ascii")
        text_native_eol = text.replace("\n", os.linesep)
        for mode in ("wt", "xt"):
            if mode == "xt":
                unlink(self.filename)
            with self.open(self.filename, mode) as f:
                f.write(text)
            with open(self.filename, "rb") as f:
                file_data = ext_decompress(f.read()).decode("ascii")
                self.assertEqual(file_data, text_native_eol)
            with self.open(self.filename, "rt") as f:
                self.assertEqual(f.read(), text)
            with self.open(self.filename, "at") as f:
                f.write(text)
            with open(self.filename, "rb") as f:
                file_data = ext_decompress(f.read()).decode("ascii")
                self.assertEqual(file_data, text_native_eol * 2)

    def test_x_mode(self):
        for mode in ("x", "xb", "xt"):
            unlink(self.filename)
            with self.open(self.filename, mode) as f:
                pass
            with self.assertRaises(FileExistsError):
                with self.open(self.filename, mode) as f:
                    pass

    def test_fileobj(self):
        with self.open(BytesIO(self.DATA), "r") as f:
            self.assertEqual(f.read(), self.TEXT)
        with self.open(BytesIO(self.DATA), "rb") as f:
            self.assertEqual(f.read(), self.TEXT)
        text = self.TEXT.decode("ascii")
        with self.open(BytesIO(self.DATA), "rt") as f:
            self.assertEqual(f.read(), text)

    def test_bad_params(self):
        # Test invalid parameter combinations.
        self.assertRaises(ValueError,
                          self.open, self.filename, "wbt")
        self.assertRaises(ValueError,
                          self.open, self.filename, "xbt")
        self.assertRaises(ValueError,
                          self.open, self.filename, "rb", encoding="utf-8")
        self.assertRaises(ValueError,
                          self.open, self.filename, "rb", errors="ignore")
        self.assertRaises(ValueError,
                          self.open, self.filename, "rb", newline="\n")

    def test_encoding(self):
        # Test non-default encoding.
        text = self.TEXT.decode("ascii")
        text_native_eol = text.replace("\n", os.linesep)
        with self.open(self.filename, "wt", encoding="utf-16-le") as f:
            f.write(text)
        with open(self.filename, "rb") as f:
            file_data = ext_decompress(f.read()).decode("utf-16-le")
            self.assertEqual(file_data, text_native_eol)
        with self.open(self.filename, "rt", encoding="utf-16-le") as f:
            self.assertEqual(f.read(), text)

    def test_encoding_error_handler(self):
        # Test with non-default encoding error handler.
        with self.open(self.filename, "wb") as f:
            f.write(b"foo\xffbar")
        with self.open(self.filename, "rt", encoding="ascii", errors="ignore") \
                as f:
            self.assertEqual(f.read(), "foobar")

    def test_newline(self):
        # Test with explicit newline (universal newline mode disabled).
        text = self.TEXT.decode("ascii")
        with self.open(self.filename, "wt", newline="\n") as f:
            f.write(text)
        with self.open(self.filename, "rt", newline="\r") as f:
            self.assertEqual(f.readlines(), [text])


def test_main():
    support.run_unittest(
        BZ2FileTest,
        BZ2CompressorTest,
        BZ2DecompressorTest,
        CompressDecompressTest,
        OpenTest,
    )
    support.reap_children()

if __name__ == '__main__':
    test_main()
