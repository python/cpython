"""Unit tests for the io module."""

# Tests of io are scattered over the test suite:
# * test_bufio - tests file buffering
# * test_memoryio - tests BytesIO and StringIO
# * test_fileio - tests FileIO
# * test_file - tests the file interface
# * test_io - tests everything else in the io module
# * test_univnewlines - tests universal newline support
# * test_largefile - tests operations on a file greater than 2**32 bytes
#     (only enabled with -ulargefile)

################################################################################
# ATTENTION TEST WRITERS!!!
################################################################################
# When writing tests for io, it's important to test both the C and Python
# implementations. This is usually done by writing a base test that refers to
# the type it is testing as a attribute. Then it provides custom subclasses to
# test both implementations. This file has lots of examples.
################################################################################

import abc
import array
import errno
import locale
import os
import pickle
import random
import signal
import sys
import time
import unittest
import warnings
import weakref
import _testcapi
from collections import deque, UserList
from itertools import cycle, count
from test import support

import codecs
import io  # C implementation of io
import _pyio as pyio # Python implementation of io
try:
    import threading
except ImportError:
    threading = None
try:
    import fcntl
except ImportError:
    fcntl = None

def _default_chunk_size():
    """Get the default TextIOWrapper chunk size"""
    with open(__file__, "r", encoding="latin-1") as f:
        return f._CHUNK_SIZE


class MockRawIOWithoutRead:
    """A RawIO implementation without read(), so as to exercise the default
    RawIO.read() which calls readinto()."""

    def __init__(self, read_stack=()):
        self._read_stack = list(read_stack)
        self._write_stack = []
        self._reads = 0
        self._extraneous_reads = 0

    def write(self, b):
        self._write_stack.append(bytes(b))
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
        return 0   # wrong but we gotta return something

    def tell(self):
        return 0   # same comment as above

    def readinto(self, buf):
        self._reads += 1
        max_len = len(buf)
        try:
            data = self._read_stack[0]
        except IndexError:
            self._extraneous_reads += 1
            return 0
        if data is None:
            del self._read_stack[0]
            return None
        n = len(data)
        if len(data) <= max_len:
            del self._read_stack[0]
            buf[:n] = data
            return n
        else:
            buf[:] = data[:max_len]
            self._read_stack[0] = data[max_len:]
            return max_len

    def truncate(self, pos=None):
        return pos

class CMockRawIOWithoutRead(MockRawIOWithoutRead, io.RawIOBase):
    pass

class PyMockRawIOWithoutRead(MockRawIOWithoutRead, pyio.RawIOBase):
    pass


class MockRawIO(MockRawIOWithoutRead):

    def read(self, n=None):
        self._reads += 1
        try:
            return self._read_stack.pop(0)
        except:
            self._extraneous_reads += 1
            return b""

class CMockRawIO(MockRawIO, io.RawIOBase):
    pass

class PyMockRawIO(MockRawIO, pyio.RawIOBase):
    pass


class MisbehavedRawIO(MockRawIO):
    def write(self, b):
        return super().write(b) * 2

    def read(self, n=None):
        return super().read(n) * 2

    def seek(self, pos, whence):
        return -123

    def tell(self):
        return -456

    def readinto(self, buf):
        super().readinto(buf)
        return len(buf) * 5

class CMisbehavedRawIO(MisbehavedRawIO, io.RawIOBase):
    pass

class PyMisbehavedRawIO(MisbehavedRawIO, pyio.RawIOBase):
    pass


class CloseFailureIO(MockRawIO):
    closed = 0

    def close(self):
        if not self.closed:
            self.closed = 1
            raise IOError

class CCloseFailureIO(CloseFailureIO, io.RawIOBase):
    pass

class PyCloseFailureIO(CloseFailureIO, pyio.RawIOBase):
    pass


class MockFileIO:

    def __init__(self, data):
        self.read_history = []
        super().__init__(data)

    def read(self, n=None):
        res = super().read(n)
        self.read_history.append(None if res is None else len(res))
        return res

    def readinto(self, b):
        res = super().readinto(b)
        self.read_history.append(res)
        return res

class CMockFileIO(MockFileIO, io.BytesIO):
    pass

class PyMockFileIO(MockFileIO, pyio.BytesIO):
    pass


class MockUnseekableIO:
    def seekable(self):
        return False

    def seek(self, *args):
        raise self.UnsupportedOperation("not seekable")

    def tell(self, *args):
        raise self.UnsupportedOperation("not seekable")

class CMockUnseekableIO(MockUnseekableIO, io.BytesIO):
    UnsupportedOperation = io.UnsupportedOperation

class PyMockUnseekableIO(MockUnseekableIO, pyio.BytesIO):
    UnsupportedOperation = pyio.UnsupportedOperation


class MockNonBlockWriterIO:

    def __init__(self):
        self._write_stack = []
        self._blocker_char = None

    def pop_written(self):
        s = b"".join(self._write_stack)
        self._write_stack[:] = []
        return s

    def block_on(self, char):
        """Block when a given char is encountered."""
        self._blocker_char = char

    def readable(self):
        return True

    def seekable(self):
        return True

    def writable(self):
        return True

    def write(self, b):
        b = bytes(b)
        n = -1
        if self._blocker_char:
            try:
                n = b.index(self._blocker_char)
            except ValueError:
                pass
            else:
                if n > 0:
                    # write data up to the first blocker
                    self._write_stack.append(b[:n])
                    return n
                else:
                    # cancel blocker and indicate would block
                    self._blocker_char = None
                    return None
        self._write_stack.append(b)
        return len(b)

class CMockNonBlockWriterIO(MockNonBlockWriterIO, io.RawIOBase):
    BlockingIOError = io.BlockingIOError

class PyMockNonBlockWriterIO(MockNonBlockWriterIO, pyio.RawIOBase):
    BlockingIOError = pyio.BlockingIOError


class IOTest(unittest.TestCase):

    def setUp(self):
        support.unlink(support.TESTFN)

    def tearDown(self):
        support.unlink(support.TESTFN)

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

    def test_invalid_operations(self):
        # Try writing on a file opened in read mode and vice-versa.
        exc = self.UnsupportedOperation
        for mode in ("w", "wb"):
            with self.open(support.TESTFN, mode) as fp:
                self.assertRaises(exc, fp.read)
                self.assertRaises(exc, fp.readline)
        with self.open(support.TESTFN, "wb", buffering=0) as fp:
            self.assertRaises(exc, fp.read)
            self.assertRaises(exc, fp.readline)
        with self.open(support.TESTFN, "rb", buffering=0) as fp:
            self.assertRaises(exc, fp.write, b"blah")
            self.assertRaises(exc, fp.writelines, [b"blah\n"])
        with self.open(support.TESTFN, "rb") as fp:
            self.assertRaises(exc, fp.write, b"blah")
            self.assertRaises(exc, fp.writelines, [b"blah\n"])
        with self.open(support.TESTFN, "r") as fp:
            self.assertRaises(exc, fp.write, "blah")
            self.assertRaises(exc, fp.writelines, ["blah\n"])
            # Non-zero seeking from current or end pos
            self.assertRaises(exc, fp.seek, 1, self.SEEK_CUR)
            self.assertRaises(exc, fp.seek, -1, self.SEEK_END)

    def test_open_handles_NUL_chars(self):
        fn_with_NUL = 'foo\0bar'
        self.assertRaises(TypeError, self.open, fn_with_NUL, 'w')
        self.assertRaises(TypeError, self.open, bytes(fn_with_NUL, 'ascii'), 'w')

    def test_raw_file_io(self):
        with self.open(support.TESTFN, "wb", buffering=0) as f:
            self.assertEqual(f.readable(), False)
            self.assertEqual(f.writable(), True)
            self.assertEqual(f.seekable(), True)
            self.write_ops(f)
        with self.open(support.TESTFN, "rb", buffering=0) as f:
            self.assertEqual(f.readable(), True)
            self.assertEqual(f.writable(), False)
            self.assertEqual(f.seekable(), True)
            self.read_ops(f)

    def test_buffered_file_io(self):
        with self.open(support.TESTFN, "wb") as f:
            self.assertEqual(f.readable(), False)
            self.assertEqual(f.writable(), True)
            self.assertEqual(f.seekable(), True)
            self.write_ops(f)
        with self.open(support.TESTFN, "rb") as f:
            self.assertEqual(f.readable(), True)
            self.assertEqual(f.writable(), False)
            self.assertEqual(f.seekable(), True)
            self.read_ops(f, True)

    def test_readline(self):
        with self.open(support.TESTFN, "wb") as f:
            f.write(b"abc\ndef\nxyzzy\nfoo\x00bar\nanother line")
        with self.open(support.TESTFN, "rb") as f:
            self.assertEqual(f.readline(), b"abc\n")
            self.assertEqual(f.readline(10), b"def\n")
            self.assertEqual(f.readline(2), b"xy")
            self.assertEqual(f.readline(4), b"zzy\n")
            self.assertEqual(f.readline(), b"foo\x00bar\n")
            self.assertEqual(f.readline(None), b"another line")
            self.assertRaises(TypeError, f.readline, 5.3)
        with self.open(support.TESTFN, "r") as f:
            self.assertRaises(TypeError, f.readline, 5.3)

    def test_raw_bytes_io(self):
        f = self.BytesIO()
        self.write_ops(f)
        data = f.getvalue()
        self.assertEqual(data, b"hello world\n")
        f = self.BytesIO(data)
        self.read_ops(f, True)

    def test_large_file_ops(self):
        # On Windows and Mac OSX this test comsumes large resources; It takes
        # a long time to build the >2GB file and takes >2GB of disk space
        # therefore the resource must be enabled to run this test.
        if sys.platform[:3] == 'win' or sys.platform == 'darwin':
            if not support.is_resource_enabled("largefile"):
                print("\nTesting large file ops skipped on %s." % sys.platform,
                      file=sys.stderr)
                print("It requires %d bytes and a long time." % self.LARGE,
                      file=sys.stderr)
                print("Use 'regrtest.py -u largefile test_io' to run it.",
                      file=sys.stderr)
                return
        with self.open(support.TESTFN, "w+b", 0) as f:
            self.large_file_ops(f)
        with self.open(support.TESTFN, "w+b") as f:
            self.large_file_ops(f)

    def test_with_open(self):
        for bufsize in (0, 1, 100):
            f = None
            with self.open(support.TESTFN, "wb", bufsize) as f:
                f.write(b"xxx")
            self.assertEqual(f.closed, True)
            f = None
            try:
                with self.open(support.TESTFN, "wb", bufsize) as f:
                    1/0
            except ZeroDivisionError:
                self.assertEqual(f.closed, True)
            else:
                self.fail("1/0 didn't raise an exception")

    # issue 5008
    def test_append_mode_tell(self):
        with self.open(support.TESTFN, "wb") as f:
            f.write(b"xxx")
        with self.open(support.TESTFN, "ab", buffering=0) as f:
            self.assertEqual(f.tell(), 3)
        with self.open(support.TESTFN, "ab") as f:
            self.assertEqual(f.tell(), 3)
        with self.open(support.TESTFN, "a") as f:
            self.assertTrue(f.tell() > 0)

    def test_destructor(self):
        record = []
        class MyFileIO(self.FileIO):
            def __del__(self):
                record.append(1)
                try:
                    f = super().__del__
                except AttributeError:
                    pass
                else:
                    f()
            def close(self):
                record.append(2)
                super().close()
            def flush(self):
                record.append(3)
                super().flush()
        with support.check_warnings(('', ResourceWarning)):
            f = MyFileIO(support.TESTFN, "wb")
            f.write(b"xxx")
            del f
            support.gc_collect()
            self.assertEqual(record, [1, 2, 3])
            with self.open(support.TESTFN, "rb") as f:
                self.assertEqual(f.read(), b"xxx")

    def _check_base_destructor(self, base):
        record = []
        class MyIO(base):
            def __init__(self):
                # This exercises the availability of attributes on object
                # destruction.
                # (in the C version, close() is called by the tp_dealloc
                # function, not by __del__)
                self.on_del = 1
                self.on_close = 2
                self.on_flush = 3
            def __del__(self):
                record.append(self.on_del)
                try:
                    f = super().__del__
                except AttributeError:
                    pass
                else:
                    f()
            def close(self):
                record.append(self.on_close)
                super().close()
            def flush(self):
                record.append(self.on_flush)
                super().flush()
        f = MyIO()
        del f
        support.gc_collect()
        self.assertEqual(record, [1, 2, 3])

    def test_IOBase_destructor(self):
        self._check_base_destructor(self.IOBase)

    def test_RawIOBase_destructor(self):
        self._check_base_destructor(self.RawIOBase)

    def test_BufferedIOBase_destructor(self):
        self._check_base_destructor(self.BufferedIOBase)

    def test_TextIOBase_destructor(self):
        self._check_base_destructor(self.TextIOBase)

    def test_close_flushes(self):
        with self.open(support.TESTFN, "wb") as f:
            f.write(b"xxx")
        with self.open(support.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"xxx")

    def test_array_writes(self):
        a = array.array('i', range(10))
        n = len(a.tobytes())
        with self.open(support.TESTFN, "wb", 0) as f:
            self.assertEqual(f.write(a), n)
        with self.open(support.TESTFN, "wb") as f:
            self.assertEqual(f.write(a), n)

    def test_closefd(self):
        self.assertRaises(ValueError, self.open, support.TESTFN, 'w',
                          closefd=False)

    def test_read_closed(self):
        with self.open(support.TESTFN, "w") as f:
            f.write("egg\n")
        with self.open(support.TESTFN, "r") as f:
            file = self.open(f.fileno(), "r", closefd=False)
            self.assertEqual(file.read(), "egg\n")
            file.seek(0)
            file.close()
            self.assertRaises(ValueError, file.read)

    def test_no_closefd_with_filename(self):
        # can't use closefd in combination with a file name
        self.assertRaises(ValueError, self.open, support.TESTFN, "r", closefd=False)

    def test_closefd_attr(self):
        with self.open(support.TESTFN, "wb") as f:
            f.write(b"egg\n")
        with self.open(support.TESTFN, "r") as f:
            self.assertEqual(f.buffer.raw.closefd, True)
            file = self.open(f.fileno(), "r", closefd=False)
            self.assertEqual(file.buffer.raw.closefd, False)

    def test_garbage_collection(self):
        # FileIO objects are collected, and collecting them flushes
        # all data to disk.
        with support.check_warnings(('', ResourceWarning)):
            f = self.FileIO(support.TESTFN, "wb")
            f.write(b"abcxxx")
            f.f = f
            wr = weakref.ref(f)
            del f
            support.gc_collect()
        self.assertTrue(wr() is None, wr)
        with self.open(support.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"abcxxx")

    def test_unbounded_file(self):
        # Issue #1174606: reading from an unbounded stream such as /dev/zero.
        zero = "/dev/zero"
        if not os.path.exists(zero):
            self.skipTest("{0} does not exist".format(zero))
        if sys.maxsize > 0x7FFFFFFF:
            self.skipTest("test can only run in a 32-bit address space")
        if support.real_max_memuse < support._2G:
            self.skipTest("test requires at least 2GB of memory")
        with self.open(zero, "rb", buffering=0) as f:
            self.assertRaises(OverflowError, f.read)
        with self.open(zero, "rb") as f:
            self.assertRaises(OverflowError, f.read)
        with self.open(zero, "r") as f:
            self.assertRaises(OverflowError, f.read)

    def test_flush_error_on_close(self):
        f = self.open(support.TESTFN, "wb", buffering=0)
        def bad_flush():
            raise IOError()
        f.flush = bad_flush
        self.assertRaises(IOError, f.close) # exception not swallowed
        self.assertTrue(f.closed)

    def test_multi_close(self):
        f = self.open(support.TESTFN, "wb", buffering=0)
        f.close()
        f.close()
        f.close()
        self.assertRaises(ValueError, f.flush)

    def test_RawIOBase_read(self):
        # Exercise the default RawIOBase.read() implementation (which calls
        # readinto() internally).
        rawio = self.MockRawIOWithoutRead((b"abc", b"d", None, b"efg", None))
        self.assertEqual(rawio.read(2), b"ab")
        self.assertEqual(rawio.read(2), b"c")
        self.assertEqual(rawio.read(2), b"d")
        self.assertEqual(rawio.read(2), None)
        self.assertEqual(rawio.read(2), b"ef")
        self.assertEqual(rawio.read(2), b"g")
        self.assertEqual(rawio.read(2), None)
        self.assertEqual(rawio.read(2), b"")

    def test_types_have_dict(self):
        test = (
            self.IOBase(),
            self.RawIOBase(),
            self.TextIOBase(),
            self.StringIO(),
            self.BytesIO()
        )
        for obj in test:
            self.assertTrue(hasattr(obj, "__dict__"))

    def test_opener(self):
        with self.open(support.TESTFN, "w") as f:
            f.write("egg\n")
        fd = os.open(support.TESTFN, os.O_RDONLY)
        def opener(path, flags):
            return fd
        with self.open("non-existent", "r", opener=opener) as f:
            self.assertEqual(f.read(), "egg\n")

    def test_fileio_closefd(self):
        # Issue #4841
        with self.open(__file__, 'rb') as f1, \
             self.open(__file__, 'rb') as f2:
            fileio = self.FileIO(f1.fileno(), closefd=False)
            # .__init__() must not close f1
            fileio.__init__(f2.fileno(), closefd=False)
            f1.readline()
            # .close() must not close f2
            fileio.close()
            f2.readline()


