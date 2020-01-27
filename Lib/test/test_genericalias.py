"""Tests for C-implemented GenericAlias."""

import unittest


class BaseTest(unittest.TestCase):
    """Test basics."""

    def test_subscriptable(self):
        for t in tuple, list, dict, set, frozenset:
            tname = t.__name__
            with self.subTest(f"Testing {tname}"):
                t[int]
                ## self.assertEqual(t[int], t)

    def test_unsubscriptable(self):
        for t in int, str, float:
            tname = t.__name__
            with self.subTest(f"Testing {tname}"):
                with self.assertRaises(TypeError):
                    t[int]


if __name__ == "__main__":
    unittest.main()
