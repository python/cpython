import unittest
from ctypes import *

import _ctypes_test

lib = CDLL(_ctypes_test.__file__)

class StringPtrTestCase(unittest.TestCase):

    def test__POINTER_c_char(self):
        class X(Structure):
            _fields_ = [("str", POINTER(c_char))]
        x = X()

        # NULL pointer access
        self.assertRaises(ValueError, getattr, x.str, "contents")
        b = c_buffer("Hello, World")
        from sys import getrefcount as grc
        self.failUnlessEqual(grc(b), 2)
        x.str = b
        self.failUnlessEqual(grc(b), 3)

        # POINTER(c_char) and Python string is NOT compatible
        # POINTER(c_char) and c_buffer() is compatible
        for i in range(len(b)):
            self.failUnlessEqual(b[i], x.str[i])

        self.assertRaises(TypeError, setattr, x, "str", "Hello, World")

    def test__c_char_p(self):
        class X(Structure):
            _fields_ = [("str", c_char_p)]
        x = X()

        # c_char_p and Python string is compatible
        # c_char_p and c_buffer is NOT compatible
        self.failUnlessEqual(x.str, None)
        x.str = "Hello, World"
        self.failUnlessEqual(x.str, "Hello, World")
        b = c_buffer("Hello, World")
        self.failUnlessRaises(TypeError, setattr, x, "str", b)


    def test_functions(self):
        strchr = lib.my_strchr
        strchr.restype = c_char_p

        # c_char_p and Python string is compatible
        # c_char_p and c_buffer are now compatible
        strchr.argtypes = c_char_p, c_char
        self.failUnlessEqual(strchr("abcdef", "c"), "cdef")
        self.failUnlessEqual(strchr(c_buffer("abcdef"), "c"), "cdef")

        # POINTER(c_char) and Python string is NOT compatible
        # POINTER(c_char) and c_buffer() is compatible
        strchr.argtypes = POINTER(c_char), c_char
        buf = c_buffer("abcdef")
        self.failUnlessEqual(strchr(buf, "c"), "cdef")
        self.failUnlessEqual(strchr("abcdef", "c"), "cdef")

        # XXX These calls are dangerous, because the first argument
        # to strchr is no longer valid after the function returns!
        # So we must keep a reference to buf separately

        strchr.restype = POINTER(c_char)
        buf = c_buffer("abcdef")
        r = strchr(buf, "c")
        x = r[0], r[1], r[2], r[3], r[4]
        self.failUnlessEqual(x, ("c", "d", "e", "f", "\000"))
        del buf
        # x1 will NOT be the same as x, usually:
        x1 = r[0], r[1], r[2], r[3], r[4]

if __name__ == '__main__':
    unittest.main()
