from importlib import util
from . import util as test_util
frozen_init, source_init = test_util.import_importlib('importlib')
frozen_machinery, source_machinery = test_util.import_importlib('importlib.machinery')
frozen_util, source_util = test_util.import_importlib('importlib.util')

import os
import sys
from test import support
import types
import unittest
import warnings


class DecodeSourceBytesTests:

    source = "string ='ü'"

    def test_ut8_default(self):
        source_bytes = self.source.encode('utf-8')
        self.assertEqual(self.util.decode_source(source_bytes), self.source)

    def test_specified_encoding(self):
        source = '# coding=latin-1\n' + self.source
        source_bytes = source.encode('latin-1')
        assert source_bytes != source.encode('utf-8')
        self.assertEqual(self.util.decode_source(source_bytes), source)

    def test_universal_newlines(self):
        source = '\r\n'.join([self.source, self.source])
        source_bytes = source.encode('utf-8')
        self.assertEqual(self.util.decode_source(source_bytes),
                         '\n'.join([self.source, self.source]))

Frozen_DecodeSourceBytesTests, Source_DecodeSourceBytesTests = test_util.test_both(
        DecodeSourceBytesTests, util=[frozen_util, source_util])


class ModuleForLoaderTests:

    """Tests for importlib.util.module_for_loader."""

    @classmethod
    def module_for_loader(cls, func):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            return cls.util.module_for_loader(func)

    def test_warning(self):
        # Should raise a PendingDeprecationWarning when used.
        with warnings.catch_warnings():
            warnings.simplefilter('error', DeprecationWarning)
            with self.assertRaises(DeprecationWarning):
                func = self.util.module_for_loader(lambda x: x)

    def return_module(self, name):
        fxn = self.module_for_loader(lambda self, module: module)
        return fxn(self, name)

    def raise_exception(self, name):
        def to_wrap(self, module):
            raise ImportError
        fxn = self.module_for_loader(to_wrap)
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
        class FakeLoader:
            def is_package(self, name):
                return True
            @self.module_for_loader
            def load_module(self, module):
                return module
        name = 'a.b.c'
        module = types.ModuleType('a.b.c')
        module.__loader__ = 42
        module.__package__ = 42
        with test_util.uncache(name):
            sys.modules[name] = module
            loader = FakeLoader()
            returned_module = loader.load_module(name)
            self.assertIs(returned_module, sys.modules[name])
            self.assertEqual(module.__loader__, loader)
            self.assertEqual(module.__package__, name)

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
        module = types.ModuleType(name)
        with test_util.uncache(name):
            sys.modules[name] = module
            self.raise_exception(name)
            self.assertIs(module, sys.modules[name])

    def test_decorator_attrs(self):
        def fxn(self, module): pass
        wrapped = self.module_for_loader(fxn)
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
            @self.module_for_loader
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

Frozen_ModuleForLoaderTests, Source_ModuleForLoaderTests = test_util.test_both(
        ModuleForLoaderTests, util=[frozen_util, source_util])


class SetPackageTests:

    """Tests for importlib.util.set_package."""

    def verify(self, module, expect):
        """Verify the module has the expected value for __package__ after
        passing through set_package."""
        fxn = lambda: module
        wrapped = self.util.set_package(fxn)
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            wrapped()
        self.assertTrue(hasattr(module, '__package__'))
        self.assertEqual(expect, module.__package__)

    def test_top_level(self):
        # __package__ should be set to the empty string if a top-level module.
        # Implicitly tests when package is set to None.
        module = types.ModuleType('module')
        module.__package__ = None
        self.verify(module, '')

    def test_package(self):
        # Test setting __package__ for a package.
        module = types.ModuleType('pkg')
        module.__path__ = ['<path>']
        module.__package__ = None
        self.verify(module, 'pkg')

    def test_submodule(self):
        # Test __package__ for a module in a package.
        module = types.ModuleType('pkg.mod')
        module.__package__ = None
        self.verify(module, 'pkg')

    def test_setting_if_missing(self):
        # __package__ should be set if it is missing.
        module = types.ModuleType('mod')
        if hasattr(module, '__package__'):
            delattr(module, '__package__')
        self.verify(module, '')

    def test_leaving_alone(self):
        # If __package__ is set and not None then leave it alone.
        for value in (True, False):
            module = types.ModuleType('mod')
            module.__package__ = value
            self.verify(module, value)

    def test_decorator_attrs(self):
        def fxn(module): pass
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            wrapped = self.util.set_package(fxn)
        self.assertEqual(wrapped.__name__, fxn.__name__)
        self.assertEqual(wrapped.__qualname__, fxn.__qualname__)

