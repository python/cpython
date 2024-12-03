from __future__ import unicode_literals
import unittest


class Tests(unittest.TestCase):
    def test_unicode_literals(self):
        self.assertIsInstance("literal", str)


if __name__ == "__main__":
    unittest.main()
