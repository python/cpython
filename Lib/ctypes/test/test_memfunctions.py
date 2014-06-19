import sys
import unittest
from ctypes import *
from ctypes.test import need_symbol

class MemFunctionsTest(unittest.TestCase):
    @unittest.skip('test disabled')
    def test_overflow(self):
        # string_at and wstring_at must use the Python calling
        # convention (which acquires the GIL and checks the Python
        # error flag).  Provoke an error and catch it; see also issue
        # #3554: <http://bugs.python.org/issue3554>
        self.assertRaises((OverflowError, MemoryError, SystemError),
                          lambda: wstring_at(u"foo", sys.maxint - 1))
        self.assertRaises((OverflowError, MemoryError, SystemError),
                          lambda: string_at("foo", sys.maxint - 1))

    def test_memmove(self):
        # large buffers apparently increase the chance that the memory
        # is allocated in high address space.
        a = create_string_buffer(1000000)
        p = "Hello, World"
        result = memmove(a, p, len(p))
        self.assertEqual(a.value, "Hello, World")

        self.assertEqual(string_at(result), "Hello, World")
        self.assertEqual(string_at(result, 5), "Hello")
        self.assertEqual(string_at(result, 16), "Hello, World\0\0\0\0")
        self.assertEqual(string_at(result, 0), "")

    def test_memset(self):
        a = create_string_buffer(1000000)
        result = memset(a, ord('x'), 16)
        self.assertEqual(a.value, "xxxxxxxxxxxxxxxx")

        self.assertEqual(string_at(result), "xxxxxxxxxxxxxxxx")
        self.assertEqual(string_at(a), "xxxxxxxxxxxxxxxx")
        self.assertEqual(string_at(a, 20), "xxxxxxxxxxxxxxxx\0\0\0\0")

    def test_cast(self):
        a = (c_ubyte * 32)(*map(ord, "abcdef"))
        self.assertEqual(cast(a, c_char_p).value, "abcdef")
        self.assertEqual(cast(a, POINTER(c_byte))[:7],
                             [97, 98, 99, 100, 101, 102, 0])
        self.assertEqual(cast(a, POINTER(c_byte))[:7:],
                             [97, 98, 99, 100, 101, 102, 0])
        self.assertEqual(cast(a, POINTER(c_byte))[6:-1:-1],
                             [0, 102, 101, 100, 99, 98, 97])
        self.assertEqual(cast(a, POINTER(c_byte))[:7:2],
                             [97, 99, 101, 0])
        self.assertEqual(cast(a, POINTER(c_byte))[:7:7],
                             [97])

    def test_string_at(self):
        s = string_at("foo bar")
        # XXX The following may be wrong, depending on how Python
        # manages string instances
        self.assertEqual(2, sys.getrefcount(s))
        self.assertTrue(s, "foo bar")

        self.assertEqual(string_at("foo bar", 8), "foo bar\0")
        self.assertEqual(string_at("foo bar", 3), "foo")

    @need_symbol('create_unicode_buffer')
    def test_wstring_at(self):
        p = create_unicode_buffer("Hello, World")
        a = create_unicode_buffer(1000000)
        result = memmove(a, p, len(p) * sizeof(c_wchar))
        self.assertEqual(a.value, "Hello, World")

        self.assertEqual(wstring_at(a), "Hello, World")
        self.assertEqual(wstring_at(a, 5), "Hello")
        self.assertEqual(wstring_at(a, 16), "Hello, World\0\0\0\0")
        self.assertEqual(wstring_at(a, 0), "")

if __name__ == "__main__":
    unittest.main()
