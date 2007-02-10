import unittest
from test import test_support

class DictSetTest(unittest.TestCase):

    def test_constructors_not_callable(self):
        kt = type({}.KEYS())
        self.assertRaises(TypeError, kt, {})
        self.assertRaises(TypeError, kt)
        it = type({}.ITEMS())
        self.assertRaises(TypeError, it, {})
        self.assertRaises(TypeError, it)
        vt = type({}.VALUES())
        self.assertRaises(TypeError, vt, {})
        self.assertRaises(TypeError, vt)

    def test_dict_keys(self):
        d = {1: 10, "a": "ABC"}
        keys = d.KEYS()
        self.assertEqual(set(keys), {1, "a"})
        self.assertEqual(len(keys), 2)
        self.assert_(1 in keys)
        self.assert_("a" in keys)
        self.assert_(10 not in keys)
        self.assert_("Z" not in keys)

    def test_dict_items(self):
        d = {1: 10, "a": "ABC"}
        items = d.ITEMS()
        self.assertEqual(set(items), {(1, 10), ("a", "ABC")})
        self.assertEqual(len(items), 2)
        self.assert_((1, 10) in items)
        self.assert_(("a", "ABC") in items)
        self.assert_((1, 11) not in items)
        self.assert_(1 not in items)
        self.assert_(() not in items)
        self.assert_((1,) not in items)
        self.assert_((1, 2, 3) not in items)

    def test_dict_values(self):
        d = {1: 10, "a": "ABC"}
        values = d.VALUES()
        self.assertEqual(set(values), {10, "ABC"})
        self.assertEqual(len(values), 2)

def test_main():
    test_support.run_unittest(DictSetTest)

if __name__ == "__main__":
    test_main()
