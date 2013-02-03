from . import util
import imp
import importlib
from importlib import _bootstrap
from importlib import machinery
import sys
from test import support
import types
import unittest


class ImportModuleTests(unittest.TestCase):

    """Test importlib.import_module."""

    def test_module_import(self):
        # Test importing a top-level module.
        with util.mock_modules('top_level') as mock:
            with util.import_state(meta_path=[mock]):
                module = importlib.import_module('top_level')
                self.assertEqual(module.__name__, 'top_level')

    def test_absolute_package_import(self):
        # Test importing a module from a package with an absolute name.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with util.mock_modules(pkg_long_name, name) as mock:
            with util.import_state(meta_path=[mock]):
                module = importlib.import_module(name)
                self.assertEqual(module.__name__, name)

    def test_shallow_relative_package_import(self):
        # Test importing a module from a package through a relative import.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        module_name = 'mod'
        absolute_name = '{0}.{1}'.format(pkg_name, module_name)
        relative_name = '.{0}'.format(module_name)
        with util.mock_modules(pkg_long_name, absolute_name) as mock:
            with util.import_state(meta_path=[mock]):
                importlib.import_module(pkg_name)
                module = importlib.import_module(relative_name, pkg_name)
                self.assertEqual(module.__name__, absolute_name)

    def test_deep_relative_package_import(self):
        modules = ['a.__init__', 'a.b.__init__', 'a.c']
        with util.mock_modules(*modules) as mock:
            with util.import_state(meta_path=[mock]):
                importlib.import_module('a')
                importlib.import_module('a.b')
                module = importlib.import_module('..c', 'a.b')
                self.assertEqual(module.__name__, 'a.c')

    def test_absolute_import_with_package(self):
        # Test importing a module from a package with an absolute name with
        # the 'package' argument given.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with util.mock_modules(pkg_long_name, name) as mock:
            with util.import_state(meta_path=[mock]):
                importlib.import_module(pkg_name)
                module = importlib.import_module(name, pkg_name)
                self.assertEqual(module.__name__, name)

    def test_relative_import_wo_package(self):
        # Relative imports cannot happen without the 'package' argument being
        # set.
        with self.assertRaises(TypeError):
            importlib.import_module('.support')


    def test_loaded_once(self):
        # Issue #13591: Modules should only be loaded once when
        # initializing the parent package attempts to import the
        # module currently being imported.
        b_load_count = 0
        def load_a():
            importlib.import_module('a.b')
        def load_b():
            nonlocal b_load_count
            b_load_count += 1
        code = {'a': load_a, 'a.b': load_b}
        modules = ['a.__init__', 'a.b']
        with util.mock_modules(*modules, module_code=code) as mock:
            with util.import_state(meta_path=[mock]):
                importlib.import_module('a.b')
        self.assertEqual(b_load_count, 1)


class FindLoaderTests(unittest.TestCase):

    class FakeMetaFinder:
        @staticmethod
        def find_module(name, path=None): return name, path

    def test_sys_modules(self):
        # If a module with __loader__ is in sys.modules, then return it.
        name = 'some_mod'
        with util.uncache(name):
            module = imp.new_module(name)
            loader = 'a loader!'
            module.__loader__ = loader
            sys.modules[name] = module
            found = importlib.find_loader(name)
            self.assertEqual(loader, found)

    def test_sys_modules_loader_is_None(self):
        # If sys.modules[name].__loader__ is None, raise ValueError.
        name = 'some_mod'
        with util.uncache(name):
            module = imp.new_module(name)
            module.__loader__ = None
            sys.modules[name] = module
            with self.assertRaises(ValueError):
                importlib.find_loader(name)

    def test_success(self):
        # Return the loader found on sys.meta_path.
        name = 'some_mod'
        with util.uncache(name):
            with util.import_state(meta_path=[self.FakeMetaFinder]):
                self.assertEqual((name, None), importlib.find_loader(name))

    def test_success_path(self):
        # Searching on a path should work.
        name = 'some_mod'
        path = 'path to some place'
        with util.uncache(name):
            with util.import_state(meta_path=[self.FakeMetaFinder]):
                self.assertEqual((name, path),
                                 importlib.find_loader(name, path))

    def test_nothing(self):
        # None is returned upon failure to find a loader.
        self.assertIsNone(importlib.find_loader('nevergoingtofindthismodule'))


class InvalidateCacheTests(unittest.TestCase):

    def test_method_called(self):
        # If defined the method should be called.
        class InvalidatingNullFinder:
            def __init__(self, *ignored):
                self.called = False
            def find_module(self, *args):
                return None
            def invalidate_caches(self):
                self.called = True

        key = 'gobledeegook'
        meta_ins = InvalidatingNullFinder()
        path_ins = InvalidatingNullFinder()
        sys.meta_path.insert(0, meta_ins)
        self.addCleanup(lambda: sys.path_importer_cache.__delitem__(key))
        sys.path_importer_cache[key] = path_ins
        self.addCleanup(lambda: sys.meta_path.remove(meta_ins))
        importlib.invalidate_caches()
        self.assertTrue(meta_ins.called)
        self.assertTrue(path_ins.called)

    def test_method_lacking(self):
        # There should be no issues if the method is not defined.
        key = 'gobbledeegook'
        sys.path_importer_cache[key] = imp.NullImporter('abc')
        self.addCleanup(lambda: sys.path_importer_cache.__delitem__(key))
        importlib.invalidate_caches()  # Shouldn't trigger an exception.


class FrozenImportlibTests(unittest.TestCase):

    def test_no_frozen_importlib(self):
        # Should be able to import w/o _frozen_importlib being defined.
        module = support.import_fresh_module('importlib', blocked=['_frozen_importlib'])
        self.assertFalse(isinstance(module.__loader__,
                                    machinery.FrozenImporter))


class StartupTests(unittest.TestCase):

    def test_everyone_has___loader__(self):
        # Issue #17098: all modules should have __loader__ defined.
        for name, module in sys.modules.items():
            if isinstance(module, types.ModuleType):
                # pyexpat/xml.parsers.expat have submodules that it creates
                # by hand and do not set __loader__, which is acceptable.
                if 'expat' in name:
                    continue
                self.assertTrue(hasattr(module, '__loader__'),
                        '{!r} lacks a __loader__ attribute'.format(name))
                if name in sys.builtin_module_names:
                    self.assertIn(module.__loader__,
                            (importlib.machinery.BuiltinImporter,
                             importlib._bootstrap.BuiltinImporter))
                elif imp.is_frozen(name):
                    self.assertIn(module.__loader__,
                            (importlib.machinery.FrozenImporter,
                             importlib._bootstrap.FrozenImporter))


if __name__ == '__main__':
    unittest.main()
