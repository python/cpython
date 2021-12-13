# Run tests for functions in Python/fileutils.c.

import os
import os.path
import unittest
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testinternalcapi')


class PathTests(unittest.TestCase):

    def test_capi_normalize_path(self):
        if os.name == 'nt':
            raise unittest.SkipTest('Windows has its own helper for this')
        else:
            from .test_posixpath import PosixPathTest as posixdata
            tests = posixdata.NORMPATH_CASES
        for filename, expected in tests:
            if not os.path.isabs(filename):
                continue
            with self.subTest(filename):
                result = _testcapi.normalize_path(filename)
                self.assertEqual(result, expected,
                    msg=f'input: {filename!r} expected output: {expected!r}')


if __name__ == "__main__":
    unittest.main()
