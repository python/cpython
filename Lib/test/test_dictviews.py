import unittest
from test import test_support

class DictSetTest(unittest.TestCase):

    def test_constructors_not_callable(self):
        kt = type({}.keys())
        self.assertRaises(TypeError, kt, {})
        self.assertRaises(TypeError, kt)
        it = type({}.items())
        self.assertRaises(TypeError, it, {})
        self.assertRaises(TypeError, it)
        vt = type({}.values())
        self.assertRaises(TypeError, vt, {})
        self.assertRaises(TypeError, vt)

    def test_dict_keys(self):
        d = {1: 10, "a": "ABC"}
        keys = d.keys()
        self.assertEqual(len(keys), 2)
        self.assertEqual(set(keys), {1, "a"})
        self.assertEqual(keys, {1, "a"})
        self.assertNotEqual(keys, {1, "a", "b"})
        self.assertNotEqual(keys, {1, "b"})
        self.assertNotEqual(keys, {1})
        self.assertNotEqual(keys, 42)
        self.assert_(1 in keys)
        self.assert_("a" in keys)
        self.assert_(10 not in keys)
        self.assert_("Z" not in keys)
        self.assertEqual(d.keys(), d.keys())
        e = {1: 11, "a": "def"}
        self.assertEqual(d.keys(), e.keys())
        del e["a"]
        self.assertNotEqual(d.keys(), e.keys())

    def test_dict_items(self):
        d = {1: 10, "a": "ABC"}
        items = d.items()
        self.assertEqual(len(items), 2)
        self.assertEqual(set(items), {(1, 10), ("a", "ABC")})
        self.assertEqual(items, {(1, 10), ("a", "ABC")})
        self.assertNotEqual(items, {(1, 10), ("a", "ABC"), "junk"})
        self.assertNotEqual(items, {(1, 10), ("a", "def")})
        self.assertNotEqual(items, {(1, 10)})
        self.assertNotEqual(items, 42)
        self.assert_((1, 10) in items)
        self.assert_(("a", "ABC") in items)
        self.assert_((1, 11) not in items)
        self.assert_(1 not in items)
        self.assert_(() not in items)
        self.assert_((1,) not in items)
        self.assert_((1, 2, 3) not in items)
        self.assertEqual(d.items(), d.items())
        e = d.copy()
        self.assertEqual(d.items(), e.items())
        e["a"] = "def"
        self.assertNotEqual(d.items(), e.items())

    def test_dict_mixed_keys_items(self):
        d = {(1, 1): 11, (2, 2): 22}
        e = {1: 1, 2: 2}
        self.assertEqual(d.keys(), e.items())
        self.assertNotEqual(d.items(), e.keys())

    def test_dict_values(self):
        d = {1: 10, "a": "ABC"}
        values = d.values()
        self.assertEqual(set(values), {10, "ABC"})
        self.assertEqual(len(values), 2)

def test_main():
    test_support.run_unittest(DictSetTest)

if __name__ == "__main__":
    test_main()