class CIOTest(IOTest):

    def test_IOBase_finalize(self):
        # Issue #12149: segmentation fault on _PyIOBase_finalize when both a
        # class which inherits IOBase and an object of this class are caught
        # in a reference cycle and close() is already in the method cache.
        class MyIO(self.IOBase):
            def close(self):
                pass

        # create an instance to populate the method cache
        MyIO()
        obj = MyIO()
        obj.obj = obj
        wr = weakref.ref(obj)
        del MyIO
        del obj
        support.gc_collect()
        self.assertTrue(wr() is None, wr)

class PyIOTest(IOTest):
    pass


class CommonBufferedTests:
    # Tests common to BufferedReader, BufferedWriter and BufferedRandom

    def test_detach(self):
        raw = self.MockRawIO()
        buf = self.tp(raw)
        self.assertIs(buf.detach(), raw)
        self.assertRaises(ValueError, buf.detach)

    def test_fileno(self):
        rawio = self.MockRawIO()
        bufio = self.tp(rawio)

        self.assertEqual(42, bufio.fileno())

    def test_no_fileno(self):
        # XXX will we always have fileno() function? If so, kill
        # this test. Else, write it.
        pass

    def test_invalid_args(self):
        rawio = self.MockRawIO()
        bufio = self.tp(rawio)
        # Invalid whence
        self.assertRaises(ValueError, bufio.seek, 0, -1)
        self.assertRaises(ValueError, bufio.seek, 0, 9)

    def test_override_destructor(self):
        tp = self.tp
        record = []
        class MyBufferedIO(tp):
            def __del__(self):
                record.append(1)
                try:
                    f = super().__del__
                except AttributeError:
                    pass
                else:
                    f()
            def close(self):
                record.append(2)
                super().close()
            def flush(self):
                record.append(3)
                super().flush()
        rawio = self.MockRawIO()
        bufio = MyBufferedIO(rawio)
        writable = bufio.writable()
        del bufio
        support.gc_collect()
        if writable:
            self.assertEqual(record, [1, 2, 3])
        else:
            self.assertEqual(record, [1, 2])

    def test_context_manager(self):
        # Test usability as a context manager
        rawio = self.MockRawIO()
        bufio = self.tp(rawio)
        def _with():
            with bufio:
                pass
        _with()
        # bufio should now be closed, and using it a second time should raise
        # a ValueError.
        self.assertRaises(ValueError, _with)

    def test_error_through_destructor(self):
        # Test that the exception state is not modified by a destructor,
        # even if close() fails.
        rawio = self.CloseFailureIO()
        def f():
            self.tp(rawio).xyzzy
        with support.captured_output("stderr") as s:
            self.assertRaises(AttributeError, f)
        s = s.getvalue().strip()
        if s:
            # The destructor *may* have printed an unraisable error, check it
            self.assertEqual(len(s.splitlines()), 1)
            self.assertTrue(s.startswith("Exception IOError: "), s)
            self.assertTrue(s.endswith(" ignored"), s)

    def test_repr(self):
        raw = self.MockRawIO()
        b = self.tp(raw)
        clsname = "%s.%s" % (self.tp.__module__, self.tp.__name__)
        self.assertEqual(repr(b), "<%s>" % clsname)
        raw.name = "dummy"
        self.assertEqual(repr(b), "<%s name='dummy'>" % clsname)
        raw.name = b"dummy"
        self.assertEqual(repr(b), "<%s name=b'dummy'>" % clsname)

    def test_flush_error_on_close(self):
        raw = self.MockRawIO()
        def bad_flush():
            raise IOError()
        raw.flush = bad_flush
        b = self.tp(raw)
        self.assertRaises(IOError, b.close) # exception not swallowed
        self.assertTrue(b.closed)

    def test_close_error_on_close(self):
        raw = self.MockRawIO()
        def bad_flush():
            raise IOError('flush')
        def bad_close():
            raise IOError('close')
        raw.close = bad_close
        b = self.tp(raw)
        b.flush = bad_flush
        with self.assertRaises(IOError) as err: # exception not swallowed
            b.close()
        self.assertEqual(err.exception.args, ('close',))
        self.assertEqual(err.exception.__context__.args, ('flush',))
        self.assertFalse(b.closed)

    def test_multi_close(self):
        raw = self.MockRawIO()
        b = self.tp(raw)
        b.close()
        b.close()
        b.close()
        self.assertRaises(ValueError, b.flush)

    def test_unseekable(self):
        bufio = self.tp(self.MockUnseekableIO(b"A" * 10))
        self.assertRaises(self.UnsupportedOperation, bufio.tell)
        self.assertRaises(self.UnsupportedOperation, bufio.seek, 0)

    def test_readonly_attributes(self):
        raw = self.MockRawIO()
        buf = self.tp(raw)
        x = self.MockRawIO()
        with self.assertRaises(AttributeError):
            buf.raw = x


class SizeofTest:

    @support.cpython_only
    def test_sizeof(self):
        bufsize1 = 4096
        bufsize2 = 8192
        rawio = self.MockRawIO()
        bufio = self.tp(rawio, buffer_size=bufsize1)
        size = sys.getsizeof(bufio) - bufsize1
        rawio = self.MockRawIO()
        bufio = self.tp(rawio, buffer_size=bufsize2)
        self.assertEqual(sys.getsizeof(bufio), size + bufsize2)


