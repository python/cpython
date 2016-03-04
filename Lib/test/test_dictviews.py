import copy
import pickle
import unittest
import collections
from test import test_support

class DictSetTest(unittest.TestCase):

    def test_constructors_not_callable(self):
        kt = type({}.viewkeys())
        self.assertRaises(TypeError, kt, {})
        self.assertRaises(TypeError, kt)
        it = type({}.viewitems())
        self.assertRaises(TypeError, it, {})
        self.assertRaises(TypeError, it)
        vt = type({}.viewvalues())
        self.assertRaises(TypeError, vt, {})
        self.assertRaises(TypeError, vt)

    def test_dict_keys(self):
        d = {1: 10, "a": "ABC"}
        keys = d.viewkeys()
        self.assertEqual(len(keys), 2)
        self.assertEqual(set(keys), set([1, "a"]))
        self.assertEqual(keys, set([1, "a"]))
        self.assertNotEqual(keys, set([1, "a", "b"]))
        self.assertNotEqual(keys, set([1, "b"]))
        self.assertNotEqual(keys, set([1]))
        self.assertNotEqual(keys, 42)
        self.assertIn(1, keys)
        self.assertIn("a", keys)
        self.assertNotIn(10, keys)
        self.assertNotIn("Z", keys)
        self.assertEqual(d.viewkeys(), d.viewkeys())
        e = {1: 11, "a": "def"}
        self.assertEqual(d.viewkeys(), e.viewkeys())
        del e["a"]
        self.assertNotEqual(d.viewkeys(), e.viewkeys())

    def test_dict_items(self):
        d = {1: 10, "a": "ABC"}
        items = d.viewitems()
        self.assertEqual(len(items), 2)
        self.assertEqual(set(items), set([(1, 10), ("a", "ABC")]))
        self.assertEqual(items, set([(1, 10), ("a", "ABC")]))
        self.assertNotEqual(items, set([(1, 10), ("a", "ABC"), "junk"]))
        self.assertNotEqual(items, set([(1, 10), ("a", "def")]))
        self.assertNotEqual(items, set([(1, 10)]))
        self.assertNotEqual(items, 42)
        self.assertIn((1, 10), items)
        self.assertIn(("a", "ABC"), items)
        self.assertNotIn((1, 11), items)
        self.assertNotIn(1, items)
        self.assertNotIn((), items)
        self.assertNotIn((1,), items)
        self.assertNotIn((1, 2, 3), items)
        self.assertEqual(d.viewitems(), d.viewitems())
        e = d.copy()
        self.assertEqual(d.viewitems(), e.viewitems())
        e["a"] = "def"
        self.assertNotEqual(d.viewitems(), e.viewitems())

    def test_dict_mixed_keys_items(self):
        d = {(1, 1): 11, (2, 2): 22}
        e = {1: 1, 2: 2}
        self.assertEqual(d.viewkeys(), e.viewitems())
        self.assertNotEqual(d.viewitems(), e.viewkeys())

    def test_dict_values(self):
        d = {1: 10, "a": "ABC"}
        values = d.viewvalues()
        self.assertEqual(set(values), set([10, "ABC"]))
        self.assertEqual(len(values), 2)

    def test_dict_repr(self):
        d = {1: 10, "a": "ABC"}
        self.assertIsInstance(repr(d), str)
        r = repr(d.viewitems())
        self.assertIsInstance(r, str)
        self.assertTrue(r == "dict_items([('a', 'ABC'), (1, 10)])" or
                        r == "dict_items([(1, 10), ('a', 'ABC')])")
        r = repr(d.viewkeys())
        self.assertIsInstance(r, str)
        self.assertTrue(r == "dict_keys(['a', 1])" or
                        r == "dict_keys([1, 'a'])")
        r = repr(d.viewvalues())
        self.assertIsInstance(r, str)
        self.assertTrue(r == "dict_values(['ABC', 10])" or
                        r == "dict_values([10, 'ABC'])")

    def test_keys_set_operations(self):
        d1 = {'a': 1, 'b': 2}
        d2 = {'b': 3, 'c': 2}
        d3 = {'d': 4, 'e': 5}
        self.assertEqual(d1.viewkeys() & d1.viewkeys(), {'a', 'b'})
        self.assertEqual(d1.viewkeys() & d2.viewkeys(), {'b'})
        self.assertEqual(d1.viewkeys() & d3.viewkeys(), set())
        self.assertEqual(d1.viewkeys() & set(d1.viewkeys()), {'a', 'b'})
        self.assertEqual(d1.viewkeys() & set(d2.viewkeys()), {'b'})
        self.assertEqual(d1.viewkeys() & set(d3.viewkeys()), set())
        self.assertEqual(d1.viewkeys() & tuple(d1.viewkeys()), {'a', 'b'})

        self.assertEqual(d1.viewkeys() | d1.viewkeys(), {'a', 'b'})
        self.assertEqual(d1.viewkeys() | d2.viewkeys(), {'a', 'b', 'c'})
        self.assertEqual(d1.viewkeys() | d3.viewkeys(), {'a', 'b', 'd', 'e'})
        self.assertEqual(d1.viewkeys() | set(d1.viewkeys()), {'a', 'b'})
        self.assertEqual(d1.viewkeys() | set(d2.viewkeys()), {'a', 'b', 'c'})
        self.assertEqual(d1.viewkeys() | set(d3.viewkeys()),
                         {'a', 'b', 'd', 'e'})
        self.assertEqual(d1.viewkeys() | (1, 2), {'a', 'b', 1, 2})

        self.assertEqual(d1.viewkeys() ^ d1.viewkeys(), set())
        self.assertEqual(d1.viewkeys() ^ d2.viewkeys(), {'a', 'c'})
        self.assertEqual(d1.viewkeys() ^ d3.viewkeys(), {'a', 'b', 'd', 'e'})
        self.assertEqual(d1.viewkeys() ^ set(d1.viewkeys()), set())
        self.assertEqual(d1.viewkeys() ^ set(d2.viewkeys()), {'a', 'c'})
        self.assertEqual(d1.viewkeys() ^ set(d3.viewkeys()),
                         {'a', 'b', 'd', 'e'})
        self.assertEqual(d1.viewkeys() ^ tuple(d2.keys()), {'a', 'c'})

        self.assertEqual(d1.viewkeys() - d1.viewkeys(), set())
        self.assertEqual(d1.viewkeys() - d2.viewkeys(), {'a'})
        self.assertEqual(d1.viewkeys() - d3.viewkeys(), {'a', 'b'})
        self.assertEqual(d1.viewkeys() - set(d1.viewkeys()), set())
        self.assertEqual(d1.viewkeys() - set(d2.viewkeys()), {'a'})
        self.assertEqual(d1.viewkeys() - set(d3.viewkeys()), {'a', 'b'})
        self.assertEqual(d1.viewkeys() - (0, 1), {'a', 'b'})

    def test_items_set_operations(self):
        d1 = {'a': 1, 'b': 2}
        d2 = {'a': 2, 'b': 2}
        d3 = {'d': 4, 'e': 5}
        self.assertEqual(
            d1.viewitems() & d1.viewitems(), {('a', 1), ('b', 2)})
        self.assertEqual(d1.viewitems() & d2.viewitems(), {('b', 2)})
        self.assertEqual(d1.viewitems() & d3.viewitems(), set())
        self.assertEqual(d1.viewitems() & set(d1.viewitems()),
                         {('a', 1), ('b', 2)})
        self.assertEqual(d1.viewitems() & set(d2.viewitems()), {('b', 2)})
        self.assertEqual(d1.viewitems() & set(d3.viewitems()), set())

        self.assertEqual(d1.viewitems() | d1.viewitems(),
                         {('a', 1), ('b', 2)})
        self.assertEqual(d1.viewitems() | d2.viewitems(),
                         {('a', 1), ('a', 2), ('b', 2)})
        self.assertEqual(d1.viewitems() | d3.viewitems(),
                         {('a', 1), ('b', 2), ('d', 4), ('e', 5)})
        self.assertEqual(d1.viewitems() | set(d1.viewitems()),
                         {('a', 1), ('b', 2)})
        self.assertEqual(d1.viewitems() | set(d2.viewitems()),
                         {('a', 1), ('a', 2), ('b', 2)})
        self.assertEqual(d1.viewitems() | set(d3.viewitems()),
                         {('a', 1), ('b', 2), ('d', 4), ('e', 5)})

        self.assertEqual(d1.viewitems() ^ d1.viewitems(), set())
        self.assertEqual(d1.viewitems() ^ d2.viewitems(),
                         {('a', 1), ('a', 2)})
        self.assertEqual(d1.viewitems() ^ d3.viewitems(),
                         {('a', 1), ('b', 2), ('d', 4), ('e', 5)})

        self.assertEqual(d1.viewitems() - d1.viewitems(), set())
        self.assertEqual(d1.viewitems() - d2.viewitems(), {('a', 1)})
        self.assertEqual(d1.viewitems() - d3.viewitems(), {('a', 1), ('b', 2)})
        self.assertEqual(d1.viewitems() - set(d1.viewitems()), set())
        self.assertEqual(d1.viewitems() - set(d2.viewitems()), {('a', 1)})
        self.assertEqual(d1.viewitems() - set(d3.viewitems()),
                         {('a', 1), ('b', 2)})

    def test_recursive_repr(self):
        d = {}
        d[42] = d.viewvalues()
        self.assertRaises(RuntimeError, repr, d)

    def test_abc_registry(self):
        d = dict(a=1)

        self.assertIsInstance(d.viewkeys(), collections.KeysView)
        self.assertIsInstance(d.viewkeys(), collections.MappingView)
        self.assertIsInstance(d.viewkeys(), collections.Set)
        self.assertIsInstance(d.viewkeys(), collections.Sized)
        self.assertIsInstance(d.viewkeys(), collections.Iterable)
        self.assertIsInstance(d.viewkeys(), collections.Container)

        self.assertIsInstance(d.viewvalues(), collections.ValuesView)
        self.assertIsInstance(d.viewvalues(), collections.MappingView)
        self.assertIsInstance(d.viewvalues(), collections.Sized)

        self.assertIsInstance(d.viewitems(), collections.ItemsView)
        self.assertIsInstance(d.viewitems(), collections.MappingView)
        self.assertIsInstance(d.viewitems(), collections.Set)
        self.assertIsInstance(d.viewitems(), collections.Sized)
        self.assertIsInstance(d.viewitems(), collections.Iterable)
        self.assertIsInstance(d.viewitems(), collections.Container)

    def test_copy(self):
        d = {1: 10, "a": "ABC"}
        self.assertRaises(TypeError, copy.copy, d.viewkeys())
        self.assertRaises(TypeError, copy.copy, d.viewvalues())
        self.assertRaises(TypeError, copy.copy, d.viewitems())

    def test_pickle(self):
        d = {1: 10, "a": "ABC"}
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            self.assertRaises((TypeError, pickle.PicklingError),
                pickle.dumps, d.viewkeys(), proto)
            self.assertRaises((TypeError, pickle.PicklingError),
                pickle.dumps, d.viewvalues(), proto)
            self.assertRaises((TypeError, pickle.PicklingError),
                pickle.dumps, d.viewitems(), proto)


def test_main():
    test_support.run_unittest(DictSetTest)

if __name__ == "__main__":
    test_main()
