from __future__ import absolute_codepath
import os
import unittest

class Tests(unittest.TestCase):
    def test_absolute_codepath(self):
        def fn():
            pass
        filename = fn.__code__.co_filename
        self.assertEqual(filename, os.path.abspath(filename))


if __name__ == "__main__":
    unittest.main()
