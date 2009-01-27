import importlib
from .. import finder_tests
from . import test_path_hook

import unittest

class FinderTests(finder_tests.FinderTests):

    """Test the finder for extension modules."""

    def find_module(self, fullname):
        importer = importlib.ExtensionFileImporter(test_path_hook.PATH)
        return importer.find_module(fullname)

    def test_module(self):
        self.assert_(self.find_module(test_path_hook.NAME))

    def test_package(self):
        # Extension modules cannot be an __init__ for a package.
        pass

    def test_module_in_package(self):
        # No extension module in a package available for testing.
        pass

    def test_package_in_package(self):
        # Extension modules cannot be an __init__ for a package.
        pass

    def test_package_over_module(self):
        # Extension modules cannot be an __init__ for a package.
        pass

    def test_failure(self):
        self.assert_(self.find_module('asdfjkl;') is None)

    # XXX Raise an exception if someone tries to use the 'path' argument?


def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)


if __name__ == '__main__':
    test_main()
