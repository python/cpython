"""General tests for the io module.

New tests should go in more specific modules; see test_io/__init__.py
"""

import abc
import array
import errno
import os
import pickle
import random
import sys
import textwrap
import threading
import time
import unittest
import warnings
import weakref
from collections import deque, UserList
from itertools import cycle, count
from test import support
from test.support.script_helper import (
    assert_python_ok, assert_python_failure, run_python_until_end)
from test.support import (
    import_helper, is_apple, os_helper, threading_helper, warnings_helper,
)
from test.support.os_helper import FakePath
from .utils import byteslike, CTestCase, PyTestCase

import io  # C implementation of io
import _pyio as pyio # Python implementation of io


class IOTest:

    def setUp(self):
        os_helper.unlink(os_helper.TESTFN)

    def tearDown(self):
        os_helper.unlink(os_helper.TESTFN)

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
        buffer = bytearray(b" world\n\n\n")
        self.assertEqual(f.write(buffer), 9)
        buffer[:] = b"*" * 9  # Overwrite our copy of the data
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
        data = byteslike(data)
        self.assertEqual(f.readinto(data), 5)
        self.assertEqual(bytes(data), b" worl")
        data = bytearray(5)
        self.assertEqual(f.readinto(data), 2)
        self.assertEqual(len(data), 5)
        self.assertEqual(data[:2], b"d\n")
        self.assertEqual(f.seek(0), 0)
        self.assertEqual(f.read(20), b"hello world\n")
        self.assertEqual(f.read(1), b"")
        self.assertEqual(f.readinto(byteslike(b"x")), 0)
        self.assertEqual(f.seek(-6, 2), 6)
        self.assertEqual(f.read(5), b"world")
        self.assertEqual(f.read(0), b"")
        self.assertEqual(f.readinto(byteslike()), 0)
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
            f.seek(0)
            data = byteslike(5)
            self.assertEqual(f.readinto1(data), 5)
            self.assertEqual(bytes(data), b"hello")

    LARGE = 2**31

    def large_file_ops(self, f):
        assert f.readable()
        assert f.writable()
        try:
            self.assertEqual(f.seek(self.LARGE), self.LARGE)
        except (OverflowError, ValueError):
            self.skipTest("no largefile support")
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
        with self.open(os_helper.TESTFN, "w", encoding="utf-8") as fp:
            self.assertRaises(exc, fp.read)
            self.assertRaises(exc, fp.readline)
        with self.open(os_helper.TESTFN, "wb") as fp:
            self.assertRaises(exc, fp.read)
            self.assertRaises(exc, fp.readline)
        with self.open(os_helper.TESTFN, "wb", buffering=0) as fp:
            self.assertRaises(exc, fp.read)
            self.assertRaises(exc, fp.readline)
        with self.open(os_helper.TESTFN, "rb", buffering=0) as fp:
            self.assertRaises(exc, fp.write, b"blah")
            self.assertRaises(exc, fp.writelines, [b"blah\n"])
        with self.open(os_helper.TESTFN, "rb") as fp:
            self.assertRaises(exc, fp.write, b"blah")
            self.assertRaises(exc, fp.writelines, [b"blah\n"])
        with self.open(os_helper.TESTFN, "r", encoding="utf-8") as fp:
            self.assertRaises(exc, fp.write, "blah")
            self.assertRaises(exc, fp.writelines, ["blah\n"])
            # Non-zero seeking from current or end pos
            self.assertRaises(exc, fp.seek, 1, self.SEEK_CUR)
            self.assertRaises(exc, fp.seek, -1, self.SEEK_END)

    @support.cpython_only
    def test_startup_optimization(self):
        # gh-132952: Test that `io` is not imported at startup and that the
        # __module__ of UnsupportedOperation is set to "io".
        assert_python_ok("-S", "-c", textwrap.dedent(
            """
            import sys
            assert "io" not in sys.modules
            try:
                sys.stdin.truncate()
            except Exception as e:
                typ = type(e)
                assert typ.__module__ == "io", (typ, typ.__module__)
                assert typ.__name__ == "UnsupportedOperation", (typ, typ.__name__)
            else:
                raise AssertionError("Expected UnsupportedOperation")
            """
        ))

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_optional_abilities(self):
        # Test for OSError when optional APIs are not supported
        # The purpose of this test is to try fileno(), reading, writing and
        # seeking operations with various objects that indicate they do not
        # support these operations.

        def pipe_reader():
            [r, w] = os.pipe()
            os.close(w)  # So that read() is harmless
            return self.FileIO(r, "r")

        def pipe_writer():
            [r, w] = os.pipe()
            self.addCleanup(os.close, r)
            # Guarantee that we can write into the pipe without blocking
            thread = threading.Thread(target=os.read, args=(r, 100))
            thread.start()
            self.addCleanup(thread.join)
            return self.FileIO(w, "w")

        def buffered_reader():
            return self.BufferedReader(self.MockUnseekableIO())

        def buffered_writer():
            return self.BufferedWriter(self.MockUnseekableIO())

        def buffered_random():
            return self.BufferedRandom(self.BytesIO())

        def buffered_rw_pair():
            return self.BufferedRWPair(self.MockUnseekableIO(),
                self.MockUnseekableIO())

        def text_reader():
            class UnseekableReader(self.MockUnseekableIO):
                writable = self.BufferedIOBase.writable
                write = self.BufferedIOBase.write
            return self.TextIOWrapper(UnseekableReader(), "ascii")

        def text_writer():
            class UnseekableWriter(self.MockUnseekableIO):
                readable = self.BufferedIOBase.readable
                read = self.BufferedIOBase.read
            return self.TextIOWrapper(UnseekableWriter(), "ascii")

        tests = (
            (pipe_reader, "fr"), (pipe_writer, "fw"),
            (buffered_reader, "r"), (buffered_writer, "w"),
            (buffered_random, "rws"), (buffered_rw_pair, "rw"),
            (text_reader, "r"), (text_writer, "w"),
            (self.BytesIO, "rws"), (self.StringIO, "rws"),
        )

        def do_test(test, obj, abilities):
            readable = "r" in abilities
            self.assertEqual(obj.readable(), readable)
            writable = "w" in abilities
            self.assertEqual(obj.writable(), writable)

            if isinstance(obj, self.TextIOBase):
                data = "3"
            elif isinstance(obj, (self.BufferedIOBase, self.RawIOBase)):
                data = b"3"
            else:
                self.fail("Unknown base class")

            if "f" in abilities:
                obj.fileno()
            else:
                self.assertRaises(OSError, obj.fileno)

            if readable:
                obj.read(1)
                obj.read()
            else:
                self.assertRaises(OSError, obj.read, 1)
                self.assertRaises(OSError, obj.read)

            if writable:
                obj.write(data)
            else:
                self.assertRaises(OSError, obj.write, data)

            if sys.platform.startswith("win") and test in (
                    pipe_reader, pipe_writer):
                # Pipes seem to appear as seekable on Windows
                return
            seekable = "s" in abilities
            self.assertEqual(obj.seekable(), seekable)

            if seekable:
                obj.tell()
                obj.seek(0)
            else:
                self.assertRaises(OSError, obj.tell)
                self.assertRaises(OSError, obj.seek, 0)

            if writable and seekable:
                obj.truncate()
                obj.truncate(0)
            else:
                self.assertRaises(OSError, obj.truncate)
                self.assertRaises(OSError, obj.truncate, 0)

        for [test, abilities] in tests:
            with self.subTest(test):
                if test == pipe_writer and not threading_helper.can_start_thread:
                    self.skipTest("Need threads")
                with test() as obj:
                    do_test(test, obj, abilities)


    def test_open_handles_NUL_chars(self):
        fn_with_NUL = 'foo\0bar'
        self.assertRaises(ValueError, self.open, fn_with_NUL, 'w', encoding="utf-8")

        bytes_fn = bytes(fn_with_NUL, 'ascii')
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertRaises(ValueError, self.open, bytes_fn, 'w', encoding="utf-8")

    def test_raw_file_io(self):
        with self.open(os_helper.TESTFN, "wb", buffering=0) as f:
            self.assertEqual(f.readable(), False)
            self.assertEqual(f.writable(), True)
            self.assertEqual(f.seekable(), True)
            self.write_ops(f)
        with self.open(os_helper.TESTFN, "rb", buffering=0) as f:
            self.assertEqual(f.readable(), True)
            self.assertEqual(f.writable(), False)
            self.assertEqual(f.seekable(), True)
            self.read_ops(f)

    def test_buffered_file_io(self):
        with self.open(os_helper.TESTFN, "wb") as f:
            self.assertEqual(f.readable(), False)
            self.assertEqual(f.writable(), True)
            self.assertEqual(f.seekable(), True)
            self.write_ops(f)
        with self.open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(f.readable(), True)
            self.assertEqual(f.writable(), False)
            self.assertEqual(f.seekable(), True)
            self.read_ops(f, True)

    def test_readline(self):
        with self.open(os_helper.TESTFN, "wb") as f:
            f.write(b"abc\ndef\nxyzzy\nfoo\x00bar\nanother line")
        with self.open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(f.readline(), b"abc\n")
            self.assertEqual(f.readline(10), b"def\n")
            self.assertEqual(f.readline(2), b"xy")
            self.assertEqual(f.readline(4), b"zzy\n")
            self.assertEqual(f.readline(), b"foo\x00bar\n")
            self.assertEqual(f.readline(None), b"another line")
            self.assertRaises(TypeError, f.readline, 5.3)
        with self.open(os_helper.TESTFN, "r", encoding="utf-8") as f:
            self.assertRaises(TypeError, f.readline, 5.3)

    def test_readline_nonsizeable(self):
        # Issue #30061
        # Crash when readline() returns an object without __len__
        class R(self.IOBase):
            def readline(self):
                return None
        self.assertRaises((TypeError, StopIteration), next, R())

    def test_next_nonsizeable(self):
        # Issue #30061
        # Crash when __next__() returns an object without __len__
        class R(self.IOBase):
            def __next__(self):
                return None
        self.assertRaises(TypeError, R().readlines, 1)

    def test_raw_bytes_io(self):
        f = self.BytesIO()
        self.write_ops(f)
        data = f.getvalue()
        self.assertEqual(data, b"hello world\n")
        f = self.BytesIO(data)
        self.read_ops(f, True)

    def test_large_file_ops(self):
        # On Windows and Apple platforms this test consumes large resources; It
        # takes a long time to build the >2 GiB file and takes >2 GiB of disk
        # space therefore the resource must be enabled to run this test.
        if sys.platform[:3] == 'win' or is_apple:
            support.requires(
                'largefile',
                'test requires %s bytes and a long time to run' % self.LARGE)
        with self.open(os_helper.TESTFN, "w+b", 0) as f:
            self.large_file_ops(f)
        with self.open(os_helper.TESTFN, "w+b") as f:
            self.large_file_ops(f)

    def test_with_open(self):
        for bufsize in (0, 100):
            with self.open(os_helper.TESTFN, "wb", bufsize) as f:
                f.write(b"xxx")
            self.assertEqual(f.closed, True)
            try:
                with self.open(os_helper.TESTFN, "wb", bufsize) as f:
                    1/0
            except ZeroDivisionError:
                self.assertEqual(f.closed, True)
            else:
                self.fail("1/0 didn't raise an exception")

    # issue 5008
    def test_append_mode_tell(self):
        with self.open(os_helper.TESTFN, "wb") as f:
            f.write(b"xxx")
        with self.open(os_helper.TESTFN, "ab", buffering=0) as f:
            self.assertEqual(f.tell(), 3)
        with self.open(os_helper.TESTFN, "ab") as f:
            self.assertEqual(f.tell(), 3)
        with self.open(os_helper.TESTFN, "a", encoding="utf-8") as f:
            self.assertGreater(f.tell(), 0)

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
        with warnings_helper.check_warnings(('', ResourceWarning)):
            f = MyFileIO(os_helper.TESTFN, "wb")
            f.write(b"xxx")
            del f
            support.gc_collect()
            self.assertEqual(record, [1, 2, 3])
            with self.open(os_helper.TESTFN, "rb") as f:
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
        with self.open(os_helper.TESTFN, "wb") as f:
            f.write(b"xxx")
        with self.open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"xxx")

    def test_array_writes(self):
        a = array.array('i', range(10))
        n = len(a.tobytes())
        def check(f):
            with f:
                self.assertEqual(f.write(a), n)
                f.writelines((a,))
        check(self.BytesIO())
        check(self.FileIO(os_helper.TESTFN, "w"))
        check(self.BufferedWriter(self.MockRawIO()))
        check(self.BufferedRandom(self.MockRawIO()))
        check(self.BufferedRWPair(self.MockRawIO(), self.MockRawIO()))

    def test_closefd(self):
        self.assertRaises(ValueError, self.open, os_helper.TESTFN, 'w',
                          encoding="utf-8", closefd=False)

    def test_read_closed(self):
        with self.open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            f.write("egg\n")
        with self.open(os_helper.TESTFN, "r", encoding="utf-8") as f:
            file = self.open(f.fileno(), "r", encoding="utf-8", closefd=False)
            self.assertEqual(file.read(), "egg\n")
            file.seek(0)
            file.close()
            self.assertRaises(ValueError, file.read)
        with self.open(os_helper.TESTFN, "rb") as f:
            file = self.open(f.fileno(), "rb", closefd=False)
            self.assertEqual(file.read()[:3], b"egg")
            file.close()
            self.assertRaises(ValueError, file.readinto, bytearray(1))

    def test_no_closefd_with_filename(self):
        # can't use closefd in combination with a file name
        self.assertRaises(ValueError, self.open, os_helper.TESTFN, "r",
                          encoding="utf-8", closefd=False)

    def test_closefd_attr(self):
        with self.open(os_helper.TESTFN, "wb") as f:
            f.write(b"egg\n")
        with self.open(os_helper.TESTFN, "r", encoding="utf-8") as f:
            self.assertEqual(f.buffer.raw.closefd, True)
            file = self.open(f.fileno(), "r", encoding="utf-8", closefd=False)
            self.assertEqual(file.buffer.raw.closefd, False)

    def test_garbage_collection(self):
        # FileIO objects are collected, and collecting them flushes
        # all data to disk.
        #
        # Note that using warnings_helper.check_warnings() will keep the
        # file alive due to the `source` argument to warn().  So, use
        # catch_warnings() instead.
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ResourceWarning)
            f = self.FileIO(os_helper.TESTFN, "wb")
            f.write(b"abcxxx")
            f.f = f
            wr = weakref.ref(f)
            del f
            support.gc_collect()
        self.assertIsNone(wr(), wr)
        with self.open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"abcxxx")

    def test_unbounded_file(self):
        # Issue #1174606: reading from an unbounded stream such as /dev/zero.
        zero = "/dev/zero"
        if not os.path.exists(zero):
            self.skipTest("{0} does not exist".format(zero))
        if sys.maxsize > 0x7FFFFFFF:
            self.skipTest("test can only run in a 32-bit address space")
        if support.real_max_memuse < support._2G:
            self.skipTest("test requires at least 2 GiB of memory")
        with self.open(zero, "rb", buffering=0) as f:
            self.assertRaises(OverflowError, f.read)
        with self.open(zero, "rb") as f:
            self.assertRaises(OverflowError, f.read)
        with self.open(zero, "r") as f:
            self.assertRaises(OverflowError, f.read)

    def check_flush_error_on_close(self, *args, **kwargs):
        # Test that the file is closed despite failed flush
        # and that flush() is called before file closed.
        f = self.open(*args, **kwargs)
        closed = []
        def bad_flush():
            closed[:] = [f.closed]
            raise OSError()
        f.flush = bad_flush
        self.assertRaises(OSError, f.close) # exception not swallowed
        self.assertTrue(f.closed)
        self.assertTrue(closed)      # flush() called
        self.assertFalse(closed[0])  # flush() called before file closed
        f.flush = lambda: None  # break reference loop

    def test_flush_error_on_close(self):
        # raw file
        # Issue #5700: io.FileIO calls flush() after file closed
        self.check_flush_error_on_close(os_helper.TESTFN, 'wb', buffering=0)
        fd = os.open(os_helper.TESTFN, os.O_WRONLY|os.O_CREAT)
        self.check_flush_error_on_close(fd, 'wb', buffering=0)
        fd = os.open(os_helper.TESTFN, os.O_WRONLY|os.O_CREAT)
        self.check_flush_error_on_close(fd, 'wb', buffering=0, closefd=False)
        os.close(fd)
        # buffered io
        self.check_flush_error_on_close(os_helper.TESTFN, 'wb')
        fd = os.open(os_helper.TESTFN, os.O_WRONLY|os.O_CREAT)
        self.check_flush_error_on_close(fd, 'wb')
        fd = os.open(os_helper.TESTFN, os.O_WRONLY|os.O_CREAT)
        self.check_flush_error_on_close(fd, 'wb', closefd=False)
        os.close(fd)
        # text io
        self.check_flush_error_on_close(os_helper.TESTFN, 'w', encoding="utf-8")
        fd = os.open(os_helper.TESTFN, os.O_WRONLY|os.O_CREAT)
        self.check_flush_error_on_close(fd, 'w', encoding="utf-8")
        fd = os.open(os_helper.TESTFN, os.O_WRONLY|os.O_CREAT)
        self.check_flush_error_on_close(fd, 'w', encoding="utf-8", closefd=False)
        os.close(fd)

    def test_multi_close(self):
        f = self.open(os_helper.TESTFN, "wb", buffering=0)
        f.close()
        f.close()
        f.close()
        self.assertRaises(ValueError, f.flush)

    def test_RawIOBase_read(self):
        # Exercise the default limited RawIOBase.read(n) implementation (which
        # calls readinto() internally).
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
            self.assertHasAttr(obj, "__dict__")

    def test_opener(self):
        with self.open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            f.write("egg\n")
        fd = os.open(os_helper.TESTFN, os.O_RDONLY)
        def opener(path, flags):
            return fd
        with self.open("non-existent", "r", encoding="utf-8", opener=opener) as f:
            self.assertEqual(f.read(), "egg\n")

    def test_bad_opener_negative_1(self):
        # Issue #27066.
        def badopener(fname, flags):
            return -1
        with self.assertRaises(ValueError) as cm:
            self.open('non-existent', 'r', opener=badopener)
        self.assertEqual(str(cm.exception), 'opener returned -1')

    def test_bad_opener_other_negative(self):
        # Issue #27066.
        def badopener(fname, flags):
            return -2
        with self.assertRaises(ValueError) as cm:
            self.open('non-existent', 'r', opener=badopener)
        self.assertEqual(str(cm.exception), 'opener returned -2')

    def test_opener_invalid_fd(self):
        # Check that OSError is raised with error code EBADF if the
        # opener returns an invalid file descriptor (see gh-82212).
        fd = os_helper.make_bad_fd()
        with self.assertRaises(OSError) as cm:
            self.open('foo', opener=lambda name, flags: fd)
        self.assertEqual(cm.exception.errno, errno.EBADF)

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

    def test_nonbuffered_textio(self):
        with warnings_helper.check_no_resource_warning(self):
            with self.assertRaises(ValueError):
                self.open(os_helper.TESTFN, 'w', encoding="utf-8", buffering=0)

    def test_invalid_newline(self):
        with warnings_helper.check_no_resource_warning(self):
            with self.assertRaises(ValueError):
                self.open(os_helper.TESTFN, 'w', encoding="utf-8", newline='invalid')

    def test_buffered_readinto_mixin(self):
        # Test the implementation provided by BufferedIOBase
        class Stream(self.BufferedIOBase):
            def read(self, size):
                return b"12345"
            read1 = read
        stream = Stream()
        for method in ("readinto", "readinto1"):
            with self.subTest(method):
                buffer = byteslike(5)
                self.assertEqual(getattr(stream, method)(buffer), 5)
                self.assertEqual(bytes(buffer), b"12345")

    def test_fspath_support(self):
        def check_path_succeeds(path):
            with self.open(path, "w", encoding="utf-8") as f:
                f.write("egg\n")

            with self.open(path, "r", encoding="utf-8") as f:
                self.assertEqual(f.read(), "egg\n")

        check_path_succeeds(FakePath(os_helper.TESTFN))
        check_path_succeeds(FakePath(os.fsencode(os_helper.TESTFN)))

        with self.open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            bad_path = FakePath(f.fileno())
            with self.assertRaises(TypeError):
                self.open(bad_path, 'w', encoding="utf-8")

        bad_path = FakePath(None)
        with self.assertRaises(TypeError):
            self.open(bad_path, 'w', encoding="utf-8")

        bad_path = FakePath(FloatingPointError)
        with self.assertRaises(FloatingPointError):
            self.open(bad_path, 'w', encoding="utf-8")

        # ensure that refcounting is correct with some error conditions
        with self.assertRaisesRegex(ValueError, 'read/write/append mode'):
            self.open(FakePath(os_helper.TESTFN), 'rwxa', encoding="utf-8")

    def test_RawIOBase_readall(self):
        # Exercise the default unlimited RawIOBase.read() and readall()
        # implementations.
        rawio = self.MockRawIOWithoutRead((b"abc", b"d", b"efg"))
        self.assertEqual(rawio.read(), b"abcdefg")
        rawio = self.MockRawIOWithoutRead((b"abc", b"d", b"efg"))
        self.assertEqual(rawio.readall(), b"abcdefg")

    def test_BufferedIOBase_readinto(self):
        # Exercise the default BufferedIOBase.readinto() and readinto1()
        # implementations (which call read() or read1() internally).
        class Reader(self.BufferedIOBase):
            def __init__(self, avail):
                self.avail = avail
            def read(self, size):
                result = self.avail[:size]
                self.avail = self.avail[size:]
                return result
            def read1(self, size):
                """Returns no more than 5 bytes at once"""
                return self.read(min(size, 5))
        tests = (
            # (test method, total data available, read buffer size, expected
            #     read size)
            ("readinto", 10, 5, 5),
            ("readinto", 10, 6, 6),  # More than read1() can return
            ("readinto", 5, 6, 5),  # Buffer larger than total available
            ("readinto", 6, 7, 6),
            ("readinto", 10, 0, 0),  # Empty buffer
            ("readinto1", 10, 5, 5),  # Result limited to single read1() call
            ("readinto1", 10, 6, 5),  # Buffer larger than read1() can return
            ("readinto1", 5, 6, 5),  # Buffer larger than total available
            ("readinto1", 6, 7, 5),
            ("readinto1", 10, 0, 0),  # Empty buffer
        )
        UNUSED_BYTE = 0x81
        for test in tests:
            with self.subTest(test):
                method, avail, request, result = test
                reader = Reader(bytes(range(avail)))
                buffer = bytearray((UNUSED_BYTE,) * request)
                method = getattr(reader, method)
                self.assertEqual(method(buffer), result)
                self.assertEqual(len(buffer), request)
                self.assertSequenceEqual(buffer[:result], range(result))
                unused = (UNUSED_BYTE,) * (request - result)
                self.assertSequenceEqual(buffer[result:], unused)
                self.assertEqual(len(reader.avail), avail - result)

    def test_close_assert(self):
        class R(self.IOBase):
            def __setattr__(self, name, value):
                pass
            def flush(self):
                raise OSError()
        f = R()
        # This would cause an assertion failure.
        self.assertRaises(OSError, f.close)

        # Silence destructor error
        R.flush = lambda self: None

    @threading_helper.requires_working_threading()
    def test_write_readline_races(self):
        # gh-134908: Concurrent iteration over a file caused races
        thread_count = 2
        write_count = 100
        read_count = 100

        def writer(file, barrier):
            barrier.wait()
            for _ in range(write_count):
                file.write("x")

        def reader(file, barrier):
            barrier.wait()
            for _ in range(read_count):
                for line in file:
                    self.assertEqual(line, "")

        with self.open(os_helper.TESTFN, "w+") as f:
            barrier = threading.Barrier(thread_count + 1)
            reader = threading.Thread(target=reader, args=(f, barrier))
            writers = [threading.Thread(target=writer, args=(f, barrier))
                       for _ in range(thread_count)]
            with threading_helper.catch_threading_exception() as cm:
                with threading_helper.start_threads(writers + [reader]):
                    pass
                self.assertIsNone(cm.exc_type)

        self.assertEqual(os.stat(os_helper.TESTFN).st_size,
                         write_count * thread_count)


