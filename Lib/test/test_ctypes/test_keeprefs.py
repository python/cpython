import unittest
from ctypes import (Structure, POINTER, pointer,  _pointer_type_cache,
                    c_char_p, c_int)


class SimpleTestCase(unittest.TestCase):
    def test_cint(self):
        x = c_int()
        self.assertEqual(x._objects, None)
        x.value = 42
        self.assertEqual(x._objects, None)
        x = c_int(99)
        self.assertEqual(x._objects, None)

    def test_ccharp(self):
        x = c_char_p()
        self.assertEqual(x._objects, None)
        x.value = b"abc"
        self.assertEqual(x._objects, b"abc")
        x = c_char_p(b"spam")
        self.assertEqual(x._objects, b"spam")


class StructureTestCase(unittest.TestCase):
    def test_cint_struct(self):
        class X(Structure):
            _fields_ = [("a", c_int),
                        ("b", c_int)]

        x = X()
        self.assertEqual(x._objects, None)
        x.a = 42
        x.b = 99
        self.assertEqual(x._objects, None)

    def test_ccharp_struct(self):
        class X(Structure):
            _fields_ = [("a", c_char_p),
                        ("b", c_char_p)]
        x = X()
        self.assertEqual(x._objects, None)

        x.a = b"spam"
        x.b = b"foo"
        self.assertEqual(x._objects, {"0": b"spam", "1": b"foo"})

    def test_struct_struct(self):
        class POINT(Structure):
            _fields_ = [("x", c_int), ("y", c_int)]
        class RECT(Structure):
            _fields_ = [("ul", POINT), ("lr", POINT)]

        r = RECT()
        r.ul.x = 0
        r.ul.y = 1
        r.lr.x = 2
        r.lr.y = 3
        self.assertEqual(r._objects, None)

        r = RECT()
        pt = POINT(1, 2)
        r.ul = pt
        self.assertEqual(r._objects, {'0': {}})
        r.ul.x = 22
        r.ul.y = 44
        self.assertEqual(r._objects, {'0': {}})
        r.lr = POINT()
        self.assertEqual(r._objects, {'0': {}, '1': {}})


class ArrayTestCase(unittest.TestCase):
    def test_cint_array(self):
        INTARR = c_int * 3

        ia = INTARR()
        self.assertEqual(ia._objects, None)
        ia[0] = 1
        ia[1] = 2
        ia[2] = 3
        self.assertEqual(ia._objects, None)

        class X(Structure):
            _fields_ = [("x", c_int),
                        ("a", INTARR)]

        x = X()
        x.x = 1000
        x.a[0] = 42
        x.a[1] = 96
        self.assertEqual(x._objects, None)
        x.a = ia
        self.assertEqual(x._objects, {'1': {}})


class PointerTestCase(unittest.TestCase):
    def test_p_cint(self):
        i = c_int(42)
        x = pointer(i)
        self.assertEqual(x._objects, {'1': i})


class PointerToStructure(unittest.TestCase):
    def test(self):
        class POINT(Structure):
            _fields_ = [("x", c_int), ("y", c_int)]
        class RECT(Structure):
            _fields_ = [("a", POINTER(POINT)),
                        ("b", POINTER(POINT))]
        r = RECT()
        p1 = POINT(1, 2)

        r.a = pointer(p1)
        r.b = pointer(p1)

        r.a[0].x = 42
        r.a[0].y = 99

        # to avoid leaking when tests are run several times
        # clean up the types left in the cache.
        del _pointer_type_cache[POINT]


if __name__ == "__main__":
    unittest.main()
