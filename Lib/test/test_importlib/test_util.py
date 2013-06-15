from importlib import util
from . import util as test_util
import imp
import os
import sys
from test import support
import types
import unittest
import warnings


class ModuleToLoadTests(unittest.TestCase):

    module_name = 'ModuleManagerTest_module'

    def setUp(self):
        support.unload(self.module_name)
        self.addCleanup(support.unload, self.module_name)

    def test_new_module(self):
        # Test a new module is created, inserted into sys.modules, has
        # __initializing__ set to True after entering the context manager,
        # and __initializing__ set to False after exiting.
        with util.module_to_load(self.module_name) as module:
            self.assertIn(self.module_name, sys.modules)
            self.assertIs(sys.modules[self.module_name], module)
            self.assertTrue(module.__initializing__)
        self.assertFalse(module.__initializing__)

    def test_new_module_failed(self):
        # Test the module is removed from sys.modules.
        try:
            with util.module_to_load(self.module_name) as module:
                self.assertIn(self.module_name, sys.modules)
                raise exception
        except Exception:
            self.assertNotIn(self.module_name, sys.modules)
        else:
            self.fail('importlib.util.module_to_load swallowed an exception')

    def test_reload(self):
        # Test that the same module is in sys.modules.
        created_module = imp.new_module(self.module_name)
        sys.modules[self.module_name] = created_module
        with util.module_to_load(self.module_name) as module:
            self.assertIs(module, created_module)

    def test_reload_failed(self):
        # Test that the module was left in sys.modules.
        created_module = imp.new_module(self.module_name)
        sys.modules[self.module_name] = created_module
        try:
            with util.module_to_load(self.module_name) as module:
                raise Exception
        except Exception:
            self.assertIn(self.module_name, sys.modules)
        else:
            self.fail('importlib.util.module_to_load swallowed an exception')

    def test_reset_name(self):
        # If reset_name is true then module.__name__ = name, else leave it be.
        odd_name = 'not your typical name'
        created_module = imp.new_module(self.module_name)
        created_module.__name__ = odd_name
        sys.modules[self.module_name] = created_module
        with util.module_to_load(self.module_name) as module:
            self.assertEqual(module.__name__, self.module_name)
        created_module.__name__ = odd_name
        with util.module_to_load(self.module_name, reset_name=False) as module:
            self.assertEqual(module.__name__, odd_name)


class ModuleForLoaderTests(unittest.TestCase):

    """Tests for importlib.util.module_for_loader."""

    @staticmethod
    def module_for_loader(func):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', PendingDeprecationWarning)
            return util.module_for_loader(func)

    def test_warning(self):
        # Should raise a PendingDeprecationWarning when used.
        with warnings.catch_warnings():
            warnings.simplefilter('error', PendingDeprecationWarning)
            with self.assertRaises(PendingDeprecationWarning):
                func = util.module_for_loader(lambda x: x)

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
        module = imp.new_module('a.b.c')
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
        module = imp.new_module(name)
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


class SetPackageTests(unittest.TestCase):

    """Tests for importlib.util.set_package."""

    def verify(self, module, expect):
        """Verify the module has the expected value for __package__ after
        passing through set_package."""
        fxn = lambda: module
        wrapped = util.set_package(fxn)
        wrapped()
        self.assertTrue(hasattr(module, '__package__'))
        self.assertEqual(expect, module.__package__)

    def test_top_level(self):
        # __package__ should be set to the empty string if a top-level module.
        # Implicitly tests when package is set to None.
        module = imp.new_module('module')
        module.__package__ = None
        self.verify(module, '')

    def test_package(self):
        # Test setting __package__ for a package.
        module = imp.new_module('pkg')
        module.__path__ = ['<path>']
        module.__package__ = None
        self.verify(module, 'pkg')

    def test_submodule(self):
        # Test __package__ for a module in a package.
        module = imp.new_module('pkg.mod')
        module.__package__ = None
        self.verify(module, 'pkg')

    def test_setting_if_missing(self):
        # __package__ should be set if it is missing.
        module = imp.new_module('mod')
        if hasattr(module, '__package__'):
            delattr(module, '__package__')
        self.verify(module, '')

    def test_leaving_alone(self):
        # If __package__ is set and not None then leave it alone.
        for value in (True, False):
            module = imp.new_module('mod')
            module.__package__ = value
            self.verify(module, value)

    def test_decorator_attrs(self):
        def fxn(module): pass
        wrapped = util.set_package(fxn)
        self.assertEqual(wrapped.__name__, fxn.__name__)
        self.assertEqual(wrapped.__qualname__, fxn.__qualname__)


class SetLoaderTests(unittest.TestCase):

    """Tests importlib.util.set_loader()."""

    class DummyLoader:
        @util.set_loader
        def load_module(self, module):
            return self.module

    def test_no_attribute(self):
        loader = self.DummyLoader()
        loader.module = imp.new_module('blah')
        try:
            del loader.module.__loader__
        except AttributeError:
            pass
        self.assertEqual(loader, loader.load_module('blah').__loader__)

    def test_attribute_is_None(self):
        loader = self.DummyLoader()
        loader.module = imp.new_module('blah')
        loader.module.__loader__ = None
        self.assertEqual(loader, loader.load_module('blah').__loader__)

    def test_not_reset(self):
        loader = self.DummyLoader()
        loader.module = imp.new_module('blah')
        loader.module.__loader__ = 42
        self.assertEqual(42, loader.load_module('blah').__loader__)


