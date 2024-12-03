import unittest
from collections import OrderedDict
from test import support
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX

NULL = None

class StrSubclass(str):
    pass

class BytesSubclass(bytes):
    pass

class WithStr:
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return self.value

class WithRepr:
    def __init__(self, value):
        self.value = value
    def __repr__(self):
        return self.value

class WithBytes:
    def __init__(self, value):
        self.value = value
    def __bytes__(self):
        return self.value

class TestObject:
    @property
    def evil(self):
        raise RuntimeError('do not get evil')
    @evil.setter
    def evil(self, value):
        raise RuntimeError('do not set evil')
    @evil.deleter
    def evil(self):
        raise RuntimeError('do not del evil')

class ProxyGetItem:
    def __init__(self, obj):
        self.obj = obj
    def __getitem__(self, key):
        return self.obj[key]

class ProxySetItem:
    def __init__(self, obj):
        self.obj = obj
    def __setitem__(self, key, value):
        self.obj[key] = value

class ProxyDelItem:
    def __init__(self, obj):
        self.obj = obj
    def __delitem__(self, key):
        del self.obj[key]

def gen():
    yield 'a'
    yield 'b'
    yield 'c'


class CAPITest(unittest.TestCase):
    def assertTypedEqual(self, actual, expected):
        self.assertIs(type(actual), type(expected))
        self.assertEqual(actual, expected)

    def test_object_str(self):
        # Test PyObject_Str()
        object_str = _testlimitedcapi.object_str
        self.assertTypedEqual(object_str(''), '')
        self.assertTypedEqual(object_str('abc'), 'abc')
        self.assertTypedEqual(object_str('\U0001f40d'), '\U0001f40d')
        self.assertTypedEqual(object_str(StrSubclass('abc')), 'abc')
        self.assertTypedEqual(object_str(WithStr('abc')), 'abc')
        self.assertTypedEqual(object_str(WithStr(StrSubclass('abc'))), StrSubclass('abc'))
        self.assertTypedEqual(object_str(WithRepr('<abc>')), '<abc>')
        self.assertTypedEqual(object_str(WithRepr(StrSubclass('<abc>'))), StrSubclass('<abc>'))
        self.assertTypedEqual(object_str(NULL), '<NULL>')

    def test_object_repr(self):
        # Test PyObject_Repr()
        object_repr = _testlimitedcapi.object_repr
        self.assertTypedEqual(object_repr(''), "''")
        self.assertTypedEqual(object_repr('abc'), "'abc'")
        self.assertTypedEqual(object_repr('\U0001f40d'), "'\U0001f40d'")
        self.assertTypedEqual(object_repr(StrSubclass('abc')), "'abc'")
        self.assertTypedEqual(object_repr(WithRepr('<abc>')), '<abc>')
        self.assertTypedEqual(object_repr(WithRepr(StrSubclass('<abc>'))), StrSubclass('<abc>'))
        self.assertTypedEqual(object_repr(WithRepr('<\U0001f40d>')), '<\U0001f40d>')
        self.assertTypedEqual(object_repr(WithRepr(StrSubclass('<\U0001f40d>'))), StrSubclass('<\U0001f40d>'))
        self.assertTypedEqual(object_repr(NULL), '<NULL>')

    def test_object_ascii(self):
        # Test PyObject_ASCII()
        object_ascii = _testlimitedcapi.object_ascii
        self.assertTypedEqual(object_ascii(''), "''")
        self.assertTypedEqual(object_ascii('abc'), "'abc'")
        self.assertTypedEqual(object_ascii('\U0001f40d'), r"'\U0001f40d'")
        self.assertTypedEqual(object_ascii(StrSubclass('abc')), "'abc'")
        self.assertTypedEqual(object_ascii(WithRepr('<abc>')), '<abc>')
        self.assertTypedEqual(object_ascii(WithRepr(StrSubclass('<abc>'))), StrSubclass('<abc>'))
        self.assertTypedEqual(object_ascii(WithRepr('<\U0001f40d>')), r'<\U0001f40d>')
        self.assertTypedEqual(object_ascii(WithRepr(StrSubclass('<\U0001f40d>'))), r'<\U0001f40d>')
        self.assertTypedEqual(object_ascii(NULL), '<NULL>')

    def test_object_bytes(self):
        # Test PyObject_Bytes()
        object_bytes = _testlimitedcapi.object_bytes
        self.assertTypedEqual(object_bytes(b''), b'')
        self.assertTypedEqual(object_bytes(b'abc'), b'abc')
        self.assertTypedEqual(object_bytes(BytesSubclass(b'abc')), b'abc')
        self.assertTypedEqual(object_bytes(WithBytes(b'abc')), b'abc')
        self.assertTypedEqual(object_bytes(WithBytes(BytesSubclass(b'abc'))), BytesSubclass(b'abc'))
        self.assertTypedEqual(object_bytes(bytearray(b'abc')), b'abc')
        self.assertTypedEqual(object_bytes(memoryview(b'abc')), b'abc')
        self.assertTypedEqual(object_bytes([97, 98, 99]), b'abc')
        self.assertTypedEqual(object_bytes((97, 98, 99)), b'abc')
        self.assertTypedEqual(object_bytes(iter([97, 98, 99])), b'abc')
        self.assertRaises(TypeError, object_bytes, WithBytes(bytearray(b'abc')))
        self.assertRaises(TypeError, object_bytes, WithBytes([97, 98, 99]))
        self.assertRaises(TypeError, object_bytes, 3)
        self.assertRaises(TypeError, object_bytes, 'abc')
        self.assertRaises(TypeError, object_bytes, object())
        self.assertTypedEqual(object_bytes(NULL), b'<NULL>')

    def test_object_getattr(self):
        xgetattr = _testlimitedcapi.object_getattr
        obj = TestObject()
        obj.a = 11
        setattr(obj, '\U0001f40d', 22)
        self.assertEqual(xgetattr(obj, 'a'), 11)
        self.assertRaises(AttributeError, xgetattr, obj, 'b')
        self.assertEqual(xgetattr(obj, '\U0001f40d'), 22)

        self.assertRaises(RuntimeError, xgetattr, obj, 'evil')
        self.assertRaises(TypeError, xgetattr, obj, 1)
        # CRASHES xgetattr(obj, NULL)
        # CRASHES xgetattr(NULL, 'a')

    def test_object_getattrstring(self):
        getattrstring = _testlimitedcapi.object_getattrstring
        obj = TestObject()
        obj.a = 11
        setattr(obj, '\U0001f40d', 22)
        self.assertEqual(getattrstring(obj, b'a'), 11)
        self.assertRaises(AttributeError, getattrstring, obj, b'b')
        self.assertEqual(getattrstring(obj, '\U0001f40d'.encode()), 22)

        self.assertRaises(RuntimeError, getattrstring, obj, b'evil')
        self.assertRaises(UnicodeDecodeError, getattrstring, obj, b'\xff')
        # CRASHES getattrstring(obj, NULL)
        # CRASHES getattrstring(NULL, b'a')

    def test_object_getoptionalattr(self):
        getoptionalattr = _testcapi.object_getoptionalattr
        obj = TestObject()
        obj.a = 11
        setattr(obj, '\U0001f40d', 22)
        self.assertEqual(getoptionalattr(obj, 'a'), 11)
        self.assertIs(getoptionalattr(obj, 'b'), AttributeError)
        self.assertEqual(getoptionalattr(obj, '\U0001f40d'), 22)

        self.assertRaises(RuntimeError, getoptionalattr, obj, 'evil')
        self.assertRaises(TypeError, getoptionalattr, obj, 1)
        # CRASHES getoptionalattr(obj, NULL)
        # CRASHES getoptionalattr(NULL, 'a')

    def test_object_getoptionalattrstring(self):
        getoptionalattrstring = _testcapi.object_getoptionalattrstring
        obj = TestObject()
        obj.a = 11
        setattr(obj, '\U0001f40d', 22)
        self.assertEqual(getoptionalattrstring(obj, b'a'), 11)
        self.assertIs(getoptionalattrstring(obj, b'b'), AttributeError)
        self.assertEqual(getoptionalattrstring(obj, '\U0001f40d'.encode()), 22)

        self.assertRaises(RuntimeError, getoptionalattrstring, obj, b'evil')
        self.assertRaises(UnicodeDecodeError, getoptionalattrstring, obj, b'\xff')
        # CRASHES getoptionalattrstring(obj, NULL)
        # CRASHES getoptionalattrstring(NULL, b'a')

    def test_object_hasattr(self):
        xhasattr = _testlimitedcapi.object_hasattr
        obj = TestObject()
        obj.a = 1
        setattr(obj, '\U0001f40d', 2)
        self.assertTrue(xhasattr(obj, 'a'))
        self.assertFalse(xhasattr(obj, 'b'))
        self.assertTrue(xhasattr(obj, '\U0001f40d'))

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(xhasattr(obj, 'evil'))
            self.assertEqual(cm.unraisable.exc_type, RuntimeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             'do not get evil')

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(xhasattr(obj, 1))
            self.assertEqual(cm.unraisable.exc_type, TypeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             "attribute name must be string, not 'int'")

        # CRASHES xhasattr(obj, NULL)
        # CRASHES xhasattr(NULL, 'a')

    def test_object_hasattrstring(self):
        hasattrstring = _testlimitedcapi.object_hasattrstring
        obj = TestObject()
        obj.a = 1
        setattr(obj, '\U0001f40d', 2)
        self.assertTrue(hasattrstring(obj, b'a'))
        self.assertFalse(hasattrstring(obj, b'b'))
        self.assertTrue(hasattrstring(obj, '\U0001f40d'.encode()))

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(hasattrstring(obj, b'evil'))
            self.assertEqual(cm.unraisable.exc_type, RuntimeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             'do not get evil')

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(hasattrstring(obj, b'\xff'))
            self.assertEqual(cm.unraisable.exc_type, UnicodeDecodeError)
            self.assertRegex(str(cm.unraisable.exc_value),
                             "'utf-8' codec can't decode")

        # CRASHES hasattrstring(obj, NULL)
        # CRASHES hasattrstring(NULL, b'a')

    def test_object_hasattrwitherror(self):
        xhasattr = _testcapi.object_hasattrwitherror
        obj = TestObject()
        obj.a = 1
        setattr(obj, '\U0001f40d', 2)
        self.assertTrue(xhasattr(obj, 'a'))
        self.assertFalse(xhasattr(obj, 'b'))
        self.assertTrue(xhasattr(obj, '\U0001f40d'))

        self.assertRaises(RuntimeError, xhasattr, obj, 'evil')
        self.assertRaises(TypeError, xhasattr, obj, 1)
        # CRASHES xhasattr(obj, NULL)
        # CRASHES xhasattr(NULL, 'a')

    def test_object_hasattrstringwitherror(self):
        hasattrstring = _testcapi.object_hasattrstringwitherror
        obj = TestObject()
        obj.a = 1
        setattr(obj, '\U0001f40d', 2)
        self.assertTrue(hasattrstring(obj, b'a'))
        self.assertFalse(hasattrstring(obj, b'b'))
        self.assertTrue(hasattrstring(obj, '\U0001f40d'.encode()))

        self.assertRaises(RuntimeError, hasattrstring, obj, b'evil')
        self.assertRaises(UnicodeDecodeError, hasattrstring, obj, b'\xff')
        # CRASHES hasattrstring(obj, NULL)
        # CRASHES hasattrstring(NULL, b'a')

    def test_object_setattr(self):
        xsetattr = _testlimitedcapi.object_setattr
        obj = TestObject()
        xsetattr(obj, 'a', 5)
        self.assertEqual(obj.a, 5)
        xsetattr(obj, '\U0001f40d', 8)
        self.assertEqual(getattr(obj, '\U0001f40d'), 8)

        # PyObject_SetAttr(obj, attr_name, NULL) removes the attribute
        xsetattr(obj, 'a', NULL)
        self.assertFalse(hasattr(obj, 'a'))
        self.assertRaises(AttributeError, xsetattr, obj, 'b', NULL)
        self.assertRaises(RuntimeError, xsetattr, obj, 'evil', NULL)

        self.assertRaises(RuntimeError, xsetattr, obj, 'evil', 'good')
        self.assertRaises(AttributeError, xsetattr, 42, 'a', 5)
        self.assertRaises(TypeError, xsetattr, obj, 1, 5)
        # CRASHES xsetattr(obj, NULL, 5)
        # CRASHES xsetattr(NULL, 'a', 5)

    def test_object_setattrstring(self):
        setattrstring = _testlimitedcapi.object_setattrstring
        obj = TestObject()
        setattrstring(obj, b'a', 5)
        self.assertEqual(obj.a, 5)
        setattrstring(obj, '\U0001f40d'.encode(), 8)
        self.assertEqual(getattr(obj, '\U0001f40d'), 8)

        # PyObject_SetAttrString(obj, attr_name, NULL) removes the attribute
        setattrstring(obj, b'a', NULL)
        self.assertFalse(hasattr(obj, 'a'))
        self.assertRaises(AttributeError, setattrstring, obj, b'b', NULL)
        self.assertRaises(RuntimeError, setattrstring, obj, b'evil', NULL)

        self.assertRaises(RuntimeError, setattrstring, obj, b'evil', 'good')
        self.assertRaises(AttributeError, setattrstring, 42, b'a', 5)
        self.assertRaises(TypeError, setattrstring, obj, 1, 5)
        self.assertRaises(UnicodeDecodeError, setattrstring, obj, b'\xff', 5)
        # CRASHES setattrstring(obj, NULL, 5)
        # CRASHES setattrstring(NULL, b'a', 5)

    def test_object_delattr(self):
        xdelattr = _testlimitedcapi.object_delattr
        obj = TestObject()
        obj.a = 1
        setattr(obj, '\U0001f40d', 2)
        xdelattr(obj, 'a')
        self.assertFalse(hasattr(obj, 'a'))
        self.assertRaises(AttributeError, xdelattr, obj, 'b')
        xdelattr(obj, '\U0001f40d')
        self.assertFalse(hasattr(obj, '\U0001f40d'))

        self.assertRaises(AttributeError, xdelattr, 42, 'numerator')
        self.assertRaises(RuntimeError, xdelattr, obj, 'evil')
        self.assertRaises(TypeError, xdelattr, obj, 1)
        # CRASHES xdelattr(obj, NULL)
        # CRASHES xdelattr(NULL, 'a')

    def test_object_delattrstring(self):
        delattrstring = _testlimitedcapi.object_delattrstring
        obj = TestObject()
        obj.a = 1
        setattr(obj, '\U0001f40d', 2)
        delattrstring(obj, b'a')
        self.assertFalse(hasattr(obj, 'a'))
        self.assertRaises(AttributeError, delattrstring, obj, b'b')
        delattrstring(obj, '\U0001f40d'.encode())
        self.assertFalse(hasattr(obj, '\U0001f40d'))

        self.assertRaises(AttributeError, delattrstring, 42, b'numerator')
        self.assertRaises(RuntimeError, delattrstring, obj, b'evil')
        self.assertRaises(UnicodeDecodeError, delattrstring, obj, b'\xff')
        # CRASHES delattrstring(obj, NULL)
        # CRASHES delattrstring(NULL, b'a')


    def test_mapping_check(self):
        check = _testlimitedcapi.mapping_check
        self.assertTrue(check({1: 2}))
        self.assertTrue(check([1, 2]))
        self.assertTrue(check((1, 2)))
        self.assertTrue(check('abc'))
        self.assertTrue(check(b'abc'))
        self.assertFalse(check(42))
        self.assertFalse(check(object()))
        self.assertFalse(check(NULL))

    def test_mapping_size(self):
        for size in _testlimitedcapi.mapping_size, _testlimitedcapi.mapping_length:
            self.assertEqual(size({1: 2}), 1)
            self.assertEqual(size([1, 2]), 2)
            self.assertEqual(size((1, 2)), 2)
            self.assertEqual(size('abc'), 3)
            self.assertEqual(size(b'abc'), 3)

            self.assertRaises(TypeError, size, 42)
            self.assertRaises(TypeError, size, object())
            self.assertRaises(SystemError, size, NULL)

    def test_object_getitem(self):
        getitem = _testlimitedcapi.object_getitem
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitem(dct, 'a'), 1)
        self.assertRaises(KeyError, getitem, dct, 'b')
        self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        dct2 = ProxyGetItem(dct)
        self.assertEqual(getitem(dct2, 'a'), 1)
        self.assertRaises(KeyError, getitem, dct2, 'b')

        self.assertEqual(getitem(['a', 'b', 'c'], 1), 'b')

        self.assertRaises(TypeError, getitem, 42, 'a')
        self.assertRaises(TypeError, getitem, {}, [])  # unhashable
        self.assertRaises(SystemError, getitem, {}, NULL)
        self.assertRaises(IndexError, getitem, [], 1)
        self.assertRaises(TypeError, getitem, [], 'a')
        self.assertRaises(SystemError, getitem, NULL, 'a')

    def test_mapping_getitemstring(self):
        getitemstring = _testlimitedcapi.mapping_getitemstring
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitemstring(dct, b'a'), 1)
        self.assertRaises(KeyError, getitemstring, dct, b'b')
        self.assertEqual(getitemstring(dct, '\U0001f40d'.encode()), 2)

        dct2 = ProxyGetItem(dct)
        self.assertEqual(getitemstring(dct2, b'a'), 1)
        self.assertRaises(KeyError, getitemstring, dct2, b'b')

        self.assertRaises(TypeError, getitemstring, 42, b'a')
        self.assertRaises(UnicodeDecodeError, getitemstring, {}, b'\xff')
        self.assertRaises(SystemError, getitemstring, {}, NULL)
        self.assertRaises(TypeError, getitemstring, [], b'a')
        self.assertRaises(SystemError, getitemstring, NULL, b'a')

    def test_mapping_getoptionalitem(self):
        getitem = _testcapi.mapping_getoptionalitem
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitem(dct, 'a'), 1)
        self.assertEqual(getitem(dct, 'b'), KeyError)
        self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        dct2 = ProxyGetItem(dct)
        self.assertEqual(getitem(dct2, 'a'), 1)
        self.assertEqual(getitem(dct2, 'b'), KeyError)

        self.assertEqual(getitem(['a', 'b', 'c'], 1), 'b')

        self.assertRaises(TypeError, getitem, 42, 'a')
        self.assertRaises(TypeError, getitem, {}, [])  # unhashable
        self.assertRaises(IndexError, getitem, [], 1)
        self.assertRaises(TypeError, getitem, [], 'a')
        # CRASHES getitem({}, NULL)
        # CRASHES getitem(NULL, 'a')

    def test_mapping_getoptionalitemstring(self):
        getitemstring = _testcapi.mapping_getoptionalitemstring
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitemstring(dct, b'a'), 1)
        self.assertEqual(getitemstring(dct, b'b'), KeyError)
        self.assertEqual(getitemstring(dct, '\U0001f40d'.encode()), 2)

        dct2 = ProxyGetItem(dct)
        self.assertEqual(getitemstring(dct2, b'a'), 1)
        self.assertEqual(getitemstring(dct2, b'b'), KeyError)

        self.assertRaises(TypeError, getitemstring, 42, b'a')
        self.assertRaises(UnicodeDecodeError, getitemstring, {}, b'\xff')
        self.assertRaises(SystemError, getitemstring, {}, NULL)
        self.assertRaises(TypeError, getitemstring, [], b'a')
        # CRASHES getitemstring(NULL, b'a')

    def test_mapping_haskey(self):
        haskey = _testlimitedcapi.mapping_haskey
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertTrue(haskey(dct, 'a'))
        self.assertFalse(haskey(dct, 'b'))
        self.assertTrue(haskey(dct, '\U0001f40d'))

        dct2 = ProxyGetItem(dct)
        self.assertTrue(haskey(dct2, 'a'))
        self.assertFalse(haskey(dct2, 'b'))

        self.assertTrue(haskey(['a', 'b', 'c'], 1))

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskey(42, 'a'))
            self.assertEqual(cm.unraisable.exc_type, TypeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             "'int' object is not subscriptable")

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskey({}, []))
            self.assertEqual(cm.unraisable.exc_type, TypeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             "unhashable type: 'list'")

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskey([], 1))
            self.assertEqual(cm.unraisable.exc_type, IndexError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             'list index out of range')

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskey([], 'a'))
            self.assertEqual(cm.unraisable.exc_type, TypeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             'list indices must be integers or slices, not str')

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskey({}, NULL))
            self.assertEqual(cm.unraisable.exc_type, SystemError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             'null argument to internal routine')

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskey(NULL, 'a'))
            self.assertEqual(cm.unraisable.exc_type, SystemError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             'null argument to internal routine')

    def test_mapping_haskeystring(self):
        haskeystring = _testlimitedcapi.mapping_haskeystring
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertTrue(haskeystring(dct, b'a'))
        self.assertFalse(haskeystring(dct, b'b'))
        self.assertTrue(haskeystring(dct, '\U0001f40d'.encode()))

        dct2 = ProxyGetItem(dct)
        self.assertTrue(haskeystring(dct2, b'a'))
        self.assertFalse(haskeystring(dct2, b'b'))

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskeystring(42, b'a'))
            self.assertEqual(cm.unraisable.exc_type, TypeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             "'int' object is not subscriptable")

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskeystring({}, b'\xff'))
            self.assertEqual(cm.unraisable.exc_type, UnicodeDecodeError)
            self.assertRegex(str(cm.unraisable.exc_value),
                             "'utf-8' codec can't decode")

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskeystring({}, NULL))
            self.assertEqual(cm.unraisable.exc_type, SystemError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             "null argument to internal routine")

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskeystring([], b'a'))
            self.assertEqual(cm.unraisable.exc_type, TypeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             'list indices must be integers or slices, not str')

        with support.catch_unraisable_exception() as cm:
            self.assertFalse(haskeystring(NULL, b'a'))
            self.assertEqual(cm.unraisable.exc_type, SystemError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             "null argument to internal routine")

    def test_mapping_haskeywitherror(self):
        haskey = _testlimitedcapi.mapping_haskeywitherror
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertTrue(haskey(dct, 'a'))
        self.assertFalse(haskey(dct, 'b'))
        self.assertTrue(haskey(dct, '\U0001f40d'))

        dct2 = ProxyGetItem(dct)
        self.assertTrue(haskey(dct2, 'a'))
        self.assertFalse(haskey(dct2, 'b'))

        self.assertTrue(haskey(['a', 'b', 'c'], 1))

        self.assertRaises(TypeError, haskey, 42, 'a')
        self.assertRaises(TypeError, haskey, {}, [])  # unhashable
        self.assertRaises(IndexError, haskey, [], 1)
        self.assertRaises(TypeError, haskey, [], 'a')

        # CRASHES haskey({}, NULL))
        # CRASHES haskey(NULL, 'a'))

    def test_mapping_haskeystringwitherror(self):
        haskeystring = _testlimitedcapi.mapping_haskeystringwitherror
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertTrue(haskeystring(dct, b'a'))
        self.assertFalse(haskeystring(dct, b'b'))
        self.assertTrue(haskeystring(dct, '\U0001f40d'.encode()))

        dct2 = ProxyGetItem(dct)
        self.assertTrue(haskeystring(dct2, b'a'))
        self.assertFalse(haskeystring(dct2, b'b'))

        self.assertRaises(TypeError, haskeystring, 42, b'a')
        self.assertRaises(UnicodeDecodeError, haskeystring, {}, b'\xff')
        self.assertRaises(SystemError, haskeystring, {}, NULL)
        self.assertRaises(TypeError, haskeystring, [], b'a')
        # CRASHES haskeystring(NULL, b'a')

    def test_object_setitem(self):
        setitem = _testlimitedcapi.object_setitem
        dct = {}
        setitem(dct, 'a', 5)
        self.assertEqual(dct, {'a': 5})
        setitem(dct, '\U0001f40d', 8)
        self.assertEqual(dct, {'a': 5, '\U0001f40d': 8})

        dct = {}
        dct2 = ProxySetItem(dct)
        setitem(dct2, 'a', 5)
        self.assertEqual(dct, {'a': 5})

        lst = ['a', 'b', 'c']
        setitem(lst, 1, 'x')
        self.assertEqual(lst, ['a', 'x', 'c'])

        self.assertRaises(TypeError, setitem, 42, 'a', 5)
        self.assertRaises(TypeError, setitem, {}, [], 5)  # unhashable
        self.assertRaises(SystemError, setitem, {}, NULL, 5)
        self.assertRaises(SystemError, setitem, {}, 'a', NULL)
        self.assertRaises(IndexError, setitem, [], 1, 5)
        self.assertRaises(TypeError, setitem, [], 'a', 5)
        self.assertRaises(TypeError, setitem, (), 1, 5)
        self.assertRaises(SystemError, setitem, NULL, 'a', 5)

    def test_mapping_setitemstring(self):
        setitemstring = _testlimitedcapi.mapping_setitemstring
        dct = {}
        setitemstring(dct, b'a', 5)
        self.assertEqual(dct, {'a': 5})
        setitemstring(dct, '\U0001f40d'.encode(), 8)
        self.assertEqual(dct, {'a': 5, '\U0001f40d': 8})

        dct = {}
        dct2 = ProxySetItem(dct)
        setitemstring(dct2, b'a', 5)
        self.assertEqual(dct, {'a': 5})

        self.assertRaises(TypeError, setitemstring, 42, b'a', 5)
        self.assertRaises(UnicodeDecodeError, setitemstring, {}, b'\xff', 5)
        self.assertRaises(SystemError, setitemstring, {}, NULL, 5)
        self.assertRaises(SystemError, setitemstring, {}, b'a', NULL)
        self.assertRaises(TypeError, setitemstring, [], b'a', 5)
        self.assertRaises(SystemError, setitemstring, NULL, b'a', 5)

    def test_object_delitem(self):
        for delitem in _testlimitedcapi.object_delitem, _testlimitedcapi.mapping_delitem:
            dct = {'a': 1, 'c': 2, '\U0001f40d': 3}
            delitem(dct, 'a')
            self.assertEqual(dct, {'c': 2, '\U0001f40d': 3})
            self.assertRaises(KeyError, delitem, dct, 'b')
            delitem(dct, '\U0001f40d')
            self.assertEqual(dct, {'c': 2})

            dct = {'a': 1, 'c': 2}
            dct2 = ProxyDelItem(dct)
            delitem(dct2, 'a')
            self.assertEqual(dct, {'c': 2})
            self.assertRaises(KeyError, delitem, dct2, 'b')

            lst = ['a', 'b', 'c']
            delitem(lst, 1)
            self.assertEqual(lst, ['a', 'c'])

            self.assertRaises(TypeError, delitem, 42, 'a')
            self.assertRaises(TypeError, delitem, {}, [])  # unhashable
            self.assertRaises(SystemError, delitem, {}, NULL)
            self.assertRaises(IndexError, delitem, [], 1)
            self.assertRaises(TypeError, delitem, [], 'a')
            self.assertRaises(SystemError, delitem, NULL, 'a')

    def test_mapping_delitemstring(self):
        delitemstring = _testlimitedcapi.mapping_delitemstring
        dct = {'a': 1, 'c': 2, '\U0001f40d': 3}
        delitemstring(dct, b'a')
        self.assertEqual(dct, {'c': 2, '\U0001f40d': 3})
        self.assertRaises(KeyError, delitemstring, dct, b'b')
        delitemstring(dct, '\U0001f40d'.encode())
        self.assertEqual(dct, {'c': 2})

        dct = {'a': 1, 'c': 2}
        dct2 = ProxyDelItem(dct)
        delitemstring(dct2, b'a')
        self.assertEqual(dct, {'c': 2})
        self.assertRaises(KeyError, delitemstring, dct2, b'b')

        self.assertRaises(TypeError, delitemstring, 42, b'a')
        self.assertRaises(UnicodeDecodeError, delitemstring, {}, b'\xff')
        self.assertRaises(SystemError, delitemstring, {}, NULL)
        self.assertRaises(TypeError, delitemstring, [], b'a')
        self.assertRaises(SystemError, delitemstring, NULL, b'a')

    def test_mapping_keys_valuesitems(self):
        class Mapping1(dict):
            def keys(self):
                return list(super().keys())
            def values(self):
                return list(super().values())
            def items(self):
                return list(super().items())
        class Mapping2(dict):
            def keys(self):
                return tuple(super().keys())
            def values(self):
                return tuple(super().values())
            def items(self):
                return tuple(super().items())
        dict_obj = {'foo': 1, 'bar': 2, 'spam': 3}

        for mapping in [{}, OrderedDict(), Mapping1(), Mapping2(),
                        dict_obj, OrderedDict(dict_obj),
                        Mapping1(dict_obj), Mapping2(dict_obj)]:
            self.assertListEqual(_testlimitedcapi.mapping_keys(mapping),
                                 list(mapping.keys()))
            self.assertListEqual(_testlimitedcapi.mapping_values(mapping),
                                 list(mapping.values()))
            self.assertListEqual(_testlimitedcapi.mapping_items(mapping),
                                 list(mapping.items()))

    def test_mapping_keys_valuesitems_bad_arg(self):
        self.assertRaises(AttributeError, _testlimitedcapi.mapping_keys, object())
        self.assertRaises(AttributeError, _testlimitedcapi.mapping_values, object())
        self.assertRaises(AttributeError, _testlimitedcapi.mapping_items, object())
        self.assertRaises(AttributeError, _testlimitedcapi.mapping_keys, [])
        self.assertRaises(AttributeError, _testlimitedcapi.mapping_values, [])
        self.assertRaises(AttributeError, _testlimitedcapi.mapping_items, [])
        self.assertRaises(SystemError, _testlimitedcapi.mapping_keys, NULL)
        self.assertRaises(SystemError, _testlimitedcapi.mapping_values, NULL)
        self.assertRaises(SystemError, _testlimitedcapi.mapping_items, NULL)

        class BadMapping:
            def keys(self):
                return None
            def values(self):
                return None
            def items(self):
                return None
        bad_mapping = BadMapping()
        self.assertRaises(TypeError, _testlimitedcapi.mapping_keys, bad_mapping)
        self.assertRaises(TypeError, _testlimitedcapi.mapping_values, bad_mapping)
        self.assertRaises(TypeError, _testlimitedcapi.mapping_items, bad_mapping)

    def test_sequence_check(self):
        check = _testlimitedcapi.sequence_check
        self.assertFalse(check({1: 2}))
        self.assertTrue(check([1, 2]))
        self.assertTrue(check((1, 2)))
        self.assertTrue(check('abc'))
        self.assertTrue(check(b'abc'))
        self.assertFalse(check(42))
        self.assertFalse(check(object()))
        # CRASHES check(NULL)

    def test_sequence_size(self):
        for size in _testlimitedcapi.sequence_size, _testlimitedcapi.sequence_length:
            self.assertEqual(size([1, 2]), 2)
            self.assertEqual(size((1, 2)), 2)
            self.assertEqual(size('abc'), 3)
            self.assertEqual(size(b'abc'), 3)

            self.assertRaises(TypeError, size, {})
            self.assertRaises(TypeError, size, 42)
            self.assertRaises(TypeError, size, object())
            self.assertRaises(SystemError, size, NULL)

    def test_sequence_getitem(self):
        getitem = _testlimitedcapi.sequence_getitem
        lst = ['a', 'b', 'c']
        self.assertEqual(getitem(lst, 1), 'b')
        self.assertEqual(getitem(lst, -1), 'c')
        self.assertRaises(IndexError, getitem, lst, 3)
        self.assertRaises(IndexError, getitem, lst, PY_SSIZE_T_MAX)
        self.assertRaises(IndexError, getitem, lst, PY_SSIZE_T_MIN)

        self.assertRaises(TypeError, getitem, 42, 1)
        self.assertRaises(TypeError, getitem, {}, 1)
        self.assertRaises(SystemError, getitem, NULL, 1)

    def test_sequence_concat(self):
        concat = _testlimitedcapi.sequence_concat
        self.assertEqual(concat(['a', 'b'], [1, 2]), ['a', 'b', 1, 2])
        self.assertEqual(concat(('a', 'b'), (1, 2)), ('a', 'b', 1, 2))

        self.assertRaises(TypeError, concat, [], ())
        self.assertRaises(TypeError, concat, (), [])
        self.assertRaises(TypeError, concat, [], 42)
        self.assertRaises(TypeError, concat, 42, [])
        self.assertRaises(TypeError, concat, 42, 43)
        self.assertRaises(SystemError, concat, [], NULL)
        self.assertRaises(SystemError, concat, NULL, [])

    def test_sequence_repeat(self):
        repeat = _testlimitedcapi.sequence_repeat
        self.assertEqual(repeat(['a', 'b'], 2), ['a', 'b', 'a', 'b'])
        self.assertEqual(repeat(('a', 'b'), 2), ('a', 'b', 'a', 'b'))
        self.assertEqual(repeat(['a', 'b'], 0), [])
        self.assertEqual(repeat(['a', 'b'], -1), [])
        self.assertEqual(repeat(['a', 'b'], PY_SSIZE_T_MIN), [])
        self.assertEqual(repeat([], PY_SSIZE_T_MAX), [])
        self.assertRaises(MemoryError, repeat, ['a', 'b'], PY_SSIZE_T_MAX)

        self.assertRaises(TypeError, repeat, set(), 2)
        self.assertRaises(TypeError, repeat, 42, 2)
        self.assertRaises(SystemError, repeat, NULL, 2)

    def test_sequence_inplaceconcat(self):
        inplaceconcat = _testlimitedcapi.sequence_inplaceconcat
        lst = ['a', 'b']
        res = inplaceconcat(lst, [1, 2])
        self.assertEqual(res, ['a', 'b', 1, 2])
        self.assertIs(res, lst)
        lst = ['a', 'b']
        res = inplaceconcat(lst, (1, 2))
        self.assertEqual(res, ['a', 'b', 1, 2])
        self.assertIs(res, lst)
        self.assertEqual(inplaceconcat(('a', 'b'), (1, 2)), ('a', 'b', 1, 2))

        self.assertRaises(TypeError, inplaceconcat, (), [])
        self.assertRaises(TypeError, inplaceconcat, [], 42)
        self.assertRaises(TypeError, inplaceconcat, 42, [])
        self.assertRaises(TypeError, inplaceconcat, 42, 43)
        self.assertRaises(SystemError, inplaceconcat, [], NULL)
        self.assertRaises(SystemError, inplaceconcat, NULL, [])

    def test_sequence_inplacerepeat(self):
        inplacerepeat = _testlimitedcapi.sequence_inplacerepeat
        lst = ['a', 'b']
        res = inplacerepeat(lst, 2)
        self.assertEqual(res, ['a', 'b', 'a', 'b'])
        self.assertIs(res, lst)
        self.assertEqual(inplacerepeat(('a', 'b'), 2), ('a', 'b', 'a', 'b'))
        self.assertEqual(inplacerepeat(['a', 'b'], 0), [])
        self.assertEqual(inplacerepeat(['a', 'b'], -1), [])
        self.assertEqual(inplacerepeat(['a', 'b'], PY_SSIZE_T_MIN), [])
        self.assertEqual(inplacerepeat([], PY_SSIZE_T_MAX), [])
        self.assertRaises(MemoryError, inplacerepeat, ['a', 'b'], PY_SSIZE_T_MAX)

        self.assertRaises(TypeError, inplacerepeat, set(), 2)
        self.assertRaises(TypeError, inplacerepeat, 42, 2)
        self.assertRaises(SystemError, inplacerepeat, NULL, 2)

    def test_sequence_setitem(self):
        setitem = _testlimitedcapi.sequence_setitem
        lst = ['a', 'b', 'c']
        setitem(lst, 1, 'x')
        self.assertEqual(lst, ['a', 'x', 'c'])
        setitem(lst, -1, 'y')
        self.assertEqual(lst, ['a', 'x', 'y'])

        setitem(lst, 0, NULL)
        self.assertEqual(lst, ['x', 'y'])
        self.assertRaises(IndexError, setitem, lst, 3, 'x')
        self.assertRaises(IndexError, setitem, lst, PY_SSIZE_T_MAX, 'x')
        self.assertRaises(IndexError, setitem, lst, PY_SSIZE_T_MIN, 'x')

        self.assertRaises(TypeError, setitem, 42, 1, 'x')
        self.assertRaises(TypeError, setitem, {}, 1, 'x')
        self.assertRaises(SystemError, setitem, NULL, 1, 'x')

    def test_sequence_delitem(self):
        delitem = _testlimitedcapi.sequence_delitem
        lst = ['a', 'b', 'c']
        delitem(lst, 1)
        self.assertEqual(lst, ['a', 'c'])
        delitem(lst, -1)
        self.assertEqual(lst, ['a'])
        self.assertRaises(IndexError, delitem, lst, 3)
        self.assertRaises(IndexError, delitem, lst, PY_SSIZE_T_MAX)
        self.assertRaises(IndexError, delitem, lst, PY_SSIZE_T_MIN)

        self.assertRaises(TypeError, delitem, 42, 1)
        self.assertRaises(TypeError, delitem, {}, 1)
        self.assertRaises(SystemError, delitem, NULL, 1)

    def test_sequence_setslice(self):
        setslice = _testlimitedcapi.sequence_setslice

        # Correct case:
        for start in [*range(-6, 7), PY_SSIZE_T_MIN, PY_SSIZE_T_MAX]:
            for stop in [*range(-6, 7), PY_SSIZE_T_MIN, PY_SSIZE_T_MAX]:
                data = [1, 2, 3, 4, 5]
                data_copy = [1, 2, 3, 4, 5]
                setslice(data, start, stop, [8, 9])
                data_copy[start:stop] = [8, 9]
                self.assertEqual(data, data_copy)

                data = [1, 2, 3, 4, 5]
                data_copy = [1, 2, 3, 4, 5]
                setslice(data, start, stop, NULL)
                del data_copy[start:stop]
                self.assertEqual(data, data_copy)

        # Custom class:
        class Custom:
            def __setitem__(self, index, value):
                self.index = index
                self.value = value

        c = Custom()
        setslice(c, 0, 5, 'abc')
        self.assertEqual(c.index, slice(0, 5))
        self.assertEqual(c.value, 'abc')

        # Immutable sequences must raise:
        bad_seq1 = (1, 2, 3, 4)
        self.assertRaises(TypeError, setslice, bad_seq1, 1, 3, (8, 9))
        self.assertEqual(bad_seq1, (1, 2, 3, 4))

        bad_seq2 = 'abcd'
        self.assertRaises(TypeError, setslice, bad_seq2, 1, 3, 'xy')
        self.assertEqual(bad_seq2, 'abcd')

        # Not a sequence:
        self.assertRaises(TypeError, setslice, object(), 1, 3, 'xy')
        self.assertRaises(SystemError, setslice, NULL, 1, 3, 'xy')

    def test_sequence_delslice(self):
        delslice = _testlimitedcapi.sequence_delslice

        # Correct case:
        for start in [*range(-6, 7), PY_SSIZE_T_MIN, PY_SSIZE_T_MAX]:
            for stop in [*range(-6, 7), PY_SSIZE_T_MIN, PY_SSIZE_T_MAX]:
                data = [1, 2, 3, 4, 5]
                data_copy = [1, 2, 3, 4, 5]
                delslice(data, start, stop)
                del data_copy[start:stop]
                self.assertEqual(data, data_copy)

        # Custom class:
        class Custom:
            def __delitem__(self, index):
                self.index = index

        c = Custom()
        delslice(c, 0, 5)
        self.assertEqual(c.index, slice(0, 5))

        # Immutable sequences must raise:
        bad_seq1 = (1, 2, 3, 4)
        self.assertRaises(TypeError, delslice, bad_seq1, 1, 3)
        self.assertEqual(bad_seq1, (1, 2, 3, 4))

        bad_seq2 = 'abcd'
        self.assertRaises(TypeError, delslice, bad_seq2, 1, 3)
        self.assertEqual(bad_seq2, 'abcd')

        # Not a sequence:
        self.assertRaises(TypeError, delslice, object(), 1, 3)
        self.assertRaises(SystemError, delslice, NULL, 1, 3)

        mapping = {1: 'a', 2: 'b', 3: 'c'}
        self.assertRaises(KeyError, delslice, mapping, 1, 3)
        self.assertEqual(mapping, {1: 'a', 2: 'b', 3: 'c'})

    def test_sequence_count(self):
        count = _testlimitedcapi.sequence_count

        lst = ['a', 'b', 'a']
        self.assertEqual(count(lst, 'a'), 2)
        self.assertEqual(count(lst, 'c'), 0)
        self.assertEqual(count(iter(lst), 'a'), 2)
        self.assertEqual(count(iter(lst), 'c'), 0)
        self.assertEqual(count({'a': 2}, 'a'), 1)

        self.assertRaises(TypeError, count, 42, 'a')
        self.assertRaises(SystemError, count, [], NULL)
        self.assertRaises(SystemError, count, [1], NULL)
        self.assertRaises(SystemError, count, NULL, 'a')

    def test_sequence_contains(self):
        contains = _testlimitedcapi.sequence_contains

        lst = ['a', 'b', 'a']
        self.assertEqual(contains(lst, 'a'), 1)
        self.assertEqual(contains(lst, 'c'), 0)
        self.assertEqual(contains(iter(lst), 'a'), 1)
        self.assertEqual(contains(iter(lst), 'c'), 0)
        self.assertEqual(contains({'a': 2}, 'a'), 1)

        # XXX Only for empty sequences. Should be SystemError?
        self.assertEqual(contains([], NULL), 0)

        self.assertRaises(TypeError, contains, 42, 'a')
        self.assertRaises(SystemError, contains, [1], NULL)
        # CRASHES contains({}, NULL)
        # CRASHES contains(set(), NULL)
        # CRASHES contains(NULL, 'a')

    def test_sequence_index(self):
        index = _testlimitedcapi.sequence_index

        lst = ['a', 'b', 'a']
        self.assertEqual(index(lst, 'a'), 0)
        self.assertEqual(index(lst, 'b'), 1)
        self.assertRaises(ValueError, index, lst, 'c')
        self.assertEqual(index(iter(lst), 'a'), 0)
        self.assertEqual(index(iter(lst), 'b'), 1)
        self.assertRaises(ValueError, index, iter(lst), 'c')
        dct = {'a': 2, 'b': 3}
        self.assertEqual(index(dct, 'a'), 0)
        self.assertEqual(index(dct, 'b'), 1)
        self.assertRaises(ValueError, index, dct, 'c')

        self.assertRaises(TypeError, index, 42, 'a')
        self.assertRaises(SystemError, index, [], NULL)
        self.assertRaises(SystemError, index, [1], NULL)
        self.assertRaises(SystemError, index, NULL, 'a')

    def test_sequence_list(self):
        xlist = _testlimitedcapi.sequence_list
        self.assertEqual(xlist(['a', 'b', 'c']), ['a', 'b', 'c'])
        self.assertEqual(xlist(('a', 'b', 'c')), ['a', 'b', 'c'])
        self.assertEqual(xlist(iter(['a', 'b', 'c'])), ['a', 'b', 'c'])
        self.assertEqual(xlist(gen()), ['a', 'b', 'c'])

        self.assertRaises(TypeError, xlist, 42)
        self.assertRaises(SystemError, xlist, NULL)

    def test_sequence_tuple(self):
        xtuple = _testlimitedcapi.sequence_tuple
        self.assertEqual(xtuple(['a', 'b', 'c']), ('a', 'b', 'c'))
        self.assertEqual(xtuple(('a', 'b', 'c')), ('a', 'b', 'c'))
        self.assertEqual(xtuple(iter(['a', 'b', 'c'])), ('a', 'b', 'c'))
        self.assertEqual(xtuple(gen()), ('a', 'b', 'c'))

        self.assertRaises(TypeError, xtuple, 42)
        self.assertRaises(SystemError, xtuple, NULL)

    def test_object_generichash(self):
        # Test PyObject_GenericHash()
        generichash = _testcapi.object_generichash
        for obj in object(), 1, 'string', []:
            self.assertEqual(generichash(obj), object.__hash__(obj))


if __name__ == "__main__":
    unittest.main()
