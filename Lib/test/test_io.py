"""Unit tests for io.py."""

import os
import sys
import time
import array
import unittest
from itertools import chain
from test import test_support

import io  # The module under test


class MockRawIO(io.RawIOBase):

    def __init__(self, read_stack=()):
        self._read_stack = list(read_stack)
        self._write_stack = []

    def read(self, n=None):
        try:
            return self._read_stack.pop(0)
        except:
            return b""

    def write(self, b):
        self._write_stack.append(b[:])
        return len(b)

    def writable(self):
        return True

    def fileno(self):
        return 42

    def readable(self):
        return True

    def seekable(self):
        return True

    def seek(self, pos, whence):
        pass

    def tell(self):
        return 42


class MockFileIO(io.BytesIO):

    def __init__(self, data):
        self.read_history = []
        io.BytesIO.__init__(self, data)

    def read(self, n=None):
        res = io.BytesIO.read(self, n)
        self.read_history.append(None if res is None else len(res))
        return res


class MockNonBlockWriterIO(io.RawIOBase):

    def __init__(self, blocking_script):
        self._blocking_script = list(blocking_script)
        self._write_stack = []

    def write(self, b):
        self._write_stack.append(b[:])
        n = self._blocking_script.pop(0)
        if (n < 0):
            raise io.BlockingIOError(0, "test blocking", -n)
        else:
            return n

    def writable(self):
        return True


