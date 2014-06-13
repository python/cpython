from ctypes import *
import unittest

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

class DeletePointerTestCase(unittest.TestCase):
    @unittest.skip('test disabled')
    def test_X(self):
        class X(Structure):
            _fields_ = [("p", POINTER(c_char_p))]
        x = X()
        i = c_char_p("abc def")
        from sys import getrefcount as grc
        print("2?", grc(i))
        x.p = pointer(i)
        print("3?", grc(i))
        for i in range(320):
            c_int(99)
            x.p[0]
        print(x.p[0])
##        del x
##        print "2?", grc(i)
##        del i
        import gc
        gc.collect()
        for i in range(320):
            c_int(99)
            x.p[0]
        print(x.p[0])
        print(x.p.contents)
##        print x._objects

        x.p[0] = "spam spam"
##        print x.p[0]
        print("+" * 42)
        print(x._objects)

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
##        from pprint import pprint as pp
##        pp(p1._objects)
##        pp(r._objects)

        r.a[0].x = 42
        r.a[0].y = 99

        # to avoid leaking when tests are run several times
        # clean up the types left in the cache.
        from ctypes import _pointer_type_cache
        del _pointer_type_cache[POINT]

if __name__ == "__main__":
    unittest.main()
