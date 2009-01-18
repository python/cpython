import importlib

import collections
import imp
from os import path
import sys
import unittest


PATH = None
EXT = None
FILENAME = None
NAME = '_testcapi'
_file_exts = [x[0] for x in imp.get_suffixes() if x[2] == imp.C_EXTENSION]
try:
    for PATH in sys.path:
        for EXT in _file_exts:
            FILENAME = NAME + EXT
            FILEPATH = path.join(PATH, FILENAME)
            if path.exists(path.join(PATH, FILENAME)):
                raise StopIteration
    else:
        PATH = EXT = FILENAME = FILEPATH = None
except StopIteration:
    pass
del _file_exts


class PathHookTests(unittest.TestCase):

    """Test the path hook for extension modules."""
    # XXX Should it only succeed for pre-existing directories?
    # XXX Should it only work for directories containing an extension module?

    def hook(self, entry):
        return importlib.ExtensionFileImporter(entry)

    def test_success(self):
        # Path hook should handle a directory where a known extension module
        # exists.
        self.assert_(hasattr(self.hook(PATH), 'find_module'))


def test_main():
    from test.support import run_unittest
    run_unittest(PathHookTests)


if __name__ == '__main__':
    test_main()