class BufferedReaderTest(unittest.TestCase, CommonBufferedTests):
    read_mode = "rb"

    def test_constructor(self):
        rawio = self.MockRawIO([b"abc"])
        bufio = self.tp(rawio)
        bufio.__init__(rawio)
        bufio.__init__(rawio, buffer_size=1024)
        bufio.__init__(rawio, buffer_size=16)
        self.assertEqual(b"abc", bufio.read())
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=0)
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=-16)
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=-1)
        rawio = self.MockRawIO([b"abc"])
        bufio.__init__(rawio)
        self.assertEqual(b"abc", bufio.read())

    def test_read(self):
        for arg in (None, 7):
            rawio = self.MockRawIO((b"abc", b"d", b"efg"))
            bufio = self.tp(rawio)
            self.assertEqual(b"abcdefg", bufio.read(arg))
        # Invalid args
        self.assertRaises(ValueError, bufio.read, -2)

    def test_read1(self):
        rawio = self.MockRawIO((b"abc", b"d", b"efg"))
        bufio = self.tp(rawio)
        self.assertEqual(b"a", bufio.read(1))
        self.assertEqual(b"b", bufio.read1(1))
        self.assertEqual(rawio._reads, 1)
        self.assertEqual(b"c", bufio.read1(100))
        self.assertEqual(rawio._reads, 1)
        self.assertEqual(b"d", bufio.read1(100))
        self.assertEqual(rawio._reads, 2)
        self.assertEqual(b"efg", bufio.read1(100))
        self.assertEqual(rawio._reads, 3)
        self.assertEqual(b"", bufio.read1(100))
        self.assertEqual(rawio._reads, 4)
        # Invalid args
        self.assertRaises(ValueError, bufio.read1, -1)

    def test_readinto(self):
        rawio = self.MockRawIO((b"abc", b"d", b"efg"))
        bufio = self.tp(rawio)
        b = bytearray(2)
        self.assertEqual(bufio.readinto(b), 2)
        self.assertEqual(b, b"ab")
        self.assertEqual(bufio.readinto(b), 2)
        self.assertEqual(b, b"cd")
        self.assertEqual(bufio.readinto(b), 2)
        self.assertEqual(b, b"ef")
        self.assertEqual(bufio.readinto(b), 1)
        self.assertEqual(b, b"gf")
        self.assertEqual(bufio.readinto(b), 0)
        self.assertEqual(b, b"gf")
        rawio = self.MockRawIO((b"abc", None))
        bufio = self.tp(rawio)
        self.assertEqual(bufio.readinto(b), 2)
        self.assertEqual(b, b"ab")
        self.assertEqual(bufio.readinto(b), 1)
        self.assertEqual(b, b"cb")

    def test_readlines(self):
        def bufio():
            rawio = self.MockRawIO((b"abc\n", b"d\n", b"ef"))
            return self.tp(rawio)
        self.assertEqual(bufio().readlines(), [b"abc\n", b"d\n", b"ef"])
        self.assertEqual(bufio().readlines(5), [b"abc\n", b"d\n"])
        self.assertEqual(bufio().readlines(None), [b"abc\n", b"d\n", b"ef"])

    def test_buffering(self):
        data = b"abcdefghi"
        dlen = len(data)

        tests = [
            [ 100, [ 3, 1, 4, 8 ], [ dlen, 0 ] ],
            [ 100, [ 3, 3, 3],     [ dlen ]    ],
            [   4, [ 1, 2, 4, 2 ], [ 4, 4, 1 ] ],
        ]

        for bufsize, buf_read_sizes, raw_read_sizes in tests:
            rawio = self.MockFileIO(data)
            bufio = self.tp(rawio, buffer_size=bufsize)
            pos = 0
            for nbytes in buf_read_sizes:
                self.assertEqual(bufio.read(nbytes), data[pos:pos+nbytes])
                pos += nbytes
            # this is mildly implementation-dependent
            self.assertEqual(rawio.read_history, raw_read_sizes)

    def test_read_non_blocking(self):
        # Inject some None's in there to simulate EWOULDBLOCK
        rawio = self.MockRawIO((b"abc", b"d", None, b"efg", None, None, None))
        bufio = self.tp(rawio)
        self.assertEqual(b"abcd", bufio.read(6))
        self.assertEqual(b"e", bufio.read(1))
        self.assertEqual(b"fg", bufio.read())
        self.assertEqual(b"", bufio.peek(1))
        self.assertIsNone(bufio.read())
        self.assertEqual(b"", bufio.read())

        rawio = self.MockRawIO((b"a", None, None))
        self.assertEqual(b"a", rawio.readall())
        self.assertIsNone(rawio.readall())

    def test_read_past_eof(self):
        rawio = self.MockRawIO((b"abc", b"d", b"efg"))
        bufio = self.tp(rawio)

        self.assertEqual(b"abcdefg", bufio.read(9000))

    def test_read_all(self):
        rawio = self.MockRawIO((b"abc", b"d", b"efg"))
        bufio = self.tp(rawio)

        self.assertEqual(b"abcdefg", bufio.read())

    @unittest.skipUnless(threading, 'Threading required for this test.')
    @support.requires_resource('cpu')
    def test_threads(self):
        try:
            # Write out many bytes with exactly the same number of 0's,
            # 1's... 255's. This will help us check that concurrent reading
            # doesn't duplicate or forget contents.
            N = 1000
            l = list(range(256)) * N
            random.shuffle(l)
            s = bytes(bytearray(l))
            with self.open(support.TESTFN, "wb") as f:
                f.write(s)
            with self.open(support.TESTFN, self.read_mode, buffering=0) as raw:
                bufio = self.tp(raw, 8)
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
            support.unlink(support.TESTFN)

    def test_unseekable(self):
        bufio = self.tp(self.MockUnseekableIO(b"A" * 10))
        self.assertRaises(self.UnsupportedOperation, bufio.tell)
        self.assertRaises(self.UnsupportedOperation, bufio.seek, 0)
        bufio.read(1)
        self.assertRaises(self.UnsupportedOperation, bufio.seek, 0)
        self.assertRaises(self.UnsupportedOperation, bufio.tell)

    def test_misbehaved_io(self):
        rawio = self.MisbehavedRawIO((b"abc", b"d", b"efg"))
        bufio = self.tp(rawio)
        self.assertRaises(IOError, bufio.seek, 0)
        self.assertRaises(IOError, bufio.tell)

    def test_no_extraneous_read(self):
        # Issue #9550; when the raw IO object has satisfied the read request,
        # we should not issue any additional reads, otherwise it may block
        # (e.g. socket).
        bufsize = 16
        for n in (2, bufsize - 1, bufsize, bufsize + 1, bufsize * 2):
            rawio = self.MockRawIO([b"x" * n])
            bufio = self.tp(rawio, bufsize)
            self.assertEqual(bufio.read(n), b"x" * n)
            # Simple case: one raw read is enough to satisfy the request.
            self.assertEqual(rawio._extraneous_reads, 0,
                             "failed for {}: {} != 0".format(n, rawio._extraneous_reads))
            # A more complex case where two raw reads are needed to satisfy
            # the request.
            rawio = self.MockRawIO([b"x" * (n - 1), b"x"])
            bufio = self.tp(rawio, bufsize)
            self.assertEqual(bufio.read(n), b"x" * n)
            self.assertEqual(rawio._extraneous_reads, 0,
                             "failed for {}: {} != 0".format(n, rawio._extraneous_reads))


class CBufferedReaderTest(BufferedReaderTest, SizeofTest):
    tp = io.BufferedReader

    def test_constructor(self):
        BufferedReaderTest.test_constructor(self)
        # The allocation can succeed on 32-bit builds, e.g. with more
        # than 2GB RAM and a 64-bit kernel.
        if sys.maxsize > 0x7FFFFFFF:
            rawio = self.MockRawIO()
            bufio = self.tp(rawio)
            self.assertRaises((OverflowError, MemoryError, ValueError),
                bufio.__init__, rawio, sys.maxsize)

    def test_initialization(self):
        rawio = self.MockRawIO([b"abc"])
        bufio = self.tp(rawio)
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=0)
        self.assertRaises(ValueError, bufio.read)
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=-16)
        self.assertRaises(ValueError, bufio.read)
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=-1)
        self.assertRaises(ValueError, bufio.read)

    def test_misbehaved_io_read(self):
        rawio = self.MisbehavedRawIO((b"abc", b"d", b"efg"))
        bufio = self.tp(rawio)
        # _pyio.BufferedReader seems to implement reading different, so that
        # checking this is not so easy.
        self.assertRaises(IOError, bufio.read, 10)

    def test_garbage_collection(self):
        # C BufferedReader objects are collected.
        # The Python version has __del__, so it ends into gc.garbage instead
        rawio = self.FileIO(support.TESTFN, "w+b")
        f = self.tp(rawio)
        f.f = f
        wr = weakref.ref(f)
        del f
        support.gc_collect()
        self.assertTrue(wr() is None, wr)

class PyBufferedReaderTest(BufferedReaderTest):
    tp = pyio.BufferedReader


class BufferedWriterTest(unittest.TestCase, CommonBufferedTests):
    write_mode = "wb"

    def test_constructor(self):
        rawio = self.MockRawIO()
        bufio = self.tp(rawio)
        bufio.__init__(rawio)
        bufio.__init__(rawio, buffer_size=1024)
        bufio.__init__(rawio, buffer_size=16)
        self.assertEqual(3, bufio.write(b"abc"))
        bufio.flush()
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=0)
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=-16)
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=-1)
        bufio.__init__(rawio)
        self.assertEqual(3, bufio.write(b"ghi"))
        bufio.flush()
        self.assertEqual(b"".join(rawio._write_stack), b"abcghi")

    def test_detach_flush(self):
        raw = self.MockRawIO()
        buf = self.tp(raw)
        buf.write(b"howdy!")
        self.assertFalse(raw._write_stack)
        buf.detach()
        self.assertEqual(raw._write_stack, [b"howdy!"])

    def test_write(self):
        # Write to the buffered IO but don't overflow the buffer.
        writer = self.MockRawIO()
        bufio = self.tp(writer, 8)
        bufio.write(b"abc")
        self.assertFalse(writer._write_stack)

    def test_write_overflow(self):
        writer = self.MockRawIO()
        bufio = self.tp(writer, 8)
        contents = b"abcdefghijklmnop"
        for n in range(0, len(contents), 3):
            bufio.write(contents[n:n+3])
        flushed = b"".join(writer._write_stack)
        # At least (total - 8) bytes were implicitly flushed, perhaps more
        # depending on the implementation.
        self.assertTrue(flushed.startswith(contents[:-8]), flushed)

    def check_writes(self, intermediate_func):
        # Lots of writes, test the flushed output is as expected.
        contents = bytes(range(256)) * 1000
        n = 0
        writer = self.MockRawIO()
        bufio = self.tp(writer, 13)
        # Generator of write sizes: repeat each N 15 times then proceed to N+1
        def gen_sizes():
            for size in count(1):
                for i in range(15):
                    yield size
        sizes = gen_sizes()
        while n < len(contents):
            size = min(next(sizes), len(contents) - n)
            self.assertEqual(bufio.write(contents[n:n+size]), size)
            intermediate_func(bufio)
            n += size
        bufio.flush()
        self.assertEqual(contents, b"".join(writer._write_stack))

    def test_writes(self):
        self.check_writes(lambda bufio: None)

    def test_writes_and_flushes(self):
        self.check_writes(lambda bufio: bufio.flush())

    def test_writes_and_seeks(self):
        def _seekabs(bufio):
            pos = bufio.tell()
            bufio.seek(pos + 1, 0)
            bufio.seek(pos - 1, 0)
            bufio.seek(pos, 0)
        self.check_writes(_seekabs)
        def _seekrel(bufio):
            pos = bufio.seek(0, 1)
            bufio.seek(+1, 1)
            bufio.seek(-1, 1)
            bufio.seek(pos, 0)
        self.check_writes(_seekrel)

    def test_writes_and_truncates(self):
        self.check_writes(lambda bufio: bufio.truncate(bufio.tell()))

    def test_write_non_blocking(self):
        raw = self.MockNonBlockWriterIO()
        bufio = self.tp(raw, 8)

        self.assertEqual(bufio.write(b"abcd"), 4)
        self.assertEqual(bufio.write(b"efghi"), 5)
        # 1 byte will be written, the rest will be buffered
        raw.block_on(b"k")
        self.assertEqual(bufio.write(b"jklmn"), 5)

        # 8 bytes will be written, 8 will be buffered and the rest will be lost
        raw.block_on(b"0")
        try:
            bufio.write(b"opqrwxyz0123456789")
        except self.BlockingIOError as e:
            written = e.characters_written
        else:
            self.fail("BlockingIOError should have been raised")
        self.assertEqual(written, 16)
        self.assertEqual(raw.pop_written(),
            b"abcdefghijklmnopqrwxyz")

        self.assertEqual(bufio.write(b"ABCDEFGHI"), 9)
        s = raw.pop_written()
        # Previously buffered bytes were flushed
        self.assertTrue(s.startswith(b"01234567A"), s)

    def test_write_and_rewind(self):
        raw = io.BytesIO()
        bufio = self.tp(raw, 4)
        self.assertEqual(bufio.write(b"abcdef"), 6)
        self.assertEqual(bufio.tell(), 6)
        bufio.seek(0, 0)
        self.assertEqual(bufio.write(b"XY"), 2)
        bufio.seek(6, 0)
        self.assertEqual(raw.getvalue(), b"XYcdef")
        self.assertEqual(bufio.write(b"123456"), 6)
        bufio.flush()
        self.assertEqual(raw.getvalue(), b"XYcdef123456")

    def test_flush(self):
        writer = self.MockRawIO()
        bufio = self.tp(writer, 8)
        bufio.write(b"abc")
        bufio.flush()
        self.assertEqual(b"abc", writer._write_stack[0])

    def test_writelines(self):
        l = [b'ab', b'cd', b'ef']
        writer = self.MockRawIO()
        bufio = self.tp(writer, 8)
        bufio.writelines(l)
        bufio.flush()
        self.assertEqual(b''.join(writer._write_stack), b'abcdef')

    def test_writelines_userlist(self):
        l = UserList([b'ab', b'cd', b'ef'])
        writer = self.MockRawIO()
        bufio = self.tp(writer, 8)
        bufio.writelines(l)
        bufio.flush()
        self.assertEqual(b''.join(writer._write_stack), b'abcdef')

    def test_writelines_error(self):
        writer = self.MockRawIO()
        bufio = self.tp(writer, 8)
        self.assertRaises(TypeError, bufio.writelines, [1, 2, 3])
        self.assertRaises(TypeError, bufio.writelines, None)
        self.assertRaises(TypeError, bufio.writelines, 'abc')

    def test_destructor(self):
        writer = self.MockRawIO()
        bufio = self.tp(writer, 8)
        bufio.write(b"abc")
        del bufio
        support.gc_collect()
        self.assertEqual(b"abc", writer._write_stack[0])

    def test_truncate(self):
        # Truncate implicitly flushes the buffer.
        with self.open(support.TESTFN, self.write_mode, buffering=0) as raw:
            bufio = self.tp(raw, 8)
            bufio.write(b"abcdef")
            self.assertEqual(bufio.truncate(3), 3)
            self.assertEqual(bufio.tell(), 6)
        with self.open(support.TESTFN, "rb", buffering=0) as f:
            self.assertEqual(f.read(), b"abc")

    @unittest.skipUnless(threading, 'Threading required for this test.')
    @support.requires_resource('cpu')
    def test_threads(self):
        try:
            # Write out many bytes from many threads and test they were
            # all flushed.
            N = 1000
            contents = bytes(range(256)) * N
            sizes = cycle([1, 19])
            n = 0
            queue = deque()
            while n < len(contents):
                size = next(sizes)
                queue.append(contents[n:n+size])
                n += size
            del contents
            # We use a real file object because it allows us to
            # exercise situations where the GIL is released before
            # writing the buffer to the raw streams. This is in addition
            # to concurrency issues due to switching threads in the middle
            # of Python code.
            with self.open(support.TESTFN, self.write_mode, buffering=0) as raw:
                bufio = self.tp(raw, 8)
                errors = []
                def f():
                    try:
                        while True:
                            try:
                                s = queue.popleft()
                            except IndexError:
                                return
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
                bufio.close()
            with self.open(support.TESTFN, "rb") as f:
                s = f.read()
            for i in range(256):
                self.assertEqual(s.count(bytes([i])), N)
        finally:
            support.unlink(support.TESTFN)

    def test_misbehaved_io(self):
        rawio = self.MisbehavedRawIO()
        bufio = self.tp(rawio, 5)
        self.assertRaises(IOError, bufio.seek, 0)
        self.assertRaises(IOError, bufio.tell)
        self.assertRaises(IOError, bufio.write, b"abcdef")

    def test_max_buffer_size_removal(self):
        with self.assertRaises(TypeError):
            self.tp(self.MockRawIO(), 8, 12)

    def test_write_error_on_close(self):
        raw = self.MockRawIO()
        def bad_write(b):
            raise IOError()
        raw.write = bad_write
        b = self.tp(raw)
        b.write(b'spam')
        self.assertRaises(IOError, b.close) # exception not swallowed
        self.assertTrue(b.closed)


