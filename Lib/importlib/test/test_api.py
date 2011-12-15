from . import util
import imp
import importlib
import sys
import unittest


class ImportModuleTests(unittest.TestCase):

    """Test importlib.import_module."""

    def test_module_import(self):
        # Test importing a top-level module.
        with util.mock_modules('top_level') as mock:
            with util.import_state(meta_path=[mock]):
                module = importlib.import_module('top_level')
                self.assertEqual(module.__name__, 'top_level')

    def test_absolute_package_import(self):
        # Test importing a module from a package with an absolute name.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with util.mock_modules(pkg_long_name, name) as mock:
            with util.import_state(meta_path=[mock]):
                module = importlib.import_module(name)
                self.assertEqual(module.__name__, name)

    def test_shallow_relative_package_import(self):
        # Test importing a module from a package through a relative import.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        module_name = 'mod'
        absolute_name = '{0}.{1}'.format(pkg_name, module_name)
        relative_name = '.{0}'.format(module_name)
        with util.mock_modules(pkg_long_name, absolute_name) as mock:
            with util.import_state(meta_path=[mock]):
                importlib.import_module(pkg_name)
                module = importlib.import_module(relative_name, pkg_name)
                self.assertEqual(module.__name__, absolute_name)

    def test_deep_relative_package_import(self):
        modules = ['a.__init__', 'a.b.__init__', 'a.c']
        with util.mock_modules(*modules) as mock:
            with util.import_state(meta_path=[mock]):
                importlib.import_module('a')
                importlib.import_module('a.b')
                module = importlib.import_module('..c', 'a.b')
                self.assertEqual(module.__name__, 'a.c')

    def test_absolute_import_with_package(self):
        # Test importing a module from a package with an absolute name with
        # the 'package' argument given.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with util.mock_modules(pkg_long_name, name) as mock:
            with util.import_state(meta_path=[mock]):
                importlib.import_module(pkg_name)
                module = importlib.import_module(name, pkg_name)
                self.assertEqual(module.__name__, name)

    def test_relative_import_wo_package(self):
        # Relative imports cannot happen without the 'package' argument being
        # set.
        with self.assertRaises(TypeError):
            importlib.import_module('.support')


    def test_loaded_once(self):
        # Issue #13591: Modules should only be loaded once when
        # initializing the parent package attempts to import the
        # module currently being imported.
        b_load_count = 0
        def load_a():
            importlib.import_module('a.b')
        def load_b():
            nonlocal b_load_count
            b_load_count += 1
        code = {'a': load_a, 'a.b': load_b}
        modules = ['a.__init__', 'a.b']
        with util.mock_modules(*modules, module_code=code) as mock:
            with util.import_state(meta_path=[mock]):
                importlib.import_module('a.b')
        self.assertEqual(b_load_count, 1)

def test_main():
    from test.support import run_unittest
    run_unittest(ImportModuleTests)


if __name__ == '__main__':
    test_main()
