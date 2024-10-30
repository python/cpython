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


class CAPITest(unittest.TestCase):

    def test_dict_check(self):
        check = _testlimitedcapi.dict_check
        self.assertTrue(check({1: 2}))
        self.assertTrue(check(OrderedDict({1: 2})))
        self.assertFalse(check(UserDict({1: 2})))
        self.assertFalse(check([1, 2]))
        self.assertFalse(check(object()))
        # CRASHES check(NULL)

    def test_dict_checkexact(self):
        check = _testlimitedcapi.dict_checkexact
        self.assertTrue(check({1: 2}))
        self.assertFalse(check(OrderedDict({1: 2})))
        self.assertFalse(check(UserDict({1: 2})))
        self.assertFalse(check([1, 2]))
        self.assertFalse(check(object()))
        # CRASHES check(NULL)

    def test_dict_new(self):
        dict_new = _testlimitedcapi.dict_new
        dct = dict_new()
        self.assertEqual(dct, {})
        self.assertIs(type(dct), dict)
        dct2 = dict_new()
        self.assertIsNot(dct2, dct)

    def test_dictproxy_new(self):
        dictproxy_new = _testlimitedcapi.dictproxy_new
        for dct in {1: 2}, OrderedDict({1: 2}), UserDict({1: 2}):
            proxy = dictproxy_new(dct)
            self.assertIs(type(proxy), MappingProxyType)
            self.assertEqual(proxy, dct)
            with self.assertRaises(TypeError):
                proxy[1] = 3
            self.assertEqual(proxy[1], 2)
            dct[1] = 4
            self.assertEqual(proxy[1], 4)

        self.assertRaises(TypeError, dictproxy_new, [])
        self.assertRaises(TypeError, dictproxy_new, 42)
        # CRASHES dictproxy_new(NULL)

    def test_dict_copy(self):
        copy = _testlimitedcapi.dict_copy
        for dct in {1: 2}, OrderedDict({1: 2}):
            dct_copy = copy(dct)
            self.assertIs(type(dct_copy), dict)
            self.assertEqual(dct_copy, dct)

        self.assertRaises(SystemError, copy, UserDict())
        self.assertRaises(SystemError, copy, [])
        self.assertRaises(SystemError, copy, 42)
        self.assertRaises(SystemError, copy, NULL)

    def test_dict_clear(self):
        clear = _testlimitedcapi.dict_clear
        dct = {1: 2}
        clear(dct)
        self.assertEqual(dct, {})

        # NOTE: It is not safe to call it with OrderedDict.

        # Has no effect for non-dicts.
        dct = UserDict({1: 2})
        clear(dct)
        self.assertEqual(dct, {1: 2})
        lst = [1, 2]
        clear(lst)
        self.assertEqual(lst, [1, 2])
        clear(object())

        # CRASHES? clear(NULL)

    def test_dict_size(self):
        size = _testlimitedcapi.dict_size
        self.assertEqual(size({1: 2}), 1)
        self.assertEqual(size(OrderedDict({1: 2})), 1)

        self.assertRaises(SystemError, size, UserDict())
        self.assertRaises(SystemError, size, [])
        self.assertRaises(SystemError, size, 42)
        self.assertRaises(SystemError, size, object())
        self.assertRaises(SystemError, size, NULL)

    def test_dict_getitem(self):
        getitem = _testlimitedcapi.dict_getitem
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitem(dct, 'a'), 1)
        self.assertIs(getitem(dct, 'b'), KeyError)
        self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        dct2 = DictSubclass(dct)
        self.assertEqual(getitem(dct2, 'a'), 1)
        self.assertIs(getitem(dct2, 'b'), KeyError)

        with support.catch_unraisable_exception() as cm:
            self.assertIs(getitem({}, []), KeyError)  # unhashable
            self.assertEqual(cm.unraisable.exc_type, TypeError)
            self.assertEqual(str(cm.unraisable.exc_value),
                             "unhashable type: 'list'")

        self.assertIs(getitem(42, 'a'), KeyError)
        self.assertIs(getitem([1], 0), KeyError)
        # CRASHES getitem({}, NULL)
        # CRASHES getitem(NULL, 'a')

    def test_dict_getitemstring(self):
        getitemstring = _testlimitedcapi.dict_getitemstring
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitemstring(dct, b'a'), 1)
        self.assertIs(getitemstring(dct, b'b'), KeyError)
        self.assertEqual(getitemstring(dct, '\U0001f40d'.encode()), 2)

        dct2 = DictSubclass(dct)
        self.assertEqual(getitemstring(dct2, b'a'), 1)
        self.assertIs(getitemstring(dct2, b'b'), KeyError)

        with support.catch_unraisable_exception() as cm:
            self.assertIs(getitemstring({}, INVALID_UTF8), KeyError)
            self.assertEqual(cm.unraisable.exc_type, UnicodeDecodeError)
            self.assertRegex(str(cm.unraisable.exc_value),
                             "'utf-8' codec can't decode")

        self.assertIs(getitemstring(42, b'a'), KeyError)
        self.assertIs(getitemstring([], b'a'), KeyError)
        # CRASHES getitemstring({}, NULL)
        # CRASHES getitemstring(NULL, b'a')

    def test_dict_getitemref(self):
        getitem = _testcapi.dict_getitemref
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitem(dct, 'a'), 1)
        self.assertIs(getitem(dct, 'b'), KeyError)
        self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        dct2 = DictSubclass(dct)
        self.assertEqual(getitem(dct2, 'a'), 1)
        self.assertIs(getitem(dct2, 'b'), KeyError)

        self.assertRaises(SystemError, getitem, 42, 'a')
        self.assertRaises(TypeError, getitem, {}, [])  # unhashable
        self.assertRaises(SystemError, getitem, [], 1)
        self.assertRaises(SystemError, getitem, [], 'a')
        # CRASHES getitem({}, NULL)
        # CRASHES getitem(NULL, 'a')

    def test_dict_getitemstringref(self):
        getitemstring = _testcapi.dict_getitemstringref
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitemstring(dct, b'a'), 1)
        self.assertIs(getitemstring(dct, b'b'), KeyError)
        self.assertEqual(getitemstring(dct, '\U0001f40d'.encode()), 2)

        dct2 = DictSubclass(dct)
        self.assertEqual(getitemstring(dct2, b'a'), 1)
        self.assertIs(getitemstring(dct2, b'b'), KeyError)

        self.assertRaises(SystemError, getitemstring, 42, b'a')
        self.assertRaises(UnicodeDecodeError, getitemstring, {}, INVALID_UTF8)
        self.assertRaises(SystemError, getitemstring, [], b'a')
        # CRASHES getitemstring({}, NULL)
        # CRASHES getitemstring(NULL, b'a')

    def test_dict_getitemwitherror(self):
        getitem = _testlimitedcapi.dict_getitemwitherror
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitem(dct, 'a'), 1)
        self.assertIs(getitem(dct, 'b'), KeyError)
        self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        dct2 = DictSubclass(dct)
        self.assertEqual(getitem(dct2, 'a'), 1)
        self.assertIs(getitem(dct2, 'b'), KeyError)

        self.assertRaises(SystemError, getitem, 42, 'a')
        self.assertRaises(TypeError, getitem, {}, [])  # unhashable
        self.assertRaises(SystemError, getitem, [], 1)
        self.assertRaises(SystemError, getitem, [], 'a')
        # CRASHES getitem({}, NULL)
        # CRASHES getitem(NULL, 'a')

    def test_dict_contains(self):
        contains = _testlimitedcapi.dict_contains
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertTrue(contains(dct, 'a'))
        self.assertFalse(contains(dct, 'b'))
        self.assertTrue(contains(dct, '\U0001f40d'))

        dct2 = DictSubclass(dct)
        self.assertTrue(contains(dct2, 'a'))
        self.assertFalse(contains(dct2, 'b'))

        self.assertRaises(TypeError, contains, {}, [])  # unhashable
        # CRASHES contains({}, NULL)
        # CRASHES contains(UserDict(), 'a')
        # CRASHES contains(42, 'a')
        # CRASHES contains(NULL, 'a')

    def test_dict_contains_string(self):
        contains_string = _testcapi.dict_containsstring
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertTrue(contains_string(dct, b'a'))
        self.assertFalse(contains_string(dct, b'b'))
        self.assertTrue(contains_string(dct, '\U0001f40d'.encode()))
        self.assertRaises(UnicodeDecodeError, contains_string, dct, INVALID_UTF8)

        dct2 = DictSubclass(dct)
        self.assertTrue(contains_string(dct2, b'a'))
        self.assertFalse(contains_string(dct2, b'b'))

        # CRASHES contains({}, NULL)
        # CRASHES contains(NULL, b'a')

    def test_dict_setitem(self):
        setitem = _testlimitedcapi.dict_setitem
        dct = {}
        setitem(dct, 'a', 5)
        self.assertEqual(dct, {'a': 5})
        setitem(dct, '\U0001f40d', 8)
        self.assertEqual(dct, {'a': 5, '\U0001f40d': 8})

        dct2 = DictSubclass()
        setitem(dct2, 'a', 5)
        self.assertEqual(dct2, {'a': 5})

        self.assertRaises(TypeError, setitem, {}, [], 5)  # unhashable
        self.assertRaises(SystemError, setitem, UserDict(), 'a', 5)
        self.assertRaises(SystemError, setitem, [1], 0, 5)
        self.assertRaises(SystemError, setitem, 42, 'a', 5)
        # CRASHES setitem({}, NULL, 5)
        # CRASHES setitem({}, 'a', NULL)
        # CRASHES setitem(NULL, 'a', 5)

    def test_dict_setitemstring(self):
        setitemstring = _testlimitedcapi.dict_setitemstring
        dct = {}
        setitemstring(dct, b'a', 5)
        self.assertEqual(dct, {'a': 5})
        setitemstring(dct, '\U0001f40d'.encode(), 8)
        self.assertEqual(dct, {'a': 5, '\U0001f40d': 8})

        dct2 = DictSubclass()
        setitemstring(dct2, b'a', 5)
        self.assertEqual(dct2, {'a': 5})

        self.assertRaises(UnicodeDecodeError, setitemstring, {}, INVALID_UTF8, 5)
        self.assertRaises(SystemError, setitemstring, UserDict(), b'a', 5)
        self.assertRaises(SystemError, setitemstring, 42, b'a', 5)
        # CRASHES setitemstring({}, NULL, 5)
        # CRASHES setitemstring({}, b'a', NULL)
        # CRASHES setitemstring(NULL, b'a', 5)

    def test_dict_delitem(self):
        delitem = _testlimitedcapi.dict_delitem
        dct = {'a': 1, 'c': 2, '\U0001f40d': 3}
        delitem(dct, 'a')
        self.assertEqual(dct, {'c': 2, '\U0001f40d': 3})
        self.assertRaises(KeyError, delitem, dct, 'b')
        delitem(dct, '\U0001f40d')
        self.assertEqual(dct, {'c': 2})

        dct2 = DictSubclass({'a': 1, 'c': 2})
        delitem(dct2, 'a')
        self.assertEqual(dct2, {'c': 2})
        self.assertRaises(KeyError, delitem, dct2, 'b')

        self.assertRaises(TypeError, delitem, {}, [])  # unhashable
        self.assertRaises(SystemError, delitem, UserDict({'a': 1}), 'a')
        self.assertRaises(SystemError, delitem, [1], 0)
        self.assertRaises(SystemError, delitem, 42, 'a')
        # CRASHES delitem({}, NULL)
        # CRASHES delitem(NULL, 'a')

    def test_dict_delitemstring(self):
        delitemstring = _testlimitedcapi.dict_delitemstring
        dct = {'a': 1, 'c': 2, '\U0001f40d': 3}
        delitemstring(dct, b'a')
        self.assertEqual(dct, {'c': 2, '\U0001f40d': 3})
        self.assertRaises(KeyError, delitemstring, dct, b'b')
        delitemstring(dct, '\U0001f40d'.encode())
        self.assertEqual(dct, {'c': 2})

        dct2 = DictSubclass({'a': 1, 'c': 2})
        delitemstring(dct2, b'a')
        self.assertEqual(dct2, {'c': 2})
        self.assertRaises(KeyError, delitemstring, dct2, b'b')

        self.assertRaises(UnicodeDecodeError, delitemstring, {}, INVALID_UTF8)
        self.assertRaises(SystemError, delitemstring, UserDict({'a': 1}), b'a')
        self.assertRaises(SystemError, delitemstring, 42, b'a')
        # CRASHES delitemstring({}, NULL)
        # CRASHES delitemstring(NULL, b'a')

    def test_dict_setdefault(self):
        setdefault = _testcapi.dict_setdefault
        dct = {}
        self.assertEqual(setdefault(dct, 'a', 5), 5)
        self.assertEqual(dct, {'a': 5})
        self.assertEqual(setdefault(dct, 'a', 8), 5)
        self.assertEqual(dct, {'a': 5})

        dct2 = DictSubclass()
        self.assertEqual(setdefault(dct2, 'a', 5), 5)
        self.assertEqual(dct2, {'a': 5})
        self.assertEqual(setdefault(dct2, 'a', 8), 5)
        self.assertEqual(dct2, {'a': 5})

        self.assertRaises(TypeError, setdefault, {}, [], 5)  # unhashable
        self.assertRaises(SystemError, setdefault, UserDict(), 'a', 5)
        self.assertRaises(SystemError, setdefault, [1], 0, 5)
        self.assertRaises(SystemError, setdefault, 42, 'a', 5)
        # CRASHES setdefault({}, NULL, 5)
        # CRASHES setdefault({}, 'a', NULL)
        # CRASHES setdefault(NULL, 'a', 5)

    def test_dict_setdefaultref(self):
        setdefault = _testcapi.dict_setdefaultref
        dct = {}
        self.assertEqual(setdefault(dct, 'a', 5), 5)
        self.assertEqual(dct, {'a': 5})
        self.assertEqual(setdefault(dct, 'a', 8), 5)
        self.assertEqual(dct, {'a': 5})

        dct2 = DictSubclass()
        self.assertEqual(setdefault(dct2, 'a', 5), 5)
        self.assertEqual(dct2, {'a': 5})
        self.assertEqual(setdefault(dct2, 'a', 8), 5)
        self.assertEqual(dct2, {'a': 5})

        self.assertRaises(TypeError, setdefault, {}, [], 5)  # unhashable
        self.assertRaises(SystemError, setdefault, UserDict(), 'a', 5)
        self.assertRaises(SystemError, setdefault, [1], 0, 5)
        self.assertRaises(SystemError, setdefault, 42, 'a', 5)
        # CRASHES setdefault({}, NULL, 5)
        # CRASHES setdefault({}, 'a', NULL)
        # CRASHES setdefault(NULL, 'a', 5)

    def test_mapping_keys_valuesitems(self):
        class BadMapping(dict):
            def keys(self):
                return None
            def values(self):
                return None
            def items(self):
                return None
        dict_obj = {'foo': 1, 'bar': 2, 'spam': 3}
        for mapping in [dict_obj, DictSubclass(dict_obj), BadMapping(dict_obj)]:
            self.assertListEqual(_testlimitedcapi.dict_keys(mapping),
                                 list(dict_obj.keys()))
            self.assertListEqual(_testlimitedcapi.dict_values(mapping),
                                 list(dict_obj.values()))
            self.assertListEqual(_testlimitedcapi.dict_items(mapping),
                                 list(dict_obj.items()))

    def test_dict_keys_valuesitems_bad_arg(self):
        for mapping in UserDict(), [], object():
            self.assertRaises(SystemError, _testlimitedcapi.dict_keys, mapping)
            self.assertRaises(SystemError, _testlimitedcapi.dict_values, mapping)
            self.assertRaises(SystemError, _testlimitedcapi.dict_items, mapping)

    def test_dict_next(self):
        dict_next = _testlimitedcapi.dict_next
        self.assertIsNone(dict_next({}, 0))
        dct = {'a': 1, 'b': 2, 'c': 3}
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

        # CRASHES dict_next(NULL, 0)

    def test_dict_update(self):
        update = _testlimitedcapi.dict_update
        for cls1 in dict, DictSubclass:
            for cls2 in dict, DictSubclass, UserDict:
                dct = cls1({'a': 1, 'b': 2})
                update(dct, cls2({'b': 3, 'c': 4}))
                self.assertEqual(dct, {'a': 1, 'b': 3, 'c': 4})

        self.assertRaises(AttributeError, update, {}, [])
        self.assertRaises(AttributeError, update, {}, 42)
        self.assertRaises(SystemError, update, UserDict(), {})
        self.assertRaises(SystemError, update, 42, {})
        self.assertRaises(SystemError, update, {}, NULL)
        self.assertRaises(SystemError, update, NULL, {})

    def test_dict_merge(self):
        merge = _testlimitedcapi.dict_merge
        for cls1 in dict, DictSubclass:
            for cls2 in dict, DictSubclass, UserDict:
                dct = cls1({'a': 1, 'b': 2})
                merge(dct, cls2({'b': 3, 'c': 4}), 0)
                self.assertEqual(dct, {'a': 1, 'b': 2, 'c': 4})
                dct = cls1({'a': 1, 'b': 2})
                merge(dct, cls2({'b': 3, 'c': 4}), 1)
                self.assertEqual(dct, {'a': 1, 'b': 3, 'c': 4})

        self.assertRaises(AttributeError, merge, {}, [], 0)
        self.assertRaises(AttributeError, merge, {}, 42, 0)
        self.assertRaises(SystemError, merge, UserDict(), {}, 0)
        self.assertRaises(SystemError, merge, 42, {}, 0)
        self.assertRaises(SystemError, merge, {}, NULL, 0)
        self.assertRaises(SystemError, merge, NULL, {}, 0)

    def test_dict_mergefromseq2(self):
        mergefromseq2 = _testlimitedcapi.dict_mergefromseq2
        for cls1 in dict, DictSubclass:
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
        # CRASHES mergefromseq2(UserDict(), [], 0)
        # CRASHES mergefromseq2(42, [], 0)
        # CRASHES mergefromseq2({}, NULL, 0)
        # CRASHES mergefromseq2(NULL, {}, 0)

    def test_dict_pop(self):
        # Test PyDict_Pop()
        dict_pop = _testcapi.dict_pop
        dict_pop_null = _testcapi.dict_pop_null

        # key present, get removed value
        mydict = {"key": "value", "key2": "value2"}
        self.assertEqual(dict_pop(mydict, "key"), (1, "value"))
        self.assertEqual(mydict, {"key2": "value2"})
        self.assertEqual(dict_pop(mydict, "key2"), (1, "value2"))
        self.assertEqual(mydict, {})

        # key present, ignore removed value
        mydict = {"key": "value", "key2": "value2"}
        self.assertEqual(dict_pop_null(mydict, "key"), 1)
        self.assertEqual(mydict, {"key2": "value2"})
        self.assertEqual(dict_pop_null(mydict, "key2"), 1)
        self.assertEqual(mydict, {})

        # key missing, expect removed value; empty dict has a fast path
        self.assertEqual(dict_pop({}, "key"), (0, NULL))
        self.assertEqual(dict_pop({"a": 1}, "key"), (0, NULL))

        # key missing, ignored removed value; empty dict has a fast path
        self.assertEqual(dict_pop_null({}, "key"), 0)
        self.assertEqual(dict_pop_null({"a": 1}, "key"), 0)

        # dict error
        not_dict = UserDict({1: 2})
        self.assertRaises(SystemError, dict_pop, not_dict, "key")
        self.assertRaises(SystemError, dict_pop_null, not_dict, "key")

        # key error; don't hash key if dict is empty
        not_hashable_key = ["list"]
        self.assertEqual(dict_pop({}, not_hashable_key), (0, NULL))
        with self.assertRaises(TypeError):
            dict_pop({'key': 1}, not_hashable_key)
        dict_pop({}, NULL)  # key is not checked if dict is empty

        # CRASHES dict_pop(NULL, "key")
        # CRASHES dict_pop({"a": 1}, NULL)

    def test_dict_popstring(self):
        # Test PyDict_PopString()
        dict_popstring = _testcapi.dict_popstring
        dict_popstring_null = _testcapi.dict_popstring_null

        # key present, get removed value
        mydict = {"key": "value", "key2": "value2"}
        self.assertEqual(dict_popstring(mydict, "key"), (1, "value"))
        self.assertEqual(mydict, {"key2": "value2"})
        self.assertEqual(dict_popstring(mydict, "key2"), (1, "value2"))
        self.assertEqual(mydict, {})

        # key present, ignore removed value
        mydict = {"key": "value", "key2": "value2"}
        self.assertEqual(dict_popstring_null(mydict, "key"), 1)
        self.assertEqual(mydict, {"key2": "value2"})
        self.assertEqual(dict_popstring_null(mydict, "key2"), 1)
        self.assertEqual(mydict, {})

        # key missing; empty dict has a fast path
        self.assertEqual(dict_popstring({}, "key"), (0, NULL))
        self.assertEqual(dict_popstring_null({}, "key"), 0)
        self.assertEqual(dict_popstring({"a": 1}, "key"), (0, NULL))
        self.assertEqual(dict_popstring_null({"a": 1}, "key"), 0)

        # non-ASCII key
        non_ascii = '\U0001f40d'
        dct = {'\U0001f40d': 123}
        self.assertEqual(dict_popstring(dct, '\U0001f40d'.encode()), (1, 123))
        dct = {'\U0001f40d': 123}
        self.assertEqual(dict_popstring_null(dct, '\U0001f40d'.encode()), 1)

        # dict error
        not_dict = UserDict({1: 2})
        self.assertRaises(SystemError, dict_popstring, not_dict, "key")
        self.assertRaises(SystemError, dict_popstring_null, not_dict, "key")

        # key error
        self.assertRaises(UnicodeDecodeError, dict_popstring, {1: 2}, INVALID_UTF8)
        self.assertRaises(UnicodeDecodeError, dict_popstring_null, {1: 2}, INVALID_UTF8)

        # CRASHES dict_popstring(NULL, "key")
        # CRASHES dict_popstring({}, NULL)
        # CRASHES dict_popstring({"a": 1}, NULL)


if __name__ == "__main__":
    unittest.main()
