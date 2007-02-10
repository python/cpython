import unittest
from test import test_support

class DictSetTest(unittest.TestCase):

    def test_dict_keys(self):
        d = {1: 10, "a": "ABC"}
        keys = d.KEYS()
        self.assertEqual(set(keys), {1, "a"})
        self.assertEqual(len(keys), 2)

    def test_dict_items(self):
        d = {1: 10, "a": "ABC"}
        items = d.ITEMS()
        self.assertEqual(set(items), {(1, 10), ("a", "ABC")})
        self.assertEqual(len(items), 2)

    def test_dict_values(self):
        d = {1: 10, "a": "ABC"}
        values = d.VALUES()
        self.assertEqual(set(values), {10, "ABC"})
        self.assertEqual(len(values), 2)

def test_main():
    test_support.run_unittest(DictSetTest)

if __name__ == "__main__":
    test_main()
