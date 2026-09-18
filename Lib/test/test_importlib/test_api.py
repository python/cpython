from test.test_importlib import util as test_util

init = test_util.import_importlib('importlib')
util = test_util.import_importlib('importlib.util')
machinery = test_util.import_importlib('importlib.machinery')

import os.path
import sys
from test import support
from test.support import import_helper
from test.support import os_helper
import traceback
import types
import unittest


class ImportModuleTests:

    """Test importlib.import_module."""

    def test_module_import(self):
        # Test importing a top-level module.
        with test_util.mock_spec('top_level') as mock:
            with test_util.import_state(meta_path=[mock]):
                module = self.init.import_module('top_level')
                self.assertEqual(module.__name__, 'top_level')

    def test_absolute_package_import(self):
        # Test importing a module from a package with an absolute name.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with test_util.mock_spec(pkg_long_name, name) as mock:
            with test_util.import_state(meta_path=[mock]):
                module = self.init.import_module(name)
                self.assertEqual(module.__name__, name)

    def test_shallow_relative_package_import(self):
        # Test importing a module from a package through a relative import.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        module_name = 'mod'
        absolute_name = '{0}.{1}'.format(pkg_name, module_name)
        relative_name = '.{0}'.format(module_name)
        with test_util.mock_spec(pkg_long_name, absolute_name) as mock:
            with test_util.import_state(meta_path=[mock]):
                self.init.import_module(pkg_name)
                module = self.init.import_module(relative_name, pkg_name)
                self.assertEqual(module.__name__, absolute_name)

    def test_deep_relative_package_import(self):
        modules = ['a.__init__', 'a.b.__init__', 'a.c']
        with test_util.mock_spec(*modules) as mock:
            with test_util.import_state(meta_path=[mock]):
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
        with test_util.mock_spec(pkg_long_name, name) as mock:
            with test_util.import_state(meta_path=[mock]):
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
        with test_util.mock_spec(*modules, module_code=code) as mock:
            with test_util.import_state(meta_path=[mock]):
                self.init.import_module('a.b')
        self.assertEqual(b_load_count, 1)


(Frozen_ImportModuleTests,
 Source_ImportModuleTests
 ) = test_util.test_both(
     ImportModuleTests, init=init, util=util, machinery=machinery)


class FindLoaderTests:

    FakeMetaFinder = None

    def test_sys_modules(self):
        # If a module with __spec__.loader is in sys.modules, then return it.
        name = 'some_mod'
        with test_util.uncache(name):
            module = types.ModuleType(name)
            loader = 'a loader!'
            module.__spec__ = self.machinery.ModuleSpec(name, loader)
            sys.modules[name] = module
            spec = self.util.find_spec(name)
            self.assertIsNotNone(spec)
            self.assertEqual(spec.loader, loader)

    def test_sys_modules_loader_is_None(self):
        # If sys.modules[name].__spec__.loader is None, raise ValueError.
        name = 'some_mod'
        with test_util.uncache(name):
            module = types.ModuleType(name)
            module.__loader__ = None
            sys.modules[name] = module
            with self.assertRaises(ValueError):
                self.util.find_spec(name)

    def test_sys_modules_loader_is_not_set(self):
        # Should raise ValueError
        # Issue #17099
        name = 'some_mod'
        with test_util.uncache(name):
            module = types.ModuleType(name)
            try:
                del module.__spec__.loader
            except AttributeError:
                pass
            sys.modules[name] = module
            with self.assertRaises(ValueError):
                self.util.find_spec(name)

    def test_success(self):
        # Return the loader found on sys.meta_path.
        name = 'some_mod'
        with test_util.uncache(name):
            with test_util.import_state(meta_path=[self.FakeMetaFinder]):
                spec = self.util.find_spec(name)
                self.assertEqual((name, (name, None)), (spec.name, spec.loader))

    def test_success_path(self):
        # Searching on a path should work.
        name = 'some_mod'
        path = 'path to some place'
        with test_util.uncache(name):
            with test_util.import_state(meta_path=[self.FakeMetaFinder]):
                spec = self.util.find_spec(name, path)
                self.assertEqual(name, spec.name)

    def test_nothing(self):
        # None is returned upon failure to find a loader.
        self.assertIsNone(self.util.find_spec('nevergoingtofindthismodule'))


class FindLoaderPEP451Tests(FindLoaderTests):

    class FakeMetaFinder:
        @staticmethod
        def find_spec(name, path=None, target=None):
            return machinery['Source'].ModuleSpec(name, (name, path))


