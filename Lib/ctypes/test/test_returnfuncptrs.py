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
        self.failUnlessEqual(strchr("abcdef", "b"), "bcdef")
        self.failUnlessEqual(strchr("abcdef", "x"), None)
        self.failUnlessEqual(strchr("abcdef", 98), "bcdef")
        self.failUnlessEqual(strchr("abcdef", 107), None)
        self.assertRaises(ArgumentError, strchr, "abcdef", 3.0)
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
        self.failUnless(strchr("abcdef", "b"), "bcdef")
        self.failUnlessEqual(strchr("abcdef", "x"), None)
        self.assertRaises(ArgumentError, strchr, "abcdef", 3.0)
        self.assertRaises(TypeError, strchr, "abcdef")

if __name__ == "__main__":
    unittest.main()