class CIOTest(IOTest, CTestCase):

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
        self.assertIsNone(wr(), wr)

@support.cpython_only
class TestIOCTypes(unittest.TestCase):
    def setUp(self):
        _io = import_helper.import_module("_io")
        self.types = [
            _io.BufferedRWPair,
            _io.BufferedRandom,
            _io.BufferedReader,
            _io.BufferedWriter,
            _io.BytesIO,
            _io.FileIO,
            _io.IncrementalNewlineDecoder,
            _io.StringIO,
            _io.TextIOWrapper,
            _io._BufferedIOBase,
            _io._BytesIOBuffer,
            _io._IOBase,
            _io._RawIOBase,
            _io._TextIOBase,
        ]
        if sys.platform == "win32":
            self.types.append(_io._WindowsConsoleIO)
        self._io = _io

    def test_immutable_types(self):
        for tp in self.types:
            with self.subTest(tp=tp):
                with self.assertRaisesRegex(TypeError, "immutable"):
                    tp.foo = "bar"

    def test_class_hierarchy(self):
        def check_subs(types, base):
            for tp in types:
                with self.subTest(tp=tp, base=base):
                    self.assertIsSubclass(tp, base)

        def recursive_check(d):
            for k, v in d.items():
                if isinstance(v, dict):
                    recursive_check(v)
                elif isinstance(v, set):
                    check_subs(v, k)
                else:
                    self.fail("corrupt test dataset")

        _io = self._io
        hierarchy = {
            _io._IOBase: {
                _io._BufferedIOBase: {
                    _io.BufferedRWPair,
                    _io.BufferedRandom,
                    _io.BufferedReader,
                    _io.BufferedWriter,
                    _io.BytesIO,
                },
                _io._RawIOBase: {
                    _io.FileIO,
                },
                _io._TextIOBase: {
                    _io.StringIO,
                    _io.TextIOWrapper,
                },
            },
        }
        if sys.platform == "win32":
            hierarchy[_io._IOBase][_io._RawIOBase].add(_io._WindowsConsoleIO)

        recursive_check(hierarchy)

    def test_subclassing(self):
        _io = self._io
        dataset = {k: True for k in self.types}
        dataset[_io._BytesIOBuffer] = False

        for tp, is_basetype in dataset.items():
            with self.subTest(tp=tp, is_basetype=is_basetype):
                name = f"{tp.__name__}_subclass"
                bases = (tp,)
                if is_basetype:
                    _ = type(name, bases, {})
                else:
                    msg = "not an acceptable base type"
                    with self.assertRaisesRegex(TypeError, msg):
                        _ = type(name, bases, {})

    def test_disallow_instantiation(self):
        _io = self._io
        support.check_disallow_instantiation(self, _io._BytesIOBuffer)

    def test_stringio_setstate(self):
        # gh-127182: Calling __setstate__() with invalid arguments must not crash
        obj = self._io.StringIO()
        with self.assertRaisesRegex(
            TypeError,
            'initial_value must be str or None, not int',
        ):
            obj.__setstate__((1, '', 0, {}))

        obj.__setstate__((None, '', 0, {}))  # should not crash
        self.assertEqual(obj.getvalue(), '')

        obj.__setstate__(('', '', 0, {}))
        self.assertEqual(obj.getvalue(), '')

