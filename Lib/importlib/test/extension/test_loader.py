import importlib
from . import test_path_hook
from .. import support

import sys
import unittest


class LoaderTests(unittest.TestCase):

    """Test load_module() for extension modules."""

    def load_module(self, fullname):
        loader = importlib._ExtensionFileLoader(test_path_hook.NAME,
                                                test_path_hook.FILEPATH,
                                                False)
        return loader.load_module(fullname)

    def test_success(self):
        with support.uncache(test_path_hook.NAME):
            module = self.load_module(test_path_hook.NAME)
            for attr, value in [('__name__', test_path_hook.NAME),
                                ('__file__', test_path_hook.FILEPATH)]:
                self.assertEqual(getattr(module, attr), value)
            self.assert_(test_path_hook.NAME in sys.modules)

    def test_failure(self):
        self.assertRaises(ImportError, self.load_module, 'asdfjkl;')


def test_main():
    from test.support import run_unittest
    run_unittest(LoaderTests)


if __name__ == '__main__':
    test_main()
