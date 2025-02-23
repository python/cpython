import codecs
import contextlib
import io
import re
import sys
import unittest
import unittest.mock as mock
import _testcapi
from test.support import import_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')

NULL = None
BAD_ARGUMENT = re.escape('bad argument type for built-in operation')


class CAPIUnicodeTest(unittest.TestCase):
    # TODO: Test the following functions:
    #
    #   PyUnicode_BuildEncodingMap
    #   PyUnicode_FSConverter
    #   PyUnicode_FSDecoder
    #   PyUnicode_DecodeMBCS
    #   PyUnicode_DecodeMBCSStateful
    #   PyUnicode_DecodeCodePageStateful
    #   PyUnicode_AsMBCSString
    #   PyUnicode_EncodeCodePage
    #   PyUnicode_DecodeLocaleAndSize
    #   PyUnicode_DecodeLocale
    #   PyUnicode_EncodeLocale
    #   PyUnicode_DecodeFSDefault
    #   PyUnicode_DecodeFSDefaultAndSize
    #   PyUnicode_EncodeFSDefault

    def test_fromencodedobject(self):
        """Test PyUnicode_FromEncodedObject()"""
        fromencodedobject = _testlimitedcapi.unicode_fromencodedobject

        self.assertEqual(fromencodedobject(b'abc', NULL), 'abc')
        self.assertEqual(fromencodedobject(b'abc', 'ascii'), 'abc')
        b = b'a\xc2\xa1\xe4\xbd\xa0\xf0\x9f\x98\x80'
        s = 'a\xa1\u4f60\U0001f600'
        self.assertEqual(fromencodedobject(b, NULL), s)
        self.assertEqual(fromencodedobject(b, 'utf-8'), s)
        self.assertEqual(fromencodedobject(b, 'latin1'), b.decode('latin1'))
        self.assertRaises(UnicodeDecodeError, fromencodedobject, b, 'ascii')
        self.assertEqual(fromencodedobject(b, 'ascii', 'replace'),
                         'a' + '\ufffd'*9)
        self.assertEqual(fromencodedobject(bytearray(b), NULL), s)
        self.assertEqual(fromencodedobject(bytearray(b), 'utf-8'), s)
        self.assertRaises(LookupError, fromencodedobject, b'abc', 'foo')
        self.assertRaises(LookupError, fromencodedobject, b, 'ascii', 'foo')
        self.assertRaises(TypeError, fromencodedobject, 'abc', NULL)
        self.assertRaises(TypeError, fromencodedobject, 'abc', 'ascii')
        self.assertRaises(TypeError, fromencodedobject, [], NULL)
        self.assertRaises(TypeError, fromencodedobject, [], 'ascii')
        self.assertRaises(SystemError, fromencodedobject, NULL, NULL)
        self.assertRaises(SystemError, fromencodedobject, NULL, 'ascii')

    def test_decode(self):
        """Test PyUnicode_Decode()"""
        decode = _testlimitedcapi.unicode_decode

        self.assertEqual(decode(b'[\xe2\x82\xac]', 'utf-8'), '[\u20ac]')
        self.assertEqual(decode(b'[\xa4]', 'iso8859-15'), '[\u20ac]')
        self.assertEqual(decode(b'[\xa4]', 'iso8859-15', 'strict'), '[\u20ac]')
        self.assertRaises(UnicodeDecodeError, decode, b'[\xa4]', 'utf-8')
        self.assertEqual(decode(b'[\xa4]', 'utf-8', 'replace'), '[\ufffd]')

        self.assertEqual(decode(b'[\xe2\x82\xac]', NULL), '[\u20ac]')
        self.assertEqual(decode(b'[\xa4]', NULL, 'replace'), '[\ufffd]')

        self.assertRaises(LookupError, decode, b'\xa4', 'foo')
        self.assertRaises(LookupError, decode, b'\xa4', 'utf-8', 'foo')
        # TODO: Test PyUnicode_Decode() with NULL as data and
        # negative size.

    def test_asencodedstring(self):
        """Test PyUnicode_AsEncodedString()"""
        asencodedstring = _testlimitedcapi.unicode_asencodedstring

        self.assertEqual(asencodedstring('abc', NULL), b'abc')
        self.assertEqual(asencodedstring('abc', 'ascii'), b'abc')
        s = 'a\xa1\u4f60\U0001f600'
        b = b'a\xc2\xa1\xe4\xbd\xa0\xf0\x9f\x98\x80'
        self.assertEqual(asencodedstring(s, NULL), b)
        self.assertEqual(asencodedstring(s, 'utf-8'), b)
        self.assertEqual(asencodedstring('\xa1\xa2', 'latin1'), b'\xa1\xa2')
        self.assertRaises(UnicodeEncodeError, asencodedstring, '\xa1\xa2', 'ascii')
        self.assertEqual(asencodedstring(s, 'ascii', 'replace'), b'a???')

        self.assertRaises(LookupError, asencodedstring, 'abc', 'foo')
        self.assertRaises(LookupError, asencodedstring, s, 'ascii', 'foo')
        self.assertRaises(TypeError, asencodedstring, b'abc', NULL)
        self.assertRaises(TypeError, asencodedstring, b'abc', 'ascii')
        self.assertRaises(TypeError, asencodedstring, [], NULL)
        self.assertRaises(TypeError, asencodedstring, [], 'ascii')
        # CRASHES asencodedstring(NULL, NULL)
        # CRASHES asencodedstring(NULL, 'ascii')

    def test_decodeutf8(self):
        """Test PyUnicode_DecodeUTF8()"""
        decodeutf8 = _testlimitedcapi.unicode_decodeutf8

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            b = s.encode('utf-8')
            self.assertEqual(decodeutf8(b), s)
            self.assertEqual(decodeutf8(b, 'strict'), s)

        self.assertRaises(UnicodeDecodeError, decodeutf8, b'\x80')
        self.assertRaises(UnicodeDecodeError, decodeutf8, b'\xc0')
        self.assertRaises(UnicodeDecodeError, decodeutf8, b'\xff')
        self.assertRaises(UnicodeDecodeError, decodeutf8, b'a\xf0\x9f')
        self.assertEqual(decodeutf8(b'a\xf0\x9f', 'replace'), 'a\ufffd')
        self.assertEqual(decodeutf8(b'a\xf0\x9fb', 'replace'), 'a\ufffdb')

        self.assertRaises(LookupError, decodeutf8, b'a\x80', 'foo')
        # TODO: Test PyUnicode_DecodeUTF8() with NULL as data and
        # negative size.

    def test_decodeutf8stateful(self):
        """Test PyUnicode_DecodeUTF8Stateful()"""
        decodeutf8stateful = _testlimitedcapi.unicode_decodeutf8stateful

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            b = s.encode('utf-8')
            self.assertEqual(decodeutf8stateful(b), (s, len(b)))
            self.assertEqual(decodeutf8stateful(b, 'strict'), (s, len(b)))

        self.assertRaises(UnicodeDecodeError, decodeutf8stateful, b'\x80')
        self.assertRaises(UnicodeDecodeError, decodeutf8stateful, b'\xc0')
        self.assertRaises(UnicodeDecodeError, decodeutf8stateful, b'\xff')
        self.assertEqual(decodeutf8stateful(b'a\xf0\x9f'), ('a', 1))
        self.assertEqual(decodeutf8stateful(b'a\xf0\x9f', 'replace'), ('a', 1))
        self.assertRaises(UnicodeDecodeError, decodeutf8stateful, b'a\xf0\x9fb')
        self.assertEqual(decodeutf8stateful(b'a\xf0\x9fb', 'replace'), ('a\ufffdb', 4))

        self.assertRaises(LookupError, decodeutf8stateful, b'a\x80', 'foo')
        # TODO: Test PyUnicode_DecodeUTF8Stateful() with NULL as data and
        # negative size.
        # TODO: Test PyUnicode_DecodeUTF8Stateful() with NULL as the address of
        # "consumed".

    def test_asutf8string(self):
        """Test PyUnicode_AsUTF8String()"""
        asutf8string = _testlimitedcapi.unicode_asutf8string

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            self.assertEqual(asutf8string(s), s.encode('utf-8'))

        self.assertRaises(UnicodeEncodeError, asutf8string, '\ud8ff')
        self.assertRaises(TypeError, asutf8string, b'abc')
        self.assertRaises(TypeError, asutf8string, [])
        # CRASHES asutf8string(NULL)

    def test_decodeutf16(self):
        """Test PyUnicode_DecodeUTF16()"""
        decodeutf16 = _testlimitedcapi.unicode_decodeutf16

        naturalbyteorder = -1 if sys.byteorder == 'little' else 1
        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            b = s.encode('utf-16')
            self.assertEqual(decodeutf16(0, b), (naturalbyteorder, s))
            b = s.encode('utf-16le')
            self.assertEqual(decodeutf16(-1, b), (-1, s))
            self.assertEqual(decodeutf16(0, b'\xff\xfe'+b), (-1, s))
            b = s.encode('utf-16be')
            self.assertEqual(decodeutf16(1, b), (1, s))
            self.assertEqual(decodeutf16(0, b'\xfe\xff'+b), (1, s))

        self.assertRaises(UnicodeDecodeError, decodeutf16, -1, b'a')
        self.assertRaises(UnicodeDecodeError, decodeutf16, 1, b'a')
        self.assertRaises(UnicodeDecodeError, decodeutf16, 0, b'\xff\xfea')
        self.assertRaises(UnicodeDecodeError, decodeutf16, 0, b'\xfe\xffa')

        self.assertRaises(UnicodeDecodeError, decodeutf16, -1, b'\x00\xde')
        self.assertRaises(UnicodeDecodeError, decodeutf16, 1, b'\xde\x00')
        self.assertRaises(UnicodeDecodeError, decodeutf16, 0, b'\xde\xde')
        self.assertEqual(decodeutf16(-1, b'\x00\xde', 'replace'), (-1, '\ufffd'))
        self.assertEqual(decodeutf16(1, b'\xde\x00', 'replace'), (1, '\ufffd'))
        self.assertEqual(decodeutf16(0, b'\xde\xde', 'replace'), (0, '\ufffd'))
        self.assertEqual(decodeutf16(0, b'\xff\xfe\x00\xde', 'replace'), (-1, '\ufffd'))
        self.assertEqual(decodeutf16(0, b'\xfe\xff\xde\x00', 'replace'), (1, '\ufffd'))

        self.assertRaises(UnicodeDecodeError, decodeutf16, -1, b'\x3d\xd8')
        self.assertRaises(UnicodeDecodeError, decodeutf16, 1, b'\xd8\x3d')
        self.assertRaises(UnicodeDecodeError, decodeutf16, 0, b'\xd8\xd8')
        self.assertEqual(decodeutf16(-1, b'\x3d\xd8', 'replace'), (-1, '\ufffd'))
        self.assertEqual(decodeutf16(1, b'\xd8\x3d', 'replace'), (1, '\ufffd'))
        self.assertEqual(decodeutf16(0, b'\xd8\xd8', 'replace'), (0, '\ufffd'))
        self.assertEqual(decodeutf16(0, b'\xff\xfe\x3d\xd8', 'replace'), (-1, '\ufffd'))
        self.assertEqual(decodeutf16(0, b'\xfe\xff\xd8\x3d', 'replace'), (1, '\ufffd'))

        self.assertRaises(LookupError, decodeutf16, -1, b'\x00\xde', 'foo')
        self.assertRaises(LookupError, decodeutf16, 1, b'\xde\x00', 'foo')
        self.assertRaises(LookupError, decodeutf16, 0, b'\xde\xde', 'foo')
        # TODO: Test PyUnicode_DecodeUTF16() with NULL as data and
        # negative size.

    def test_decodeutf16stateful(self):
        """Test PyUnicode_DecodeUTF16Stateful()"""
        decodeutf16stateful = _testlimitedcapi.unicode_decodeutf16stateful

        naturalbyteorder = -1 if sys.byteorder == 'little' else 1
        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            b = s.encode('utf-16')
            self.assertEqual(decodeutf16stateful(0, b), (naturalbyteorder, s, len(b)))
            b = s.encode('utf-16le')
            self.assertEqual(decodeutf16stateful(-1, b), (-1, s, len(b)))
            self.assertEqual(decodeutf16stateful(0, b'\xff\xfe'+b), (-1, s, len(b)+2))
            b = s.encode('utf-16be')
            self.assertEqual(decodeutf16stateful(1, b), (1, s, len(b)))
            self.assertEqual(decodeutf16stateful(0, b'\xfe\xff'+b), (1, s, len(b)+2))

        self.assertEqual(decodeutf16stateful(-1, b'\x61\x00\x3d'), (-1, 'a', 2))
        self.assertEqual(decodeutf16stateful(-1, b'\x61\x00\x3d\xd8'), (-1, 'a', 2))
        self.assertEqual(decodeutf16stateful(-1, b'\x61\x00\x3d\xd8\x00'), (-1, 'a', 2))
        self.assertEqual(decodeutf16stateful(1, b'\x00\x61\xd8'), (1, 'a', 2))
        self.assertEqual(decodeutf16stateful(1, b'\x00\x61\xd8\x3d'), (1, 'a', 2))
        self.assertEqual(decodeutf16stateful(1, b'\x00\x61\xd8\x3d\xde'), (1, 'a', 2))
        self.assertEqual(decodeutf16stateful(0, b'\xff\xfe\x61\x00\x3d\xd8\x00'), (-1, 'a', 4))
        self.assertEqual(decodeutf16stateful(0, b'\xfe\xff\x00\x61\xd8\x3d\xde'), (1, 'a', 4))

        self.assertRaises(UnicodeDecodeError, decodeutf16stateful, -1, b'\x00\xde')
        self.assertRaises(UnicodeDecodeError, decodeutf16stateful, 1, b'\xde\x00')
        self.assertRaises(UnicodeDecodeError, decodeutf16stateful, 0, b'\xde\xde')
        self.assertEqual(decodeutf16stateful(-1, b'\x00\xde', 'replace'), (-1, '\ufffd', 2))
        self.assertEqual(decodeutf16stateful(1, b'\xde\x00', 'replace'), (1, '\ufffd', 2))
        self.assertEqual(decodeutf16stateful(0, b'\xde\xde', 'replace'), (0, '\ufffd', 2))
        self.assertEqual(decodeutf16stateful(0, b'\xff\xfe\x00\xde', 'replace'), (-1, '\ufffd', 4))
        self.assertEqual(decodeutf16stateful(0, b'\xfe\xff\xde\x00', 'replace'), (1, '\ufffd', 4))

        self.assertRaises(UnicodeDecodeError, decodeutf16stateful, -1, b'\x3d\xd8\x61\x00')
        self.assertEqual(decodeutf16stateful(-1, b'\x3d\xd8\x61\x00', 'replace'), (-1, '\ufffda', 4))
        self.assertRaises(UnicodeDecodeError, decodeutf16stateful, 1, b'\xd8\x3d\x00\x61')
        self.assertEqual(decodeutf16stateful(1, b'\xd8\x3d\x00\x61', 'replace'), (1, '\ufffda', 4))

        self.assertRaises(LookupError, decodeutf16stateful, -1, b'\x00\xde', 'foo')
        self.assertRaises(LookupError, decodeutf16stateful, 1, b'\xde\x00', 'foo')
        self.assertRaises(LookupError, decodeutf16stateful, 0, b'\xde\xde', 'foo')
        # TODO: Test PyUnicode_DecodeUTF16Stateful() with NULL as data and
        # negative size.
        # TODO: Test PyUnicode_DecodeUTF16Stateful() with NULL as the address of
        # "consumed".

    def test_asutf16string(self):
        """Test PyUnicode_AsUTF16String()"""
        asutf16string = _testlimitedcapi.unicode_asutf16string

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            self.assertEqual(asutf16string(s), s.encode('utf-16'))

        self.assertRaises(UnicodeEncodeError, asutf16string, '\ud8ff')
        self.assertRaises(TypeError, asutf16string, b'abc')
        self.assertRaises(TypeError, asutf16string, [])
        # CRASHES asutf16string(NULL)

    def test_decodeutf32(self):
        """Test PyUnicode_DecodeUTF8()"""
        decodeutf32 = _testlimitedcapi.unicode_decodeutf32

        naturalbyteorder = -1 if sys.byteorder == 'little' else 1
        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            b = s.encode('utf-32')
            self.assertEqual(decodeutf32(0, b), (naturalbyteorder, s))
            b = s.encode('utf-32le')
            self.assertEqual(decodeutf32(-1, b), (-1, s))
            self.assertEqual(decodeutf32(0, b'\xff\xfe\x00\x00'+b), (-1, s))
            b = s.encode('utf-32be')
            self.assertEqual(decodeutf32(1, b), (1, s))
            self.assertEqual(decodeutf32(0, b'\x00\x00\xfe\xff'+b), (1, s))

        self.assertRaises(UnicodeDecodeError, decodeutf32, -1, b'\x61\x00\x00\x00\x00')
        self.assertRaises(UnicodeDecodeError, decodeutf32, 1, b'\x00\x00\x00\x61\x00')
        self.assertRaises(UnicodeDecodeError, decodeutf32, 0, b'\xff\xfe\x00\x00\x61\x00\x00\x00\x00')
        self.assertRaises(UnicodeDecodeError, decodeutf32, 0, b'\x00\x00\xfe\xff\x00\x00\x00\x61\x00')

        self.assertRaises(UnicodeDecodeError, decodeutf32, -1, b'\xff\xff\xff\xff')
        self.assertRaises(UnicodeDecodeError, decodeutf32, 1, b'\xff\xff\xff\xff')
        self.assertRaises(UnicodeDecodeError, decodeutf32, 0, b'\xff\xff\xff\xff')
        self.assertEqual(decodeutf32(-1, b'\xff\xff\xff\xff', 'replace'), (-1, '\ufffd'))
        self.assertEqual(decodeutf32(1, b'\xff\xff\xff\xff', 'replace'), (1, '\ufffd'))
        self.assertEqual(decodeutf32(0, b'\xff\xff\xff\xff', 'replace'), (0, '\ufffd'))
        self.assertEqual(decodeutf32(0, b'\xff\xfe\x00\x00\xff\xff\xff\xff', 'replace'), (-1, '\ufffd'))
        self.assertEqual(decodeutf32(0, b'\x00\x00\xfe\xff\xff\xff\xff\xff', 'replace'), (1, '\ufffd'))

        self.assertRaises(UnicodeDecodeError, decodeutf32, -1, b'\x3d\xd8\x00\x00')
        self.assertEqual(decodeutf32(-1, b'\x3d\xd8\x00\x00', 'replace'), (-1, '\ufffd'))
        self.assertRaises(UnicodeDecodeError, decodeutf32, 1, b'\x00\x00\xd8\x3d')
        self.assertEqual(decodeutf32(1, b'\x00\x00\xd8\x3d', 'replace'), (1, '\ufffd'))

        self.assertRaises(LookupError, decodeutf32, -1, b'\xff\xff\xff\xff', 'foo')
        self.assertRaises(LookupError, decodeutf32, 1, b'\xff\xff\xff\xff', 'foo')
        self.assertRaises(LookupError, decodeutf32, 0, b'\xff\xff\xff\xff', 'foo')
        # TODO: Test PyUnicode_DecodeUTF32() with NULL as data and
        # negative size.

    def test_decodeutf32stateful(self):
        """Test PyUnicode_DecodeUTF32Stateful()"""
        decodeutf32stateful = _testlimitedcapi.unicode_decodeutf32stateful

        naturalbyteorder = -1 if sys.byteorder == 'little' else 1
        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            b = s.encode('utf-32')
            self.assertEqual(decodeutf32stateful(0, b), (naturalbyteorder, s, len(b)))
            b = s.encode('utf-32le')
            self.assertEqual(decodeutf32stateful(-1, b), (-1, s, len(b)))
            self.assertEqual(decodeutf32stateful(0, b'\xff\xfe\x00\x00'+b), (-1, s, len(b)+4))
            b = s.encode('utf-32be')
            self.assertEqual(decodeutf32stateful(1, b), (1, s, len(b)))
            self.assertEqual(decodeutf32stateful(0, b'\x00\x00\xfe\xff'+b), (1, s, len(b)+4))

        self.assertEqual(decodeutf32stateful(-1, b'\x61\x00\x00\x00\x00'), (-1, 'a', 4))
        self.assertEqual(decodeutf32stateful(-1, b'\x61\x00\x00\x00\x00\xf6'), (-1, 'a', 4))
        self.assertEqual(decodeutf32stateful(-1, b'\x61\x00\x00\x00\x00\xf6\x01'), (-1, 'a', 4))
        self.assertEqual(decodeutf32stateful(1, b'\x00\x00\x00\x61\x00'), (1, 'a', 4))
        self.assertEqual(decodeutf32stateful(1, b'\x00\x00\x00\x61\x00\x01'), (1, 'a', 4))
        self.assertEqual(decodeutf32stateful(1, b'\x00\x00\x00\x61\x00\x01\xf6'), (1, 'a', 4))
        self.assertEqual(decodeutf32stateful(0, b'\xff\xfe\x00\x00\x61\x00\x00\x00\x00\xf6\x01'), (-1, 'a', 8))
        self.assertEqual(decodeutf32stateful(0, b'\x00\x00\xfe\xff\x00\x00\x00\x61\x00\x01\xf6'), (1, 'a', 8))

        for b in b'\xff', b'\xff\xff', b'\xff\xff\xff':
            self.assertEqual(decodeutf32stateful(-1, b), (-1, '', 0))
            self.assertEqual(decodeutf32stateful(1, b), (1, '', 0))
            self.assertEqual(decodeutf32stateful(0, b), (0, '', 0))
            self.assertEqual(decodeutf32stateful(0, b'\xff\xfe\x00\x00'+b), (-1, '', 4))
            self.assertEqual(decodeutf32stateful(0, b'\x00\x00\xfe\xff'+b), (1, '', 4))
        self.assertRaises(UnicodeDecodeError, decodeutf32stateful, -1, b'\xff\xff\xff\xff')
        self.assertRaises(UnicodeDecodeError, decodeutf32stateful, 1, b'\xff\xff\xff\xff')
        self.assertRaises(UnicodeDecodeError, decodeutf32stateful, 0, b'\xff\xff\xff\xff')
        self.assertEqual(decodeutf32stateful(-1, b'\xff\xff\xff\xff', 'replace'), (-1, '\ufffd', 4))
        self.assertEqual(decodeutf32stateful(1, b'\xff\xff\xff\xff', 'replace'), (1, '\ufffd', 4))
        self.assertEqual(decodeutf32stateful(0, b'\xff\xff\xff\xff', 'replace'), (0, '\ufffd', 4))
        self.assertEqual(decodeutf32stateful(0, b'\xff\xfe\x00\x00\xff\xff\xff\xff', 'replace'), (-1, '\ufffd', 8))
        self.assertEqual(decodeutf32stateful(0, b'\x00\x00\xfe\xff\xff\xff\xff\xff', 'replace'), (1, '\ufffd', 8))

        self.assertRaises(UnicodeDecodeError, decodeutf32stateful, -1, b'\x3d\xd8\x00\x00')
        self.assertEqual(decodeutf32stateful(-1, b'\x3d\xd8\x00\x00', 'replace'), (-1, '\ufffd', 4))
        self.assertRaises(UnicodeDecodeError, decodeutf32stateful, 1, b'\x00\x00\xd8\x3d')
        self.assertEqual(decodeutf32stateful(1, b'\x00\x00\xd8\x3d', 'replace'), (1, '\ufffd', 4))

        self.assertRaises(LookupError, decodeutf32stateful, -1, b'\xff\xff\xff\xff', 'foo')
        self.assertRaises(LookupError, decodeutf32stateful, 1, b'\xff\xff\xff\xff', 'foo')
        self.assertRaises(LookupError, decodeutf32stateful, 0, b'\xff\xff\xff\xff', 'foo')
        # TODO: Test PyUnicode_DecodeUTF32Stateful() with NULL as data and
        # negative size.
        # TODO: Test PyUnicode_DecodeUTF32Stateful() with NULL as the address of
        # "consumed".

    def test_asutf32string(self):
        """Test PyUnicode_AsUTF32String()"""
        asutf32string = _testlimitedcapi.unicode_asutf32string

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            self.assertEqual(asutf32string(s), s.encode('utf-32'))

        self.assertRaises(UnicodeEncodeError, asutf32string, '\ud8ff')
        self.assertRaises(TypeError, asutf32string, b'abc')
        self.assertRaises(TypeError, asutf32string, [])
        # CRASHES asutf32string(NULL)

    def test_decodelatin1(self):
        """Test PyUnicode_DecodeLatin1()"""
        decodelatin1 = _testlimitedcapi.unicode_decodelatin1

        self.assertEqual(decodelatin1(b'abc'), 'abc')
        self.assertEqual(decodelatin1(b'abc', 'strict'), 'abc')
        self.assertEqual(decodelatin1(b'\xa1\xa2'), '\xa1\xa2')
        self.assertEqual(decodelatin1(b'\xa1\xa2', 'strict'), '\xa1\xa2')
        # TODO: Test PyUnicode_DecodeLatin1() with NULL as data and
        # negative size.

    def test_aslatin1string(self):
        """Test PyUnicode_AsLatin1String()"""
        aslatin1string = _testlimitedcapi.unicode_aslatin1string

        self.assertEqual(aslatin1string('abc'), b'abc')
        self.assertEqual(aslatin1string('\xa1\xa2'), b'\xa1\xa2')

        self.assertRaises(UnicodeEncodeError, aslatin1string, '\u4f60')
        self.assertRaises(TypeError, aslatin1string, b'abc')
        self.assertRaises(TypeError, aslatin1string, [])
        # CRASHES aslatin1string(NULL)

    def test_decodeascii(self):
        """Test PyUnicode_DecodeASCII()"""
        decodeascii = _testlimitedcapi.unicode_decodeascii

        self.assertEqual(decodeascii(b'abc'), 'abc')
        self.assertEqual(decodeascii(b'abc', 'strict'), 'abc')

        self.assertRaises(UnicodeDecodeError, decodeascii, b'\xff')
        self.assertEqual(decodeascii(b'a\xff', 'replace'), 'a\ufffd')
        self.assertEqual(decodeascii(b'a\xffb', 'replace'), 'a\ufffdb')

        self.assertRaises(LookupError, decodeascii, b'a\xff', 'foo')
        # TODO: Test PyUnicode_DecodeASCII() with NULL as data and
        # negative size.

    def test_asasciistring(self):
        """Test PyUnicode_AsASCIIString()"""
        asasciistring = _testlimitedcapi.unicode_asasciistring

        self.assertEqual(asasciistring('abc'), b'abc')

        self.assertRaises(UnicodeEncodeError, asasciistring, '\x80')
        self.assertRaises(TypeError, asasciistring, b'abc')
        self.assertRaises(TypeError, asasciistring, [])
        # CRASHES asasciistring(NULL)

    def test_decodecharmap(self):
        """Test PyUnicode_DecodeCharmap()"""
        decodecharmap = _testlimitedcapi.unicode_decodecharmap

        self.assertEqual(decodecharmap(b'\3\0\7', {0: 'a', 3: 'b', 7: 'c'}), 'bac')
        self.assertEqual(decodecharmap(b'\1\0\2', ['a', 'b', 'c']), 'bac')
        self.assertEqual(decodecharmap(b'\1\0\2', 'abc'), 'bac')
        self.assertEqual(decodecharmap(b'\1\0\2', ['\xa1', '\xa2', '\xa3']), '\xa2\xa1\xa3')
        self.assertEqual(decodecharmap(b'\1\0\2', ['\u4f60', '\u597d', '\u4e16']), '\u597d\u4f60\u4e16')
        self.assertEqual(decodecharmap(b'\1\0\2', ['\U0001f600', '\U0001f601', '\U0001f602']), '\U0001f601\U0001f600\U0001f602')

        self.assertEqual(decodecharmap(b'\1\0\2', [97, 98, 99]), 'bac')
        self.assertEqual(decodecharmap(b'\1\0\2', ['', 'b', 'cd']), 'bcd')

        self.assertRaises(UnicodeDecodeError, decodecharmap, b'\0', {})
        self.assertRaises(UnicodeDecodeError, decodecharmap, b'\0', {0: None})
        self.assertEqual(decodecharmap(b'\1\0\2', [None, 'b', 'c'], 'replace'), 'b\ufffdc')
        self.assertEqual(decodecharmap(b'\1\0\2\xff', NULL), '\1\0\2\xff')
        self.assertRaises(TypeError, decodecharmap, b'\0', 42)

        # TODO: Test PyUnicode_DecodeCharmap() with NULL as data and
        # negative size.

    def test_ascharmapstring(self):
        """Test PyUnicode_AsCharmapString()"""
        ascharmapstring = _testlimitedcapi.unicode_ascharmapstring

        self.assertEqual(ascharmapstring('abc', {97: 3, 98: 0, 99: 7}), b'\3\0\7')
        self.assertEqual(ascharmapstring('\xa1\xa2\xa3', {0xa1: 3, 0xa2: 0, 0xa3: 7}), b'\3\0\7')
        self.assertEqual(ascharmapstring('\u4f60\u597d\u4e16', {0x4f60: 3, 0x597d: 0, 0x4e16: 7}), b'\3\0\7')
        self.assertEqual(ascharmapstring('\U0001f600\U0001f601\U0001f602', {0x1f600: 3, 0x1f601: 0, 0x1f602: 7}), b'\3\0\7')
        self.assertEqual(ascharmapstring('abc', {97: 3, 98: b'', 99: b'spam'}), b'\3spam')

        self.assertRaises(UnicodeEncodeError, ascharmapstring, 'a', {})
        self.assertRaises(UnicodeEncodeError, ascharmapstring, 'a', {97: None})
        self.assertRaises(TypeError, ascharmapstring, b'a', {})
        self.assertRaises(TypeError, ascharmapstring, [], {})
        self.assertRaises(TypeError, ascharmapstring, 'a', NULL)
        # CRASHES ascharmapstring(NULL, {})

    def test_decodeunicodeescape(self):
        """Test PyUnicode_DecodeUnicodeEscape()"""
        decodeunicodeescape = _testlimitedcapi.unicode_decodeunicodeescape

        self.assertEqual(decodeunicodeescape(b'abc'), 'abc')
        self.assertEqual(decodeunicodeescape(br'\t\n\r\x0b\x0c\x00\\'), '\t\n\r\v\f\0\\')
        self.assertEqual(decodeunicodeescape(b'\t\n\r\x0b\x0c\x00'), '\t\n\r\v\f\0')
        self.assertEqual(decodeunicodeescape(br'\xa1\xa2'), '\xa1\xa2')
        self.assertEqual(decodeunicodeescape(b'\xa1\xa2'), '\xa1\xa2')
        self.assertEqual(decodeunicodeescape(br'\u4f60\u597d'), '\u4f60\u597d')
        self.assertEqual(decodeunicodeescape(br'\U0001f600'), '\U0001f600')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(decodeunicodeescape(br'\z'), r'\z')

        for b in b'\\', br'\xa', br'\u4f6', br'\U0001f60':
            self.assertRaises(UnicodeDecodeError, decodeunicodeescape, b)
            self.assertRaises(UnicodeDecodeError, decodeunicodeescape, b, 'strict')
        self.assertEqual(decodeunicodeescape(br'x\U0001f60', 'replace'), 'x\ufffd')
        self.assertEqual(decodeunicodeescape(br'x\U0001f60y', 'replace'), 'x\ufffdy')

        self.assertRaises(LookupError, decodeunicodeescape, b'\\', 'foo')
        # TODO: Test PyUnicode_DecodeUnicodeEscape() with NULL as data and
        # negative size.

    def test_asunicodeescapestring(self):
        """Test PyUnicode_AsUnicodeEscapeString()"""
        asunicodeescapestring = _testlimitedcapi.unicode_asunicodeescapestring

        self.assertEqual(asunicodeescapestring('abc'), b'abc')
        self.assertEqual(asunicodeescapestring('\t\n\r\v\f\0\\'), br'\t\n\r\x0b\x0c\x00\\')
        self.assertEqual(asunicodeescapestring('\xa1\xa2'), br'\xa1\xa2')
        self.assertEqual(asunicodeescapestring('\u4f60\u597d'), br'\u4f60\u597d')
        self.assertEqual(asunicodeescapestring('\U0001f600'), br'\U0001f600')

        self.assertRaises(TypeError, asunicodeescapestring, b'abc')
        self.assertRaises(TypeError, asunicodeescapestring, [])
        # CRASHES asunicodeescapestring(NULL)

    def test_decoderawunicodeescape(self):
        """Test PyUnicode_DecodeRawUnicodeEscape()"""
        decoderawunicodeescape = _testlimitedcapi.unicode_decoderawunicodeescape

        self.assertEqual(decoderawunicodeescape(b'abc'), 'abc')
        self.assertEqual(decoderawunicodeescape(b'\t\n\r\v\f\0\\'), '\t\n\r\v\f\0\\')
        self.assertEqual(decoderawunicodeescape(b'\xa1\xa2'), '\xa1\xa2')
        self.assertEqual(decoderawunicodeescape(br'\u4f60\u597d'), '\u4f60\u597d')
        self.assertEqual(decoderawunicodeescape(br'\U0001f600'), '\U0001f600')
        self.assertEqual(decoderawunicodeescape(br'\xa1\xa2'), r'\xa1\xa2')
        self.assertEqual(decoderawunicodeescape(br'\z'), r'\z')

        for b in br'\u4f6', br'\U0001f60':
            self.assertRaises(UnicodeDecodeError, decoderawunicodeescape, b)
            self.assertRaises(UnicodeDecodeError, decoderawunicodeescape, b, 'strict')
        self.assertEqual(decoderawunicodeescape(br'x\U0001f60', 'replace'), 'x\ufffd')
        self.assertEqual(decoderawunicodeescape(br'x\U0001f60y', 'replace'), 'x\ufffdy')

        self.assertRaises(LookupError, decoderawunicodeescape, br'\U0001f60', 'foo')
        # TODO: Test PyUnicode_DecodeRawUnicodeEscape() with NULL as data and
        # negative size.

    def test_asrawunicodeescapestring(self):
        """Test PyUnicode_AsRawUnicodeEscapeString()"""
        asrawunicodeescapestring = _testlimitedcapi.unicode_asrawunicodeescapestring

        self.assertEqual(asrawunicodeescapestring('abc'), b'abc')
        self.assertEqual(asrawunicodeescapestring('\t\n\r\v\f\0\\'), b'\t\n\r\v\f\0\\')
        self.assertEqual(asrawunicodeescapestring('\xa1\xa2'), b'\xa1\xa2')
        self.assertEqual(asrawunicodeescapestring('\u4f60\u597d'), br'\u4f60\u597d')
        self.assertEqual(asrawunicodeescapestring('\U0001f600'), br'\U0001f600')

        self.assertRaises(TypeError, asrawunicodeescapestring, b'abc')
        self.assertRaises(TypeError, asrawunicodeescapestring, [])
        # CRASHES asrawunicodeescapestring(NULL)