class PyIOTest(IOTest, PyTestCase):
    pass


@support.cpython_only
class APIMismatchTest(unittest.TestCase):

    def test_RawIOBase_io_in_pyio_match(self):
        """Test that pyio RawIOBase class has all c RawIOBase methods"""
        mismatch = support.detect_api_mismatch(pyio.RawIOBase, io.RawIOBase,
                                               ignore=('__weakref__', '__static_attributes__'))
        self.assertEqual(mismatch, set(), msg='Python RawIOBase does not have all C RawIOBase methods')

    def test_RawIOBase_pyio_in_io_match(self):
        """Test that c RawIOBase class has all pyio RawIOBase methods"""
        mismatch = support.detect_api_mismatch(io.RawIOBase, pyio.RawIOBase)
        self.assertEqual(mismatch, set(), msg='C RawIOBase does not have all Python RawIOBase methods')


class CommonBufferedTests:
    # Tests common to BufferedReader, BufferedWriter and BufferedRandom

    def test_detach(self):
        raw = self.MockRawIO()
        buf = self.tp(raw)
        self.assertIs(buf.detach(), raw)
        self.assertRaises(ValueError, buf.detach)

        repr(buf)  # Should still work

    def test_fileno(self):
        rawio = self.MockRawIO()
        bufio = self.tp(rawio)

        self.assertEqual(42, bufio.fileno())

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
        del bufio
        support.gc_collect()
        self.assertEqual(record, [1, 2, 3])

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
        with support.catch_unraisable_exception() as cm:
            with self.assertRaises(AttributeError):
                self.tp(rawio).xyzzy

            self.assertEqual(cm.unraisable.exc_type, OSError)

    def test_repr(self):
        raw = self.MockRawIO()
        b = self.tp(raw)
        clsname = r"(%s\.)?%s" % (self.tp.__module__, self.tp.__qualname__)
        self.assertRegex(repr(b), "<%s>" % clsname)
        raw.name = "dummy"
        self.assertRegex(repr(b), "<%s name='dummy'>" % clsname)
        raw.name = b"dummy"
        self.assertRegex(repr(b), "<%s name=b'dummy'>" % clsname)

    def test_recursive_repr(self):
        # Issue #25455
        raw = self.MockRawIO()
        b = self.tp(raw)
        with support.swap_attr(raw, 'name', b), support.infinite_recursion(25):
            with self.assertRaises(RuntimeError):
                repr(b)  # Should not crash

    def test_flush_error_on_close(self):
        # Test that buffered file is closed despite failed flush
        # and that flush() is called before file closed.
        raw = self.MockRawIO()
        closed = []
        def bad_flush():
            closed[:] = [b.closed, raw.closed]
            raise OSError()
        raw.flush = bad_flush
        b = self.tp(raw)
        self.assertRaises(OSError, b.close) # exception not swallowed
        self.assertTrue(b.closed)
        self.assertTrue(raw.closed)
        self.assertTrue(closed)      # flush() called
        self.assertFalse(closed[0])  # flush() called before file closed
        self.assertFalse(closed[1])
        raw.flush = lambda: None  # break reference loop

    def test_close_error_on_close(self):
        raw = self.MockRawIO()
        def bad_flush():
            raise OSError('flush')
        def bad_close():
            raise OSError('close')
        raw.close = bad_close
        b = self.tp(raw)
        b.flush = bad_flush
        with self.assertRaises(OSError) as err: # exception not swallowed
            b.close()
        self.assertEqual(err.exception.args, ('close',))
        self.assertIsInstance(err.exception.__context__, OSError)
        self.assertEqual(err.exception.__context__.args, ('flush',))
        self.assertFalse(b.closed)

        # Silence destructor error
        raw.close = lambda: None
        b.flush = lambda: None

    def test_nonnormalized_close_error_on_close(self):
        # Issue #21677
        raw = self.MockRawIO()
        def bad_flush():
            raise non_existing_flush
        def bad_close():
            raise non_existing_close
        raw.close = bad_close
        b = self.tp(raw)
        b.flush = bad_flush
        with self.assertRaises(NameError) as err: # exception not swallowed
            b.close()
        self.assertIn('non_existing_close', str(err.exception))
        self.assertIsInstance(err.exception.__context__, NameError)
        self.assertIn('non_existing_flush', str(err.exception.__context__))
        self.assertFalse(b.closed)

        # Silence destructor error
        b.flush = lambda: None
        raw.close = lambda: None

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

    def test_pickling_subclass(self):
        global MyBufferedIO
        class MyBufferedIO(self.tp):
            def __init__(self, raw, tag):
                super().__init__(raw)
                self.tag = tag
            def __getstate__(self):
                return self.tag, self.raw.getvalue()
            def __setstate__(slf, state):
                tag, value = state
                slf.__init__(self.BytesIO(value), tag)

        raw = self.BytesIO(b'data')
        buf = MyBufferedIO(raw, tag='ham')
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(protocol=proto):
                pickled = pickle.dumps(buf, proto)
                newbuf = pickle.loads(pickled)
                self.assertEqual(newbuf.raw.getvalue(), b'data')
                self.assertEqual(newbuf.tag, 'ham')
        del MyBufferedIO


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

    @support.cpython_only
    def test_buffer_freeing(self) :
        bufsize = 4096
        rawio = self.MockRawIO()
        bufio = self.tp(rawio, buffer_size=bufsize)
        size = sys.getsizeof(bufio) - bufsize
        bufio.close()
        self.assertEqual(sys.getsizeof(bufio), size)

