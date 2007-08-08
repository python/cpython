from ctypes import *
import unittest

class StringBufferTestCase(unittest.TestCase):

    def test_buffer(self):
        b = create_string_buffer(32)
        self.failUnlessEqual(len(b), 32)
        self.failUnlessEqual(sizeof(b), 32 * sizeof(c_char))
        self.failUnless(type(b[0]) is bytes)

        b = create_string_buffer("abc")
        self.failUnlessEqual(len(b), 4) # trailing nul char
        self.failUnlessEqual(sizeof(b), 4 * sizeof(c_char))
        self.failUnless(type(b[0]) is bytes)
        self.failUnlessEqual(b[0], b"a")
        self.failUnlessEqual(b[:], "abc\0")

    def test_string_conversion(self):
        b = create_string_buffer("abc")
        self.failUnlessEqual(len(b), 4) # trailing nul char
        self.failUnlessEqual(sizeof(b), 4 * sizeof(c_char))
        self.failUnless(type(b[0]) is bytes)
        self.failUnlessEqual(b[0], b"a")
        self.failUnlessEqual(b[:], "abc\0")

    try:
        c_wchar
    except NameError:
        pass
    else:
        def test_unicode_buffer(self):
            b = create_unicode_buffer(32)
            self.failUnlessEqual(len(b), 32)
            self.failUnlessEqual(sizeof(b), 32 * sizeof(c_wchar))
            self.failUnless(type(b[0]) is str)

            b = create_unicode_buffer("abc")
            self.failUnlessEqual(len(b), 4) # trailing nul char
            self.failUnlessEqual(sizeof(b), 4 * sizeof(c_wchar))
            self.failUnless(type(b[0]) is str)
            self.failUnlessEqual(b[0], "a")
            self.failUnlessEqual(b[:], "abc\0")

        def test_unicode_conversion(self):
            b = create_unicode_buffer("abc")
            self.failUnlessEqual(len(b), 4) # trailing nul char
            self.failUnlessEqual(sizeof(b), 4 * sizeof(c_wchar))
            self.failUnless(type(b[0]) is str)
            self.failUnlessEqual(b[0], "a")
            self.failUnlessEqual(b[:], "abc\0")

if __name__ == "__main__":
    unittest.main()
