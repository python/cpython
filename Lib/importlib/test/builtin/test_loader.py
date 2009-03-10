import importlib
from importlib import machinery
from .. import abc
from .. import util

import sys
import types
import unittest


class LoaderTests(abc.LoaderTests):

    """Test load_module() for built-in modules."""

    assert 'errno' in sys.builtin_module_names
    name = 'errno'

    verification = {'__name__': 'errno', '__package__': '',
                    '__loader__': machinery.BuiltinImporter}

    def verify(self, module):
        """Verify that the module matches against what it should have."""
        self.assert_(isinstance(module, types.ModuleType))
        for attr, value in self.verification.items():
            self.assertEqual(getattr(module, attr), value)
        self.assert_(module.__name__ in sys.modules)

    load_module = staticmethod(lambda name:
                                machinery.BuiltinImporter.load_module(name))

    def test_module(self):
        # Common case.
        with util.uncache(self.name):
            module = self.load_module(self.name)
            self.verify(module)

    def test_package(self):
        # Built-in modules cannot be a package.
        pass

    def test_lacking_parent(self):
        # Built-in modules cannot be a package.
        pass

    def test_state_after_failure(self):
        # Not way to force an imoprt failure.
        pass

    def test_module_reuse(self):
        # Test that the same module is used in a reload.
        with util.uncache(self.name):
            module1 = self.load_module(self.name)
            module2 = self.load_module(self.name)
            self.assert_(module1 is module2)

    def test_unloadable(self):
        name = 'dssdsdfff'
        assert name not in sys.builtin_module_names
        self.assertRaises(ImportError, self.load_module, name)

    def test_already_imported(self):
        # Using the name of a module already imported but not a built-in should
        # still fail.
        assert hasattr(importlib, '__file__')  # Not a built-in.
        self.assertRaises(ImportError, self.load_module, 'importlib')


def test_main():
    from test.support import run_unittest
    run_unittest(LoaderTests)


if __name__ == '__main__':
    test_main()
