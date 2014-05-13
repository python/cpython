from . import util

frozen_init, source_init = util.import_importlib('importlib')
frozen_util, source_util = util.import_importlib('importlib.util')
frozen_machinery, source_machinery = util.import_importlib('importlib.machinery')

import os.path
import sys
from test import support
import types
import unittest
import warnings


class ImportModuleTests:

    """Test importlib.import_module."""

    def test_module_import(self):
        # Test importing a top-level module.
        with util.mock_modules('top_level') as mock:
            with util.import_state(meta_path=[mock]):
                module = self.init.import_module('top_level')
                self.assertEqual(module.__name__, 'top_level')

    def test_absolute_package_import(self):
        # Test importing a module from a package with an absolute name.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with util.mock_modules(pkg_long_name, name) as mock:
            with util.import_state(meta_path=[mock]):
                module = self.init.import_module(name)
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
                self.init.import_module(pkg_name)
                module = self.init.import_module(relative_name, pkg_name)
                self.assertEqual(module.__name__, absolute_name)

    def test_deep_relative_package_import(self):
        modules = ['a.__init__', 'a.b.__init__', 'a.c']
        with util.mock_modules(*modules) as mock:
            with util.import_state(meta_path=[mock]):
                self.init.import_module('a')
                self.init.import_module('a.b')
                module = self.init.import_module('..c', 'a.b')
                self.assertEqual(module.__name__, 'a.c')

    def test_absolute_import_with_package(self):
        # Test importing a module from a package with an absolute name with
        # the 'package' argument given.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with util.mock_modules(pkg_long_name, name) as mock:
            with util.import_state(meta_path=[mock]):
                self.init.import_module(pkg_name)
                module = self.init.import_module(name, pkg_name)
                self.assertEqual(module.__name__, name)

    def test_relative_import_wo_package(self):
        # Relative imports cannot happen without the 'package' argument being
        # set.
        with self.assertRaises(TypeError):
            self.init.import_module('.support')


    def test_loaded_once(self):
        # Issue #13591: Modules should only be loaded once when
        # initializing the parent package attempts to import the
        # module currently being imported.
        b_load_count = 0
        def load_a():
            self.init.import_module('a.b')
        def load_b():
            nonlocal b_load_count
            b_load_count += 1
        code = {'a': load_a, 'a.b': load_b}
        modules = ['a.__init__', 'a.b']
        with util.mock_modules(*modules, module_code=code) as mock:
            with util.import_state(meta_path=[mock]):
                self.init.import_module('a.b')
        self.assertEqual(b_load_count, 1)

class Frozen_ImportModuleTests(ImportModuleTests, unittest.TestCase):
    init = frozen_init

class Source_ImportModuleTests(ImportModuleTests, unittest.TestCase):
    init = source_init


class FindLoaderTests:

    class FakeMetaFinder:
        @staticmethod
        def find_module(name, path=None): return name, path

    def test_sys_modules(self):
        # If a module with __loader__ is in sys.modules, then return it.
        name = 'some_mod'
        with util.uncache(name):
            module = types.ModuleType(name)
            loader = 'a loader!'
            module.__loader__ = loader
            sys.modules[name] = module
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', DeprecationWarning)
                found = self.init.find_loader(name)
            self.assertEqual(loader, found)

    def test_sys_modules_loader_is_None(self):
        # If sys.modules[name].__loader__ is None, raise ValueError.
        name = 'some_mod'
        with util.uncache(name):
            module = types.ModuleType(name)
            module.__loader__ = None
            sys.modules[name] = module
            with self.assertRaises(ValueError):
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', DeprecationWarning)
                    self.init.find_loader(name)

    def test_sys_modules_loader_is_not_set(self):
        # Should raise ValueError
        # Issue #17099
        name = 'some_mod'
        with util.uncache(name):
            module = types.ModuleType(name)
            try:
                del module.__loader__
            except AttributeError:
                pass
            sys.modules[name] = module
            with self.assertRaises(ValueError):
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', DeprecationWarning)
                    self.init.find_loader(name)

    def test_success(self):
        # Return the loader found on sys.meta_path.
        name = 'some_mod'
        with util.uncache(name):
            with util.import_state(meta_path=[self.FakeMetaFinder]):
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', DeprecationWarning)
                    self.assertEqual((name, None), self.init.find_loader(name))

    def test_success_path(self):
        # Searching on a path should work.
        name = 'some_mod'
        path = 'path to some place'
        with util.uncache(name):
            with util.import_state(meta_path=[self.FakeMetaFinder]):
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', DeprecationWarning)
                    self.assertEqual((name, path),
                                     self.init.find_loader(name, path))

    def test_nothing(self):
        # None is returned upon failure to find a loader.
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            self.assertIsNone(self.init.find_loader('nevergoingtofindthismodule'))