class CAPICodecs(unittest.TestCase):

    def setUp(self):
        # Encoding names are normalized internally by converting them
        # to lowercase and their hyphens are replaced by underscores.
        self.encoding_name = 'test.test_capi.test_codecs.codec_reversed'
        # Make sure that our custom codec is not already registered (that
        # way we know whether we correctly unregistered the custom codec
        # after a test or not).
        self.assertRaises(LookupError, codecs.lookup, self.encoding_name)
        # create the search function without registering yet
        self._create_custom_codec()

    def _create_custom_codec(self):
        def codec_encoder(m, errors='strict'):
            return (type(m)().join(reversed(m)), len(m))

        def codec_decoder(c, errors='strict'):
            return (type(c)().join(reversed(c)), len(c))

        class IncrementalEncoder(codecs.IncrementalEncoder):
            def encode(self, input, final=False):
                return codec_encoder(input)

        class IncrementalDecoder(codecs.IncrementalDecoder):
            def decode(self, input, final=False):
                return codec_decoder(input)

        class StreamReader(codecs.StreamReader):
            def encode(self, input, errors='strict'):
                return codec_encoder(input, errors=errors)

            def decode(self, input, errors='strict'):
                return codec_decoder(input, errors=errors)

        class StreamWriter(codecs.StreamWriter):
            def encode(self, input, errors='strict'):
                return codec_encoder(input, errors=errors)

            def decode(self, input, errors='strict'):
                return codec_decoder(input, errors=errors)

        info = codecs.CodecInfo(
            encode=codec_encoder,
            decode=codec_decoder,
            streamreader=StreamReader,
            streamwriter=StreamWriter,
            incrementalencoder=IncrementalEncoder,
            incrementaldecoder=IncrementalDecoder,
            name=self.encoding_name
        )

        def search_function(encoding):
            if encoding == self.encoding_name:
                return info
            return None

        self.codec_info = info
        self.search_function = search_function

    @contextlib.contextmanager
    def use_custom_encoder(self):
        self.assertRaises(LookupError, codecs.lookup, self.encoding_name)
        codecs.register(self.search_function)
        yield
        codecs.unregister(self.search_function)
        self.assertRaises(LookupError, codecs.lookup, self.encoding_name)

    def test_codec_register(self):
        search_function, encoding = self.search_function, self.encoding_name
        # register the search function using the C API
        self.assertIsNone(_testcapi.codec_register(search_function))
        # in case the test failed before cleaning up
        self.addCleanup(codecs.unregister, self.search_function)
        self.assertIs(codecs.lookup(encoding), search_function(encoding))
        self.assertEqual(codecs.encode('123', encoding=encoding), '321')
        # unregister the search function using the regular API
        codecs.unregister(search_function)
        self.assertRaises(LookupError, codecs.lookup, encoding)

    def test_codec_unregister(self):
        search_function, encoding = self.search_function, self.encoding_name
        self.assertRaises(LookupError, codecs.lookup, encoding)
        # register the search function using the regular API
        codecs.register(search_function)
        # in case the test failed before cleaning up
        self.addCleanup(codecs.unregister, self.search_function)
        self.assertIsNotNone(codecs.lookup(encoding))
        # unregister the search function using the C API
        self.assertIsNone(_testcapi.codec_unregister(search_function))
        self.assertRaises(LookupError, codecs.lookup, encoding)

    def test_codec_known_encoding(self):
        self.assertRaises(LookupError, codecs.lookup, 'unknown-codec')
        self.assertFalse(_testcapi.codec_known_encoding('unknown-codec'))
        self.assertFalse(_testcapi.codec_known_encoding('unknown_codec'))
        self.assertFalse(_testcapi.codec_known_encoding('UNKNOWN-codec'))

        encoding_name = self.encoding_name
        self.assertRaises(LookupError, codecs.lookup, encoding_name)

        codecs.register(self.search_function)
        self.addCleanup(codecs.unregister, self.search_function)

        for name in [
            encoding_name,
            encoding_name.upper(),
            encoding_name.replace('_', '-'),
        ]:
            with self.subTest(name):
                self.assertTrue(_testcapi.codec_known_encoding(name))

    def test_codec_encode(self):
        encode = _testcapi.codec_encode
        self.assertEqual(encode('a', 'utf-8', NULL), b'a')
        self.assertEqual(encode('a', 'utf-8', 'strict'), b'a')
        self.assertEqual(encode('[é]', 'ascii', 'ignore'), b'[]')

        self.assertRaises(TypeError, encode, NULL, 'ascii', 'strict')
        with self.assertRaisesRegex(TypeError, BAD_ARGUMENT):
            encode('a', NULL, 'strict')

    def test_codec_decode(self):
        decode = _testcapi.codec_decode

        s = 'a\xa1\u4f60\U0001f600'
        b = s.encode()

        self.assertEqual(decode(b, 'utf-8', 'strict'), s)
        self.assertEqual(decode(b, 'utf-8', NULL), s)
        self.assertEqual(decode(b, 'latin1', 'strict'), b.decode('latin1'))
        self.assertRaises(UnicodeDecodeError, decode, b, 'ascii', 'strict')
        self.assertRaises(UnicodeDecodeError, decode, b, 'ascii', NULL)
        self.assertEqual(decode(b, 'ascii', 'replace'), 'a' + '\ufffd'*9)

        # _codecs.decode() only reports an unknown error handling name when
        # the corresponding error handling function is used; this difers
        # from PyUnicode_Decode() which checks that both the encoding and
        # the error handling name are recognized before even attempting to
        # call the decoder.
        self.assertEqual(decode(b'', 'utf-8', 'unknown-error-handler'), '')
        self.assertEqual(decode(b'a', 'utf-8', 'unknown-error-handler'), 'a')

        self.assertRaises(TypeError, decode, NULL, 'ascii', 'strict')
        with self.assertRaisesRegex(TypeError, BAD_ARGUMENT):
            decode(b, NULL, 'strict')

    def test_codec_encoder(self):
        codec_encoder = _testcapi.codec_encoder

        with self.use_custom_encoder():
            encoder = codec_encoder(self.encoding_name)
            self.assertIs(encoder, self.codec_info.encode)

            with self.assertRaisesRegex(TypeError, BAD_ARGUMENT):
                codec_encoder(NULL)

    def test_codec_decoder(self):
        codec_decoder = _testcapi.codec_decoder

        with self.use_custom_encoder():
            decoder = codec_decoder(self.encoding_name)
            self.assertIs(decoder, self.codec_info.decode)

            with self.assertRaisesRegex(TypeError, BAD_ARGUMENT):
                codec_decoder(NULL)

    def test_codec_incremental_encoder(self):
        codec_incremental_encoder = _testcapi.codec_incremental_encoder

        with self.use_custom_encoder():
            encoding = self.encoding_name

            for errors in ['strict', NULL]:
                with self.subTest(errors):
                    encoder = codec_incremental_encoder(encoding, errors)
                    self.assertIsInstance(encoder, self.codec_info.incrementalencoder)

            with self.assertRaisesRegex(TypeError, BAD_ARGUMENT):
                codec_incremental_encoder(NULL, 'strict')

    def test_codec_incremental_decoder(self):
        codec_incremental_decoder = _testcapi.codec_incremental_decoder

        with self.use_custom_encoder():
            encoding = self.encoding_name

            for errors in ['strict', NULL]:
                with self.subTest(errors):
                    decoder = codec_incremental_decoder(encoding, errors)
                    self.assertIsInstance(decoder, self.codec_info.incrementaldecoder)

            with self.assertRaisesRegex(TypeError, BAD_ARGUMENT):
                codec_incremental_decoder(NULL, 'strict')

    def test_codec_stream_reader(self):
        codec_stream_reader = _testcapi.codec_stream_reader

        with self.use_custom_encoder():
            encoding, stream = self.encoding_name, io.StringIO()
            for errors in ['strict', NULL]:
                with self.subTest(errors):
                    writer = codec_stream_reader(encoding, stream, errors)
                    self.assertIsInstance(writer, self.codec_info.streamreader)

            with self.assertRaisesRegex(TypeError, BAD_ARGUMENT):
                codec_stream_reader(NULL, stream, 'strict')

    def test_codec_stream_writer(self):
        codec_stream_writer = _testcapi.codec_stream_writer

        with self.use_custom_encoder():
            encoding, stream = self.encoding_name, io.StringIO()
            for errors in ['strict', NULL]:
                with self.subTest(errors):
                    writer = codec_stream_writer(encoding, stream, errors)
                    self.assertIsInstance(writer, self.codec_info.streamwriter)

            with self.assertRaisesRegex(TypeError, BAD_ARGUMENT):
                codec_stream_writer(NULL, stream, 'strict')


