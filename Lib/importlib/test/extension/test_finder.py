import importlib
from . import test_path_hook

import unittest

class FinderTests(unittest.TestCase):

    """Test the finder for extension modules."""

    def find_module(self, fullname):
        importer = importlib.ExtensionFileImporter(test_path_hook.PATH)
        return importer.find_module(fullname)

    def test_success(self):
        self.assert_(self.find_module(test_path_hook.NAME))

    def test_failure(self):
        self.assert_(self.find_module('asdfjkl;') is None)

    # XXX Raise an exception if someone tries to use the 'path' argument?


def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)


if __name__ == '__main__':
    test_main()
