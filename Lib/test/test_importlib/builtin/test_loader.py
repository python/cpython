from test.test_importlib import abc, util

machinery = util.import_importlib('importlib.machinery')

import os
import sys
import types
import unittest
import warnings


@unittest.skipIf(util.BUILTINS.good_name is None, 'no reasonable builtin module')
class InspectLoaderTests:

    """Tests for InspectLoader methods for BuiltinImporter."""

    def test_get_code(self):
        # There is no code object.
        result = self.machinery.BuiltinImporter.get_code(util.BUILTINS.good_name)
        self.assertIsNone(result)

    def test_get_source(self):
        # There is no source.
        result = self.machinery.BuiltinImporter.get_source(util.BUILTINS.good_name)
        self.assertIsNone(result)

    def test_is_package(self):
        # Cannot be a package.
        result = self.machinery.BuiltinImporter.is_package(util.BUILTINS.good_name)
        self.assertFalse(result)

    @unittest.skipIf(util.BUILTINS.bad_name is None, 'all modules are built in')
    def test_not_builtin(self):
        # Modules not built-in should raise ImportError.
        for meth_name in ('get_code', 'get_source', 'is_package'):
            method = getattr(self.machinery.BuiltinImporter, meth_name)
        with self.assertRaises(ImportError) as cm:
            method(util.BUILTINS.bad_name)


(Frozen_InspectLoaderTests,
 Source_InspectLoaderTests
 ) = util.test_both(InspectLoaderTests, machinery=machinery)


class OsModuleTests:

    """Test loading _os and its historical name by hand."""

    def test_load_by_hand(self):
        # As LazyLoader does: a working module, sys.modules left alone.
        loader = self.machinery.BuiltinImporter
        for name in ('_os', os.name):
            with self.subTest(name):
                spec = loader.find_spec(name)
                mod = loader.create_module(spec)
                loader.exec_module(mod)
                self.assertEqual(mod.__name__, 'os')
                self.assertEqual(mod.getpid(), os.getpid())
                self.assertIsNot(mod, sys.modules[name])
                self.assertIs(sys.modules[os.name], sys.modules['_os'])
                self.assertIs(sys.modules[os.name].stat, os.stat)


(Frozen_OsModuleTests,
 Source_OsModuleTests
 ) = util.test_both(OsModuleTests, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
