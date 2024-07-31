from test.test_importlib import util

abc = util.import_importlib('importlib.abc')
init = util.import_importlib('importlib')
machinery = util.import_importlib('importlib.machinery')
importlib_util = util.import_importlib('importlib.util')

import importlib.util
import os
import pathlib
import re
import string
import sys
from test import support
import textwrap
import types
import unittest
import unittest.mock
import warnings

try:
    import _testsinglephase
except ImportError:
    _testsinglephase = None
try:
    import _testmultiphase
except ImportError:
    _testmultiphase = None
try:
    import _interpreters
except ModuleNotFoundError:
    _interpreters = None


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


(Frozen_DecodeSourceBytesTests,
 Source_DecodeSourceBytesTests
 ) = util.test_both(DecodeSourceBytesTests, util=importlib_util)


class ModuleFromSpecTests:

    def test_no_create_module(self):
        class Loader:
            def exec_module(self, module):
                pass
        spec = self.machinery.ModuleSpec('test', Loader())
        with self.assertRaises(ImportError):
            module = self.util.module_from_spec(spec)

    def test_create_module_returns_None(self):
        class Loader(self.abc.Loader):
            def create_module(self, spec):
                return None
        spec = self.machinery.ModuleSpec('test', Loader())
        module = self.util.module_from_spec(spec)
        self.assertIsInstance(module, types.ModuleType)
        self.assertEqual(module.__name__, spec.name)

    def test_create_module(self):
        name = 'already set'
        class CustomModule(types.ModuleType):
            pass
        class Loader(self.abc.Loader):
            def create_module(self, spec):
                module = CustomModule(spec.name)
                module.__name__ = name
                return module
        spec = self.machinery.ModuleSpec('test', Loader())
        module = self.util.module_from_spec(spec)
        self.assertIsInstance(module, CustomModule)
        self.assertEqual(module.__name__, name)

    def test___name__(self):
        spec = self.machinery.ModuleSpec('test', object())
        module = self.util.module_from_spec(spec)
        self.assertEqual(module.__name__, spec.name)

    def test___spec__(self):
        spec = self.machinery.ModuleSpec('test', object())
        module = self.util.module_from_spec(spec)
        self.assertEqual(module.__spec__, spec)

    def test___loader__(self):
        loader = object()
        spec = self.machinery.ModuleSpec('test', loader)
        module = self.util.module_from_spec(spec)
        self.assertIs(module.__loader__, loader)

    def test___package__(self):
        spec = self.machinery.ModuleSpec('test.pkg', object())
        module = self.util.module_from_spec(spec)
        self.assertEqual(module.__package__, spec.parent)

    def test___path__(self):
        spec = self.machinery.ModuleSpec('test', object(), is_package=True)
        module = self.util.module_from_spec(spec)
        self.assertEqual(module.__path__, spec.submodule_search_locations)

    def test___file__(self):
        spec = self.machinery.ModuleSpec('test', object(), origin='some/path')
        spec.has_location = True
        module = self.util.module_from_spec(spec)
        self.assertEqual(module.__file__, spec.origin)

    def test___cached__(self):
        spec = self.machinery.ModuleSpec('test', object())
        spec.cached = 'some/path'
        spec.has_location = True
        module = self.util.module_from_spec(spec)
        self.assertEqual(module.__cached__, spec.cached)

(Frozen_ModuleFromSpecTests,
 Source_ModuleFromSpecTests
) = util.test_both(ModuleFromSpecTests, abc=abc, machinery=machinery,
                   util=importlib_util)


class ResolveNameTests:

    """Tests importlib.util.resolve_name()."""

    def test_absolute(self):
        # bacon
        self.assertEqual('bacon', self.util.resolve_name('bacon', None))

    def test_absolute_within_package(self):
        # bacon in spam
        self.assertEqual('bacon', self.util.resolve_name('bacon', 'spam'))

    def test_no_package(self):
        # .bacon in ''
        with self.assertRaises(ImportError):
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
        with self.assertRaises(ImportError):
            self.util.resolve_name('..bacon', 'spam')


