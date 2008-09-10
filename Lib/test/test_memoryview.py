"""Unit tests for the memoryview

XXX We need more tests! Some tests are in test_bytes
"""

import unittest
import test.support
import sys
import gc
import weakref


class CommonMemoryTests:
    #
    # Tests common to direct memoryviews and sliced memoryviews
    #

    base_object = b"abcdef"

    def check_getitem_with_type(self, tp):
        b = tp(self.base_object)
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        self.assertEquals(m[0], b"a")
        self.assert_(isinstance(m[0], bytes), type(m[0]))
        self.assertEquals(m[5], b"f")
        self.assertEquals(m[-1], b"f")
        self.assertEquals(m[-6], b"a")
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
        self.assertEquals(sys.getrefcount(b), oldrefcount)

    def test_getitem_readonly(self):
        self.check_getitem_with_type(bytes)

    def test_getitem_writable(self):
        self.check_getitem_with_type(bytearray)

    def test_setitem_readonly(self):
        b = self.base_object
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        def setitem(value):
            m[0] = value
        self.assertRaises(TypeError, setitem, b"a")
        self.assertRaises(TypeError, setitem, 65)
        self.assertRaises(TypeError, setitem, memoryview(b"a"))
        m = None
        self.assertEquals(sys.getrefcount(b), oldrefcount)

    def test_setitem_writable(self):
        b = bytearray(self.base_object)
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        m[0] = b"0"
        self._check_contents(b, b"0bcdef")
        m[1:3] = b"12"
        self._check_contents(b, b"012def")
        m[1:1] = b""
        self._check_contents(b, b"012def")
        m[:] = b"abcdef"
        self._check_contents(b, b"abcdef")

        # Overlapping copies of a view into itself
        m[0:3] = m[2:5]
        self._check_contents(b, b"cdedef")
        m[:] = b"abcdef"
        m[2:5] = m[0:3]
        self._check_contents(b, b"ababcf")

        def setitem(key, value):
            m[key] = value
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
        self.assertEquals(sys.getrefcount(b), oldrefcount)

    def test_len(self):
        self.assertEquals(len(self._view(self.base_object)), 6)

    def test_tobytes(self):
        m = self._view(self.base_object)
        b = m.tobytes()
        self.assertEquals(b, b"abcdef")
        self.assert_(isinstance(b, bytes), type(b))

    def test_tolist(self):
        m = self._view(self.base_object)
        l = m.tolist()
        self.assertEquals(l, list(b"abcdef"))

    def test_compare(self):
        # memoryviews can compare for equality with other objects
        # having the buffer interface.
        m = self._view(self.base_object)
        for tp in (bytes, bytearray):
            self.assertTrue(m == tp(b"abcdef"))
            self.assertFalse(m != tp(b"abcdef"))
            self.assertFalse(m == tp(b"abcde"))
            self.assertTrue(m != tp(b"abcde"))
            self.assertFalse(m == tp(b"abcde1"))
            self.assertTrue(m != tp(b"abcde1"))
        self.assertTrue(m == m)
        self.assertTrue(m == m[:])
        self.assertTrue(m[0:6] == m[:])
        self.assertFalse(m[0:5] == m)

        # Comparison with objects which don't support the buffer API
        self.assertFalse(m == "abc")
        self.assertTrue(m != "abc")
        self.assertFalse("abc" == m)
        self.assertTrue("abc" != m)

        # Unordered comparisons
        for c in (m, b"abcdef"):
            self.assertRaises(TypeError, lambda: m < c)
            self.assertRaises(TypeError, lambda: c <= m)
            self.assertRaises(TypeError, lambda: m >= c)
            self.assertRaises(TypeError, lambda: c > m)

    def check_attributes_with_type(self, tp):
        b = tp(self.base_object)
        m = self._view(b)
        self.assertEquals(m.format, 'B')
        self.assertEquals(m.itemsize, 1)
        self.assertEquals(m.ndim, 1)
        self.assertEquals(m.shape, (6,))
        self.assertEquals(len(m), 6)
        self.assertEquals(m.strides, (1,))
        self.assertEquals(m.suboffsets, None)
        return m

    def test_attributes_readonly(self):
        m = self.check_attributes_with_type(bytes)
        self.assertEquals(m.readonly, True)

    def test_attributes_writable(self):
        m = self.check_attributes_with_type(bytearray)
        self.assertEquals(m.readonly, False)

    def test_getbuffer(self):
        # Test PyObject_GetBuffer() on a memoryview object.
        b = self.base_object
        oldrefcount = sys.getrefcount(b)
        m = self._view(b)
        oldviewrefcount = sys.getrefcount(m)
        s = str(m, "utf-8")
        self._check_contents(b, s.encode("utf-8"))
        self.assertEquals(sys.getrefcount(m), oldviewrefcount)
        m = None
        self.assertEquals(sys.getrefcount(b), oldrefcount)

    def test_gc(self):
        class MyBytes(bytes):
            pass
        class MyObject:
            pass

        # Create a reference cycle through a memoryview object
        b = MyBytes(b'abc')
        m = self._view(b)
        o = MyObject()
        b.m = m
        b.o = o
        wr = weakref.ref(o)
        b = m = o = None
        # The cycle must be broken
        gc.collect()
        self.assert_(wr() is None, wr())


class MemoryviewTest(unittest.TestCase, CommonMemoryTests):

    def _view(self, obj):
        return memoryview(obj)

    def _check_contents(self, obj, contents):
        self.assertEquals(obj, contents)

    def test_constructor(self):
        ob = b'test'
        self.assert_(memoryview(ob))
        self.assert_(memoryview(object=ob))
        self.assertRaises(TypeError, memoryview)
        self.assertRaises(TypeError, memoryview, ob, ob)
        self.assertRaises(TypeError, memoryview, argument=ob)
        self.assertRaises(TypeError, memoryview, ob, argument=True)


class MemorySliceTest(unittest.TestCase, CommonMemoryTests):
    base_object = b"XabcdefY"

    def _view(self, obj):
        m = memoryview(obj)
        return m[1:7]

    def _check_contents(self, obj, contents):
        self.assertEquals(obj[1:7], contents)

    def test_refs(self):
        m = memoryview(b"ab")
        oldrefcount = sys.getrefcount(m)
        m[1:2]
        self.assertEquals(sys.getrefcount(m), oldrefcount)


class MemorySliceSliceTest(unittest.TestCase, CommonMemoryTests):
    base_object = b"XabcdefY"

    def _view(self, obj):
        m = memoryview(obj)
        return m[:7][1:]

    def _check_contents(self, obj, contents):
        self.assertEquals(obj[1:7], contents)


def test_main():
    test.support.run_unittest(
        MemoryviewTest, MemorySliceTest, MemorySliceSliceTest)


if __name__ == "__main__":
    test_main()