class BufferedReaderTest(CommonBufferedTests):
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

    def test_uninitialized(self):
        bufio = self.tp.__new__(self.tp)
        del bufio
        bufio = self.tp.__new__(self.tp)
        self.assertRaisesRegex((ValueError, AttributeError),
                               'uninitialized|has no attribute',
                               bufio.read, 0)
        bufio.__init__(self.MockRawIO())
        self.assertEqual(bufio.read(0), b'')

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
        self.assertEqual(b"", bufio.read1(0))
        self.assertEqual(b"c", bufio.read1(100))
        self.assertEqual(rawio._reads, 1)
        self.assertEqual(b"d", bufio.read1(100))
        self.assertEqual(rawio._reads, 2)
        self.assertEqual(b"efg", bufio.read1(100))
        self.assertEqual(rawio._reads, 3)
        self.assertEqual(b"", bufio.read1(100))
        self.assertEqual(rawio._reads, 4)

    def test_read1_arbitrary(self):
        rawio = self.MockRawIO((b"abc", b"d", b"efg"))
        bufio = self.tp(rawio)
        self.assertEqual(b"a", bufio.read(1))
        self.assertEqual(b"bc", bufio.read1())
        self.assertEqual(b"d", bufio.read1())
        self.assertEqual(b"efg", bufio.read1(-1))
        self.assertEqual(rawio._reads, 3)
        self.assertEqual(b"", bufio.read1())
        self.assertEqual(rawio._reads, 4)

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

    def test_readinto1(self):
        buffer_size = 10
        rawio = self.MockRawIO((b"abc", b"de", b"fgh", b"jkl"))
        bufio = self.tp(rawio, buffer_size=buffer_size)
        b = bytearray(2)
        self.assertEqual(bufio.peek(3), b'abc')
        self.assertEqual(rawio._reads, 1)
        self.assertEqual(bufio.readinto1(b), 2)
        self.assertEqual(b, b"ab")
        self.assertEqual(rawio._reads, 1)
        self.assertEqual(bufio.readinto1(b), 1)
        self.assertEqual(b[:1], b"c")
        self.assertEqual(rawio._reads, 1)
        self.assertEqual(bufio.readinto1(b), 2)
        self.assertEqual(b, b"de")
        self.assertEqual(rawio._reads, 2)
        b = bytearray(2*buffer_size)
        self.assertEqual(bufio.peek(3), b'fgh')
        self.assertEqual(rawio._reads, 3)
        self.assertEqual(bufio.readinto1(b), 6)
        self.assertEqual(b[:6], b"fghjkl")
        self.assertEqual(rawio._reads, 4)

    def test_readinto_array(self):
        buffer_size = 60
        data = b"a" * 26
        rawio = self.MockRawIO((data,))
        bufio = self.tp(rawio, buffer_size=buffer_size)

        # Create an array with element size > 1 byte
        b = array.array('i', b'x' * 32)
        assert len(b) != 16

        # Read into it. We should get as many *bytes* as we can fit into b
        # (which is more than the number of elements)
        n = bufio.readinto(b)
        self.assertGreater(n, len(b))

        # Check that old contents of b are preserved
        bm = memoryview(b).cast('B')
        self.assertLess(n, len(bm))
        self.assertEqual(bm[:n], data[:n])
        self.assertEqual(bm[n:], b'x' * (len(bm[n:])))

    def test_readinto1_array(self):
        buffer_size = 60
        data = b"a" * 26
        rawio = self.MockRawIO((data,))
        bufio = self.tp(rawio, buffer_size=buffer_size)

        # Create an array with element size > 1 byte
        b = array.array('i', b'x' * 32)
        assert len(b) != 16

        # Read into it. We should get as many *bytes* as we can fit into b
        # (which is more than the number of elements)
        n = bufio.readinto1(b)
        self.assertGreater(n, len(b))

        # Check that old contents of b are preserved
        bm = memoryview(b).cast('B')
        self.assertLess(n, len(bm))
        self.assertEqual(bm[:n], data[:n])
        self.assertEqual(bm[n:], b'x' * (len(bm[n:])))

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

    @threading_helper.requires_working_threading()
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
            with self.open(os_helper.TESTFN, "wb") as f:
                f.write(s)
            with self.open(os_helper.TESTFN, self.read_mode, buffering=0) as raw:
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
                with threading_helper.start_threads(threads):
                    time.sleep(0.02) # yield
                self.assertFalse(errors,
                    "the following exceptions were caught: %r" % errors)
                s = b''.join(results)
                for i in range(256):
                    c = bytes(bytearray([i]))
                    self.assertEqual(s.count(c), N)
        finally:
            os_helper.unlink(os_helper.TESTFN)

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
        self.assertRaises(OSError, bufio.seek, 0)
        self.assertRaises(OSError, bufio.tell)

        # Silence destructor error
        bufio.close = lambda: None

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

    def test_read_on_closed(self):
        # Issue #23796
        b = self.BufferedReader(self.BytesIO(b"12"))
        b.read(1)
        b.close()
        with self.subTest('peek'):
            self.assertRaises(ValueError, b.peek)
        with self.subTest('read1'):
            self.assertRaises(ValueError, b.read1, 1)
        with self.subTest('read'):
            self.assertRaises(ValueError, b.read)
        with self.subTest('readinto'):
            self.assertRaises(ValueError, b.readinto, bytearray())
        with self.subTest('readinto1'):
            self.assertRaises(ValueError, b.readinto1, bytearray())
        with self.subTest('flush'):
            self.assertRaises(ValueError, b.flush)
        with self.subTest('truncate'):
            self.assertRaises(ValueError, b.truncate)
        with self.subTest('seek'):
            self.assertRaises(ValueError, b.seek, 0)

    def test_truncate_on_read_only(self):
        rawio = self.MockFileIO(b"abc")
        bufio = self.tp(rawio)
        self.assertFalse(bufio.writable())
        self.assertRaises(self.UnsupportedOperation, bufio.truncate)
        self.assertRaises(self.UnsupportedOperation, bufio.truncate, 0)

    def test_tell_character_device_file(self):
        # GH-95782
        # For the (former) bug in BufferedIO to manifest, the wrapped IO obj
        # must be able to produce at least 2 bytes.
        raw = self.MockCharPseudoDevFileIO(b"12")
        buf = self.tp(raw)
        self.assertEqual(buf.tell(), 0)
        self.assertEqual(buf.read(1), b"1")
        self.assertEqual(buf.tell(), 0)

    def test_seek_character_device_file(self):
        raw = self.MockCharPseudoDevFileIO(b"12")
        buf = self.tp(raw)
        self.assertEqual(buf.seek(0, io.SEEK_CUR), 0)
        self.assertEqual(buf.seek(1, io.SEEK_SET), 0)
        self.assertEqual(buf.seek(0, io.SEEK_CUR), 0)
        self.assertEqual(buf.read(1), b"1")

        # In the C implementation, tell() sets the BufferedIO's abs_pos to 0,
        # which means that the next seek() could return a negative offset if it
        # does not sanity-check:
        self.assertEqual(buf.tell(), 0)
        self.assertEqual(buf.seek(0, io.SEEK_CUR), 0)


