import unittest
from test.support import import_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX

NULL = None

class ByteArraySubclass(bytearray):
    pass

class BytesLike:
    def __init__(self, value):
        self.value = value
    def __bytes__(self):
        return self.value


class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyByteArray_Check()
        check = _testlimitedcapi.bytearray_check
        self.assertTrue(check(bytearray(b'')))
        self.assertTrue(check(bytearray(b'abc')))
        self.assertFalse(check(b'abc'))
        self.assertTrue(check(ByteArraySubclass(b'abc')))
        self.assertFalse(check(BytesLike(b'abc')))
        self.assertFalse(check(3))
        self.assertFalse(check([]))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_checkexact(self):
        # Test PyByteArray_CheckExact()
        check = _testlimitedcapi.bytearray_checkexact
        self.assertTrue(check(bytearray(b'')))
        self.assertTrue(check(bytearray(b'abc')))
        self.assertFalse(check(b'abc'))
        self.assertFalse(check(ByteArraySubclass(b'abc')))
        self.assertFalse(check(BytesLike(b'abc')))
        self.assertFalse(check(3))
        self.assertFalse(check([]))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_fromstringandsize(self):
        # Test PyByteArray_FromStringAndSize()
        fromstringandsize = _testlimitedcapi.bytearray_fromstringandsize

        self.assertEqual(fromstringandsize(b'abc'), bytearray(b'abc'))
        self.assertEqual(fromstringandsize(b'abc', 2), bytearray(b'ab'))
        self.assertEqual(fromstringandsize(b'abc\0def'), bytearray(b'abc\0def'))
        self.assertEqual(fromstringandsize(b'', 0), bytearray())
        self.assertEqual(fromstringandsize(NULL, 0), bytearray())
        self.assertEqual(len(fromstringandsize(NULL, 3)), 3)
        self.assertRaises(MemoryError, fromstringandsize, NULL, PY_SSIZE_T_MAX)

        self.assertRaises(SystemError, fromstringandsize, b'abc', -1)
        self.assertRaises(SystemError, fromstringandsize, b'abc', PY_SSIZE_T_MIN)
        self.assertRaises(SystemError, fromstringandsize, NULL, -1)
        self.assertRaises(SystemError, fromstringandsize, NULL, PY_SSIZE_T_MIN)

    def test_fromobject(self):
        # Test PyByteArray_FromObject()
        fromobject = _testlimitedcapi.bytearray_fromobject

        self.assertEqual(fromobject(b''), bytearray(b''))
        self.assertEqual(fromobject(b'abc'), bytearray(b'abc'))
        self.assertEqual(fromobject(bytearray(b'abc')), bytearray(b'abc'))
        self.assertEqual(fromobject(ByteArraySubclass(b'abc')), bytearray(b'abc'))
        self.assertEqual(fromobject([97, 98, 99]), bytearray(b'abc'))
        self.assertEqual(fromobject(3), bytearray(b'\0\0\0'))
        self.assertRaises(TypeError, fromobject, BytesLike(b'abc'))
        self.assertRaises(TypeError, fromobject, 'abc')
        self.assertRaises(TypeError, fromobject, object())

        # CRASHES fromobject(NULL)

    def test_size(self):
        # Test PyByteArray_Size()
        size = _testlimitedcapi.bytearray_size
        self.assertEqual(size(bytearray(b'')), 0)
        self.assertEqual(size(bytearray(b'abc')), 3)
        self.assertEqual(size(ByteArraySubclass(b'abc')), 3)

        # CRASHES size(b'abc')
        # CRASHES size(object())
        # CRASHES size(NULL)

    def test_asstring(self):
        """Test PyByteArray_AsString()"""
        asstring = _testlimitedcapi.bytearray_asstring
        self.assertEqual(asstring(bytearray(b''), 1), b'\0')
        self.assertEqual(asstring(bytearray(b'abc'), 4), b'abc\0')
        self.assertEqual(asstring(ByteArraySubclass(b'abc'), 4), b'abc\0')
        self.assertEqual(asstring(bytearray(b'abc\0def'), 8), b'abc\0def\0')

        # CRASHES asstring(b'abc', 0)
        # CRASHES asstring(object()', 0)
        # CRASHES asstring(NULL, 0)

    def test_concat(self):
        """Test PyByteArray_Concat()"""
        concat = _testlimitedcapi.bytearray_concat

        ba = bytearray(b'abc')
        self.assertEqual(concat(ba, b'def'), bytearray(b'abcdef'))
        self.assertEqual(ba, b'abc')
        self.assertEqual(concat(ba, ba), bytearray(b'abcabc'))

        self.assertEqual(concat(b'abc', b'def'), bytearray(b'abcdef'))
        self.assertEqual(concat(b'a\0b', b'c\0d'), bytearray(b'a\0bc\0d'))
        self.assertEqual(concat(bytearray(b'abc'), b'def'), bytearray(b'abcdef'))
        self.assertEqual(concat(b'abc', bytearray(b'def')), bytearray(b'abcdef'))
        self.assertEqual(concat(bytearray(b'abc'), b''), bytearray(b'abc'))
        self.assertEqual(concat(b'', bytearray(b'def')), bytearray(b'def'))
        self.assertEqual(concat(bytearray(b''), bytearray(b'')), bytearray(b''))
        self.assertEqual(concat(memoryview(b'xabcy')[1:4], b'def'),
                         bytearray(b'abcdef'))
        self.assertEqual(concat(b'abc', memoryview(b'xdefy')[1:4]),
                         bytearray(b'abcdef'))

        self.assertRaises(TypeError, concat, memoryview(b'axbycz')[::2], b'def')
        self.assertRaises(TypeError, concat, b'abc', memoryview(b'dxeyfz')[::2])
        self.assertRaises(TypeError, concat, b'abc', 'def')
        self.assertRaises(TypeError, concat, 'abc', b'def')
        self.assertRaises(TypeError, concat, 'abc', 'def')
        self.assertRaises(TypeError, concat, [], b'def')
        self.assertRaises(TypeError, concat, b'abc', [])
        self.assertRaises(TypeError, concat, [], [])

        # CRASHES concat(NULL, bytearray(b'def'))
        # CRASHES concat(bytearray(b'abc'), NULL)
        # CRASHES concat(NULL, object())
        # CRASHES concat(object(), NULL)

    def test_resize(self):
        """Test PyByteArray_Resize()"""
        resize = _testlimitedcapi.bytearray_resize

        ba = bytearray(b'abcdef')
        self.assertEqual(resize(ba, 3), 0)
        self.assertEqual(ba, bytearray(b'abc'))
        self.assertEqual(resize(ba, 10), 0)
        self.assertEqual(len(ba), 10)
        self.assertEqual(ba[:3], bytearray(b'abc'))
        self.assertEqual(resize(ba, 2**20), 0)
        self.assertEqual(len(ba), 2**20)
        self.assertEqual(ba[:3], bytearray(b'abc'))
        self.assertEqual(resize(ba, 0), 0)
        self.assertEqual(ba, bytearray())

        ba = bytearray(b'')
        self.assertEqual(resize(ba, 0), 0)
        self.assertEqual(ba, bytearray())

        ba = ByteArraySubclass(b'abcdef')
        self.assertEqual(resize(ba, 3), 0)
        self.assertEqual(ba, bytearray(b'abc'))

        self.assertRaises(ValueError, resize, bytearray(), -1)
        self.assertRaises(ValueError, resize, bytearray(), -200)
        self.assertRaises(MemoryError, resize, bytearray(), PY_SSIZE_T_MAX)
        self.assertRaises(MemoryError, resize, bytearray(1000), PY_SSIZE_T_MAX)

        # CRASHES resize(b'abc', 0)
        # CRASHES resize(object(), 0)
        # CRASHES resize(NULL, 0)


if __name__ == "__main__":
    unittest.main()
