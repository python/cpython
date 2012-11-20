from importlib import _bootstrap
from importlib import machinery
from importlib import import_module
from .. import util
from . import util as import_util
import os
import sys
from types import ModuleType
import unittest
import warnings
import zipimport


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

    def test_None_on_sys_path(self):
        # Putting None in sys.path[0] caused an import regression from Python
        # 3.2: http://bugs.python.org/issue16514
        new_path = sys.path[:]
        new_path.insert(0, None)
        new_path_importer_cache = sys.path_importer_cache.copy()
        new_path_importer_cache.pop(None, None)
        new_path_hooks = [zipimport.zipimporter,
                          _bootstrap.FileFinder.path_hook(
                              *_bootstrap._get_supported_file_loaders())]
        missing = object()
        email = sys.modules.pop('email', missing)
        try:
            with util.import_state(meta_path=sys.meta_path[:],
                                   path=new_path,
                                   path_importer_cache=new_path_importer_cache,
                                   path_hooks=new_path_hooks):
                module = import_module('email')
                self.assertIsInstance(module, ModuleType)
        finally:
            if email is not missing:
                sys.modules['email'] = email


def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)

if __name__ == '__main__':
    test_main()