class CBufferedWriterTest(BufferedWriterTest, SizeofTest):
    tp = io.BufferedWriter

    def test_constructor(self):
        BufferedWriterTest.test_constructor(self)
        # The allocation can succeed on 32-bit builds, e.g. with more
        # than 2GB RAM and a 64-bit kernel.
        if sys.maxsize > 0x7FFFFFFF:
            rawio = self.MockRawIO()
            bufio = self.tp(rawio)
            self.assertRaises((OverflowError, MemoryError, ValueError),
                bufio.__init__, rawio, sys.maxsize)

    def test_initialization(self):
        rawio = self.MockRawIO()
        bufio = self.tp(rawio)
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=0)
        self.assertRaises(ValueError, bufio.write, b"def")
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=-16)
        self.assertRaises(ValueError, bufio.write, b"def")
        self.assertRaises(ValueError, bufio.__init__, rawio, buffer_size=-1)
        self.assertRaises(ValueError, bufio.write, b"def")

    def test_garbage_collection(self):
        # C BufferedWriter objects are collected, and collecting them flushes
        # all data to disk.
        # The Python version has __del__, so it ends into gc.garbage instead
        rawio = self.FileIO(support.TESTFN, "w+b")
        f = self.tp(rawio)
        f.write(b"123xxx")
        f.x = f
        wr = weakref.ref(f)
        del f
        support.gc_collect()
        self.assertTrue(wr() is None, wr)
        with self.open(support.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"123xxx")


class PyBufferedWriterTest(BufferedWriterTest):
    tp = pyio.BufferedWriter

class BufferedRWPairTest(unittest.TestCase):

    def test_constructor(self):
        pair = self.tp(self.MockRawIO(), self.MockRawIO())
        self.assertFalse(pair.closed)

    def test_detach(self):
        pair = self.tp(self.MockRawIO(), self.MockRawIO())
        self.assertRaises(self.UnsupportedOperation, pair.detach)

    def test_constructor_max_buffer_size_removal(self):
        with self.assertRaises(TypeError):
            self.tp(self.MockRawIO(), self.MockRawIO(), 8, 12)

    def test_constructor_with_not_readable(self):
        class NotReadable(MockRawIO):
            def readable(self):
                return False

        self.assertRaises(IOError, self.tp, NotReadable(), self.MockRawIO())

    def test_constructor_with_not_writeable(self):
        class NotWriteable(MockRawIO):
            def writable(self):
                return False

        self.assertRaises(IOError, self.tp, self.MockRawIO(), NotWriteable())

    def test_read(self):
        pair = self.tp(self.BytesIO(b"abcdef"), self.MockRawIO())

        self.assertEqual(pair.read(3), b"abc")
        self.assertEqual(pair.read(1), b"d")
        self.assertEqual(pair.read(), b"ef")
        pair = self.tp(self.BytesIO(b"abc"), self.MockRawIO())
        self.assertEqual(pair.read(None), b"abc")

    def test_readlines(self):
        pair = lambda: self.tp(self.BytesIO(b"abc\ndef\nh"), self.MockRawIO())
        self.assertEqual(pair().readlines(), [b"abc\n", b"def\n", b"h"])
        self.assertEqual(pair().readlines(), [b"abc\n", b"def\n", b"h"])
        self.assertEqual(pair().readlines(5), [b"abc\n", b"def\n"])

    def test_read1(self):
        # .read1() is delegated to the underlying reader object, so this test
        # can be shallow.
        pair = self.tp(self.BytesIO(b"abcdef"), self.MockRawIO())

        self.assertEqual(pair.read1(3), b"abc")

    def test_readinto(self):
        pair = self.tp(self.BytesIO(b"abcdef"), self.MockRawIO())

        data = bytearray(5)
        self.assertEqual(pair.readinto(data), 5)
        self.assertEqual(data, b"abcde")

    def test_write(self):
        w = self.MockRawIO()
        pair = self.tp(self.MockRawIO(), w)

        pair.write(b"abc")
        pair.flush()
        pair.write(b"def")
        pair.flush()
        self.assertEqual(w._write_stack, [b"abc", b"def"])

    def test_peek(self):
        pair = self.tp(self.BytesIO(b"abcdef"), self.MockRawIO())

        self.assertTrue(pair.peek(3).startswith(b"abc"))
        self.assertEqual(pair.read(3), b"abc")

    def test_readable(self):
        pair = self.tp(self.MockRawIO(), self.MockRawIO())
        self.assertTrue(pair.readable())

    def test_writeable(self):
        pair = self.tp(self.MockRawIO(), self.MockRawIO())
        self.assertTrue(pair.writable())

    def test_seekable(self):
        # BufferedRWPairs are never seekable, even if their readers and writers
        # are.
        pair = self.tp(self.MockRawIO(), self.MockRawIO())
        self.assertFalse(pair.seekable())

    # .flush() is delegated to the underlying writer object and has been
    # tested in the test_write method.

    def test_close_and_closed(self):
        pair = self.tp(self.MockRawIO(), self.MockRawIO())
        self.assertFalse(pair.closed)
        pair.close()
        self.assertTrue(pair.closed)

    def test_isatty(self):
        class SelectableIsAtty(MockRawIO):
            def __init__(self, isatty):
                MockRawIO.__init__(self)
                self._isatty = isatty

            def isatty(self):
                return self._isatty

        pair = self.tp(SelectableIsAtty(False), SelectableIsAtty(False))
        self.assertFalse(pair.isatty())

        pair = self.tp(SelectableIsAtty(True), SelectableIsAtty(False))
        self.assertTrue(pair.isatty())

        pair = self.tp(SelectableIsAtty(False), SelectableIsAtty(True))
        self.assertTrue(pair.isatty())

        pair = self.tp(SelectableIsAtty(True), SelectableIsAtty(True))
        self.assertTrue(pair.isatty())

class CBufferedRWPairTest(BufferedRWPairTest):
    tp = io.BufferedRWPair

class PyBufferedRWPairTest(BufferedRWPairTest):
    tp = pyio.BufferedRWPair


class BufferedRandomTest(BufferedReaderTest, BufferedWriterTest):
    read_mode = "rb+"
    write_mode = "wb+"

    def test_constructor(self):
        BufferedReaderTest.test_constructor(self)
        BufferedWriterTest.test_constructor(self)

    def test_read_and_write(self):
        raw = self.MockRawIO((b"asdf", b"ghjk"))
        rw = self.tp(raw, 8)

        self.assertEqual(b"as", rw.read(2))
        rw.write(b"ddd")
        rw.write(b"eee")
        self.assertFalse(raw._write_stack) # Buffer writes
        self.assertEqual(b"ghjk", rw.read())
        self.assertEqual(b"dddeee", raw._write_stack[0])

    def test_seek_and_tell(self):
        raw = self.BytesIO(b"asdfghjkl")
        rw = self.tp(raw)

        self.assertEqual(b"as", rw.read(2))
        self.assertEqual(2, rw.tell())
        rw.seek(0, 0)
        self.assertEqual(b"asdf", rw.read(4))

        rw.write(b"123f")
        rw.seek(0, 0)
        self.assertEqual(b"asdf123fl", rw.read())
        self.assertEqual(9, rw.tell())
        rw.seek(-4, 2)
        self.assertEqual(5, rw.tell())
        rw.seek(2, 1)
        self.assertEqual(7, rw.tell())
        self.assertEqual(b"fl", rw.read(11))
        rw.flush()
        self.assertEqual(b"asdf123fl", raw.getvalue())

        self.assertRaises(TypeError, rw.seek, 0.0)

    def check_flush_and_read(self, read_func):
        raw = self.BytesIO(b"abcdefghi")
        bufio = self.tp(raw)

        self.assertEqual(b"ab", read_func(bufio, 2))
        bufio.write(b"12")
        self.assertEqual(b"ef", read_func(bufio, 2))
        self.assertEqual(6, bufio.tell())
        bufio.flush()
        self.assertEqual(6, bufio.tell())
        self.assertEqual(b"ghi", read_func(bufio))
        raw.seek(0, 0)
        raw.write(b"XYZ")
        # flush() resets the read buffer
        bufio.flush()
        bufio.seek(0, 0)
        self.assertEqual(b"XYZ", read_func(bufio, 3))

    def test_flush_and_read(self):
        self.check_flush_and_read(lambda bufio, *args: bufio.read(*args))

    def test_flush_and_readinto(self):
        def _readinto(bufio, n=-1):
            b = bytearray(n if n >= 0 else 9999)
            n = bufio.readinto(b)
            return bytes(b[:n])
        self.check_flush_and_read(_readinto)

    def test_flush_and_peek(self):
        def _peek(bufio, n=-1):
            # This relies on the fact that the buffer can contain the whole
            # raw stream, otherwise peek() can return less.
            b = bufio.peek(n)
            if n != -1:
                b = b[:n]
            bufio.seek(len(b), 1)
            return b
        self.check_flush_and_read(_peek)

    def test_flush_and_write(self):
        raw = self.BytesIO(b"abcdefghi")
        bufio = self.tp(raw)

        bufio.write(b"123")
        bufio.flush()
        bufio.write(b"45")
        bufio.flush()
        bufio.seek(0, 0)
        self.assertEqual(b"12345fghi", raw.getvalue())
        self.assertEqual(b"12345fghi", bufio.read())

    def test_threads(self):
        BufferedReaderTest.test_threads(self)
        BufferedWriterTest.test_threads(self)

    def test_writes_and_peek(self):
        def _peek(bufio):
            bufio.peek(1)
        self.check_writes(_peek)
        def _peek(bufio):
            pos = bufio.tell()
            bufio.seek(-1, 1)
            bufio.peek(1)
            bufio.seek(pos, 0)
        self.check_writes(_peek)

    def test_writes_and_reads(self):
        def _read(bufio):
            bufio.seek(-1, 1)
            bufio.read(1)
        self.check_writes(_read)

    def test_writes_and_read1s(self):
        def _read1(bufio):
            bufio.seek(-1, 1)
            bufio.read1(1)
        self.check_writes(_read1)

    def test_writes_and_readintos(self):
        def _read(bufio):
            bufio.seek(-1, 1)
            bufio.readinto(bytearray(1))
        self.check_writes(_read)

    def test_write_after_readahead(self):
        # Issue #6629: writing after the buffer was filled by readahead should
        # first rewind the raw stream.
        for overwrite_size in [1, 5]:
            raw = self.BytesIO(b"A" * 10)
            bufio = self.tp(raw, 4)
            # Trigger readahead
            self.assertEqual(bufio.read(1), b"A")
            self.assertEqual(bufio.tell(), 1)
            # Overwriting should rewind the raw stream if it needs so
            bufio.write(b"B" * overwrite_size)
            self.assertEqual(bufio.tell(), overwrite_size + 1)
            # If the write size was smaller than the buffer size, flush() and
            # check that rewind happens.
            bufio.flush()
            self.assertEqual(bufio.tell(), overwrite_size + 1)
            s = raw.getvalue()
            self.assertEqual(s,
                b"A" + b"B" * overwrite_size + b"A" * (9 - overwrite_size))

    def test_write_rewind_write(self):
        # Various combinations of reading / writing / seeking backwards / writing again
        def mutate(bufio, pos1, pos2):
            assert pos2 >= pos1
            # Fill the buffer
            bufio.seek(pos1)
            bufio.read(pos2 - pos1)
            bufio.write(b'\x02')
            # This writes earlier than the previous write, but still inside
            # the buffer.
            bufio.seek(pos1)
            bufio.write(b'\x01')

        b = b"\x80\x81\x82\x83\x84"
        for i in range(0, len(b)):
            for j in range(i, len(b)):
                raw = self.BytesIO(b)
                bufio = self.tp(raw, 100)
                mutate(bufio, i, j)
                bufio.flush()
                expected = bytearray(b)
                expected[j] = 2
                expected[i] = 1
                self.assertEqual(raw.getvalue(), expected,
                                 "failed result for i=%d, j=%d" % (i, j))

    def test_truncate_after_read_or_write(self):
        raw = self.BytesIO(b"A" * 10)
        bufio = self.tp(raw, 100)
        self.assertEqual(bufio.read(2), b"AA") # the read buffer gets filled
        self.assertEqual(bufio.truncate(), 2)
        self.assertEqual(bufio.write(b"BB"), 2) # the write buffer increases
        self.assertEqual(bufio.truncate(), 4)

    def test_misbehaved_io(self):
        BufferedReaderTest.test_misbehaved_io(self)
        BufferedWriterTest.test_misbehaved_io(self)

    def test_interleaved_read_write(self):
        # Test for issue #12213
        with self.BytesIO(b'abcdefgh') as raw:
            with self.tp(raw, 100) as f:
                f.write(b"1")
                self.assertEqual(f.read(1), b'b')
                f.write(b'2')
                self.assertEqual(f.read1(1), b'd')
                f.write(b'3')
                buf = bytearray(1)
                f.readinto(buf)
                self.assertEqual(buf, b'f')
                f.write(b'4')
                self.assertEqual(f.peek(1), b'h')
                f.flush()
                self.assertEqual(raw.getvalue(), b'1b2d3f4h')

        with self.BytesIO(b'abc') as raw:
            with self.tp(raw, 100) as f:
                self.assertEqual(f.read(1), b'a')
                f.write(b"2")
                self.assertEqual(f.read(1), b'c')
                f.flush()
                self.assertEqual(raw.getvalue(), b'a2c')

    def test_interleaved_readline_write(self):
        with self.BytesIO(b'ab\ncdef\ng\n') as raw:
            with self.tp(raw) as f:
                f.write(b'1')
                self.assertEqual(f.readline(), b'b\n')
                f.write(b'2')
                self.assertEqual(f.readline(), b'def\n')
                f.write(b'3')
                self.assertEqual(f.readline(), b'\n')
                f.flush()
                self.assertEqual(raw.getvalue(), b'1b\n2def\n3\n')

    # You can't construct a BufferedRandom over a non-seekable stream.
    test_unseekable = None

