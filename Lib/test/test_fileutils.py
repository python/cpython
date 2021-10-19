# Run tests for functions in Python/fileutils.c.

import os
import os.path
import unittest
from test.support import import_helper
from .test_posixpath import PosixPathTest as posixdata

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testinternalcapi')


class PathTests(unittest.TestCase):

    if os.name == 'nt':
        raise Unittest.SkipTest()
    else:
        NORMPATH_CASES = posixdata.NORMPATH_CASES

    def test_normalize_path(self):
        for filename, expected in self.NORMPATH_CASES:
            if not os.path.isabs(filename):
                continue
            with self.subTest(filename):
                result = _testcapi.test_normalize_path(filename)
                self.assertEqual(result, expected)

del posixdata


if __name__ == "__main__":
    unittest.main()