(Frozen_ResolveNameTests,
 Source_ResolveNameTests
 ) = util.test_both(ResolveNameTests, util=importlib_util)


class FindSpecTests:

    class FakeMetaFinder:
        @staticmethod
        def find_spec(name, path=None, target=None): return name, path, target

    def test_sys_modules(self):
        name = 'some_mod'
        with util.uncache(name):
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
        with util.uncache(name):
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
        with util.uncache(name):
            module = types.ModuleType(name)
            module.__spec__ = None
            sys.modules[name] = module
            with self.assertRaises(ValueError):
                self.util.find_spec(name)

    def test_sys_modules_loader_is_None(self):
        name = 'some_mod'
        with util.uncache(name):
            module = types.ModuleType(name)
            spec = self.machinery.ModuleSpec(name, None)
            module.__spec__ = spec
            sys.modules[name] = module
            found = self.util.find_spec(name)
            self.assertEqual(found, spec)

    def test_sys_modules_spec_is_not_set(self):
        name = 'some_mod'
        with util.uncache(name):
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
        with util.uncache(name):
            with util.import_state(meta_path=[self.FakeMetaFinder]):
                self.assertEqual((name, None, None),
                                 self.util.find_spec(name))

    def test_nothing(self):
        # None is returned upon failure to find a loader.
        self.assertIsNone(self.util.find_spec('nevergoingtofindthismodule'))

    def test_find_submodule(self):
        name = 'spam'
        subname = 'ham'
        with util.temp_module(name, pkg=True) as pkg_dir:
            fullname, _ = util.submodule(name, subname, pkg_dir)
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
        with util.temp_module(name, pkg=True) as pkg_dir:
            self.init.import_module(name)
            fullname, _ = util.submodule(name, subname, pkg_dir)
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
        with util.temp_module(name, pkg=True) as pkg_dir:
            fullname, _ = util.submodule(name, subname, pkg_dir)
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
        with util.temp_module(name, pkg=True) as pkg_dir:
            fullname, _ = util.submodule(name, subname, pkg_dir)
            relname = '.' + subname
            with self.assertRaises(ImportError):
                self.util.find_spec(relname)
            self.assertNotIn(name, sorted(sys.modules))
            self.assertNotIn(fullname, sorted(sys.modules))

    def test_find_submodule_in_module(self):
        # ModuleNotFoundError raised when a module is specified as
        # a parent instead of a package.
        with self.assertRaises(ModuleNotFoundError):
            self.util.find_spec('module.name')


(Frozen_FindSpecTests,
 Source_FindSpecTests
 ) = util.test_both(FindSpecTests, init=init, util=importlib_util,
                         machinery=machinery)


class MagicNumberTests:

    def test_length(self):
        # Should be 4 bytes.
        self.assertEqual(len(self.util.MAGIC_NUMBER), 4)

    def test_incorporates_rn(self):
        # The magic number uses \r\n to come out wrong when splitting on lines.
        self.assertTrue(self.util.MAGIC_NUMBER.endswith(b'\r\n'))


(Frozen_MagicNumberTests,
 Source_MagicNumberTests
 ) = util.test_both(MagicNumberTests, util=importlib_util)