class CBufferedReaderTest(BufferedReaderTest, SizeofTest, CTestCase):
    tp = io.BufferedReader

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
        self.assertRaises(OSError, bufio.read, 10)

    def test_garbage_collection(self):
        # C BufferedReader objects are collected.
        # The Python version has __del__, so it ends into gc.garbage instead
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        # Note that using warnings_helper.check_warnings() will keep the
        # file alive due to the `source` argument to warn().  So, use
        # catch_warnings() instead.
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ResourceWarning)
            rawio = self.FileIO(os_helper.TESTFN, "w+b")
            f = self.tp(rawio)
            f.f = f
            wr = weakref.ref(f)
            del f
            support.gc_collect()
        self.assertIsNone(wr(), wr)

    def test_args_error(self):
        # Issue #17275
        with self.assertRaisesRegex(TypeError, "BufferedReader"):
            self.tp(self.BytesIO(), 1024, 1024, 1024)

    def test_bad_readinto_value(self):
        rawio = self.tp(self.BytesIO(b"12"))
        rawio.readinto = lambda buf: -1
        bufio = self.tp(rawio)
        with self.assertRaises(OSError) as cm:
            bufio.readline()
        self.assertIsNone(cm.exception.__cause__)

    def test_bad_readinto_type(self):
        rawio = self.tp(self.BytesIO(b"12"))
        rawio.readinto = lambda buf: b''
        bufio = self.tp(rawio)
        with self.assertRaises(OSError) as cm:
            bufio.readline()
        self.assertIsInstance(cm.exception.__cause__, TypeError)


