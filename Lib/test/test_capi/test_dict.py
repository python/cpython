import unittest
from collections import OrderedDict, UserDict
from types import MappingProxyType
from test import support
from test.support import import_helper


_testcapi = import_helper.import_module("_testcapi")
_testlimitedcapi = import_helper.import_module("_testlimitedcapi")


NULL = None
INVALID_UTF8 = b'\xff'

class DictSubclass(dict):
    def __getitem__(self, key):
        raise RuntimeError('do not get evil')
    def __setitem__(self, key, value):
        raise RuntimeError('do not set evil')
    def __delitem__(self, key):
        raise RuntimeError('do not del evil')

def gen():
    yield 'a'
    yield 'b'
    yield 'c'


class FrozenDictSubclass(frozendict):
    pass


DICT_TYPES = (dict, DictSubclass, OrderedDict)
FROZENDICT_TYPES = (frozendict, FrozenDictSubclass)
ANYDICT_TYPES = DICT_TYPES + FROZENDICT_TYPES
MAPPING_TYPES = (UserDict,)
NOT_DICT_TYPES = FROZENDICT_TYPES + MAPPING_TYPES
NOT_FROZENDICT_TYPES = DICT_TYPES + MAPPING_TYPES
NOT_ANYDICT_TYPES = MAPPING_TYPES
OTHER_TYPES = (lambda: [1], lambda: 42, object)  # (list, int, object)


