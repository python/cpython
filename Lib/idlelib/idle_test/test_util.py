"""Test util, coverage 100%"""

import unittest
from idlelib import util


class UtilTest(unittest.TestCase):
    supported_extensions = {'.pyi', '.py', '.pyw'}

    def test_extensions(self):
        for extension in self.supported_extensions:
            self.assertIn(extension, util.py_extensions)

    def test_is_supported_extension(self):
        path = '/some/path/myfile'
        for ext in self.supported_extensions:
            with self.subTest(ext=ext):
                filename = f'{path}{ext}'
                self.assertTrue(util.is_supported_extension(filename))

    def test_is_not_supported_extension(self):
        path = '/some/path/myfile'
        for ext in '.txt', '.p', '.pyww', '.pyii', '.pyy':
            with self.subTest(ext=ext):
                filename = f"{path}{ext}"
                self.assertFalse(util.is_supported_extension(filename))


if __name__ == '__main__':
    unittest.main(verbosity=2)