class PyBufferedReaderTest(BufferedReaderTest, PyTestCase):
    tp = pyio.BufferedReader


class BufferedWriterTest(CommonBufferedTests):
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

    def test_uninitialized(self):
        bufio = self.tp.__new__(self.tp)
        del bufio
        bufio = self.tp.__new__(self.tp)
        self.assertRaisesRegex((ValueError, AttributeError),
                               'uninitialized|has no attribute',
                               bufio.write, b'')
        bufio.__init__(self.MockRawIO())
        self.assertEqual(bufio.write(b''), 0)

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
        buffer = bytearray(b"def")
        bufio.write(buffer)
        buffer[:] = b"***"  # Overwrite our copy of the data
        bufio.flush()
        self.assertEqual(b"".join(writer._write_stack), b"abcdef")

    def test_write_overflow(self):
        writer = self.MockRawIO()
        bufio = self.tp(writer, 8)
        contents = b"abcdefghijklmnop"
        for n in range(0, len(contents), 3):
            bufio.write(contents[n:n+3])
        flushed = b"".join(writer._write_stack)
        # At least (total - 8) bytes were implicitly flushed, perhaps more
        # depending on the implementation.
        self.assertStartsWith(flushed, contents[:-8])

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
        self.assertStartsWith(s, b"01234567A")

    def test_write_and_rewind(self):
        raw = self.BytesIO()
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
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with self.open(os_helper.TESTFN, self.write_mode, buffering=0) as raw:
            bufio = self.tp(raw, 8)
            bufio.write(b"abcdef")
            self.assertEqual(bufio.truncate(3), 3)
            self.assertEqual(bufio.tell(), 6)
        with self.open(os_helper.TESTFN, "rb", buffering=0) as f:
            self.assertEqual(f.read(), b"abc")

    def test_truncate_after_write(self):
        # Ensure that truncate preserves the file position after
        # writes longer than the buffer size.
        # Issue: https://bugs.python.org/issue32228
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with self.open(os_helper.TESTFN, "wb") as f:
            # Fill with some buffer
            f.write(b'\x00' * 10000)
        buffer_sizes = [8192, 4096, 200]
        for buffer_size in buffer_sizes:
            with self.open(os_helper.TESTFN, "r+b", buffering=buffer_size) as f:
                f.write(b'\x00' * (buffer_size + 1))
                # After write write_pos and write_end are set to 0
                f.read(1)
                # read operation makes sure that pos != raw_pos
                f.truncate()
                self.assertEqual(f.tell(), buffer_size + 2)

    @threading_helper.requires_working_threading()
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
            with self.open(os_helper.TESTFN, self.write_mode, buffering=0) as raw:
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
                with threading_helper.start_threads(threads):
                    time.sleep(0.02) # yield
                self.assertFalse(errors,
                    "the following exceptions were caught: %r" % errors)
                bufio.close()
            with self.open(os_helper.TESTFN, "rb") as f:
                s = f.read()
            for i in range(256):
                self.assertEqual(s.count(bytes([i])), N)
        finally:
            os_helper.unlink(os_helper.TESTFN)

    def test_misbehaved_io(self):
        rawio = self.MisbehavedRawIO()
        bufio = self.tp(rawio, 5)
        self.assertRaises(OSError, bufio.seek, 0)
        self.assertRaises(OSError, bufio.tell)
        self.assertRaises(OSError, bufio.write, b"abcdef")

        # Silence destructor error
        bufio.close = lambda: None

    def test_max_buffer_size_removal(self):
        with self.assertRaises(TypeError):
            self.tp(self.MockRawIO(), 8, 12)

    def test_write_error_on_close(self):
        raw = self.MockRawIO()
        def bad_write(b):
            raise OSError()
        raw.write = bad_write
        b = self.tp(raw)
        b.write(b'spam')
        self.assertRaises(OSError, b.close) # exception not swallowed
        self.assertTrue(b.closed)

    @threading_helper.requires_working_threading()
    def test_slow_close_from_thread(self):
        # Issue #31976
        rawio = self.SlowFlushRawIO()
        bufio = self.tp(rawio, 8)
        t = threading.Thread(target=bufio.close)
        t.start()
        rawio.in_flush.wait()
        self.assertRaises(ValueError, bufio.write, b'spam')
        self.assertTrue(bufio.closed)
        t.join()


class CBufferedWriterTest(BufferedWriterTest, SizeofTest, CTestCase):
    tp = io.BufferedWriter

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
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        # Note that using warnings_helper.check_warnings() will keep the
        # file alive due to the `source` argument to warn().  So, use
        # catch_warnings() instead.
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ResourceWarning)
            rawio = self.FileIO(os_helper.TESTFN, "w+b")
            f = self.tp(rawio)
            f.write(b"123xxx")
            f.x = f
            wr = weakref.ref(f)
            del f
            support.gc_collect()
        self.assertIsNone(wr(), wr)
        with self.open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"123xxx")

    def test_args_error(self):
        # Issue #17275
        with self.assertRaisesRegex(TypeError, "BufferedWriter"):
            self.tp(self.BytesIO(), 1024, 1024, 1024)


class PyBufferedWriterTest(BufferedWriterTest, PyTestCase):
    tp = pyio.BufferedWriter

class BufferedRWPairTest:

    def test_constructor(self):
        pair = self.tp(self.MockRawIO(), self.MockRawIO())
        self.assertFalse(pair.closed)

    def test_uninitialized(self):
        pair = self.tp.__new__(self.tp)
        del pair
        pair = self.tp.__new__(self.tp)
        self.assertRaisesRegex((ValueError, AttributeError),
                               'uninitialized|has no attribute',
                               pair.read, 0)
        self.assertRaisesRegex((ValueError, AttributeError),
                               'uninitialized|has no attribute',
                               pair.write, b'')
        pair.__init__(self.MockRawIO(), self.MockRawIO())
        self.assertEqual(pair.read(0), b'')
        self.assertEqual(pair.write(b''), 0)

    def test_detach(self):
        pair = self.tp(self.MockRawIO(), self.MockRawIO())
        self.assertRaises(self.UnsupportedOperation, pair.detach)

    def test_constructor_max_buffer_size_removal(self):
        with self.assertRaises(TypeError):
            self.tp(self.MockRawIO(), self.MockRawIO(), 8, 12)

    def test_constructor_with_not_readable(self):
        class NotReadable(self.MockRawIO):
            def readable(self):
                return False

        self.assertRaises(OSError, self.tp, NotReadable(), self.MockRawIO())

    def test_constructor_with_not_writeable(self):
        class NotWriteable(self.MockRawIO):
            def writable(self):
                return False

        self.assertRaises(OSError, self.tp, self.MockRawIO(), NotWriteable())

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
        self.assertEqual(pair.read1(), b"def")

    def test_readinto(self):
        for method in ("readinto", "readinto1"):
            with self.subTest(method):
                pair = self.tp(self.BytesIO(b"abcdef"), self.MockRawIO())

                data = byteslike(b'\0' * 5)
                self.assertEqual(getattr(pair, method)(data), 5)
                self.assertEqual(bytes(data), b"abcde")

        # gh-138720: C BufferedRWPair would destruct in a bad order resulting in
        # an unraisable exception.
        support.gc_collect()

    def test_write(self):
        w = self.MockRawIO()
        pair = self.tp(self.MockRawIO(), w)

        pair.write(b"abc")
        pair.flush()
        buffer = bytearray(b"def")
        pair.write(buffer)
        buffer[:] = b"***"  # Overwrite our copy of the data
        pair.flush()
        self.assertEqual(w._write_stack, [b"abc", b"def"])

    def test_peek(self):
        pair = self.tp(self.BytesIO(b"abcdef"), self.MockRawIO())

        self.assertStartsWith(pair.peek(3), b"abc")
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

    def test_reader_close_error_on_close(self):
        def reader_close():
            reader_non_existing
        reader = self.MockRawIO()
        reader.close = reader_close
        writer = self.MockRawIO()
        pair = self.tp(reader, writer)
        with self.assertRaises(NameError) as err:
            pair.close()
        self.assertIn('reader_non_existing', str(err.exception))
        self.assertTrue(pair.closed)
        self.assertFalse(reader.closed)
        self.assertTrue(writer.closed)

        # Silence destructor error
        reader.close = lambda: None

    def test_writer_close_error_on_close(self):
        def writer_close():
            writer_non_existing
        reader = self.MockRawIO()
        writer = self.MockRawIO()
        writer.close = writer_close
        pair = self.tp(reader, writer)
        with self.assertRaises(NameError) as err:
            pair.close()
        self.assertIn('writer_non_existing', str(err.exception))
        self.assertFalse(pair.closed)
        self.assertTrue(reader.closed)
        self.assertFalse(writer.closed)

        # Silence destructor error
        writer.close = lambda: None
        writer = None

        # Ignore BufferedWriter (of the BufferedRWPair) unraisable exception
        with support.catch_unraisable_exception():
            # Ignore BufferedRWPair unraisable exception
            with support.catch_unraisable_exception():
                pair = None
                support.gc_collect()
            support.gc_collect()

    def test_reader_writer_close_error_on_close(self):
        def reader_close():
            reader_non_existing
        def writer_close():
            writer_non_existing
        reader = self.MockRawIO()
        reader.close = reader_close
        writer = self.MockRawIO()
        writer.close = writer_close
        pair = self.tp(reader, writer)
        with self.assertRaises(NameError) as err:
            pair.close()
        self.assertIn('reader_non_existing', str(err.exception))
        self.assertIsInstance(err.exception.__context__, NameError)
        self.assertIn('writer_non_existing', str(err.exception.__context__))
        self.assertFalse(pair.closed)
        self.assertFalse(reader.closed)
        self.assertFalse(writer.closed)

        # Silence destructor error
        reader.close = lambda: None
        writer.close = lambda: None

    def test_isatty(self):
        class SelectableIsAtty(self.MockRawIO):
            def __init__(self, isatty):
                super().__init__()
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

    def test_weakref_clearing(self):
        brw = self.tp(self.MockRawIO(), self.MockRawIO())
        ref = weakref.ref(brw)
        brw = None
        ref = None # Shouldn't segfault.