class PEP3147Tests:

    """Tests of PEP 3147-related functions: cache_from_source and source_from_cache."""

    tag = sys.implementation.cache_tag

    @unittest.skipIf(sys.implementation.cache_tag is None,
                     'requires sys.implementation.cache_tag not be None')
    def test_cache_from_source(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyc file (i.e. under __pycache__).
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        expect = os.path.join('foo', 'bar', 'baz', '__pycache__',
                              'qux.{}.pyc'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, optimization=''),
                         expect)

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
        self.assertEqual(self.util.cache_from_source(path, optimization=''),
                         expect)

    def test_cache_from_source_debug_override(self):
        # Given the path to a .py file, return the path to its PEP 3147/PEP 488
        # defined .pyc file (i.e. under __pycache__).
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')
            self.assertEqual(self.util.cache_from_source(path, False),
                             self.util.cache_from_source(path, optimization=1))
            self.assertEqual(self.util.cache_from_source(path, True),
                             self.util.cache_from_source(path, optimization=''))
        with warnings.catch_warnings():
            warnings.simplefilter('error')
            with self.assertRaises(DeprecationWarning):
                self.util.cache_from_source(path, False)
            with self.assertRaises(DeprecationWarning):
                self.util.cache_from_source(path, True)

    def test_cache_from_source_cwd(self):
        path = 'foo.py'
        expect = os.path.join('__pycache__', 'foo.{}.pyc'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, optimization=''),
                         expect)

    def test_cache_from_source_override(self):
        # When debug_override is not None, it can be any true-ish or false-ish
        # value.
        path = os.path.join('foo', 'bar', 'baz.py')
        # However if the bool-ishness can't be determined, the exception
        # propagates.
        class Bearish:
            def __bool__(self): raise RuntimeError
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')
            self.assertEqual(self.util.cache_from_source(path, []),
                             self.util.cache_from_source(path, optimization=1))
            self.assertEqual(self.util.cache_from_source(path, [17]),
                             self.util.cache_from_source(path, optimization=''))
            with self.assertRaises(RuntimeError):
                self.util.cache_from_source('/foo/bar/baz.py', Bearish())


    def test_cache_from_source_optimization_empty_string(self):
        # Setting 'optimization' to '' leads to no optimization tag (PEP 488).
        path = 'foo.py'
        expect = os.path.join('__pycache__', 'foo.{}.pyc'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, optimization=''),
                         expect)

    def test_cache_from_source_optimization_None(self):
        # Setting 'optimization' to None uses the interpreter's optimization.
        # (PEP 488)
        path = 'foo.py'
        optimization_level = sys.flags.optimize
        almost_expect = os.path.join('__pycache__', 'foo.{}'.format(self.tag))
        if optimization_level == 0:
            expect = almost_expect + '.pyc'
        elif optimization_level <= 2:
            expect = almost_expect + '.opt-{}.pyc'.format(optimization_level)
        else:
            msg = '{!r} is a non-standard optimization level'.format(optimization_level)
            self.skipTest(msg)
        self.assertEqual(self.util.cache_from_source(path, optimization=None),
                         expect)

    def test_cache_from_source_optimization_set(self):
        # The 'optimization' parameter accepts anything that has a string repr
        # that passes str.alnum().
        path = 'foo.py'
        valid_characters = string.ascii_letters + string.digits
        almost_expect = os.path.join('__pycache__', 'foo.{}'.format(self.tag))
        got = self.util.cache_from_source(path, optimization=valid_characters)
        # Test all valid characters are accepted.
        self.assertEqual(got,
                         almost_expect + '.opt-{}.pyc'.format(valid_characters))
        # str() should be called on argument.
        self.assertEqual(self.util.cache_from_source(path, optimization=42),
                         almost_expect + '.opt-42.pyc')
        # Invalid characters raise ValueError.
        with self.assertRaises(ValueError):
            self.util.cache_from_source(path, optimization='path/is/bad')

    def test_cache_from_source_debug_override_optimization_both_set(self):
        # Can only set one of the optimization-related parameters.
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')
            with self.assertRaises(TypeError):
                self.util.cache_from_source('foo.py', False, optimization='')

    @unittest.skipUnless(os.sep == '\\' and os.altsep == '/',
                     'test meaningful only where os.altsep is defined')
    def test_sep_altsep_and_sep_cache_from_source(self):
        # Windows path and PEP 3147 where sep is right of altsep.
        self.assertEqual(
            self.util.cache_from_source('\\foo\\bar\\baz/qux.py', optimization=''),
            '\\foo\\bar\\baz\\__pycache__\\qux.{}.pyc'.format(self.tag))

    @unittest.skipIf(sys.implementation.cache_tag is None,
                     'requires sys.implementation.cache_tag not be None')
    def test_cache_from_source_path_like_arg(self):
        path = pathlib.PurePath('foo', 'bar', 'baz', 'qux.py')
        expect = os.path.join('foo', 'bar', 'baz', '__pycache__',
                              'qux.{}.pyc'.format(self.tag))
        self.assertEqual(self.util.cache_from_source(path, optimization=''),
                         expect)

    @unittest.skipIf(sys.implementation.cache_tag is None,
                     'requires sys.implementation.cache_tag to not be None')
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
        with self.assertRaises(ValueError):
            self.util.source_from_cache(
                    '__pycache__/foo.cpython-32.opt-1.foo.pyc')

    def test_source_from_cache_not_opt(self):
        # Non-`opt-` path component -> ValueError
        self.assertRaises(
            ValueError, self.util.source_from_cache,
            '__pycache__/foo.cpython-32.foo.pyc')

    def test_source_from_cache_no__pycache__(self):
        # Another problem with the path -> ValueError
        self.assertRaises(
            ValueError, self.util.source_from_cache,
            '/foo/bar/foo.cpython-32.foo.pyc')

    def test_source_from_cache_optimized_bytecode(self):
        # Optimized bytecode is not an issue.
        path = os.path.join('__pycache__', 'foo.{}.opt-1.pyc'.format(self.tag))
        self.assertEqual(self.util.source_from_cache(path), 'foo.py')

    def test_source_from_cache_missing_optimization(self):
        # An empty optimization level is a no-no.
        path = os.path.join('__pycache__', 'foo.{}.opt-.pyc'.format(self.tag))
        with self.assertRaises(ValueError):
            self.util.source_from_cache(path)

    @unittest.skipIf(sys.implementation.cache_tag is None,
                     'requires sys.implementation.cache_tag to not be None')
    def test_source_from_cache_path_like_arg(self):
        path = pathlib.PurePath('foo', 'bar', 'baz', '__pycache__',
                                'qux.{}.pyc'.format(self.tag))
        expect = os.path.join('foo', 'bar', 'baz', 'qux.py')
        self.assertEqual(self.util.source_from_cache(path), expect)

    @unittest.skipIf(sys.implementation.cache_tag is None,
                     'requires sys.implementation.cache_tag to not be None')
    def test_cache_from_source_respects_pycache_prefix(self):
        # If pycache_prefix is set, cache_from_source will return a bytecode
        # path inside that directory (in a subdirectory mirroring the .py file's
        # path) rather than in a __pycache__ dir next to the py file.
        pycache_prefixes = [
            os.path.join(os.path.sep, 'tmp', 'bytecode'),
            os.path.join(os.path.sep, 'tmp', '\u2603'),  # non-ASCII in path!
            os.path.join(os.path.sep, 'tmp', 'trailing-slash') + os.path.sep,
        ]
        drive = ''
        if os.name == 'nt':
            drive = 'C:'
            pycache_prefixes = [
                f'{drive}{prefix}' for prefix in pycache_prefixes]
            pycache_prefixes += [r'\\?\C:\foo', r'\\localhost\c$\bar']
        for pycache_prefix in pycache_prefixes:
            with self.subTest(path=pycache_prefix):
                path = drive + os.path.join(
                    os.path.sep, 'foo', 'bar', 'baz', 'qux.py')
                expect = os.path.join(
                    pycache_prefix, 'foo', 'bar', 'baz',
                    'qux.{}.pyc'.format(self.tag))
                with util.temporary_pycache_prefix(pycache_prefix):
                    self.assertEqual(
                        self.util.cache_from_source(path, optimization=''),
                        expect)

    @unittest.skipIf(sys.implementation.cache_tag is None,
                     'requires sys.implementation.cache_tag to not be None')
    def test_cache_from_source_respects_pycache_prefix_relative(self):
        # If the .py path we are given is relative, we will resolve to an
        # absolute path before prefixing with pycache_prefix, to avoid any
        # possible ambiguity.
        pycache_prefix = os.path.join(os.path.sep, 'tmp', 'bytecode')
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        root = os.path.splitdrive(os.getcwd())[0] + os.path.sep
        expect = os.path.join(
            pycache_prefix,
            os.path.relpath(os.getcwd(), root),
            'foo', 'bar', 'baz', f'qux.{self.tag}.pyc')
        with util.temporary_pycache_prefix(pycache_prefix):
            self.assertEqual(
                self.util.cache_from_source(path, optimization=''),
                os.path.normpath(expect))

    @unittest.skipIf(sys.implementation.cache_tag is None,
                     'requires sys.implementation.cache_tag to not be None')
    def test_source_from_cache_inside_pycache_prefix(self):
        # If pycache_prefix is set and the cache path we get is inside it,
        # we return an absolute path to the py file based on the remainder of
        # the path within pycache_prefix.
        pycache_prefix = os.path.join(os.path.sep, 'tmp', 'bytecode')
        path = os.path.join(pycache_prefix, 'foo', 'bar', 'baz',
                            f'qux.{self.tag}.pyc')
        expect = os.path.join(os.path.sep, 'foo', 'bar', 'baz', 'qux.py')
        with util.temporary_pycache_prefix(pycache_prefix):
            self.assertEqual(self.util.source_from_cache(path), expect)

    @unittest.skipIf(sys.implementation.cache_tag is None,
                     'requires sys.implementation.cache_tag to not be None')
    def test_source_from_cache_outside_pycache_prefix(self):
        # If pycache_prefix is set but the cache path we get is not inside
        # it, just ignore it and handle the cache path according to the default
        # behavior.
        pycache_prefix = os.path.join(os.path.sep, 'tmp', 'bytecode')
        path = os.path.join('foo', 'bar', 'baz', '__pycache__',
                            f'qux.{self.tag}.pyc')
        expect = os.path.join('foo', 'bar', 'baz', 'qux.py')
        with util.temporary_pycache_prefix(pycache_prefix):
            self.assertEqual(self.util.source_from_cache(path), expect)


