import unittest
from ctypes import (create_string_buffer, create_unicode_buffer,
                    sizeof, byref, c_char, c_wchar)
from test.support import cpython_only


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


class InternalAPITestCase(unittest.TestCase):
    @cpython_only
    def test_from_format(self):
        """Test PyUnicode_FromFormat()"""
        # Length modifiers "j" and "t" are not tested here because ctypes does
        # not expose types for intmax_t and ptrdiff_t.
        # _testcapi.test_string_from_format() has a wider coverage of all
        # formats.
        from ctypes import (
            c_char_p,
            pythonapi, py_object, sizeof,
            c_int, c_long, c_longlong, c_ssize_t,
            c_uint, c_ulong, c_ulonglong, c_size_t, c_void_p,
            c_wchar, c_wchar_p)
        name = "PyUnicode_FromFormat"
        _PyUnicode_FromFormat = getattr(pythonapi, name)
        _PyUnicode_FromFormat.argtypes = (c_char_p,)
        _PyUnicode_FromFormat.restype = py_object

        def PyUnicode_FromFormat(format, *args):
            cargs = tuple(
                py_object(arg) if isinstance(arg, str) else arg
                for arg in args)
            return _PyUnicode_FromFormat(format, *cargs)

        def check_format(expected, format, *args):
            text = PyUnicode_FromFormat(format, *args)
            self.assertEqual(expected, text)

        # ascii format, non-ascii argument
        check_format('ascii\x7f=unicode\xe9',
                     b'ascii\x7f=%U', 'unicode\xe9')

        # non-ascii format, ascii argument: ensure that PyUnicode_FromFormatV()
        # raises an error
        self.assertRaisesRegex(ValueError,
            r'^PyUnicode_FromFormatV\(\) expects an ASCII-encoded format '
            'string, got a non-ASCII byte: 0xe9$',
            PyUnicode_FromFormat, b'unicode\xe9=%s', 'ascii')

        # test "%c"
        check_format('\uabcd',
                     b'%c', c_int(0xabcd))
        check_format('\U0010ffff',
                     b'%c', c_int(0x10ffff))
        with self.assertRaises(OverflowError):
            PyUnicode_FromFormat(b'%c', c_int(0x110000))
        # Issue #18183
        check_format('\U00010000\U00100000',
                     b'%c%c', c_int(0x10000), c_int(0x100000))

        # test "%"
        check_format('%',
                     b'%%')
        check_format('%s',
                     b'%%s')
        check_format('[%]',
                     b'[%%]')
        check_format('%abc',
                     b'%%%s', b'abc')

        # truncated string
        check_format('abc',
                     b'%.3s', b'abcdef')
        check_format('abc[\ufffd',
                     b'%.5s', 'abc[\u20ac]'.encode('utf8'))
        check_format("'\\u20acABC'",
                     b'%A', '\u20acABC')
        check_format("'\\u20",
                     b'%.5A', '\u20acABCDEF')
        check_format("'\u20acABC'",
                     b'%R', '\u20acABC')
        check_format("'\u20acA",
                     b'%.3R', '\u20acABCDEF')
        check_format('\u20acAB',
                     b'%.3S', '\u20acABCDEF')
        check_format('\u20acAB',
                     b'%.3U', '\u20acABCDEF')
        check_format('\u20acAB',
                     b'%.3V', '\u20acABCDEF', None)
        check_format('abc[\ufffd',
                     b'%.5V', None, 'abc[\u20ac]'.encode('utf8'))

        # following tests comes from #7330
        # test width modifier and precision modifier with %S
        check_format("repr=  abc",
                     b'repr=%5S', 'abc')
        check_format("repr=ab",
                     b'repr=%.2S', 'abc')
        check_format("repr=   ab",
                     b'repr=%5.2S', 'abc')

        # test width modifier and precision modifier with %R
        check_format("repr=   'abc'",
                     b'repr=%8R', 'abc')
        check_format("repr='ab",
                     b'repr=%.3R', 'abc')
        check_format("repr=  'ab",
                     b'repr=%5.3R', 'abc')

        # test width modifier and precision modifier with %A
        check_format("repr=   'abc'",
                     b'repr=%8A', 'abc')
        check_format("repr='ab",
                     b'repr=%.3A', 'abc')
        check_format("repr=  'ab",
                     b'repr=%5.3A', 'abc')

        # test width modifier and precision modifier with %s
        check_format("repr=  abc",
                     b'repr=%5s', b'abc')
        check_format("repr=ab",
                     b'repr=%.2s', b'abc')
        check_format("repr=   ab",
                     b'repr=%5.2s', b'abc')

        # test width modifier and precision modifier with %U
        check_format("repr=  abc",
                     b'repr=%5U', 'abc')
        check_format("repr=ab",
                     b'repr=%.2U', 'abc')
        check_format("repr=   ab",
                     b'repr=%5.2U', 'abc')

        # test width modifier and precision modifier with %V
        check_format("repr=  abc",
                     b'repr=%5V', 'abc', b'123')
        check_format("repr=ab",
                     b'repr=%.2V', 'abc', b'123')
        check_format("repr=   ab",
                     b'repr=%5.2V', 'abc', b'123')
        check_format("repr=  123",
                     b'repr=%5V', None, b'123')
        check_format("repr=12",
                     b'repr=%.2V', None, b'123')
        check_format("repr=   12",
                     b'repr=%5.2V', None, b'123')

        # test integer formats (%i, %d, %u, %o, %x, %X)
        check_format('010',
                     b'%03i', c_int(10))
        check_format('0010',
                     b'%0.4i', c_int(10))
        for conv, signed, value, expected in [
            (b'i', True, -123, '-123'),
            (b'd', True, -123, '-123'),
            (b'u', False, 123, '123'),
            (b'o', False, 0o123, '123'),
            (b'x', False, 0xabc, 'abc'),
            (b'X', False, 0xabc, 'ABC'),
        ]:
            for mod, ctype in [
                (b'', c_int if signed else c_uint),
                (b'l', c_long if signed else c_ulong),
                (b'll', c_longlong if signed else c_ulonglong),
                (b'z', c_ssize_t if signed else c_size_t),
            ]:
                with self.subTest(format=b'%' + mod + conv):
                    check_format(expected,
                                 b'%' + mod + conv, ctype(value))

        # test long output
        min_longlong = -(2 ** (8 * sizeof(c_longlong) - 1))
        max_longlong = -min_longlong - 1
        check_format(str(min_longlong),
                     b'%lld', c_longlong(min_longlong))
        check_format(str(max_longlong),
                     b'%lld', c_longlong(max_longlong))
        max_ulonglong = 2 ** (8 * sizeof(c_ulonglong)) - 1
        check_format(str(max_ulonglong),
                     b'%llu', c_ulonglong(max_ulonglong))
        PyUnicode_FromFormat(b'%p', c_void_p(-1))

        # test padding (width and/or precision)
        check_format('123',        b'%2i', c_int(123))
        check_format('       123', b'%10i', c_int(123))
        check_format('0000000123', b'%010i', c_int(123))
        check_format('123       ', b'%-10i', c_int(123))
        check_format('123       ', b'%-010i', c_int(123))
        check_format('123',        b'%.2i', c_int(123))
        check_format('0000123',    b'%.7i', c_int(123))
        check_format('       123', b'%10.2i', c_int(123))
        check_format('   0000123', b'%10.7i', c_int(123))
        check_format('0000000123', b'%010.7i', c_int(123))
        check_format('0000123   ', b'%-10.7i', c_int(123))
        check_format('0000123   ', b'%-010.7i', c_int(123))

        check_format('-123',       b'%2i', c_int(-123))
        check_format('      -123', b'%10i', c_int(-123))
        check_format('-000000123', b'%010i', c_int(-123))
        check_format('-123      ', b'%-10i', c_int(-123))
        check_format('-123      ', b'%-010i', c_int(-123))
        check_format('-123',       b'%.2i', c_int(-123))
        check_format('-0000123',   b'%.7i', c_int(-123))
        check_format('      -123', b'%10.2i', c_int(-123))
        check_format('  -0000123', b'%10.7i', c_int(-123))
        check_format('-000000123', b'%010.7i', c_int(-123))
        check_format('-0000123  ', b'%-10.7i', c_int(-123))
        check_format('-0000123  ', b'%-010.7i', c_int(-123))

        check_format('123',        b'%2u', c_uint(123))
        check_format('       123', b'%10u', c_uint(123))
        check_format('0000000123', b'%010u', c_uint(123))
        check_format('123       ', b'%-10u', c_uint(123))
        check_format('123       ', b'%-010u', c_uint(123))
        check_format('123',        b'%.2u', c_uint(123))
        check_format('0000123',    b'%.7u', c_uint(123))
        check_format('       123', b'%10.2u', c_uint(123))
        check_format('   0000123', b'%10.7u', c_uint(123))
        check_format('0000000123', b'%010.7u', c_uint(123))
        check_format('0000123   ', b'%-10.7u', c_uint(123))
        check_format('0000123   ', b'%-010.7u', c_uint(123))

        check_format('123',        b'%2o', c_uint(0o123))
        check_format('       123', b'%10o', c_uint(0o123))
        check_format('0000000123', b'%010o', c_uint(0o123))
        check_format('123       ', b'%-10o', c_uint(0o123))
        check_format('123       ', b'%-010o', c_uint(0o123))
        check_format('123',        b'%.2o', c_uint(0o123))
        check_format('0000123',    b'%.7o', c_uint(0o123))
        check_format('       123', b'%10.2o', c_uint(0o123))
        check_format('   0000123', b'%10.7o', c_uint(0o123))
        check_format('0000000123', b'%010.7o', c_uint(0o123))
        check_format('0000123   ', b'%-10.7o', c_uint(0o123))
        check_format('0000123   ', b'%-010.7o', c_uint(0o123))

        check_format('abc',        b'%2x', c_uint(0xabc))
        check_format('       abc', b'%10x', c_uint(0xabc))
        check_format('0000000abc', b'%010x', c_uint(0xabc))
        check_format('abc       ', b'%-10x', c_uint(0xabc))
        check_format('abc       ', b'%-010x', c_uint(0xabc))
        check_format('abc',        b'%.2x', c_uint(0xabc))
        check_format('0000abc',    b'%.7x', c_uint(0xabc))
        check_format('       abc', b'%10.2x', c_uint(0xabc))
        check_format('   0000abc', b'%10.7x', c_uint(0xabc))
        check_format('0000000abc', b'%010.7x', c_uint(0xabc))
        check_format('0000abc   ', b'%-10.7x', c_uint(0xabc))
        check_format('0000abc   ', b'%-010.7x', c_uint(0xabc))

        check_format('ABC',        b'%2X', c_uint(0xabc))
        check_format('       ABC', b'%10X', c_uint(0xabc))
        check_format('0000000ABC', b'%010X', c_uint(0xabc))
        check_format('ABC       ', b'%-10X', c_uint(0xabc))
        check_format('ABC       ', b'%-010X', c_uint(0xabc))
        check_format('ABC',        b'%.2X', c_uint(0xabc))
        check_format('0000ABC',    b'%.7X', c_uint(0xabc))
        check_format('       ABC', b'%10.2X', c_uint(0xabc))
        check_format('   0000ABC', b'%10.7X', c_uint(0xabc))
        check_format('0000000ABC', b'%010.7X', c_uint(0xabc))
        check_format('0000ABC   ', b'%-10.7X', c_uint(0xabc))
        check_format('0000ABC   ', b'%-010.7X', c_uint(0xabc))

        # test %A
        check_format(r"%A:'abc\xe9\uabcd\U0010ffff'",
                     b'%%A:%A', 'abc\xe9\uabcd\U0010ffff')

        # test %V
        check_format('abc',
                     b'%V', 'abc', b'xyz')
        check_format('xyz',
                     b'%V', None, b'xyz')

        # test %ls
        check_format('abc', b'%ls', c_wchar_p('abc'))
        check_format('\u4eba\u6c11', b'%ls', c_wchar_p('\u4eba\u6c11'))
        check_format('\U0001f4bb+\U0001f40d',
                     b'%ls', c_wchar_p('\U0001f4bb+\U0001f40d'))
        check_format('   ab', b'%5.2ls', c_wchar_p('abc'))
        check_format('   \u4eba\u6c11', b'%5ls', c_wchar_p('\u4eba\u6c11'))
        check_format('  \U0001f4bb+\U0001f40d',
                     b'%5ls', c_wchar_p('\U0001f4bb+\U0001f40d'))
        check_format('\u4eba', b'%.1ls', c_wchar_p('\u4eba\u6c11'))
        check_format('\U0001f4bb' if sizeof(c_wchar) > 2 else '\ud83d',
                     b'%.1ls', c_wchar_p('\U0001f4bb+\U0001f40d'))
        check_format('\U0001f4bb+' if sizeof(c_wchar) > 2 else '\U0001f4bb',
                     b'%.2ls', c_wchar_p('\U0001f4bb+\U0001f40d'))

        # test %lV
        check_format('abc',
                     b'%lV', 'abc', c_wchar_p('xyz'))
        check_format('xyz',
                     b'%lV', None, c_wchar_p('xyz'))
        check_format('\u4eba\u6c11',
                     b'%lV', None, c_wchar_p('\u4eba\u6c11'))
        check_format('\U0001f4bb+\U0001f40d',
                     b'%lV', None, c_wchar_p('\U0001f4bb+\U0001f40d'))
        check_format('   ab',
                     b'%5.2lV', None, c_wchar_p('abc'))
        check_format('   \u4eba\u6c11',
                     b'%5lV', None, c_wchar_p('\u4eba\u6c11'))
        check_format('  \U0001f4bb+\U0001f40d',
                     b'%5lV', None, c_wchar_p('\U0001f4bb+\U0001f40d'))
        check_format('\u4eba',
                     b'%.1lV', None, c_wchar_p('\u4eba\u6c11'))
        check_format('\U0001f4bb' if sizeof(c_wchar) > 2 else '\ud83d',
                     b'%.1lV', None, c_wchar_p('\U0001f4bb+\U0001f40d'))
        check_format('\U0001f4bb+' if sizeof(c_wchar) > 2 else '\U0001f4bb',
                     b'%.2lV', None, c_wchar_p('\U0001f4bb+\U0001f40d'))

        # test variable width and precision
        check_format('  abc', b'%*s', c_int(5), b'abc')
        check_format('ab', b'%.*s', c_int(2), b'abc')
        check_format('   ab', b'%*.*s', c_int(5), c_int(2), b'abc')
        check_format('  abc', b'%*U', c_int(5), 'abc')
        check_format('ab', b'%.*U', c_int(2), 'abc')
        check_format('   ab', b'%*.*U', c_int(5), c_int(2), 'abc')
        check_format('   ab', b'%*.*V', c_int(5), c_int(2), None, b'abc')
        check_format('   ab', b'%*.*lV', c_int(5), c_int(2),
                     None, c_wchar_p('abc'))
        check_format('     123', b'%*i', c_int(8), c_int(123))
        check_format('00123', b'%.*i', c_int(5), c_int(123))
        check_format('   00123', b'%*.*i', c_int(8), c_int(5), c_int(123))

        # test %p
        # We cannot test the exact result,
        # because it returns a hex representation of a C pointer,
        # which is going to be different each time. But, we can test the format.
        p_format_regex = r'^0x[a-zA-Z0-9]{3,}$'
        p_format1 = PyUnicode_FromFormat(b'%p', 'abc')
        self.assertIsInstance(p_format1, str)
        self.assertRegex(p_format1, p_format_regex)

        p_format2 = PyUnicode_FromFormat(b'%p %p', '123456', b'xyz')
        self.assertIsInstance(p_format2, str)
        self.assertRegex(p_format2,
                         r'0x[a-zA-Z0-9]{3,} 0x[a-zA-Z0-9]{3,}')

        # Extra args are ignored:
        p_format3 = PyUnicode_FromFormat(b'%p', '123456', None, b'xyz')
        self.assertIsInstance(p_format3, str)
        self.assertRegex(p_format3, p_format_regex)

        # Test string decode from parameter of %s using utf-8.
        # b'\xe4\xba\xba\xe6\xb0\x91' is utf-8 encoded byte sequence of
        # '\u4eba\u6c11'
        check_format('repr=\u4eba\u6c11',
                     b'repr=%V', None, b'\xe4\xba\xba\xe6\xb0\x91')

        #Test replace error handler.
        check_format('repr=abc\ufffd',
                     b'repr=%V', None, b'abc\xff')

        # Issue #33817: empty strings
        check_format('',
                     b'')
        check_format('',
                     b'%s', b'')

        # test invalid format strings. these tests are just here
        # to check for crashes and should not be considered as specifications
        for fmt in (b'%', b'%0', b'%01', b'%.', b'%.1',
                    b'%0%s', b'%1%s', b'%.%s', b'%.1%s', b'%1abc',
                    b'%l', b'%ll', b'%z', b'%lls', b'%zs'):
            with self.subTest(fmt=fmt):
                self.assertRaisesRegex(SystemError, 'invalid format string',
                    PyUnicode_FromFormat, fmt, b'abc')
        self.assertRaisesRegex(SystemError, 'invalid format string',
            PyUnicode_FromFormat, b'%+i', c_int(10))



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