class CBufferedRWPairTest(BufferedRWPairTest, CTestCase):
    tp = io.BufferedRWPair

class PyBufferedRWPairTest(BufferedRWPairTest, PyTestCase):
    tp = pyio.BufferedRWPair


class BufferedRandomTest(BufferedReaderTest, BufferedWriterTest):
    read_mode = "rb+"
    write_mode = "wb+"

    def test_constructor(self):
        BufferedReaderTest.test_constructor(self)
        BufferedWriterTest.test_constructor(self)

    def test_uninitialized(self):
        BufferedReaderTest.test_uninitialized(self)
        BufferedWriterTest.test_uninitialized(self)

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

    def test_read1_after_write(self):
        with self.BytesIO(b'abcdef') as raw:
            with self.tp(raw, 3) as f:
                f.write(b"1")
                self.assertEqual(f.read1(1), b'b')
                f.flush()
                self.assertEqual(raw.getvalue(), b'1bcdef')
        with self.BytesIO(b'abcdef') as raw:
            with self.tp(raw, 3) as f:
                f.write(b"1")
                self.assertEqual(f.read1(), b'bcd')
                f.flush()
                self.assertEqual(raw.getvalue(), b'1bcdef')
        with self.BytesIO(b'abcdef') as raw:
            with self.tp(raw, 3) as f:
                f.write(b"1")
                # XXX: read(100) returns different numbers of bytes
                # in Python and C implementations.
                self.assertEqual(f.read1(100)[:3], b'bcd')
                f.flush()
                self.assertEqual(raw.getvalue(), b'1bcdef')

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

    # writable() returns True, so there's no point to test it over
    # a writable stream.
    test_truncate_on_read_only = None


class CBufferedRandomTest(BufferedRandomTest, SizeofTest, CTestCase):
    tp = io.BufferedRandom

    def test_garbage_collection(self):
        CBufferedReaderTest.test_garbage_collection(self)
        CBufferedWriterTest.test_garbage_collection(self)

    def test_args_error(self):
        # Issue #17275
        with self.assertRaisesRegex(TypeError, "BufferedRandom"):
            self.tp(self.BytesIO(), 1024, 1024, 1024)


class PyBufferedRandomTest(BufferedRandomTest, PyTestCase):
    tp = pyio.BufferedRandom


# XXX Tests for open()

