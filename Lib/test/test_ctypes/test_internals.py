# This tests the internal _objects attribute

# XXX This test must be reviewed for correctness!!!

# ctypes' types are container types.
#
# They have an internal memory block, which only consists of some bytes,
# but it has to keep references to other objects as well. This is not
# really needed for trivial C types like int or char, but it is important
# for aggregate types like strings or pointers in particular.
#
# What about pointers?

import sys
import unittest
from ctypes import Structure, POINTER, c_char_p, c_int


class ObjectsTestCase(unittest.TestCase):
    def assertSame(self, a, b):
        self.assertEqual(id(a), id(b))

    def test_ints(self):
        i = 42000123
        refcnt = sys.getrefcount(i)
        ci = c_int(i)
        self.assertEqual(refcnt, sys.getrefcount(i))
        self.assertEqual(ci._objects, None)

    def test_c_char_p(self):
        s = "Hello, World".encode("ascii")
        refcnt = sys.getrefcount(s)
        cs = c_char_p(s)
        self.assertEqual(refcnt + 1, sys.getrefcount(s))
        self.assertSame(cs._objects, s)

    def test_simple_struct(self):
        class X(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]

        a = 421234
        b = 421235
        x = X()
        self.assertEqual(x._objects, None)
        x.a = a
        x.b = b
        self.assertEqual(x._objects, None)

    def test_embedded_structs(self):
        class X(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]

        class Y(Structure):
            _fields_ = [("x", X), ("y", X)]

        y = Y()
        self.assertEqual(y._objects, None)

        x1, x2 = X(), X()
        y.x, y.y = x1, x2
        self.assertEqual(y._objects, {"0": {}, "1": {}})
        x1.a, x2.b = 42, 93
        self.assertEqual(y._objects, {"0": {}, "1": {}})

    def test_xxx(self):
        class X(Structure):
            _fields_ = [("a", c_char_p), ("b", c_char_p)]

        class Y(Structure):
            _fields_ = [("x", X), ("y", X)]

        s1 = b"Hello, World"
        s2 = b"Hallo, Welt"

        x = X()
        x.a = s1
        x.b = s2
        self.assertEqual(x._objects, {"0": s1, "1": s2})

        y = Y()
        y.x = x
        self.assertEqual(y._objects, {"0": {"0": s1, "1": s2}})

    def test_ptr_struct(self):
        class X(Structure):
            _fields_ = [("data", POINTER(c_int))]

        A = c_int*4
        a = A(11, 22, 33, 44)
        self.assertEqual(a._objects, None)

        x = X()
        x.data = a


if __name__ == '__main__':
    unittest.main()