class Frozen_FindLoaderTests(FindLoaderTests, unittest.TestCase):
    init = frozen_init

class Source_FindLoaderTests(FindLoaderTests, unittest.TestCase):
    init = source_init


class ReloadTests:

    """Test module reloading for builtin and extension modules."""

    def test_reload_modules(self):
        for mod in ('tokenize', 'time', 'marshal'):
            with self.subTest(module=mod):
                with support.CleanImport(mod):
                    module = self.init.import_module(mod)
                    self.init.reload(module)

    def test_module_replaced(self):
        def code():
            import sys
            module = type(sys)('top_level')
            module.spam = 3
            sys.modules['top_level'] = module
        mock = util.mock_modules('top_level',
                                 module_code={'top_level': code})
        with mock:
            with util.import_state(meta_path=[mock]):
                module = self.init.import_module('top_level')
                reloaded = self.init.reload(module)
                actual = sys.modules['top_level']
                self.assertEqual(actual.spam, 3)
                self.assertEqual(reloaded.spam, 3)

    def test_reload_missing_loader(self):
        with support.CleanImport('types'):
            import types
            loader = types.__loader__
            del types.__loader__
            reloaded = self.init.reload(types)

            self.assertIs(reloaded, types)
            self.assertIs(sys.modules['types'], types)
            self.assertEqual(reloaded.__loader__.path, loader.path)

    def test_reload_loader_replaced(self):
        with support.CleanImport('types'):
            import types
            types.__loader__ = None
            self.init.invalidate_caches()
            reloaded = self.init.reload(types)

            self.assertIsNot(reloaded.__loader__, None)
            self.assertIs(reloaded, types)
            self.assertIs(sys.modules['types'], types)

    def test_reload_location_changed(self):
        name = 'spam'
        with support.temp_cwd(None) as cwd:
            with util.uncache('spam'):
                with support.DirsOnSysPath(cwd):
                    # Start as a plain module.
                    self.init.invalidate_caches()
                    path = os.path.join(cwd, name + '.py')
                    cached = self.util.cache_from_source(path)
                    expected = {'__name__': name,
                                '__package__': '',
                                '__file__': path,
                                '__cached__': cached,
                                '__doc__': None,
                                }
                    support.create_empty_file(path)
                    module = self.init.import_module(name)
                    ns = vars(module).copy()
                    loader = ns.pop('__loader__')
                    spec = ns.pop('__spec__')
                    ns.pop('__builtins__', None)  # An implementation detail.
                    self.assertEqual(spec.name, name)
                    self.assertEqual(spec.loader, loader)
                    self.assertEqual(loader.path, path)
                    self.assertEqual(ns, expected)

                    # Change to a package.
                    self.init.invalidate_caches()
                    init_path = os.path.join(cwd, name, '__init__.py')
                    cached = self.util.cache_from_source(init_path)
                    expected = {'__name__': name,
                                '__package__': name,
                                '__file__': init_path,
                                '__cached__': cached,
                                '__path__': [os.path.dirname(init_path)],
                                '__doc__': None,
                                }
                    os.mkdir(name)
                    os.rename(path, init_path)
                    reloaded = self.init.reload(module)
                    ns = vars(reloaded).copy()
                    loader = ns.pop('__loader__')
                    spec = ns.pop('__spec__')
                    ns.pop('__builtins__', None)  # An implementation detail.
                    self.assertEqual(spec.name, name)
                    self.assertEqual(spec.loader, loader)
                    self.assertIs(reloaded, module)
                    self.assertEqual(loader.path, init_path)
                    self.maxDiff = None
                    self.assertEqual(ns, expected)

    def test_reload_namespace_changed(self):
        name = 'spam'
        with support.temp_cwd(None) as cwd:
            with util.uncache('spam'):
                with support.DirsOnSysPath(cwd):
                    # Start as a namespace package.
                    self.init.invalidate_caches()
                    bad_path = os.path.join(cwd, name, '__init.py')
                    cached = self.util.cache_from_source(bad_path)
                    expected = {'__name__': name,
                                '__package__': name,
                                '__doc__': None,
                                }
                    os.mkdir(name)
                    with open(bad_path, 'w') as init_file:
                        init_file.write('eggs = None')
                    module = self.init.import_module(name)
                    ns = vars(module).copy()
                    loader = ns.pop('__loader__')
                    path = ns.pop('__path__')
                    spec = ns.pop('__spec__')
                    ns.pop('__builtins__', None)  # An implementation detail.
                    self.assertEqual(spec.name, name)
                    self.assertIs(spec.loader, None)
                    self.assertIsNot(loader, None)
                    self.assertEqual(set(path),
                                     set([os.path.dirname(bad_path)]))
                    with self.assertRaises(AttributeError):
                        # a NamespaceLoader
                        loader.path
                    self.assertEqual(ns, expected)

                    # Change to a regular package.
                    self.init.invalidate_caches()
                    init_path = os.path.join(cwd, name, '__init__.py')
                    cached = self.util.cache_from_source(init_path)
                    expected = {'__name__': name,
                                '__package__': name,
                                '__file__': init_path,
                                '__cached__': cached,
                                '__path__': [os.path.dirname(init_path)],
                                '__doc__': None,
                                'eggs': None,
                                }
                    os.rename(bad_path, init_path)
                    reloaded = self.init.reload(module)
                    ns = vars(reloaded).copy()
                    loader = ns.pop('__loader__')
                    spec = ns.pop('__spec__')
                    ns.pop('__builtins__', None)  # An implementation detail.
                    self.assertEqual(spec.name, name)
                    self.assertEqual(spec.loader, loader)
                    self.assertIs(reloaded, module)
                    self.assertEqual(loader.path, init_path)
                    self.assertEqual(ns, expected)

    def test_reload_submodule(self):
        # See #19851.
        name = 'spam'
        subname = 'ham'
        with util.temp_module(name, pkg=True) as pkg_dir:
            fullname, _ = util.submodule(name, subname, pkg_dir)
            ham = self.init.import_module(fullname)
            reloaded = self.init.reload(ham)
            self.assertIs(reloaded, ham)


