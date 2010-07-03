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


class FinderTests(unittest.TestCase):

    """Tests for PathFinder."""

    def test_failure(self):
        # Test None returned upon not finding a suitable finder.
        module = '<test module>'
        with util.import_state():
            self.assertTrue(machinery.PathFinder.find_module(module) is None)

    def test_sys_path(self):
        # Test that sys.path is used when 'path' is None.
        # Implicitly tests that sys.path_importer_cache is used.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        with util.import_state(path_importer_cache={path: importer},
                               path=[path]):
            loader = machinery.PathFinder.find_module(module)
            self.assertTrue(loader is importer)

    def test_path(self):
        # Test that 'path' is used when set.
        # Implicitly tests that sys.path_importer_cache is used.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        with util.import_state(path_importer_cache={path: importer}):
            loader = machinery.PathFinder.find_module(module, [path])
            self.assertTrue(loader is importer)

    def test_path_hooks(self):
        # Test that sys.path_hooks is used.
        # Test that sys.path_importer_cache is set.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        hook = import_util.mock_path_hook(path, importer=importer)
        with util.import_state(path_hooks=[hook]):
            loader = machinery.PathFinder.find_module(module, [path])
            self.assertTrue(loader is importer)
            self.assertTrue(path in sys.path_importer_cache)
            self.assertTrue(sys.path_importer_cache[path] is importer)

    def test_path_importer_cache_has_None(self):
        # Test that if sys.path_importer_cache has None that None is returned.
        clear_cache = {path: None for path in sys.path}
        with util.import_state(path_importer_cache=clear_cache):
            for name in ('asynchat', 'sys', '<test module>'):
                self.assertTrue(machinery.PathFinder.find_module(name) is None)

    def test_path_importer_cache_has_None_continues(self):
        # Test that having None in sys.path_importer_cache causes the search to
        # continue.
        path = '<test path>'
        module = '<test module>'
        importer = util.mock_modules(module)
        with util.import_state(path=['1', '2'],
                            path_importer_cache={'1': None, '2': importer}):
            loader = machinery.PathFinder.find_module(module)
            self.assertTrue(loader is importer)



class DefaultPathFinderTests(unittest.TestCase):

    """Test importlib._bootstrap._DefaultPathFinder."""

    def test_implicit_hooks(self):
        # Test that the implicit path hooks are used.
        bad_path = '<path>'
        module = '<module>'
        assert not os.path.exists(bad_path)
        existing_path = tempfile.mkdtemp()
        try:
            with util.import_state():
                nothing = _bootstrap._DefaultPathFinder.find_module(module,
                                                        path=[existing_path])
                self.assertTrue(nothing is None)
                self.assertTrue(existing_path in sys.path_importer_cache)
                result = isinstance(sys.path_importer_cache[existing_path],
                                    imp.NullImporter)
                self.assertFalse(result)
                nothing = _bootstrap._DefaultPathFinder.find_module(module,
                                                            path=[bad_path])
                self.assertTrue(nothing is None)
                self.assertTrue(bad_path in sys.path_importer_cache)
                self.assertTrue(isinstance(sys.path_importer_cache[bad_path],
                                           imp.NullImporter))
        finally:
            os.rmdir(existing_path)


    def test_path_importer_cache_has_None(self):
        # Test that the default hook is used when sys.path_importer_cache
        # contains None for a path.
        module = '<test module>'
        importer = util.mock_modules(module)
        path = '<test path>'
        # XXX Not blackbox.
        original_hook = _bootstrap._DEFAULT_PATH_HOOK
        mock_hook = import_util.mock_path_hook(path, importer=importer)
        _bootstrap._DEFAULT_PATH_HOOK = mock_hook
        try:
            with util.import_state(path_importer_cache={path: None}):
                loader = _bootstrap._DefaultPathFinder.find_module(module,
                                                                    path=[path])
                self.assertTrue(loader is importer)
        finally:
            _bootstrap._DEFAULT_PATH_HOOK = original_hook


def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests, DefaultPathFinderTests)

if __name__ == '__main__':
    test_main()
