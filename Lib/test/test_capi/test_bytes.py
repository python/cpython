import unittest
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')
from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX

NULL = None

class BytesSubclass(bytes):
    pass

class BytesLike:
    def __init__(self, value):
        self.value = value
    def __bytes__(self):
        return self.value


class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyBytes_Check()
        check = _testcapi.bytes_check
        self.assertTrue(check(b'abc'))
        self.assertFalse(check('abc'))
        self.assertFalse(check(bytearray(b'abc')))
        self.assertTrue(check(BytesSubclass(b'abc')))
        self.assertFalse(check(BytesLike(b'abc')))
        self.assertFalse(check(3))
        self.assertFalse(check([]))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_checkexact(self):
        # Test PyBytes_CheckExact()
        check = _testcapi.bytes_checkexact
        self.assertTrue(check(b'abc'))
        self.assertFalse(check('abc'))
        self.assertFalse(check(bytearray(b'abc')))
        self.assertFalse(check(BytesSubclass(b'abc')))
        self.assertFalse(check(BytesLike(b'abc')))
        self.assertFalse(check(3))
        self.assertFalse(check([]))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_fromstringandsize(self):
        # Test PyBytes_FromStringAndSize()
        fromstringandsize = _testcapi.bytes_fromstringandsize

        self.assertEqual(fromstringandsize(b'abc'), b'abc')
        self.assertEqual(fromstringandsize(b'abc', 2), b'ab')
        self.assertEqual(fromstringandsize(b'abc\0def'), b'abc\0def')
        self.assertEqual(fromstringandsize(b'a'), b'a')
        self.assertEqual(fromstringandsize(b'a', 1), b'a')
        self.assertEqual(fromstringandsize(b'', 0), b'')
        self.assertEqual(fromstringandsize(NULL, 0), b'')
        self.assertEqual(len(fromstringandsize(NULL, 3)), 3)
        self.assertRaises((MemoryError, OverflowError),
                          fromstringandsize, NULL, PY_SSIZE_T_MAX)

        self.assertRaises(SystemError, fromstringandsize, b'abc', -1)
        self.assertRaises(SystemError, fromstringandsize, b'abc', PY_SSIZE_T_MIN)
        self.assertRaises(SystemError, fromstringandsize, NULL, -1)
        self.assertRaises(SystemError, fromstringandsize, NULL, PY_SSIZE_T_MIN)

    def test_fromstring(self):
        # Test PyBytes_FromString()
        fromstring = _testcapi.bytes_fromstring

        self.assertEqual(fromstring(b'abc\0def'), b'abc')
        self.assertEqual(fromstring(b''), b'')

        # CRASHES fromstring(NULL)

    def test_fromobject(self):
        # Test PyBytes_FromObject()
        fromobject = _testcapi.bytes_fromobject

        self.assertEqual(fromobject(b'abc'), b'abc')
        self.assertEqual(fromobject(bytearray(b'abc')), b'abc')
        self.assertEqual(fromobject(BytesSubclass(b'abc')), b'abc')
        self.assertEqual(fromobject([97, 98, 99]), b'abc')
        self.assertRaises(TypeError, fromobject, 3)
        self.assertRaises(TypeError, fromobject, BytesLike(b'abc'))
        self.assertRaises(TypeError, fromobject, 'abc')
        self.assertRaises(TypeError, fromobject, object())
        self.assertRaises(SystemError, fromobject, NULL)

    def test_size(self):
        # Test PyBytes_Size()
        size = _testcapi.bytes_size

        self.assertEqual(size(b'abc'), 3)
        self.assertEqual(size(BytesSubclass(b'abc')), 3)
        self.assertRaises(TypeError, size, bytearray(b'abc'))
        self.assertRaises(TypeError, size, 'abc')
        self.assertRaises(TypeError, size, object())

        # CRASHES size(NULL)

    def test_asstring(self):
        """Test PyBytes_AsString()"""
        asstring = _testcapi.bytes_asstring

        self.assertEqual(asstring(b'abc', 4), b'abc\0')
        self.assertEqual(asstring(b'abc\0def', 8), b'abc\0def\0')
        self.assertRaises(TypeError, asstring, 'abc', 0)
        self.assertRaises(TypeError, asstring, object(), 0)

        # CRASHES asstring(NULL, 0)

    def test_asstringandsize(self):
        """Test PyBytes_AsStringAndSize()"""
        asstringandsize = _testcapi.bytes_asstringandsize
        asstringandsize_null = _testcapi.bytes_asstringandsize_null

        self.assertEqual(asstringandsize(b'abc', 4), (b'abc\0', 3))
        self.assertEqual(asstringandsize(b'abc\0def', 8), (b'abc\0def\0', 7))
        self.assertEqual(asstringandsize_null(b'abc', 4), b'abc\0')
        self.assertRaises(ValueError, asstringandsize_null, b'abc\0def', 8)
        self.assertRaises(TypeError, asstringandsize, 'abc', 0)
        self.assertRaises(TypeError, asstringandsize_null, 'abc', 0)
        self.assertRaises(TypeError, asstringandsize, object(), 0)
        self.assertRaises(TypeError, asstringandsize_null, object(), 0)

        # CRASHES asstringandsize(NULL, 0)
        # CRASHES asstringandsize_null(NULL, 0)

    def test_repr(self):
        # Test PyBytes_Repr()
        bytes_repr = _testcapi.bytes_repr

        self.assertEqual(bytes_repr(b'''abc''', 0), r"""b'abc'""")
        self.assertEqual(bytes_repr(b'''abc''', 1), r"""b'abc'""")
        self.assertEqual(bytes_repr(b'''a'b"c"d''', 0), r"""b'a\'b"c"d'""")
        self.assertEqual(bytes_repr(b'''a'b"c"d''', 1), r"""b'a\'b"c"d'""")
        self.assertEqual(bytes_repr(b'''a'b"c''', 0), r"""b'a\'b"c'""")
        self.assertEqual(bytes_repr(b'''a'b"c''', 1), r"""b'a\'b"c'""")
        self.assertEqual(bytes_repr(b'''a'b'c"d''', 0), r"""b'a\'b\'c"d'""")
        self.assertEqual(bytes_repr(b'''a'b'c"d''', 1), r"""b'a\'b\'c"d'""")
        self.assertEqual(bytes_repr(b'''a'b'c'd''', 0), r"""b'a\'b\'c\'d'""")
        self.assertEqual(bytes_repr(b'''a'b'c'd''', 1), r'''b"a'b'c'd"''')

        self.assertEqual(bytes_repr(BytesSubclass(b'abc'), 0), r"""b'abc'""")

        # UDEFINED bytes_repr(object(), 0)
        # CRASHES bytes_repr(NULL, 0)

    def test_concat(self, concat=None):
        """Test PyBytes_Concat()"""
        if concat is None:
            concat = _testcapi.bytes_concat

        self.assertEqual(concat(b'abc', b'def'), b'abcdef')
        self.assertEqual(concat(b'a\0b', b'c\0d'), b'a\0bc\0d')
        self.assertEqual(concat(bytearray(b'abc'), b'def'), b'abcdef')
        self.assertEqual(concat(b'abc', bytearray(b'def')), b'abcdef')
        self.assertEqual(concat(bytearray(b'abc'), b''), b'abc')
        self.assertEqual(concat(b'', bytearray(b'def')), b'def')
        self.assertEqual(concat(memoryview(b'xabcy')[1:4], b'def'), b'abcdef')
        self.assertEqual(concat(b'abc', memoryview(b'xdefy')[1:4]), b'abcdef')

        self.assertEqual(concat(b'abc', b'def', True), b'abcdef')
        self.assertEqual(concat(b'abc', bytearray(b'def'), True), b'abcdef')
        # Check that it does not change the singleton
        self.assertEqual(concat(bytes(), b'def', True), b'def')
        self.assertEqual(len(bytes()), 0)

        self.assertRaises(TypeError, concat, memoryview(b'axbycz')[::2], b'def')
        self.assertRaises(TypeError, concat, b'abc', memoryview(b'dxeyfz')[::2])
        self.assertRaises(TypeError, concat, b'abc', 'def')
        self.assertRaises(TypeError, concat, 'abc', b'def')
        self.assertRaises(TypeError, concat, 'abc', 'def')
        self.assertRaises(TypeError, concat, [], b'def')
        self.assertRaises(TypeError, concat, b'abc', [])
        self.assertRaises(TypeError, concat, [], [])

        self.assertEqual(concat(NULL, b'def'), NULL)
        self.assertEqual(concat(b'abc', NULL), NULL)
        self.assertEqual(concat(NULL, object()), NULL)
        self.assertEqual(concat(object(), NULL), NULL)

    def test_concatanddel(self):
        """Test PyBytes_ConcatAndDel()"""
        self.test_concat(_testcapi.bytes_concatanddel)

    def test_decodeescape(self):
        """Test PyBytes_DecodeEscape()"""
        decodeescape = _testcapi.bytes_decodeescape

        self.assertEqual(decodeescape(b'abc'), b'abc')
        self.assertEqual(decodeescape(br'\t\n\r\x0b\x0c\x00\\\'\"'),
                         b'''\t\n\r\v\f\0\\'"''')
        self.assertEqual(decodeescape(b'\t\n\r\x0b\x0c\x00'), b'\t\n\r\v\f\0')
        self.assertEqual(decodeescape(br'\xa1\xa2'), b'\xa1\xa2')
        self.assertEqual(decodeescape(br'\2\24\241'), b'\x02\x14\xa1')
        self.assertEqual(decodeescape(b'\xa1\xa2'), b'\xa1\xa2')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(decodeescape(br'\u4f60'), br'\u4f60')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(decodeescape(br'\z'), br'\z')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(decodeescape(br'\541'), b'a')

        for b in b'\\', br'\x', br'\xa', br'\xz', br'\xaz':
            self.assertRaises(ValueError, decodeescape, b)
            self.assertRaises(ValueError, decodeescape, b, 'strict')
        self.assertEqual(decodeescape(br'x\xa', 'replace'), b'x?')
        self.assertEqual(decodeescape(br'x\xay', 'replace'), b'x?y')
        self.assertEqual(decodeescape(br'x\xa\xy', 'replace'), b'x??y')
        self.assertEqual(decodeescape(br'x\xa\xy', 'ignore'), b'xy')
        self.assertRaises(ValueError, decodeescape, b'\\', 'spam')
        self.assertEqual(decodeescape(NULL), b'')
        self.assertRaises(OverflowError, decodeescape, b'abc', NULL, PY_SSIZE_T_MAX)
        self.assertRaises(OverflowError, decodeescape, NULL, NULL, PY_SSIZE_T_MAX)

        # CRASHES decodeescape(b'abc', NULL, -1)
        # CRASHES decodeescape(NULL, NULL, 1)


if __name__ == "__main__":
    unittest.main()