class Frozen_ReloadTests(ReloadTests, unittest.TestCase):
    init = frozen_init
    util = frozen_util

class Source_ReloadTests(ReloadTests, unittest.TestCase):
    init = source_init
    util = source_util


class InvalidateCacheTests:

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
        self.init.invalidate_caches()
        self.assertTrue(meta_ins.called)
        self.assertTrue(path_ins.called)

    def test_method_lacking(self):
        # There should be no issues if the method is not defined.
        key = 'gobbledeegook'
        sys.path_importer_cache[key] = None
        self.addCleanup(lambda: sys.path_importer_cache.__delitem__(key))
        self.init.invalidate_caches()  # Shouldn't trigger an exception.

class Frozen_InvalidateCacheTests(InvalidateCacheTests, unittest.TestCase):
    init = frozen_init

class Source_InvalidateCacheTests(InvalidateCacheTests, unittest.TestCase):
    init = source_init


class FrozenImportlibTests(unittest.TestCase):

    def test_no_frozen_importlib(self):
        # Should be able to import w/o _frozen_importlib being defined.
        # Can't do an isinstance() check since separate copies of importlib
        # may have been used for import, so just check the name is not for the
        # frozen loader.
        self.assertNotEqual(source_init.__loader__.__class__.__name__,
                            'FrozenImporter')


class StartupTests:

    def test_everyone_has___loader__(self):
        # Issue #17098: all modules should have __loader__ defined.
        for name, module in sys.modules.items():
            if isinstance(module, types.ModuleType):
                with self.subTest(name=name):
                    self.assertTrue(hasattr(module, '__loader__'),
                                    '{!r} lacks a __loader__ attribute'.format(name))
                    if self.machinery.BuiltinImporter.find_module(name):
                        self.assertIsNot(module.__loader__, None)
                    elif self.machinery.FrozenImporter.find_module(name):
                        self.assertIsNot(module.__loader__, None)

    def test_everyone_has___spec__(self):
        for name, module in sys.modules.items():
            if isinstance(module, types.ModuleType):
                with self.subTest(name=name):
                    self.assertTrue(hasattr(module, '__spec__'))
                    if self.machinery.BuiltinImporter.find_module(name):
                        self.assertIsNot(module.__spec__, None)
                    elif self.machinery.FrozenImporter.find_module(name):
                        self.assertIsNot(module.__spec__, None)

class Frozen_StartupTests(StartupTests, unittest.TestCase):
    machinery = frozen_machinery

class Source_StartupTests(StartupTests, unittest.TestCase):
    machinery = source_machinery


if __name__ == '__main__':
    unittest.main()
