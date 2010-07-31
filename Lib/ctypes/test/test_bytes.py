"""Test where byte objects are accepted"""
import unittest
import sys
from ctypes import *

class BytesTest(unittest.TestCase):
    def test_c_char(self):
        x = c_char(b"x")
        x.value = b"y"
        c_char.from_param(b"x")
        (c_char * 3)(b"a", b"b", b"c")

    def test_c_wchar(self):
        x = c_wchar("x")
        x.value = "y"
        c_wchar.from_param("x")
        (c_wchar * 3)("a", "b", "c")

    def test_c_char_p(self):
        c_char_p(b"foo bar")

    def test_c_wchar_p(self):
        c_wchar_p("foo bar")

    def test_struct(self):
        class X(Structure):
            _fields_ = [("a", c_char * 3)]

        x = X(b"abc")
        self.assertEqual(x.a, b"abc")
        self.assertEqual(type(x.a), bytes)

    def test_struct_W(self):
        class X(Structure):
            _fields_ = [("a", c_wchar * 3)]

        x = X("abc")
        self.assertEqual(x.a, "abc")
        self.assertEqual(type(x.a), str)

    if sys.platform == "win32":
        def test_BSTR(self):
            from _ctypes import _SimpleCData
            class BSTR(_SimpleCData):
                _type_ = "X"

            BSTR("abc")

if __name__ == '__main__':
    unittest.main()
