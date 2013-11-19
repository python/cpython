from importlib import machinery
from .. import abc
from .. import util
from . import util as builtin_util

import sys
import unittest

class FinderTests(abc.FinderTests):

    """Test find_module() for built-in modules."""

    def test_module(self):
        # Common case.
        with util.uncache(builtin_util.NAME):
            found = machinery.BuiltinImporter.find_module(builtin_util.NAME)
            self.assertTrue(found)

    # Built-in modules cannot be a package.
    test_package = test_package_in_package = test_package_over_module = None

    # Built-in modules cannot be in a package.
    test_module_in_package = None

    def test_failure(self):
        assert 'importlib' not in sys.builtin_module_names
        loader = machinery.BuiltinImporter.find_module('importlib')
        self.assertIsNone(loader)

    def test_ignore_path(self):
        # The value for 'path' should always trigger a failed import.
        with util.uncache(builtin_util.NAME):
            loader = machinery.BuiltinImporter.find_module(builtin_util.NAME,
                                                            ['pkg'])
            self.assertIsNone(loader)



def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)


if __name__ == '__main__':
    test_main()