(Frozen_PEP3147Tests,
 Source_PEP3147Tests
 ) = util.test_both(PEP3147Tests, util=importlib_util)


class MagicNumberTests(unittest.TestCase):
    """
    Test release compatibility issues relating to importlib
    """
    @unittest.skipUnless(
        sys.version_info.releaselevel in ('candidate', 'final'),
        'only applies to candidate or final python release levels'
    )
    def test_magic_number(self):
        # Each python minor release should generally have a MAGIC_NUMBER
        # that does not change once the release reaches candidate status.

        # Once a release reaches candidate status, the value of the constant
        # EXPECTED_MAGIC_NUMBER in this test should be changed.
        # This test will then check that the actual MAGIC_NUMBER matches
        # the expected value for the release.

        # In exceptional cases, it may be required to change the MAGIC_NUMBER
        # for a maintenance release. In this case the change should be
        # discussed in python-dev. If a change is required, community
        # stakeholders such as OS package maintainers must be notified
        # in advance. Such exceptional releases will then require an
        # adjustment to this test case.
        EXPECTED_MAGIC_NUMBER = 3571
        actual = int.from_bytes(importlib.util.MAGIC_NUMBER[:2], 'little')

        msg = (
            "To avoid breaking backwards compatibility with cached bytecode "
            "files that can't be automatically regenerated by the current "
            "user, candidate and final releases require the current  "
            "importlib.util.MAGIC_NUMBER to match the expected "
            "magic number in this test. Set the expected "
            "magic number in this test to the current MAGIC_NUMBER to "
            "continue with the release.\n\n"
            "Changing the MAGIC_NUMBER for a maintenance release "
            "requires discussion in python-dev and notification of "
            "community stakeholders."
        )
        self.assertEqual(EXPECTED_MAGIC_NUMBER, actual, msg)


