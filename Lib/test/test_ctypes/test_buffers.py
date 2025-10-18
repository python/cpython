import unittest
from ctypes import (create_string_buffer, create_unicode_buffer, sizeof,
                    c_char, c_wchar)


class StringBufferTestCase(unittest.TestCase):
    def test_buffer(self):
        b = create_string_buffer(32)
        self.assertEqual(len(b), 32)
        self.assertEqual(sizeof(b), 32 * sizeof(c_char))
        self.assertIs(type(b[0]), bytes)

        b = create_string_buffer(b"abc")
        self.assertEqual(len(b), 4) # trailing nul char
        self.assertEqual(sizeof(b), 4 * sizeof(c_char))
        self.assertIs(type(b[0]), bytes)
        self.assertEqual(b[0], b"a")
        self.assertEqual(b[:], b"abc\0")
        self.assertEqual(b[::], b"abc\0")
        self.assertEqual(b[::-1], b"\0cba")
        self.assertEqual(b[::2], b"ac")
        self.assertEqual(b[::5], b"a")

        self.assertRaises(TypeError, create_string_buffer, "abc")

    def test_buffer_interface(self):
        self.assertEqual(len(bytearray(create_string_buffer(0))), 0)
        self.assertEqual(len(bytearray(create_string_buffer(1))), 1)

    def test_unicode_buffer(self):
        b = create_unicode_buffer(32)
        self.assertEqual(len(b), 32)
        self.assertEqual(sizeof(b), 32 * sizeof(c_wchar))
        self.assertIs(type(b[0]), str)

        b = create_unicode_buffer("abc")
        self.assertEqual(len(b), 4) # trailing nul char
        self.assertEqual(sizeof(b), 4 * sizeof(c_wchar))
        self.assertIs(type(b[0]), str)
        self.assertEqual(b[0], "a")
        self.assertEqual(b[:], "abc\0")
        self.assertEqual(b[::], "abc\0")
        self.assertEqual(b[::-1], "\0cba")
        self.assertEqual(b[::2], "ac")
        self.assertEqual(b[::5], "a")

        self.assertRaises(TypeError, create_unicode_buffer, b"abc")

    def test_unicode_conversion(self):
        b = create_unicode_buffer("abc")
        self.assertEqual(len(b), 4) # trailing nul char
        self.assertEqual(sizeof(b), 4 * sizeof(c_wchar))
        self.assertIs(type(b[0]), str)
        self.assertEqual(b[0], "a")
        self.assertEqual(b[:], "abc\0")
        self.assertEqual(b[::], "abc\0")
        self.assertEqual(b[::-1], "\0cba")
        self.assertEqual(b[::2], "ac")
        self.assertEqual(b[::5], "a")

    def test_create_unicode_buffer_non_bmp(self):
        expected = 5 if sizeof(c_wchar) == 2 else 3
        for s in '\U00010000\U00100000', '\U00010000\U0010ffff':
            b = create_unicode_buffer(s)
            self.assertEqual(len(b), expected)
            self.assertEqual(b[-1], '\0')


if __name__ == "__main__":
    unittest.main()
