import importlib
from ..builtin import test_finder
from .. import support

import unittest


class FinderTests(test_finder.FinderTests):

    """Test finding frozen modules."""

    def find(self, name, path=None):
        finder = importlib.FrozenImporter()
        return finder.find_module(name, path)


    def test_module(self):
        name = '__hello__'
        loader = self.find(name)
        self.assert_(hasattr(loader, 'load_module'))

    def test_package(self):
        loader = self.find('__phello__')
        self.assert_(hasattr(loader, 'load_module'))

    def test_module_in_package(self):
        loader = self.find('__phello__.spam', ['__phello__'])
        self.assert_(hasattr(loader, 'load_module'))

    def test_package_in_package(self):
        pass

    def test_failure(self):
        loader = self.find('<not real>')
        self.assert_(loader is None)


def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)


if __name__ == '__main__':
    test_main()