@unittest.skipIf(_interpreters is None, 'subinterpreters required')
class IncompatibleExtensionModuleRestrictionsTests(unittest.TestCase):

    def run_with_own_gil(self, script):
        interpid = _interpreters.create('isolated')
        def ensure_destroyed():
            try:
                _interpreters.destroy(interpid)
            except _interpreters.InterpreterNotFoundError:
                pass
        self.addCleanup(ensure_destroyed)
        excsnap = _interpreters.exec(interpid, script)
        if excsnap is not None:
            if excsnap.type.__name__ == 'ImportError':
                raise ImportError(excsnap.msg)

    def run_with_shared_gil(self, script):
        interpid = _interpreters.create('legacy')
        def ensure_destroyed():
            try:
                _interpreters.destroy(interpid)
            except _interpreters.InterpreterNotFoundError:
                pass
        self.addCleanup(ensure_destroyed)
        excsnap = _interpreters.exec(interpid, script)
        if excsnap is not None:
            if excsnap.type.__name__ == 'ImportError':
                raise ImportError(excsnap.msg)

    @unittest.skipIf(_testsinglephase is None, "test requires _testsinglephase module")
    # gh-117649: single-phase init modules are not currently supported in
    # subinterpreters in the free-threaded build
    @support.expected_failure_if_gil_disabled()
    def test_single_phase_init_module(self):
        script = textwrap.dedent('''
            from importlib.util import _incompatible_extension_module_restrictions
            with _incompatible_extension_module_restrictions(disable_check=True):
                import _testsinglephase
            ''')
        with self.subTest('check disabled, shared GIL'):
            self.run_with_shared_gil(script)
        with self.subTest('check disabled, per-interpreter GIL'):
            self.run_with_own_gil(script)

        script = textwrap.dedent(f'''
            from importlib.util import _incompatible_extension_module_restrictions
            with _incompatible_extension_module_restrictions(disable_check=False):
                import _testsinglephase
            ''')
        with self.subTest('check enabled, shared GIL'):
            with self.assertRaises(ImportError):
                self.run_with_shared_gil(script)
        with self.subTest('check enabled, per-interpreter GIL'):
            with self.assertRaises(ImportError):
                self.run_with_own_gil(script)

    @unittest.skipIf(_testmultiphase is None, "test requires _testmultiphase module")
    @support.requires_gil_enabled("gh-117649: not supported in free-threaded build")
    def test_incomplete_multi_phase_init_module(self):
        # Apple extensions must be distributed as frameworks. This requires
        # a specialist loader.
        if support.is_apple_mobile:
            loader = "AppleFrameworkLoader"
        else:
            loader = "ExtensionFileLoader"

        prescript = textwrap.dedent(f'''
            from importlib.util import spec_from_loader, module_from_spec
            from importlib.machinery import {loader}

            name = '_test_shared_gil_only'
            filename = {_testmultiphase.__file__!r}
            loader = {loader}(name, filename)
            spec = spec_from_loader(name, loader)

            ''')

        script = prescript + textwrap.dedent('''
            from importlib.util import _incompatible_extension_module_restrictions
            with _incompatible_extension_module_restrictions(disable_check=True):
                module = module_from_spec(spec)
                loader.exec_module(module)
            ''')
        with self.subTest('check disabled, shared GIL'):
            self.run_with_shared_gil(script)
        with self.subTest('check disabled, per-interpreter GIL'):
            self.run_with_own_gil(script)

        script = prescript + textwrap.dedent('''
            from importlib.util import _incompatible_extension_module_restrictions
            with _incompatible_extension_module_restrictions(disable_check=False):
                module = module_from_spec(spec)
                loader.exec_module(module)
            ''')
        with self.subTest('check enabled, shared GIL'):
            self.run_with_shared_gil(script)
        with self.subTest('check enabled, per-interpreter GIL'):
            with self.assertRaises(ImportError):
                self.run_with_own_gil(script)

    @unittest.skipIf(_testmultiphase is None, "test requires _testmultiphase module")
    def test_complete_multi_phase_init_module(self):
        script = textwrap.dedent('''
            from importlib.util import _incompatible_extension_module_restrictions
            with _incompatible_extension_module_restrictions(disable_check=True):
                import _testmultiphase
            ''')
        with self.subTest('check disabled, shared GIL'):
            self.run_with_shared_gil(script)
        with self.subTest('check disabled, per-interpreter GIL'):
            self.run_with_own_gil(script)

        script = textwrap.dedent(f'''
            from importlib.util import _incompatible_extension_module_restrictions
            with _incompatible_extension_module_restrictions(disable_check=False):
                import _testmultiphase
            ''')
        with self.subTest('check enabled, shared GIL'):
            self.run_with_shared_gil(script)
        with self.subTest('check enabled, per-interpreter GIL'):
            self.run_with_own_gil(script)


if __name__ == '__main__':
    unittest.main()
