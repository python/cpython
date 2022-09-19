"""Unit tests for the memoryview

   Some tests are in test_bytes. Many tests that require _testbuffer.ndarray
   are in test_buffer.
"""

import unittest
import test.support
import sys
import gc
import weakref
import array
import io
import copy
import pickle

from test.support import import_helper


class AbstractMemoryTests:
    source_bytes = b"abcdef"

    @property
    def _source(self):
        return self.source_bytes

    @property
    def _types(self):
        return filter(None, [self.ro_type, self.rw_type])

    def check_getitem_with_type(self, tp):
        b = tp(self._source)
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        self.assertEqual(m[0], ord(b"a"))
        self.assertIsInstance(m[0], int)
        self.assertEqual(m[5], ord(b"f"))
        self.assertEqual(m[-1], ord(b"f"))
        self.assertEqual(m[-6], ord(b"a"))
        # Bounds checking
        self.assertRaises(IndexError, lambda: m[6])
        self.assertRaises(IndexError, lambda: m[-7])
        self.assertRaises(IndexError, lambda: m[sys.maxsize])
        self.assertRaises(IndexError, lambda: m[-sys.maxsize])
        # Type checking
        self.assertRaises(TypeError, lambda: m[None])
        self.assertRaises(TypeError, lambda: m[0.0])
        self.assertRaises(TypeError, lambda: m["a"])
        m = None
        self.assertEqual(sys.getrefcount(b), oldrefcount)

    def test_getitem(self):
        for tp in self._types:
            self.check_getitem_with_type(tp)

    def test_iter(self):
        for tp in self._types:
            b = tp(self._source)
            m = self._view(b)
            self.assertEqual(list(m), [m[i] for i in range(len(m))])

    def test_setitem_readonly(self):
        if not self.ro_type:
            self.skipTest("no read-only type to test")
        b = self.ro_type(self._source)
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        def setitem(value):
            m[0] = value
        self.assertRaises(TypeError, setitem, b"a")
        self.assertRaises(TypeError, setitem, 65)
        self.assertRaises(TypeError, setitem, memoryview(b"a"))
        m = None
        self.assertEqual(sys.getrefcount(b), oldrefcount)

    def test_setitem_writable(self):
        if not self.rw_type:
            self.skipTest("no writable type to test")
        tp = self.rw_type
        b = self.rw_type(self._source)
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        m[0] = ord(b'1')
        self._check_contents(tp, b, b"1bcdef")
        m[0:1] = tp(b"0")
        self._check_contents(tp, b, b"0bcdef")
        m[1:3] = tp(b"12")
        self._check_contents(tp, b, b"012def")
        m[1:1] = tp(b"")
        self._check_contents(tp, b, b"012def")
        m[:] = tp(b"abcdef")
        self._check_contents(tp, b, b"abcdef")

        # Overlapping copies of a view into itself
        m[0:3] = m[2:5]
        self._check_contents(tp, b, b"cdedef")
        m[:] = tp(b"abcdef")
        m[2:5] = m[0:3]
        self._check_contents(tp, b, b"ababcf")

        def setitem(key, value):
            m[key] = tp(value)
        # Bounds checking
        self.assertRaises(IndexError, setitem, 6, b"a")
        self.assertRaises(IndexError, setitem, -7, b"a")
        self.assertRaises(IndexError, setitem, sys.maxsize, b"a")
        self.assertRaises(IndexError, setitem, -sys.maxsize, b"a")
        # Wrong index/slice types
        self.assertRaises(TypeError, setitem, 0.0, b"a")
        self.assertRaises(TypeError, setitem, (0,), b"a")
        self.assertRaises(TypeError, setitem, (slice(0,1,1), 0), b"a")
        self.assertRaises(TypeError, setitem, (0, slice(0,1,1)), b"a")
        self.assertRaises(TypeError, setitem, (0,), b"a")
        self.assertRaises(TypeError, setitem, "a", b"a")
        # Not implemented: multidimensional slices
        slices = (slice(0,1,1), slice(0,1,2))
        self.assertRaises(NotImplementedError, setitem, slices, b"a")
        # Trying to resize the memory object
        exc = ValueError if m.format == 'c' else TypeError
        self.assertRaises(exc, setitem, 0, b"")
        self.assertRaises(exc, setitem, 0, b"ab")
        self.assertRaises(ValueError, setitem, slice(1,1), b"a")
        self.assertRaises(ValueError, setitem, slice(0,2), b"a")

        m = None
        self.assertEqual(sys.getrefcount(b), oldrefcount)

    def test_delitem(self):
        for tp in self._types:
            b = tp(self._source)
            m = self._view(b)
            with self.assertRaises(TypeError):
                del m[1]
            with self.assertRaises(TypeError):
                del m[1:4]

    def test_tobytes(self):
        for tp in self._types:
            m = self._view(tp(self._source))
            b = m.tobytes()
            # This calls self.getitem_type() on each separate byte of b"abcdef"
            expected = b"".join(
                self.getitem_type(bytes([c])) for c in b"abcdef")
            self.assertEqual(b, expected)
            self.assertIsInstance(b, bytes)

    def test_tolist(self):
        for tp in self._types:
            m = self._view(tp(self._source))
            l = m.tolist()
            self.assertEqual(l, list(b"abcdef"))

    def test_compare(self):
        # memoryviews can compare for equality with other objects
        # having the buffer interface.
        for tp in self._types:
            m = self._view(tp(self._source))
            for tp_comp in self._types:
                self.assertTrue(m == tp_comp(b"abcdef"))
                self.assertFalse(m != tp_comp(b"abcdef"))
                self.assertFalse(m == tp_comp(b"abcde"))
                self.assertTrue(m != tp_comp(b"abcde"))
                self.assertFalse(m == tp_comp(b"abcde1"))
                self.assertTrue(m != tp_comp(b"abcde1"))
            self.assertTrue(m == m)
            self.assertTrue(m == m[:])
            self.assertTrue(m[0:6] == m[:])
            self.assertFalse(m[0:5] == m)

            # Comparison with objects which don't support the buffer API
            self.assertFalse(m == "abcdef")
            self.assertTrue(m != "abcdef")
            self.assertFalse("abcdef" == m)
            self.assertTrue("abcdef" != m)

            # Unordered comparisons
            for c in (m, b"abcdef"):
                self.assertRaises(TypeError, lambda: m < c)
                self.assertRaises(TypeError, lambda: c <= m)
                self.assertRaises(TypeError, lambda: m >= c)
                self.assertRaises(TypeError, lambda: c > m)

    def check_attributes_with_type(self, tp):
        m = self._view(tp(self._source))
        self.assertEqual(m.format, self.format)
        self.assertEqual(m.itemsize, self.itemsize)
        self.assertEqual(m.ndim, 1)
        self.assertEqual(m.shape, (6,))
        self.assertEqual(len(m), 6)
        self.assertEqual(m.strides, (self.itemsize,))
        self.assertEqual(m.suboffsets, ())
        return m

    def test_attributes_readonly(self):
        if not self.ro_type:
            self.skipTest("no read-only type to test")
        m = self.check_attributes_with_type(self.ro_type)
        self.assertEqual(m.readonly, True)

    def test_attributes_writable(self):
        if not self.rw_type:
            self.skipTest("no writable type to test")
        m = self.check_attributes_with_type(self.rw_type)
        self.assertEqual(m.readonly, False)

    def test_getbuffer(self):
        # Test PyObject_GetBuffer() on a memoryview object.
        for tp in self._types:
            b = tp(self._source)
            oldrefcount = sys.getrefcount(b)
            m = self._view(b)
            oldviewrefcount = sys.getrefcount(m)
            s = str(m, "utf-8")
            self._check_contents(tp, b, s.encode("utf-8"))
            self.assertEqual(sys.getrefcount(m), oldviewrefcount)
            m = None
            self.assertEqual(sys.getrefcount(b), oldrefcount)

    def test_gc(self):
        for tp in self._types:
            if not isinstance(tp, type):
                # If tp is a factory rather than a plain type, skip
                continue

            class MyView():
                def __init__(self, base):
                    self.m = memoryview(base)
            class MySource(tp):
                pass
            class MyObject:
                pass

            # Create a reference cycle through a memoryview object.
            # This exercises mbuf_clear().
            b = MySource(tp(b'abc'))
            m = self._view(b)
            o = MyObject()
            b.m = m
            b.o = o
            wr = weakref.ref(o)
            b = m = o = None
            # The cycle must be broken
            gc.collect()
            self.assertTrue(wr() is None, wr())

            # This exercises memory_clear().
            m = MyView(tp(b'abc'))
            o = MyObject()
            m.x = m
            m.o = o
            wr = weakref.ref(o)
            m = o = None
            # The cycle must be broken
            gc.collect()
            self.assertTrue(wr() is None, wr())

    def _check_released(self, m, tp):
        check = self.assertRaisesRegex(ValueError, "released")
        with check: bytes(m)
        with check: m.tobytes()
        with check: m.tolist()
        with check: m[0]
        with check: m[0] = b'x'
        with check: len(m)
        with check: m.format
        with check: m.itemsize
        with check: m.ndim
        with check: m.readonly
        with check: m.shape
        with check: m.strides
        with check:
            with m:
                pass
        # str() and repr() still function
        self.assertIn("released memory", str(m))
        self.assertIn("released memory", repr(m))
        self.assertEqual(m, m)
        self.assertNotEqual(m, memoryview(tp(self._source)))
        self.assertNotEqual(m, tp(self._source))

    def test_contextmanager(self):
        for tp in self._types:
            b = tp(self._source)
            m = self._view(b)
            with m as cm:
                self.assertIs(cm, m)
            self._check_released(m, tp)
            m = self._view(b)
            # Can release explicitly inside the context manager
            with m:
                m.release()

    def test_release(self):
        for tp in self._types:
            b = tp(self._source)
            m = self._view(b)
            m.release()
            self._check_released(m, tp)
            # Can be called a second time (it's a no-op)
            m.release()
            self._check_released(m, tp)

    def test_writable_readonly(self):
        # Issue #10451: memoryview incorrectly exposes a readonly
        # buffer as writable causing a segfault if using mmap
        tp = self.ro_type
        if tp is None:
            self.skipTest("no read-only type to test")
        b = tp(self._source)
        m = self._view(b)
        i = io.BytesIO(b'ZZZZ')
        self.assertRaises(TypeError, i.readinto, m)

    def test_getbuf_fail(self):
        self.assertRaises(TypeError, self._view, {})

    def test_hash(self):
        # Memoryviews of readonly (hashable) types are hashable, and they
        # hash as hash(obj.tobytes()).
        tp = self.ro_type
        if tp is None:
            self.skipTest("no read-only type to test")
        b = tp(self._source)
        m = self._view(b)
        self.assertEqual(hash(m), hash(b"abcdef"))
        # Releasing the memoryview keeps the stored hash value (as with weakrefs)
        m.release()
        self.assertEqual(hash(m), hash(b"abcdef"))
        # Hashing a memoryview for the first time after it is released
        # results in an error (as with weakrefs).
        m = self._view(b)
        m.release()
        self.assertRaises(ValueError, hash, m)

    def test_hash_writable(self):
        # Memoryviews of writable types are unhashable
        tp = self.rw_type
        if tp is None:
            self.skipTest("no writable type to test")
        b = tp(self._source)
        m = self._view(b)
        self.assertRaises(ValueError, hash, m)

    def test_weakref(self):
        # Check memoryviews are weakrefable
        for tp in self._types:
            b = tp(self._source)
            m = self._view(b)
            L = []
            def callback(wr, b=b):
                L.append(b)
            wr = weakref.ref(m, callback)
            self.assertIs(wr(), m)
            del m
            test.support.gc_collect()
            self.assertIs(wr(), None)
            self.assertIs(L[0], b)

    def test_reversed(self):
        for tp in self._types:
            b = tp(self._source)
            m = self._view(b)
            aslist = list(reversed(m.tolist()))
            self.assertEqual(list(reversed(m)), aslist)
            self.assertEqual(list(reversed(m)), list(m[::-1]))

    def test_toreadonly(self):
        for tp in self._types:
            b = tp(self._source)
            m = self._view(b)
            mm = m.toreadonly()
            self.assertTrue(mm.readonly)
            self.assertTrue(memoryview(mm).readonly)
            self.assertEqual(mm.tolist(), m.tolist())
            mm.release()
            m.tolist()

    def test_issue22668(self):
        a = array.array('H', [256, 256, 256, 256])
        x = memoryview(a)
        m = x.cast('B')
        b = m.cast('H')
        c = b[0:2]
        d = memoryview(b)

        del b

        self.assertEqual(c[0], 256)
        self.assertEqual(d[0], 256)
        self.assertEqual(c.format, "H")
        self.assertEqual(d.format, "H")

        _ = m.cast('I')
        self.assertEqual(c[0], 256)
        self.assertEqual(d[0], 256)
        self.assertEqual(c.format, "H")
        self.assertEqual(d.format, "H")


