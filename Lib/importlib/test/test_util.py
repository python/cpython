from importlib import util
from . import util as test_util
import imp
import sys
import types
import unittest


class ModuleForLoaderTests(unittest.TestCase):

    """Tests for importlib.util.module_for_loader."""

    def return_module(self, name):
        fxn = util.module_for_loader(lambda self, module: module)
        return fxn(self, name)

    def raise_exception(self, name):
        def to_wrap(self, module):
            raise ImportError
        fxn = util.module_for_loader(to_wrap)
        try:
            fxn(self, name)
        except ImportError:
            pass

    def test_new_module(self):
        # Test that when no module exists in sys.modules a new module is
        # created.
        module_name = 'a.b.c'
        with test_util.uncache(module_name):
            module = self.return_module(module_name)
            self.assert_(module_name in sys.modules)
        self.assert_(isinstance(module, types.ModuleType))
        self.assertEqual(module.__name__, module_name)

    def test_reload(self):
        # Test that a module is reused if already in sys.modules.
        name = 'a.b.c'
        module = imp.new_module('a.b.c')
        with test_util.uncache(name):
            sys.modules[name] = module
            returned_module = self.return_module(name)
            self.assert_(sys.modules[name] is returned_module)

    def test_new_module_failure(self):
        # Test that a module is removed from sys.modules if added but an
        # exception is raised.
        name = 'a.b.c'
        with test_util.uncache(name):
            self.raise_exception(name)
            self.assert_(name not in sys.modules)

    def test_reload_failure(self):
        # Test that a failure on reload leaves the module in-place.
        name = 'a.b.c'
        module = imp.new_module(name)
        with test_util.uncache(name):
            sys.modules[name] = module
            self.raise_exception(name)
            self.assert_(sys.modules[name] is module)


def test_main():
    from test import support
    support.run_unittest(ModuleForLoaderTests)


if __name__ == '__main__':
    test_main()
