import importlib
from importlib import machinery
from .. import abc
from .. import util
from . import util as builtin_util

import sys
import types
import unittest


class LoaderTests(abc.LoaderTests):

    """Test load_module() for built-in modules."""

    verification = {'__name__': 'errno', '__package__': '',
                    '__loader__': machinery.BuiltinImporter}

    def verify(self, module):
        """Verify that the module matches against what it should have."""
        self.assertTrue(isinstance(module, types.ModuleType))
        for attr, value in self.verification.items():
            self.assertEqual(getattr(module, attr), value)
        self.assertTrue(module.__name__ in sys.modules)

    load_module = staticmethod(lambda name:
                                machinery.BuiltinImporter.load_module(name))

    def test_module(self):
        # Common case.
        with util.uncache(builtin_util.NAME):
            module = self.load_module(builtin_util.NAME)
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
        with util.uncache(builtin_util.NAME):
            module1 = self.load_module(builtin_util.NAME)
            module2 = self.load_module(builtin_util.NAME)
            self.assertTrue(module1 is module2)

    def test_unloadable(self):
        name = 'dssdsdfff'
        assert name not in sys.builtin_module_names
        with self.assertRaises(ImportError):
            self.load_module(name)

    def test_already_imported(self):
        # Using the name of a module already imported but not a built-in should
        # still fail.
        assert hasattr(importlib, '__file__')  # Not a built-in.
        with self.assertRaises(ImportError):
            self.load_module('importlib')


class InspectLoaderTests(unittest.TestCase):

    """Tests for InspectLoader methods for BuiltinImporter."""

    def test_get_code(self):
        # There is no code object.
        result = machinery.BuiltinImporter.get_code(builtin_util.NAME)
        self.assertTrue(result is None)

    def test_get_source(self):
        # There is no source.
        result = machinery.BuiltinImporter.get_source(builtin_util.NAME)
        self.assertTrue(result is None)

    def test_is_package(self):
        # Cannot be a package.
        result = machinery.BuiltinImporter.is_package(builtin_util.NAME)
        self.assertTrue(not result)

    def test_not_builtin(self):
        # Modules not built-in should raise ImportError.
        for meth_name in ('get_code', 'get_source', 'is_package'):
            method = getattr(machinery.BuiltinImporter, meth_name)
        with self.assertRaises(ImportError):
            method(builtin_util.BAD_NAME)



def test_main():
    from test.support import run_unittest
    run_unittest(LoaderTests, InspectLoaderTests)


if __name__ == '__main__':
    test_main()