# Variations on source objects for the buffer: bytes-like objects, then arrays
# with itemsize > 1.
# NOTE: support for multi-dimensional objects is unimplemented.

class BaseBytesMemoryTests(AbstractMemoryTests):
    ro_type = bytes
    rw_type = bytearray
    getitem_type = bytes
    itemsize = 1
    format = 'B'

class BaseArrayMemoryTests(AbstractMemoryTests):
    ro_type = None
    rw_type = lambda self, b: array.array('i', list(b))
    getitem_type = lambda self, b: array.array('i', list(b)).tobytes()
    itemsize = array.array('i').itemsize
    format = 'i'

    @unittest.skip('XXX test should be adapted for non-byte buffers')
    def test_getbuffer(self):
        pass

    @unittest.skip('XXX NotImplementedError: tolist() only supports byte views')
    def test_tolist(self):
        pass


# Variations on indirection levels: memoryview, slice of memoryview,
# slice of slice of memoryview.
# This is important to test allocation subtleties.

class BaseMemoryviewTests:
    def _view(self, obj):
        return memoryview(obj)

    def _check_contents(self, tp, obj, contents):
        self.assertEqual(obj, tp(contents))

class BaseMemorySliceTests:
    source_bytes = b"XabcdefY"

    def _view(self, obj):
        m = memoryview(obj)
        return m[1:7]

    def _check_contents(self, tp, obj, contents):
        self.assertEqual(obj[1:7], tp(contents))

    def test_refs(self):
        for tp in self._types:
            m = memoryview(tp(self._source))
            oldrefcount = sys.getrefcount(m)
            m[1:2]
            self.assertEqual(sys.getrefcount(m), oldrefcount)

