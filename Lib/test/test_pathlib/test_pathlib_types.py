import ntpath
import posixpath
import unittest

from pathlib._types import Parser


class ParserTest(unittest.TestCase):
    def test_path_modules(self):
        self.assertTrue(isinstance(posixpath, Parser))
        self.assertTrue(isinstance(ntpath, Parser))


if __name__ == "__main__":
    unittest.main()
