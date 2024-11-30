import ntpath
import os
import posixpath
import unittest

from pathlib._abc import PathBase
from pathlib._types import Parser, DirEntry, StatResult

from test.support.os_helper import TESTFN


class ParserTest(unittest.TestCase):
    def test_path_modules(self):
        self.assertTrue(isinstance(posixpath, Parser))
        self.assertTrue(isinstance(ntpath, Parser))


class DirEntryTest(unittest.TestCase):
    def test_os_direntry(self):
        os.mkdir(TESTFN)
        try:
            with os.scandir() as entries:
                entry = next(entries)
                self.assertTrue(isinstance(entry, DirEntry))
        finally:
            os.rmdir(TESTFN)

    def test_pathbase(self):
        self.assertTrue(isinstance(PathBase(), DirEntry))


class StatResultTest(unittest.TestCase):
    def test_os_stat_result(self):
        self.assertTrue(isinstance(os.stat('.'), StatResult))


if __name__ == "__main__":
    unittest.main()
