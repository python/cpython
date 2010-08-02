"""Unit tests for io.py."""
from __future__ import print_function
from __future__ import unicode_literals

import os
import sys
import time
import array
import threading
import random
import unittest
from itertools import cycle, count
from test import test_support

import codecs
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
        f.truncate(0)
        self.assertEqual(f.tell(), 5)
        f.seek(0)

        self.assertEqual(f.write(b"blah."), 5)
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.write(b"Hello."), 6)
        self.assertEqual(f.tell(), 6)
        self.assertEqual(f.seek(-1, 1), 5)
        self.assertEqual(f.tell(), 5)
        self.assertEqual(f.write(bytearray(b" world\n\n\n")), 9)
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.write(b"h"), 1)
        self.assertEqual(f.seek(-1, 2), 13)
        self.assertEqual(f.tell(), 13)

        self.assertEqual(f.truncate(12), 12)
        self.assertEqual(f.tell(), 13)
        self.assertEqual(f.write(b"hij"), 3)
        self.assertEqual(f.seek(0,1), 16)
        self.assertEqual(f.tell(), 16)
        self.assertEqual(f.truncate(12), 12)
        self.assertRaises(TypeError, f.seek, 0.0)

    def read_ops(self, f, buffered=False):
        data = f.read(5)
        self.assertEqual(data, b"hello")
        data = bytearray(data)
        self.assertEqual(f.readinto(data), 5)
        self.assertEqual(data, b" worl")
        self.assertEqual(f.readinto(data), 2)
        self.assertEqual(len(data), 5)
        self.assertEqual(data[:2], b"d\n")
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.read(20), b"hello world\n")
        self.assertEqual(f.read(1), b"")
        self.assertEqual(f.readinto(bytearray(b"x")), 0)
        self.assertEqual(f.seek(-6, 2), 6)
        self.assertEqual(f.read(5), b"world")
        self.assertEqual(f.read(0), b"")
        self.assertEqual(f.readinto(bytearray()), 0)
        self.assertEqual(f.seek(-6, 1), 5)
        self.assertEqual(f.read(5), b" worl")
        self.assertEqual(f.tell(), 10)
        f.seek(0)
        f.read(2)
        f.seek(0, 1)
        self.assertEqual(f.tell(), 2)
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
        f = io.open(test_support.TESTFN, "r+b")
        self.assertEqual(f.readable(), True)
        self.assertEqual(f.writable(), True)
        self.assertEqual(f.seekable(), True)
        self.write_ops(f)
        f.seek(0)
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
        if sys.platform[:3] in ('win', 'os2') or sys.platform == 'darwin':
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
                    1 // 0
            except ZeroDivisionError:
                self.assertEqual(f.closed, True)
            else:
                self.fail("1 // 0 didn't raise an exception")

    # issue 5008
    def test_append_mode_tell(self):
        with io.open(test_support.TESTFN, "wb") as f:
            f.write(b"xxx")
        with io.open(test_support.TESTFN, "ab", buffering=0) as f:
            self.assertEqual(f.tell(), 3)
        with io.open(test_support.TESTFN, "ab") as f:
            self.assertEqual(f.tell(), 3)
        with io.open(test_support.TESTFN, "a") as f:
            self.assert_(f.tell() > 0)

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

    def XXXtest_array_writes(self):
        # XXX memory view not available yet
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

    def testReadClosed(self):
        with io.open(test_support.TESTFN, "w") as f:
            f.write("egg\n")
        with io.open(test_support.TESTFN, "r") as f:
            file = io.open(f.fileno(), "r", closefd=False)
            self.assertEqual(file.read(), "egg\n")
            file.seek(0)
            file.close()
            self.assertRaises(ValueError, file.read)

    def test_no_closefd_with_filename(self):
        # can't use closefd in combination with a file name
        self.assertRaises(ValueError,
                          io.open, test_support.TESTFN, "r", closefd=False)

    def test_closefd_attr(self):
        with io.open(test_support.TESTFN, "wb") as f:
            f.write(b"egg\n")
        with io.open(test_support.TESTFN, "r") as f:
            self.assertEqual(f.buffer.raw.closefd, True)
            file = io.open(f.fileno(), "r", closefd=False)
            self.assertEqual(file.buffer.raw.closefd, False)

    def test_flush_error_on_close(self):
        f = io.open(test_support.TESTFN, "wb", buffering=0)
        def bad_flush():
            raise IOError()
        f.flush = bad_flush
        self.assertRaises(IOError, f.close) # exception not swallowed

    def test_multi_close(self):
        f = io.open(test_support.TESTFN, "wb", buffering=0)
        f.close()
        f.close()
        f.close()
        self.assertRaises(ValueError, f.flush)


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

    def testThreads(self):
        try:
            # Write out many bytes with exactly the same number of 0's,
            # 1's... 255's. This will help us check that concurrent reading
            # doesn't duplicate or forget contents.
            N = 1000
            l = range(256) * N
            random.shuffle(l)
            s = bytes(bytearray(l))
            with io.open(test_support.TESTFN, "wb") as f:
                f.write(s)
            with io.open(test_support.TESTFN, "rb", buffering=0) as raw:
                bufio = io.BufferedReader(raw, 8)
                errors = []
                results = []
                def f():
                    try:
                        # Intra-buffer read then buffer-flushing read
                        for n in cycle([1, 19]):
                            s = bufio.read(n)
                            if not s:
                                break
                            # list.append() is atomic
                            results.append(s)
                    except Exception as e:
                        errors.append(e)
                        raise
                threads = [threading.Thread(target=f) for x in range(20)]
                for t in threads:
                    t.start()
                time.sleep(0.02) # yield
                for t in threads:
                    t.join()
                self.assertFalse(errors,
                    "the following exceptions were caught: %r" % errors)
                s = b''.join(results)
                for i in range(256):
                    c = bytes(bytearray([i]))
                    self.assertEqual(s.count(c), N)
        finally:
            test_support.unlink(test_support.TESTFN)



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
        raw = MockNonBlockWriterIO((9, 2, 10, -6, 10, 8, 12))
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

    def testThreads(self):
        # BufferedWriter should not raise exceptions or crash
        # when called from multiple threads.
        try:
            # We use a real file object because it allows us to
            # exercise situations where the GIL is released before
            # writing the buffer to the raw streams. This is in addition
            # to concurrency issues due to switching threads in the middle
            # of Python code.
            with io.open(test_support.TESTFN, "wb", buffering=0) as raw:
                bufio = io.BufferedWriter(raw, 8)
                errors = []
                def f():
                    try:
                        # Write enough bytes to flush the buffer
                        s = b"a" * 19
                        for i in range(50):
                            bufio.write(s)
                    except Exception as e:
                        errors.append(e)
                        raise
                threads = [threading.Thread(target=f) for x in range(20)]
                for t in threads:
                    t.start()
                time.sleep(0.02) # yield
                for t in threads:
                    t.join()
                self.assertFalse(errors,
                    "the following exceptions were caught: %r" % errors)
        finally:
            test_support.unlink(test_support.TESTFN)

    def test_flush_error_on_close(self):
        raw = MockRawIO()
        def bad_flush():
            raise IOError()
        raw.flush = bad_flush
        b = io.BufferedWriter(raw)
        self.assertRaises(IOError, b.close) # exception not swallowed

    def test_multi_close(self):
        raw = MockRawIO()
        b = io.BufferedWriter(raw)
        b.close()
        b.close()
        b.close()
        self.assertRaises(ValueError, b.flush)