Frozen_SetPackageTests, Source_SetPackageTests = test_util.test_both(
        SetPackageTests, util=[frozen_util, source_util])


class SetLoaderTests:

    """Tests importlib.util.set_loader()."""

    class DummyLoader:
        @util.set_loader
        def load_module(self, module):
            return self.module

    def test_no_attribute(self):
        loader = self.DummyLoader()
        loader.module = types.ModuleType('blah')
        try:
            del loader.module.__loader__
        except AttributeError:
            pass
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            self.assertEqual(loader, loader.load_module('blah').__loader__)

    def test_attribute_is_None(self):
        loader = self.DummyLoader()
        loader.module = types.ModuleType('blah')
        loader.module.__loader__ = None
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            self.assertEqual(loader, loader.load_module('blah').__loader__)

    def test_not_reset(self):
        loader = self.DummyLoader()
        loader.module = types.ModuleType('blah')
        loader.module.__loader__ = 42
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            self.assertEqual(42, loader.load_module('blah').__loader__)

class Frozen_SetLoaderTests(SetLoaderTests, unittest.TestCase):
    class DummyLoader:
        @frozen_util.set_loader
        def load_module(self, module):
            return self.module

class Source_SetLoaderTests(SetLoaderTests, unittest.TestCase):
    class DummyLoader:
        @source_util.set_loader
        def load_module(self, module):
            return self.module


class ResolveNameTests:

    """Tests importlib.util.resolve_name()."""

    def test_absolute(self):
        # bacon
        self.assertEqual('bacon', self.util.resolve_name('bacon', None))

    def test_aboslute_within_package(self):
        # bacon in spam
        self.assertEqual('bacon', self.util.resolve_name('bacon', 'spam'))

    def test_no_package(self):
        # .bacon in ''
        with self.assertRaises(ValueError):
            self.util.resolve_name('.bacon', '')

    def test_in_package(self):
        # .bacon in spam
        self.assertEqual('spam.eggs.bacon',
                         self.util.resolve_name('.bacon', 'spam.eggs'))

    def test_other_package(self):
        # ..bacon in spam.bacon
        self.assertEqual('spam.bacon',
                         self.util.resolve_name('..bacon', 'spam.eggs'))

    def test_escape(self):
        # ..bacon in spam
        with self.assertRaises(ValueError):
            self.util.resolve_name('..bacon', 'spam')

Frozen_ResolveNameTests, Source_ResolveNameTests = test_util.test_both(
        ResolveNameTests,
        util=[frozen_util, source_util])


class FindSpecTests:

    class FakeMetaFinder:
        @staticmethod
        def find_spec(name, path=None, target=None): return name, path, target

    def test_sys_modules(self):
        name = 'some_mod'
        with test_util.uncache(name):
            module = types.ModuleType(name)
            loader = 'a loader!'
            spec = self.machinery.ModuleSpec(name, loader)
            module.__loader__ = loader
            module.__spec__ = spec
            sys.modules[name] = module
            found = self.util.find_spec(name)
            self.assertEqual(found, spec)

    def test_sys_modules_without___loader__(self):
        name = 'some_mod'
        with test_util.uncache(name):
            module = types.ModuleType(name)
            del module.__loader__
            loader = 'a loader!'
            spec = self.machinery.ModuleSpec(name, loader)
            module.__spec__ = spec
            sys.modules[name] = module
            found = self.util.find_spec(name)
            self.assertEqual(found, spec)

    def test_sys_modules_spec_is_None(self):
        name = 'some_mod'
        with test_util.uncache(name):
            module = types.ModuleType(name)
            module.__spec__ = None
            sys.modules[name] = module
            with self.assertRaises(ValueError):
                self.util.find_spec(name)

    def test_sys_modules_loader_is_None(self):
        name = 'some_mod'
        with test_util.uncache(name):
            module = types.ModuleType(name)
            spec = self.machinery.ModuleSpec(name, None)
            module.__spec__ = spec
            sys.modules[name] = module
            found = self.util.find_spec(name)
            self.assertEqual(found, spec)

    def test_sys_modules_spec_is_not_set(self):
        name = 'some_mod'
        with test_util.uncache(name):
            module = types.ModuleType(name)
            try:
                del module.__spec__
            except AttributeError:
                pass
            sys.modules[name] = module
            with self.assertRaises(ValueError):
                self.util.find_spec(name)

    def test_success(self):
        name = 'some_mod'
        with test_util.uncache(name):
            with test_util.import_state(meta_path=[self.FakeMetaFinder]):
                self.assertEqual((name, None, None),
                                 self.util.find_spec(name))