(Frozen_FindLoaderPEP451Tests,
 Source_FindLoaderPEP451Tests
 ) = test_util.test_both(
     FindLoaderPEP451Tests, init=init, util=util, machinery=machinery)


class ReloadTests:

    def test_reload_modules(self):
        for mod in ('tokenize', 'time', 'marshal'):
            with self.subTest(module=mod):
                with import_helper.CleanImport(mod):
                    module = self.init.import_module(mod)
                    self.init.reload(module)

    def test_module_replaced(self):
        def code():
            import sys
            module = type(sys)('top_level')
            module.spam = 3
            sys.modules['top_level'] = module
        mock = test_util.mock_spec('top_level',
                                   module_code={'top_level': code})
        with mock:
            with test_util.import_state(meta_path=[mock]):
                module = self.init.import_module('top_level')
                reloaded = self.init.reload(module)
                actual = sys.modules['top_level']
                self.assertEqual(actual.spam, 3)
                self.assertEqual(reloaded.spam, 3)

    def test_reload_missing_loader(self):
        with import_helper.CleanImport('types'):
            import types
            loader = types.__loader__
            del types.__loader__
            reloaded = self.init.reload(types)

            self.assertIs(reloaded, types)
            self.assertIs(sys.modules['types'], types)
            self.assertEqual(reloaded.__loader__.path, loader.path)

    def test_reload_loader_replaced(self):
        with import_helper.CleanImport('types'):
            import types
            types.__loader__ = None
            self.init.invalidate_caches()
            reloaded = self.init.reload(types)

            self.assertIsNot(reloaded.__loader__, None)
            self.assertIs(reloaded, types)
            self.assertIs(sys.modules['types'], types)

    def test_reload_location_changed(self):
        name = 'spam'
        with os_helper.temp_cwd(None) as cwd:
            with test_util.uncache('spam'):
                with import_helper.DirsOnSysPath(cwd):
                    # Start as a plain module.
                    self.init.invalidate_caches()
                    path = os.path.join(cwd, name + '.py')
                    expected = {'__name__': name,
                                '__package__': '',
                                '__file__': path,
                                '__doc__': None,
                                }
                    os_helper.create_empty_file(path)
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
                    expected = {'__name__': name,
                                '__package__': name,
                                '__file__': init_path,
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
        with os_helper.temp_cwd(None) as cwd:
            with test_util.uncache('spam'):
                with test_util.import_state(path=[cwd]):
                    self.init._bootstrap_external._install(self.init._bootstrap)
                    # Start as a namespace package.
                    self.init.invalidate_caches()
                    bad_path = os.path.join(cwd, name, '__init.py')
                    expected = {'__name__': name,
                                '__package__': name,
                                '__doc__': None,
                                '__file__': None,
                                }
                    os.mkdir(name)
                    with open(bad_path, 'w', encoding='utf-8') as init_file:
                        init_file.write('eggs = None')
                    module = self.init.import_module(name)
                    ns = vars(module).copy()
                    loader = ns.pop('__loader__')
                    path = ns.pop('__path__')
                    spec = ns.pop('__spec__')
                    ns.pop('__builtins__', None)  # An implementation detail.
                    self.assertEqual(spec.name, name)
                    self.assertIsNotNone(spec.loader)
                    self.assertIsNotNone(loader)
                    self.assertEqual(spec.loader, loader)
                    self.assertEqual(set(path),
                                     set([os.path.dirname(bad_path)]))
                    with self.assertRaises(AttributeError):
                        # a NamespaceLoader
                        loader.path
                    self.assertEqual(ns, expected)

                    # Change to a regular package.
                    self.init.invalidate_caches()
                    init_path = os.path.join(cwd, name, '__init__.py')
                    expected = {'__name__': name,
                                '__package__': name,
                                '__file__': init_path,
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
        with test_util.temp_module(name, pkg=True) as pkg_dir:
            fullname, _ = test_util.submodule(name, subname, pkg_dir)
            ham = self.init.import_module(fullname)
            reloaded = self.init.reload(ham)
            self.assertIs(reloaded, ham)

    def test_module_missing_spec(self):
        #Test that reload() throws ModuleNotFounderror when reloading
        # a module whose missing a spec. (bpo-29851)
        name = 'spam'
        with test_util.uncache(name):
            module = sys.modules[name] = types.ModuleType(name)
            # Sanity check by attempting an import.
            module = self.init.import_module(name)
            self.assertIsNone(module.__spec__)
            with self.assertRaises(ModuleNotFoundError):
                self.init.reload(module)

    def test_reload_traceback_with_non_str(self):
        # gh-125519
        with support.captured_stdout() as stdout:
            try:
                self.init.reload("typing")
            except TypeError as exc:
                traceback.print_exception(exc, file=stdout)
            else:
                self.fail("Expected TypeError to be raised")
        printed_traceback = stdout.getvalue()
        self.assertIn("TypeError", printed_traceback)
        self.assertNotIn("AttributeError", printed_traceback)
        self.assertNotIn("module.__spec__.name", printed_traceback)


(Frozen_ReloadTests,
 Source_ReloadTests
 ) = test_util.test_both(
     ReloadTests, init=init, util=util, machinery=machinery)


class InvalidateCacheTests:

    def test_method_called(self):
        # If defined the method should be called.
        class InvalidatingNullFinder:
            def __init__(self, *ignored):
                self.called = False
            def invalidate_caches(self):
                self.called = True

        key = os.path.abspath('gobledeegook')
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
        self.addCleanup(lambda: sys.path_importer_cache.pop(key, None))
        self.init.invalidate_caches()  # Shouldn't trigger an exception.


(Frozen_InvalidateCacheTests,
 Source_InvalidateCacheTests
 ) = test_util.test_both(
     InvalidateCacheTests, init=init, util=util, machinery=machinery)


class FrozenImportlibTests(unittest.TestCase):

    def test_no_frozen_importlib(self):
        # Should be able to import w/o _frozen_importlib being defined.
        # Can't do an isinstance() check since separate copies of importlib
        # may have been used for import, so just check the name is not for the
        # frozen loader.
        source_init = init['Source']
        self.assertNotEqual(source_init.__loader__.__class__.__name__,
                            'FrozenImporter')


class StartupTests:

    def test_everyone_has___loader__(self):
        # Issue #17098: all modules should have __loader__ defined.
        for name, module in sys.modules.items():
            if isinstance(module, types.ModuleType):
                with self.subTest(name=name):
                    self.assertHasAttr(module, '__loader__')
                    if self.machinery.BuiltinImporter.find_spec(name):
                        self.assertIsNot(module.__loader__, None)
                    elif self.machinery.FrozenImporter.find_spec(name):
                        self.assertIsNot(module.__loader__, None)

    def test_everyone_has___spec__(self):
        for name, module in sys.modules.items():
            if isinstance(module, types.ModuleType):
                with self.subTest(name=name):
                    self.assertHasAttr(module, '__spec__')
                    if self.machinery.BuiltinImporter.find_spec(name):
                        self.assertIsNot(module.__spec__, None)
                    elif self.machinery.FrozenImporter.find_spec(name):
                        self.assertIsNot(module.__spec__, None)


(Frozen_StartupTests,
 Source_StartupTests
 ) = test_util.test_both(StartupTests, machinery=machinery)


class TestModuleAll(unittest.TestCase):
    def test_machinery(self):
        extra = (
            # from importlib._bootstrap and importlib._bootstrap_external
            'AppleFrameworkLoader',
            'BYTECODE_SUFFIXES',
            'BuiltinImporter',
            'DEBUG_BYTECODE_SUFFIXES',
            'EXTENSION_SUFFIXES',
            'ExtensionFileLoader',
            'FileFinder',
            'FrozenImporter',
            'ModuleSpec',
            'NamespaceLoader',
            'OPTIMIZED_BYTECODE_SUFFIXES',
            'PathFinder',
            'SOURCE_SUFFIXES',
            'SourceFileLoader',
            'SourcelessFileLoader',
            'WindowsRegistryFinder',
        )
        support.check__all__(self, machinery['Source'], extra=extra)

    def test_util(self):
        extra = (
            # from importlib.abc, importlib._bootstrap
            # and importlib._bootstrap_external
            'Loader',
            'MAGIC_NUMBER',
            'cache_from_source',
            'decode_source',
            'module_from_spec',
            'source_from_cache',
            'spec_from_file_location',
            'spec_from_loader',
        )
        support.check__all__(self, util['Source'], extra=extra)


class TestDeprecations(unittest.TestCase):
    def test_machinery_deprecated_attributes(self):
        from importlib import machinery
        attributes = (
            'DEBUG_BYTECODE_SUFFIXES',
            'OPTIMIZED_BYTECODE_SUFFIXES',
        )
        for attr in attributes:
            with self.subTest(attr=attr):
                with self.assertWarns(DeprecationWarning):
                    getattr(machinery, attr)


if __name__ == '__main__':
    unittest.main()