class CBufferedRandomTest(BufferedRandomTest, SizeofTest):
    tp = io.BufferedRandom

    def test_constructor(self):
        BufferedRandomTest.test_constructor(self)
        # The allocation can succeed on 32-bit builds, e.g. with more
        # than 2GB RAM and a 64-bit kernel.
        if sys.maxsize > 0x7FFFFFFF:
            rawio = self.MockRawIO()
            bufio = self.tp(rawio)
            self.assertRaises((OverflowError, MemoryError, ValueError),
                bufio.__init__, rawio, sys.maxsize)

    def test_garbage_collection(self):
        CBufferedReaderTest.test_garbage_collection(self)
        CBufferedWriterTest.test_garbage_collection(self)

class PyBufferedRandomTest(BufferedRandomTest):
    tp = pyio.BufferedRandom


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
                if b == ord('.'):
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

    def test_decoder(self):
        # Try a few one-shot test cases.
        for input, eof, output in self.test_cases:
            d = StatefulIncrementalDecoder()
            self.assertEqual(d.decode(input, eof), output)

        # Also test an unfinished decode, followed by forcing EOF.
        d = StatefulIncrementalDecoder()
        self.assertEqual(d.decode(b'oiabcd'), '')
        self.assertEqual(d.decode(b'', 1), 'abcd.')

