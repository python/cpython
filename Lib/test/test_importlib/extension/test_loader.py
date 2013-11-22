from . import util as ext_util
from .. import abc
from .. import util

machinery = util.import_importlib('importlib.machinery')

import os.path
import sys
import types
import unittest


class ExecModuleTests(abc.LoaderTests):

    """Test load_module() for extension modules."""

    def setUp(self):
        self.loader = self.machinery.ExtensionFileLoader(ext_util.NAME,
                                                         ext_util.FILEPATH)

    def exec_module(self, fullname):
        module = types.ModuleType(fullname)
        module.__spec__ = self.machinery.ModuleSpec(fullname, self.loader)
        self.loader.exec_module(module)
        return sys.modules[fullname]

    def test_exec_module_API(self):
        with self.assertRaises(ImportError):
            self.exec_module('XXX')


    def test_module(self):
        with util.uncache(ext_util.NAME):
            module = self.exec_module(ext_util.NAME)
            for attr, value in [('__name__', ext_util.NAME),
                                ('__file__', ext_util.FILEPATH),
                                ('__package__', '')]:
                given = getattr(module, attr)
                self.assertEqual(given, value,
                                 '{}: {!r} != {!r}'.format(attr, given, value))
            self.assertIn(ext_util.NAME, sys.modules)
            self.assertIsInstance(module.__loader__,
                                  self.machinery.ExtensionFileLoader)

    # No extension module as __init__ available for testing.
    test_package = None

    # No extension module in a package available for testing.
    test_lacking_parent = None

    def test_module_reuse(self):
        with util.uncache(ext_util.NAME):
            module1 = self.exec_module(ext_util.NAME)
            module2 = self.exec_module(ext_util.NAME)
            self.assertIs(module1, module2)

    def test_state_after_failure(self):
        # No easy way to trigger a failure after a successful import.
        pass

    def test_unloadable(self):
        name = 'asdfjkl;'
        with self.assertRaises(ImportError) as cm:
            self.exec_module(name)
        self.assertEqual(cm.exception.name, name)

    def test_is_package(self):
        self.assertFalse(self.loader.is_package(ext_util.NAME))
        for suffix in self.machinery.EXTENSION_SUFFIXES:
            path = os.path.join('some', 'path', 'pkg', '__init__' + suffix)
            loader = self.machinery.ExtensionFileLoader('pkg', path)
            self.assertTrue(loader.is_package('pkg'))

Frozen_ExecModuleTests, Source_ExecModuleTests = util.test_both(
        ExecModuleTests, machinery=machinery)


class LoaderTests(abc.LoaderTests):

    """Test load_module() for extension modules."""

    def setUp(self):
        self.loader = self.machinery.ExtensionFileLoader(ext_util.NAME,
                                                         ext_util.FILEPATH)

    def load_module(self, fullname):
        return self.loader.load_module(fullname)

    def test_load_module_API(self):
        # Test the default argument for load_module().
        self.loader.load_module()
        self.loader.load_module(None)
        with self.assertRaises(ImportError):
            self.load_module('XXX')


    def test_module(self):
        with util.uncache(ext_util.NAME):
            module = self.load_module(ext_util.NAME)
            for attr, value in [('__name__', ext_util.NAME),
                                ('__file__', ext_util.FILEPATH),
                                ('__package__', '')]:
                self.assertEqual(getattr(module, attr), value)
            self.assertIn(ext_util.NAME, sys.modules)
            self.assertIsInstance(module.__loader__,
                                  self.machinery.ExtensionFileLoader)

    # No extension module as __init__ available for testing.
    test_package = None

    # No extension module in a package available for testing.
    test_lacking_parent = None

    def test_module_reuse(self):
        with util.uncache(ext_util.NAME):
            module1 = self.load_module(ext_util.NAME)
            module2 = self.load_module(ext_util.NAME)
            self.assertIs(module1, module2)

    # No easy way to trigger a failure after a successful import.
    test_state_after_failure = None

    def test_unloadable(self):
        name = 'asdfjkl;'
        with self.assertRaises(ImportError) as cm:
            self.load_module(name)
        self.assertEqual(cm.exception.name, name)

    def test_is_package(self):
        self.assertFalse(self.loader.is_package(ext_util.NAME))
        for suffix in self.machinery.EXTENSION_SUFFIXES:
            path = os.path.join('some', 'path', 'pkg', '__init__' + suffix)
            loader = self.machinery.ExtensionFileLoader('pkg', path)
            self.assertTrue(loader.is_package('pkg'))

Frozen_LoaderTests, Source_LoaderTests = util.test_both(
        LoaderTests, machinery=machinery)



if __name__ == '__main__':
    unittest.main()
