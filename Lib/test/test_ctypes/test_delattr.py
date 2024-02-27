import unittest
from ctypes import Structure, c_char, c_int


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

    def test_struct(self):
        struct = X()
        with self.assertRaises(TypeError):
            del struct.foo


if __name__ == "__main__":
    unittest.main()