#    def test_success_path(self):
#        # Searching on a path should work.
#        name = 'some_mod'
#        path = 'path to some place'
#        with test_util.uncache(name):
#            with test_util.import_state(meta_path=[self.FakeMetaFinder]):
#                self.assertEqual((name, path, None),
#                                 self.util.find_spec(name, path))

    def test_nothing(self):
        # None is returned upon failure to find a loader.
        self.assertIsNone(self.util.find_spec('nevergoingtofindthismodule'))

    def test_find_submodule(self):
        name = 'spam'
        subname = 'ham'
        with test_util.temp_module(name, pkg=True) as pkg_dir:
            fullname, _ = test_util.submodule(name, subname, pkg_dir)
            spec = self.util.find_spec(fullname)
            self.assertIsNot(spec, None)
            self.assertIn(name, sorted(sys.modules))
            self.assertNotIn(fullname, sorted(sys.modules))
            # Ensure successive calls behave the same.
            spec_again = self.util.find_spec(fullname)
            self.assertEqual(spec_again, spec)

    def test_find_submodule_parent_already_imported(self):
        name = 'spam'
        subname = 'ham'
        with test_util.temp_module(name, pkg=True) as pkg_dir:
            self.init.import_module(name)
            fullname, _ = test_util.submodule(name, subname, pkg_dir)
            spec = self.util.find_spec(fullname)
            self.assertIsNot(spec, None)
            self.assertIn(name, sorted(sys.modules))
            self.assertNotIn(fullname, sorted(sys.modules))
            # Ensure successive calls behave the same.
            spec_again = self.util.find_spec(fullname)
            self.assertEqual(spec_again, spec)

    def test_find_relative_module(self):
        name = 'spam'
        subname = 'ham'
        with test_util.temp_module(name, pkg=True) as pkg_dir:
            fullname, _ = test_util.submodule(name, subname, pkg_dir)
            relname = '.' + subname
            spec = self.util.find_spec(relname, name)
            self.assertIsNot(spec, None)
            self.assertIn(name, sorted(sys.modules))
            self.assertNotIn(fullname, sorted(sys.modules))
            # Ensure successive calls behave the same.
            spec_again = self.util.find_spec(fullname)
            self.assertEqual(spec_again, spec)

    def test_find_relative_module_missing_package(self):
        name = 'spam'
        subname = 'ham'
        with test_util.temp_module(name, pkg=True) as pkg_dir:
            fullname, _ = test_util.submodule(name, subname, pkg_dir)
            relname = '.' + subname
            with self.assertRaises(ValueError):
                self.util.find_spec(relname)
            self.assertNotIn(name, sorted(sys.modules))
            self.assertNotIn(fullname, sorted(sys.modules))


class Frozen_FindSpecTests(FindSpecTests, unittest.TestCase):
    init = frozen_init
    machinery = frozen_machinery
    util = frozen_util

class Source_FindSpecTests(FindSpecTests, unittest.TestCase):
    init = source_init
    machinery = source_machinery
    util = source_util


class MagicNumberTests:

    def test_length(self):
        # Should be 4 bytes.
        self.assertEqual(len(self.util.MAGIC_NUMBER), 4)

    def test_incorporates_rn(self):
        # The magic number uses \r\n to come out wrong when splitting on lines.
        self.assertTrue(self.util.MAGIC_NUMBER.endswith(b'\r\n'))

Frozen_MagicNumberTests, Source_MagicNumberTests = test_util.test_both(
        MagicNumberTests, util=[frozen_util, source_util])


