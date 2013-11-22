from .. import abc
from .. import util
from . import util as builtin_util

frozen_machinery, source_machinery = util.import_importlib('importlib.machinery')

import sys
import unittest


class FindSpecTests(abc.FinderTests):

    """Test find_spec() for built-in modules."""

    def test_module(self):
        # Common case.
        with util.uncache(builtin_util.NAME):
            found = self.machinery.BuiltinImporter.find_spec(builtin_util.NAME)
            self.assertTrue(found)
            self.assertEqual(found.origin, 'built-in')

    # Built-in modules cannot be a package.
    test_package = None

    # Built-in modules cannobt be in a package.
    test_module_in_package = None

    # Built-in modules cannot be a package.
    test_package_in_package = None

    # Built-in modules cannot be a package.
    test_package_over_module = None

    def test_failure(self):
        name = 'importlib'
        assert name not in sys.builtin_module_names
        spec = self.machinery.BuiltinImporter.find_spec(name)
        self.assertIsNone(spec)

    def test_ignore_path(self):
        # The value for 'path' should always trigger a failed import.
        with util.uncache(builtin_util.NAME):
            spec = self.machinery.BuiltinImporter.find_spec(builtin_util.NAME,
                                                            ['pkg'])
            self.assertIsNone(spec)

Frozen_FindSpecTests, Source_FindSpecTests = util.test_both(FindSpecTests,
        machinery=[frozen_machinery, source_machinery])


class FinderTests(abc.FinderTests):

    """Test find_module() for built-in modules."""

    def test_module(self):
        # Common case.
        with util.uncache(builtin_util.NAME):
            found = self.machinery.BuiltinImporter.find_module(builtin_util.NAME)
            self.assertTrue(found)
            self.assertTrue(hasattr(found, 'load_module'))

    # Built-in modules cannot be a package.
    test_package = test_package_in_package = test_package_over_module = None

    # Built-in modules cannot be in a package.
    test_module_in_package = None

    def test_failure(self):
        assert 'importlib' not in sys.builtin_module_names
        loader = self.machinery.BuiltinImporter.find_module('importlib')
        self.assertIsNone(loader)

    def test_ignore_path(self):
        # The value for 'path' should always trigger a failed import.
        with util.uncache(builtin_util.NAME):
            loader = self.machinery.BuiltinImporter.find_module(builtin_util.NAME,
                                                            ['pkg'])
            self.assertIsNone(loader)

Frozen_FinderTests, Source_FinderTests = util.test_both(FinderTests,
        machinery=[frozen_machinery, source_machinery])


if __name__ == '__main__':
    unittest.main()
