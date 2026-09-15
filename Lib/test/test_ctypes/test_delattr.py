import unittest
from ctypes import CDLL, POINTER, Structure, c_char, c_int
from test.support import import_helper


class X(Structure):
    _fields_ = [("foo", c_int)]


class TestCase(unittest.TestCase):
    def test_simple(self):
        with self.assertRaises(TypeError):
            del c_int(42).value

    def test_chararray(self):
        chararray = (c_char * 5)()
        with self.assertRaises(TypeError):
            del chararray.value

    def test_pointer_contents(self):
        ptr = POINTER(c_int)(c_int(42))
        with self.assertRaises(TypeError):
            del ptr.contents

    def test_struct(self):
        struct = X()
        with self.assertRaises(TypeError):
            del struct.foo

    def test_raw(self):
        chararray = (c_char * 5)()
        with self.assertRaises(AttributeError):
            del chararray.raw

    def test_func_pointer(self):
        # Deleting these attributes restores the default.
        dll = CDLL(import_helper.import_module('_ctypes_test').__file__)
        func = dll._testfunc_i_bhilfd
        func.argtypes = [c_int]
        func.restype = c_int
        func.errcheck = lambda *args: None
        del func.argtypes
        self.assertIsNone(func.argtypes)
        del func.errcheck
        self.assertIsNone(func.errcheck)
        del func.restype
        self.assertIs(func.restype, c_int)


if __name__ == "__main__":
    unittest.main()
