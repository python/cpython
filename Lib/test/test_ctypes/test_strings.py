import unittest
from ctypes import (create_string_buffer, create_unicode_buffer,
                    sizeof, byref, c_char, c_wchar)


class StringArrayTestCase(unittest.TestCase):
    def test(self):
        BUF = c_char * 4

        buf = BUF(b"a", b"b", b"c")
        self.assertEqual(buf.value, b"abc")
        self.assertEqual(buf.raw, b"abc\000")

        buf.value = b"ABCD"
        self.assertEqual(buf.value, b"ABCD")
        self.assertEqual(buf.raw, b"ABCD")

        buf.value = b"x"
        self.assertEqual(buf.value, b"x")
        self.assertEqual(buf.raw, b"x\000CD")

        buf[1] = b"Z"
        self.assertEqual(buf.value, b"xZCD")
        self.assertEqual(buf.raw, b"xZCD")

        self.assertRaises(ValueError, setattr, buf, "value", b"aaaaaaaa")
        self.assertRaises(TypeError, setattr, buf, "value", 42)

    def test_create_string_buffer_value(self):
        buf = create_string_buffer(32)

        buf.value = b"Hello, World"
        self.assertEqual(buf.value, b"Hello, World")

        self.assertRaises(TypeError, setattr, buf, "value", memoryview(b"Hello, World"))
        self.assertRaises(TypeError, setattr, buf, "value", memoryview(b"abc"))
        self.assertRaises(ValueError, setattr, buf, "raw", memoryview(b"x" * 100))

    def test_create_string_buffer_raw(self):
        buf = create_string_buffer(32)

        buf.raw = memoryview(b"Hello, World")
        self.assertEqual(buf.value, b"Hello, World")
        self.assertRaises(TypeError, setattr, buf, "value", memoryview(b"abc"))
        self.assertRaises(ValueError, setattr, buf, "raw", memoryview(b"x" * 100))

    def test_param_1(self):
        BUF = c_char * 4
        buf = BUF()

    def test_param_2(self):
        BUF = c_char * 4
        buf = BUF()

    def test_del_segfault(self):
        BUF = c_char * 4
        buf = BUF()
        with self.assertRaises(AttributeError):
            del buf.raw


class WStringArrayTestCase(unittest.TestCase):
    def test(self):
        BUF = c_wchar * 4

        buf = BUF("a", "b", "c")
        self.assertEqual(buf.value, "abc")

        buf.value = "ABCD"
        self.assertEqual(buf.value, "ABCD")

        buf.value = "x"
        self.assertEqual(buf.value, "x")

        buf[1] = "Z"
        self.assertEqual(buf.value, "xZCD")

    @unittest.skipIf(sizeof(c_wchar) < 4,
                     "sizeof(wchar_t) is smaller than 4 bytes")
    def test_nonbmp(self):
        u = chr(0x10ffff)
        w = c_wchar(u)
        self.assertEqual(w.value, u)


class WStringTestCase(unittest.TestCase):
    def test_wchar(self):
        c_wchar("x")
        repr(byref(c_wchar("x")))
        c_wchar("x")

    def test_basic_wstrings(self):
        cs = create_unicode_buffer("abcdef")
        self.assertEqual(cs.value, "abcdef")

        # value can be changed
        cs.value = "abc"
        self.assertEqual(cs.value, "abc")

        # string is truncated at NUL character
        cs.value = "def\0z"
        self.assertEqual(cs.value, "def")

        self.assertEqual(create_unicode_buffer("abc\0def").value, "abc")

        # created with an empty string
        cs = create_unicode_buffer(3)
        self.assertEqual(cs.value, "")

        cs.value = "abc"
        self.assertEqual(cs.value, "abc")

    def test_toolong(self):
        cs = create_unicode_buffer("abc")
        with self.assertRaises(ValueError):
            cs.value = "abcdef"

        cs = create_unicode_buffer(4)
        with self.assertRaises(ValueError):
            cs.value = "abcdef"


def run_test(rep, msg, func, arg):
    items = range(rep)
    from time import perf_counter as clock
    start = clock()
    for i in items:
        func(arg); func(arg); func(arg); func(arg); func(arg)
    stop = clock()
    print("%20s: %.2f us" % (msg, ((stop-start)*1e6/5/rep)))


if __name__ == '__main__':
    unittest.main()
