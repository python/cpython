import unittest
import sys
from collections import OrderedDict, UserDict
from types import MappingProxyType
from test import support
from test.support import import_helper


_testcapi = import_helper.import_module("_testcapi")


NULL = None

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
        check = _testcapi.dict_check
        self.assertTrue(check({1: 2}))
        self.assertTrue(check(OrderedDict({1: 2})))
        self.assertFalse(check(UserDict({1: 2})))
        self.assertFalse(check([1, 2]))
        self.assertFalse(check(object()))
        #self.assertFalse(check(NULL))

    def test_dict_checkexact(self):
        check = _testcapi.dict_checkexact
        self.assertTrue(check({1: 2}))
        self.assertFalse(check(OrderedDict({1: 2})))
        self.assertFalse(check(UserDict({1: 2})))
        self.assertFalse(check([1, 2]))
        self.assertFalse(check(object()))
        #self.assertFalse(check(NULL))

    def test_dict_new(self):
        dict_new = _testcapi.dict_new
        dct = dict_new()
        self.assertEqual(dct, {})
        self.assertIs(type(dct), dict)
        dct2 = dict_new()
        self.assertIsNot(dct2, dct)

    def test_dictproxy_new(self):
        dictproxy_new = _testcapi.dictproxy_new
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
        copy = _testcapi.dict_copy
        for dct in {1: 2}, OrderedDict({1: 2}):
            dct_copy = copy(dct)
            self.assertIs(type(dct_copy), dict)
            self.assertEqual(dct_copy, dct)

        self.assertRaises(SystemError, copy, UserDict())
        self.assertRaises(SystemError, copy, [])
        self.assertRaises(SystemError, copy, 42)
        self.assertRaises(SystemError, copy, NULL)

    def test_dict_clear(self):
        clear = _testcapi.dict_clear
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
        size = _testcapi.dict_size
        self.assertEqual(size({1: 2}), 1)
        self.assertEqual(size(OrderedDict({1: 2})), 1)

        self.assertRaises(SystemError, size, UserDict())
        self.assertRaises(SystemError, size, [])
        self.assertRaises(SystemError, size, 42)
        self.assertRaises(SystemError, size, object())
        self.assertRaises(SystemError, size, NULL)

    def test_dict_getitem(self):
        getitem = _testcapi.dict_getitem
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitem(dct, 'a'), 1)
        self.assertIs(getitem(dct, 'b'), KeyError)
        self.assertEqual(getitem(dct, '\U0001f40d'), 2)

        dct2 = DictSubclass(dct)
        self.assertEqual(getitem(dct2, 'a'), 1)
        self.assertIs(getitem(dct2, 'b'), KeyError)

        self.assertIs(getitem({}, []), KeyError)  # unhashable
        self.assertIs(getitem(42, 'a'), KeyError)
        self.assertIs(getitem([1], 0), KeyError)
        # CRASHES getitem({}, NULL)
        # CRASHES getitem(NULL, 'a')

    def test_dict_getitemstring(self):
        getitemstring = _testcapi.dict_getitemstring
        dct = {'a': 1, '\U0001f40d': 2}
        self.assertEqual(getitemstring(dct, b'a'), 1)
        self.assertIs(getitemstring(dct, b'b'), KeyError)
        self.assertEqual(getitemstring(dct, '\U0001f40d'.encode()), 2)

        dct2 = DictSubclass(dct)
        self.assertEqual(getitemstring(dct2, b'a'), 1)
        self.assertIs(getitemstring(dct2, b'b'), KeyError)

        self.assertIs(getitemstring({}, b'\xff'), KeyError)
        self.assertIs(getitemstring(42, b'a'), KeyError)
        self.assertIs(getitemstring([], b'a'), KeyError)
        # CRASHES getitemstring({}, NULL)
        # CRASHES getitemstring(NULL, b'a')

    def test_dict_getitemwitherror(self):
        getitem = _testcapi.dict_getitemwitherror
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
        contains = _testcapi.dict_contains
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

    def test_dict_setitem(self):
        setitem = _testcapi.dict_setitem
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
        setitemstring = _testcapi.dict_setitemstring
        dct = {}
        setitemstring(dct, b'a', 5)
        self.assertEqual(dct, {'a': 5})
        setitemstring(dct, '\U0001f40d'.encode(), 8)
        self.assertEqual(dct, {'a': 5, '\U0001f40d': 8})

        dct2 = DictSubclass()
        setitemstring(dct2, b'a', 5)
        self.assertEqual(dct2, {'a': 5})

        self.assertRaises(UnicodeDecodeError, setitemstring, {}, b'\xff', 5)
        self.assertRaises(SystemError, setitemstring, UserDict(), b'a', 5)
        self.assertRaises(SystemError, setitemstring, 42, b'a', 5)
        # CRASHES setitemstring({}, NULL, 5)
        # CRASHES setitemstring({}, b'a', NULL)
        # CRASHES setitemstring(NULL, b'a', 5)

    def test_dict_delitem(self):
        delitem = _testcapi.dict_delitem
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
        delitemstring = _testcapi.dict_delitemstring
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

        self.assertRaises(UnicodeDecodeError, delitemstring, {}, b'\xff')
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
            self.assertListEqual(_testcapi.dict_keys(mapping),
                                 list(dict_obj.keys()))
            self.assertListEqual(_testcapi.dict_values(mapping),
                                 list(dict_obj.values()))
            self.assertListEqual(_testcapi.dict_items(mapping),
                                 list(dict_obj.items()))

    def test_dict_keys_valuesitems_bad_arg(self):
        for mapping in UserDict(), [], object():
            self.assertRaises(SystemError, _testcapi.dict_keys, mapping)
            self.assertRaises(SystemError, _testcapi.dict_values, mapping)
            self.assertRaises(SystemError, _testcapi.dict_items, mapping)

    def test_dict_next(self):
        dict_next = _testcapi.dict_next
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
        update = _testcapi.dict_update
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
        merge = _testcapi.dict_merge
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
        mergefromseq2 = _testcapi.dict_mergefromseq2
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


if __name__ == "__main__":
    unittest.main()
