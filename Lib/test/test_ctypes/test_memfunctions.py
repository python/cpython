import sys
import unittest
from test import support
from ctypes import (POINTER, sizeof, cast,
                    create_string_buffer, string_at,
                    create_unicode_buffer, wstring_at,
                    memmove, memset,
                    memoryview_at, c_void_p,
                    c_char_p, c_byte, c_ubyte, c_wchar,
                    addressof, byref)


class MemFunctionsTest(unittest.TestCase):
    def test_overflow(self):
        # string_at and wstring_at must use the Python calling
        # convention (which acquires the GIL and checks the Python
        # error flag).  Provoke an error and catch it; see also issue
        # gh-47804.
        self.assertRaises((OverflowError, MemoryError, SystemError),
                          lambda: wstring_at(u"foo", sys.maxsize - 1))
        self.assertRaises((OverflowError, MemoryError, SystemError),
                          lambda: string_at("foo", sys.maxsize - 1))

    def test_memmove(self):
        # large buffers apparently increase the chance that the memory
        # is allocated in high address space.
        a = create_string_buffer(1000000)
        p = b"Hello, World"
        result = memmove(a, p, len(p))
        self.assertEqual(a.value, b"Hello, World")

        self.assertEqual(string_at(result), b"Hello, World")
        self.assertEqual(string_at(result, 5), b"Hello")
        self.assertEqual(string_at(result, 16), b"Hello, World\0\0\0\0")
        self.assertEqual(string_at(result, 0), b"")

    def test_memset(self):
        a = create_string_buffer(1000000)
        result = memset(a, ord('x'), 16)
        self.assertEqual(a.value, b"xxxxxxxxxxxxxxxx")

        self.assertEqual(string_at(result), b"xxxxxxxxxxxxxxxx")
        self.assertEqual(string_at(a), b"xxxxxxxxxxxxxxxx")
        self.assertEqual(string_at(a, 20), b"xxxxxxxxxxxxxxxx\0\0\0\0")

    def test_cast(self):
        a = (c_ubyte * 32)(*map(ord, "abcdef"))
        self.assertEqual(cast(a, c_char_p).value, b"abcdef")
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

    @support.refcount_test
    def test_string_at(self):
        s = string_at(b"foo bar")
        self.assertTrue(s, "foo bar")

        self.assertEqual(string_at(b"foo bar", 7), b"foo bar")
        self.assertEqual(string_at(b"foo bar", 3), b"foo")

    def test_wstring_at(self):
        p = create_unicode_buffer("Hello, World")
        a = create_unicode_buffer(1000000)
        result = memmove(a, p, len(p) * sizeof(c_wchar))
        self.assertEqual(a.value, "Hello, World")

        self.assertEqual(wstring_at(a), "Hello, World")
        self.assertEqual(wstring_at(a, 5), "Hello")
        self.assertEqual(wstring_at(a, 16), "Hello, World\0\0\0\0")
        self.assertEqual(wstring_at(a, 0), "")

    def test_memoryview_at(self):
        b = (c_byte * 10)()

        size = len(b)
        for foreign_ptr in (
            b,
            cast(b, c_void_p),
            byref(b),
            addressof(b),
        ):
            with self.subTest(foreign_ptr=type(foreign_ptr).__name__):
                b[:] = b"initialval"
                v = memoryview_at(foreign_ptr, size)
                self.assertIsInstance(v, memoryview)
                self.assertEqual(bytes(v), b"initialval")

                # test that writes to source buffer get reflected in memoryview
                b[:] = b"0123456789"
                self.assertEqual(bytes(v), b"0123456789")

                # test that writes to memoryview get reflected in source buffer
                v[:] = b"9876543210"
                self.assertEqual(bytes(b), b"9876543210")

                with self.assertRaises(ValueError):
                    memoryview_at(foreign_ptr, -1)

                with self.assertRaises(ValueError):
                    memoryview_at(foreign_ptr, sys.maxsize + 1)

                v0 = memoryview_at(foreign_ptr, 0)
                self.assertEqual(bytes(v0), b'')

    def test_memoryview_at_readonly(self):
        b = (c_byte * 10)()

        size = len(b)
        for foreign_ptr in (
            b,
            cast(b, c_void_p),
            byref(b),
            addressof(b),
        ):
            with self.subTest(foreign_ptr=type(foreign_ptr).__name__):
                b[:] = b"initialval"
                v = memoryview_at(foreign_ptr, size, readonly=True)
                self.assertIsInstance(v, memoryview)
                self.assertEqual(bytes(v), b"initialval")

                # test that writes to source buffer get reflected in memoryview
                b[:] = b"0123456789"
                self.assertEqual(bytes(v), b"0123456789")

                # test that writes to the memoryview are blocked
                with self.assertRaises(TypeError):
                    v[:] = b"9876543210"

if __name__ == "__main__":
    unittest.main()