class CAPITest(unittest.TestCase):

    def test_dict_check(self):
        # Test PyDict_Check()
        check = _testlimitedcapi.dict_check
        for dict_type in DICT_TYPES:
            self.assertTrue(check(dict_type({1: 2})))
        for test_type in NOT_DICT_TYPES:
            self.assertFalse(check(test_type({1: 2})))
        for test_type in OTHER_TYPES:
            self.assertFalse(check(test_type()))
        # CRASHES check(NULL)

    def test_dict_checkexact(self):
        # Test PyDict_CheckExact()
        check = _testlimitedcapi.dict_checkexact
        for dict_type in DICT_TYPES:
            self.assertEqual(check(dict_type({1: 2})), dict_type == dict)
        for test_type in NOT_DICT_TYPES:
            self.assertFalse(check(test_type({1: 2})))
        for test_type in OTHER_TYPES:
            self.assertFalse(check(test_type()))
        # CRASHES check(NULL)

    def test_dict_new(self):
        # Test PyDict_New()
        dict_new = _testlimitedcapi.dict_new
        dct = dict_new()
        self.assertEqual(dct, {})
        self.assertIs(type(dct), dict)
        dct2 = dict_new()
        self.assertIsNot(dct2, dct)

    def test_dictproxy_new(self):
        # Test PyDictProxy_New()
        dictproxy_new = _testlimitedcapi.dictproxy_new
        for dict_type in ANYDICT_TYPES + MAPPING_TYPES:
            if dict_type == DictSubclass:
                continue
            dct = dict_type({1: 2})
            proxy = dictproxy_new(dct)
            self.assertIs(type(proxy), MappingProxyType)
            self.assertEqual(proxy, dct)
            with self.assertRaises(TypeError):
                proxy[1] = 3
            self.assertEqual(proxy[1], 2)
            if not isinstance(dct, frozendict):
                dct[1] = 4
                self.assertEqual(proxy[1], 4)

        self.assertRaises(TypeError, dictproxy_new, [])
        self.assertRaises(TypeError, dictproxy_new, 42)
        # CRASHES dictproxy_new(NULL)

    def test_dict_copy(self):
        # Test PyDict_Copy()
        copy = _testlimitedcapi.dict_copy
        for dict_type in ANYDICT_TYPES:
            if issubclass(dict_type, frozendict):
                expected_type = frozendict
            else:
                expected_type = dict
            dct = dict_type({1: 2})
            dct_copy = copy(dct)
            self.assertIs(type(dct_copy), expected_type)
            self.assertEqual(dct_copy, dct)

        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, copy, test_type())
        self.assertRaises(SystemError, copy, NULL)

    def test_dict_clear(self):
        # Test PyDict_Clear()
        clear = _testlimitedcapi.dict_clear
        for dict_type in DICT_TYPES:
            if dict_type == OrderedDict:
                # NOTE: It is not safe to call it with OrderedDict.
                continue
            dct = dict_type({1: 2})
            clear(dct)
            self.assertEqual(dct, {})

        # Has no effect for non-dicts.
        for test_type in NOT_DICT_TYPES:
            dct = test_type({1: 2})
            clear(dct)
            self.assertEqual(dct, {1: 2})
        lst = [1, 2]
        clear(lst)
        self.assertEqual(lst, [1, 2])
        clear(object())

        # CRASHES clear(NULL)

    def test_dict_size(self):
        # Test PyDict_Size()
        size = _testlimitedcapi.dict_size
        for dict_type in ANYDICT_TYPES:
            self.assertEqual(size(dict_type({1: 2, 3: 4})), 2)

        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, size, test_type())
        self.assertRaises(SystemError, size, NULL)

    def test_dict_getitem(self):
        # Test PyDict_GetItem()
        getitem = _testlimitedcapi.dict_getitem
        for dict_type in ANYDICT_TYPES:
            dct = dict_type({'a': 1, '\U0001f40d': 2})
            self.assertEqual(getitem(dct, 'a'), 1)
            self.assertIs(getitem(dct, 'b'), KeyError)
            self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        with support.catch_unraisable_exception() as cm:
            self.assertIs(getitem({}, []), KeyError)  # unhashable
            self.assertEqual(cm.unraisable.exc_type, TypeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             "unhashable type: 'list'")

        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertIs(getitem(test_type(), 'a'), KeyError)

        # CRASHES getitem({}, NULL)
        # CRASHES getitem(NULL, 'a')

    def test_dict_getitemstring(self):
        # Test PyDict_GetItemString()
        getitemstring = _testlimitedcapi.dict_getitemstring
        for dict_type in ANYDICT_TYPES:
            dct = dict_type({'a': 1, '\U0001f40d': 2})
            self.assertEqual(getitemstring(dct, b'a'), 1)
            self.assertIs(getitemstring(dct, b'b'), KeyError)
            self.assertEqual(getitemstring(dct, '\U0001f40d'.encode()), 2)

        with support.catch_unraisable_exception() as cm:
            self.assertIs(getitemstring({}, INVALID_UTF8), KeyError)
            self.assertEqual(cm.unraisable.exc_type, UnicodeDecodeError)
            self.assertRegex(str(cm.unraisable.exc_value),
                             "'utf-8' codec can't decode")

        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertIs(getitemstring(test_type(), b'a'), KeyError)
        # CRASHES getitemstring({}, NULL)
        # CRASHES getitemstring(NULL, b'a')

    def test_dict_getitemref(self):
        # Test PyDict_GetItemRef()
        getitem = _testcapi.dict_getitemref
        for dict_type in ANYDICT_TYPES:
            dct = dict_type({'a': 1, '\U0001f40d': 2})
            self.assertEqual(getitem(dct, 'a'), 1)
            self.assertIs(getitem(dct, 'b'), KeyError)
            self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        self.assertRaises(TypeError, getitem, {}, [])  # unhashable
        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, getitem, test_type(), 'a')
        # CRASHES getitem({}, NULL)
        # CRASHES getitem(NULL, 'a')

    def test_dict_getitemstringref(self):
        # Test PyDict_GetItemStringRef()
        getitemstring = _testcapi.dict_getitemstringref
        for dict_type in ANYDICT_TYPES:
            dct = dict_type({'a': 1, '\U0001f40d': 2})
            self.assertEqual(getitemstring(dct, b'a'), 1)
            self.assertIs(getitemstring(dct, b'b'), KeyError)
            self.assertEqual(getitemstring(dct, '\U0001f40d'.encode()), 2)

        self.assertRaises(UnicodeDecodeError, getitemstring, {}, INVALID_UTF8)
        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, getitemstring, test_type(), b'a')
        # CRASHES getitemstring({}, NULL)
        # CRASHES getitemstring(NULL, b'a')

    def test_dict_getitemwitherror(self):
        # Test PyDict_GetItemWithError()
        getitem = _testlimitedcapi.dict_getitemwitherror
        for dict_type in ANYDICT_TYPES:
            dct = dict_type({'a': 1, '\U0001f40d': 2})
            self.assertEqual(getitem(dct, 'a'), 1)
            self.assertIs(getitem(dct, 'b'), KeyError)
            self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        self.assertRaises(TypeError, getitem, {}, [])  # unhashable
        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, getitem, [], 'a')
        # CRASHES getitem({}, NULL)
        # CRASHES getitem(NULL, 'a')

    def test_dict_contains(self):
        # Test PyDict_Contains()
        contains = _testlimitedcapi.dict_contains
        for dict_type in ANYDICT_TYPES:
            dct = dict_type({'a': 1, '\U0001f40d': 2})
            self.assertTrue(contains(dct, 'a'))
            self.assertFalse(contains(dct, 'b'))
            self.assertTrue(contains(dct, '\U0001f40d'))

        self.assertRaises(TypeError, contains, {}, [])  # unhashable
        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, contains, test_type(), 'a')

        # CRASHES contains({}, NULL)
        # CRASHES contains(NULL, 'a')

    def test_dict_contains_string(self):
        # Test PyDict_ContainsString()
        contains_string = _testcapi.dict_containsstring
        for dict_type in ANYDICT_TYPES:
            dct = dict_type({'a': 1, '\U0001f40d': 2})
            self.assertTrue(contains_string(dct, b'a'))
            self.assertFalse(contains_string(dct, b'b'))
            self.assertTrue(contains_string(dct, '\U0001f40d'.encode()))
            self.assertRaises(UnicodeDecodeError, contains_string, dct, INVALID_UTF8)

        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, contains_string, test_type(), b'a')

        # CRASHES contains({}, NULL)
        # CRASHES contains(NULL, b'a')

    def test_dict_setitem(self):
        # Test PyDict_SetItem()
        setitem = _testlimitedcapi.dict_setitem
        for dict_type in DICT_TYPES:
            dct = dict_type()
            setitem(dct, 'a', 5)
            self.assertEqual(dct, {'a': 5})
            setitem(dct, '\U0001f40d', 8)
            self.assertEqual(dct, {'a': 5, '\U0001f40d': 8})

        self.assertRaises(TypeError, setitem, {}, [], 5)  # unhashable
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, setitem, test_type(), 'a', 5)
        # CRASHES setitem({}, NULL, 5)
        # CRASHES setitem({}, 'a', NULL)
        # CRASHES setitem(NULL, 'a', 5)

    def test_dict_setitemstring(self):
        # Test PyDict_SetItemString()
        setitemstring = _testlimitedcapi.dict_setitemstring
        for dict_type in DICT_TYPES:
            dct = dict_type()
            setitemstring(dct, b'a', 5)
            self.assertEqual(dct, {'a': 5})
            setitemstring(dct, '\U0001f40d'.encode(), 8)
            self.assertEqual(dct, {'a': 5, '\U0001f40d': 8})

        self.assertRaises(UnicodeDecodeError, setitemstring, {}, INVALID_UTF8, 5)
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, setitemstring, test_type(), b'a', 5)
        # CRASHES setitemstring({}, NULL, 5)
        # CRASHES setitemstring({}, b'a', NULL)
        # CRASHES setitemstring(NULL, b'a', 5)

    def test_dict_delitem(self):
        # Test PyDict_DelItem()
        delitem = _testlimitedcapi.dict_delitem
        for dict_type in DICT_TYPES:
            dct = dict_type({'a': 1, 'c': 2, '\U0001f40d': 3})
            delitem(dct, 'a')
            self.assertEqual(dct, {'c': 2, '\U0001f40d': 3})
            self.assertRaises(KeyError, delitem, dct, 'b')
            delitem(dct, '\U0001f40d')
            self.assertEqual(dct, {'c': 2})

        self.assertRaises(TypeError, delitem, {}, [])  # unhashable
        for test_type in NOT_DICT_TYPES:
            self.assertRaises(SystemError, delitem, test_type({'a': 1}), 'a')
        for test_type in OTHER_TYPES:
            self.assertRaises(SystemError, delitem, test_type(), 'a')
        # CRASHES delitem({}, NULL)
        # CRASHES delitem(NULL, 'a')

    def test_dict_delitemstring(self):
        # Test PyDict_DelItemString()
        delitemstring = _testlimitedcapi.dict_delitemstring
        for dict_type in DICT_TYPES:
            dct = dict_type({'a': 1, 'c': 2, '\U0001f40d': 3})
            delitemstring(dct, b'a')
            self.assertEqual(dct, {'c': 2, '\U0001f40d': 3})
            self.assertRaises(KeyError, delitemstring, dct, b'b')
            delitemstring(dct, '\U0001f40d'.encode())
            self.assertEqual(dct, {'c': 2})

        self.assertRaises(UnicodeDecodeError, delitemstring, {}, INVALID_UTF8)
        for test_type in NOT_DICT_TYPES:
            self.assertRaises(SystemError, delitemstring, test_type({'a': 1}), b'a')
        for test_type in OTHER_TYPES:
            self.assertRaises(SystemError, delitemstring, test_type(), b'a')
        # CRASHES delitemstring({}, NULL)
        # CRASHES delitemstring(NULL, b'a')

    def test_dict_setdefault(self):
        # Test PyDict_SetDefault()
        setdefault = _testcapi.dict_setdefault
        for dict_type in DICT_TYPES:
            dct = dict_type()
            self.assertEqual(setdefault(dct, 'a', 5), 5)
            self.assertEqual(dct, {'a': 5})
            self.assertEqual(setdefault(dct, 'a', 8), 5)
            self.assertEqual(dct, {'a': 5})

        self.assertRaises(TypeError, setdefault, {}, [], 5)  # unhashable
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, setdefault, test_type(), 'a', 5)
        # CRASHES setdefault({}, NULL, 5)
        # CRASHES setdefault({}, 'a', NULL)
        # CRASHES setdefault(NULL, 'a', 5)

    def test_dict_setdefaultref(self):
        # Test PyDict_SetDefaultRef()
        setdefault = _testcapi.dict_setdefaultref
        for dict_type in DICT_TYPES:
            dct = dict_type()
            self.assertEqual(setdefault(dct, 'a', 5), 5)
            self.assertEqual(dct, {'a': 5})
            self.assertEqual(setdefault(dct, 'a', 8), 5)
            self.assertEqual(dct, {'a': 5})

        self.assertRaises(TypeError, setdefault, {}, [], 5)  # unhashable
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, setdefault, test_type(), 'a', 5)
        # CRASHES setdefault({}, NULL, 5)
        # CRASHES setdefault({}, 'a', NULL)
        # CRASHES setdefault(NULL, 'a', 5)

    def test_mapping_keys_valuesitems(self):
        # Test PyDict_Keys(), PyDict_Values() and PyDict_Items()
        class BadMapping(dict):
            def keys(self):
                return None
            def values(self):
                return None
            def items(self):
                return None
        dict_obj = {'foo': 1, 'bar': 2, 'spam': 3}
        for dict_type in ANYDICT_TYPES + (BadMapping,):
            mapping = dict_type(dict_obj)
            self.assertListEqual(_testlimitedcapi.dict_keys(mapping),
                                 list(dict_obj.keys()))
            self.assertListEqual(_testlimitedcapi.dict_values(mapping),
                                 list(dict_obj.values()))
            self.assertListEqual(_testlimitedcapi.dict_items(mapping),
                                 list(dict_obj.items()))

        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            mapping = test_type()
            self.assertRaises(SystemError, _testlimitedcapi.dict_keys, mapping)
            self.assertRaises(SystemError, _testlimitedcapi.dict_values, mapping)
            self.assertRaises(SystemError, _testlimitedcapi.dict_items, mapping)

    def test_dict_next(self):
        # Test PyDict_Next()
        dict_next = _testlimitedcapi.dict_next
        self.assertIsNone(dict_next({}, 0))
        for dict_type in ANYDICT_TYPES:
            dct = dict_type({'a': 1, 'b': 2, 'c': 3})
            pos = 0
            pairs = []
            while True:
                res = dict_next(dct, pos)
                if res is None:
                    break
                rc, pos, key, value = res
                self.assertEqual(rc, 1)
                pairs.append((key, value))
            self.assertEqual(pairs, list(dct.items()))

        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            dct = test_type()
            pos = 0
            self.assertEqual(dict_next(dct, pos), None)

        # CRASHES dict_next(NULL, 0)

    def test_dict_update(self):
        # Test PyDict_Update()
        update = _testlimitedcapi.dict_update
        for cls1 in DICT_TYPES:
            for cls2 in ANYDICT_TYPES + MAPPING_TYPES:
                dct = cls1({'a': 1, 'b': 2})
                update(dct, cls2({'b': 3, 'c': 4}))
                self.assertEqual(dct, {'a': 1, 'b': 3, 'c': 4})

        self.assertRaises(AttributeError, update, {}, [])
        self.assertRaises(AttributeError, update, {}, 42)
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, update, test_type(), {})
        self.assertRaises(SystemError, update, {}, NULL)
        self.assertRaises(SystemError, update, NULL, {})

    def test_dict_merge(self):
        # Test PyDict_Merge()
        merge = _testlimitedcapi.dict_merge
        for cls1 in DICT_TYPES:
            for cls2 in ANYDICT_TYPES + MAPPING_TYPES:
                dct = cls1({'a': 1, 'b': 2})
                merge(dct, cls2({'b': 3, 'c': 4}), 0)
                self.assertEqual(dct, {'a': 1, 'b': 2, 'c': 4})
                dct = cls1({'a': 1, 'b': 2})
                merge(dct, cls2({'b': 3, 'c': 4}), 1)
                self.assertEqual(dct, {'a': 1, 'b': 3, 'c': 4})

        self.assertRaises(AttributeError, merge, {}, [], 0)
        self.assertRaises(AttributeError, merge, {}, 42, 0)
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, merge, test_type(), {}, 0)
        self.assertRaises(SystemError, merge, {}, NULL, 0)
        self.assertRaises(SystemError, merge, NULL, {}, 0)

    def test_dict_mergefromseq2(self):
        # Test PyDict_MergeFromSeq2()
        mergefromseq2 = _testlimitedcapi.dict_mergefromseq2
        for cls1 in DICT_TYPES:
            for cls2 in list, iter:
                dct = cls1({'a': 1, 'b': 2})
                mergefromseq2(dct, cls2([('b', 3), ('c', 4)]), 0)
                self.assertEqual(dct, {'a': 1, 'b': 2, 'c': 4})
                dct = cls1({'a': 1, 'b': 2})
                mergefromseq2(dct, cls2([('b', 3), ('c', 4)]), 1)
                self.assertEqual(dct, {'a': 1, 'b': 3, 'c': 4})

        self.assertRaises(ValueError, mergefromseq2, {}, [(1,)], 0)
        self.assertRaises(ValueError, mergefromseq2, {}, [(1, 2, 3)], 0)
        self.assertRaises(TypeError, mergefromseq2, {}, [1], 0)
        self.assertRaises(TypeError, mergefromseq2, {}, 42, 0)
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            self.assertRaises(SystemError, mergefromseq2, test_type(), [], 0)
        # CRASHES mergefromseq2({}, NULL, 0)
        # CRASHES mergefromseq2(NULL, {}, 0)

    def test_dict_pop(self):
        # Test PyDict_Pop()
        dict_pop = _testcapi.dict_pop
        dict_pop_null = _testcapi.dict_pop_null

        for dict_type in DICT_TYPES:
            # key present, get removed value
            mydict = dict_type({"key": "value", "key2": "value2"})
            self.assertEqual(dict_pop(mydict, "key"), (1, "value"))
            self.assertEqual(mydict, {"key2": "value2"})
            self.assertEqual(dict_pop(mydict, "key2"), (1, "value2"))
            self.assertEqual(mydict, {})

            # key present, ignore removed value
            mydict = dict_type({"key": "value", "key2": "value2"})
            self.assertEqual(dict_pop_null(mydict, "key"), 1)
            self.assertEqual(mydict, {"key2": "value2"})
            self.assertEqual(dict_pop_null(mydict, "key2"), 1)
            self.assertEqual(mydict, {})

            # key missing, expect removed value; empty dict has a fast path
            mydict = dict_type()
            self.assertEqual(dict_pop(mydict, "key"), (0, NULL))
            mydict = dict_type({"a": 1})
            self.assertEqual(dict_pop(mydict, "key"), (0, NULL))

            # key missing, ignored removed value; empty dict has a fast path
            mydict = dict_type()
            self.assertEqual(dict_pop_null(mydict, "key"), 0)
            mydict = dict_type({"a": 1})
            self.assertEqual(dict_pop_null(mydict, "key"), 0)

            # key error; don't hash key if dict is empty
            not_hashable_key = ["list"]
            mydict = dict_type()
            self.assertEqual(dict_pop(mydict, not_hashable_key), (0, NULL))
            dict_pop(mydict, NULL)  # key is not checked if dict is empty
            mydict = dict_type({'key': 1})
            with self.assertRaises(TypeError):
                dict_pop(mydict, not_hashable_key)

        # wrong dict type
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            not_dict = test_type()
            self.assertRaises(SystemError, dict_pop, not_dict, "key")
            self.assertRaises(SystemError, dict_pop_null, not_dict, "key")

        # CRASHES dict_pop(NULL, "key")
        # CRASHES dict_pop({"a": 1}, NULL)

    def test_dict_popstring(self):
        # Test PyDict_PopString()
        dict_popstring = _testcapi.dict_popstring
        dict_popstring_null = _testcapi.dict_popstring_null

        for dict_type in DICT_TYPES:
            # key present, get removed value
            mydict = dict_type({"key": "value", "key2": "value2"})
            self.assertEqual(dict_popstring(mydict, "key"), (1, "value"))
            self.assertEqual(mydict, {"key2": "value2"})
            self.assertEqual(dict_popstring(mydict, "key2"), (1, "value2"))
            self.assertEqual(mydict, {})

            # key present, ignore removed value
            mydict = dict_type({"key": "value", "key2": "value2"})
            self.assertEqual(dict_popstring_null(mydict, "key"), 1)
            self.assertEqual(mydict, {"key2": "value2"})
            self.assertEqual(dict_popstring_null(mydict, "key2"), 1)
            self.assertEqual(mydict, {})

            # key missing; empty dict has a fast path
            mydict = dict_type()
            self.assertEqual(dict_popstring(mydict, "key"), (0, NULL))
            self.assertEqual(dict_popstring_null(mydict, "key"), 0)
            mydict = dict_type({"a": 1})
            self.assertEqual(dict_popstring(mydict, "key"), (0, NULL))
            self.assertEqual(dict_popstring_null(mydict, "key"), 0)

            # non-ASCII key
            non_ascii = '\U0001f40d'
            dct = dict_type({'\U0001f40d': 123})
            self.assertEqual(dict_popstring(dct, '\U0001f40d'.encode()), (1, 123))
            dct = dict_type({'\U0001f40d': 123})
            self.assertEqual(dict_popstring_null(dct, '\U0001f40d'.encode()), 1)

            # key error
            mydict = dict_type({1: 2})
            self.assertRaises(UnicodeDecodeError, dict_popstring, mydict, INVALID_UTF8)
            self.assertRaises(UnicodeDecodeError, dict_popstring_null, mydict, INVALID_UTF8)

        # wrong dict type
        for test_type in NOT_DICT_TYPES + OTHER_TYPES:
            not_dict = test_type()
            self.assertRaises(SystemError, dict_popstring, not_dict, "key")
            self.assertRaises(SystemError, dict_popstring_null, not_dict, "key")

        # CRASHES dict_popstring(NULL, "key")
        # CRASHES dict_popstring({}, NULL)
        # CRASHES dict_popstring({"a": 1}, NULL)

    def test_frozendict_check(self):
        # Test PyFrozenDict_Check()
        check = _testcapi.frozendict_check
        for dict_type in FROZENDICT_TYPES:
            self.assertTrue(check(dict_type(x=1)))
        for dict_type in NOT_FROZENDICT_TYPES + OTHER_TYPES:
            self.assertFalse(check(dict_type()))
        # CRASHES check(NULL)

    def test_frozendict_checkexact(self):
        # Test PyFrozenDict_CheckExact()
        check = _testcapi.frozendict_checkexact
        for dict_type in FROZENDICT_TYPES:
            self.assertEqual(check(dict_type(x=1)), dict_type == frozendict)
        for dict_type in NOT_FROZENDICT_TYPES + OTHER_TYPES:
            self.assertFalse(check(dict_type()))
        # CRASHES check(NULL)

    def test_anydict_check(self):
        # Test PyAnyDict_Check()
        check = _testcapi.anydict_check
        for dict_type in ANYDICT_TYPES:
            self.assertTrue(check(dict_type({1: 2})))
        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertFalse(check(test_type()))
        # CRASHES check(NULL)

    def test_anydict_checkexact(self):
        # Test PyAnyDict_CheckExact()
        check = _testcapi.anydict_checkexact
        for dict_type in ANYDICT_TYPES:
            self.assertEqual(check(dict_type(x=1)),
                             dict_type in (dict, frozendict))
        for test_type in NOT_ANYDICT_TYPES + OTHER_TYPES:
            self.assertFalse(check(test_type()))
        # CRASHES check(NULL)

    def test_frozendict_new(self):
        # Test PyFrozenDict_New()
        frozendict_new = _testcapi.frozendict_new

        for dict_type in ANYDICT_TYPES:
            dct = frozendict_new(dict_type({'x': 1}))
            self.assertEqual(dct, frozendict(x=1))
            self.assertIs(type(dct), frozendict)

        dct = frozendict_new([('x', 1), ('y', 2)])
        self.assertEqual(dct, frozendict(x=1, y=2))
        self.assertIs(type(dct), frozendict)

        # PyFrozenDict_New(NULL) creates an empty dictionary
        dct = frozendict_new(NULL)
        self.assertEqual(dct, frozendict())
        self.assertIs(type(dct), frozendict)


if __name__ == "__main__":
    unittest.main()
