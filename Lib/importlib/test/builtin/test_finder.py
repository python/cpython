from importlib import machinery
from .. import abc
from .. import util

import sys
import unittest

class FinderTests(abc.FinderTests):

    """Test find_module() for built-in modules."""

    assert 'errno' in sys.builtin_module_names
    name = 'errno'

    def test_module(self):
        # Common case.
        with util.uncache(self.name):
            self.assert_(machinery.BuiltinImporter.find_module(self.name))

    def test_package(self):
        # Built-in modules cannot be a package.
        pass

    def test_module_in_package(self):
        # Built-in modules cannobt be in a package.
        pass

    def test_package_in_package(self):
        # Built-in modules cannot be a package.
        pass

    def test_package_over_module(self):
        # Built-in modules cannot be a package.
        pass

    def test_failure(self):
        assert 'importlib' not in sys.builtin_module_names
        loader = machinery.BuiltinImporter.find_module('importlib')
        self.assert_(loader is None)

    def test_ignore_path(self):
        # The value for 'path' should always trigger a failed import.
        with util.uncache(self.name):
            loader = machinery.BuiltinImporter.find_module(self.name, ['pkg'])
            self.assert_(loader is None)



def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)


if __name__ == '__main__':
    test_main()
