from . import util

frozen_init, source_init = util.import_importlib('importlib')
frozen_bootstrap = frozen_init._bootstrap
source_bootstrap = source_init._bootstrap
frozen_machinery, source_machinery = util.import_importlib('importlib.machinery')
frozen_util, source_util = util.import_importlib('importlib.util')

import os.path
from test.support import CleanImport
import unittest
import sys
import warnings



class TestLoader:

    def __init__(self, path=None, is_package=None):
        self.path = path
        self.package = is_package

    def __repr__(self):
        return '<TestLoader object>'

    def __getattr__(self, name):
        if name == 'get_filename' and self.path is not None:
            return self._get_filename
        if name == 'is_package':
            return self._is_package
        raise AttributeError(name)

    def _get_filename(self, name):
        return self.path

    def _is_package(self, name):
        return self.package


class NewLoader(TestLoader):

    EGGS = 1

    def exec_module(self, module):
        module.eggs = self.EGGS


class LegacyLoader(TestLoader):

    HAM = -1

    with warnings.catch_warnings():
        warnings.simplefilter("ignore", DeprecationWarning)

        @frozen_util.module_for_loader
        def load_module(self, module):
            module.ham = self.HAM
            return module


class ModuleSpecTests:

    def setUp(self):
        self.name = 'spam'
        self.path = 'spam.py'
        self.cached = self.util.cache_from_source(self.path)
        self.loader = TestLoader()
        self.spec = self.machinery.ModuleSpec(self.name, self.loader)
        self.loc_spec = self.machinery.ModuleSpec(self.name, self.loader,
                                                  origin=self.path)
        self.loc_spec._set_fileattr = True

    def test_default(self):
        spec = self.machinery.ModuleSpec(self.name, self.loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_default_no_loader(self):
        spec = self.machinery.ModuleSpec(self.name, None)

        self.assertEqual(spec.name, self.name)
        self.assertIs(spec.loader, None)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_default_is_package_false(self):
        spec = self.machinery.ModuleSpec(self.name, self.loader,
                                         is_package=False)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_default_is_package_true(self):
        spec = self.machinery.ModuleSpec(self.name, self.loader,
                                         is_package=True)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertEqual(spec.submodule_search_locations, [])
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_has_location_setter(self):
        spec = self.machinery.ModuleSpec(self.name, self.loader,
                                         origin='somewhere')
        self.assertFalse(spec.has_location)
        spec.has_location = True
        self.assertTrue(spec.has_location)

    def test_equality(self):
        other = type(sys.implementation)(name=self.name,
                                         loader=self.loader,
                                         origin=None,
                                         submodule_search_locations=None,
                                         has_location=False,
                                         cached=None,
                                         )

        self.assertTrue(self.spec == other)

    def test_equality_location(self):
        other = type(sys.implementation)(name=self.name,
                                         loader=self.loader,
                                         origin=self.path,
                                         submodule_search_locations=None,
                                         has_location=True,
                                         cached=self.cached,
                                         )

        self.assertEqual(self.loc_spec, other)

    def test_inequality(self):
        other = type(sys.implementation)(name='ham',
                                         loader=self.loader,
                                         origin=None,
                                         submodule_search_locations=None,
                                         has_location=False,
                                         cached=None,
                                         )

        self.assertNotEqual(self.spec, other)

    def test_inequality_incomplete(self):
        other = type(sys.implementation)(name=self.name,
                                         loader=self.loader,
                                         )

        self.assertNotEqual(self.spec, other)

    def test_package(self):
        spec = self.machinery.ModuleSpec('spam.eggs', self.loader)

        self.assertEqual(spec.parent, 'spam')

    def test_package_is_package(self):
        spec = self.machinery.ModuleSpec('spam.eggs', self.loader,
                                         is_package=True)

        self.assertEqual(spec.parent, 'spam.eggs')

    # cached

    def test_cached_set(self):
        before = self.spec.cached
        self.spec.cached = 'there'
        after = self.spec.cached

        self.assertIs(before, None)
        self.assertEqual(after, 'there')

    def test_cached_no_origin(self):
        spec = self.machinery.ModuleSpec(self.name, self.loader)

        self.assertIs(spec.cached, None)

    def test_cached_with_origin_not_location(self):
        spec = self.machinery.ModuleSpec(self.name, self.loader,
                                         origin=self.path)

        self.assertIs(spec.cached, None)

    def test_cached_source(self):
        expected = self.util.cache_from_source(self.path)

        self.assertEqual(self.loc_spec.cached, expected)

    def test_cached_source_unknown_suffix(self):
        self.loc_spec.origin = 'spam.spamspamspam'

        self.assertIs(self.loc_spec.cached, None)

    def test_cached_source_missing_cache_tag(self):
        original = sys.implementation.cache_tag
        sys.implementation.cache_tag = None
        try:
            cached = self.loc_spec.cached
        finally:
            sys.implementation.cache_tag = original

        self.assertIs(cached, None)

    def test_cached_sourceless(self):
        self.loc_spec.origin = 'spam.pyc'

        self.assertEqual(self.loc_spec.cached, 'spam.pyc')


class Frozen_ModuleSpecTests(ModuleSpecTests, unittest.TestCase):
    util = frozen_util
    machinery = frozen_machinery


class Source_ModuleSpecTests(ModuleSpecTests, unittest.TestCase):
    util = source_util
    machinery = source_machinery


class ModuleSpecMethodsTests:

    def setUp(self):
        self.name = 'spam'
        self.path = 'spam.py'
        self.cached = self.util.cache_from_source(self.path)
        self.loader = TestLoader()
        self.spec = self.machinery.ModuleSpec(self.name, self.loader)
        self.loc_spec = self.machinery.ModuleSpec(self.name, self.loader,
                                                  origin=self.path)
        self.loc_spec._set_fileattr = True

    # init_module_attrs

    def test_init_module_attrs(self):
        module = type(sys)(self.name)
        spec = self.machinery.ModuleSpec(self.name, self.loader)
        self.bootstrap._SpecMethods(spec).init_module_attrs(module)

        self.assertEqual(module.__name__, spec.name)
        self.assertIs(module.__loader__, spec.loader)
        self.assertEqual(module.__package__, spec.parent)
        self.assertIs(module.__spec__, spec)
        self.assertFalse(hasattr(module, '__path__'))
        self.assertFalse(hasattr(module, '__file__'))
        self.assertFalse(hasattr(module, '__cached__'))

    def test_init_module_attrs_package(self):
        module = type(sys)(self.name)
        spec = self.machinery.ModuleSpec(self.name, self.loader)
        spec.submodule_search_locations = ['spam', 'ham']
        self.bootstrap._SpecMethods(spec).init_module_attrs(module)

        self.assertEqual(module.__name__, spec.name)
        self.assertIs(module.__loader__, spec.loader)
        self.assertEqual(module.__package__, spec.parent)
        self.assertIs(module.__spec__, spec)
        self.assertIs(module.__path__, spec.submodule_search_locations)
        self.assertFalse(hasattr(module, '__file__'))
        self.assertFalse(hasattr(module, '__cached__'))

    def test_init_module_attrs_location(self):
        module = type(sys)(self.name)
        spec = self.loc_spec
        self.bootstrap._SpecMethods(spec).init_module_attrs(module)

        self.assertEqual(module.__name__, spec.name)
        self.assertIs(module.__loader__, spec.loader)
        self.assertEqual(module.__package__, spec.parent)
        self.assertIs(module.__spec__, spec)
        self.assertFalse(hasattr(module, '__path__'))
        self.assertEqual(module.__file__, spec.origin)
        self.assertEqual(module.__cached__,
                         self.util.cache_from_source(spec.origin))

    def test_init_module_attrs_different_name(self):
        module = type(sys)('eggs')
        spec = self.machinery.ModuleSpec(self.name, self.loader)
        self.bootstrap._SpecMethods(spec).init_module_attrs(module)

        self.assertEqual(module.__name__, spec.name)

    def test_init_module_attrs_different_spec(self):
        module = type(sys)(self.name)
        module.__spec__ = self.machinery.ModuleSpec('eggs', object())
        spec = self.machinery.ModuleSpec(self.name, self.loader)
        self.bootstrap._SpecMethods(spec).init_module_attrs(module)

        self.assertEqual(module.__name__, spec.name)
        self.assertIs(module.__loader__, spec.loader)
        self.assertEqual(module.__package__, spec.parent)
        self.assertIs(module.__spec__, spec)

    def test_init_module_attrs_already_set(self):
        module = type(sys)('ham.eggs')
        module.__loader__ = object()
        module.__package__ = 'ham'
        module.__path__ = ['eggs']
        module.__file__ = 'ham/eggs/__init__.py'
        module.__cached__ = self.util.cache_from_source(module.__file__)
        original = vars(module).copy()
        spec = self.loc_spec
        spec.submodule_search_locations = ['']
        self.bootstrap._SpecMethods(spec).init_module_attrs(module)

        self.assertIs(module.__loader__, original['__loader__'])
        self.assertEqual(module.__package__, original['__package__'])
        self.assertIs(module.__path__, original['__path__'])
        self.assertEqual(module.__file__, original['__file__'])
        self.assertEqual(module.__cached__, original['__cached__'])

    def test_init_module_attrs_immutable(self):
        module = object()
        spec = self.loc_spec
        spec.submodule_search_locations = ['']
        self.bootstrap._SpecMethods(spec).init_module_attrs(module)

        self.assertFalse(hasattr(module, '__name__'))
        self.assertFalse(hasattr(module, '__loader__'))
        self.assertFalse(hasattr(module, '__package__'))
        self.assertFalse(hasattr(module, '__spec__'))
        self.assertFalse(hasattr(module, '__path__'))
        self.assertFalse(hasattr(module, '__file__'))
        self.assertFalse(hasattr(module, '__cached__'))

    # create()

    def test_create(self):
        created = self.bootstrap._SpecMethods(self.spec).create()

        self.assertEqual(created.__name__, self.spec.name)
        self.assertIs(created.__loader__, self.spec.loader)
        self.assertEqual(created.__package__, self.spec.parent)
        self.assertIs(created.__spec__, self.spec)
        self.assertFalse(hasattr(created, '__path__'))
        self.assertFalse(hasattr(created, '__file__'))
        self.assertFalse(hasattr(created, '__cached__'))

    def test_create_from_loader(self):
        module = type(sys.implementation)()
        class CreatingLoader(TestLoader):
            def create_module(self, spec):
                return module
        self.spec.loader = CreatingLoader()
        created = self.bootstrap._SpecMethods(self.spec).create()

        self.assertIs(created, module)
        self.assertEqual(created.__name__, self.spec.name)
        self.assertIs(created.__loader__, self.spec.loader)
        self.assertEqual(created.__package__, self.spec.parent)
        self.assertIs(created.__spec__, self.spec)
        self.assertFalse(hasattr(created, '__path__'))
        self.assertFalse(hasattr(created, '__file__'))
        self.assertFalse(hasattr(created, '__cached__'))

    def test_create_from_loader_not_handled(self):
        class CreatingLoader(TestLoader):
            def create_module(self, spec):
                return None
        self.spec.loader = CreatingLoader()
        created = self.bootstrap._SpecMethods(self.spec).create()

        self.assertEqual(created.__name__, self.spec.name)
        self.assertIs(created.__loader__, self.spec.loader)
        self.assertEqual(created.__package__, self.spec.parent)
        self.assertIs(created.__spec__, self.spec)
        self.assertFalse(hasattr(created, '__path__'))
        self.assertFalse(hasattr(created, '__file__'))
        self.assertFalse(hasattr(created, '__cached__'))

    # exec()

    def test_exec(self):
        self.spec.loader = NewLoader()
        module = self.bootstrap._SpecMethods(self.spec).create()
        sys.modules[self.name] = module
        self.assertFalse(hasattr(module, 'eggs'))
        self.bootstrap._SpecMethods(self.spec).exec(module)

        self.assertEqual(module.eggs, 1)

    # load()

    def test_load(self):
        self.spec.loader = NewLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()
            installed = sys.modules[self.spec.name]

        self.assertEqual(loaded.eggs, 1)
        self.assertIs(loaded, installed)

    def test_load_replaced(self):
        replacement = object()
        class ReplacingLoader(TestLoader):
            def exec_module(self, module):
                sys.modules[module.__name__] = replacement
        self.spec.loader = ReplacingLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()
            installed = sys.modules[self.spec.name]

        self.assertIs(loaded, replacement)
        self.assertIs(installed, replacement)

    def test_load_failed(self):
        class FailedLoader(TestLoader):
            def exec_module(self, module):
                raise RuntimeError
        self.spec.loader = FailedLoader()
        with CleanImport(self.spec.name):
            with self.assertRaises(RuntimeError):
                loaded = self.bootstrap._SpecMethods(self.spec).load()
            self.assertNotIn(self.spec.name, sys.modules)

    def test_load_failed_removed(self):
        class FailedLoader(TestLoader):
            def exec_module(self, module):
                del sys.modules[module.__name__]
                raise RuntimeError
        self.spec.loader = FailedLoader()
        with CleanImport(self.spec.name):
            with self.assertRaises(RuntimeError):
                loaded = self.bootstrap._SpecMethods(self.spec).load()
            self.assertNotIn(self.spec.name, sys.modules)

    def test_load_legacy(self):
        self.spec.loader = LegacyLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()

        self.assertEqual(loaded.ham, -1)

    def test_load_legacy_attributes(self):
        self.spec.loader = LegacyLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()

        self.assertIs(loaded.__loader__, self.spec.loader)
        self.assertEqual(loaded.__package__, self.spec.parent)
        self.assertIs(loaded.__spec__, self.spec)

    def test_load_legacy_attributes_immutable(self):
        module = object()
        class ImmutableLoader(TestLoader):
            def load_module(self, name):
                sys.modules[name] = module
                return module
        self.spec.loader = ImmutableLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()

            self.assertIs(sys.modules[self.spec.name], module)

    # reload()

    def test_reload(self):
        self.spec.loader = NewLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()
            reloaded = self.bootstrap._SpecMethods(self.spec).exec(loaded)
            installed = sys.modules[self.spec.name]

        self.assertEqual(loaded.eggs, 1)
        self.assertIs(reloaded, loaded)
        self.assertIs(installed, loaded)

    def test_reload_modified(self):
        self.spec.loader = NewLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()
            loaded.eggs = 2
            reloaded = self.bootstrap._SpecMethods(self.spec).exec(loaded)

        self.assertEqual(loaded.eggs, 1)
        self.assertIs(reloaded, loaded)

    def test_reload_extra_attributes(self):
        self.spec.loader = NewLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()
            loaded.available = False
            reloaded = self.bootstrap._SpecMethods(self.spec).exec(loaded)

        self.assertFalse(loaded.available)
        self.assertIs(reloaded, loaded)

    def test_reload_init_module_attrs(self):
        self.spec.loader = NewLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()
            loaded.__name__ = 'ham'
            del loaded.__loader__
            del loaded.__package__
            del loaded.__spec__
            self.bootstrap._SpecMethods(self.spec).exec(loaded)

        self.assertEqual(loaded.__name__, self.spec.name)
        self.assertIs(loaded.__loader__, self.spec.loader)
        self.assertEqual(loaded.__package__, self.spec.parent)
        self.assertIs(loaded.__spec__, self.spec)
        self.assertFalse(hasattr(loaded, '__path__'))
        self.assertFalse(hasattr(loaded, '__file__'))
        self.assertFalse(hasattr(loaded, '__cached__'))

    def test_reload_legacy(self):
        self.spec.loader = LegacyLoader()
        with CleanImport(self.spec.name):
            loaded = self.bootstrap._SpecMethods(self.spec).load()
            reloaded = self.bootstrap._SpecMethods(self.spec).exec(loaded)
            installed = sys.modules[self.spec.name]

        self.assertEqual(loaded.ham, -1)
        self.assertIs(reloaded, loaded)
        self.assertIs(installed, loaded)


class Frozen_ModuleSpecMethodsTests(ModuleSpecMethodsTests, unittest.TestCase):
    bootstrap = frozen_bootstrap
    machinery = frozen_machinery
    util = frozen_util


class Source_ModuleSpecMethodsTests(ModuleSpecMethodsTests, unittest.TestCase):
    bootstrap = source_bootstrap
    machinery = source_machinery
    util = source_util


class ModuleReprTests:

    def setUp(self):
        self.module = type(os)('spam')
        self.spec = self.machinery.ModuleSpec('spam', TestLoader())

    def test_module___loader___module_repr(self):
        class Loader:
            def module_repr(self, module):
                return '<delicious {}>'.format(module.__name__)
        self.module.__loader__ = Loader()
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr, '<delicious spam>')

    def test_module___loader___module_repr_bad(self):
        class Loader(TestLoader):
            def module_repr(self, module):
                raise Exception
        self.module.__loader__ = Loader()
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr,
                         '<module {!r} (<TestLoader object>)>'.format('spam'))

    def test_module___spec__(self):
        origin = 'in a hole, in the ground'
        self.spec.origin = origin
        self.module.__spec__ = self.spec
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr, '<module {!r} ({})>'.format('spam', origin))

    def test_module___spec___location(self):
        location = 'in_a_galaxy_far_far_away.py'
        self.spec.origin = location
        self.spec._set_fileattr = True
        self.module.__spec__ = self.spec
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr,
                         '<module {!r} from {!r}>'.format('spam', location))

    def test_module___spec___no_origin(self):
        self.spec.loader = TestLoader()
        self.module.__spec__ = self.spec
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr,
                         '<module {!r} (<TestLoader object>)>'.format('spam'))

    def test_module___spec___no_origin_no_loader(self):
        self.spec.loader = None
        self.module.__spec__ = self.spec
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr, '<module {!r}>'.format('spam'))

    def test_module_no_name(self):
        del self.module.__name__
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr, '<module {!r}>'.format('?'))

    def test_module_with_file(self):
        filename = 'e/i/e/i/o/spam.py'
        self.module.__file__ = filename
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr,
                         '<module {!r} from {!r}>'.format('spam', filename))

    def test_module_no_file(self):
        self.module.__loader__ = TestLoader()
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr,
                         '<module {!r} (<TestLoader object>)>'.format('spam'))

    def test_module_no_file_no_loader(self):
        modrepr = self.bootstrap._module_repr(self.module)

        self.assertEqual(modrepr, '<module {!r}>'.format('spam'))


