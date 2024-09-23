from __future__ import nested_scopes
from __future__ import division

import unittest

x = 2
def nester():
    x = 3
    def inner():
        return x
    return inner()


class TestFuture(unittest.TestCase):

    def test_floor_div_operator(self):
        self.assertEqual(7 // 2, 3)

    def test_true_div_as_default(self):
        self.assertAlmostEqual(7 / 2, 3.5)

    def test_nested_scopes(self):
        self.assertEqual(nester(), 3)

if __name__ == "__main__":
    unittest.main()