class BaseMemorySliceSliceTests:
    source_bytes = b"XabcdefY"

    def _view(self, obj):
        m = memoryview(obj)
        return m[:7][1:]

    def _check_contents(self, tp, obj, contents):
        self.assertEqual(obj[1:7], tp(contents))


# Concrete test classes

class BytesMemoryviewTest(unittest.TestCase,
    BaseMemoryviewTests, BaseBytesMemoryTests):

    def test_constructor(self):
        for tp in self._types:
            ob = tp(self._source)
            self.assertTrue(memoryview(ob))
            self.assertTrue(memoryview(object=ob))
            self.assertRaises(TypeError, memoryview)
            self.assertRaises(TypeError, memoryview, ob, ob)
            self.assertRaises(TypeError, memoryview, argument=ob)
            self.assertRaises(TypeError, memoryview, ob, argument=True)

class ArrayMemoryviewTest(unittest.TestCase,
    BaseMemoryviewTests, BaseArrayMemoryTests):

    def test_array_assign(self):
        # Issue #4569: segfault when mutating a memoryview with itemsize != 1
        a = array.array('i', range(10))
        m = memoryview(a)
        new_a = array.array('i', range(9, -1, -1))
        m[:] = new_a
        self.assertEqual(a, new_a)


class BytesMemorySliceTest(unittest.TestCase,
    BaseMemorySliceTests, BaseBytesMemoryTests):
    pass