class BufferedRWPairTest(unittest.TestCase):

    def testRWPair(self):
        r = MockRawIO(())
        w = MockRawIO()
        pair = io.BufferedRWPair(r, w)
        self.assertFalse(pair.closed)

        # XXX More Tests


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

# To fully exercise seek/tell, the StatefulIncrementalDecoder has these
# properties:
#   - A single output character can correspond to many bytes of input.
#   - The number of input bytes to complete the character can be
#     undetermined until the last input byte is received.
#   - The number of input bytes can vary depending on previous input.
#   - A single input byte can correspond to many characters of output.
#   - The number of output characters can be undetermined until the
#     last input byte is received.
#   - The number of output characters can vary depending on previous input.

class StatefulIncrementalDecoder(codecs.IncrementalDecoder):
    """
    For testing seek/tell behavior with a stateful, buffering decoder.

    Input is a sequence of words.  Words may be fixed-length (length set
    by input) or variable-length (period-terminated).  In variable-length
    mode, extra periods are ignored.  Possible words are:
      - 'i' followed by a number sets the input length, I (maximum 99).
        When I is set to 0, words are space-terminated.
      - 'o' followed by a number sets the output length, O (maximum 99).
      - Any other word is converted into a word followed by a period on
        the output.  The output word consists of the input word truncated
        or padded out with hyphens to make its length equal to O.  If O
        is 0, the word is output verbatim without truncating or padding.
    I and O are initially set to 1.  When I changes, any buffered input is
    re-scanned according to the new I.  EOF also terminates the last word.
    """

    def __init__(self, errors='strict'):
        codecs.IncrementalDecoder.__init__(self, errors)
        self.reset()

    def __repr__(self):
        return '<SID %x>' % id(self)

    def reset(self):
        self.i = 1
        self.o = 1
        self.buffer = bytearray()

    def getstate(self):
        i, o = self.i ^ 1, self.o ^ 1 # so that flags = 0 after reset()
        return bytes(self.buffer), i*100 + o

    def setstate(self, state):
        buffer, io = state
        self.buffer = bytearray(buffer)
        i, o = divmod(io, 100)
        self.i, self.o = i ^ 1, o ^ 1

    def decode(self, input, final=False):
        output = ''
        for b in input:
            if self.i == 0: # variable-length, terminated with period
                if b == '.':
                    if self.buffer:
                        output += self.process_word()
                else:
                    self.buffer.append(b)
            else: # fixed-length, terminate after self.i bytes
                self.buffer.append(b)
                if len(self.buffer) == self.i:
                    output += self.process_word()
        if final and self.buffer: # EOF terminates the last word
            output += self.process_word()
        return output

    def process_word(self):
        output = ''
        if self.buffer[0] == ord('i'):
            self.i = min(99, int(self.buffer[1:] or 0)) # set input length
        elif self.buffer[0] == ord('o'):
            self.o = min(99, int(self.buffer[1:] or 0)) # set output length
        else:
            output = self.buffer.decode('ascii')
            if len(output) < self.o:
                output += '-'*self.o # pad out with hyphens
            if self.o:
                output = output[:self.o] # truncate to output length
            output += '.'
        self.buffer = bytearray()
        return output

    codecEnabled = False

    @classmethod
    def lookupTestDecoder(cls, name):
        if cls.codecEnabled and name == 'test_decoder':
            latin1 = codecs.lookup('latin-1')
            return codecs.CodecInfo(
                name='test_decoder', encode=latin1.encode, decode=None,
                incrementalencoder=None,
                streamreader=None, streamwriter=None,
                incrementaldecoder=cls)

