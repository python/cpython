import sys
import unittest

class Test(unittest.TestCase):
    def test(self):
        self.assertNotIn('test_regrtest_a', sys.modules)
        self.assertIn('test_regrtest_b', sys.modules)
        self.assertNotIn('test_regrtest_b.util', sys.modules)
        self.assertNotIn('test_regrtest_c', sys.modules)