class IOTest(unittest.TestCase):

    def tearDown(self):
        test_support.unlink(test_support.TESTFN)

    def write_ops(self, f):
        self.assertEqual(f.write(b"blah."), 5)
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.write(b"Hello."), 6)
        self.assertEqual(f.tell(), 6)
        self.assertEqual(f.seek(-1, 1), 5)
        self.assertEqual(f.tell(), 5)
        self.assertEqual(f.write(buffer(b" world\n\n\n")), 9)
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.write(b"h"), 1)
        self.assertEqual(f.seek(-1, 2), 13)
        self.assertEqual(f.tell(), 13)
        self.assertEqual(f.truncate(12), 12)
        self.assertEqual(f.tell(), 13)
        self.assertRaises(TypeError, f.seek, 0.0)

    def read_ops(self, f, buffered=False):
        data = f.read(5)
        self.assertEqual(data, b"hello")
        data = buffer(data)
        self.assertEqual(f.readinto(data), 5)
        self.assertEqual(data, b" worl")
        self.assertEqual(f.readinto(data), 2)
        self.assertEqual(len(data), 5)
        self.assertEqual(data[:2], b"d\n")
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.read(20), b"hello world\n")
        self.assertEqual(f.read(1), b"")
        self.assertEqual(f.readinto(buffer(b"x")), 0)
        self.assertEqual(f.seek(-6, 2), 6)
        self.assertEqual(f.read(5), b"world")
        self.assertEqual(f.read(0), b"")
        self.assertEqual(f.readinto(buffer()), 0)
        self.assertEqual(f.seek(-6, 1), 5)
        self.assertEqual(f.read(5), b" worl")
        self.assertEqual(f.tell(), 10)
        self.assertRaises(TypeError, f.seek, 0.0)
        if buffered:
            f.seek(0)
            self.assertEqual(f.read(), b"hello world\n")
            f.seek(6)
            self.assertEqual(f.read(), b"world\n")
            self.assertEqual(f.read(), b"")

    LARGE = 2**31

    def large_file_ops(self, f):
        assert f.readable()
        assert f.writable()
        self.assertEqual(f.seek(self.LARGE), self.LARGE)
        self.assertEqual(f.tell(), self.LARGE)
        self.assertEqual(f.write(b"xxx"), 3)
        self.assertEqual(f.tell(), self.LARGE + 3)
        self.assertEqual(f.seek(-1, 1), self.LARGE + 2)
        self.assertEqual(f.truncate(), self.LARGE + 2)
        self.assertEqual(f.tell(), self.LARGE + 2)
        self.assertEqual(f.seek(0, 2), self.LARGE + 2)
        self.assertEqual(f.truncate(self.LARGE + 1), self.LARGE + 1)
        self.assertEqual(f.tell(), self.LARGE + 2)
        self.assertEqual(f.seek(0, 2), self.LARGE + 1)
        self.assertEqual(f.seek(-1, 2), self.LARGE)
        self.assertEqual(f.read(2), b"x")

    def test_raw_file_io(self):
        f = io.open(test_support.TESTFN, "wb", buffering=0)
        self.assertEqual(f.readable(), False)
        self.assertEqual(f.writable(), True)
        self.assertEqual(f.seekable(), True)
        self.write_ops(f)
        f.close()
        f = io.open(test_support.TESTFN, "rb", buffering=0)
        self.assertEqual(f.readable(), True)
        self.assertEqual(f.writable(), False)
        self.assertEqual(f.seekable(), True)
        self.read_ops(f)
        f.close()

    def test_buffered_file_io(self):
        f = io.open(test_support.TESTFN, "wb")
        self.assertEqual(f.readable(), False)
        self.assertEqual(f.writable(), True)
        self.assertEqual(f.seekable(), True)
        self.write_ops(f)
        f.close()
        f = io.open(test_support.TESTFN, "rb")
        self.assertEqual(f.readable(), True)
        self.assertEqual(f.writable(), False)
        self.assertEqual(f.seekable(), True)
        self.read_ops(f, True)
        f.close()

    def test_readline(self):
        f = io.open(test_support.TESTFN, "wb")
        f.write(b"abc\ndef\nxyzzy\nfoo")
        f.close()
        f = io.open(test_support.TESTFN, "rb")
        self.assertEqual(f.readline(), b"abc\n")
        self.assertEqual(f.readline(10), b"def\n")
        self.assertEqual(f.readline(2), b"xy")
        self.assertEqual(f.readline(4), b"zzy\n")
        self.assertEqual(f.readline(), b"foo")
        f.close()

    def test_raw_bytes_io(self):
        f = io.BytesIO()
        self.write_ops(f)
        data = f.getvalue()
        self.assertEqual(data, b"hello world\n")
        f = io.BytesIO(data)
        self.read_ops(f, True)

    def test_large_file_ops(self):
        # On Windows and Mac OSX this test comsumes large resources; It takes
        # a long time to build the >2GB file and takes >2GB of disk space
        # therefore the resource must be enabled to run this test.
        if sys.platform[:3] == 'win' or sys.platform == 'darwin':
            if not test_support.is_resource_enabled("largefile"):
                print("\nTesting large file ops skipped on %s." % sys.platform,
                      file=sys.stderr)
                print("It requires %d bytes and a long time." % self.LARGE,
                      file=sys.stderr)
                print("Use 'regrtest.py -u largefile test_io' to run it.",
                      file=sys.stderr)
                return
        f = io.open(test_support.TESTFN, "w+b", 0)
        self.large_file_ops(f)
        f.close()
        f = io.open(test_support.TESTFN, "w+b")
        self.large_file_ops(f)
        f.close()

    def test_with_open(self):
        for bufsize in (0, 1, 100):
            f = None
            with open(test_support.TESTFN, "wb", bufsize) as f:
                f.write(b"xxx")
            self.assertEqual(f.closed, True)
            f = None
            try:
                with open(test_support.TESTFN, "wb", bufsize) as f:
                    1/0
            except ZeroDivisionError:
                self.assertEqual(f.closed, True)
            else:
                self.fail("1/0 didn't raise an exception")

    def test_destructor(self):
        record = []
        class MyFileIO(io.FileIO):
            def __del__(self):
                record.append(1)
                io.FileIO.__del__(self)
            def close(self):
                record.append(2)
                io.FileIO.close(self)
            def flush(self):
                record.append(3)
                io.FileIO.flush(self)
        f = MyFileIO(test_support.TESTFN, "w")
        f.write("xxx")
        del f
        self.assertEqual(record, [1, 2, 3])

    def test_close_flushes(self):
        f = io.open(test_support.TESTFN, "wb")
        f.write(b"xxx")
        f.close()
        f = io.open(test_support.TESTFN, "rb")
        self.assertEqual(f.read(), b"xxx")
        f.close()

    def test_array_writes(self):
        a = array.array('i', range(10))
        n = len(memoryview(a))
        f = io.open(test_support.TESTFN, "wb", 0)
        self.assertEqual(f.write(a), n)
        f.close()
        f = io.open(test_support.TESTFN, "wb")
        self.assertEqual(f.write(a), n)
        f.close()

    def test_closefd(self):
        self.assertRaises(ValueError, io.open, test_support.TESTFN, 'w',
                          closefd=False)

