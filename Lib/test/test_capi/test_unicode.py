import unittest
from test import support

class CAPITest(unittest.TestCase):

    # Test PyUnicode_FromFormat()
    def test_from_format(self):
        support.import_module('ctypes')
        from ctypes import (
            pythonapi, py_object, sizeof,
            c_int, c_long, c_longlong, c_ssize_t,
            c_uint, c_ulong, c_ulonglong, c_size_t, c_void_p)
        name = "PyUnicode_FromFormat"
        _PyUnicode_FromFormat = getattr(pythonapi, name)
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
                     b'%')
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

        # test integer formats (%i, %d, %u)
        check_format('010',
                     b'%03i', c_int(10))
        check_format('0010',
                     b'%0.4i', c_int(10))
        check_format('-123',
                     b'%i', c_int(-123))
        check_format('-123',
                     b'%li', c_long(-123))
        check_format('-123',
                     b'%lli', c_longlong(-123))
        check_format('-123',
                     b'%zi', c_ssize_t(-123))

        check_format('-123',
                     b'%d', c_int(-123))
        check_format('-123',
                     b'%ld', c_long(-123))
        check_format('-123',
                     b'%lld', c_longlong(-123))
        check_format('-123',
                     b'%zd', c_ssize_t(-123))

        check_format('123',
                     b'%u', c_uint(123))
        check_format('123',
                     b'%lu', c_ulong(123))
        check_format('123',
                     b'%llu', c_ulonglong(123))
        check_format('123',
                     b'%zu', c_size_t(123))

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
        check_format('123'.rjust(10, '0'),
                     b'%010i', c_int(123))
        check_format('123'.rjust(100),
                     b'%100i', c_int(123))
        check_format('123'.rjust(100, '0'),
                     b'%.100i', c_int(123))
        check_format('123'.rjust(80, '0').rjust(100),
                     b'%100.80i', c_int(123))

        check_format('123'.rjust(10, '0'),
                     b'%010u', c_uint(123))
        check_format('123'.rjust(100),
                     b'%100u', c_uint(123))
        check_format('123'.rjust(100, '0'),
                     b'%.100u', c_uint(123))
        check_format('123'.rjust(80, '0').rjust(100),
                     b'%100.80u', c_uint(123))

        check_format('123'.rjust(10, '0'),
                     b'%010x', c_int(0x123))
        check_format('123'.rjust(100),
                     b'%100x', c_int(0x123))
        check_format('123'.rjust(100, '0'),
                     b'%.100x', c_int(0x123))
        check_format('123'.rjust(80, '0').rjust(100),
                     b'%100.80x', c_int(0x123))

        # test %A
        check_format(r"%A:'abc\xe9\uabcd\U0010ffff'",
                     b'%%A:%A', 'abc\xe9\uabcd\U0010ffff')

        # test %V
        check_format('repr=abc',
                     b'repr=%V', 'abc', b'xyz')

        # Test string decode from parameter of %s using utf-8.
        # b'\xe4\xba\xba\xe6\xb0\x91' is utf-8 encoded byte sequence of
        # '\u4eba\u6c11'
        check_format('repr=\u4eba\u6c11',
                     b'repr=%V', None, b'\xe4\xba\xba\xe6\xb0\x91')

        #Test replace error handler.
        check_format('repr=abc\ufffd',
                     b'repr=%V', None, b'abc\xff')

        # not supported: copy the raw format string. these tests are just here
        # to check for crashes and should not be considered as specifications
        check_format('%s',
                     b'%1%s', b'abc')
        check_format('%1abc',
                     b'%1abc')
        check_format('%+i',
                     b'%+i', c_int(10))
        check_format('%.%s',
                     b'%.%s', b'abc')

    # Test PyUnicode_AsWideChar()
    @support.cpython_only
    def test_aswidechar(self):
        from _testcapi import unicode_aswidechar
        support.import_module('ctypes')
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
    def test_aswidecharstring(self):
        from _testcapi import unicode_aswidecharstring
        support.import_module('ctypes')
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
    def test_asucs4(self):
        from _testcapi import unicode_asucs4
        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600',
                  'a\ud800b\udfffc', '\ud834\udd1e']:
            l = len(s)
            self.assertEqual(unicode_asucs4(s, l, 1), s+'\0')
            self.assertEqual(unicode_asucs4(s, l, 0), s+'\uffff')
            self.assertEqual(unicode_asucs4(s, l+1, 1), s+'\0\uffff')
            self.assertEqual(unicode_asucs4(s, l+1, 0), s+'\0\uffff')
            self.assertRaises(SystemError, unicode_asucs4, s, l-1, 1)
            self.assertRaises(SystemError, unicode_asucs4, s, l-2, 0)
            s = '\0'.join([s, s])
            self.assertEqual(unicode_asucs4(s, len(s), 1), s+'\0')
            self.assertEqual(unicode_asucs4(s, len(s), 0), s+'\uffff')

    # Test PyUnicode_FindChar()
    @support.cpython_only
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
    def test_encode_decimal(self):
        from _testcapi import unicode_encodedecimal
        self.assertEqual(unicode_encodedecimal('123'),
                         b'123')
        self.assertEqual(unicode_encodedecimal('\u0663.\u0661\u0664'),
                         b'3.14')
        self.assertEqual(unicode_encodedecimal("\N{EM SPACE}3.14\N{EN SPACE}"),
                         b' 3.14 ')
        self.assertRaises(UnicodeEncodeError,
                          unicode_encodedecimal, "123\u20ac", "strict")
        self.assertRaisesRegex(
            ValueError,
            "^'decimal' codec can't encode character",
            unicode_encodedecimal, "123\u20ac", "replace")

    @support.cpython_only
    def test_transform_decimal(self):
        from _testcapi import unicode_transformdecimaltoascii as transform_decimal
        self.assertEqual(transform_decimal('123'),
                         '123')
        self.assertEqual(transform_decimal('\u0663.\u0661\u0664'),
                         '3.14')
        self.assertEqual(transform_decimal("\N{EM SPACE}3.14\N{EN SPACE}"),
                         "\N{EM SPACE}3.14\N{EN SPACE}")
        self.assertEqual(transform_decimal('123\u20ac'),
                         '123\u20ac')

    @support.cpython_only
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