class ResolveNameTests(unittest.TestCase):

    """Tests importlib.util.resolve_name()."""

    def test_absolute(self):
        # bacon
        self.assertEqual('bacon', util.resolve_name('bacon', None))

    def test_aboslute_within_package(self):
        # bacon in spam
        self.assertEqual('bacon', util.resolve_name('bacon', 'spam'))

    def test_no_package(self):
        # .bacon in ''
        with self.assertRaises(ValueError):
            util.resolve_name('.bacon', '')

    def test_in_package(self):
        # .bacon in spam
        self.assertEqual('spam.eggs.bacon',
                         util.resolve_name('.bacon', 'spam.eggs'))

    def test_other_package(self):
        # ..bacon in spam.bacon
        self.assertEqual('spam.bacon',
                         util.resolve_name('..bacon', 'spam.eggs'))

    def test_escape(self):
        # ..bacon in spam
        with self.assertRaises(ValueError):
            util.resolve_name('..bacon', 'spam')


class MagicNumberTests(unittest.TestCase):

    def test_length(self):
        # Should be 4 bytes.
        self.assertEqual(len(util.MAGIC_NUMBER), 4)

    def test_incorporates_rn(self):
        # The magic number uses \r\n to come out wrong when splitting on lines.
        self.assertTrue(util.MAGIC_NUMBER.endswith(b'\r\n'))


class PEP3147Tests(unittest.TestCase):
    """Tests of PEP 3147-related functions:
    cache_from_source and source_from_cache.

    """

    tag = imp.get_tag()

    @unittest.skipUnless(sys.implementation.cache_tag is not None,
                         'requires sys.implementation.cache_tag not be None')
    def test_cache_from_source(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyc file (i.e. under __pycache__).
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        expect = os.path.join('foo', 'bar', 'baz', '__pycache__',
                              'qux.{}.pyc'.format(self.tag))
        self.assertEqual(util.cache_from_source(path, True), expect)

    def test_cache_from_source_no_cache_tag(self):
        # No cache tag means NotImplementedError.
        with support.swap_attr(sys.implementation, 'cache_tag', None):
            with self.assertRaises(NotImplementedError):
                util.cache_from_source('whatever.py')

    def test_cache_from_source_no_dot(self):
        # Directory with a dot, filename without dot.
        path = os.path.join('foo.bar', 'file')
        expect = os.path.join('foo.bar', '__pycache__',
                              'file{}.pyc'.format(self.tag))
        self.assertEqual(util.cache_from_source(path, True), expect)

    def test_cache_from_source_optimized(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyo file (i.e. under __pycache__).
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        expect = os.path.join('foo', 'bar', 'baz', '__pycache__',
                              'qux.{}.pyo'.format(self.tag))
        self.assertEqual(util.cache_from_source(path, False), expect)

    def test_cache_from_source_cwd(self):
        path = 'foo.py'
        expect = os.path.join('__pycache__', 'foo.{}.pyc'.format(self.tag))
        self.assertEqual(util.cache_from_source(path, True), expect)

    def test_cache_from_source_override(self):
        # When debug_override is not None, it can be any true-ish or false-ish
        # value.
        path = os.path.join('foo', 'bar', 'baz.py')
        partial_expect = os.path.join('foo', 'bar', '__pycache__',
                                      'baz.{}.py'.format(self.tag))
        self.assertEqual(util.cache_from_source(path, []), partial_expect + 'o')
        self.assertEqual(util.cache_from_source(path, [17]),
                         partial_expect + 'c')
        # However if the bool-ishness can't be determined, the exception
        # propagates.
        class Bearish:
            def __bool__(self): raise RuntimeError
        with self.assertRaises(RuntimeError):
            util.cache_from_source('/foo/bar/baz.py', Bearish())

    @unittest.skipUnless(os.sep == '\\' and os.altsep == '/',
                     'test meaningful only where os.altsep is defined')
    def test_sep_altsep_and_sep_cache_from_source(self):
        # Windows path and PEP 3147 where sep is right of altsep.
        self.assertEqual(
            util.cache_from_source('\\foo\\bar\\baz/qux.py', True),
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
        self.assertEqual(util.source_from_cache(path), expect)

    def test_source_from_cache_no_cache_tag(self):
        # If sys.implementation.cache_tag is None, raise NotImplementedError.
        path = os.path.join('blah', '__pycache__', 'whatever.pyc')
        with support.swap_attr(sys.implementation, 'cache_tag', None):
            with self.assertRaises(NotImplementedError):
                util.source_from_cache(path)

    def test_source_from_cache_bad_path(self):
        # When the path to a pyc file is not in PEP 3147 format, a ValueError
        # is raised.
        self.assertRaises(
            ValueError, util.source_from_cache, '/foo/bar/bazqux.pyc')

    def test_source_from_cache_no_slash(self):
        # No slashes at all in path -> ValueError
        self.assertRaises(
            ValueError, util.source_from_cache, 'foo.cpython-32.pyc')

    def test_source_from_cache_too_few_dots(self):
        # Too few dots in final path component -> ValueError
        self.assertRaises(
            ValueError, util.source_from_cache, '__pycache__/foo.pyc')

    def test_source_from_cache_too_many_dots(self):
        # Too many dots in final path component -> ValueError
        self.assertRaises(
            ValueError, util.source_from_cache,
            '__pycache__/foo.cpython-32.foo.pyc')

    def test_source_from_cache_no__pycache__(self):
        # Another problem with the path -> ValueError
        self.assertRaises(
            ValueError, util.source_from_cache,
            '/foo/bar/foo.cpython-32.foo.pyc')


if __name__ == '__main__':
    unittest.main()
