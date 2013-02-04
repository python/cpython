import unittest
from ctypes import *

import _ctypes_test

class ReturnFuncPtrTestCase(unittest.TestCase):

    def test_with_prototype(self):
        # The _ctypes_test shared lib/dll exports quite some functions for testing.
        # The get_strchr function returns a *pointer* to the C strchr function.
        dll = CDLL(_ctypes_test.__file__)
        get_strchr = dll.get_strchr
        get_strchr.restype = CFUNCTYPE(c_char_p, c_char_p, c_char)
        strchr = get_strchr()
        self.assertEqual(strchr("abcdef", "b"), "bcdef")
        self.assertEqual(strchr("abcdef", "x"), None)
        self.assertRaises(ArgumentError, strchr, "abcdef", 3)
        self.assertRaises(TypeError, strchr, "abcdef")

    def test_without_prototype(self):
        dll = CDLL(_ctypes_test.__file__)
        get_strchr = dll.get_strchr
        # the default 'c_int' would not work on systems where sizeof(int) != sizeof(void *)
        get_strchr.restype = c_void_p
        addr = get_strchr()
        # _CFuncPtr instances are now callable with an integer argument
        # which denotes a function address:
        strchr = CFUNCTYPE(c_char_p, c_char_p, c_char)(addr)
        self.assertTrue(strchr("abcdef", "b"), "bcdef")
        self.assertEqual(strchr("abcdef", "x"), None)
        self.assertRaises(ArgumentError, strchr, "abcdef", 3)
        self.assertRaises(TypeError, strchr, "abcdef")

    def test_from_dll(self):
        dll = CDLL(_ctypes_test.__file__)
        # _CFuncPtr instances are now callable with a tuple argument
        # which denotes a function name and a dll:
        strchr = CFUNCTYPE(c_char_p, c_char_p, c_char)(("strchr", dll))
        self.assertTrue(strchr(b"abcdef", b"b"), "bcdef")
        self.assertEqual(strchr(b"abcdef", b"x"), None)
        self.assertRaises(ArgumentError, strchr, b"abcdef", 3.0)
        self.assertRaises(TypeError, strchr, b"abcdef")

    # Issue 6083: Reference counting bug
    def test_test_from_dll_refcount(self):
        class BadSequence(tuple):
            def __getitem__(self, key):
                if key == 0:
                    return "strchr"
                if key == 1:
                    return CDLL(_ctypes_test.__file__)
                raise IndexError

        # _CFuncPtr instances are now callable with a tuple argument
        # which denotes a function name and a dll:
        strchr = CFUNCTYPE(c_char_p, c_char_p, c_char)(BadSequence(("strchr", CDLL(_ctypes_test.__file__))))
        self.assertTrue(strchr(b"abcdef", b"b"), "bcdef")
        self.assertEqual(strchr(b"abcdef", b"x"), None)
        self.assertRaises(ArgumentError, strchr, b"abcdef", 3.0)
        self.assertRaises(TypeError, strchr, b"abcdef")

if __name__ == "__main__":
    unittest.main()
