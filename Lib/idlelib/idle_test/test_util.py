"""Test util, coverage 100%"""

import unittest
from idlelib import util


class UtilTest(unittest.TestCase):
    path = '/some/path/myfile'

    def test_is_supported_extension(self):
        for ext in util.py_extensions:
            with self.subTest(ext=ext):
                filename = f'{self.path}{ext}'
                self.assertTrue(util.is_supported_extension(filename))

    def test_is_not_supported_extension(self):
        for ext in '.txt', '.p', '.pyww', '.pyii', '.pyy', '':
            with self.subTest(ext=ext):
                filename = f"{self.path}{ext}"
                self.assertFalse(util.is_supported_extension(filename))

    def test_is_browseable_extension(self):
        for ext in util.py_extensions:
            with self.subTest(ext=ext):
                filename = f'{self.path}{ext}'
                actual = util.is_browseable_extension(filename)
                expected = ext not in util.browseable_extension_blocklist
                self.assertEqual(actual, expected)


if __name__ == '__main__':
    unittest.main(verbosity=2)
