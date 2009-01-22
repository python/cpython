from importlib import machinery
from .. import support

import sys
import unittest

class FinderTests(unittest.TestCase):

    """Test find_module() for built-in modules."""

    assert 'errno' in sys.builtin_module_names
    name = 'errno'

    find_module = staticmethod(lambda name, path=None:
                    machinery.BuiltinImporter.find_module(name, path))


    def test_find_module(self):
        # Common case.
        with support.uncache(self.name):
            self.assert_(self.find_module(self.name))

    def test_ignore_path(self):
        # The value for 'path' should always trigger a failed import.
        with support.uncache(self.name):
            self.assert_(self.find_module(self.name, ['pkg']) is None)



def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)


if __name__ == '__main__':
    test_main()
