"""Unit tests for the memoryview

XXX We need more tests! Some tests are in test_bytes
"""

import unittest
import sys
import gc
import weakref
import array
from test import test_support


class AbstractMemoryTests:
    source_bytes = b"abcdef"

    @property
    def _source(self):
        return self.source_bytes

    @property
    def _types(self):
        return filter(None, [self.ro_type, self.rw_type])

    def check_getitem_with_type(self, tp):
        item = self.getitem_type
        b = tp(self._source)
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        self.assertEqual(m[0], item(b"a"))
        self.assertIsInstance(m[0], bytes)
        self.assertEqual(m[5], item(b"f"))
        self.assertEqual(m[-1], item(b"f"))
        self.assertEqual(m[-6], item(b"a"))
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

    def test_repr(self):
        for tp in self._types:
            b = tp(self._source)
            m = self._view(b)
            self.assertIsInstance(m.__repr__(), str)

    def test_setitem_readonly(self):
        if not self.ro_type:
            return
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
            return
        tp = self.rw_type
        b = self.rw_type(self._source)
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        m[0] = tp(b"0")
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
        self.assertRaises(TypeError, setitem, "a", b"a")
        # Trying to resize the memory object
        self.assertRaises(ValueError, setitem, 0, b"")
        self.assertRaises(ValueError, setitem, 0, b"ab")
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
                self.getitem_type(c) for c in b"abcdef")
            self.assertEqual(b, expected)
            self.assertIsInstance(b, bytes)

    def test_tolist(self):
        for tp in self._types:
            m = self._view(tp(self._source))
            l = m.tolist()
            self.assertEqual(l, map(ord, b"abcdef"))

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
            self.assertFalse(m == u"abcdef")
            self.assertTrue(m != u"abcdef")
            self.assertFalse(u"abcdef" == m)
            self.assertTrue(u"abcdef" != m)

            # Unordered comparisons are unimplemented, and therefore give
            # arbitrary results (they raise a TypeError in py3k)

    def check_attributes_with_type(self, tp):
        m = self._view(tp(self._source))
        self.assertEqual(m.format, self.format)
        self.assertIsInstance(m.format, str)
        self.assertEqual(m.itemsize, self.itemsize)
        self.assertEqual(m.ndim, 1)
        self.assertEqual(m.shape, (6,))
        self.assertEqual(len(m), 6)
        self.assertEqual(m.strides, (self.itemsize,))
        self.assertEqual(m.suboffsets, None)
        return m

    def test_attributes_readonly(self):
        if not self.ro_type:
            return
        m = self.check_attributes_with_type(self.ro_type)
        self.assertEqual(m.readonly, True)

    def test_attributes_writable(self):
        if not self.rw_type:
            return
        m = self.check_attributes_with_type(self.rw_type)
        self.assertEqual(m.readonly, False)

    # Disabled: unicode uses the old buffer API in 2.x

    #def test_getbuffer(self):
        ## Test PyObject_GetBuffer() on a memoryview object.
        #for tp in self._types:
            #b = tp(self._source)
            #oldrefcount = sys.getrefcount(b)
            #m = self._view(b)
            #oldviewrefcount = sys.getrefcount(m)
            #s = unicode(m, "utf-8")
            #self._check_contents(tp, b, s.encode("utf-8"))
            #self.assertEqual(sys.getrefcount(m), oldviewrefcount)
            #m = None
            #self.assertEqual(sys.getrefcount(b), oldrefcount)

    def test_gc(self):
        for tp in self._types:
            if not isinstance(tp, type):
                # If tp is a factory rather than a plain type, skip
                continue

            class MySource(tp):
                pass
            class MyObject:
                pass

            # Create a reference cycle through a memoryview object
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


# Variations on source objects for the buffer: bytes-like objects, then arrays
# with itemsize > 1.
# NOTE: support for multi-dimensional objects is unimplemented.

class BaseBytesMemoryTests(AbstractMemoryTests):
    ro_type = bytes
    rw_type = bytearray
    getitem_type = bytes
    itemsize = 1
    format = 'B'

# Disabled: array.array() does not support the new buffer API in 2.x

#class BaseArrayMemoryTests(AbstractMemoryTests):
    #ro_type = None
    #rw_type = lambda self, b: array.array('i', map(ord, b))
    #getitem_type = lambda self, b: array.array('i', map(ord, b)).tostring()
    #itemsize = array.array('i').itemsize
    #format = 'i'

    #def test_getbuffer(self):
        ## XXX Test should be adapted for non-byte buffers
        #pass

    #def test_tolist(self):
        ## XXX NotImplementedError: tolist() only supports byte views
        #pass


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

#class ArrayMemoryviewTest(unittest.TestCase,
    #BaseMemoryviewTests, BaseArrayMemoryTests):

    #def test_array_assign(self):
        ## Issue #4569: segfault when mutating a memoryview with itemsize != 1
        #a = array.array('i', range(10))
        #m = memoryview(a)
        #new_a = array.array('i', range(9, -1, -1))
        #m[:] = new_a
        #self.assertEqual(a, new_a)


class BytesMemorySliceTest(unittest.TestCase,
    BaseMemorySliceTests, BaseBytesMemoryTests):
    pass

#class ArrayMemorySliceTest(unittest.TestCase,
    #BaseMemorySliceTests, BaseArrayMemoryTests):
    #pass

class BytesMemorySliceSliceTest(unittest.TestCase,
    BaseMemorySliceSliceTests, BaseBytesMemoryTests):
    pass

#class ArrayMemorySliceSliceTest(unittest.TestCase,
    #BaseMemorySliceSliceTests, BaseArrayMemoryTests):
    #pass


def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
