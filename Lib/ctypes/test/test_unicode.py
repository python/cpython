# coding: latin-1
import unittest
import ctypes

try:
    ctypes.c_wchar
except AttributeError:
    pass
else:
    import _ctypes_test
    dll = ctypes.CDLL(_ctypes_test.__file__)
    wcslen = dll.my_wcslen
    wcslen.argtypes = [ctypes.c_wchar_p]


    class UnicodeTestCase(unittest.TestCase):
        def setUp(self):
            self.prev_conv_mode = ctypes.set_conversion_mode("ascii", "strict")

        def tearDown(self):
            ctypes.set_conversion_mode(*self.prev_conv_mode)

        def test_ascii_strict(self):
            ctypes.set_conversion_mode("ascii", "strict")
            # no conversions take place with unicode arguments
            self.failUnlessEqual(wcslen(u"abc"), 3)
            self.failUnlessEqual(wcslen(u"ab\u2070"), 3)
            # string args are converted
            self.failUnlessEqual(wcslen("abc"), 3)
            self.failUnlessRaises(ctypes.ArgumentError, wcslen, "abä")

        def test_ascii_replace(self):
            ctypes.set_conversion_mode("ascii", "replace")
            self.failUnlessEqual(wcslen(u"abc"), 3)
            self.failUnlessEqual(wcslen(u"ab\u2070"), 3)
            self.failUnlessEqual(wcslen("abc"), 3)
            self.failUnlessEqual(wcslen("abä"), 3)

        def test_ascii_ignore(self):
            ctypes.set_conversion_mode("ascii", "ignore")
            self.failUnlessEqual(wcslen(u"abc"), 3)
            self.failUnlessEqual(wcslen(u"ab\u2070"), 3)
            # ignore error mode skips non-ascii characters
            self.failUnlessEqual(wcslen("abc"), 3)
            self.failUnlessEqual(wcslen("äöüß"), 0)

        def test_latin1_strict(self):
            ctypes.set_conversion_mode("latin-1", "strict")
            self.failUnlessEqual(wcslen(u"abc"), 3)
            self.failUnlessEqual(wcslen(u"ab\u2070"), 3)
            self.failUnlessEqual(wcslen("abc"), 3)
            self.failUnlessEqual(wcslen("äöüß"), 4)

        def test_buffers(self):
            ctypes.set_conversion_mode("ascii", "strict")
            buf = ctypes.create_unicode_buffer("abc")
            self.failUnlessEqual(len(buf), 3+1)

            ctypes.set_conversion_mode("ascii", "replace")
            buf = ctypes.create_unicode_buffer("abäöü")
            self.failUnlessEqual(buf[:], u"ab\uFFFD\uFFFD\uFFFD\0")

            ctypes.set_conversion_mode("ascii", "ignore")
            buf = ctypes.create_unicode_buffer("abäöü")
            # is that correct? not sure.  But with 'ignore', you get what you pay for..
            self.failUnlessEqual(buf[:], u"ab\0\0\0\0")

    import _ctypes_test
    func = ctypes.CDLL(_ctypes_test.__file__)._testfunc_p_p

    class StringTestCase(UnicodeTestCase):
        def setUp(self):
            self.prev_conv_mode = ctypes.set_conversion_mode("ascii", "strict")
            func.argtypes = [ctypes.c_char_p]
            func.restype = ctypes.c_char_p

        def tearDown(self):
            ctypes.set_conversion_mode(*self.prev_conv_mode)
            func.argtypes = None
            func.restype = ctypes.c_int

        def test_ascii_replace(self):
            ctypes.set_conversion_mode("ascii", "strict")
            self.failUnlessEqual(func("abc"), "abc")
            self.failUnlessEqual(func(u"abc"), "abc")
            self.assertRaises(ctypes.ArgumentError, func, u"abä")

        def test_ascii_ignore(self):
            ctypes.set_conversion_mode("ascii", "ignore")
            self.failUnlessEqual(func("abc"), "abc")
            self.failUnlessEqual(func(u"abc"), "abc")
            self.failUnlessEqual(func(u"äöüß"), "")

        def test_ascii_replace(self):
            ctypes.set_conversion_mode("ascii", "replace")
            self.failUnlessEqual(func("abc"), "abc")
            self.failUnlessEqual(func(u"abc"), "abc")
            self.failUnlessEqual(func(u"äöüß"), "????")

        def test_buffers(self):
            ctypes.set_conversion_mode("ascii", "strict")
            buf = ctypes.create_string_buffer(u"abc")
            self.failUnlessEqual(len(buf), 3+1)

            ctypes.set_conversion_mode("ascii", "replace")
            buf = ctypes.create_string_buffer(u"abäöü")
            self.failUnlessEqual(buf[:], "ab???\0")

            ctypes.set_conversion_mode("ascii", "ignore")
            buf = ctypes.create_string_buffer(u"abäöü")
            # is that correct? not sure.  But with 'ignore', you get what you pay for..
            self.failUnlessEqual(buf[:], "ab\0\0\0\0")

if __name__ == '__main__':
    unittest.main()