class CAPICodecErrors(unittest.TestCase):

    @classmethod
    def _generate_exception_args(cls):
        for objlen in range(5):
            maxind = 2 * max(2, objlen)
            for start in range(-maxind, maxind + 1):
                for end in range(-maxind, maxind + 1):
                    yield objlen, start, end

    @classmethod
    def generate_encode_errors(cls):
        return tuple(
            UnicodeEncodeError('utf-8', '0' * objlen, start, end, 'why')
            for objlen, start, end in cls._generate_exception_args()
        )

    @classmethod
    def generate_decode_errors(cls):
        return tuple(
            UnicodeDecodeError('utf-8', b'0' * objlen, start, end, 'why')
            for objlen, start, end in cls._generate_exception_args()
        )

    @classmethod
    def generate_translate_errors(cls):
        return tuple(
            UnicodeTranslateError('0' * objlen, start, end, 'why')
            for objlen, start, end in cls._generate_exception_args()
        )

    @classmethod
    def setUpClass(cls):
        cls.unicode_encode_errors = cls.generate_encode_errors()
        cls.unicode_decode_errors = cls.generate_decode_errors()
        cls.unicode_translate_errors = cls.generate_translate_errors()
        cls.all_unicode_errors = (
            cls.unicode_encode_errors
            + cls.unicode_decode_errors
            + cls.unicode_translate_errors
        )
        cls.bad_unicode_errors = (
            ValueError(),
        )

    def test_codec_register_error(self):
        # for cleaning up between tests
        from _codecs import _unregister_error as _codecs_unregister_error

        self.assertRaises(LookupError, _testcapi.codec_lookup_error, 'custom')

        def custom_error_handler(exc):
            raise exc

        error_handler = mock.Mock(wraps=custom_error_handler)
        _testcapi.codec_register_error('custom', error_handler)
        self.addCleanup(_codecs_unregister_error, 'custom')

        self.assertRaises(UnicodeEncodeError, codecs.encode,
                          '\xff', 'ascii', errors='custom')
        error_handler.assert_called_once()
        error_handler.reset_mock()

        self.assertRaises(UnicodeDecodeError, codecs.decode,
                          b'\xff', 'ascii', errors='custom')
        error_handler.assert_called_once()

    # _codecs._unregister_error directly delegates to the internal C
    # function so a Python-level function test is sufficient (it is
    # tested in test_codeccallbacks).

    def test_codec_lookup_error(self):
        codec_lookup_error = _testcapi.codec_lookup_error
        self.assertIs(codec_lookup_error(NULL), codecs.strict_errors)
        self.assertIs(codec_lookup_error('strict'), codecs.strict_errors)
        self.assertIs(codec_lookup_error('ignore'), codecs.ignore_errors)
        self.assertIs(codec_lookup_error('replace'), codecs.replace_errors)
        self.assertIs(codec_lookup_error('xmlcharrefreplace'), codecs.xmlcharrefreplace_errors)
        self.assertIs(codec_lookup_error('backslashreplace'), codecs.backslashreplace_errors)
        self.assertIs(codec_lookup_error('namereplace'), codecs.namereplace_errors)
        self.assertRaises(LookupError, codec_lookup_error, 'unknown')

    def test_codec_strict_errors_handler(self):
        handler = _testcapi.codec_strict_errors
        for exc in self.all_unicode_errors + self.bad_unicode_errors:
            with self.subTest(handler=handler, exc=exc):
                self.assertRaises(type(exc), handler, exc)

    def test_codec_ignore_errors_handler(self):
        handler = _testcapi.codec_ignore_errors
        self.do_test_codec_errors_handler(handler, self.all_unicode_errors)

    def test_codec_replace_errors_handler(self):
        handler = _testcapi.codec_replace_errors
        self.do_test_codec_errors_handler(handler, self.all_unicode_errors)

    def test_codec_xmlcharrefreplace_errors_handler(self):
        handler = _testcapi.codec_xmlcharrefreplace_errors
        self.do_test_codec_errors_handler(handler, self.unicode_encode_errors)

    def test_codec_backslashreplace_errors_handler(self):
        handler = _testcapi.codec_backslashreplace_errors
        self.do_test_codec_errors_handler(handler, self.all_unicode_errors)

    def test_codec_namereplace_errors_handler(self):
        handler = _testlimitedcapi.codec_namereplace_errors
        self.do_test_codec_errors_handler(handler, self.unicode_encode_errors)

    def do_test_codec_errors_handler(self, handler, exceptions):
        self.assertNotEqual(len(exceptions), 0)
        for exc in exceptions:
            with self.subTest(handler=handler, exc=exc):
                # test that the handler does not crash
                res = handler(exc)
                self.assertIsInstance(res, tuple)
                self.assertEqual(len(res), 2)
                replacement, continue_from = res
                self.assertIsInstance(replacement, str)
                self.assertIsInstance(continue_from, int)
                self.assertGreaterEqual(continue_from, 0)
                self.assertLessEqual(continue_from, len(exc.object))

        for bad_exc in (
            self.bad_unicode_errors
            + tuple(e for e in self.all_unicode_errors if e not in exceptions)
        ):
            with self.subTest('bad type', handler=handler, exc=bad_exc):
                self.assertRaises(TypeError, handler, bad_exc)


if __name__ == "__main__":
    unittest.main()
