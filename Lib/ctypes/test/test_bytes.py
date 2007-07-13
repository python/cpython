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
        x = c_wchar(b"x")
        x.value = b"y"
        c_wchar.from_param(b"x")
        (c_wchar * 3)(b"a", b"b", b"c")

    def test_c_char_p(self):
        c_char_p("foo bar")
        c_char_p(b"foo bar")

    def test_c_wchar_p(self):
        c_wchar_p("foo bar")
        c_wchar_p(b"foo bar")

    def test_struct(self):
        class X(Structure):
            _fields_ = [("a", c_char * 3)]

        X("abc")
        X(b"abc")

    def test_struct_W(self):
        class X(Structure):
            _fields_ = [("a", c_wchar * 3)]

        X("abc")
        X(b"abc")

    if sys.platform == "win32":
        def test_BSTR(self):
            from _ctypes import _SimpleCData
            class BSTR(_SimpleCData):
                _type_ = "X"

            BSTR("abc")
            BSTR(b"abc")

if __name__ == '__main__':
    unittest.main()