# Register the previous decoder for testing.
# Disabled by default, tests will enable it.
codecs.register(StatefulIncrementalDecoder.lookupTestDecoder)


class StatefulIncrementalDecoderTest(unittest.TestCase):
    """
    Make sure the StatefulIncrementalDecoder actually works.
    """

    test_cases = [
        # I=1, O=1 (fixed-length input == fixed-length output)
        (b'abcd', False, 'a.b.c.d.'),
        # I=0, O=0 (variable-length input, variable-length output)
        (b'oiabcd', True, 'abcd.'),
        # I=0, O=0 (should ignore extra periods)
        (b'oi...abcd...', True, 'abcd.'),
        # I=0, O=6 (variable-length input, fixed-length output)
        (b'i.o6.x.xyz.toolongtofit.', False, 'x-----.xyz---.toolon.'),
        # I=2, O=6 (fixed-length input < fixed-length output)
        (b'i.i2.o6xyz', True, 'xy----.z-----.'),
        # I=6, O=3 (fixed-length input > fixed-length output)
        (b'i.o3.i6.abcdefghijklmnop', True, 'abc.ghi.mno.'),
        # I=0, then 3; O=29, then 15 (with longer output)
        (b'i.o29.a.b.cde.o15.abcdefghijabcdefghij.i3.a.b.c.d.ei00k.l.m', True,
         'a----------------------------.' +
         'b----------------------------.' +
         'cde--------------------------.' +
         'abcdefghijabcde.' +
         'a.b------------.' +
         '.c.------------.' +
         'd.e------------.' +
         'k--------------.' +
         'l--------------.' +
         'm--------------.')
    ]

    def testDecoder(self):
        # Try a few one-shot test cases.
        for input, eof, output in self.test_cases:
            d = StatefulIncrementalDecoder()
            self.assertEquals(d.decode(input, eof), output)

        # Also test an unfinished decode, followed by forcing EOF.
        d = StatefulIncrementalDecoder()
        self.assertEquals(d.decode(b'oiabcd'), '')
        self.assertEquals(d.decode(b'', 1), 'abcd.')

    def test_append_bom(self):
        # The BOM is not written again when appending to a non-empty file
        filename = test_support.TESTFN
        for charset in ('utf-8-sig', 'utf-16', 'utf-32'):
            with io.open(filename, 'w', encoding=charset) as f:
                f.write('aaa')
                pos = f.tell()
            with io.open(filename, 'rb') as f:
                self.assertEquals(f.read(), 'aaa'.encode(charset))

            with io.open(filename, 'a', encoding=charset) as f:
                f.write('xxx')
            with io.open(filename, 'rb') as f:
                self.assertEquals(f.read(), 'aaaxxx'.encode(charset))

    def test_seek_bom(self):
        # Same test, but when seeking manually
        filename = test_support.TESTFN
        for charset in ('utf-8-sig', 'utf-16', 'utf-32'):
            with io.open(filename, 'w', encoding=charset) as f:
                f.write('aaa')
                pos = f.tell()
            with io.open(filename, 'r+', encoding=charset) as f:
                f.seek(pos)
                f.write('zzz')
                f.seek(0)
                f.write('bbb')
            with io.open(filename, 'rb') as f:
                self.assertEquals(f.read(), 'bbbzzz'.encode(charset))


