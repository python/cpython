import importlib
from . import test_path_hook
from .. import abc
from .. import util

import sys
import unittest


class LoaderTests(abc.LoaderTests):

    """Test load_module() for extension modules."""

    def load_module(self, fullname):
        loader = importlib._ExtensionFileLoader(test_path_hook.NAME,
                                                test_path_hook.FILEPATH,
                                                False)
        return loader.load_module(fullname)

    def test_module(self):
        with util.uncache(test_path_hook.NAME):
            module = self.load_module(test_path_hook.NAME)
            for attr, value in [('__name__', test_path_hook.NAME),
                                ('__file__', test_path_hook.FILEPATH)]:
                self.assertEqual(getattr(module, attr), value)
            self.assert_(test_path_hook.NAME in sys.modules)

    def test_package(self):
        # Extensions are not found in packages.
        pass

    def test_lacking_parent(self):
        # Extensions are not found in packages.
        pass

    def test_module_reuse(self):
        with util.uncache(test_path_hook.NAME):
            module1 = self.load_module(test_path_hook.NAME)
            module2 = self.load_module(test_path_hook.NAME)
            self.assert_(module1 is module2)

    def test_state_after_failure(self):
        # No easy way to trigger a failure after a successful import.
        pass

    def test_unloadable(self):
        self.assertRaises(ImportError, self.load_module, 'asdfjkl;')


def test_main():
    from test.support import run_unittest
    run_unittest(LoaderTests)


if __name__ == '__main__':
    test_main()
