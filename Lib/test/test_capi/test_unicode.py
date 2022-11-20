import unittest
import sys
from test import support
from test.support import import_helper

try:
    import _testcapi
except ImportError:
    _testcapi = None


class CAPITest(unittest.TestCase):

    # Test PyUnicode_FromFormat()
    def test_from_format(self):
        # Length modifiers "j" and "t" are not tested here because ctypes does
        # not expose types for intmax_t and ptrdiff_t.
        # _testcapi.test_string_from_format() has a wider coverage of all
        # formats.
        import_helper.import_module('ctypes')
        from ctypes import (
            c_char_p,
            pythonapi, py_object, sizeof,
            c_int, c_long, c_longlong, c_ssize_t,
            c_uint, c_ulong, c_ulonglong, c_size_t, c_void_p,
            sizeof, c_wchar, c_wchar_p)
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

    # Test PyUnicode_AsWideChar()
    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_aswidechar(self):
        from _testcapi import unicode_aswidechar
        import_helper.import_module('ctypes')
        from ctypes import c_wchar, sizeof

        wchar, size = unicode_aswidechar('abcdef', 2)
        self.assertEqual(size, 2)
        self.assertEqual(wchar, 'ab')

        wchar, size = unicode_aswidechar('abc', 3)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc')

        wchar, size = unicode_aswidechar('abc', 4)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')

        wchar, size = unicode_aswidechar('abc', 10)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')

        wchar, size = unicode_aswidechar('abc\0def', 20)
        self.assertEqual(size, 7)
        self.assertEqual(wchar, 'abc\0def\0')

        nonbmp = chr(0x10ffff)
        if sizeof(c_wchar) == 2:
            buflen = 3
            nchar = 2
        else: # sizeof(c_wchar) == 4
            buflen = 2
            nchar = 1
        wchar, size = unicode_aswidechar(nonbmp, buflen)
        self.assertEqual(size, nchar)
        self.assertEqual(wchar, nonbmp + '\0')

    # Test PyUnicode_AsWideCharString()
    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_aswidecharstring(self):
        from _testcapi import unicode_aswidecharstring
        import_helper.import_module('ctypes')
        from ctypes import c_wchar, sizeof

        wchar, size = unicode_aswidecharstring('abc')
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')

        wchar, size = unicode_aswidecharstring('abc\0def')
        self.assertEqual(size, 7)
        self.assertEqual(wchar, 'abc\0def\0')

        nonbmp = chr(0x10ffff)
        if sizeof(c_wchar) == 2:
            nchar = 2
        else: # sizeof(c_wchar) == 4
            nchar = 1
        wchar, size = unicode_aswidecharstring(nonbmp)
        self.assertEqual(size, nchar)
        self.assertEqual(wchar, nonbmp + '\0')

    # Test PyUnicode_AsUCS4()
    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_asucs4(self):
        from _testcapi import unicode_asucs4
        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600',
                  'a\ud800b\udfffc', '\ud834\udd1e']:
            l = len(s)
            self.assertEqual(unicode_asucs4(s, l, True), s+'\0')
            self.assertEqual(unicode_asucs4(s, l, False), s+'\uffff')
            self.assertEqual(unicode_asucs4(s, l+1, True), s+'\0\uffff')
            self.assertEqual(unicode_asucs4(s, l+1, False), s+'\0\uffff')
            self.assertRaises(SystemError, unicode_asucs4, s, l-1, True)
            self.assertRaises(SystemError, unicode_asucs4, s, l-2, False)
            s = '\0'.join([s, s])
            self.assertEqual(unicode_asucs4(s, len(s), True), s+'\0')
            self.assertEqual(unicode_asucs4(s, len(s), False), s+'\uffff')

    # Test PyUnicode_AsUTF8()
    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_asutf8(self):
        from _testcapi import unicode_asutf8

        bmp = '\u0100'
        bmp2 = '\uffff'
        nonbmp = chr(0x10ffff)

        self.assertEqual(unicode_asutf8(bmp), b'\xc4\x80')
        self.assertEqual(unicode_asutf8(bmp2), b'\xef\xbf\xbf')
        self.assertEqual(unicode_asutf8(nonbmp), b'\xf4\x8f\xbf\xbf')
        self.assertRaises(UnicodeEncodeError, unicode_asutf8, 'a\ud800b\udfffc')

    # Test PyUnicode_AsUTF8AndSize()
    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_asutf8andsize(self):
        from _testcapi import unicode_asutf8andsize

        bmp = '\u0100'
        bmp2 = '\uffff'
        nonbmp = chr(0x10ffff)

        self.assertEqual(unicode_asutf8andsize(bmp), (b'\xc4\x80', 2))
        self.assertEqual(unicode_asutf8andsize(bmp2), (b'\xef\xbf\xbf', 3))
        self.assertEqual(unicode_asutf8andsize(nonbmp), (b'\xf4\x8f\xbf\xbf', 4))
        self.assertRaises(UnicodeEncodeError, unicode_asutf8andsize, 'a\ud800b\udfffc')

    # Test PyUnicode_Count()
    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_count(self):
        from _testcapi import unicode_count

        st = 'abcabd'
        self.assertEqual(unicode_count(st, 'a', 0, len(st)), 2)
        self.assertEqual(unicode_count(st, 'ab', 0, len(st)), 2)
        self.assertEqual(unicode_count(st, 'abc', 0, len(st)), 1)
        self.assertEqual(unicode_count(st, 'а', 0, len(st)), 0)  # cyrillic "a"
        # start < end
        self.assertEqual(unicode_count(st, 'a', 3, len(st)), 1)
        self.assertEqual(unicode_count(st, 'a', 4, len(st)), 0)
        self.assertEqual(unicode_count(st, 'a', 0, sys.maxsize), 2)
        # start >= end
        self.assertEqual(unicode_count(st, 'abc', 0, 0), 0)
        self.assertEqual(unicode_count(st, 'a', 3, 2), 0)
        self.assertEqual(unicode_count(st, 'a', sys.maxsize, 5), 0)
        # negative
        self.assertEqual(unicode_count(st, 'ab', -len(st), -1), 2)
        self.assertEqual(unicode_count(st, 'a', -len(st), -3), 1)
        # wrong args
        self.assertRaises(TypeError, unicode_count, 'a', 'a')
        self.assertRaises(TypeError, unicode_count, 'a', 'a', 1)
        self.assertRaises(TypeError, unicode_count, 1, 'a', 0, 1)
        self.assertRaises(TypeError, unicode_count, 'a', 1, 0, 1)
        # empty string
        self.assertEqual(unicode_count('abc', '', 0, 3), 4)
        self.assertEqual(unicode_count('abc', '', 1, 3), 3)
        self.assertEqual(unicode_count('', '', 0, 1), 1)
        self.assertEqual(unicode_count('', 'a', 0, 1), 0)
        # different unicode kinds
        for uni in "\xa1", "\u8000\u8080", "\ud800\udc02", "\U0001f100\U0001f1f1":
            for ch in uni:
                self.assertEqual(unicode_count(uni, ch, 0, len(uni)), 1)
                self.assertEqual(unicode_count(st, ch, 0, len(st)), 0)

        # subclasses should still work
        class MyStr(str):
            pass

        self.assertEqual(unicode_count(MyStr('aab'), 'a', 0, 3), 2)

    # Test PyUnicode_FindChar()
    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_findchar(self):
        from _testcapi import unicode_findchar

        for str in "\xa1", "\u8000\u8080", "\ud800\udc02", "\U0001f100\U0001f1f1":
            for i, ch in enumerate(str):
                self.assertEqual(unicode_findchar(str, ord(ch), 0, len(str), 1), i)
                self.assertEqual(unicode_findchar(str, ord(ch), 0, len(str), -1), i)

        str = "!>_<!"
        self.assertEqual(unicode_findchar(str, 0x110000, 0, len(str), 1), -1)
        self.assertEqual(unicode_findchar(str, 0x110000, 0, len(str), -1), -1)
        # start < end
        self.assertEqual(unicode_findchar(str, ord('!'), 1, len(str)+1, 1), 4)
        self.assertEqual(unicode_findchar(str, ord('!'), 1, len(str)+1, -1), 4)
        # start >= end
        self.assertEqual(unicode_findchar(str, ord('!'), 0, 0, 1), -1)
        self.assertEqual(unicode_findchar(str, ord('!'), len(str), 0, 1), -1)
        # negative
        self.assertEqual(unicode_findchar(str, ord('!'), -len(str), -1, 1), 0)
        self.assertEqual(unicode_findchar(str, ord('!'), -len(str), -1, -1), 0)

    # Test PyUnicode_CopyCharacters()
    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_copycharacters(self):
        from _testcapi import unicode_copycharacters

        strings = [
            'abcde', '\xa1\xa2\xa3\xa4\xa5',
            '\u4f60\u597d\u4e16\u754c\uff01',
            '\U0001f600\U0001f601\U0001f602\U0001f603\U0001f604'
        ]

        for idx, from_ in enumerate(strings):
            # wide -> narrow: exceed maxchar limitation
            for to in strings[:idx]:
                self.assertRaises(
                    SystemError,
                    unicode_copycharacters, to, 0, from_, 0, 5
                )
            # same kind
            for from_start in range(5):
                self.assertEqual(
                    unicode_copycharacters(from_, 0, from_, from_start, 5),
                    (from_[from_start:from_start+5].ljust(5, '\0'),
                     5-from_start)
                )
            for to_start in range(5):
                self.assertEqual(
                    unicode_copycharacters(from_, to_start, from_, to_start, 5),
                    (from_[to_start:to_start+5].rjust(5, '\0'),
                     5-to_start)
                )
            # narrow -> wide
            # Tests omitted since this creates invalid strings.

        s = strings[0]
        self.assertRaises(IndexError, unicode_copycharacters, s, 6, s, 0, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, -1, s, 0, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, 0, s, 6, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, 0, s, -1, 5)
        self.assertRaises(SystemError, unicode_copycharacters, s, 1, s, 0, 5)
        self.assertRaises(SystemError, unicode_copycharacters, s, 0, s, 0, -1)
        self.assertRaises(SystemError, unicode_copycharacters, s, 0, b'', 0, 0)

    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_pep393_utf8_caching_bug(self):
        # Issue #25709: Problem with string concatenation and utf-8 cache
        from _testcapi import getargs_s_hash
        for k in 0x24, 0xa4, 0x20ac, 0x1f40d:
            s = ''
            for i in range(5):
                # Due to CPython specific optimization the 's' string can be
                # resized in-place.
                s += chr(k)
                # Parsing with the "s#" format code calls indirectly
                # PyUnicode_AsUTF8AndSize() which creates the UTF-8
                # encoded string cached in the Unicode object.
                self.assertEqual(getargs_s_hash(s), chr(k).encode() * (i + 1))
                # Check that the second call returns the same result
                self.assertEqual(getargs_s_hash(s), chr(k).encode() * (i + 1))


if __name__ == "__main__":
    unittest.main()
