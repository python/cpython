"""Test where byte objects are accepted"""
import unittest
import sys
from ctypes import *

class BytesTest(unittest.TestCase):
    def test_c_char(self):
        x = c_char(b"x")
        self.assertRaises(TypeError, c_char, "x")
        x.value = b"y"
        with self.assertRaises(TypeError):
            x.value = "y"
        c_char.from_param(b"x")
        self.assertRaises(TypeError, c_char.from_param, "x")
        self.assertIn('xbd', repr(c_char.from_param(b"\xbd")))
        (c_char * 3)(b"a", b"b", b"c")
        self.assertRaises(TypeError, c_char * 3, "a", "b", "c")

    def test_c_wchar(self):
        x = c_wchar("x")
        self.assertRaises(TypeError, c_wchar, b"x")
        x.value = "y"
        with self.assertRaises(TypeError):
            x.value = b"y"
        c_wchar.from_param("x")
        self.assertRaises(TypeError, c_wchar.from_param, b"x")
        (c_wchar * 3)("a", "b", "c")
        self.assertRaises(TypeError, c_wchar * 3, b"a", b"b", b"c")

    def test_c_char_p(self):
        c_char_p(b"foo bar")
        self.assertRaises(TypeError, c_char_p, "foo bar")

    def test_c_wchar_p(self):
        c_wchar_p("foo bar")
        self.assertRaises(TypeError, c_wchar_p, b"foo bar")

    def test_struct(self):
        class X(Structure):
            _fields_ = [("a", c_char * 3)]

        x = X(b"abc")
        self.assertRaises(TypeError, X, "abc")
        self.assertEqual(x.a, b"abc")
        self.assertEqual(type(x.a), bytes)

    def test_struct_W(self):
        class X(Structure):
            _fields_ = [("a", c_wchar * 3)]

        x = X("abc")
        self.assertRaises(TypeError, X, b"abc")
        self.assertEqual(x.a, "abc")
        self.assertEqual(type(x.a), str)

    @unittest.skipUnless(sys.platform == "win32", 'Windows-specific test')
    def test_BSTR(self):
        from _ctypes import _SimpleCData
        class BSTR(_SimpleCData):
            _type_ = "X"

        BSTR("abc")


if __name__ == '__main__':
    unittest.main()
