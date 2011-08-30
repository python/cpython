from ctypes import *
import unittest

class StringBufferTestCase(unittest.TestCase):

    def test_buffer(self):
        b = create_string_buffer(32)
        self.assertEqual(len(b), 32)
        self.assertEqual(sizeof(b), 32 * sizeof(c_char))
        self.assertTrue(type(b[0]) is bytes)

        b = create_string_buffer(b"abc")
        self.assertEqual(len(b), 4) # trailing nul char
        self.assertEqual(sizeof(b), 4 * sizeof(c_char))
        self.assertTrue(type(b[0]) is bytes)
        self.assertEqual(b[0], b"a")
        self.assertEqual(b[:], b"abc\0")
        self.assertEqual(b[::], b"abc\0")
        self.assertEqual(b[::-1], b"\0cba")
        self.assertEqual(b[::2], b"ac")
        self.assertEqual(b[::5], b"a")

    def test_buffer_interface(self):
        self.assertEqual(len(bytearray(create_string_buffer(0))), 0)
        self.assertEqual(len(bytearray(create_string_buffer(1))), 1)

    try:
        c_wchar
    except NameError:
        pass
    else:
        def test_unicode_buffer(self):
            b = create_unicode_buffer(32)
            self.assertEqual(len(b), 32)
            self.assertEqual(sizeof(b), 32 * sizeof(c_wchar))
            self.assertTrue(type(b[0]) is str)

            b = create_unicode_buffer("abc")
            self.assertEqual(len(b), 4) # trailing nul char
            self.assertEqual(sizeof(b), 4 * sizeof(c_wchar))
            self.assertTrue(type(b[0]) is str)
            self.assertEqual(b[0], "a")
            self.assertEqual(b[:], "abc\0")
            self.assertEqual(b[::], "abc\0")
            self.assertEqual(b[::-1], "\0cba")
            self.assertEqual(b[::2], "ac")
            self.assertEqual(b[::5], "a")

        def test_unicode_conversion(self):
            b = create_unicode_buffer("abc")
            self.assertEqual(len(b), 4) # trailing nul char
            self.assertEqual(sizeof(b), 4 * sizeof(c_wchar))
            self.assertTrue(type(b[0]) is str)
            self.assertEqual(b[0], "a")
            self.assertEqual(b[:], "abc\0")
            self.assertEqual(b[::], "abc\0")
            self.assertEqual(b[::-1], "\0cba")
            self.assertEqual(b[::2], "ac")
            self.assertEqual(b[::5], "a")

if __name__ == "__main__":
    unittest.main()
