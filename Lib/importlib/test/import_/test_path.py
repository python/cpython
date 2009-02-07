from importlib import machinery
from .. import util
from . import util as import_util
from contextlib import nested
import imp
import os
import sys
from test import support
from types import MethodType
import unittest


class FinderTests(unittest.TestCase):

    """Tests for SysPathImporter."""

    def test_failure(self):
        # Test None returned upon not finding a suitable finder.
        def mock_implicit_hooks():
            return []
        # XXX Not blackbox.
        original_hooks = machinery.PathFinder._implicit_hooks
        machinery.PathFinder._implicit_hooks = staticmethod(mock_implicit_hooks)
        try:
            with util.import_state():
                self.assert_(machinery.PathFinder.find_module('XXX') is None)
        finally:
            machinery.PathFinder._implicit_hooks = original_hooks

    def test_sys_path(self):
        # Test that sys.path is used when 'path' is None.
        # Implicitly tests that sys.path_importer_cache is used.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        with util.import_state(path_importer_cache={path: importer},
                               path=[path]):
            loader = machinery.PathFinder.find_module(module)
            self.assert_(loader is importer)

    def test_path(self):
        # Test that 'path' is used when set.
        # Implicitly tests that sys.path_importer_cache is used.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        with util.import_state(path_importer_cache={path: importer}):
            loader = machinery.PathFinder.find_module(module, [path])
            self.assert_(loader is importer)

    def test_path_importer_cache_has_None(self):
        # Test that the default hook is used when sys.path_importer_cache
        # contains None for a path.
        module = '<test module>'
        importer = util.mock_modules(module)
        path = '<test path>'
        # XXX Not blackbox.
        original_hook = machinery.PathFinder._default_hook
        mock_hook = import_util.mock_path_hook(path, importer=importer)
        machinery.PathFinder._default_hook = staticmethod(mock_hook)
        try:
            with util.import_state(path_importer_cache={path: None}):
                loader = machinery.PathFinder.find_module(module, path=[path])
                self.assert_(loader is importer)
        finally:
            machinery.PathFinder._default_hook = original_hook

    def test_path_hooks(self):
        # Test that sys.path_hooks is used.
        # Test that sys.path_importer_cache is set.
        module = '<test module>'
        path = '<test path>'
        importer = util.mock_modules(module)
        hook = import_util.mock_path_hook(path, importer=importer)
        with util.import_state(path_hooks=[hook]):
            loader = machinery.PathFinder.find_module(module, [path])
            self.assert_(loader is importer)
            self.assert_(path in sys.path_importer_cache)
            self.assert_(sys.path_importer_cache[path] is importer)

    def test_implicit_hooks(self):
        # Test that the implicit path hooks are used.
        existing_path = os.path.dirname(support.TESTFN)
        bad_path = '<path>'
        module = '<module>'
        assert not os.path.exists(bad_path)
        with util.import_state():
            nothing = machinery.PathFinder.find_module(module,
                                                       path=[existing_path])
            self.assert_(nothing is None)
            self.assert_(existing_path in sys.path_importer_cache)
            self.assert_(not isinstance(sys.path_importer_cache[existing_path],
                                        imp.NullImporter))
            nothing = machinery.PathFinder.find_module(module, path=[bad_path])
            self.assert_(nothing is None)
            self.assert_(bad_path in sys.path_importer_cache)
            self.assert_(isinstance(sys.path_importer_cache[bad_path],
                                    imp.NullImporter))


def test_main():
    from test.support import run_unittest
    run_unittest(PathTests, __path__Tests, FinderTests)

if __name__ == '__main__':
    test_main()