class Frozen_ModuleReprTests(ModuleReprTests, unittest.TestCase):
    bootstrap = frozen_bootstrap
    machinery = frozen_machinery
    util = frozen_util


class Source_ModuleReprTests(ModuleReprTests, unittest.TestCase):
    bootstrap = source_bootstrap
    machinery = source_machinery
    util = source_util


class FactoryTests:

    def setUp(self):
        self.name = 'spam'
        self.path = 'spam.py'
        self.cached = self.util.cache_from_source(self.path)
        self.loader = TestLoader()
        self.fileloader = TestLoader(self.path)
        self.pkgloader = TestLoader(self.path, True)

    # spec_from_loader()

    def test_spec_from_loader_default(self):
        spec = self.util.spec_from_loader(self.name, self.loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_spec_from_loader_default_with_bad_is_package(self):
        class Loader:
            def is_package(self, name):
                raise ImportError
        loader = Loader()
        spec = self.util.spec_from_loader(self.name, loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_spec_from_loader_origin(self):
        origin = 'somewhere over the rainbow'
        spec = self.util.spec_from_loader(self.name, self.loader,
                                          origin=origin)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertIs(spec.origin, origin)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_spec_from_loader_is_package_false(self):
        spec = self.util.spec_from_loader(self.name, self.loader,
                                          is_package=False)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_spec_from_loader_is_package_true(self):
        spec = self.util.spec_from_loader(self.name, self.loader,
                                          is_package=True)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertEqual(spec.submodule_search_locations, [])
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_spec_from_loader_origin_and_is_package(self):
        origin = 'where the streets have no name'
        spec = self.util.spec_from_loader(self.name, self.loader,
                                          origin=origin, is_package=True)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertIs(spec.origin, origin)
        self.assertIs(spec.loader_state, None)
        self.assertEqual(spec.submodule_search_locations, [])
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_spec_from_loader_is_package_with_loader_false(self):
        loader = TestLoader(is_package=False)
        spec = self.util.spec_from_loader(self.name, loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_spec_from_loader_is_package_with_loader_true(self):
        loader = TestLoader(is_package=True)
        spec = self.util.spec_from_loader(self.name, loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, loader)
        self.assertIs(spec.origin, None)
        self.assertIs(spec.loader_state, None)
        self.assertEqual(spec.submodule_search_locations, [])
        self.assertIs(spec.cached, None)
        self.assertFalse(spec.has_location)

    def test_spec_from_loader_default_with_file_loader(self):
        spec = self.util.spec_from_loader(self.name, self.fileloader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.fileloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_loader_is_package_false_with_fileloader(self):
        spec = self.util.spec_from_loader(self.name, self.fileloader,
                                          is_package=False)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.fileloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_loader_is_package_true_with_fileloader(self):
        spec = self.util.spec_from_loader(self.name, self.fileloader,
                                          is_package=True)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.fileloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertEqual(spec.submodule_search_locations, [''])
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    # spec_from_file_location()

    def test_spec_from_file_location_default(self):
        if self.machinery is source_machinery:
            raise unittest.SkipTest('not sure why this is breaking...')
        spec = self.util.spec_from_file_location(self.name, self.path)

        self.assertEqual(spec.name, self.name)
        self.assertIsInstance(spec.loader,
                              self.machinery.SourceFileLoader)
        self.assertEqual(spec.loader.name, self.name)
        self.assertEqual(spec.loader.path, self.path)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_default_without_location(self):
        spec = self.util.spec_from_file_location(self.name)

        self.assertIs(spec, None)

    def test_spec_from_file_location_default_bad_suffix(self):
        spec = self.util.spec_from_file_location(self.name, 'spam.eggs')

        self.assertIs(spec, None)

    def test_spec_from_file_location_loader_no_location(self):
        spec = self.util.spec_from_file_location(self.name,
                                                 loader=self.fileloader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.fileloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_loader_no_location_no_get_filename(self):
        spec = self.util.spec_from_file_location(self.name,
                                                 loader=self.loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.loader)
        self.assertEqual(spec.origin, '<unknown>')
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_loader_no_location_bad_get_filename(self):
        class Loader:
            def get_filename(self, name):
                raise ImportError
        loader = Loader()
        spec = self.util.spec_from_file_location(self.name, loader=loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, loader)
        self.assertEqual(spec.origin, '<unknown>')
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertIs(spec.cached, None)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_smsl_none(self):
        spec = self.util.spec_from_file_location(self.name, self.path,
                                       loader=self.fileloader,
                                       submodule_search_locations=None)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.fileloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_smsl_empty(self):
        spec = self.util.spec_from_file_location(self.name, self.path,
                                       loader=self.fileloader,
                                       submodule_search_locations=[])

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.fileloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertEqual(spec.submodule_search_locations, [''])
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_smsl_not_empty(self):
        spec = self.util.spec_from_file_location(self.name, self.path,
                                       loader=self.fileloader,
                                       submodule_search_locations=['eggs'])

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.fileloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertEqual(spec.submodule_search_locations, ['eggs'])
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_smsl_default(self):
        spec = self.util.spec_from_file_location(self.name, self.path,
                                       loader=self.pkgloader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.pkgloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertEqual(spec.submodule_search_locations, [''])
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_smsl_default_not_package(self):
        class Loader:
            def is_package(self, name):
                return False
        loader = Loader()
        spec = self.util.spec_from_file_location(self.name, self.path,
                                                 loader=loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, loader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_smsl_default_no_is_package(self):
        spec = self.util.spec_from_file_location(self.name, self.path,
                                       loader=self.fileloader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, self.fileloader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)

    def test_spec_from_file_location_smsl_default_bad_is_package(self):
        class Loader:
            def is_package(self, name):
                raise ImportError
        loader = Loader()
        spec = self.util.spec_from_file_location(self.name, self.path,
                                                 loader=loader)

        self.assertEqual(spec.name, self.name)
        self.assertEqual(spec.loader, loader)
        self.assertEqual(spec.origin, self.path)
        self.assertIs(spec.loader_state, None)
        self.assertIs(spec.submodule_search_locations, None)
        self.assertEqual(spec.cached, self.cached)
        self.assertTrue(spec.has_location)


class Frozen_FactoryTests(FactoryTests, unittest.TestCase):
    util = frozen_util
    machinery = frozen_machinery


class Source_FactoryTests(FactoryTests, unittest.TestCase):
    util = source_util
    machinery = source_machinery
