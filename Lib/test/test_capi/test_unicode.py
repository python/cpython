import unittest
import sys
from test import support
from test.support import threading_helper

try:
    import _testcapi
    from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX
except ImportError:
    _testcapi = None
try:
    import _testlimitedcapi
except ImportError:
    _testlimitedcapi = None
try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None
try:
    import ctypes
except ImportError:
    ctypes = None


NULL = None

class Str(str):
    pass


class CAPITest(unittest.TestCase):

    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_new(self):
        """Test PyUnicode_New()"""
        from _testcapi import unicode_new as new

        for maxchar in 0, 0x61, 0xa1, 0x4f60, 0x1f600, 0x10ffff:
            self.assertEqual(new(0, maxchar), '')
            self.assertEqual(new(5, maxchar), chr(maxchar)*5)
            self.assertRaises(MemoryError, new, PY_SSIZE_T_MAX, maxchar)
        self.assertEqual(new(0, 0x110000), '')
        self.assertRaises(MemoryError, new, PY_SSIZE_T_MAX//2, 0x4f60)
        self.assertRaises(MemoryError, new, PY_SSIZE_T_MAX//2+1, 0x4f60)
        self.assertRaises(MemoryError, new, PY_SSIZE_T_MAX//2, 0x1f600)
        self.assertRaises(MemoryError, new, PY_SSIZE_T_MAX//2+1, 0x1f600)
        self.assertRaises(MemoryError, new, PY_SSIZE_T_MAX//4, 0x1f600)
        self.assertRaises(MemoryError, new, PY_SSIZE_T_MAX//4+1, 0x1f600)
        self.assertRaises(SystemError, new, 5, 0x110000)
        self.assertRaises(SystemError, new, -1, 0)
        self.assertRaises(SystemError, new, PY_SSIZE_T_MIN, 0)

    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_fill(self):
        """Test PyUnicode_Fill()"""
        from _testcapi import unicode_fill as fill

        strings = [
            # all strings have exactly 5 characters
            'abcde', '\xa1\xa2\xa3\xa4\xa5',
            '\u4f60\u597d\u4e16\u754c\uff01',
            '\U0001f600\U0001f601\U0001f602\U0001f603\U0001f604'
        ]
        chars = [0x78, 0xa9, 0x20ac, 0x1f638]

        for idx, fill_char in enumerate(chars):
            # wide -> narrow: exceed maxchar limitation
            for to in strings[:idx]:
                self.assertRaises(ValueError, fill, to, 0, 0, fill_char)
            for to in strings[idx:]:
                for start in [*range(7), PY_SSIZE_T_MAX]:
                    for length in [*range(-1, 7 - start), PY_SSIZE_T_MIN, PY_SSIZE_T_MAX]:
                        filled = max(min(length, 5 - start), 0)
                        if filled == 5 and to != strings[idx]:
                            # narrow -> wide
                            # Tests omitted since this creates invalid strings.
                            continue
                        expected = to[:start] + chr(fill_char) * filled + to[start + filled:]
                        self.assertEqual(fill(to, start, length, fill_char),
                                        (expected, filled))

        s = strings[0]
        self.assertRaises(IndexError, fill, s, -1, 0, 0x78)
        self.assertRaises(IndexError, fill, s, PY_SSIZE_T_MIN, 0, 0x78)
        self.assertRaises(ValueError, fill, s, 0, 0, 0x110000)
        self.assertRaises(SystemError, fill, b'abc', 0, 0, 0x78)
        self.assertRaises(SystemError, fill, [], 0, 0, 0x78)
        # CRASHES fill(s, 0, NULL, 0, 0)
        # CRASHES fill(NULL, 0, 0, 0x78)
        # TODO: Test PyUnicode_Fill() with non-modifiable unicode.

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_writechar(self):
        """Test PyUnicode_WriteChar()"""
        from _testlimitedcapi import unicode_writechar as writechar

        strings = [
            # one string for every kind
            'abc', '\xa1\xa2\xa3', '\u4f60\u597d\u4e16',
            '\U0001f600\U0001f601\U0001f602'
        ]
        # one character for every kind + out of range code
        chars = [0x78, 0xa9, 0x20ac, 0x1f638, 0x110000]
        for i, s in enumerate(strings):
            for j, c in enumerate(chars):
                if j <= i:
                    self.assertEqual(writechar(s, 1, c),
                                     (s[:1] + chr(c) + s[2:], 0))
                else:
                    self.assertRaises(ValueError, writechar, s, 1, c)

        self.assertRaises(IndexError, writechar, 'abc', 3, 0x78)
        self.assertRaises(IndexError, writechar, 'abc', -1, 0x78)
        self.assertRaises(IndexError, writechar, 'abc', PY_SSIZE_T_MAX, 0x78)
        self.assertRaises(IndexError, writechar, 'abc', PY_SSIZE_T_MIN, 0x78)
        self.assertRaises(TypeError, writechar, b'abc', 0, 0x78)
        self.assertRaises(TypeError, writechar, [], 0, 0x78)
        # CRASHES writechar(NULL, 0, 0x78)
        # TODO: Test PyUnicode_WriteChar() with non-modifiable and legacy
        # unicode.

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_resize(self):
        """Test PyUnicode_Resize()"""
        from _testlimitedcapi import unicode_resize as resize

        strings = [
            # all strings have exactly 3 characters
            'abc', '\xa1\xa2\xa3', '\u4f60\u597d\u4e16',
            '\U0001f600\U0001f601\U0001f602'
        ]
        for s in strings:
            self.assertEqual(resize(s, 3), (s, 0))
            self.assertEqual(resize(s, 2), (s[:2], 0))
            self.assertEqual(resize(s, 4), (s + '\0', 0))
            self.assertEqual(resize(s, 10), (s + '\0'*7, 0))
            self.assertEqual(resize(s, 0), ('', 0))
            self.assertRaises(MemoryError, resize, s, PY_SSIZE_T_MAX)
            self.assertRaises(SystemError, resize, s, -1)
            self.assertRaises(SystemError, resize, s, PY_SSIZE_T_MIN)
        self.assertRaises(SystemError, resize, b'abc', 0)
        self.assertRaises(SystemError, resize, [], 0)
        self.assertRaises(SystemError, resize, NULL, 0)
        # TODO: Test PyUnicode_Resize() with non-modifiable and legacy unicode
        # and with NULL as the address.

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_append(self):
        """Test PyUnicode_Append()"""
        from _testlimitedcapi import unicode_append as append

        strings = [
            'abc', '\xa1\xa2\xa3', '\u4f60\u597d\u4e16',
            '\U0001f600\U0001f601\U0001f602'
        ]
        for left in strings:
            left = left[::-1]
            for right in strings:
                expected = left + right
                self.assertEqual(append(left, right), expected)

        self.assertRaises(SystemError, append, 'abc', b'abc')
        self.assertRaises(SystemError, append, b'abc', 'abc')
        self.assertRaises(SystemError, append, b'abc', b'abc')
        self.assertRaises(SystemError, append, 'abc', [])
        self.assertRaises(SystemError, append, [], 'abc')
        self.assertRaises(SystemError, append, [], [])
        self.assertRaises(SystemError, append, NULL, 'abc')
        self.assertRaises(SystemError, append, 'abc', NULL)
        # TODO: Test PyUnicode_Append() with modifiable unicode
        # and with NULL as the address.
        # TODO: Check reference counts.

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_appendanddel(self):
        """Test PyUnicode_AppendAndDel()"""
        from _testlimitedcapi import unicode_appendanddel as appendanddel

        strings = [
            'abc', '\xa1\xa2\xa3', '\u4f60\u597d\u4e16',
            '\U0001f600\U0001f601\U0001f602'
        ]
        for left in strings:
            left = left[::-1]
            for right in strings:
                self.assertEqual(appendanddel(left, right), left + right)

        self.assertRaises(SystemError, appendanddel, 'abc', b'abc')
        self.assertRaises(SystemError, appendanddel, b'abc', 'abc')
        self.assertRaises(SystemError, appendanddel, b'abc', b'abc')
        self.assertRaises(SystemError, appendanddel, 'abc', [])
        self.assertRaises(SystemError, appendanddel, [], 'abc')
        self.assertRaises(SystemError, appendanddel, [], [])
        self.assertRaises(SystemError, appendanddel, NULL, 'abc')
        self.assertRaises(SystemError, appendanddel, 'abc', NULL)
        # TODO: Test PyUnicode_AppendAndDel() with modifiable unicode
        # and with NULL as the address.
        # TODO: Check reference counts.

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_fromstringandsize(self):
        """Test PyUnicode_FromStringAndSize()"""
        from _testlimitedcapi import unicode_fromstringandsize as fromstringandsize

        self.assertEqual(fromstringandsize(b'abc'), 'abc')
        self.assertEqual(fromstringandsize(b'abc', 2), 'ab')
        self.assertEqual(fromstringandsize(b'abc\0def'), 'abc\0def')
        self.assertEqual(fromstringandsize(b'\xc2\xa1\xc2\xa2'), '\xa1\xa2')
        self.assertEqual(fromstringandsize(b'\xe4\xbd\xa0'), '\u4f60')
        self.assertEqual(fromstringandsize(b'\xf0\x9f\x98\x80'), '\U0001f600')
        self.assertRaises(UnicodeDecodeError, fromstringandsize, b'\xc2\xa1', 1)
        self.assertRaises(UnicodeDecodeError, fromstringandsize, b'\xa1', 1)
        self.assertEqual(fromstringandsize(b'', 0), '')
        self.assertEqual(fromstringandsize(NULL, 0), '')

        self.assertRaises(MemoryError, fromstringandsize, b'abc', PY_SSIZE_T_MAX)
        self.assertRaises(SystemError, fromstringandsize, b'abc', -1)
        self.assertRaises(SystemError, fromstringandsize, b'abc', PY_SSIZE_T_MIN)
        self.assertRaises(SystemError, fromstringandsize, NULL, -1)
        self.assertRaises(SystemError, fromstringandsize, NULL, PY_SSIZE_T_MIN)
        self.assertRaises(SystemError, fromstringandsize, NULL, 3)
        self.assertRaises(SystemError, fromstringandsize, NULL, PY_SSIZE_T_MAX)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_fromstring(self):
        """Test PyUnicode_FromString()"""
        from _testlimitedcapi import unicode_fromstring as fromstring

        self.assertEqual(fromstring(b'abc'), 'abc')
        self.assertEqual(fromstring(b'\xc2\xa1\xc2\xa2'), '\xa1\xa2')
        self.assertEqual(fromstring(b'\xe4\xbd\xa0'), '\u4f60')
        self.assertEqual(fromstring(b'\xf0\x9f\x98\x80'), '\U0001f600')
        self.assertRaises(UnicodeDecodeError, fromstring, b'\xc2')
        self.assertRaises(UnicodeDecodeError, fromstring, b'\xa1')
        self.assertEqual(fromstring(b''), '')

        # CRASHES fromstring(NULL)

    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_fromkindanddata(self):
        """Test PyUnicode_FromKindAndData()"""
        from _testcapi import unicode_fromkindanddata as fromkindanddata

        strings = [
            'abcde', '\xa1\xa2\xa3\xa4\xa5',
            '\u4f60\u597d\u4e16\u754c\uff01',
            '\U0001f600\U0001f601\U0001f602\U0001f603\U0001f604'
        ]
        enc1 = 'latin1'
        for s in strings[:2]:
            self.assertEqual(fromkindanddata(1, s.encode(enc1)), s)
        enc2 = 'utf-16le' if sys.byteorder == 'little' else 'utf-16be'
        for s in strings[:3]:
            self.assertEqual(fromkindanddata(2, s.encode(enc2)), s)
        enc4 = 'utf-32le' if sys.byteorder == 'little' else 'utf-32be'
        for s in strings:
            self.assertEqual(fromkindanddata(4, s.encode(enc4)), s)
        self.assertEqual(fromkindanddata(2, '\U0001f600'.encode(enc2)),
                         '\ud83d\ude00')
        for kind in 1, 2, 4:
            self.assertEqual(fromkindanddata(kind, b''), '')
            self.assertEqual(fromkindanddata(kind, b'\0'*kind), '\0')
            self.assertEqual(fromkindanddata(kind, NULL, 0), '')

        for kind in -1, 0, 3, 5, 8:
            self.assertRaises(SystemError, fromkindanddata, kind, b'')
        self.assertRaises(ValueError, fromkindanddata, 1, b'abc', -1)
        self.assertRaises(ValueError, fromkindanddata, 1, b'abc', PY_SSIZE_T_MIN)
        self.assertRaises(ValueError, fromkindanddata, 1, NULL, -1)
        self.assertRaises(ValueError, fromkindanddata, 1, NULL, PY_SSIZE_T_MIN)
        # CRASHES fromkindanddata(1, NULL, 1)
        # CRASHES fromkindanddata(4, b'\xff\xff\xff\xff')

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_substring(self):
        """Test PyUnicode_Substring()"""
        from _testlimitedcapi import unicode_substring as substring

        strings = [
            'ab', 'ab\xa1\xa2',
            'ab\xa1\xa2\u4f60\u597d',
            'ab\xa1\xa2\u4f60\u597d\U0001f600\U0001f601'
        ]
        for s in strings:
            for start in [*range(0, len(s) + 2), PY_SSIZE_T_MAX]:
                for end in [*range(max(start-1, 0), len(s) + 2), PY_SSIZE_T_MAX]:
                    self.assertEqual(substring(s, start, end), s[start:end])

        self.assertRaises(IndexError, substring, 'abc', -1, 0)
        self.assertRaises(IndexError, substring, 'abc', PY_SSIZE_T_MIN, 0)
        self.assertRaises(IndexError, substring, 'abc', 0, -1)
        self.assertRaises(IndexError, substring, 'abc', 0, PY_SSIZE_T_MIN)
        # CRASHES substring(b'abc', 0, 0)
        # CRASHES substring([], 0, 0)
        # CRASHES substring(NULL, 0, 0)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_getlength(self):
        """Test PyUnicode_GetLength()"""
        from _testlimitedcapi import unicode_getlength as getlength

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600',
                  'a\ud800b\udfffc', '\ud834\udd1e']:
            self.assertEqual(getlength(s), len(s))

        self.assertRaises(TypeError, getlength, b'abc')
        self.assertRaises(TypeError, getlength, [])
        # CRASHES getlength(NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_readchar(self):
        """Test PyUnicode_ReadChar()"""
        from _testlimitedcapi import unicode_readchar as readchar

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600',
                  'a\ud800b\udfffc', '\ud834\udd1e']:
            for i, c in enumerate(s):
                self.assertEqual(readchar(s, i), ord(c))
            self.assertRaises(IndexError, readchar, s, len(s))
            self.assertRaises(IndexError, readchar, s, PY_SSIZE_T_MAX)
            self.assertRaises(IndexError, readchar, s, -1)
            self.assertRaises(IndexError, readchar, s, PY_SSIZE_T_MIN)

        self.assertRaises(TypeError, readchar, b'abc', 0)
        self.assertRaises(TypeError, readchar, [], 0)
        # CRASHES readchar(NULL, 0)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_fromobject(self):
        """Test PyUnicode_FromObject()"""
        from _testlimitedcapi import unicode_fromobject as fromobject

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600',
                  'a\ud800b\udfffc', '\ud834\udd1e']:
            self.assertEqual(fromobject(s), s)
            o = Str(s)
            s2 = fromobject(o)
            self.assertEqual(s2, s)
            self.assertIs(type(s2), str)
            self.assertIsNot(s2, s)

        self.assertRaises(TypeError, fromobject, b'abc')
        self.assertRaises(TypeError, fromobject, [])
        # CRASHES fromobject(NULL)

    @unittest.skipIf(ctypes is None, 'need ctypes')
    def test_from_format(self):
        """Test PyUnicode_FromFormat()"""
        # Length modifiers "j" and "t" are not tested here because ctypes does
        # not expose types for intmax_t and ptrdiff_t.
        # _testlimitedcapi.test_string_from_format() has a wider coverage of all
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
        check_format('abc[',
                     b'%.6s', 'abc[\u20ac]'.encode('utf8'))
        check_format('abc[\u20ac',
                     b'%.7s', 'abc[\u20ac]'.encode('utf8'))
        check_format('abc[\ufffd',
                     b'%.5s', b'abc[\xff]')
        check_format('abc[',
                     b'%.6s', b'abc[\xe2\x82]')
        check_format('abc[\ufffd]',
                     b'%.7s', b'abc[\xe2\x82]')
        check_format('abc[\ufffd',
                     b'%.7s', b'abc[\xe2\x82\0')
        check_format('      abc[',
                     b'%10.6s', 'abc[\u20ac]'.encode('utf8'))
        check_format('     abc[\u20ac',
                     b'%10.7s', 'abc[\u20ac]'.encode('utf8'))
        check_format('     abc[\ufffd',
                     b'%10.5s', b'abc[\xff]')
        check_format('      abc[',
                     b'%10.6s', b'abc[\xe2\x82]')
        check_format('    abc[\ufffd]',
                     b'%10.7s', b'abc[\xe2\x82]')

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
        check_format('abc[',
                     b'%.6V', None, 'abc[\u20ac]'.encode('utf8'))
        check_format('abc[\u20ac',
                     b'%.7V', None, 'abc[\u20ac]'.encode('utf8'))
        check_format('abc[\ufffd',
                     b'%.5V', None, b'abc[\xff]')
        check_format('abc[',
                     b'%.6V', None, b'abc[\xe2\x82]')
        check_format('abc[\ufffd]',
                     b'%.7V', None, b'abc[\xe2\x82]')
        check_format('      abc[',
                     b'%10.6V', None, 'abc[\u20ac]'.encode('utf8'))
        check_format('     abc[\u20ac',
                     b'%10.7V', None, 'abc[\u20ac]'.encode('utf8'))
        check_format('     abc[\ufffd',
                     b'%10.5V', None, b'abc[\xff]')
        check_format('      abc[',
                     b'%10.6V', None, b'abc[\xe2\x82]')
        check_format('    abc[\ufffd]',
                     b'%10.7V', None, b'abc[\xe2\x82]')
        check_format('     abc[\ufffd',
                     b'%10.7V', None, b'abc[\xe2\x82\0')

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

        # test %T
        check_format('type: str',
                     b'type: %T', py_object("abc"))
        check_format(f'type: st',
                     b'type: %.2T', py_object("abc"))
        check_format(f'type:        str',
                     b'type: %10T', py_object("abc"))

        class LocalType:
            pass
        obj = LocalType()
        fullname = f'{__name__}.{LocalType.__qualname__}'
        check_format(f'type: {fullname}',
                     b'type: %T', py_object(obj))
        fullname_alt = f'{__name__}:{LocalType.__qualname__}'
        check_format(f'type: {fullname_alt}',
                     b'type: %#T', py_object(obj))

        # test %N
        check_format('type: str',
                     b'type: %N', py_object(str))
        check_format(f'type: st',
                     b'type: %.2N', py_object(str))
        check_format(f'type:        str',
                     b'type: %10N', py_object(str))

        check_format(f'type: {fullname}',
                     b'type: %N', py_object(type(obj)))
        check_format(f'type: {fullname_alt}',
                     b'type: %#N', py_object(type(obj)))
        with self.assertRaisesRegex(TypeError, "%N argument must be a type"):
            check_format('type: str',
                         b'type: %N', py_object("abc"))

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

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_interninplace(self):
        """Test PyUnicode_InternInPlace()"""
        from _testlimitedcapi import unicode_interninplace as interninplace

        s = b'abc'.decode()
        r = interninplace(s)
        self.assertEqual(r, 'abc')

        # CRASHES interninplace(b'abc')
        # CRASHES interninplace(NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_internfromstring(self):
        """Test PyUnicode_InternFromString()"""
        from _testlimitedcapi import unicode_internfromstring as internfromstring

        self.assertEqual(internfromstring(b'abc'), 'abc')
        self.assertEqual(internfromstring(b'\xf0\x9f\x98\x80'), '\U0001f600')
        self.assertRaises(UnicodeDecodeError, internfromstring, b'\xc2')
        self.assertRaises(UnicodeDecodeError, internfromstring, b'\xa1')
        self.assertEqual(internfromstring(b''), '')

        # CRASHES internfromstring(NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_fromwidechar(self):
        """Test PyUnicode_FromWideChar()"""
        from _testlimitedcapi import unicode_fromwidechar as fromwidechar
        from _testcapi import SIZEOF_WCHAR_T

        if SIZEOF_WCHAR_T == 2:
            encoding = 'utf-16le' if sys.byteorder == 'little' else 'utf-16be'
        elif SIZEOF_WCHAR_T == 4:
            encoding = 'utf-32le' if sys.byteorder == 'little' else 'utf-32be'

        for s in '', 'abc', '\xa1\xa2', '\u4f60', '\U0001f600':
            b = s.encode(encoding)
            self.assertEqual(fromwidechar(b), s)
            self.assertEqual(fromwidechar(b + b'\0'*SIZEOF_WCHAR_T, -1), s)
        for s in '\ud83d', '\ude00':
            b = s.encode(encoding, 'surrogatepass')
            self.assertEqual(fromwidechar(b), s)
            self.assertEqual(fromwidechar(b + b'\0'*SIZEOF_WCHAR_T, -1), s)

        self.assertEqual(fromwidechar('abc'.encode(encoding), 2), 'ab')
        if SIZEOF_WCHAR_T == 2:
            self.assertEqual(fromwidechar('a\U0001f600'.encode(encoding), 2), 'a\ud83d')

        self.assertRaises(MemoryError, fromwidechar, b'', PY_SSIZE_T_MAX)
        self.assertRaises(SystemError, fromwidechar, b'\0'*SIZEOF_WCHAR_T, -2)
        self.assertRaises(SystemError, fromwidechar, b'\0'*SIZEOF_WCHAR_T, PY_SSIZE_T_MIN)
        self.assertEqual(fromwidechar(NULL, 0), '')
        self.assertRaises(SystemError, fromwidechar, NULL, 1)
        self.assertRaises(SystemError, fromwidechar, NULL, PY_SSIZE_T_MAX)
        self.assertRaises(SystemError, fromwidechar, NULL, -1)
        self.assertRaises(SystemError, fromwidechar, NULL, -2)
        self.assertRaises(SystemError, fromwidechar, NULL, PY_SSIZE_T_MIN)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_aswidechar(self):
        """Test PyUnicode_AsWideChar()"""
        from _testlimitedcapi import unicode_aswidechar
        from _testlimitedcapi import unicode_aswidechar_null
        from _testcapi import SIZEOF_WCHAR_T

        wchar, size = unicode_aswidechar('abcdef', 2)
        self.assertEqual(size, 2)
        self.assertEqual(wchar, 'ab')

        wchar, size = unicode_aswidechar('abc', 3)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc')
        self.assertEqual(unicode_aswidechar_null('abc', 10), 4)
        self.assertEqual(unicode_aswidechar_null('abc', 0), 4)

        wchar, size = unicode_aswidechar('abc', 4)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')

        wchar, size = unicode_aswidechar('abc', 10)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')

        wchar, size = unicode_aswidechar('abc\0def', 20)
        self.assertEqual(size, 7)
        self.assertEqual(wchar, 'abc\0def\0')
        self.assertEqual(unicode_aswidechar_null('abc\0def', 20), 8)

        nonbmp = chr(0x10ffff)
        if SIZEOF_WCHAR_T == 2:
            nchar = 2
        else: # SIZEOF_WCHAR_T == 4
            nchar = 1
        wchar, size = unicode_aswidechar(nonbmp, 10)
        self.assertEqual(size, nchar)
        self.assertEqual(wchar, nonbmp + '\0')
        self.assertEqual(unicode_aswidechar_null(nonbmp, 10), nchar + 1)

        self.assertRaises(TypeError, unicode_aswidechar, b'abc', 10)
        self.assertRaises(TypeError, unicode_aswidechar, [], 10)
        self.assertRaises(SystemError, unicode_aswidechar, NULL, 10)
        self.assertRaises(TypeError, unicode_aswidechar_null, b'abc', 10)
        self.assertRaises(TypeError, unicode_aswidechar_null, [], 10)
        self.assertRaises(SystemError, unicode_aswidechar_null, NULL, 10)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_aswidecharstring(self):
        """Test PyUnicode_AsWideCharString()"""
        from _testlimitedcapi import unicode_aswidecharstring
        from _testlimitedcapi import unicode_aswidecharstring_null
        from _testcapi import SIZEOF_WCHAR_T

        wchar, size = unicode_aswidecharstring('abc')
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')
        self.assertEqual(unicode_aswidecharstring_null('abc'), 'abc')

        wchar, size = unicode_aswidecharstring('abc\0def')
        self.assertEqual(size, 7)
        self.assertEqual(wchar, 'abc\0def\0')
        self.assertRaises(ValueError, unicode_aswidecharstring_null, 'abc\0def')

        nonbmp = chr(0x10ffff)
        if SIZEOF_WCHAR_T == 2:
            nchar = 2
        else: # SIZEOF_WCHAR_T == 4
            nchar = 1
        wchar, size = unicode_aswidecharstring(nonbmp)
        self.assertEqual(size, nchar)
        self.assertEqual(wchar, nonbmp + '\0')
        self.assertEqual(unicode_aswidecharstring_null(nonbmp), nonbmp)

        self.assertRaises(TypeError, unicode_aswidecharstring, b'abc')
        self.assertRaises(TypeError, unicode_aswidecharstring, [])
        self.assertRaises(SystemError, unicode_aswidecharstring, NULL)
        self.assertRaises(TypeError, unicode_aswidecharstring_null, b'abc')
        self.assertRaises(TypeError, unicode_aswidecharstring_null, [])
        self.assertRaises(SystemError, unicode_aswidecharstring_null, NULL)

    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_asucs4(self):
        """Test PyUnicode_AsUCS4()"""
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

        # CRASHES unicode_asucs4(b'abc', 1, 0)
        # CRASHES unicode_asucs4(b'abc', 1, 1)
        # CRASHES unicode_asucs4([], 1, 1)
        # CRASHES unicode_asucs4(NULL, 1, 0)
        # CRASHES unicode_asucs4(NULL, 1, 1)

    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_asucs4copy(self):
        """Test PyUnicode_AsUCS4Copy()"""
        from _testcapi import unicode_asucs4copy as asucs4copy

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600',
                  'a\ud800b\udfffc', '\ud834\udd1e']:
            self.assertEqual(asucs4copy(s), s+'\0')
            s = '\0'.join([s, s])
            self.assertEqual(asucs4copy(s), s+'\0')

        # CRASHES asucs4copy(b'abc')
        # CRASHES asucs4copy([])
        # CRASHES asucs4copy(NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_fromordinal(self):
        """Test PyUnicode_FromOrdinal()"""
        from _testlimitedcapi import unicode_fromordinal as fromordinal

        self.assertEqual(fromordinal(0x61), 'a')
        self.assertEqual(fromordinal(0x20ac), '\u20ac')
        self.assertEqual(fromordinal(0x1f600), '\U0001f600')

        self.assertRaises(ValueError, fromordinal, 0x110000)
        self.assertRaises(ValueError, fromordinal, -1)

    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_asutf8(self):
        """Test PyUnicode_AsUTF8()"""
        from _testcapi import unicode_asutf8

        self.assertEqual(unicode_asutf8('abc', 4), b'abc\0')
        self.assertEqual(unicode_asutf8('абв', 7), b'\xd0\xb0\xd0\xb1\xd0\xb2\0')
        self.assertEqual(unicode_asutf8('\U0001f600', 5), b'\xf0\x9f\x98\x80\0')
        self.assertEqual(unicode_asutf8('abc\0def', 8), b'abc\0def\0')

        self.assertRaises(UnicodeEncodeError, unicode_asutf8, '\ud8ff', 0)
        self.assertRaises(TypeError, unicode_asutf8, b'abc', 0)
        self.assertRaises(TypeError, unicode_asutf8, [], 0)
        # CRASHES unicode_asutf8(NULL, 0)

    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    @threading_helper.requires_working_threading()
    def test_asutf8_race(self):
        """Test that there's no race condition in PyUnicode_AsUTF8()"""
        unicode_asutf8 = _testcapi.unicode_asutf8
        from threading import Thread

        data = "😊"

        def worker():
            for _ in range(1000):
                self.assertEqual(unicode_asutf8(data, 5), b'\xf0\x9f\x98\x8a\0')

        threads = [Thread(target=worker) for _ in range(10)]
        with threading_helper.start_threads(threads):
            pass


    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_asutf8andsize(self):
        """Test PyUnicode_AsUTF8AndSize()"""
        from _testlimitedcapi import unicode_asutf8andsize
        from _testlimitedcapi import unicode_asutf8andsize_null

        self.assertEqual(unicode_asutf8andsize('abc', 4), (b'abc\0', 3))
        self.assertEqual(unicode_asutf8andsize('абв', 7), (b'\xd0\xb0\xd0\xb1\xd0\xb2\0', 6))
        self.assertEqual(unicode_asutf8andsize('\U0001f600', 5), (b'\xf0\x9f\x98\x80\0', 4))
        self.assertEqual(unicode_asutf8andsize('abc\0def', 8), (b'abc\0def\0', 7))
        self.assertEqual(unicode_asutf8andsize_null('abc', 4), b'abc\0')
        self.assertEqual(unicode_asutf8andsize_null('abc\0def', 8), b'abc\0def\0')

        self.assertRaises(UnicodeEncodeError, unicode_asutf8andsize, '\ud8ff', 0)
        self.assertRaises(TypeError, unicode_asutf8andsize, b'abc', 0)
        self.assertRaises(TypeError, unicode_asutf8andsize, [], 0)
        self.assertRaises(UnicodeEncodeError, unicode_asutf8andsize_null, '\ud8ff', 0)
        self.assertRaises(TypeError, unicode_asutf8andsize_null, b'abc', 0)
        self.assertRaises(TypeError, unicode_asutf8andsize_null, [], 0)
        # CRASHES unicode_asutf8andsize(NULL, 0)
        # CRASHES unicode_asutf8andsize_null(NULL, 0)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_getdefaultencoding(self):
        """Test PyUnicode_GetDefaultEncoding()"""
        from _testlimitedcapi import unicode_getdefaultencoding as getdefaultencoding

        self.assertEqual(getdefaultencoding(), b'utf-8')

    @support.cpython_only
    @unittest.skipIf(_testinternalcapi is None, 'need _testinternalcapi module')
    def test_transform_decimal_and_space(self):
        """Test _PyUnicode_TransformDecimalAndSpaceToASCII()"""
        from _testinternalcapi import _PyUnicode_TransformDecimalAndSpaceToASCII as transform_decimal

        self.assertEqual(transform_decimal('123'),
                         '123')
        self.assertEqual(transform_decimal('\u0663.\u0661\u0664'),
                         '3.14')
        self.assertEqual(transform_decimal("\N{EM SPACE}3.14\N{EN SPACE}"),
                         " 3.14 ")
        self.assertEqual(transform_decimal('12\u20ac3'),
                         '12?')
        self.assertEqual(transform_decimal(''), '')

        self.assertRaises(SystemError, transform_decimal, b'123')
        self.assertRaises(SystemError, transform_decimal, [])
        # CRASHES transform_decimal(NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_concat(self):
        """Test PyUnicode_Concat()"""
        from _testlimitedcapi import unicode_concat as concat

        self.assertEqual(concat('abc', 'def'), 'abcdef')
        self.assertEqual(concat('abc', 'где'), 'abcгде')
        self.assertEqual(concat('абв', 'def'), 'абвdef')
        self.assertEqual(concat('абв', 'где'), 'абвгде')
        self.assertEqual(concat('a\0b', 'c\0d'), 'a\0bc\0d')

        self.assertRaises(TypeError, concat, b'abc', 'def')
        self.assertRaises(TypeError, concat, 'abc', b'def')
        self.assertRaises(TypeError, concat, b'abc', b'def')
        self.assertRaises(TypeError, concat, [], 'def')
        self.assertRaises(TypeError, concat, 'abc', [])
        self.assertRaises(TypeError, concat, [], [])
        # CRASHES concat(NULL, 'def')
        # CRASHES concat('abc', NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_split(self):
        """Test PyUnicode_Split()"""
        from _testlimitedcapi import unicode_split as split

        self.assertEqual(split('a|b|c|d', '|'), ['a', 'b', 'c', 'd'])
        self.assertEqual(split('a|b|c|d', '|', 2), ['a', 'b', 'c|d'])
        self.assertEqual(split('a|b|c|d', '|', PY_SSIZE_T_MAX),
                         ['a', 'b', 'c', 'd'])
        self.assertEqual(split('a|b|c|d', '|', -1), ['a', 'b', 'c', 'd'])
        self.assertEqual(split('a|b|c|d', '|', PY_SSIZE_T_MIN),
                         ['a', 'b', 'c', 'd'])
        self.assertEqual(split('a|b|c|d', '\u20ac'), ['a|b|c|d'])
        self.assertEqual(split('a||b|c||d', '||'), ['a', 'b|c', 'd'])
        self.assertEqual(split('а|б|в|г', '|'), ['а', 'б', 'в', 'г'])
        self.assertEqual(split('абабагаламага', 'а'),
                         ['', 'б', 'б', 'г', 'л', 'м', 'г', ''])
        self.assertEqual(split(' a\tb\nc\rd\ve\f', NULL),
                         ['a', 'b', 'c', 'd', 'e'])
        self.assertEqual(split('a\x85b\xa0c\u1680d\u2000e', NULL),
                         ['a', 'b', 'c', 'd', 'e'])

        self.assertRaises(ValueError, split, 'a|b|c|d', '')
        self.assertRaises(TypeError, split, 'a|b|c|d', ord('|'))
        self.assertRaises(TypeError, split, [], '|')
        # CRASHES split(NULL, '|')

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_rsplit(self):
        """Test PyUnicode_RSplit()"""
        from _testlimitedcapi import unicode_rsplit as rsplit

        self.assertEqual(rsplit('a|b|c|d', '|'), ['a', 'b', 'c', 'd'])
        self.assertEqual(rsplit('a|b|c|d', '|', 2), ['a|b', 'c', 'd'])
        self.assertEqual(rsplit('a|b|c|d', '|', PY_SSIZE_T_MAX),
                         ['a', 'b', 'c', 'd'])
        self.assertEqual(rsplit('a|b|c|d', '|', -1), ['a', 'b', 'c', 'd'])
        self.assertEqual(rsplit('a|b|c|d', '|', PY_SSIZE_T_MIN),
                         ['a', 'b', 'c', 'd'])
        self.assertEqual(rsplit('a|b|c|d', '\u20ac'), ['a|b|c|d'])
        self.assertEqual(rsplit('a||b|c||d', '||'), ['a', 'b|c', 'd'])
        self.assertEqual(rsplit('а|б|в|г', '|'), ['а', 'б', 'в', 'г'])
        self.assertEqual(rsplit('абабагаламага', 'а'),
                         ['', 'б', 'б', 'г', 'л', 'м', 'г', ''])
        self.assertEqual(rsplit('aжbжcжd', 'ж'), ['a', 'b', 'c', 'd'])
        self.assertEqual(rsplit(' a\tb\nc\rd\ve\f', NULL),
                         ['a', 'b', 'c', 'd', 'e'])
        self.assertEqual(rsplit('a\x85b\xa0c\u1680d\u2000e', NULL),
                         ['a', 'b', 'c', 'd', 'e'])

        self.assertRaises(ValueError, rsplit, 'a|b|c|d', '')
        self.assertRaises(TypeError, rsplit, 'a|b|c|d', ord('|'))
        self.assertRaises(TypeError, rsplit, [], '|')
        # CRASHES rsplit(NULL, '|')

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_partition(self):
        """Test PyUnicode_Partition()"""
        from _testlimitedcapi import unicode_partition as partition

        self.assertEqual(partition('a|b|c', '|'), ('a', '|', 'b|c'))
        self.assertEqual(partition('a||b||c', '||'), ('a', '||', 'b||c'))
        self.assertEqual(partition('а|б|в', '|'), ('а', '|', 'б|в'))
        self.assertEqual(partition('кабан', 'а'), ('к', 'а', 'бан'))
        self.assertEqual(partition('aжbжc', 'ж'), ('a', 'ж', 'bжc'))

        self.assertRaises(ValueError, partition, 'a|b|c', '')
        self.assertRaises(TypeError, partition, b'a|b|c', '|')
        self.assertRaises(TypeError, partition, 'a|b|c', b'|')
        self.assertRaises(TypeError, partition, 'a|b|c', ord('|'))
        self.assertRaises(TypeError, partition, [], '|')
        # CRASHES partition(NULL, '|')
        # CRASHES partition('a|b|c', NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_rpartition(self):
        """Test PyUnicode_RPartition()"""
        from _testlimitedcapi import unicode_rpartition as rpartition

        self.assertEqual(rpartition('a|b|c', '|'), ('a|b', '|', 'c'))
        self.assertEqual(rpartition('a||b||c', '||'), ('a||b', '||', 'c'))
        self.assertEqual(rpartition('а|б|в', '|'), ('а|б', '|', 'в'))
        self.assertEqual(rpartition('кабан', 'а'), ('каб', 'а', 'н'))
        self.assertEqual(rpartition('aжbжc', 'ж'), ('aжb', 'ж', 'c'))

        self.assertRaises(ValueError, rpartition, 'a|b|c', '')
        self.assertRaises(TypeError, rpartition, b'a|b|c', '|')
        self.assertRaises(TypeError, rpartition, 'a|b|c', b'|')
        self.assertRaises(TypeError, rpartition, 'a|b|c', ord('|'))
        self.assertRaises(TypeError, rpartition, [], '|')
        # CRASHES rpartition(NULL, '|')
        # CRASHES rpartition('a|b|c', NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_splitlines(self):
        """Test PyUnicode_SplitLines()"""
        from _testlimitedcapi import unicode_splitlines as splitlines

        self.assertEqual(splitlines('a\nb\rc\r\nd'), ['a', 'b', 'c', 'd'])
        self.assertEqual(splitlines('a\nb\rc\r\nd', True),
                         ['a\n', 'b\r', 'c\r\n', 'd'])
        self.assertEqual(splitlines('a\x85b\u2028c\u2029d'),
                         ['a', 'b', 'c', 'd'])
        self.assertEqual(splitlines('a\x85b\u2028c\u2029d', True),
                         ['a\x85', 'b\u2028', 'c\u2029', 'd'])
        self.assertEqual(splitlines('а\nб\rв\r\nг'), ['а', 'б', 'в', 'г'])

        self.assertRaises(TypeError, splitlines, b'a\nb\rc\r\nd')
        # CRASHES splitlines(NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_translate(self):
        """Test PyUnicode_Translate()"""
        from _testlimitedcapi import unicode_translate as translate

        self.assertEqual(translate('abcd', {ord('a'): 'A', ord('b'): ord('B'), ord('c'): '<>'}), 'AB<>d')
        self.assertEqual(translate('абвг', {ord('а'): 'А', ord('б'): ord('Б'), ord('в'): '<>'}), 'АБ<>г')
        self.assertEqual(translate('abc', {}), 'abc')
        self.assertEqual(translate('abc', []), 'abc')
        self.assertRaises(UnicodeTranslateError, translate, 'abc', {ord('b'): None})
        self.assertRaises(UnicodeTranslateError, translate, 'abc', {ord('b'): None}, 'strict')
        self.assertRaises(LookupError, translate, 'abc', {ord('b'): None}, 'foo')
        self.assertEqual(translate('abc', {ord('b'): None}, 'ignore'), 'ac')
        self.assertEqual(translate('abc', {ord('b'): None}, 'replace'), 'a\ufffdc')
        self.assertEqual(translate('abc', {ord('b'): None}, 'backslashreplace'), r'a\x62c')
        # XXX Other error handlers do not support UnicodeTranslateError
        self.assertRaises(TypeError, translate, b'abc', [])
        self.assertRaises(TypeError, translate, 123, [])
        self.assertRaises(TypeError, translate, 'abc', {ord('a'): b'A'})
        self.assertRaises(TypeError, translate, 'abc', 123)
        self.assertRaises(TypeError, translate, 'abc', NULL)
        self.assertRaises(LookupError, translate, 'abc', {ord('b'): None}, 'foo')
        # CRASHES translate(NULL, [])

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_join(self):
        """Test PyUnicode_Join()"""
        from _testlimitedcapi import unicode_join as join
        self.assertEqual(join('|', ['a', 'b', 'c']), 'a|b|c')
        self.assertEqual(join('|', ['a', '', 'c']), 'a||c')
        self.assertEqual(join('', ['a', 'b', 'c']), 'abc')
        self.assertEqual(join(NULL, ['a', 'b', 'c']), 'a b c')
        self.assertEqual(join('|', ['а', 'б', 'в']), 'а|б|в')
        self.assertEqual(join('ж', ['а', 'б', 'в']), 'ажбжв')
        self.assertRaises(TypeError, join, b'|', ['a', 'b', 'c'])
        self.assertRaises(TypeError, join, '|', [b'a', b'b', b'c'])
        self.assertRaises(TypeError, join, NULL, [b'a', b'b', b'c'])
        self.assertRaises(TypeError, join, '|', b'123')
        self.assertRaises(TypeError, join, '|', 123)
        self.assertRaises(SystemError, join, '|', NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_count(self):
        """Test PyUnicode_Count()"""
        from _testlimitedcapi import unicode_count

        for str in "\xa1", "\u8000\u8080", "\ud800\udc02", "\U0001f100\U0001f1f1":
            for i, ch in enumerate(str):
                self.assertEqual(unicode_count(str, ch, 0, len(str)), 1)

        str = "!>_<!"
        self.assertEqual(unicode_count(str, 'z', 0, len(str)), 0)
        self.assertEqual(unicode_count(str, '', 0, len(str)), len(str)+1)
        # start < end
        self.assertEqual(unicode_count(str, '!', 1, len(str)+1), 1)
        self.assertEqual(unicode_count(str, '!', 1, PY_SSIZE_T_MAX), 1)
        # start >= end
        self.assertEqual(unicode_count(str, '!', 0, 0), 0)
        self.assertEqual(unicode_count(str, '!', len(str), 0), 0)
        # negative
        self.assertEqual(unicode_count(str, '!', -len(str), -1), 1)
        self.assertEqual(unicode_count(str, '!', -len(str)-1, -1), 1)
        self.assertEqual(unicode_count(str, '!', PY_SSIZE_T_MIN, -1), 1)
        # bad arguments
        self.assertRaises(TypeError, unicode_count, str, b'!', 0, len(str))
        self.assertRaises(TypeError, unicode_count, b"!>_<!", '!', 0, len(str))
        self.assertRaises(TypeError, unicode_count, str, ord('!'), 0, len(str))
        self.assertRaises(TypeError, unicode_count, [], '!', 0, len(str), 1)
        # CRASHES unicode_count(NULL, '!', 0, len(str))
        # CRASHES unicode_count(str, NULL, 0, len(str))

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_tailmatch(self):
        """Test PyUnicode_Tailmatch()"""
        from _testlimitedcapi import unicode_tailmatch as tailmatch

        str = 'ababahalamaha'
        self.assertEqual(tailmatch(str, 'aba', 0, len(str), -1), 1)
        self.assertEqual(tailmatch(str, 'aha', 0, len(str), 1), 1)

        self.assertEqual(tailmatch(str, 'aba', 0, PY_SSIZE_T_MAX, -1), 1)
        self.assertEqual(tailmatch(str, 'aba', -len(str), PY_SSIZE_T_MAX, -1), 1)
        self.assertEqual(tailmatch(str, 'aba', PY_SSIZE_T_MIN, len(str), -1), 1)
        self.assertEqual(tailmatch(str, 'aha', 0, PY_SSIZE_T_MAX, 1), 1)
        self.assertEqual(tailmatch(str, 'aha', PY_SSIZE_T_MIN, len(str), 1), 1)

        self.assertEqual(tailmatch(str, 'z', 0, len(str), 1), 0)
        self.assertEqual(tailmatch(str, 'z', 0, len(str), -1), 0)
        self.assertEqual(tailmatch(str, '', 0, len(str), 1), 1)
        self.assertEqual(tailmatch(str, '', 0, len(str), -1), 1)

        self.assertEqual(tailmatch(str, 'ba', 0, len(str)-1, -1), 0)
        self.assertEqual(tailmatch(str, 'ba', 1, len(str)-1, -1), 1)
        self.assertEqual(tailmatch(str, 'aba', 1, len(str)-1, -1), 0)
        self.assertEqual(tailmatch(str, 'ba', -len(str)+1, -1, -1), 1)
        self.assertEqual(tailmatch(str, 'ah', 0, len(str), 1), 0)
        self.assertEqual(tailmatch(str, 'ah', 0, len(str)-1, 1), 1)
        self.assertEqual(tailmatch(str, 'ah', -len(str), -1, 1), 1)

        # bad arguments
        self.assertRaises(TypeError, tailmatch, str, ('aba', 'aha'), 0, len(str), -1)
        self.assertRaises(TypeError, tailmatch, str, ('aba', 'aha'), 0, len(str), 1)
        # CRASHES tailmatch(NULL, 'aba', 0, len(str), -1)
        # CRASHES tailmatch(str, NULL, 0, len(str), -1)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_find(self):
        """Test PyUnicode_Find()"""
        from _testlimitedcapi import unicode_find as find

        for str in "\xa1", "\u8000\u8080", "\ud800\udc02", "\U0001f100\U0001f1f1":
            for i, ch in enumerate(str):
                self.assertEqual(find(str, ch, 0, len(str), 1), i)
                self.assertEqual(find(str, ch, 0, len(str), -1), i)

        str = "!>_<!"
        self.assertEqual(find(str, 'z', 0, len(str), 1), -1)
        self.assertEqual(find(str, 'z', 0, len(str), -1), -1)
        self.assertEqual(find(str, '', 0, len(str), 1), 0)
        self.assertEqual(find(str, '', 0, len(str), -1), len(str))
        # start < end
        self.assertEqual(find(str, '!', 1, len(str)+1, 1), 4)
        self.assertEqual(find(str, '!', 1, PY_SSIZE_T_MAX, 1), 4)
        self.assertEqual(find(str, '!', 0, len(str)+1, -1), 4)
        self.assertEqual(find(str, '!', 0, PY_SSIZE_T_MAX, -1), 4)
        # start >= end
        self.assertEqual(find(str, '!', 0, 0, 1), -1)
        self.assertEqual(find(str, '!', 0, 0, -1), -1)
        self.assertEqual(find(str, '!', len(str), 0, 1), -1)
        self.assertEqual(find(str, '!', len(str), 0, -1), -1)
        # negative
        self.assertEqual(find(str, '!', -len(str), -1, 1), 0)
        self.assertEqual(find(str, '!', -len(str), -1, -1), 0)
        self.assertEqual(find(str, '!', PY_SSIZE_T_MIN, -1, 1), 0)
        self.assertEqual(find(str, '!', PY_SSIZE_T_MIN, -1, -1), 0)
        self.assertEqual(find(str, '!', PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, 1), 0)
        self.assertEqual(find(str, '!', PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, -1), 4)
        # bad arguments
        self.assertRaises(TypeError, find, str, b'!', 0, len(str), 1)
        self.assertRaises(TypeError, find, b"!>_<!", '!', 0, len(str), 1)
        self.assertRaises(TypeError, find, str, ord('!'), 0, len(str), 1)
        self.assertRaises(TypeError, find, [], '!', 0, len(str), 1)
        # CRASHES find(NULL, '!', 0, len(str), 1)
        # CRASHES find(str, NULL, 0, len(str), 1)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_findchar(self):
        """Test PyUnicode_FindChar()"""
        from _testlimitedcapi import unicode_findchar

        for str in "\xa1", "\u8000\u8080", "\ud800\udc02", "\U0001f100\U0001f1f1":
            for i, ch in enumerate(str):
                self.assertEqual(unicode_findchar(str, ord(ch), 0, len(str), 1), i)
                self.assertEqual(unicode_findchar(str, ord(ch), 0, len(str), -1), i)

        str = "!>_<!"
        self.assertEqual(unicode_findchar(str, 0x110000, 0, len(str), 1), -1)
        self.assertEqual(unicode_findchar(str, 0x110000, 0, len(str), -1), -1)
        # start < end
        self.assertEqual(unicode_findchar(str, ord('!'), 1, len(str)+1, 1), 4)
        self.assertEqual(unicode_findchar(str, ord('!'), 1, PY_SSIZE_T_MAX, 1), 4)
        self.assertEqual(unicode_findchar(str, ord('!'), 0, len(str)+1, -1), 4)
        self.assertEqual(unicode_findchar(str, ord('!'), 0, PY_SSIZE_T_MAX, -1), 4)
        # start >= end
        self.assertEqual(unicode_findchar(str, ord('!'), 0, 0, 1), -1)
        self.assertEqual(unicode_findchar(str, ord('!'), 0, 0, -1), -1)
        self.assertEqual(unicode_findchar(str, ord('!'), len(str), 0, 1), -1)
        self.assertEqual(unicode_findchar(str, ord('!'), len(str), 0, -1), -1)
        # negative
        self.assertEqual(unicode_findchar(str, ord('!'), -len(str), -1, 1), 0)
        self.assertEqual(unicode_findchar(str, ord('!'), -len(str), -1, -1), 0)
        self.assertEqual(unicode_findchar(str, ord('!'), PY_SSIZE_T_MIN, -1, 1), 0)
        self.assertEqual(unicode_findchar(str, ord('!'), PY_SSIZE_T_MIN, -1, -1), 0)
        self.assertEqual(unicode_findchar(str, ord('!'), PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, 1), 0)
        self.assertEqual(unicode_findchar(str, ord('!'), PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, -1), 4)
        # bad arguments
        # CRASHES unicode_findchar(b"!>_<!", ord('!'), 0, len(str), 1)
        # CRASHES unicode_findchar([], ord('!'), 0, len(str), 1)
        # CRASHES unicode_findchar(NULL, ord('!'), 0, len(str), 1), 1)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_replace(self):
        """Test PyUnicode_Replace()"""
        from _testlimitedcapi import unicode_replace as replace

        str = 'abracadabra'
        self.assertEqual(replace(str, 'a', '='), '=br=c=d=br=')
        self.assertEqual(replace(str, 'a', '<>'), '<>br<>c<>d<>br<>')
        self.assertEqual(replace(str, 'abra', '='), '=cad=')
        self.assertEqual(replace(str, 'a', '=', 2), '=br=cadabra')
        self.assertEqual(replace(str, 'a', '=', 0), str)
        self.assertEqual(replace(str, 'a', '=', PY_SSIZE_T_MAX), '=br=c=d=br=')
        self.assertEqual(replace(str, 'a', '=', -1), '=br=c=d=br=')
        self.assertEqual(replace(str, 'a', '=', PY_SSIZE_T_MIN), '=br=c=d=br=')
        self.assertEqual(replace(str, 'z', '='), str)
        self.assertEqual(replace(str, '', '='), '=a=b=r=a=c=a=d=a=b=r=a=')
        self.assertEqual(replace(str, 'a', 'ж'), 'жbrжcжdжbrж')
        self.assertEqual(replace('абабагаламага', 'а', '='), '=б=б=г=л=м=г=')
        self.assertEqual(replace('Баден-Баден', 'Баден', 'Baden'), 'Baden-Baden')
        # bad arguments
        self.assertRaises(TypeError, replace, 'a', 'a', b'=')
        self.assertRaises(TypeError, replace, 'a', b'a', '=')
        self.assertRaises(TypeError, replace, b'a', 'a', '=')
        self.assertRaises(TypeError, replace, 'a', 'a', ord('='))
        self.assertRaises(TypeError, replace, 'a', ord('a'), '=')
        self.assertRaises(TypeError, replace, [], 'a', '=')
        # CRASHES replace('a', 'a', NULL)
        # CRASHES replace('a', NULL, '=')
        # CRASHES replace(NULL, 'a', '=')

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_compare(self):
        """Test PyUnicode_Compare()"""
        from _testlimitedcapi import unicode_compare as compare

        self.assertEqual(compare('abc', 'abc'), 0)
        self.assertEqual(compare('abc', 'def'), -1)
        self.assertEqual(compare('def', 'abc'), 1)
        self.assertEqual(compare('abc', 'abc\0def'), -1)
        self.assertEqual(compare('abc\0def', 'abc\0def'), 0)
        self.assertEqual(compare('абв', 'abc'), 1)

        self.assertRaises(TypeError, compare, b'abc', 'abc')
        self.assertRaises(TypeError, compare, 'abc', b'abc')
        self.assertRaises(TypeError, compare, b'abc', b'abc')
        self.assertRaises(TypeError, compare, [], 'abc')
        self.assertRaises(TypeError, compare, 'abc', [])
        self.assertRaises(TypeError, compare, [], [])
        # CRASHES compare(NULL, 'abc')
        # CRASHES compare('abc', NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_comparewithasciistring(self):
        """Test PyUnicode_CompareWithASCIIString()"""
        from _testlimitedcapi import unicode_comparewithasciistring as comparewithasciistring

        self.assertEqual(comparewithasciistring('abc', b'abc'), 0)
        self.assertEqual(comparewithasciistring('abc', b'def'), -1)
        self.assertEqual(comparewithasciistring('def', b'abc'), 1)
        self.assertEqual(comparewithasciistring('abc', b'abc\0def'), 0)
        self.assertEqual(comparewithasciistring('abc\0def', b'abc\0def'), 1)
        self.assertEqual(comparewithasciistring('абв', b'abc'), 1)

        # CRASHES comparewithasciistring(b'abc', b'abc')
        # CRASHES comparewithasciistring([], b'abc')
        # CRASHES comparewithasciistring(NULL, b'abc')

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_equaltoutf8(self):
        # Test PyUnicode_EqualToUTF8()
        from _testlimitedcapi import unicode_equaltoutf8 as equaltoutf8
        from _testlimitedcapi import unicode_asutf8andsize as asutf8andsize

        strings = [
            'abc', '\xa1\xa2\xa3', '\u4f60\u597d\u4e16',
            '\U0001f600\U0001f601\U0001f602',
            '\U0010ffff',
        ]
        for s in strings:
            # Call PyUnicode_AsUTF8AndSize() which creates the UTF-8
            # encoded string cached in the Unicode object.
            asutf8andsize(s, 0)
            b = s.encode()
            self.assertEqual(equaltoutf8(s, b), 1)  # Use the UTF-8 cache.
            s2 = b.decode()  # New Unicode object without the UTF-8 cache.
            self.assertEqual(equaltoutf8(s2, b), 1)
            self.assertEqual(equaltoutf8(s + 'x', b + b'x'), 1)
            self.assertEqual(equaltoutf8(s + 'x', b + b'y'), 0)
            self.assertEqual(equaltoutf8(s, b + b'\0'), 1)
            self.assertEqual(equaltoutf8(s2, b + b'\0'), 1)
            self.assertEqual(equaltoutf8(s + '\0', b + b'\0'), 0)
            self.assertEqual(equaltoutf8(s + '\0', b), 0)
            self.assertEqual(equaltoutf8(s2, b + b'x'), 0)
            self.assertEqual(equaltoutf8(s2, b[:-1]), 0)
            self.assertEqual(equaltoutf8(s2, b[:-1] + b'x'), 0)

        self.assertEqual(equaltoutf8('', b''), 1)
        self.assertEqual(equaltoutf8('', b'\0'), 1)

        # embedded null chars/bytes
        self.assertEqual(equaltoutf8('abc', b'abc\0def\0'), 1)
        self.assertEqual(equaltoutf8('a\0bc', b'abc'), 0)
        self.assertEqual(equaltoutf8('abc', b'a\0bc'), 0)

        # Surrogate characters are always treated as not equal
        self.assertEqual(equaltoutf8('\udcfe',
                            '\udcfe'.encode("utf8", "surrogateescape")), 0)
        self.assertEqual(equaltoutf8('\udcfe',
                            '\udcfe'.encode("utf8", "surrogatepass")), 0)
        self.assertEqual(equaltoutf8('\ud801',
                            '\ud801'.encode("utf8", "surrogatepass")), 0)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_equaltoutf8andsize(self):
        # Test PyUnicode_EqualToUTF8AndSize()
        from _testlimitedcapi import unicode_equaltoutf8andsize as equaltoutf8andsize
        from _testlimitedcapi import unicode_asutf8andsize as asutf8andsize

        strings = [
            'abc', '\xa1\xa2\xa3', '\u4f60\u597d\u4e16',
            '\U0001f600\U0001f601\U0001f602',
            '\U0010ffff',
        ]
        for s in strings:
            # Call PyUnicode_AsUTF8AndSize() which creates the UTF-8
            # encoded string cached in the Unicode object.
            asutf8andsize(s, 0)
            b = s.encode()
            self.assertEqual(equaltoutf8andsize(s, b), 1)  # Use the UTF-8 cache.
            s2 = b.decode()  # New Unicode object without the UTF-8 cache.
            self.assertEqual(equaltoutf8andsize(s2, b), 1)
            self.assertEqual(equaltoutf8andsize(s + 'x', b + b'x'), 1)
            self.assertEqual(equaltoutf8andsize(s + 'x', b + b'y'), 0)
            self.assertEqual(equaltoutf8andsize(s, b + b'\0'), 0)
            self.assertEqual(equaltoutf8andsize(s2, b + b'\0'), 0)
            self.assertEqual(equaltoutf8andsize(s + '\0', b + b'\0'), 1)
            self.assertEqual(equaltoutf8andsize(s + '\0', b), 0)
            self.assertEqual(equaltoutf8andsize(s2, b + b'x'), 0)
            self.assertEqual(equaltoutf8andsize(s2, b[:-1]), 0)
            self.assertEqual(equaltoutf8andsize(s2, b[:-1] + b'x'), 0)
            # Not null-terminated,
            self.assertEqual(equaltoutf8andsize(s, b + b'x', len(b)), 1)
            self.assertEqual(equaltoutf8andsize(s2, b + b'x', len(b)), 1)
            self.assertEqual(equaltoutf8andsize(s + '\0', b + b'\0x', len(b) + 1), 1)
            self.assertEqual(equaltoutf8andsize(s2, b, len(b) - 1), 0)
            self.assertEqual(equaltoutf8andsize(s, b, -1), 0)
            self.assertEqual(equaltoutf8andsize(s, b, PY_SSIZE_T_MAX), 0)
            self.assertEqual(equaltoutf8andsize(s, b, PY_SSIZE_T_MIN), 0)

        self.assertEqual(equaltoutf8andsize('', b''), 1)
        self.assertEqual(equaltoutf8andsize('', b'\0'), 0)
        self.assertEqual(equaltoutf8andsize('', b'x', 0), 1)

        # embedded null chars/bytes
        self.assertEqual(equaltoutf8andsize('abc\0def', b'abc\0def'), 1)
        self.assertEqual(equaltoutf8andsize('abc\0def\0', b'abc\0def\0'), 1)

        # Surrogate characters are always treated as not equal
        self.assertEqual(equaltoutf8andsize('\udcfe',
                            '\udcfe'.encode("utf8", "surrogateescape")), 0)
        self.assertEqual(equaltoutf8andsize('\udcfe',
                            '\udcfe'.encode("utf8", "surrogatepass")), 0)
        self.assertEqual(equaltoutf8andsize('\ud801',
                            '\ud801'.encode("utf8", "surrogatepass")), 0)

        def check_not_equal_encoding(text, encoding):
            self.assertEqual(equaltoutf8andsize(text, text.encode(encoding)), 0)
            self.assertNotEqual(text.encode(encoding), text.encode("utf8"))

        # Strings encoded to other encodings are not equal to expected UTF8-encoding string
        check_not_equal_encoding('Stéphane', 'latin1')
        check_not_equal_encoding('Stéphane', 'utf-16-le')  # embedded null characters
        check_not_equal_encoding('北京市', 'gbk')

        # CRASHES equaltoutf8andsize('abc', b'abc', -1)
        # CRASHES equaltoutf8andsize(b'abc', b'abc')
        # CRASHES equaltoutf8andsize([], b'abc')
        # CRASHES equaltoutf8andsize(NULL, b'abc')
        # CRASHES equaltoutf8andsize('abc', NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_richcompare(self):
        """Test PyUnicode_RichCompare()"""
        from _testlimitedcapi import unicode_richcompare as richcompare

        LT, LE, EQ, NE, GT, GE = range(6)
        strings = ('abc', 'абв', '\U0001f600', 'abc\0')
        for s1 in strings:
            for s2 in strings:
                self.assertIs(richcompare(s1, s2, LT), s1 < s2)
                self.assertIs(richcompare(s1, s2, LE), s1 <= s2)
                self.assertIs(richcompare(s1, s2, EQ), s1 == s2)
                self.assertIs(richcompare(s1, s2, NE), s1 != s2)
                self.assertIs(richcompare(s1, s2, GT), s1 > s2)
                self.assertIs(richcompare(s1, s2, GE), s1 >= s2)

        for op in LT, LE, EQ, NE, GT, GE:
            self.assertIs(richcompare(b'abc', 'abc', op), NotImplemented)
            self.assertIs(richcompare('abc', b'abc', op), NotImplemented)
            self.assertIs(richcompare(b'abc', b'abc', op), NotImplemented)
            self.assertIs(richcompare([], 'abc', op), NotImplemented)
            self.assertIs(richcompare('abc', [], op), NotImplemented)
            self.assertIs(richcompare([], [], op), NotImplemented)

            # CRASHES richcompare(NULL, 'abc', op)
            # CRASHES richcompare('abc', NULL, op)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_format(self):
        """Test PyUnicode_Format()"""
        from _testlimitedcapi import unicode_format as format

        self.assertEqual(format('x=%d!', 42), 'x=42!')
        self.assertEqual(format('x=%d!', (42,)), 'x=42!')
        self.assertEqual(format('x=%d y=%s!', (42, [])), 'x=42 y=[]!')

        self.assertRaises(SystemError, format, 'x=%d!', NULL)
        self.assertRaises(SystemError, format, NULL, 42)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_contains(self):
        """Test PyUnicode_Contains()"""
        from _testlimitedcapi import unicode_contains as contains

        self.assertEqual(contains('abcd', ''), 1)
        self.assertEqual(contains('abcd', 'b'), 1)
        self.assertEqual(contains('abcd', 'x'), 0)
        self.assertEqual(contains('abcd', 'ж'), 0)
        self.assertEqual(contains('abcd', '\0'), 0)
        self.assertEqual(contains('abc\0def', '\0'), 1)
        self.assertEqual(contains('abcd', 'bc'), 1)

        self.assertRaises(TypeError, contains, b'abcd', 'b')
        self.assertRaises(TypeError, contains, 'abcd', b'b')
        self.assertRaises(TypeError, contains, b'abcd', b'b')
        self.assertRaises(TypeError, contains, [], 'b')
        self.assertRaises(TypeError, contains, 'abcd', ord('b'))
        # CRASHES contains(NULL, 'b')
        # CRASHES contains('abcd', NULL)

    @support.cpython_only
    @unittest.skipIf(_testlimitedcapi is None, 'need _testlimitedcapi module')
    def test_isidentifier(self):
        """Test PyUnicode_IsIdentifier()"""
        from _testlimitedcapi import unicode_isidentifier as isidentifier

        self.assertEqual(isidentifier("a"), 1)
        self.assertEqual(isidentifier("b0"), 1)
        self.assertEqual(isidentifier("µ"), 1)
        self.assertEqual(isidentifier("𝔘𝔫𝔦𝔠𝔬𝔡𝔢"), 1)

        self.assertEqual(isidentifier(""), 0)
        self.assertEqual(isidentifier(" "), 0)
        self.assertEqual(isidentifier("["), 0)
        self.assertEqual(isidentifier("©"), 0)
        self.assertEqual(isidentifier("0"), 0)
        self.assertEqual(isidentifier("32M"), 0)

        # CRASHES isidentifier(b"a")
        # CRASHES isidentifier([])
        # CRASHES isidentifier(NULL)

    @support.cpython_only
    @unittest.skipIf(_testcapi is None, 'need _testcapi module')
    def test_copycharacters(self):
        """Test PyUnicode_CopyCharacters()"""
        from _testcapi import unicode_copycharacters

        strings = [
            # all strings have exactly 5 characters
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
        self.assertRaises(IndexError, unicode_copycharacters, s, PY_SSIZE_T_MAX, s, 0, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, -1, s, 0, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, PY_SSIZE_T_MIN, s, 0, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, 0, s, 6, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, 0, s, PY_SSIZE_T_MAX, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, 0, s, -1, 5)
        self.assertRaises(IndexError, unicode_copycharacters, s, 0, s, PY_SSIZE_T_MIN, 5)
        self.assertRaises(SystemError, unicode_copycharacters, s, 1, s, 0, 5)
        self.assertRaises(SystemError, unicode_copycharacters, s, 1, s, 0, PY_SSIZE_T_MAX)
        self.assertRaises(SystemError, unicode_copycharacters, s, 0, s, 0, -1)
        self.assertRaises(SystemError, unicode_copycharacters, s, 0, s, 0, PY_SSIZE_T_MIN)
        self.assertRaises(SystemError, unicode_copycharacters, s, 0, b'', 0, 0)
        self.assertRaises(SystemError, unicode_copycharacters, s, 0, [], 0, 0)
        # CRASHES unicode_copycharacters(s, 0, NULL, 0, 0)
        # TODO: Test PyUnicode_CopyCharacters() with non-unicode and
        # non-modifiable unicode as "to".

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


class PyUnicodeWriterTest(unittest.TestCase):
    def create_writer(self, size):
        return _testcapi.PyUnicodeWriter(size)

    def test_basic(self):
        writer = self.create_writer(100)

        # test PyUnicodeWriter_WriteUTF8()
        writer.write_utf8(b'var', -1)

        # test PyUnicodeWriter_WriteChar()
        writer.write_char('=')

        # test PyUnicodeWriter_WriteSubstring()
        writer.write_substring("[long]", 1, 5)

        # test PyUnicodeWriter_WriteStr()
        writer.write_str(" value ")

        # test PyUnicodeWriter_WriteRepr()
        writer.write_repr("repr")

        self.assertEqual(writer.finish(),
                         "var=long value 'repr'")

    def test_utf8(self):
        writer = self.create_writer(0)
        writer.write_utf8(b"ascii", -1)
        writer.write_char('-')
        writer.write_utf8(b"latin1=\xC3\xA9", -1)
        writer.write_char('-')
        writer.write_utf8(b"euro=\xE2\x82\xAC", -1)
        writer.write_char('.')
        self.assertEqual(writer.finish(),
                         "ascii-latin1=\xE9-euro=\u20AC.")

    def test_invalid_utf8(self):
        writer = self.create_writer(0)
        with self.assertRaises(UnicodeDecodeError):
            writer.write_utf8(b"invalid=\xFF", -1)

    def test_recover_utf8_error(self):
        # test recovering from PyUnicodeWriter_WriteUTF8() error
        writer = self.create_writer(0)
        writer.write_utf8(b"value=", -1)

        # write fails with an invalid string
        with self.assertRaises(UnicodeDecodeError):
            writer.write_utf8(b"invalid\xFF", -1)

        # retry write with a valid string
        writer.write_utf8(b"valid", -1)

        self.assertEqual(writer.finish(),
                         "value=valid")

    def test_decode_utf8(self):
        # test PyUnicodeWriter_DecodeUTF8Stateful()
        writer = self.create_writer(0)
        writer.decodeutf8stateful(b"ign\xFFore", -1, b"ignore")
        writer.write_char('-')
        writer.decodeutf8stateful(b"replace\xFF", -1, b"replace")
        writer.write_char('-')

        # incomplete trailing UTF-8 sequence
        writer.decodeutf8stateful(b"incomplete\xC3", -1, b"replace")

        self.assertEqual(writer.finish(),
                         "ignore-replace\uFFFD-incomplete\uFFFD")

    def test_decode_utf8_consumed(self):
        # test PyUnicodeWriter_DecodeUTF8Stateful() with consumed
        writer = self.create_writer(0)

        # valid string
        consumed = writer.decodeutf8stateful(b"text", -1, b"strict", True)
        self.assertEqual(consumed, 4)
        writer.write_char('-')

        # non-ASCII
        consumed = writer.decodeutf8stateful(b"\xC3\xA9-\xE2\x82\xAC", 6, b"strict", True)
        self.assertEqual(consumed, 6)
        writer.write_char('-')

        # invalid UTF-8 (consumed is 0 on error)
        with self.assertRaises(UnicodeDecodeError):
            writer.decodeutf8stateful(b"invalid\xFF", -1, b"strict", True)

        # ignore error handler
        consumed = writer.decodeutf8stateful(b"more\xFF", -1, b"ignore", True)
        self.assertEqual(consumed, 5)
        writer.write_char('-')

        # incomplete trailing UTF-8 sequence
        consumed = writer.decodeutf8stateful(b"incomplete\xC3", -1, b"ignore", True)
        self.assertEqual(consumed, 10)

        self.assertEqual(writer.finish(), "text-\xE9-\u20AC-more-incomplete")

    def test_widechar(self):
        writer = self.create_writer(0)
        writer.write_widechar("latin1=\xE9")
        writer.write_widechar("-")
        writer.write_widechar("euro=\u20AC")
        writer.write_char("-")
        writer.write_widechar("max=\U0010ffff")
        writer.write_char('.')
        self.assertEqual(writer.finish(),
                         "latin1=\xE9-euro=\u20AC-max=\U0010ffff.")

    def test_ucs4(self):
        writer = self.create_writer(0)
        writer.write_ucs4("ascii IGNORED", 5)
        writer.write_char("-")
        writer.write_ucs4("latin1=\xe9", 8)
        writer.write_char("-")
        writer.write_ucs4("euro=\u20ac", 6)
        writer.write_char("-")
        writer.write_ucs4("max=\U0010ffff", 5)
        writer.write_char(".")
        self.assertEqual(writer.finish(),
                         "ascii-latin1=\xE9-euro=\u20AC-max=\U0010ffff.")

        # Test some special characters
        writer = self.create_writer(0)
        # Lone surrogate character
        writer.write_ucs4("lone\uDC80", 5)
        writer.write_char("-")
        # Surrogate pair
        writer.write_ucs4("pair\uDBFF\uDFFF", 5)
        writer.write_char("-")
        writer.write_ucs4("null[\0]", 7)
        self.assertEqual(writer.finish(),
                         "lone\udc80-pair\udbff-null[\0]")

        # invalid size
        writer = self.create_writer(0)
        with self.assertRaises(ValueError):
            writer.write_ucs4("text", -1)

    def test_substring_empty(self):
        writer = self.create_writer(0)
        writer.write_substring("abc", 1, 1)
        self.assertEqual(writer.finish(), '')


@unittest.skipIf(ctypes is None, 'need ctypes')
class PyUnicodeWriterFormatTest(unittest.TestCase):
    def create_writer(self, size):
        return _testcapi.PyUnicodeWriter(size)

    def writer_format(self, writer, *args):
        from ctypes import c_char_p, pythonapi, c_int, c_void_p
        _PyUnicodeWriter_Format = getattr(pythonapi, "PyUnicodeWriter_Format")
        _PyUnicodeWriter_Format.argtypes = (c_void_p, c_char_p,)
        _PyUnicodeWriter_Format.restype = c_int

        if _PyUnicodeWriter_Format(writer.get_pointer(), *args) < 0:
            raise ValueError("PyUnicodeWriter_Format failed")

    def test_format(self):
        from ctypes import c_int
        writer = self.create_writer(0)
        self.writer_format(writer, b'%s %i', b'abc', c_int(123))
        writer.write_char('.')
        self.assertEqual(writer.finish(), 'abc 123.')

    def test_recover_error(self):
        # test recovering from PyUnicodeWriter_Format() error
        writer = self.create_writer(0)
        self.writer_format(writer, b"%s ", b"Hello")

        # PyUnicodeWriter_Format() fails with an invalid format string
        with self.assertRaises(ValueError):
            self.writer_format(writer, b"%s\xff", b"World")

        # Retry PyUnicodeWriter_Format() with a valid format string
        self.writer_format(writer, b"%s.", b"World")

        self.assertEqual(writer.finish(), 'Hello World.')

    def test_unicode_equal(self):
        unicode_equal = _testlimitedcapi.unicode_equal

        def copy(text):
            return text.encode().decode()

        self.assertTrue(unicode_equal("", ""))
        self.assertTrue(unicode_equal("abc", "abc"))
        self.assertTrue(unicode_equal("abc", copy("abc")))
        self.assertTrue(unicode_equal("\u20ac", copy("\u20ac")))
        self.assertTrue(unicode_equal("\U0010ffff", copy("\U0010ffff")))

        self.assertFalse(unicode_equal("abc", "abcd"))
        self.assertFalse(unicode_equal("\u20ac", "\u20ad"))
        self.assertFalse(unicode_equal("\U0010ffff", "\U0010fffe"))

        # str subclass
        self.assertTrue(unicode_equal("abc", Str("abc")))
        self.assertTrue(unicode_equal(Str("abc"), "abc"))
        self.assertFalse(unicode_equal("abc", Str("abcd")))
        self.assertFalse(unicode_equal(Str("abc"), "abcd"))

        # invalid type
        for invalid_type in (b'bytes', 123, ("tuple",)):
            with self.subTest(invalid_type=invalid_type):
                with self.assertRaises(TypeError):
                    unicode_equal("abc", invalid_type)
                with self.assertRaises(TypeError):
                    unicode_equal(invalid_type, "abc")

        # CRASHES unicode_equal("abc", NULL)
        # CRASHES unicode_equal(NULL, "abc")


if __name__ == "__main__":
    unittest.main()
