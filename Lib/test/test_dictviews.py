import unittest
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
        self.assert_(1 in keys)
        self.assert_("a" in keys)
        self.assert_(10 not in keys)
        self.assert_("Z" not in keys)
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
        self.assert_((1, 10) in items)
        self.assert_(("a", "ABC") in items)
        self.assert_((1, 11) not in items)
        self.assert_(1 not in items)
        self.assert_(() not in items)
        self.assert_((1,) not in items)
        self.assert_((1, 2, 3) not in items)
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
        self.assertTrue(isinstance(repr(d), str))
        r = repr(d.viewitems())
        self.assertTrue(isinstance(r, str))
        self.assertTrue(r == "dict_items([('a', 'ABC'), (1, 10)])" or
                        r == "dict_items([(1, 10), ('a', 'ABC')])")
        r = repr(d.viewkeys())
        self.assertTrue(isinstance(r, str))
        self.assertTrue(r == "dict_keys(['a', 1])" or
                        r == "dict_keys([1, 'a'])")
        r = repr(d.viewvalues())
        self.assertTrue(isinstance(r, str))
        self.assertTrue(r == "dict_values(['ABC', 10])" or
                        r == "dict_values([10, 'ABC'])")


def test_main():
    test_support.run_unittest(DictSetTest)

if __name__ == "__main__":
    test_main()