class MemorySeekTestMixin:

    def testInit(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

    def testRead(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

        self.assertEquals(buf[:1], bytesIo.read(1))
        self.assertEquals(buf[1:5], bytesIo.read(4))
        self.assertEquals(buf[5:], bytesIo.read(900))
        self.assertEquals(self.EOF, bytesIo.read())

    def testReadNoArgs(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

        self.assertEquals(buf, bytesIo.read())
        self.assertEquals(self.EOF, bytesIo.read())

    def testSeek(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

        bytesIo.read(5)
        bytesIo.seek(0)
        self.assertEquals(buf, bytesIo.read())

        bytesIo.seek(3)
        self.assertEquals(buf[3:], bytesIo.read())
        self.assertRaises(TypeError, bytesIo.seek, 0.0)

    def testTell(self):
        buf = self.buftype("1234567890")
        bytesIo = self.ioclass(buf)

        self.assertEquals(0, bytesIo.tell())
        bytesIo.seek(5)
        self.assertEquals(5, bytesIo.tell())
        bytesIo.seek(10000)
        self.assertEquals(10000, bytesIo.tell())


class BytesIOTest(MemorySeekTestMixin, unittest.TestCase):
    @staticmethod
    def buftype(s):
        return s.encode("utf-8")
    ioclass = io.BytesIO
    EOF = b""


class StringIOTest(MemorySeekTestMixin, unittest.TestCase):
    buftype = str
    ioclass = io.StringIO
    EOF = ""


class BufferedReaderTest(unittest.TestCase):

    def testRead(self):
        rawio = MockRawIO((b"abc", b"d", b"efg"))
        bufio = io.BufferedReader(rawio)

        self.assertEquals(b"abcdef", bufio.read(6))

    def testBuffering(self):
        data = b"abcdefghi"
        dlen = len(data)

        tests = [
            [ 100, [ 3, 1, 4, 8 ], [ dlen, 0 ] ],
            [ 100, [ 3, 3, 3],     [ dlen ]    ],
            [   4, [ 1, 2, 4, 2 ], [ 4, 4, 1 ] ],
        ]

        for bufsize, buf_read_sizes, raw_read_sizes in tests:
            rawio = MockFileIO(data)
            bufio = io.BufferedReader(rawio, buffer_size=bufsize)
            pos = 0
            for nbytes in buf_read_sizes:
                self.assertEquals(bufio.read(nbytes), data[pos:pos+nbytes])
                pos += nbytes
            self.assertEquals(rawio.read_history, raw_read_sizes)

    def testReadNonBlocking(self):
        # Inject some None's in there to simulate EWOULDBLOCK
        rawio = MockRawIO((b"abc", b"d", None, b"efg", None, None))
        bufio = io.BufferedReader(rawio)

        self.assertEquals(b"abcd", bufio.read(6))
        self.assertEquals(b"e", bufio.read(1))
        self.assertEquals(b"fg", bufio.read())
        self.assert_(None is bufio.read())
        self.assertEquals(b"", bufio.read())

    def testReadToEof(self):
        rawio = MockRawIO((b"abc", b"d", b"efg"))
        bufio = io.BufferedReader(rawio)

        self.assertEquals(b"abcdefg", bufio.read(9000))

    def testReadNoArgs(self):
        rawio = MockRawIO((b"abc", b"d", b"efg"))
        bufio = io.BufferedReader(rawio)

        self.assertEquals(b"abcdefg", bufio.read())

    def testFileno(self):
        rawio = MockRawIO((b"abc", b"d", b"efg"))
        bufio = io.BufferedReader(rawio)

        self.assertEquals(42, bufio.fileno())

    def testFilenoNoFileno(self):
        # XXX will we always have fileno() function? If so, kill
        # this test. Else, write it.
        pass


class BufferedWriterTest(unittest.TestCase):

    def testWrite(self):
        # Write to the buffered IO but don't overflow the buffer.
        writer = MockRawIO()
        bufio = io.BufferedWriter(writer, 8)

        bufio.write(b"abc")

        self.assertFalse(writer._write_stack)

    def testWriteOverflow(self):
        writer = MockRawIO()
        bufio = io.BufferedWriter(writer, 8)

        bufio.write(b"abc")
        bufio.write(b"defghijkl")

        self.assertEquals(b"abcdefghijkl", writer._write_stack[0])

    def testWriteNonBlocking(self):
        raw = MockNonBlockWriterIO((9, 2, 22, -6, 10, 12, 12))
        bufio = io.BufferedWriter(raw, 8, 16)

        bufio.write(b"asdf")
        bufio.write(b"asdfa")
        self.assertEquals(b"asdfasdfa", raw._write_stack[0])

        bufio.write(b"asdfasdfasdf")
        self.assertEquals(b"asdfasdfasdf", raw._write_stack[1])
        bufio.write(b"asdfasdfasdf")
        self.assertEquals(b"dfasdfasdf", raw._write_stack[2])
        self.assertEquals(b"asdfasdfasdf", raw._write_stack[3])

        bufio.write(b"asdfasdfasdf")

        # XXX I don't like this test. It relies too heavily on how the
        # algorithm actually works, which we might change. Refactor
        # later.

    def testFileno(self):
        rawio = MockRawIO((b"abc", b"d", b"efg"))
        bufio = io.BufferedWriter(rawio)

        self.assertEquals(42, bufio.fileno())

    def testFlush(self):
        writer = MockRawIO()
        bufio = io.BufferedWriter(writer, 8)

        bufio.write(b"abc")
        bufio.flush()

        self.assertEquals(b"abc", writer._write_stack[0])


class BufferedRWPairTest(unittest.TestCase):

    def testRWPair(self):
        r = MockRawIO(())
        w = MockRawIO()
        pair = io.BufferedRWPair(r, w)

        # XXX need implementation


class BufferedRandomTest(unittest.TestCase):

    def testReadAndWrite(self):
        raw = MockRawIO((b"asdf", b"ghjk"))
        rw = io.BufferedRandom(raw, 8, 12)

        self.assertEqual(b"as", rw.read(2))
        rw.write(b"ddd")
        rw.write(b"eee")
        self.assertFalse(raw._write_stack) # Buffer writes
        self.assertEqual(b"ghjk", rw.read()) # This read forces write flush
        self.assertEquals(b"dddeee", raw._write_stack[0])

    def testSeekAndTell(self):
        raw = io.BytesIO(b"asdfghjkl")
        rw = io.BufferedRandom(raw)

        self.assertEquals(b"as", rw.read(2))
        self.assertEquals(2, rw.tell())
        rw.seek(0, 0)
        self.assertEquals(b"asdf", rw.read(4))

        rw.write(b"asdf")
        rw.seek(0, 0)
        self.assertEquals(b"asdfasdfl", rw.read())
        self.assertEquals(9, rw.tell())
        rw.seek(-4, 2)
        self.assertEquals(5, rw.tell())
        rw.seek(2, 1)
        self.assertEquals(7, rw.tell())
        self.assertEquals(b"fl", rw.read(11))
        self.assertRaises(TypeError, rw.seek, 0.0)


class TextIOWrapperTest(unittest.TestCase):

    def setUp(self):
        self.testdata = b"AAA\r\nBBB\rCCC\r\nDDD\nEEE\r\n"
        self.normalized = b"AAA\nBBB\nCCC\nDDD\nEEE\n".decode("ascii")

    def tearDown(self):
        test_support.unlink(test_support.TESTFN)

    def testNewlinesInput(self):
        testdata = b"AAA\nBBB\nCCC\rDDD\rEEE\r\nFFF\r\nGGG"
        normalized = testdata.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
        for newline, expected in [
            (None, normalized.decode("ascii").splitlines(True)),
            ("", testdata.decode("ascii").splitlines(True)),
            ("\n", ["AAA\n", "BBB\n", "CCC\rDDD\rEEE\r\n", "FFF\r\n", "GGG"]),
            ("\r\n", ["AAA\nBBB\nCCC\rDDD\rEEE\r\n", "FFF\r\n", "GGG"]),
            ("\r",  ["AAA\nBBB\nCCC\r", "DDD\r", "EEE\r", "\nFFF\r", "\nGGG"]),
            ]:
            buf = io.BytesIO(testdata)
            txt = io.TextIOWrapper(buf, encoding="ascii", newline=newline)
            self.assertEquals(txt.readlines(), expected)
            txt.seek(0)
            self.assertEquals(txt.read(), "".join(expected))

    def testNewlinesOutput(self):
        testdict = {
            "": b"AAA\nBBB\nCCC\nX\rY\r\nZ",
            "\n": b"AAA\nBBB\nCCC\nX\rY\r\nZ",
            "\r": b"AAA\rBBB\rCCC\rX\rY\r\rZ",
            "\r\n": b"AAA\r\nBBB\r\nCCC\r\nX\rY\r\r\nZ",
            }
        tests = [(None, testdict[os.linesep])] + sorted(testdict.items())
        for newline, expected in tests:
            buf = io.BytesIO()
            txt = io.TextIOWrapper(buf, encoding="ascii", newline=newline)
            txt.write("AAA\nB")
            txt.write("BB\nCCC\n")
            txt.write("X\rY\r\nZ")
            txt.flush()
            self.assertEquals(buf.getvalue(), expected)

    def testNewlines(self):
        input_lines = [ "unix\n", "windows\r\n", "os9\r", "last\n", "nonl" ]

        tests = [
            [ None, [ 'unix\n', 'windows\n', 'os9\n', 'last\n', 'nonl' ] ],
            [ '', input_lines ],
            [ '\n', [ "unix\n", "windows\r\n", "os9\rlast\n", "nonl" ] ],
            [ '\r\n', [ "unix\nwindows\r\n", "os9\rlast\nnonl" ] ],
            [ '\r', [ "unix\nwindows\r", "\nos9\r", "last\nnonl" ] ],
        ]

        encodings = ('utf-8', 'latin-1')

        # Try a range of buffer sizes to test the case where \r is the last
        # character in TextIOWrapper._pending_line.
        for encoding in encodings:
            # XXX: str.encode() should return bytes
            data = bytes(''.join(input_lines).encode(encoding))
            for do_reads in (False, True):
                for bufsize in range(1, 10):
                    for newline, exp_lines in tests:
                        bufio = io.BufferedReader(io.BytesIO(data), bufsize)
                        textio = io.TextIOWrapper(bufio, newline=newline,
                                                  encoding=encoding)
                        if do_reads:
                            got_lines = []
                            while True:
                                c2 = textio.read(2)
                                if c2 == '':
                                    break
                                self.assertEquals(len(c2), 2)
                                got_lines.append(c2 + textio.readline())
                        else:
                            got_lines = list(textio)

                        for got_line, exp_line in zip(got_lines, exp_lines):
                            self.assertEquals(got_line, exp_line)
                        self.assertEquals(len(got_lines), len(exp_lines))

    def testNewlinesInput(self):
        testdata = b"AAA\nBBB\nCCC\rDDD\rEEE\r\nFFF\r\nGGG"
        normalized = testdata.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
        for newline, expected in [
            (None, normalized.decode("ascii").splitlines(True)),
            ("", testdata.decode("ascii").splitlines(True)),
            ("\n", ["AAA\n", "BBB\n", "CCC\rDDD\rEEE\r\n", "FFF\r\n", "GGG"]),
            ("\r\n", ["AAA\nBBB\nCCC\rDDD\rEEE\r\n", "FFF\r\n", "GGG"]),
            ("\r",  ["AAA\nBBB\nCCC\r", "DDD\r", "EEE\r", "\nFFF\r", "\nGGG"]),
            ]:
            buf = io.BytesIO(testdata)
            txt = io.TextIOWrapper(buf, encoding="ascii", newline=newline)
            self.assertEquals(txt.readlines(), expected)
            txt.seek(0)
            self.assertEquals(txt.read(), "".join(expected))

    def testNewlinesOutput(self):
        data = "AAA\nBBB\rCCC\n"
        data_lf = b"AAA\nBBB\rCCC\n"
        data_cr = b"AAA\rBBB\rCCC\r"
        data_crlf = b"AAA\r\nBBB\rCCC\r\n"
        save_linesep = os.linesep
        try:
            for os.linesep, newline, expected in [
                ("\n", None, data_lf),
                ("\r\n", None, data_crlf),
                ("\n", "", data_lf),
                ("\r\n", "", data_lf),
                ("\n", "\n", data_lf),
                ("\r\n", "\n", data_lf),
                ("\n", "\r", data_cr),
                ("\r\n", "\r", data_cr),
                ("\n", "\r\n", data_crlf),
                ("\r\n", "\r\n", data_crlf),
                ]:
                buf = io.BytesIO()
                txt = io.TextIOWrapper(buf, encoding="ascii", newline=newline)
                txt.write(data)
                txt.close()
                self.assertEquals(buf.getvalue(), expected)
        finally:
            os.linesep = save_linesep

    # Systematic tests of the text I/O API

    def testBasicIO(self):
        for chunksize in (1, 2, 3, 4, 5, 15, 16, 17, 31, 32, 33, 63, 64, 65):
            for enc in "ascii", "latin1", "utf8" :# , "utf-16-be", "utf-16-le":
                f = io.open(test_support.TESTFN, "w+", encoding=enc)
                f._CHUNK_SIZE = chunksize
                self.assertEquals(f.write("abc"), 3)
                f.close()
                f = io.open(test_support.TESTFN, "r+", encoding=enc)
                f._CHUNK_SIZE = chunksize
                self.assertEquals(f.tell(), 0)
                self.assertEquals(f.read(), "abc")
                cookie = f.tell()
                self.assertEquals(f.seek(0), 0)
                self.assertEquals(f.read(2), "ab")
                self.assertEquals(f.read(1), "c")
                self.assertEquals(f.read(1), "")
                self.assertEquals(f.read(), "")
                self.assertEquals(f.tell(), cookie)
                self.assertEquals(f.seek(0), 0)
                self.assertEquals(f.seek(0, 2), cookie)
                self.assertEquals(f.write("def"), 3)
                self.assertEquals(f.seek(cookie), cookie)
                self.assertEquals(f.read(), "def")
                if enc.startswith("utf"):
                    self.multi_line_test(f, enc)
                f.close()

    def multi_line_test(self, f, enc):
        f.seek(0)
        f.truncate()
        sample = "s\xff\u0fff\uffff"
        wlines = []
        for size in (0, 1, 2, 3, 4, 5, 30, 31, 32, 33, 62, 63, 64, 65, 1000):
            chars = []
            for i in range(size):
                chars.append(sample[i % len(sample)])
            line = "".join(chars) + "\n"
            wlines.append((f.tell(), line))
            f.write(line)
        f.seek(0)
        rlines = []
        while True:
            pos = f.tell()
            line = f.readline()
            if not line:
                break
            rlines.append((pos, line))
        self.assertEquals(rlines, wlines)

    def testTelling(self):
        f = io.open(test_support.TESTFN, "w+", encoding="utf8")
        p0 = f.tell()
        f.write("\xff\n")
        p1 = f.tell()
        f.write("\xff\n")
        p2 = f.tell()
        f.seek(0)
        self.assertEquals(f.tell(), p0)
        self.assertEquals(f.readline(), "\xff\n")
        self.assertEquals(f.tell(), p1)
        self.assertEquals(f.readline(), "\xff\n")
        self.assertEquals(f.tell(), p2)
        f.seek(0)
        for line in f:
            self.assertEquals(line, "\xff\n")
            self.assertRaises(IOError, f.tell)
        self.assertEquals(f.tell(), p2)
        f.close()

    def testSeeking(self):
        chunk_size = io.TextIOWrapper._CHUNK_SIZE
        prefix_size = chunk_size - 2
        u_prefix = "a" * prefix_size
        prefix = bytes(u_prefix.encode("utf-8"))
        self.assertEquals(len(u_prefix), len(prefix))
        u_suffix = "\u8888\n"
        suffix = bytes(u_suffix.encode("utf-8"))
        line = prefix + suffix
        f = io.open(test_support.TESTFN, "wb")
        f.write(line*2)
        f.close()
        f = io.open(test_support.TESTFN, "r", encoding="utf-8")
        s = f.read(prefix_size)
        self.assertEquals(s, str(prefix, "ascii"))
        self.assertEquals(f.tell(), prefix_size)
        self.assertEquals(f.readline(), u_suffix)

    def testSeekingToo(self):
        # Regression test for a specific bug
        data = b'\xe0\xbf\xbf\n'
        f = io.open(test_support.TESTFN, "wb")
        f.write(data)
        f.close()
        f = io.open(test_support.TESTFN, "r", encoding="utf-8")
        f._CHUNK_SIZE  # Just test that it exists
        f._CHUNK_SIZE = 2
        f.readline()
        f.tell()

    def timingTest(self):
        timer = time.time
        enc = "utf8"
        line = "\0\x0f\xff\u0fff\uffff\U000fffff\U0010ffff"*3 + "\n"
        nlines = 10000
        nchars = len(line)
        nbytes = len(line.encode(enc))
        for chunk_size in (32, 64, 128, 256):
            f = io.open(test_support.TESTFN, "w+", encoding=enc)
            f._CHUNK_SIZE = chunk_size
            t0 = timer()
            for i in range(nlines):
                f.write(line)
            f.flush()
            t1 = timer()
            f.seek(0)
            for line in f:
                pass
            t2 = timer()
            f.seek(0)
            while f.readline():
                pass
            t3 = timer()
            f.seek(0)
            while f.readline():
                f.tell()
            t4 = timer()
            f.close()
            if test_support.verbose:
                print("\nTiming test: %d lines of %d characters (%d bytes)" %
                      (nlines, nchars, nbytes))
                print("File chunk size:          %6s" % f._CHUNK_SIZE)
                print("Writing:                  %6.3f seconds" % (t1-t0))
                print("Reading using iteration:  %6.3f seconds" % (t2-t1))
                print("Reading using readline(): %6.3f seconds" % (t3-t2))
                print("Using readline()+tell():  %6.3f seconds" % (t4-t3))

    def testReadOneByOne(self):
        txt = io.TextIOWrapper(io.BytesIO(b"AA\r\nBB"))
        reads = ""
        while True:
            c = txt.read(1)
            if not c:
                break
            reads += c
        self.assertEquals(reads, "AA\nBB")

    # read in amounts equal to TextIOWrapper._CHUNK_SIZE which is 128.
    def testReadByChunk(self):
        # make sure "\r\n" straddles 128 char boundary.
        txt = io.TextIOWrapper(io.BytesIO(b"A" * 127 + b"\r\nB"))
        reads = ""
        while True:
            c = txt.read(128)
            if not c:
                break
            reads += c
        self.assertEquals(reads, "A"*127+"\nB")

    def test_issue1395_1(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ascii")

        # read one char at a time
        reads = ""
        while True:
            c = txt.read(1)
            if not c:
                break
            reads += c
        self.assertEquals(reads, self.normalized)

    def test_issue1395_2(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = ""
        while True:
            c = txt.read(4)
            if not c:
                break
            reads += c
        self.assertEquals(reads, self.normalized)

    def test_issue1395_3(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read(4)
        reads += txt.readline()
        reads += txt.readline()
        reads += txt.readline()
        self.assertEquals(reads, self.normalized)

    def test_issue1395_4(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read()
        self.assertEquals(reads, self.normalized)

    def test_issue1395_5(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        pos = txt.tell()
        txt.seek(0)
        txt.seek(pos)
        self.assertEquals(txt.read(4), "BBB\n")

    def test_newline_decoder(self):
        import codecs
        decoder = codecs.getincrementaldecoder("utf-8")()
        decoder = io.IncrementalNewlineDecoder(decoder, translate=True)

        self.assertEquals(decoder.decode(b'\xe8\xa2\x88'), "\u8888")

        self.assertEquals(decoder.decode(b'\xe8'), "")
        self.assertEquals(decoder.decode(b'\xa2'), "")
        self.assertEquals(decoder.decode(b'\x88'), "\u8888")

        self.assertEquals(decoder.decode(b'\xe8'), "")
        self.assertRaises(UnicodeDecodeError, decoder.decode, b'', final=True)

        decoder.setstate((b'', 0))
        self.assertEquals(decoder.decode(b'\n'), "\n")
        self.assertEquals(decoder.decode(b'\r'), "")
        self.assertEquals(decoder.decode(b'', final=True), "\n")
        self.assertEquals(decoder.decode(b'\r', final=True), "\n")

        self.assertEquals(decoder.decode(b'\r'), "")
        self.assertEquals(decoder.decode(b'a'), "\na")

        self.assertEquals(decoder.decode(b'\r\r\n'), "\n\n")
        self.assertEquals(decoder.decode(b'\r'), "")
        self.assertEquals(decoder.decode(b'\r'), "\n")
        self.assertEquals(decoder.decode(b'\na'), "\na")

        self.assertEquals(decoder.decode(b'\xe8\xa2\x88\r\n'), "\u8888\n")
        self.assertEquals(decoder.decode(b'\xe8\xa2\x88'), "\u8888")
        self.assertEquals(decoder.decode(b'\n'), "\n")
        self.assertEquals(decoder.decode(b'\xe8\xa2\x88\r'), "\u8888")
        self.assertEquals(decoder.decode(b'\n'), "\n")

# XXX Tests for open()

class MiscIOTest(unittest.TestCase):

    def testImport__all__(self):
        for name in io.__all__:
            obj = getattr(io, name, None)
            self.assert_(obj is not None, name)
            if name == "open":
                continue
            elif "error" in name.lower():
                self.assert_(issubclass(obj, Exception), name)
            else:
                self.assert_(issubclass(obj, io.IOBase))


def test_main():
    test_support.run_unittest(IOTest, BytesIOTest, StringIOTest,
                              BufferedReaderTest,
                              BufferedWriterTest, BufferedRWPairTest,
                              BufferedRandomTest, TextIOWrapperTest,
                              MiscIOTest)

if __name__ == "__main__":
    unittest.main()
