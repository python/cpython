import unittest
from ctypes import *

import _ctypes_test

class SlicesTestCase(unittest.TestCase):
    def test_getslice_cint(self):
        a = (c_int * 100)(*range(1100, 1200))
        b = list(range(1100, 1200))
        self.failUnlessEqual(a[0:2], b[0:2])
        self.failUnlessEqual(len(a), len(b))
        self.failUnlessEqual(a[5:7], b[5:7])
        self.failUnlessEqual(a[-1], b[-1])
        self.failUnlessEqual(a[:], b[:])

        a[0:5] = range(5, 10)
        self.failUnlessEqual(a[0:5], list(range(5, 10)))

    def test_setslice_cint(self):
        a = (c_int * 100)(*range(1100, 1200))
        b = list(range(1100, 1200))

        a[32:47] = list(range(32, 47))
        self.failUnlessEqual(a[32:47], list(range(32, 47)))

        from operator import setslice

        # TypeError: int expected instead of str instance
        self.assertRaises(TypeError, setslice, a, 0, 5, "abcde")
        # TypeError: int expected instead of str instance
        self.assertRaises(TypeError, setslice, a, 0, 5, ["a", "b", "c", "d", "e"])
        # TypeError: int expected instead of float instance
        self.assertRaises(TypeError, setslice, a, 0, 5, [1, 2, 3, 4, 3.14])
        # ValueError: Can only assign sequence of same size
        self.assertRaises(ValueError, setslice, a, 0, 5, range(32))

    def test_char_ptr(self):
        s = b"abcdefghijklmnopqrstuvwxyz"

        dll = CDLL(_ctypes_test.__file__)
        dll.my_strdup.restype = POINTER(c_char)
        dll.my_free.restype = None
        res = dll.my_strdup(s)
        self.failUnlessEqual(res[:len(s)], s)

        import operator
        self.assertRaises(TypeError, operator.setslice,
                          res, 0, 5, "abcde")
        dll.my_free(res)

        dll.my_strdup.restype = POINTER(c_byte)
        res = dll.my_strdup(s)
        self.failUnlessEqual(res[:len(s)], list(range(ord("a"), ord("z")+1)))
        dll.my_free(res)

    def test_char_ptr_with_free(self):
        dll = CDLL(_ctypes_test.__file__)
        s = b"abcdefghijklmnopqrstuvwxyz"

        class allocated_c_char_p(c_char_p):
            pass

        dll.my_free.restype = None
        def errcheck(result, func, args):
            retval = result.value
            dll.my_free(result)
            return retval

        dll.my_strdup.restype = allocated_c_char_p
        dll.my_strdup.errcheck = errcheck
        try:
            res = dll.my_strdup(s)
            self.failUnlessEqual(res, str(s))
        finally:
            del dll.my_strdup.errcheck


    def test_char_array(self):
        s = "abcdefghijklmnopqrstuvwxyz\0"

        p = (c_char * 27)(*s)
        self.failUnlessEqual(p[:], s)


    try:
        c_wchar
    except NameError:
        pass
    else:
        def test_wchar_ptr(self):
            s = "abcdefghijklmnopqrstuvwxyz\0"

            dll = CDLL(_ctypes_test.__file__)
            dll.my_wcsdup.restype = POINTER(c_wchar)
            dll.my_wcsdup.argtypes = POINTER(c_wchar),
            dll.my_free.restype = None
            res = dll.my_wcsdup(s)
            self.failUnlessEqual(res[:len(s)], s)

            import operator
            self.assertRaises(TypeError, operator.setslice,
                              res, 0, 5, "abcde")
            dll.my_free(res)

            if sizeof(c_wchar) == sizeof(c_short):
                dll.my_wcsdup.restype = POINTER(c_short)
            elif sizeof(c_wchar) == sizeof(c_int):
                dll.my_wcsdup.restype = POINTER(c_int)
            elif sizeof(c_wchar) == sizeof(c_long):
                dll.my_wcsdup.restype = POINTER(c_long)
            else:
                return
            res = dll.my_wcsdup(s)
            self.failUnlessEqual(res[:len(s)-1],
                                 list(range(ord("a"), ord("z")+1)))
            dll.my_free(res)

################################################################

if __name__ == "__main__":
    unittest.main()
