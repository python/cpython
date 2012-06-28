from importlib import _bootstrap
from importlib import machinery
from .. import util
from . import util as import_util
import imp
import os
import sys
import tempfile
from test import support
from types import MethodType
import unittest
import warnings


class FinderTests(unittest.TestCase):

    """Tests for PathFinder."""

    def test_failure(self):
        # Test None returned upon not finding a suitable finder.
        module = '<test module>'
        with util.import_state():
            self.assertIsNone(machinery.PathFinder.find_module(module))

    def test_sys_path(self):
        # Test that sys.path is used when 'path' is None.
        # Implicitly tests that sys.path_importer_cache is used.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        with util.import_state(path_importer_cache={path: importer},
                               path=[path]):
            loader = machinery.PathFinder.find_module(module)
            self.assertIs(loader, importer)

    def test_path(self):
        # Test that 'path' is used when set.
        # Implicitly tests that sys.path_importer_cache is used.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        with util.import_state(path_importer_cache={path: importer}):
            loader = machinery.PathFinder.find_module(module, [path])
            self.assertIs(loader, importer)

    def test_empty_list(self):
        # An empty list should not count as asking for sys.path.
        module = 'module'
        path = '<test path>'
        importer = util.mock_modules(module)
        with util.import_state(path_importer_cache={path: importer},
                               path=[path]):
            self.assertIsNone(machinery.PathFinder.find_module('module', []))

    def test_path_hooks(self):
        # Test that sys.path_hooks is used.
        # Test that sys.path_importer_cache is set.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        hook = import_util.mock_path_hook(path, importer=importer)
        with util.import_state(path_hooks=[hook]):
            loader = machinery.PathFinder.find_module(module, [path])
            self.assertIs(loader, importer)
            self.assertIn(path, sys.path_importer_cache)
            self.assertIs(sys.path_importer_cache[path], importer)

    def test_empty_path_hooks(self):
        # Test that if sys.path_hooks is empty a warning is raised,
        # sys.path_importer_cache gets None set, and PathFinder returns None.
        path_entry = 'bogus_path'
        with util.import_state(path_importer_cache={}, path_hooks=[],
                               path=[path_entry]):
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter('always')
                self.assertIsNone(machinery.PathFinder.find_module('os'))
                self.assertIsNone(sys.path_importer_cache[path_entry])
                self.assertEqual(len(w), 1)
                self.assertTrue(issubclass(w[-1].category, ImportWarning))

    def test_path_importer_cache_empty_string(self):
        # The empty string should create a finder using the cwd.
        path = ''
        module = '<test module>'
        importer = util.mock_modules(module)
        hook = import_util.mock_path_hook(os.curdir, importer=importer)
        with util.import_state(path=[path], path_hooks=[hook]):
            loader = machinery.PathFinder.find_module(module)
            self.assertIs(loader, importer)
            self.assertIn(os.curdir, sys.path_importer_cache)


def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)

if __name__ == '__main__':
    test_main()
