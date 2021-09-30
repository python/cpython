from .. import abc
import os.path
from .. import util

machinery = util.import_importlib('importlib.machinery')

import unittest
import warnings

from test.support import import_helper


class FindSpecTests(abc.FinderTests):

    """Test finding frozen modules."""

    def find(self, name, **kwargs):
        finder = self.machinery.FrozenImporter
        with import_helper.frozen_modules():
            return finder.find_spec(name, **kwargs)

    def check(self, spec, name):
        self.assertEqual(spec.name, name)
        self.assertIs(spec.loader, self.machinery.FrozenImporter)
        self.assertEqual(spec.origin, 'frozen')
        self.assertFalse(spec.has_location)

    def test_module(self):
        names = [
            '__hello__',
            '__hello_alias__',
            '__hello_only__',
            '__phello__.__init__',
            '__phello__.spam',
            '__phello__.ham.__init__',
            '__phello__.ham.eggs',
        ]
        for name in names:
            with self.subTest(name):
                spec = self.find(name)
                self.check(spec, name)
                self.assertEqual(spec.submodule_search_locations, None)

    def test_package(self):
        names = [
            '__phello__',
            '__phello__.ham',
            '__phello_alias__',
        ]
        for name in names:
            with self.subTest(name):
                spec = self.find(name)
                self.check(spec, name)
                self.assertEqual(spec.submodule_search_locations, [])

    # These are covered by test_module() and test_package().
    test_module_in_package = None
    test_package_in_package = None

    # No easy way to test.
    test_package_over_module = None

    def test_path_ignored(self):
        for name in ('__hello__', '__phello__', '__phello__.spam'):
            actual = self.find(name)
            for path in (None, object(), '', 'eggs', [], [''], ['eggs']):
                with self.subTest((name, path)):
                    spec = self.find(name, path=path)
                    self.assertEqual(spec, actual)

    def test_target_ignored(self):
        imported = ('__hello__', '__phello__')
        with import_helper.CleanImport(*imported, usefrozen=True):
            import __hello__ as match
            import __phello__ as nonmatch
        name = '__hello__'
        actual = self.find(name)
        for target in (None, match, nonmatch, object(), 'not-a-module-object'):
            with self.subTest(target):
                spec = self.find(name, target=target)
                self.assertEqual(spec, actual)

    def test_failure(self):
        spec = self.find('<not real>')
        self.assertIsNone(spec)

    def test_not_using_frozen(self):
        finder = self.machinery.FrozenImporter
        with import_helper.frozen_modules(enabled=False):
            # both frozen and not frozen
            spec1 = finder.find_spec('__hello__')
            # only frozen
            spec2 = finder.find_spec('__hello_only__')
        self.assertIsNone(spec1)
        self.assertIsNone(spec2)


(Frozen_FindSpecTests,
 Source_FindSpecTests
 ) = util.test_both(FindSpecTests, machinery=machinery)


class FinderTests(abc.FinderTests):

    """Test finding frozen modules."""

    def find(self, name, path=None):
        finder = self.machinery.FrozenImporter
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            with import_helper.frozen_modules():
                return finder.find_module(name, path)

    def test_module(self):
        name = '__hello__'
        loader = self.find(name)
        self.assertTrue(hasattr(loader, 'load_module'))

    def test_package(self):
        loader = self.find('__phello__')
        self.assertTrue(hasattr(loader, 'load_module'))

    def test_module_in_package(self):
        loader = self.find('__phello__.spam', ['__phello__'])
        self.assertTrue(hasattr(loader, 'load_module'))

    # No frozen package within another package to test with.
    test_package_in_package = None

    # No easy way to test.
    test_package_over_module = None

    def test_failure(self):
        loader = self.find('<not real>')
        self.assertIsNone(loader)


(Frozen_FinderTests,
 Source_FinderTests
 ) = util.test_both(FinderTests, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
