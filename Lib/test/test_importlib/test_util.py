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
            self.assertIn(module_name, sys.modules)
        self.assertIsInstance(module, types.ModuleType)
        self.assertEqual(module.__name__, module_name)

    def test_reload(self):
        # Test that a module is reused if already in sys.modules.
        name = 'a.b.c'
        module = imp.new_module('a.b.c')
        with test_util.uncache(name):
            sys.modules[name] = module
            returned_module = self.return_module(name)
            self.assertIs(returned_module, sys.modules[name])

    def test_new_module_failure(self):
        # Test that a module is removed from sys.modules if added but an
        # exception is raised.
        name = 'a.b.c'
        with test_util.uncache(name):
            self.raise_exception(name)
            self.assertNotIn(name, sys.modules)

    def test_reload_failure(self):
        # Test that a failure on reload leaves the module in-place.
        name = 'a.b.c'
        module = imp.new_module(name)
        with test_util.uncache(name):
            sys.modules[name] = module
            self.raise_exception(name)
            self.assertIs(module, sys.modules[name])

    def test_decorator_attrs(self):
        def fxn(self, module): pass
        wrapped = util.module_for_loader(fxn)
        self.assertEqual(wrapped.__name__, fxn.__name__)
        self.assertEqual(wrapped.__qualname__, fxn.__qualname__)

    def test_false_module(self):
        # If for some odd reason a module is considered false, still return it
        # from sys.modules.
        class FalseModule(types.ModuleType):
            def __bool__(self): return False

        name = 'mod'
        module = FalseModule(name)
        with test_util.uncache(name):
            self.assertFalse(module)
            sys.modules[name] = module
            given = self.return_module(name)
            self.assertIs(given, module)

    def test_attributes_set(self):
        # __name__, __loader__, and __package__ should be set (when
        # is_package() is defined; undefined implicitly tested elsewhere).
        class FakeLoader:
            def __init__(self, is_package):
                self._pkg = is_package
            def is_package(self, name):
                return self._pkg
            @util.module_for_loader
            def load_module(self, module):
                return module

        name = 'pkg.mod'
        with test_util.uncache(name):
            loader = FakeLoader(False)
            module = loader.load_module(name)
            self.assertEqual(module.__name__, name)
            self.assertIs(module.__loader__, loader)
            self.assertEqual(module.__package__, 'pkg')

        name = 'pkg.sub'
        with test_util.uncache(name):
            loader = FakeLoader(True)
            module = loader.load_module(name)
            self.assertEqual(module.__name__, name)
            self.assertIs(module.__loader__, loader)
            self.assertEqual(module.__package__, name)


class SetPackageTests(unittest.TestCase):

    """Tests for importlib.util.set_package."""

    def verify(self, module, expect):
        """Verify the module has the expected value for __package__ after
        passing through set_package."""
        fxn = lambda: module
        wrapped = util.set_package(fxn)
        wrapped()
        self.assertTrue(hasattr(module, '__package__'))
        self.assertEqual(expect, module.__package__)

    def test_top_level(self):
        # __package__ should be set to the empty string if a top-level module.
        # Implicitly tests when package is set to None.
        module = imp.new_module('module')
        module.__package__ = None
        self.verify(module, '')

    def test_package(self):
        # Test setting __package__ for a package.
        module = imp.new_module('pkg')
        module.__path__ = ['<path>']
        module.__package__ = None
        self.verify(module, 'pkg')

    def test_submodule(self):
        # Test __package__ for a module in a package.
        module = imp.new_module('pkg.mod')
        module.__package__ = None
        self.verify(module, 'pkg')

    def test_setting_if_missing(self):
        # __package__ should be set if it is missing.
        module = imp.new_module('mod')
        if hasattr(module, '__package__'):
            delattr(module, '__package__')
        self.verify(module, '')

    def test_leaving_alone(self):
        # If __package__ is set and not None then leave it alone.
        for value in (True, False):
            module = imp.new_module('mod')
            module.__package__ = value
            self.verify(module, value)

    def test_decorator_attrs(self):
        def fxn(module): pass
        wrapped = util.set_package(fxn)
        self.assertEqual(wrapped.__name__, fxn.__name__)
        self.assertEqual(wrapped.__qualname__, fxn.__qualname__)


class ResolveNameTests(unittest.TestCase):

    """Tests importlib.util.resolve_name()."""

    def test_absolute(self):
        # bacon
        self.assertEqual('bacon', util.resolve_name('bacon', None))

    def test_aboslute_within_package(self):
        # bacon in spam
        self.assertEqual('bacon', util.resolve_name('bacon', 'spam'))

    def test_no_package(self):
        # .bacon in ''
        with self.assertRaises(ValueError):
            util.resolve_name('.bacon', '')

    def test_in_package(self):
        # .bacon in spam
        self.assertEqual('spam.eggs.bacon',
                         util.resolve_name('.bacon', 'spam.eggs'))

    def test_other_package(self):
        # ..bacon in spam.bacon
        self.assertEqual('spam.bacon',
                         util.resolve_name('..bacon', 'spam.eggs'))

    def test_escape(self):
        # ..bacon in spam
        with self.assertRaises(ValueError):
            util.resolve_name('..bacon', 'spam')


def test_main():
    from test import support
    support.run_unittest(
            ModuleForLoaderTests,
            SetPackageTests,
            ResolveNameTests
        )


if __name__ == '__main__':
    test_main()
