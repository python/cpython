from importlib import _bootstrap
from .. import abc
from . import util

import imp
import unittest

class FinderTests(abc.FinderTests):

    """Test the finder for extension modules."""

    def find_module(self, fullname):
        importer = _bootstrap.FileFinder(util.PATH,
                                          (_bootstrap.ExtensionFileLoader,
                                              imp.extension_suffixes(),
                                              False))
        return importer.find_module(fullname)

    def test_module(self):
        self.assertTrue(self.find_module(util.NAME))

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
        self.assertIsNone(self.find_module('asdfjkl;'))

    # XXX Raise an exception if someone tries to use the 'path' argument?


def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)


if __name__ == '__main__':
    test_main()