class TextIOWrapperTest(unittest.TestCase):

    def setUp(self):
        self.testdata = b"AAA\r\nBBB\rCCC\r\nDDD\nEEE\r\n"
        self.normalized = b"AAA\nBBB\nCCC\nDDD\nEEE\n".decode("ascii")

    def tearDown(self):
        test_support.unlink(test_support.TESTFN)

    def testLineBuffering(self):
        r = io.BytesIO()
        b = io.BufferedWriter(r, 1000)
        t = io.TextIOWrapper(b, newline="\n", line_buffering=True)
        t.write(u"X")
        self.assertEquals(r.getvalue(), b"")  # No flush happened
        t.write(u"Y\nZ")
        self.assertEquals(r.getvalue(), b"XY\nZ")  # All got flushed
        t.write(u"A\rB")
        self.assertEquals(r.getvalue(), b"XY\nZA\rB")

    def testEncodingErrorsReading(self):
        # (1) default
        b = io.BytesIO(b"abc\n\xff\n")
        t = io.TextIOWrapper(b, encoding="ascii")
        self.assertRaises(UnicodeError, t.read)
        # (2) explicit strict
        b = io.BytesIO(b"abc\n\xff\n")
        t = io.TextIOWrapper(b, encoding="ascii", errors="strict")
        self.assertRaises(UnicodeError, t.read)
        # (3) ignore
        b = io.BytesIO(b"abc\n\xff\n")
        t = io.TextIOWrapper(b, encoding="ascii", errors="ignore")
        self.assertEquals(t.read(), "abc\n\n")
        # (4) replace
        b = io.BytesIO(b"abc\n\xff\n")
        t = io.TextIOWrapper(b, encoding="ascii", errors="replace")
        self.assertEquals(t.read(), u"abc\n\ufffd\n")

    def testEncodingErrorsWriting(self):
        # (1) default
        b = io.BytesIO()
        t = io.TextIOWrapper(b, encoding="ascii")
        self.assertRaises(UnicodeError, t.write, u"\xff")
        # (2) explicit strict
        b = io.BytesIO()
        t = io.TextIOWrapper(b, encoding="ascii", errors="strict")
        self.assertRaises(UnicodeError, t.write, u"\xff")
        # (3) ignore
        b = io.BytesIO()
        t = io.TextIOWrapper(b, encoding="ascii", errors="ignore",
                             newline="\n")
        t.write(u"abc\xffdef\n")
        t.flush()
        self.assertEquals(b.getvalue(), b"abcdef\n")
        # (4) replace
        b = io.BytesIO()
        t = io.TextIOWrapper(b, encoding="ascii", errors="replace",
                             newline="\n")
        t.write(u"abc\xffdef\n")
        t.flush()
        self.assertEquals(b.getvalue(), b"abc?def\n")

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
            self.assertEquals(buf.closed, False)
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
        encodings = (
            'utf-8', 'latin-1',
            'utf-16', 'utf-16-le', 'utf-16-be',
            'utf-32', 'utf-32-le', 'utf-32-be',
        )

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
        data = u"AAA\nBBB\rCCC\n"
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
                self.assertEquals(buf.closed, True)
                self.assertRaises(ValueError, buf.getvalue)
        finally:
            os.linesep = save_linesep

    # Systematic tests of the text I/O API

    def testBasicIO(self):
        for chunksize in (1, 2, 3, 4, 5, 15, 16, 17, 31, 32, 33, 63, 64, 65):
            for enc in "ascii", "latin1", "utf8" :# , "utf-16-be", "utf-16-le":
                f = io.open(test_support.TESTFN, "w+", encoding=enc)
                f._CHUNK_SIZE = chunksize
                self.assertEquals(f.write(u"abc"), 3)
                f.close()
                f = io.open(test_support.TESTFN, "r+", encoding=enc)
                f._CHUNK_SIZE = chunksize
                self.assertEquals(f.tell(), 0)
                self.assertEquals(f.read(), u"abc")
                cookie = f.tell()
                self.assertEquals(f.seek(0), 0)
                self.assertEquals(f.read(2), u"ab")
                self.assertEquals(f.read(1), u"c")
                self.assertEquals(f.read(1), u"")
                self.assertEquals(f.read(), u"")
                self.assertEquals(f.tell(), cookie)
                self.assertEquals(f.seek(0), 0)
                self.assertEquals(f.seek(0, 2), cookie)
                self.assertEquals(f.write(u"def"), 3)
                self.assertEquals(f.seek(cookie), cookie)
                self.assertEquals(f.read(), u"def")
                if enc.startswith("utf"):
                    self.multi_line_test(f, enc)
                f.close()

    def multi_line_test(self, f, enc):
        f.seek(0)
        f.truncate()
        sample = u"s\xff\u0fff\uffff"
        wlines = []
        for size in (0, 1, 2, 3, 4, 5, 30, 31, 32, 33, 62, 63, 64, 65, 1000):
            chars = []
            for i in range(size):
                chars.append(sample[i % len(sample)])
            line = u"".join(chars) + u"\n"
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
        f.write(u"\xff\n")
        p1 = f.tell()
        f.write(u"\xff\n")
        p2 = f.tell()
        f.seek(0)
        self.assertEquals(f.tell(), p0)
        self.assertEquals(f.readline(), u"\xff\n")
        self.assertEquals(f.tell(), p1)
        self.assertEquals(f.readline(), u"\xff\n")
        self.assertEquals(f.tell(), p2)
        f.seek(0)
        for line in f:
            self.assertEquals(line, u"\xff\n")
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
        self.assertEquals(s, unicode(prefix, "ascii"))
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

    def testSeekAndTell(self):
        """Test seek/tell using the StatefulIncrementalDecoder."""

        def testSeekAndTellWithData(data, min_pos=0):
            """Tell/seek to various points within a data stream and ensure
            that the decoded data returned by read() is consistent."""
            f = io.open(test_support.TESTFN, 'wb')
            f.write(data)
            f.close()
            f = io.open(test_support.TESTFN, encoding='test_decoder')
            decoded = f.read()
            f.close()

            for i in range(min_pos, len(decoded) + 1): # seek positions
                for j in [1, 5, len(decoded) - i]: # read lengths
                    f = io.open(test_support.TESTFN, encoding='test_decoder')
                    self.assertEquals(f.read(i), decoded[:i])
                    cookie = f.tell()
                    self.assertEquals(f.read(j), decoded[i:i + j])
                    f.seek(cookie)
                    self.assertEquals(f.read(), decoded[i:])
                    f.close()

        # Enable the test decoder.
        StatefulIncrementalDecoder.codecEnabled = 1

        # Run the tests.
        try:
            # Try each test case.
            for input, _, _ in StatefulIncrementalDecoderTest.test_cases:
                testSeekAndTellWithData(input)

            # Position each test case so that it crosses a chunk boundary.
            CHUNK_SIZE = io.TextIOWrapper._CHUNK_SIZE
            for input, _, _ in StatefulIncrementalDecoderTest.test_cases:
                offset = CHUNK_SIZE - len(input)//2
                prefix = b'.'*offset
                # Don't bother seeking into the prefix (takes too long).
                min_pos = offset*2
                testSeekAndTellWithData(prefix + input, min_pos)

        # Ensure our test decoder won't interfere with subsequent tests.
        finally:
            StatefulIncrementalDecoder.codecEnabled = 0

    def testEncodedWrites(self):
        data = u"1234567890"
        tests = ("utf-16",
                 "utf-16-le",
                 "utf-16-be",
                 "utf-32",
                 "utf-32-le",
                 "utf-32-be")
        for encoding in tests:
            buf = io.BytesIO()
            f = io.TextIOWrapper(buf, encoding=encoding)
            # Check if the BOM is written only once (see issue1753).
            f.write(data)
            f.write(data)
            f.seek(0)
            self.assertEquals(f.read(), data * 2)
            self.assertEquals(buf.getvalue(), (data * 2).encode(encoding))

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

    def test_issue2282(self):
        buffer = io.BytesIO(self.testdata)
        txt = io.TextIOWrapper(buffer, encoding="ascii")

        self.assertEqual(buffer.seekable(), txt.seekable())

    def check_newline_decoder_utf8(self, decoder):
        # UTF-8 specific tests for a newline decoder
        def _check_decode(b, s, **kwargs):
            # We exercise getstate() / setstate() as well as decode()
            state = decoder.getstate()
            self.assertEquals(decoder.decode(b, **kwargs), s)
            decoder.setstate(state)
            self.assertEquals(decoder.decode(b, **kwargs), s)

        _check_decode(b'\xe8\xa2\x88', "\u8888")

        _check_decode(b'\xe8', "")
        _check_decode(b'\xa2', "")
        _check_decode(b'\x88', "\u8888")

        _check_decode(b'\xe8', "")
        _check_decode(b'\xa2', "")
        _check_decode(b'\x88', "\u8888")

        _check_decode(b'\xe8', "")
        self.assertRaises(UnicodeDecodeError, decoder.decode, b'', final=True)

        decoder.reset()
        _check_decode(b'\n', "\n")
        _check_decode(b'\r', "")
        _check_decode(b'', "\n", final=True)
        _check_decode(b'\r', "\n", final=True)

        _check_decode(b'\r', "")
        _check_decode(b'a', "\na")

        _check_decode(b'\r\r\n', "\n\n")
        _check_decode(b'\r', "")
        _check_decode(b'\r', "\n")
        _check_decode(b'\na', "\na")

        _check_decode(b'\xe8\xa2\x88\r\n', "\u8888\n")
        _check_decode(b'\xe8\xa2\x88', "\u8888")
        _check_decode(b'\n', "\n")
        _check_decode(b'\xe8\xa2\x88\r', "\u8888")
        _check_decode(b'\n', "\n")

    def check_newline_decoder(self, decoder, encoding):
        result = []
        encoder = codecs.getincrementalencoder(encoding)()
        def _decode_bytewise(s):
            for b in encoder.encode(s):
                result.append(decoder.decode(b))
        self.assertEquals(decoder.newlines, None)
        _decode_bytewise("abc\n\r")
        self.assertEquals(decoder.newlines, '\n')
        _decode_bytewise("\nabc")
        self.assertEquals(decoder.newlines, ('\n', '\r\n'))
        _decode_bytewise("abc\r")
        self.assertEquals(decoder.newlines, ('\n', '\r\n'))
        _decode_bytewise("abc")
        self.assertEquals(decoder.newlines, ('\r', '\n', '\r\n'))
        _decode_bytewise("abc\r")
        self.assertEquals("".join(result), "abc\n\nabcabc\nabcabc")
        decoder.reset()
        self.assertEquals(decoder.decode("abc".encode(encoding)), "abc")
        self.assertEquals(decoder.newlines, None)

    def test_newline_decoder(self):
        encodings = (
            'utf-8', 'latin-1',
            'utf-16', 'utf-16-le', 'utf-16-be',
            'utf-32', 'utf-32-le', 'utf-32-be',
        )
        for enc in encodings:
            decoder = codecs.getincrementaldecoder(enc)()
            decoder = io.IncrementalNewlineDecoder(decoder, translate=True)
            self.check_newline_decoder(decoder, enc)
        decoder = codecs.getincrementaldecoder("utf-8")()
        decoder = io.IncrementalNewlineDecoder(decoder, translate=True)
        self.check_newline_decoder_utf8(decoder)

    def test_flush_error_on_close(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ascii")
        def bad_flush():
            raise IOError()
        txt.flush = bad_flush
        self.assertRaises(IOError, txt.close) # exception not swallowed

    def test_multi_close(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ascii")
        txt.close()
        txt.close()
        txt.close()
        self.assertRaises(ValueError, txt.flush)


# XXX Tests for open()

class MiscIOTest(unittest.TestCase):

    def tearDown(self):
        test_support.unlink(test_support.TESTFN)

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


    def test_attributes(self):
        f = io.open(test_support.TESTFN, "wb", buffering=0)
        self.assertEquals(f.mode, "wb")
        f.close()

        f = io.open(test_support.TESTFN, "U")
        self.assertEquals(f.name,            test_support.TESTFN)
        self.assertEquals(f.buffer.name,     test_support.TESTFN)
        self.assertEquals(f.buffer.raw.name, test_support.TESTFN)
        self.assertEquals(f.mode,            "U")
        self.assertEquals(f.buffer.mode,     "rb")
        self.assertEquals(f.buffer.raw.mode, "rb")
        f.close()

        f = io.open(test_support.TESTFN, "w+")
        self.assertEquals(f.mode,            "w+")
        self.assertEquals(f.buffer.mode,     "rb+") # Does it really matter?
        self.assertEquals(f.buffer.raw.mode, "rb+")

        g = io.open(f.fileno(), "wb", closefd=False)
        self.assertEquals(g.mode,     "wb")
        self.assertEquals(g.raw.mode, "wb")
        self.assertEquals(g.name,     f.fileno())
        self.assertEquals(g.raw.name, f.fileno())
        f.close()
        g.close()


def test_main():
    test_support.run_unittest(IOTest, BytesIOTest, StringIOTest,
                              BufferedReaderTest, BufferedWriterTest,
                              BufferedRWPairTest, BufferedRandomTest,
                              StatefulIncrementalDecoderTest,
                              TextIOWrapperTest, MiscIOTest)

if __name__ == "__main__":
    unittest.main()