class TextIOWrapperTest(unittest.TestCase):

    def setUp(self):
        self.testdata = b"AAA\r\nBBB\rCCC\r\nDDD\nEEE\r\n"
        self.normalized = b"AAA\nBBB\nCCC\nDDD\nEEE\n".decode("ascii")
        support.unlink(support.TESTFN)

    def tearDown(self):
        support.unlink(support.TESTFN)

    def test_constructor(self):
        r = self.BytesIO(b"\xc3\xa9\n\n")
        b = self.BufferedReader(r, 1000)
        t = self.TextIOWrapper(b)
        t.__init__(b, encoding="latin-1", newline="\r\n")
        self.assertEqual(t.encoding, "latin-1")
        self.assertEqual(t.line_buffering, False)
        t.__init__(b, encoding="utf-8", line_buffering=True)
        self.assertEqual(t.encoding, "utf-8")
        self.assertEqual(t.line_buffering, True)
        self.assertEqual("\xe9\n", t.readline())
        self.assertRaises(TypeError, t.__init__, b, newline=42)
        self.assertRaises(ValueError, t.__init__, b, newline='xyzzy')

    def test_detach(self):
        r = self.BytesIO()
        b = self.BufferedWriter(r)
        t = self.TextIOWrapper(b)
        self.assertIs(t.detach(), b)

        t = self.TextIOWrapper(b, encoding="ascii")
        t.write("howdy")
        self.assertFalse(r.getvalue())
        t.detach()
        self.assertEqual(r.getvalue(), b"howdy")
        self.assertRaises(ValueError, t.detach)

    def test_repr(self):
        raw = self.BytesIO("hello".encode("utf-8"))
        b = self.BufferedReader(raw)
        t = self.TextIOWrapper(b, encoding="utf-8")
        modname = self.TextIOWrapper.__module__
        self.assertEqual(repr(t),
                         "<%s.TextIOWrapper encoding='utf-8'>" % modname)
        raw.name = "dummy"
        self.assertEqual(repr(t),
                         "<%s.TextIOWrapper name='dummy' encoding='utf-8'>" % modname)
        t.mode = "r"
        self.assertEqual(repr(t),
                         "<%s.TextIOWrapper name='dummy' mode='r' encoding='utf-8'>" % modname)
        raw.name = b"dummy"
        self.assertEqual(repr(t),
                         "<%s.TextIOWrapper name=b'dummy' mode='r' encoding='utf-8'>" % modname)

    def test_line_buffering(self):
        r = self.BytesIO()
        b = self.BufferedWriter(r, 1000)
        t = self.TextIOWrapper(b, newline="\n", line_buffering=True)
        t.write("X")
        self.assertEqual(r.getvalue(), b"")  # No flush happened
        t.write("Y\nZ")
        self.assertEqual(r.getvalue(), b"XY\nZ")  # All got flushed
        t.write("A\rB")
        self.assertEqual(r.getvalue(), b"XY\nZA\rB")

    def test_default_encoding(self):
        old_environ = dict(os.environ)
        try:
            # try to get a user preferred encoding different than the current
            # locale encoding to check that TextIOWrapper() uses the current
            # locale encoding and not the user preferred encoding
            for key in ('LC_ALL', 'LANG', 'LC_CTYPE'):
                if key in os.environ:
                    del os.environ[key]

            current_locale_encoding = locale.getpreferredencoding(False)
            b = self.BytesIO()
            t = self.TextIOWrapper(b)
            self.assertEqual(t.encoding, current_locale_encoding)
        finally:
            os.environ.clear()
            os.environ.update(old_environ)

    # Issue 15989
    def test_device_encoding(self):
        b = self.BytesIO()
        b.fileno = lambda: _testcapi.INT_MAX + 1
        self.assertRaises(OverflowError, self.TextIOWrapper, b)
        b.fileno = lambda: _testcapi.UINT_MAX + 1
        self.assertRaises(OverflowError, self.TextIOWrapper, b)

    def test_encoding(self):
        # Check the encoding attribute is always set, and valid
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="utf-8")
        self.assertEqual(t.encoding, "utf-8")
        t = self.TextIOWrapper(b)
        self.assertTrue(t.encoding is not None)
        codecs.lookup(t.encoding)

    def test_encoding_errors_reading(self):
        # (1) default
        b = self.BytesIO(b"abc\n\xff\n")
        t = self.TextIOWrapper(b, encoding="ascii")
        self.assertRaises(UnicodeError, t.read)
        # (2) explicit strict
        b = self.BytesIO(b"abc\n\xff\n")
        t = self.TextIOWrapper(b, encoding="ascii", errors="strict")
        self.assertRaises(UnicodeError, t.read)
        # (3) ignore
        b = self.BytesIO(b"abc\n\xff\n")
        t = self.TextIOWrapper(b, encoding="ascii", errors="ignore")
        self.assertEqual(t.read(), "abc\n\n")
        # (4) replace
        b = self.BytesIO(b"abc\n\xff\n")
        t = self.TextIOWrapper(b, encoding="ascii", errors="replace")
        self.assertEqual(t.read(), "abc\n\ufffd\n")

    def test_encoding_errors_writing(self):
        # (1) default
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="ascii")
        self.assertRaises(UnicodeError, t.write, "\xff")
        # (2) explicit strict
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="ascii", errors="strict")
        self.assertRaises(UnicodeError, t.write, "\xff")
        # (3) ignore
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="ascii", errors="ignore",
                             newline="\n")
        t.write("abc\xffdef\n")
        t.flush()
        self.assertEqual(b.getvalue(), b"abcdef\n")
        # (4) replace
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="ascii", errors="replace",
                             newline="\n")
        t.write("abc\xffdef\n")
        t.flush()
        self.assertEqual(b.getvalue(), b"abc?def\n")

    def test_newlines(self):
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
                        bufio = self.BufferedReader(self.BytesIO(data), bufsize)
                        textio = self.TextIOWrapper(bufio, newline=newline,
                                                  encoding=encoding)
                        if do_reads:
                            got_lines = []
                            while True:
                                c2 = textio.read(2)
                                if c2 == '':
                                    break
                                self.assertEqual(len(c2), 2)
                                got_lines.append(c2 + textio.readline())
                        else:
                            got_lines = list(textio)

                        for got_line, exp_line in zip(got_lines, exp_lines):
                            self.assertEqual(got_line, exp_line)
                        self.assertEqual(len(got_lines), len(exp_lines))

    def test_newlines_input(self):
        testdata = b"AAA\nBB\x00B\nCCC\rDDD\rEEE\r\nFFF\r\nGGG"
        normalized = testdata.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
        for newline, expected in [
            (None, normalized.decode("ascii").splitlines(keepends=True)),
            ("", testdata.decode("ascii").splitlines(keepends=True)),
            ("\n", ["AAA\n", "BB\x00B\n", "CCC\rDDD\rEEE\r\n", "FFF\r\n", "GGG"]),
            ("\r\n", ["AAA\nBB\x00B\nCCC\rDDD\rEEE\r\n", "FFF\r\n", "GGG"]),
            ("\r",  ["AAA\nBB\x00B\nCCC\r", "DDD\r", "EEE\r", "\nFFF\r", "\nGGG"]),
            ]:
            buf = self.BytesIO(testdata)
            txt = self.TextIOWrapper(buf, encoding="ascii", newline=newline)
            self.assertEqual(txt.readlines(), expected)
            txt.seek(0)
            self.assertEqual(txt.read(), "".join(expected))

    def test_newlines_output(self):
        testdict = {
            "": b"AAA\nBBB\nCCC\nX\rY\r\nZ",
            "\n": b"AAA\nBBB\nCCC\nX\rY\r\nZ",
            "\r": b"AAA\rBBB\rCCC\rX\rY\r\rZ",
            "\r\n": b"AAA\r\nBBB\r\nCCC\r\nX\rY\r\r\nZ",
            }
        tests = [(None, testdict[os.linesep])] + sorted(testdict.items())
        for newline, expected in tests:
            buf = self.BytesIO()
            txt = self.TextIOWrapper(buf, encoding="ascii", newline=newline)
            txt.write("AAA\nB")
            txt.write("BB\nCCC\n")
            txt.write("X\rY\r\nZ")
            txt.flush()
            self.assertEqual(buf.closed, False)
            self.assertEqual(buf.getvalue(), expected)

    def test_destructor(self):
        l = []
        base = self.BytesIO
        class MyBytesIO(base):
            def close(self):
                l.append(self.getvalue())
                base.close(self)
        b = MyBytesIO()
        t = self.TextIOWrapper(b, encoding="ascii")
        t.write("abc")
        del t
        support.gc_collect()
        self.assertEqual([b"abc"], l)

    def test_override_destructor(self):
        record = []
        class MyTextIO(self.TextIOWrapper):
            def __del__(self):
                record.append(1)
                try:
                    f = super().__del__
                except AttributeError:
                    pass
                else:
                    f()
            def close(self):
                record.append(2)
                super().close()
            def flush(self):
                record.append(3)
                super().flush()
        b = self.BytesIO()
        t = MyTextIO(b, encoding="ascii")
        del t
        support.gc_collect()
        self.assertEqual(record, [1, 2, 3])

    def test_error_through_destructor(self):
        # Test that the exception state is not modified by a destructor,
        # even if close() fails.
        rawio = self.CloseFailureIO()
        def f():
            self.TextIOWrapper(rawio).xyzzy
        with support.captured_output("stderr") as s:
            self.assertRaises(AttributeError, f)
        s = s.getvalue().strip()
        if s:
            # The destructor *may* have printed an unraisable error, check it
            self.assertEqual(len(s.splitlines()), 1)
            self.assertTrue(s.startswith("Exception IOError: "), s)
            self.assertTrue(s.endswith(" ignored"), s)

    # Systematic tests of the text I/O API

    def test_basic_io(self):
        for chunksize in (1, 2, 3, 4, 5, 15, 16, 17, 31, 32, 33, 63, 64, 65):
            for enc in "ascii", "latin-1", "utf-8" :# , "utf-16-be", "utf-16-le":
                f = self.open(support.TESTFN, "w+", encoding=enc)
                f._CHUNK_SIZE = chunksize
                self.assertEqual(f.write("abc"), 3)
                f.close()
                f = self.open(support.TESTFN, "r+", encoding=enc)
                f._CHUNK_SIZE = chunksize
                self.assertEqual(f.tell(), 0)
                self.assertEqual(f.read(), "abc")
                cookie = f.tell()
                self.assertEqual(f.seek(0), 0)
                self.assertEqual(f.read(None), "abc")
                f.seek(0)
                self.assertEqual(f.read(2), "ab")
                self.assertEqual(f.read(1), "c")
                self.assertEqual(f.read(1), "")
                self.assertEqual(f.read(), "")
                self.assertEqual(f.tell(), cookie)
                self.assertEqual(f.seek(0), 0)
                self.assertEqual(f.seek(0, 2), cookie)
                self.assertEqual(f.write("def"), 3)
                self.assertEqual(f.seek(cookie), cookie)
                self.assertEqual(f.read(), "def")
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
        self.assertEqual(rlines, wlines)

    def test_telling(self):
        f = self.open(support.TESTFN, "w+", encoding="utf-8")
        p0 = f.tell()
        f.write("\xff\n")
        p1 = f.tell()
        f.write("\xff\n")
        p2 = f.tell()
        f.seek(0)
        self.assertEqual(f.tell(), p0)
        self.assertEqual(f.readline(), "\xff\n")
        self.assertEqual(f.tell(), p1)
        self.assertEqual(f.readline(), "\xff\n")
        self.assertEqual(f.tell(), p2)
        f.seek(0)
        for line in f:
            self.assertEqual(line, "\xff\n")
            self.assertRaises(IOError, f.tell)
        self.assertEqual(f.tell(), p2)
        f.close()

    def test_seeking(self):
        chunk_size = _default_chunk_size()
        prefix_size = chunk_size - 2
        u_prefix = "a" * prefix_size
        prefix = bytes(u_prefix.encode("utf-8"))
        self.assertEqual(len(u_prefix), len(prefix))
        u_suffix = "\u8888\n"
        suffix = bytes(u_suffix.encode("utf-8"))
        line = prefix + suffix
        with self.open(support.TESTFN, "wb") as f:
            f.write(line*2)
        with self.open(support.TESTFN, "r", encoding="utf-8") as f:
            s = f.read(prefix_size)
            self.assertEqual(s, str(prefix, "ascii"))
            self.assertEqual(f.tell(), prefix_size)
            self.assertEqual(f.readline(), u_suffix)

    def test_seeking_too(self):
        # Regression test for a specific bug
        data = b'\xe0\xbf\xbf\n'
        with self.open(support.TESTFN, "wb") as f:
            f.write(data)
        with self.open(support.TESTFN, "r", encoding="utf-8") as f:
            f._CHUNK_SIZE  # Just test that it exists
            f._CHUNK_SIZE = 2
            f.readline()
            f.tell()

    def test_seek_and_tell(self):
        #Test seek/tell using the StatefulIncrementalDecoder.
        # Make test faster by doing smaller seeks
        CHUNK_SIZE = 128

        def test_seek_and_tell_with_data(data, min_pos=0):
            """Tell/seek to various points within a data stream and ensure
            that the decoded data returned by read() is consistent."""
            f = self.open(support.TESTFN, 'wb')
            f.write(data)
            f.close()
            f = self.open(support.TESTFN, encoding='test_decoder')
            f._CHUNK_SIZE = CHUNK_SIZE
            decoded = f.read()
            f.close()

            for i in range(min_pos, len(decoded) + 1): # seek positions
                for j in [1, 5, len(decoded) - i]: # read lengths
                    f = self.open(support.TESTFN, encoding='test_decoder')
                    self.assertEqual(f.read(i), decoded[:i])
                    cookie = f.tell()
                    self.assertEqual(f.read(j), decoded[i:i + j])
                    f.seek(cookie)
                    self.assertEqual(f.read(), decoded[i:])
                    f.close()

        # Enable the test decoder.
        StatefulIncrementalDecoder.codecEnabled = 1

        # Run the tests.
        try:
            # Try each test case.
            for input, _, _ in StatefulIncrementalDecoderTest.test_cases:
                test_seek_and_tell_with_data(input)

            # Position each test case so that it crosses a chunk boundary.
            for input, _, _ in StatefulIncrementalDecoderTest.test_cases:
                offset = CHUNK_SIZE - len(input)//2
                prefix = b'.'*offset
                # Don't bother seeking into the prefix (takes too long).
                min_pos = offset*2
                test_seek_and_tell_with_data(prefix + input, min_pos)

        # Ensure our test decoder won't interfere with subsequent tests.
        finally:
            StatefulIncrementalDecoder.codecEnabled = 0

    def test_encoded_writes(self):
        data = "1234567890"
        tests = ("utf-16",
                 "utf-16-le",
                 "utf-16-be",
                 "utf-32",
                 "utf-32-le",
                 "utf-32-be")
        for encoding in tests:
            buf = self.BytesIO()
            f = self.TextIOWrapper(buf, encoding=encoding)
            # Check if the BOM is written only once (see issue1753).
            f.write(data)
            f.write(data)
            f.seek(0)
            self.assertEqual(f.read(), data * 2)
            f.seek(0)
            self.assertEqual(f.read(), data * 2)
            self.assertEqual(buf.getvalue(), (data * 2).encode(encoding))

    def test_unreadable(self):
        class UnReadable(self.BytesIO):
            def readable(self):
                return False
        txt = self.TextIOWrapper(UnReadable())
        self.assertRaises(IOError, txt.read)

    def test_read_one_by_one(self):
        txt = self.TextIOWrapper(self.BytesIO(b"AA\r\nBB"))
        reads = ""
        while True:
            c = txt.read(1)
            if not c:
                break
            reads += c
        self.assertEqual(reads, "AA\nBB")

    def test_readlines(self):
        txt = self.TextIOWrapper(self.BytesIO(b"AA\nBB\nCC"))
        self.assertEqual(txt.readlines(), ["AA\n", "BB\n", "CC"])
        txt.seek(0)
        self.assertEqual(txt.readlines(None), ["AA\n", "BB\n", "CC"])
        txt.seek(0)
        self.assertEqual(txt.readlines(5), ["AA\n", "BB\n"])

    # read in amounts equal to TextIOWrapper._CHUNK_SIZE which is 128.
    def test_read_by_chunk(self):
        # make sure "\r\n" straddles 128 char boundary.
        txt = self.TextIOWrapper(self.BytesIO(b"A" * 127 + b"\r\nB"))
        reads = ""
        while True:
            c = txt.read(128)
            if not c:
                break
            reads += c
        self.assertEqual(reads, "A"*127+"\nB")

    def test_writelines(self):
        l = ['ab', 'cd', 'ef']
        buf = self.BytesIO()
        txt = self.TextIOWrapper(buf)
        txt.writelines(l)
        txt.flush()
        self.assertEqual(buf.getvalue(), b'abcdef')

    def test_writelines_userlist(self):
        l = UserList(['ab', 'cd', 'ef'])
        buf = self.BytesIO()
        txt = self.TextIOWrapper(buf)
        txt.writelines(l)
        txt.flush()
        self.assertEqual(buf.getvalue(), b'abcdef')

    def test_writelines_error(self):
        txt = self.TextIOWrapper(self.BytesIO())
        self.assertRaises(TypeError, txt.writelines, [1, 2, 3])
        self.assertRaises(TypeError, txt.writelines, None)
        self.assertRaises(TypeError, txt.writelines, b'abc')

    def test_issue1395_1(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")

        # read one char at a time
        reads = ""
        while True:
            c = txt.read(1)
            if not c:
                break
            reads += c
        self.assertEqual(reads, self.normalized)

    def test_issue1395_2(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = ""
        while True:
            c = txt.read(4)
            if not c:
                break
            reads += c
        self.assertEqual(reads, self.normalized)

    def test_issue1395_3(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read(4)
        reads += txt.readline()
        reads += txt.readline()
        reads += txt.readline()
        self.assertEqual(reads, self.normalized)

    def test_issue1395_4(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read()
        self.assertEqual(reads, self.normalized)

    def test_issue1395_5(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        pos = txt.tell()
        txt.seek(0)
        txt.seek(pos)
        self.assertEqual(txt.read(4), "BBB\n")

    def test_issue2282(self):
        buffer = self.BytesIO(self.testdata)
        txt = self.TextIOWrapper(buffer, encoding="ascii")

        self.assertEqual(buffer.seekable(), txt.seekable())

    def test_append_bom(self):
        # The BOM is not written again when appending to a non-empty file
        filename = support.TESTFN
        for charset in ('utf-8-sig', 'utf-16', 'utf-32'):
            with self.open(filename, 'w', encoding=charset) as f:
                f.write('aaa')
                pos = f.tell()
            with self.open(filename, 'rb') as f:
                self.assertEqual(f.read(), 'aaa'.encode(charset))

            with self.open(filename, 'a', encoding=charset) as f:
                f.write('xxx')
            with self.open(filename, 'rb') as f:
                self.assertEqual(f.read(), 'aaaxxx'.encode(charset))

    def test_seek_bom(self):
        # Same test, but when seeking manually
        filename = support.TESTFN
        for charset in ('utf-8-sig', 'utf-16', 'utf-32'):
            with self.open(filename, 'w', encoding=charset) as f:
                f.write('aaa')
                pos = f.tell()
            with self.open(filename, 'r+', encoding=charset) as f:
                f.seek(pos)
                f.write('zzz')
                f.seek(0)
                f.write('bbb')
            with self.open(filename, 'rb') as f:
                self.assertEqual(f.read(), 'bbbzzz'.encode(charset))

    def test_errors_property(self):
        with self.open(support.TESTFN, "w") as f:
            self.assertEqual(f.errors, "strict")
        with self.open(support.TESTFN, "w", errors="replace") as f:
            self.assertEqual(f.errors, "replace")

    @support.no_tracing
    @unittest.skipUnless(threading, 'Threading required for this test.')
    def test_threads_write(self):
        # Issue6750: concurrent writes could duplicate data
        event = threading.Event()
        with self.open(support.TESTFN, "w", buffering=1) as f:
            def run(n):
                text = "Thread%03d\n" % n
                event.wait()
                f.write(text)
            threads = [threading.Thread(target=lambda n=x: run(n))
                       for x in range(20)]
            for t in threads:
                t.start()
            time.sleep(0.02)
            event.set()
            for t in threads:
                t.join()
        with self.open(support.TESTFN) as f:
            content = f.read()
            for n in range(20):
                self.assertEqual(content.count("Thread%03d\n" % n), 1)

    def test_flush_error_on_close(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        def bad_flush():
            raise IOError()
        txt.flush = bad_flush
        self.assertRaises(IOError, txt.close) # exception not swallowed
        self.assertTrue(txt.closed)

    def test_multi_close(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt.close()
        txt.close()
        txt.close()
        self.assertRaises(ValueError, txt.flush)

    def test_unseekable(self):
        txt = self.TextIOWrapper(self.MockUnseekableIO(self.testdata))
        self.assertRaises(self.UnsupportedOperation, txt.tell)
        self.assertRaises(self.UnsupportedOperation, txt.seek, 0)

    def test_readonly_attributes(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        buf = self.BytesIO(self.testdata)
        with self.assertRaises(AttributeError):
            txt.buffer = buf

    def test_rawio(self):
        # Issue #12591: TextIOWrapper must work with raw I/O objects, so
        # that subprocess.Popen() can have the required unbuffered
        # semantics with universal_newlines=True.
        raw = self.MockRawIO([b'abc', b'def', b'ghi\njkl\nopq\n'])
        txt = self.TextIOWrapper(raw, encoding='ascii', newline='\n')
        # Reads
        self.assertEqual(txt.read(4), 'abcd')
        self.assertEqual(txt.readline(), 'efghi\n')
        self.assertEqual(list(txt), ['jkl\n', 'opq\n'])

    def test_rawio_write_through(self):
        # Issue #12591: with write_through=True, writes don't need a flush
        raw = self.MockRawIO([b'abc', b'def', b'ghi\njkl\nopq\n'])
        txt = self.TextIOWrapper(raw, encoding='ascii', newline='\n',
                                 write_through=True)
        txt.write('1')
        txt.write('23\n4')
        txt.write('5')
        self.assertEqual(b''.join(raw._write_stack), b'123\n45')

    def test_read_nonbytes(self):
        # Issue #17106
        # Crash when underlying read() returns non-bytes
        t = self.TextIOWrapper(self.StringIO('a'))
        self.assertRaises(TypeError, t.read, 1)
        t = self.TextIOWrapper(self.StringIO('a'))
        self.assertRaises(TypeError, t.readline)
        t = self.TextIOWrapper(self.StringIO('a'))
        self.assertRaises(TypeError, t.read)

    def test_illegal_decoder(self):
        # Issue #17106
        # Crash when decoder returns non-string
        t = self.TextIOWrapper(self.BytesIO(b'aaaaaa'), newline='\n',
                               encoding='quopri_codec')
        self.assertRaises(TypeError, t.read, 1)
        t = self.TextIOWrapper(self.BytesIO(b'aaaaaa'), newline='\n',
                               encoding='quopri_codec')
        self.assertRaises(TypeError, t.readline)
        t = self.TextIOWrapper(self.BytesIO(b'aaaaaa'), newline='\n',
                               encoding='quopri_codec')
        self.assertRaises(TypeError, t.read)


class CTextIOWrapperTest(TextIOWrapperTest):

    def test_initialization(self):
        r = self.BytesIO(b"\xc3\xa9\n\n")
        b = self.BufferedReader(r, 1000)
        t = self.TextIOWrapper(b)
        self.assertRaises(TypeError, t.__init__, b, newline=42)
        self.assertRaises(ValueError, t.read)
        self.assertRaises(ValueError, t.__init__, b, newline='xyzzy')
        self.assertRaises(ValueError, t.read)

    def test_garbage_collection(self):
        # C TextIOWrapper objects are collected, and collecting them flushes
        # all data to disk.
        # The Python version has __del__, so it ends in gc.garbage instead.
        rawio = io.FileIO(support.TESTFN, "wb")
        b = self.BufferedWriter(rawio)
        t = self.TextIOWrapper(b, encoding="ascii")
        t.write("456def")
        t.x = t
        wr = weakref.ref(t)
        del t
        support.gc_collect()
        self.assertTrue(wr() is None, wr)
        with self.open(support.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"456def")

    def test_rwpair_cleared_before_textio(self):
        # Issue 13070: TextIOWrapper's finalization would crash when called
        # after the reference to the underlying BufferedRWPair's writer got
        # cleared by the GC.
        for i in range(1000):
            b1 = self.BufferedRWPair(self.MockRawIO(), self.MockRawIO())
            t1 = self.TextIOWrapper(b1, encoding="ascii")
            b2 = self.BufferedRWPair(self.MockRawIO(), self.MockRawIO())
            t2 = self.TextIOWrapper(b2, encoding="ascii")
            # circular references
            t1.buddy = t2
            t2.buddy = t1
        support.gc_collect()


class PyTextIOWrapperTest(TextIOWrapperTest):
    pass


class IncrementalNewlineDecoderTest(unittest.TestCase):

    def check_newline_decoding_utf8(self, decoder):
        # UTF-8 specific tests for a newline decoder
        def _check_decode(b, s, **kwargs):
            # We exercise getstate() / setstate() as well as decode()
            state = decoder.getstate()
            self.assertEqual(decoder.decode(b, **kwargs), s)
            decoder.setstate(state)
            self.assertEqual(decoder.decode(b, **kwargs), s)

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

    def check_newline_decoding(self, decoder, encoding):
        result = []
        if encoding is not None:
            encoder = codecs.getincrementalencoder(encoding)()
            def _decode_bytewise(s):
                # Decode one byte at a time
                for b in encoder.encode(s):
                    result.append(decoder.decode(bytes([b])))
        else:
            encoder = None
            def _decode_bytewise(s):
                # Decode one char at a time
                for c in s:
                    result.append(decoder.decode(c))
        self.assertEqual(decoder.newlines, None)
        _decode_bytewise("abc\n\r")
        self.assertEqual(decoder.newlines, '\n')
        _decode_bytewise("\nabc")
        self.assertEqual(decoder.newlines, ('\n', '\r\n'))
        _decode_bytewise("abc\r")
        self.assertEqual(decoder.newlines, ('\n', '\r\n'))
        _decode_bytewise("abc")
        self.assertEqual(decoder.newlines, ('\r', '\n', '\r\n'))
        _decode_bytewise("abc\r")
        self.assertEqual("".join(result), "abc\n\nabcabc\nabcabc")
        decoder.reset()
        input = "abc"
        if encoder is not None:
            encoder.reset()
            input = encoder.encode(input)
        self.assertEqual(decoder.decode(input), "abc")
        self.assertEqual(decoder.newlines, None)

    def test_newline_decoder(self):
        encodings = (
            # None meaning the IncrementalNewlineDecoder takes unicode input
            # rather than bytes input
            None, 'utf-8', 'latin-1',
            'utf-16', 'utf-16-le', 'utf-16-be',
            'utf-32', 'utf-32-le', 'utf-32-be',
        )
        for enc in encodings:
            decoder = enc and codecs.getincrementaldecoder(enc)()
            decoder = self.IncrementalNewlineDecoder(decoder, translate=True)
            self.check_newline_decoding(decoder, enc)
        decoder = codecs.getincrementaldecoder("utf-8")()
        decoder = self.IncrementalNewlineDecoder(decoder, translate=True)
        self.check_newline_decoding_utf8(decoder)

    def test_newline_bytes(self):
        # Issue 5433: Excessive optimization in IncrementalNewlineDecoder
        def _check(dec):
            self.assertEqual(dec.newlines, None)
            self.assertEqual(dec.decode("\u0D00"), "\u0D00")
            self.assertEqual(dec.newlines, None)
            self.assertEqual(dec.decode("\u0A00"), "\u0A00")
            self.assertEqual(dec.newlines, None)
        dec = self.IncrementalNewlineDecoder(None, translate=False)
        _check(dec)
        dec = self.IncrementalNewlineDecoder(None, translate=True)
        _check(dec)

class CIncrementalNewlineDecoderTest(IncrementalNewlineDecoderTest):
    pass

class PyIncrementalNewlineDecoderTest(IncrementalNewlineDecoderTest):
    pass


# XXX Tests for open()

class MiscIOTest(unittest.TestCase):

    def tearDown(self):
        support.unlink(support.TESTFN)

    def test___all__(self):
        for name in self.io.__all__:
            obj = getattr(self.io, name, None)
            self.assertTrue(obj is not None, name)
            if name == "open":
                continue
            elif "error" in name.lower() or name == "UnsupportedOperation":
                self.assertTrue(issubclass(obj, Exception), name)
            elif not name.startswith("SEEK_"):
                self.assertTrue(issubclass(obj, self.IOBase))

    def test_attributes(self):
        f = self.open(support.TESTFN, "wb", buffering=0)
        self.assertEqual(f.mode, "wb")
        f.close()

        f = self.open(support.TESTFN, "U")
        self.assertEqual(f.name,            support.TESTFN)
        self.assertEqual(f.buffer.name,     support.TESTFN)
        self.assertEqual(f.buffer.raw.name, support.TESTFN)
        self.assertEqual(f.mode,            "U")
        self.assertEqual(f.buffer.mode,     "rb")
        self.assertEqual(f.buffer.raw.mode, "rb")
        f.close()

        f = self.open(support.TESTFN, "w+")
        self.assertEqual(f.mode,            "w+")
        self.assertEqual(f.buffer.mode,     "rb+") # Does it really matter?
        self.assertEqual(f.buffer.raw.mode, "rb+")

        g = self.open(f.fileno(), "wb", closefd=False)
        self.assertEqual(g.mode,     "wb")
        self.assertEqual(g.raw.mode, "wb")
        self.assertEqual(g.name,     f.fileno())
        self.assertEqual(g.raw.name, f.fileno())
        f.close()
        g.close()

    def test_io_after_close(self):
        for kwargs in [
                {"mode": "w"},
                {"mode": "wb"},
                {"mode": "w", "buffering": 1},
                {"mode": "w", "buffering": 2},
                {"mode": "wb", "buffering": 0},
                {"mode": "r"},
                {"mode": "rb"},
                {"mode": "r", "buffering": 1},
                {"mode": "r", "buffering": 2},
                {"mode": "rb", "buffering": 0},
                {"mode": "w+"},
                {"mode": "w+b"},
                {"mode": "w+", "buffering": 1},
                {"mode": "w+", "buffering": 2},
                {"mode": "w+b", "buffering": 0},
            ]:
            f = self.open(support.TESTFN, **kwargs)
            f.close()
            self.assertRaises(ValueError, f.flush)
            self.assertRaises(ValueError, f.fileno)
            self.assertRaises(ValueError, f.isatty)
            self.assertRaises(ValueError, f.__iter__)
            if hasattr(f, "peek"):
                self.assertRaises(ValueError, f.peek, 1)
            self.assertRaises(ValueError, f.read)
            if hasattr(f, "read1"):
                self.assertRaises(ValueError, f.read1, 1024)
            if hasattr(f, "readall"):
                self.assertRaises(ValueError, f.readall)
            if hasattr(f, "readinto"):
                self.assertRaises(ValueError, f.readinto, bytearray(1024))
            self.assertRaises(ValueError, f.readline)
            self.assertRaises(ValueError, f.readlines)
            self.assertRaises(ValueError, f.seek, 0)
            self.assertRaises(ValueError, f.tell)
            self.assertRaises(ValueError, f.truncate)
            self.assertRaises(ValueError, f.write,
                              b"" if "b" in kwargs['mode'] else "")
            self.assertRaises(ValueError, f.writelines, [])
            self.assertRaises(ValueError, next, f)

    def test_blockingioerror(self):
        # Various BlockingIOError issues
        class C(str):
            pass
        c = C("")
        b = self.BlockingIOError(1, c)
        c.b = b
        b.c = c
        wr = weakref.ref(c)
        del c, b
        support.gc_collect()
        self.assertTrue(wr() is None, wr)

    def test_abcs(self):
        # Test the visible base classes are ABCs.
        self.assertIsInstance(self.IOBase, abc.ABCMeta)
        self.assertIsInstance(self.RawIOBase, abc.ABCMeta)
        self.assertIsInstance(self.BufferedIOBase, abc.ABCMeta)
        self.assertIsInstance(self.TextIOBase, abc.ABCMeta)

    def _check_abc_inheritance(self, abcmodule):
        with self.open(support.TESTFN, "wb", buffering=0) as f:
            self.assertIsInstance(f, abcmodule.IOBase)
            self.assertIsInstance(f, abcmodule.RawIOBase)
            self.assertNotIsInstance(f, abcmodule.BufferedIOBase)
            self.assertNotIsInstance(f, abcmodule.TextIOBase)
        with self.open(support.TESTFN, "wb") as f:
            self.assertIsInstance(f, abcmodule.IOBase)
            self.assertNotIsInstance(f, abcmodule.RawIOBase)
            self.assertIsInstance(f, abcmodule.BufferedIOBase)
            self.assertNotIsInstance(f, abcmodule.TextIOBase)
        with self.open(support.TESTFN, "w") as f:
            self.assertIsInstance(f, abcmodule.IOBase)
            self.assertNotIsInstance(f, abcmodule.RawIOBase)
            self.assertNotIsInstance(f, abcmodule.BufferedIOBase)
            self.assertIsInstance(f, abcmodule.TextIOBase)

    def test_abc_inheritance(self):
        # Test implementations inherit from their respective ABCs
        self._check_abc_inheritance(self)

    def test_abc_inheritance_official(self):
        # Test implementations inherit from the official ABCs of the
        # baseline "io" module.
        self._check_abc_inheritance(io)

    def _check_warn_on_dealloc(self, *args, **kwargs):
        f = open(*args, **kwargs)
        r = repr(f)
        with self.assertWarns(ResourceWarning) as cm:
            f = None
            support.gc_collect()
        self.assertIn(r, str(cm.warning.args[0]))

    def test_warn_on_dealloc(self):
        self._check_warn_on_dealloc(support.TESTFN, "wb", buffering=0)
        self._check_warn_on_dealloc(support.TESTFN, "wb")
        self._check_warn_on_dealloc(support.TESTFN, "w")

    def _check_warn_on_dealloc_fd(self, *args, **kwargs):
        fds = []
        def cleanup_fds():
            for fd in fds:
                try:
                    os.close(fd)
                except EnvironmentError as e:
                    if e.errno != errno.EBADF:
                        raise
        self.addCleanup(cleanup_fds)
        r, w = os.pipe()
        fds += r, w
        self._check_warn_on_dealloc(r, *args, **kwargs)
        # When using closefd=False, there's no warning
        r, w = os.pipe()
        fds += r, w
        with warnings.catch_warnings(record=True) as recorded:
            open(r, *args, closefd=False, **kwargs)
            support.gc_collect()
        self.assertEqual(recorded, [])

    def test_warn_on_dealloc_fd(self):
        self._check_warn_on_dealloc_fd("rb", buffering=0)
        self._check_warn_on_dealloc_fd("rb")
        self._check_warn_on_dealloc_fd("r")


    def test_pickling(self):
        # Pickling file objects is forbidden
        for kwargs in [
                {"mode": "w"},
                {"mode": "wb"},
                {"mode": "wb", "buffering": 0},
                {"mode": "r"},
                {"mode": "rb"},
                {"mode": "rb", "buffering": 0},
                {"mode": "w+"},
                {"mode": "w+b"},
                {"mode": "w+b", "buffering": 0},
            ]:
            for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.open(support.TESTFN, **kwargs) as f:
                    self.assertRaises(TypeError, pickle.dumps, f, protocol)

    @unittest.skipUnless(fcntl, 'fcntl required for this test')
    def test_nonblock_pipe_write_bigbuf(self):
        self._test_nonblock_pipe_write(16*1024)

    @unittest.skipUnless(fcntl, 'fcntl required for this test')
    def test_nonblock_pipe_write_smallbuf(self):
        self._test_nonblock_pipe_write(1024)

    def _set_non_blocking(self, fd):
        flags = fcntl.fcntl(fd, fcntl.F_GETFL)
        self.assertNotEqual(flags, -1)
        res = fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)
        self.assertEqual(res, 0)

    def _test_nonblock_pipe_write(self, bufsize):
        sent = []
        received = []
        r, w = os.pipe()
        self._set_non_blocking(r)
        self._set_non_blocking(w)

        # To exercise all code paths in the C implementation we need
        # to play with buffer sizes.  For instance, if we choose a
        # buffer size less than or equal to _PIPE_BUF (4096 on Linux)
        # then we will never get a partial write of the buffer.
        rf = self.open(r, mode='rb', closefd=True, buffering=bufsize)
        wf = self.open(w, mode='wb', closefd=True, buffering=bufsize)

        with rf, wf:
            for N in 9999, 73, 7574:
                try:
                    i = 0
                    while True:
                        msg = bytes([i % 26 + 97]) * N
                        sent.append(msg)
                        wf.write(msg)
                        i += 1

                except self.BlockingIOError as e:
                    self.assertEqual(e.args[0], errno.EAGAIN)
                    self.assertEqual(e.args[2], e.characters_written)
                    sent[-1] = sent[-1][:e.characters_written]
                    received.append(rf.read())
                    msg = b'BLOCKED'
                    wf.write(msg)
                    sent.append(msg)

            while True:
                try:
                    wf.flush()
                    break
                except self.BlockingIOError as e:
                    self.assertEqual(e.args[0], errno.EAGAIN)
                    self.assertEqual(e.args[2], e.characters_written)
                    self.assertEqual(e.characters_written, 0)
                    received.append(rf.read())

            received += iter(rf.read, None)

        sent, received = b''.join(sent), b''.join(received)
        self.assertTrue(sent == received)
        self.assertTrue(wf.closed)
        self.assertTrue(rf.closed)

    def test_create_fail(self):
        # 'x' mode fails if file is existing
        with self.open(support.TESTFN, 'w'):
            pass
        self.assertRaises(FileExistsError, self.open, support.TESTFN, 'x')

    def test_create_writes(self):
        # 'x' mode opens for writing
        with self.open(support.TESTFN, 'xb') as f:
            f.write(b"spam")
        with self.open(support.TESTFN, 'rb') as f:
            self.assertEqual(b"spam", f.read())

    def test_open_allargs(self):
        # there used to be a buffer overflow in the parser for rawmode
        self.assertRaises(ValueError, self.open, support.TESTFN, 'rwax+')


class CMiscIOTest(MiscIOTest):
    io = io

class PyMiscIOTest(MiscIOTest):
    io = pyio


@unittest.skipIf(os.name == 'nt', 'POSIX signals required for this test.')
class SignalsTest(unittest.TestCase):

    def setUp(self):
        self.oldalrm = signal.signal(signal.SIGALRM, self.alarm_interrupt)

    def tearDown(self):
        signal.signal(signal.SIGALRM, self.oldalrm)

    def alarm_interrupt(self, sig, frame):
        1/0

    @unittest.skipUnless(threading, 'Threading required for this test.')
    def check_interrupted_write(self, item, bytes, **fdopen_kwargs):
        """Check that a partial write, when it gets interrupted, properly
        invokes the signal handler, and bubbles up the exception raised
        in the latter."""
        read_results = []
        def _read():
            if hasattr(signal, 'pthread_sigmask'):
                signal.pthread_sigmask(signal.SIG_BLOCK, [signal.SIGALRM])
            s = os.read(r, 1)
            read_results.append(s)
        t = threading.Thread(target=_read)
        t.daemon = True
        r, w = os.pipe()
        fdopen_kwargs["closefd"] = False
        try:
            wio = self.io.open(w, **fdopen_kwargs)
            t.start()
            signal.alarm(1)
            # Fill the pipe enough that the write will be blocking.
            # It will be interrupted by the timer armed above.  Since the
            # other thread has read one byte, the low-level write will
            # return with a successful (partial) result rather than an EINTR.
            # The buffered IO layer must check for pending signal
            # handlers, which in this case will invoke alarm_interrupt().
            self.assertRaises(ZeroDivisionError,
                        wio.write, item * (support.PIPE_MAX_SIZE // len(item)))
            t.join()
            # We got one byte, get another one and check that it isn't a
            # repeat of the first one.
            read_results.append(os.read(r, 1))
            self.assertEqual(read_results, [bytes[0:1], bytes[1:2]])
        finally:
            os.close(w)
            os.close(r)
            # This is deliberate. If we didn't close the file descriptor
            # before closing wio, wio would try to flush its internal
            # buffer, and block again.
            try:
                wio.close()
            except IOError as e:
                if e.errno != errno.EBADF:
                    raise

    def test_interrupted_write_unbuffered(self):
        self.check_interrupted_write(b"xy", b"xy", mode="wb", buffering=0)

    def test_interrupted_write_buffered(self):
        self.check_interrupted_write(b"xy", b"xy", mode="wb")

    def test_interrupted_write_text(self):
        self.check_interrupted_write("xy", b"xy", mode="w", encoding="ascii")

    @support.no_tracing
    def check_reentrant_write(self, data, **fdopen_kwargs):
        def on_alarm(*args):
            # Will be called reentrantly from the same thread
            wio.write(data)
            1/0
        signal.signal(signal.SIGALRM, on_alarm)
        r, w = os.pipe()
        wio = self.io.open(w, **fdopen_kwargs)
        try:
            signal.alarm(1)
            # Either the reentrant call to wio.write() fails with RuntimeError,
            # or the signal handler raises ZeroDivisionError.
            with self.assertRaises((ZeroDivisionError, RuntimeError)) as cm:
                while 1:
                    for i in range(100):
                        wio.write(data)
                        wio.flush()
                    # Make sure the buffer doesn't fill up and block further writes
                    os.read(r, len(data) * 100)
            exc = cm.exception
            if isinstance(exc, RuntimeError):
                self.assertTrue(str(exc).startswith("reentrant call"), str(exc))
        finally:
            wio.close()
            os.close(r)

    def test_reentrant_write_buffered(self):
        self.check_reentrant_write(b"xy", mode="wb")

    def test_reentrant_write_text(self):
        self.check_reentrant_write("xy", mode="w", encoding="ascii")

    def check_interrupted_read_retry(self, decode, **fdopen_kwargs):
        """Check that a buffered read, when it gets interrupted (either
        returning a partial result or EINTR), properly invokes the signal
        handler and retries if the latter returned successfully."""
        r, w = os.pipe()
        fdopen_kwargs["closefd"] = False
        def alarm_handler(sig, frame):
            os.write(w, b"bar")
        signal.signal(signal.SIGALRM, alarm_handler)
        try:
            rio = self.io.open(r, **fdopen_kwargs)
            os.write(w, b"foo")
            signal.alarm(1)
            # Expected behaviour:
            # - first raw read() returns partial b"foo"
            # - second raw read() returns EINTR
            # - third raw read() returns b"bar"
            self.assertEqual(decode(rio.read(6)), "foobar")
        finally:
            rio.close()
            os.close(w)
            os.close(r)

    def test_interrupted_read_retry_buffered(self):
        self.check_interrupted_read_retry(lambda x: x.decode('latin1'),
                                          mode="rb")

    def test_interrupted_read_retry_text(self):
        self.check_interrupted_read_retry(lambda x: x,
                                          mode="r")

    @unittest.skipUnless(threading, 'Threading required for this test.')
    def check_interrupted_write_retry(self, item, **fdopen_kwargs):
        """Check that a buffered write, when it gets interrupted (either
        returning a partial result or EINTR), properly invokes the signal
        handler and retries if the latter returned successfully."""
        select = support.import_module("select")
        # A quantity that exceeds the buffer size of an anonymous pipe's
        # write end.
        N = 1024 * 1024
        r, w = os.pipe()
        fdopen_kwargs["closefd"] = False
        # We need a separate thread to read from the pipe and allow the
        # write() to finish.  This thread is started after the SIGALRM is
        # received (forcing a first EINTR in write()).
        read_results = []
        write_finished = False
        def _read():
            while not write_finished:
                while r in select.select([r], [], [], 1.0)[0]:
                    s = os.read(r, 1024)
                    read_results.append(s)
        t = threading.Thread(target=_read)
        t.daemon = True
        def alarm1(sig, frame):
            signal.signal(signal.SIGALRM, alarm2)
            signal.alarm(1)
        def alarm2(sig, frame):
            t.start()
        signal.signal(signal.SIGALRM, alarm1)
        try:
            wio = self.io.open(w, **fdopen_kwargs)
            signal.alarm(1)
            # Expected behaviour:
            # - first raw write() is partial (because of the limited pipe buffer
            #   and the first alarm)
            # - second raw write() returns EINTR (because of the second alarm)
            # - subsequent write()s are successful (either partial or complete)
            self.assertEqual(N, wio.write(item * N))
            wio.flush()
            write_finished = True
            t.join()
            self.assertEqual(N, sum(len(x) for x in read_results))
        finally:
            write_finished = True
            os.close(w)
            os.close(r)
            # This is deliberate. If we didn't close the file descriptor
            # before closing wio, wio would try to flush its internal
            # buffer, and could block (in case of failure).
            try:
                wio.close()
            except IOError as e:
                if e.errno != errno.EBADF:
                    raise

    def test_interrupted_write_retry_buffered(self):
        self.check_interrupted_write_retry(b"x", mode="wb")

    def test_interrupted_write_retry_text(self):
        self.check_interrupted_write_retry("x", mode="w", encoding="latin1")


class CSignalsTest(SignalsTest):
    io = io

class PySignalsTest(SignalsTest):
    io = pyio

    # Handling reentrancy issues would slow down _pyio even more, so the
    # tests are disabled.
    test_reentrant_write_buffered = None
    test_reentrant_write_text = None


def test_main():
    tests = (CIOTest, PyIOTest,
             CBufferedReaderTest, PyBufferedReaderTest,
             CBufferedWriterTest, PyBufferedWriterTest,
             CBufferedRWPairTest, PyBufferedRWPairTest,
             CBufferedRandomTest, PyBufferedRandomTest,
             StatefulIncrementalDecoderTest,
             CIncrementalNewlineDecoderTest, PyIncrementalNewlineDecoderTest,
             CTextIOWrapperTest, PyTextIOWrapperTest,
             CMiscIOTest, PyMiscIOTest,
             CSignalsTest, PySignalsTest,
             )

    # Put the namespaces of the IO module we are testing and some useful mock
    # classes in the __dict__ of each test.
    mocks = (MockRawIO, MisbehavedRawIO, MockFileIO, CloseFailureIO,
             MockNonBlockWriterIO, MockUnseekableIO, MockRawIOWithoutRead)
    all_members = io.__all__ + ["IncrementalNewlineDecoder"]
    c_io_ns = {name : getattr(io, name) for name in all_members}
    py_io_ns = {name : getattr(pyio, name) for name in all_members}
    globs = globals()
    c_io_ns.update((x.__name__, globs["C" + x.__name__]) for x in mocks)
    py_io_ns.update((x.__name__, globs["Py" + x.__name__]) for x in mocks)
    # Avoid turning open into a bound method.
    py_io_ns["open"] = pyio.OpenWrapper
    for test in tests:
        if test.__name__.startswith("C"):
            for name, obj in c_io_ns.items():
                setattr(test, name, obj)
        elif test.__name__.startswith("Py"):
            for name, obj in py_io_ns.items():
                setattr(test, name, obj)

    support.run_unittest(*tests)

if __name__ == "__main__":
    test_main()