class ArrayMemorySliceTest(unittest.TestCase,
    BaseMemorySliceTests, BaseArrayMemoryTests):
    pass

class BytesMemorySliceSliceTest(unittest.TestCase,
    BaseMemorySliceSliceTests, BaseBytesMemoryTests):
    pass

class ArrayMemorySliceSliceTest(unittest.TestCase,
    BaseMemorySliceSliceTests, BaseArrayMemoryTests):
    pass


class OtherTest(unittest.TestCase):
    def test_ctypes_cast(self):
        # Issue 15944: Allow all source formats when casting to bytes.
        ctypes = import_helper.import_module("ctypes")
        p6 = bytes(ctypes.c_double(0.6))

        d = ctypes.c_double()
        m = memoryview(d).cast("B")
        m[:2] = p6[:2]
        m[2:] = p6[2:]
        self.assertEqual(d.value, 0.6)

        for format in "Bbc":
            with self.subTest(format):
                d = ctypes.c_double()
                m = memoryview(d).cast(format)
                m[:2] = memoryview(p6).cast(format)[:2]
                m[2:] = memoryview(p6).cast(format)[2:]
                self.assertEqual(d.value, 0.6)

    def test_memoryview_hex(self):
        # Issue #9951: memoryview.hex() segfaults with non-contiguous buffers.
        x = b'0' * 200000
        m1 = memoryview(x)
        m2 = m1[::-1]
        self.assertEqual(m2.hex(), '30' * 200000)

    def test_copy(self):
        m = memoryview(b'abc')
        with self.assertRaises(TypeError):
            copy.copy(m)

    def test_pickle(self):
        m = memoryview(b'abc')
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises(TypeError):
                pickle.dumps(m, proto)

    def test_use_released_memory(self):
        # gh-92888: Previously it was possible to use a memoryview even after
        # backing buffer is freed in certain cases. This tests that those
        # cases raise an exception.
        size = 128
        def release():
            m.release()
            nonlocal ba
            ba = bytearray(size)
        class MyIndex:
            def __index__(self):
                release()
                return 4
        class MyFloat:
            def __float__(self):
                release()
                return 4.25
        class MyBool:
            def __bool__(self):
                release()
                return True

        ba = None
        m = memoryview(bytearray(b'\xff'*size))
        with self.assertRaises(ValueError):
            m[MyIndex()]

        ba = None
        m = memoryview(bytearray(b'\xff'*size))
        self.assertEqual(list(m[:MyIndex()]), [255] * 4)

        ba = None
        m = memoryview(bytearray(b'\xff'*size))
        self.assertEqual(list(m[MyIndex():8]), [255] * 4)

        ba = None
        m = memoryview(bytearray(b'\xff'*size)).cast('B', (64, 2))
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[MyIndex(), 0]

        ba = None
        m = memoryview(bytearray(b'\xff'*size)).cast('B', (2, 64))
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[0, MyIndex()]

        ba = None
        m = memoryview(bytearray(b'\xff'*size))
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[MyIndex()] = 42
        self.assertEqual(ba[:8], b'\0'*8)

        ba = None
        m = memoryview(bytearray(b'\xff'*size))
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[:MyIndex()] = b'spam'
        self.assertEqual(ba[:8], b'\0'*8)

        ba = None
        m = memoryview(bytearray(b'\xff'*size))
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[MyIndex():8] = b'spam'
        self.assertEqual(ba[:8], b'\0'*8)

        ba = None
        m = memoryview(bytearray(b'\xff'*size)).cast('B', (64, 2))
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[MyIndex(), 0] = 42
        self.assertEqual(ba[8:16], b'\0'*8)
        ba = None
        m = memoryview(bytearray(b'\xff'*size)).cast('B', (2, 64))
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[0, MyIndex()] = 42
        self.assertEqual(ba[:8], b'\0'*8)

        ba = None
        m = memoryview(bytearray(b'\xff'*size))
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[0] = MyIndex()
        self.assertEqual(ba[:8], b'\0'*8)

        for fmt in 'bhilqnBHILQN':
            with self.subTest(fmt=fmt):
                ba = None
                m = memoryview(bytearray(b'\xff'*size)).cast(fmt)
                with self.assertRaisesRegex(ValueError, "operation forbidden"):
                    m[0] = MyIndex()
                self.assertEqual(ba[:8], b'\0'*8)

        for fmt in 'fd':
            with self.subTest(fmt=fmt):
                ba = None
                m = memoryview(bytearray(b'\xff'*size)).cast(fmt)
                with self.assertRaisesRegex(ValueError, "operation forbidden"):
                    m[0] = MyFloat()
                self.assertEqual(ba[:8], b'\0'*8)

        ba = None
        m = memoryview(bytearray(b'\xff'*size)).cast('?')
        with self.assertRaisesRegex(ValueError, "operation forbidden"):
            m[0] = MyBool()
        self.assertEqual(ba[:8], b'\0'*8)

if __name__ == "__main__":
    unittest.main()