class MiscIOTest:

    # for test__all__, actual values are set in subclasses
    name_of_module = None
    extra_exported = ()
    not_exported = ()

    def tearDown(self):
        os_helper.unlink(os_helper.TESTFN)

    def test___all__(self):
        support.check__all__(self, self.io, self.name_of_module,
                             extra=self.extra_exported,
                             not_exported=self.not_exported)

    def test_attributes(self):
        f = self.open(os_helper.TESTFN, "wb", buffering=0)
        self.assertEqual(f.mode, "wb")
        f.close()

        f = self.open(os_helper.TESTFN, "w+", encoding="utf-8")
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

    def test_removed_u_mode(self):
        # bpo-37330: The "U" mode has been removed in Python 3.11
        for mode in ("U", "rU", "r+U"):
            with self.assertRaises(ValueError) as cm:
                self.open(os_helper.TESTFN, mode)
            self.assertIn('invalid mode', str(cm.exception))

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_open_pipe_with_append(self):
        # bpo-27805: Ignore ESPIPE from lseek() in open().
        r, w = os.pipe()
        self.addCleanup(os.close, r)
        f = self.open(w, 'a', encoding="utf-8")
        self.addCleanup(f.close)
        # Check that the file is marked non-seekable. On Windows, however, lseek
        # somehow succeeds on pipes.
        if sys.platform != 'win32':
            self.assertFalse(f.seekable())

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
            if "b" not in kwargs["mode"]:
                kwargs["encoding"] = "utf-8"
            f = self.open(os_helper.TESTFN, **kwargs)
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
                self.assertRaises(ValueError, f.read1)
            if hasattr(f, "readall"):
                self.assertRaises(ValueError, f.readall)
            if hasattr(f, "readinto"):
                self.assertRaises(ValueError, f.readinto, bytearray(1024))
            if hasattr(f, "readinto1"):
                self.assertRaises(ValueError, f.readinto1, bytearray(1024))
            self.assertRaises(ValueError, f.readline)
            self.assertRaises(ValueError, f.readlines)
            self.assertRaises(ValueError, f.readlines, 1)
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
        self.assertIsNone(wr(), wr)

    def test_abcs(self):
        # Test the visible base classes are ABCs.
        self.assertIsInstance(self.IOBase, abc.ABCMeta)
        self.assertIsInstance(self.RawIOBase, abc.ABCMeta)
        self.assertIsInstance(self.BufferedIOBase, abc.ABCMeta)
        self.assertIsInstance(self.TextIOBase, abc.ABCMeta)

    def _check_abc_inheritance(self, abcmodule):
        with self.open(os_helper.TESTFN, "wb", buffering=0) as f:
            self.assertIsInstance(f, abcmodule.IOBase)
            self.assertIsInstance(f, abcmodule.RawIOBase)
            self.assertNotIsInstance(f, abcmodule.BufferedIOBase)
            self.assertNotIsInstance(f, abcmodule.TextIOBase)
        with self.open(os_helper.TESTFN, "wb") as f:
            self.assertIsInstance(f, abcmodule.IOBase)
            self.assertNotIsInstance(f, abcmodule.RawIOBase)
            self.assertIsInstance(f, abcmodule.BufferedIOBase)
            self.assertNotIsInstance(f, abcmodule.TextIOBase)
        with self.open(os_helper.TESTFN, "w", encoding="utf-8") as f:
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
        f = self.open(*args, **kwargs)
        r = repr(f)
        with self.assertWarns(ResourceWarning) as cm:
            f = None
            support.gc_collect()
        self.assertIn(r, str(cm.warning.args[0]))

    def test_warn_on_dealloc(self):
        self._check_warn_on_dealloc(os_helper.TESTFN, "wb", buffering=0)
        self._check_warn_on_dealloc(os_helper.TESTFN, "wb")
        self._check_warn_on_dealloc(os_helper.TESTFN, "w", encoding="utf-8")

    def _check_warn_on_dealloc_fd(self, *args, **kwargs):
        fds = []
        def cleanup_fds():
            for fd in fds:
                try:
                    os.close(fd)
                except OSError as e:
                    if e.errno != errno.EBADF:
                        raise
        self.addCleanup(cleanup_fds)
        r, w = os.pipe()
        fds += r, w
        self._check_warn_on_dealloc(r, *args, **kwargs)
        # When using closefd=False, there's no warning
        r, w = os.pipe()
        fds += r, w
        with warnings_helper.check_no_resource_warning(self):
            self.open(r, *args, closefd=False, **kwargs)

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_warn_on_dealloc_fd(self):
        self._check_warn_on_dealloc_fd("rb", buffering=0)
        self._check_warn_on_dealloc_fd("rb")
        self._check_warn_on_dealloc_fd("r", encoding="utf-8")


    def test_pickling(self):
        # Pickling file objects is forbidden
        msg = "cannot pickle"
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
            if "b" not in kwargs["mode"]:
                kwargs["encoding"] = "utf-8"
            for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(protocol=protocol, kwargs=kwargs):
                    with self.open(os_helper.TESTFN, **kwargs) as f:
                        with self.assertRaisesRegex(TypeError, msg):
                            pickle.dumps(f, protocol)

    @unittest.skipIf(support.is_emscripten, "Emscripten corrupts memory when writing to nonblocking fd")
    def test_nonblock_pipe_write_bigbuf(self):
        self._test_nonblock_pipe_write(16*1024)

    @unittest.skipIf(support.is_emscripten, "Emscripten corrupts memory when writing to nonblocking fd")
    def test_nonblock_pipe_write_smallbuf(self):
        self._test_nonblock_pipe_write(1024)

    @unittest.skipUnless(hasattr(os, 'set_blocking'),
                         'os.set_blocking() required for this test')
    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def _test_nonblock_pipe_write(self, bufsize):
        sent = []
        received = []
        r, w = os.pipe()
        os.set_blocking(r, False)
        os.set_blocking(w, False)

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
        self.assertEqual(sent, received)
        self.assertTrue(wf.closed)
        self.assertTrue(rf.closed)

    def test_create_fail(self):
        # 'x' mode fails if file is existing
        with self.open(os_helper.TESTFN, 'w', encoding="utf-8"):
            pass
        self.assertRaises(FileExistsError, self.open, os_helper.TESTFN, 'x', encoding="utf-8")

    def test_create_writes(self):
        # 'x' mode opens for writing
        with self.open(os_helper.TESTFN, 'xb') as f:
            f.write(b"spam")
        with self.open(os_helper.TESTFN, 'rb') as f:
            self.assertEqual(b"spam", f.read())

    def test_open_allargs(self):
        # there used to be a buffer overflow in the parser for rawmode
        self.assertRaises(ValueError, self.open, os_helper.TESTFN, 'rwax+', encoding="utf-8")

    def test_check_encoding_errors(self):
        # bpo-37388: open() and TextIOWrapper must check encoding and errors
        # arguments in dev mode
        mod = self.io.__name__
        filename = __file__
        invalid = 'Boom, Shaka Laka, Boom!'
        code = textwrap.dedent(f'''
            import sys
            from {mod} import open, TextIOWrapper

            try:
                open({filename!r}, encoding={invalid!r})
            except LookupError:
                pass
            else:
                sys.exit(21)

            try:
                open({filename!r}, errors={invalid!r})
            except LookupError:
                pass
            else:
                sys.exit(22)

            fp = open({filename!r}, "rb")
            with fp:
                try:
                    TextIOWrapper(fp, encoding={invalid!r})
                except LookupError:
                    pass
                else:
                    sys.exit(23)

                try:
                    TextIOWrapper(fp, errors={invalid!r})
                except LookupError:
                    pass
                else:
                    sys.exit(24)

            sys.exit(10)
        ''')
        proc = assert_python_failure('-X', 'dev', '-c', code)
        self.assertEqual(proc.rc, 10, proc)

    def test_check_encoding_warning(self):
        # PEP 597: Raise warning when encoding is not specified
        # and sys.flags.warn_default_encoding is set.
        mod = self.io.__name__
        filename = __file__
        code = textwrap.dedent(f'''\
            import sys
            from {mod} import open, TextIOWrapper
            import pathlib

            with open({filename!r}) as f:           # line 5
                pass

            pathlib.Path({filename!r}).read_text()  # line 8
        ''')
        proc = assert_python_ok('-X', 'warn_default_encoding', '-c', code)
        warnings = proc.err.splitlines()
        self.assertEqual(len(warnings), 2)
        self.assertStartsWith(warnings[0], b"<string>:5: EncodingWarning: ")
        self.assertStartsWith(warnings[1], b"<string>:8: EncodingWarning: ")

    def test_text_encoding(self):
        # PEP 597, bpo-47000. io.text_encoding() returns "locale" or "utf-8"
        # based on sys.flags.utf8_mode
        code = "import io; print(io.text_encoding(None))"

        proc = assert_python_ok('-X', 'utf8=0', '-c', code)
        self.assertEqual(b"locale", proc.out.strip())

        proc = assert_python_ok('-X', 'utf8=1', '-c', code)
        self.assertEqual(b"utf-8", proc.out.strip())


class CMiscIOTest(MiscIOTest, CTestCase):
    name_of_module = "io", "_io"
    extra_exported = "BlockingIOError",

    def test_readinto_buffer_overflow(self):
        # Issue #18025
        class BadReader(self.io.BufferedIOBase):
            def read(self, n=-1):
                return b'x' * 10**6
        bufio = BadReader()
        b = bytearray(2)
        self.assertRaises(ValueError, bufio.readinto, b)

    def check_daemon_threads_shutdown_deadlock(self, stream_name):
        # Issue #23309: deadlocks at shutdown should be avoided when a
        # daemon thread and the main thread both write to a file.
        code = """if 1:
            import sys
            import time
            import threading
            from test.support import SuppressCrashReport

            file = sys.{stream_name}

            def run():
                while True:
                    file.write('.')
                    file.flush()

            crash = SuppressCrashReport()
            crash.__enter__()
            # don't call __exit__(): the crash occurs at Python shutdown

            thread = threading.Thread(target=run)
            thread.daemon = True
            thread.start()

            time.sleep(0.5)
            file.write('!')
            file.flush()
            """.format_map(locals())
        res, _ = run_python_until_end("-c", code)
        err = res.err.decode()
        if res.rc != 0:
            # Failure: should be a fatal error
            pattern = (r"Fatal Python error: _enter_buffered_busy: "
                       r"could not acquire lock "
                       r"for <(_io\.)?BufferedWriter name='<{stream_name}>'> "
                       r"at interpreter shutdown, possibly due to "
                       r"daemon threads".format_map(locals()))
            self.assertRegex(err, pattern)
        else:
            self.assertFalse(err.strip('.!'))

    @threading_helper.requires_working_threading()
    @support.requires_resource('walltime')
    def test_daemon_threads_shutdown_stdout_deadlock(self):
        self.check_daemon_threads_shutdown_deadlock('stdout')

    @threading_helper.requires_working_threading()
    @support.requires_resource('walltime')
    def test_daemon_threads_shutdown_stderr_deadlock(self):
        self.check_daemon_threads_shutdown_deadlock('stderr')


class PyMiscIOTest(MiscIOTest, PyTestCase):
    name_of_module = "_pyio", "io"
    extra_exported = "BlockingIOError", "open_code",
    not_exported = "valid_seek_flags",


class ProtocolsTest(unittest.TestCase):
    class MyReader:
        def read(self, sz=-1):
            return b""

    class MyWriter:
        def write(self, b: bytes):
            pass

    def test_reader_subclass(self):
        self.assertIsSubclass(self.MyReader, io.Reader)
        self.assertNotIsSubclass(str, io.Reader)

    def test_writer_subclass(self):
        self.assertIsSubclass(self.MyWriter, io.Writer)
        self.assertNotIsSubclass(str, io.Writer)


if __name__ == "__main__":
    unittest.main()
