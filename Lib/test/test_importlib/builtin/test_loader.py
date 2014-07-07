from .. import abc
from .. import util
from . import util as builtin_util

frozen_machinery, source_machinery = util.import_importlib('importlib.machinery')

import sys
import types
import unittest


class LoaderTests(abc.LoaderTests):

    """Test load_module() for built-in modules."""

    def setUp(self):
        self.verification = {'__name__': 'errno', '__package__': '',
                             '__loader__': self.machinery.BuiltinImporter}

    def verify(self, module):
        """Verify that the module matches against what it should have."""
        self.assertIsInstance(module, types.ModuleType)
        for attr, value in self.verification.items():
            self.assertEqual(getattr(module, attr), value)
        self.assertIn(module.__name__, sys.modules)

    def load_module(self, name):
        return self.machinery.BuiltinImporter.load_module(name)

    def test_module(self):
        # Common case.
        with util.uncache(builtin_util.NAME):
            module = self.load_module(builtin_util.NAME)
            self.verify(module)

    # Built-in modules cannot be a package.
    test_package = test_lacking_parent = None

    # No way to force an import failure.
    test_state_after_failure = None

    def test_module_reuse(self):
        # Test that the same module is used in a reload.
        with util.uncache(builtin_util.NAME):
            module1 = self.load_module(builtin_util.NAME)
            module2 = self.load_module(builtin_util.NAME)
            self.assertIs(module1, module2)

    def test_unloadable(self):
        name = 'dssdsdfff'
        assert name not in sys.builtin_module_names
        with self.assertRaises(ImportError) as cm:
            self.load_module(name)
        self.assertEqual(cm.exception.name, name)

    def test_already_imported(self):
        # Using the name of a module already imported but not a built-in should
        # still fail.
        module_name = 'builtin_reload_test'
        assert module_name not in sys.builtin_module_names
        with util.uncache(module_name):
            module = types.ModuleType(module_name)
            sys.modules[module_name] = module
        with self.assertRaises(ImportError) as cm:
            self.load_module(module_name)
        self.assertEqual(cm.exception.name, module_name)


Frozen_LoaderTests, Source_LoaderTests = util.test_both(LoaderTests,
        machinery=[frozen_machinery, source_machinery])


class InspectLoaderTests:

    """Tests for InspectLoader methods for BuiltinImporter."""

    def test_get_code(self):
        # There is no code object.
        result = self.machinery.BuiltinImporter.get_code(builtin_util.NAME)
        self.assertIsNone(result)

    def test_get_source(self):
        # There is no source.
        result = self.machinery.BuiltinImporter.get_source(builtin_util.NAME)
        self.assertIsNone(result)

    def test_is_package(self):
        # Cannot be a package.
        result = self.machinery.BuiltinImporter.is_package(builtin_util.NAME)
        self.assertFalse(result)

    def test_not_builtin(self):
        # Modules not built-in should raise ImportError.
        for meth_name in ('get_code', 'get_source', 'is_package'):
            method = getattr(self.machinery.BuiltinImporter, meth_name)
        with self.assertRaises(ImportError) as cm:
            method(builtin_util.BAD_NAME)
        self.assertRaises(builtin_util.BAD_NAME)

Frozen_InspectLoaderTests, Source_InspectLoaderTests = util.test_both(
        InspectLoaderTests,
        machinery=[frozen_machinery, source_machinery])


if __name__ == '__main__':
    unittest.main()
