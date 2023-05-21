import unittest
import sys
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')

NULL = None


class CAPITest(unittest.TestCase):
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
        fromencodedobject = _testcapi.unicode_fromencodedobject

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
        decode = _testcapi.unicode_decode

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
        asencodedstring = _testcapi.unicode_asencodedstring

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
        decodeutf8 = _testcapi.unicode_decodeutf8

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
        decodeutf8stateful = _testcapi.unicode_decodeutf8stateful

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
        asutf8string = _testcapi.unicode_asutf8string

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            self.assertEqual(asutf8string(s), s.encode('utf-8'))

        self.assertRaises(UnicodeEncodeError, asutf8string, '\ud8ff')
        self.assertRaises(TypeError, asutf8string, b'abc')
        self.assertRaises(TypeError, asutf8string, [])
        # CRASHES asutf8string(NULL)

    def test_decodeutf16(self):
        """Test PyUnicode_DecodeUTF16()"""
        decodeutf16 = _testcapi.unicode_decodeutf16

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
        decodeutf16stateful = _testcapi.unicode_decodeutf16stateful

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
        asutf16string = _testcapi.unicode_asutf16string

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            self.assertEqual(asutf16string(s), s.encode('utf-16'))

        self.assertRaises(UnicodeEncodeError, asutf16string, '\ud8ff')
        self.assertRaises(TypeError, asutf16string, b'abc')
        self.assertRaises(TypeError, asutf16string, [])
        # CRASHES asutf16string(NULL)

    def test_decodeutf32(self):
        """Test PyUnicode_DecodeUTF8()"""
        decodeutf32 = _testcapi.unicode_decodeutf32

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
        decodeutf32stateful = _testcapi.unicode_decodeutf32stateful

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
        asutf32string = _testcapi.unicode_asutf32string

        for s in ['abc', '\xa1\xa2', '\u4f60\u597d', 'a\U0001f600']:
            self.assertEqual(asutf32string(s), s.encode('utf-32'))

        self.assertRaises(UnicodeEncodeError, asutf32string, '\ud8ff')
        self.assertRaises(TypeError, asutf32string, b'abc')
        self.assertRaises(TypeError, asutf32string, [])
        # CRASHES asutf32string(NULL)

    def test_decodelatin1(self):
        """Test PyUnicode_DecodeLatin1()"""
        decodelatin1 = _testcapi.unicode_decodelatin1

        self.assertEqual(decodelatin1(b'abc'), 'abc')
        self.assertEqual(decodelatin1(b'abc', 'strict'), 'abc')
        self.assertEqual(decodelatin1(b'\xa1\xa2'), '\xa1\xa2')
        self.assertEqual(decodelatin1(b'\xa1\xa2', 'strict'), '\xa1\xa2')
        # TODO: Test PyUnicode_DecodeLatin1() with NULL as data and
        # negative size.

    def test_aslatin1string(self):
        """Test PyUnicode_AsLatin1String()"""
        aslatin1string = _testcapi.unicode_aslatin1string

        self.assertEqual(aslatin1string('abc'), b'abc')
        self.assertEqual(aslatin1string('\xa1\xa2'), b'\xa1\xa2')

        self.assertRaises(UnicodeEncodeError, aslatin1string, '\u4f60')
        self.assertRaises(TypeError, aslatin1string, b'abc')
        self.assertRaises(TypeError, aslatin1string, [])
        # CRASHES aslatin1string(NULL)

    def test_decodeascii(self):
        """Test PyUnicode_DecodeASCII()"""
        decodeascii = _testcapi.unicode_decodeascii

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
        asasciistring = _testcapi.unicode_asasciistring

        self.assertEqual(asasciistring('abc'), b'abc')

        self.assertRaises(UnicodeEncodeError, asasciistring, '\x80')
        self.assertRaises(TypeError, asasciistring, b'abc')
        self.assertRaises(TypeError, asasciistring, [])
        # CRASHES asasciistring(NULL)

    def test_decodecharmap(self):
        """Test PyUnicode_DecodeCharmap()"""
        decodecharmap = _testcapi.unicode_decodecharmap

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
        ascharmapstring = _testcapi.unicode_ascharmapstring

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
        decodeunicodeescape = _testcapi.unicode_decodeunicodeescape

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
        asunicodeescapestring = _testcapi.unicode_asunicodeescapestring

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
        decoderawunicodeescape = _testcapi.unicode_decoderawunicodeescape

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
        asrawunicodeescapestring = _testcapi.unicode_asrawunicodeescapestring

        self.assertEqual(asrawunicodeescapestring('abc'), b'abc')
        self.assertEqual(asrawunicodeescapestring('\t\n\r\v\f\0\\'), b'\t\n\r\v\f\0\\')
        self.assertEqual(asrawunicodeescapestring('\xa1\xa2'), b'\xa1\xa2')
        self.assertEqual(asrawunicodeescapestring('\u4f60\u597d'), br'\u4f60\u597d')
        self.assertEqual(asrawunicodeescapestring('\U0001f600'), br'\U0001f600')

        self.assertRaises(TypeError, asrawunicodeescapestring, b'abc')
        self.assertRaises(TypeError, asrawunicodeescapestring, [])
        # CRASHES asrawunicodeescapestring(NULL)


if __name__ == "__main__":
    unittest.main()
