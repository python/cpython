import unittest
from ctypes import *
from ctypes.test import need_symbol

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

    def test_c_buffer_value(self):
        buf = c_buffer(32)

        buf.value = b"Hello, World"
        self.assertEqual(buf.value, b"Hello, World")

        self.assertRaises(TypeError, setattr, buf, "value", memoryview(b"Hello, World"))
        self.assertRaises(TypeError, setattr, buf, "value", memoryview(b"abc"))
        self.assertRaises(ValueError, setattr, buf, "raw", memoryview(b"x" * 100))

    def test_c_buffer_raw(self):
        buf = c_buffer(32)

        buf.raw = memoryview(b"Hello, World")
        self.assertEqual(buf.value, b"Hello, World")
        self.assertRaises(TypeError, setattr, buf, "value", memoryview(b"abc"))
        self.assertRaises(ValueError, setattr, buf, "raw", memoryview(b"x" * 100))

    def test_param_1(self):
        BUF = c_char * 4
        buf = BUF()
##        print c_char_p.from_param(buf)

    def test_param_2(self):
        BUF = c_char * 4
        buf = BUF()
##        print BUF.from_param(c_char_p("python"))
##        print BUF.from_param(BUF(*"pyth"))

@need_symbol('c_wchar')
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

class StringTestCase(unittest.TestCase):
    @unittest.skip('test disabled')
    def test_basic_strings(self):
        cs = c_string("abcdef")

        # Cannot call len on a c_string any longer
        self.assertRaises(TypeError, len, cs)
        self.assertEqual(sizeof(cs), 7)

        # The value property is the string up to the first terminating NUL.
        self.assertEqual(cs.value, "abcdef")
        self.assertEqual(c_string("abc\000def").value, "abc")

        # The raw property is the total buffer contents:
        self.assertEqual(cs.raw, "abcdef\000")
        self.assertEqual(c_string("abc\000def").raw, "abc\000def\000")

        # We can change the value:
        cs.value = "ab"
        self.assertEqual(cs.value, "ab")
        self.assertEqual(cs.raw, "ab\000\000\000\000\000")

        cs.raw = "XY"
        self.assertEqual(cs.value, "XY")
        self.assertEqual(cs.raw, "XY\000\000\000\000\000")

        self.assertRaises(TypeError, c_string, "123")

    @unittest.skip('test disabled')
    def test_sized_strings(self):

        # New in releases later than 0.4.0:
        self.assertRaises(TypeError, c_string, None)

        # New in releases later than 0.4.0:
        # c_string(number) returns an empty string of size number
        self.assertEqual(len(c_string(32).raw), 32)
        self.assertRaises(ValueError, c_string, -1)
        self.assertRaises(ValueError, c_string, 0)

        # These tests fail, because it is no longer initialized
##        self.assertEqual(c_string(2).value, "")
##        self.assertEqual(c_string(2).raw, "\000\000")
        self.assertEqual(c_string(2).raw[-1], "\000")
        self.assertEqual(len(c_string(2).raw), 2)

    @unittest.skip('test disabled')
    def test_initialized_strings(self):

        self.assertEqual(c_string("ab", 4).raw[:2], "ab")
        self.assertEqual(c_string("ab", 4).raw[:2:], "ab")
        self.assertEqual(c_string("ab", 4).raw[:2:-1], "ba")
        self.assertEqual(c_string("ab", 4).raw[:2:2], "a")
        self.assertEqual(c_string("ab", 4).raw[-1], "\000")
        self.assertEqual(c_string("ab", 2).raw, "a\000")

    @unittest.skip('test disabled')
    def test_toolong(self):
        cs = c_string("abcdef")
        # Much too long string:
        self.assertRaises(ValueError, setattr, cs, "value", "123456789012345")

        # One char too long values:
        self.assertRaises(ValueError, setattr, cs, "value", "1234567")

    @unittest.skip('test disabled')
    def test_perf(self):
        check_perf()

@need_symbol('c_wchar')
class WStringTestCase(unittest.TestCase):
    def test_wchar(self):
        c_wchar("x")
        repr(byref(c_wchar("x")))
        c_wchar("x")


    @unittest.skip('test disabled')
    def test_basic_wstrings(self):
        cs = c_wstring("abcdef")

        # XXX This behaviour is about to change:
        # len returns the size of the internal buffer in bytes.
        # This includes the terminating NUL character.
        self.assertEqual(sizeof(cs), 14)

        # The value property is the string up to the first terminating NUL.
        self.assertEqual(cs.value, "abcdef")
        self.assertEqual(c_wstring("abc\000def").value, "abc")

        self.assertEqual(c_wstring("abc\000def").value, "abc")

        # The raw property is the total buffer contents:
        self.assertEqual(cs.raw, "abcdef\000")
        self.assertEqual(c_wstring("abc\000def").raw, "abc\000def\000")

        # We can change the value:
        cs.value = "ab"
        self.assertEqual(cs.value, "ab")
        self.assertEqual(cs.raw, "ab\000\000\000\000\000")

        self.assertRaises(TypeError, c_wstring, "123")
        self.assertRaises(ValueError, c_wstring, 0)

    @unittest.skip('test disabled')
    def test_toolong(self):
        cs = c_wstring("abcdef")
        # Much too long string:
        self.assertRaises(ValueError, setattr, cs, "value", "123456789012345")

        # One char too long values:
        self.assertRaises(ValueError, setattr, cs, "value", "1234567")


def run_test(rep, msg, func, arg):
    items = range(rep)
    from time import perf_counter as clock
    start = clock()
    for i in items:
        func(arg); func(arg); func(arg); func(arg); func(arg)
    stop = clock()
    print("%20s: %.2f us" % (msg, ((stop-start)*1e6/5/rep)))

def check_perf():
    # Construct 5 objects

    REP = 200000

    run_test(REP, "c_string(None)", c_string, None)
    run_test(REP, "c_string('abc')", c_string, 'abc')

# Python 2.3 -OO, win2k, P4 700 MHz:
#
#      c_string(None): 1.75 us
#     c_string('abc'): 2.74 us

# Python 2.2 -OO, win2k, P4 700 MHz:
#
#      c_string(None): 2.95 us
#     c_string('abc'): 3.67 us


if __name__ == '__main__':
##    check_perf()
    unittest.main()
