import sys
import unittest
from ctypes import *

class MemFunctionsTest(unittest.TestCase):
    def test_memmove(self):
        # large buffers apparently increase the chance that the memory
        # is allocated in high address space.
        a = create_string_buffer(1000000)
        p = "Hello, World"
        result = memmove(a, p, len(p))
        self.failUnlessEqual(a.value, "Hello, World")

        self.failUnlessEqual(string_at(result), "Hello, World")
        self.failUnlessEqual(string_at(result, 5), "Hello")
        self.failUnlessEqual(string_at(result, 16), "Hello, World\0\0\0\0")
        self.failUnlessEqual(string_at(result, 0), "")

    def test_memset(self):
        a = create_string_buffer(1000000)
        result = memset(a, ord('x'), 16)
        self.failUnlessEqual(a.value, "xxxxxxxxxxxxxxxx")

        self.failUnlessEqual(string_at(result), "xxxxxxxxxxxxxxxx")
        self.failUnlessEqual(string_at(a), "xxxxxxxxxxxxxxxx")
        self.failUnlessEqual(string_at(a, 20), "xxxxxxxxxxxxxxxx\0\0\0\0")

    def test_cast(self):
        a = (c_ubyte * 32)(*map(ord, "abcdef"))
        self.failUnlessEqual(cast(a, c_char_p).value, "abcdef")
        self.failUnlessEqual(cast(a, POINTER(c_byte))[:7],
                             [97, 98, 99, 100, 101, 102, 0])

    def test_string_at(self):
        s = string_at("foo bar")
        # XXX The following may be wrong, depending on how Python
        # manages string instances
        self.failUnlessEqual(2, sys.getrefcount(s))
        self.failUnless(s, "foo bar")

        self.failUnlessEqual(string_at("foo bar", 8), "foo bar\0")
        self.failUnlessEqual(string_at("foo bar", 3), "foo")

    try:
        create_unicode_buffer
    except NameError:
        pass
    else:
        def test_wstring_at(self):
            p = create_unicode_buffer("Hello, World")
            a = create_unicode_buffer(1000000)
            result = memmove(a, p, len(p) * sizeof(c_wchar))
            self.failUnlessEqual(a.value, "Hello, World")

            self.failUnlessEqual(wstring_at(a), "Hello, World")
            self.failUnlessEqual(wstring_at(a, 5), "Hello")
            self.failUnlessEqual(wstring_at(a, 16), "Hello, World\0\0\0\0")
            self.failUnlessEqual(wstring_at(a, 0), "")

if __name__ == "__main__":
    unittest.main()
