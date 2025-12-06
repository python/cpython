import array
import pickle
import random
import sys
import threading
import time
import unittest
import warnings
import weakref
from collections import deque, UserList
from itertools import cycle, count
from test import support
from test.support import os_helper, threading_helper
from .utils import byteslike, CTestCase, PyTestCase


import io # C implementation.
import _pyio as pyio # Python implementation.


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

    def test_non_boolean_closed_attr(self):
        # gh-140650: check TypeError is raised
        class MockRawIOWithoutClosed(self.MockRawIO):
            closed = NotImplemented

        bufio = self.tp(MockRawIOWithoutClosed())
        self.assertRaises(TypeError, bufio.write, b"")
        self.assertRaises(TypeError, bufio.flush)
        self.assertRaises(TypeError, bufio.close)

    def test_closed_attr_raises(self):
        class MockRawIOClosedRaises(self.MockRawIO):
            @property
            def closed(self):
                raise ValueError("test")

        bufio = self.tp(MockRawIOClosedRaises())
        self.assertRaisesRegex(ValueError, "test", bufio.write, b"")
        self.assertRaisesRegex(ValueError, "test", bufio.flush)
        self.assertRaisesRegex(ValueError, "test", bufio.close)


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
            b.resize(n)
            return b.take_bytes()
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


# Simple test to ensure that optimizations in the IO library deliver the
# expected results.  For best testing, run this under a debug-build Python too
# (to exercise asserts in the C code).

lengths = list(range(1, 257)) + [512, 1000, 1024, 2048, 4096, 8192, 10000,
                                 16384, 32768, 65536, 1000000]

class BufferSizeTest:
    def try_one(self, s):
        # Write s + "\n" + s to file, then open it and ensure that successive
        # .readline()s deliver what we wrote.

        # Ensure we can open TESTFN for writing.
        os_helper.unlink(os_helper.TESTFN)

        # Since C doesn't guarantee we can write/read arbitrary bytes in text
        # files, use binary mode.
        f = self.open(os_helper.TESTFN, "wb")
        try:
            # write once with \n and once without
            f.write(s)
            f.write(b"\n")
            f.write(s)
            f.close()
            f = self.open(os_helper.TESTFN, "rb")
            line = f.readline()
            self.assertEqual(line, s + b"\n")
            line = f.readline()
            self.assertEqual(line, s)
            line = f.readline()
            self.assertFalse(line) # Must be at EOF
            f.close()
        finally:
            os_helper.unlink(os_helper.TESTFN)

    def drive_one(self, pattern):
        for length in lengths:
            # Repeat string 'pattern' as often as needed to reach total length
            # 'length'.  Then call try_one with that string, a string one larger
            # than that, and a string one smaller than that.  Try this with all
            # small sizes and various powers of 2, so we exercise all likely
            # stdio buffer sizes, and "off by one" errors on both sides.
            q, r = divmod(length, len(pattern))
            teststring = pattern * q + pattern[:r]
            self.assertEqual(len(teststring), length)
            self.try_one(teststring)
            self.try_one(teststring + b"x")
            self.try_one(teststring[:-1])

    def test_primepat(self):
        # A pattern with prime length, to avoid simple relationships with
        # stdio buffer sizes.
        self.drive_one(b"1234567890\00\01\02\03\04\05\06")

    def test_nullpat(self):
        self.drive_one(b'\0' * 1000)


class CBufferSizeTest(BufferSizeTest, unittest.TestCase):
    open = io.open

class PyBufferSizeTest(BufferSizeTest, unittest.TestCase):
    open = staticmethod(pyio.open)


if __name__ == "__main__":
    unittest.main()
