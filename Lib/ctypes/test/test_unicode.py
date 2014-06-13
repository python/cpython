# coding: latin-1
import unittest
import ctypes
from ctypes.test import need_symbol
import _ctypes_test

@need_symbol('c_wchar')
class UnicodeTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        dll = ctypes.CDLL(_ctypes_test.__file__)
        cls.wcslen = dll.my_wcslen
        cls.wcslen.argtypes = [ctypes.c_wchar_p]
    def setUp(self):
        self.prev_conv_mode = ctypes.set_conversion_mode("ascii", "strict")

    def tearDown(self):
        ctypes.set_conversion_mode(*self.prev_conv_mode)

    def test_ascii_strict(self):
        wcslen = self.wcslen
        ctypes.set_conversion_mode("ascii", "strict")
        # no conversions take place with unicode arguments
        self.assertEqual(wcslen(u"abc"), 3)
        self.assertEqual(wcslen(u"ab\u2070"), 3)
        # string args are converted
        self.assertEqual(wcslen("abc"), 3)
        self.assertRaises(ctypes.ArgumentError, wcslen, "abä")

    def test_ascii_replace(self):
        wcslen = self.wcslen
        ctypes.set_conversion_mode("ascii", "replace")
        self.assertEqual(wcslen(u"abc"), 3)
        self.assertEqual(wcslen(u"ab\u2070"), 3)
        self.assertEqual(wcslen("abc"), 3)
        self.assertEqual(wcslen("abä"), 3)

    def test_ascii_ignore(self):
        wcslen = self.wcslen
        ctypes.set_conversion_mode("ascii", "ignore")
        self.assertEqual(wcslen(u"abc"), 3)
        self.assertEqual(wcslen(u"ab\u2070"), 3)
        # ignore error mode skips non-ascii characters
        self.assertEqual(wcslen("abc"), 3)
        self.assertEqual(wcslen("äöüß"), 0)

    def test_latin1_strict(self):
        wcslen = self.wcslen
        ctypes.set_conversion_mode("latin-1", "strict")
        self.assertEqual(wcslen(u"abc"), 3)
        self.assertEqual(wcslen(u"ab\u2070"), 3)
        self.assertEqual(wcslen("abc"), 3)
        self.assertEqual(wcslen("äöüß"), 4)

    def test_buffers(self):
        ctypes.set_conversion_mode("ascii", "strict")
        buf = ctypes.create_unicode_buffer("abc")
        self.assertEqual(len(buf), 3+1)

        ctypes.set_conversion_mode("ascii", "replace")
        buf = ctypes.create_unicode_buffer("abäöü")
        self.assertEqual(buf[:], u"ab\uFFFD\uFFFD\uFFFD\0")
        self.assertEqual(buf[::], u"ab\uFFFD\uFFFD\uFFFD\0")
        self.assertEqual(buf[::-1], u"\0\uFFFD\uFFFD\uFFFDba")
        self.assertEqual(buf[::2], u"a\uFFFD\uFFFD")
        self.assertEqual(buf[6:5:-1], u"")

        ctypes.set_conversion_mode("ascii", "ignore")
        buf = ctypes.create_unicode_buffer("abäöü")
        # is that correct? not sure.  But with 'ignore', you get what you pay for..
        self.assertEqual(buf[:], u"ab\0\0\0\0")
        self.assertEqual(buf[::], u"ab\0\0\0\0")
        self.assertEqual(buf[::-1], u"\0\0\0\0ba")
        self.assertEqual(buf[::2], u"a\0\0")
        self.assertEqual(buf[6:5:-1], u"")

@need_symbol('c_wchar')
class StringTestCase(UnicodeTestCase):
    @classmethod
    def setUpClass(cls):
        super(StringTestCase, cls).setUpClass()
        cls.func = ctypes.CDLL(_ctypes_test.__file__)._testfunc_p_p

    def setUp(self):
        func = self.func
        self.prev_conv_mode = ctypes.set_conversion_mode("ascii", "strict")
        func.argtypes = [ctypes.c_char_p]
        func.restype = ctypes.c_char_p

    def tearDown(self):
        func = self.func
        ctypes.set_conversion_mode(*self.prev_conv_mode)
        func.argtypes = None
        func.restype = ctypes.c_int

    def test_ascii_replace(self):
        func = self.func
        ctypes.set_conversion_mode("ascii", "strict")
        self.assertEqual(func("abc"), "abc")
        self.assertEqual(func(u"abc"), "abc")
        self.assertRaises(ctypes.ArgumentError, func, u"abä")

    def test_ascii_ignore(self):
        func = self.func
        ctypes.set_conversion_mode("ascii", "ignore")
        self.assertEqual(func("abc"), "abc")
        self.assertEqual(func(u"abc"), "abc")
        self.assertEqual(func(u"äöüß"), "")

    def test_ascii_replace(self):
        func = self.func
        ctypes.set_conversion_mode("ascii", "replace")
        self.assertEqual(func("abc"), "abc")
        self.assertEqual(func(u"abc"), "abc")
        self.assertEqual(func(u"äöüß"), "????")

    def test_buffers(self):
        ctypes.set_conversion_mode("ascii", "strict")
        buf = ctypes.create_string_buffer(u"abc")
        self.assertEqual(len(buf), 3+1)

        ctypes.set_conversion_mode("ascii", "replace")
        buf = ctypes.create_string_buffer(u"abäöü")
        self.assertEqual(buf[:], "ab???\0")
        self.assertEqual(buf[::], "ab???\0")
        self.assertEqual(buf[::-1], "\0???ba")
        self.assertEqual(buf[::2], "a??")
        self.assertEqual(buf[6:5:-1], "")

        ctypes.set_conversion_mode("ascii", "ignore")
        buf = ctypes.create_string_buffer(u"abäöü")
        # is that correct? not sure.  But with 'ignore', you get what you pay for..
        self.assertEqual(buf[:], "ab\0\0\0\0")
        self.assertEqual(buf[::], "ab\0\0\0\0")
        self.assertEqual(buf[::-1], "\0\0\0\0ba")

if __name__ == '__main__':
    unittest.main()