class PEP3147Tests:

    """Tests of PEP 3147-related functions: cache_from_source and source_from_cache."""

    tag = sys.implementation.cache_tag

    @unittest.skipUnless(sys.implementation.cache_tag is not None,
                         'requires sys.implementation.cache_tag not be None')
    def test_cache_from_source(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyc file (i.e. under __pycache__).
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        expect = os.path.join('foo', 'bar', 'baz', '__pycache__',
                              'qux.{}.pyc'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, True), expect)

    def test_cache_from_source_no_cache_tag(self):
        # No cache tag means NotImplementedError.
        with support.swap_attr(sys.implementation, 'cache_tag', None):
            with self.assertRaises(NotImplementedError):
                self.util.cache_from_source('whatever.py')

    def test_cache_from_source_no_dot(self):
        # Directory with a dot, filename without dot.
        path = os.path.join('foo.bar', 'file')
        expect = os.path.join('foo.bar', '__pycache__',
                              'file{}.pyc'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, True), expect)

    def test_cache_from_source_optimized(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyo file (i.e. under __pycache__).
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        expect = os.path.join('foo', 'bar', 'baz', '__pycache__',
                              'qux.{}.pyo'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, False), expect)

    def test_cache_from_source_cwd(self):
        path = 'foo.py'
        expect = os.path.join('__pycache__', 'foo.{}.pyc'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, True), expect)

    def test_cache_from_source_override(self):
        # When debug_override is not None, it can be any true-ish or false-ish
        # value.
        path = os.path.join('foo', 'bar', 'baz.py')
        partial_expect = os.path.join('foo', 'bar', '__pycache__',
                                      'baz.{}.py'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, []), partial_expect + 'o')
        self.assertEqual(self.util.cache_from_source(path, [17]),
                         partial_expect + 'c')
        # However if the bool-ishness can't be determined, the exception
        # propagates.
        class Bearish:
            def __bool__(self): raise RuntimeError
        with self.assertRaises(RuntimeError):
            self.util.cache_from_source('/foo/bar/baz.py', Bearish())

    @unittest.skipUnless(os.sep == '\\' and os.altsep == '/',
                     'test meaningful only where os.altsep is defined')
    def test_sep_altsep_and_sep_cache_from_source(self):
        # Windows path and PEP 3147 where sep is right of altsep.
        self.assertEqual(
            self.util.cache_from_source('\\foo\\bar\\baz/qux.py', True),
            '\\foo\\bar\\baz\\__pycache__\\qux.{}.pyc'.format(self.tag))

    @unittest.skipUnless(sys.implementation.cache_tag is not None,
                         'requires sys.implementation.cache_tag to not be '
                         'None')
    def test_source_from_cache(self):
        # Given the path to a PEP 3147 defined .pyc file, return the path to
        # its source.  This tests the good path.
        path = os.path.join('foo', 'bar', 'baz', '__pycache__',
                            'qux.{}.pyc'.format(self.tag))
        expect = os.path.join('foo', 'bar', 'baz', 'qux.py')
        self.assertEqual(self.util.source_from_cache(path), expect)

    def test_source_from_cache_no_cache_tag(self):
        # If sys.implementation.cache_tag is None, raise NotImplementedError.
        path = os.path.join('blah', '__pycache__', 'whatever.pyc')
        with support.swap_attr(sys.implementation, 'cache_tag', None):
            with self.assertRaises(NotImplementedError):
                self.util.source_from_cache(path)

    def test_source_from_cache_bad_path(self):
        # When the path to a pyc file is not in PEP 3147 format, a ValueError
        # is raised.
        self.assertRaises(
            ValueError, self.util.source_from_cache, '/foo/bar/bazqux.pyc')

    def test_source_from_cache_no_slash(self):
        # No slashes at all in path -> ValueError
        self.assertRaises(
            ValueError, self.util.source_from_cache, 'foo.cpython-32.pyc')

    def test_source_from_cache_too_few_dots(self):
        # Too few dots in final path component -> ValueError
        self.assertRaises(
            ValueError, self.util.source_from_cache, '__pycache__/foo.pyc')

    def test_source_from_cache_too_many_dots(self):
        # Too many dots in final path component -> ValueError
        self.assertRaises(
            ValueError, self.util.source_from_cache,
            '__pycache__/foo.cpython-32.foo.pyc')

    def test_source_from_cache_no__pycache__(self):
        # Another problem with the path -> ValueError
        self.assertRaises(
            ValueError, self.util.source_from_cache,
            '/foo/bar/foo.cpython-32.foo.pyc')

Frozen_PEP3147Tests, Source_PEP3147Tests = test_util.test_both(
        PEP3147Tests,
        util=[frozen_util, source_util])


if __name__ == '__main__':
    unittest.main()
