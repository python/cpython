import contextlib
import imp
import importlib
import sys
import unittest


@contextlib.contextmanager
def uncache(*names):
    """Uncache a module from sys.modules.

    A basic sanity check is performed to prevent uncaching modules that either
    cannot/shouldn't be uncached.

    """
    for name in names:
        if name in ('sys', 'marshal', 'imp'):
            raise ValueError(
                "cannot uncache {0} as it will break _importlib".format(name))
        try:
            del sys.modules[name]
        except KeyError:
            pass
    try:
        yield
    finally:
        for name in names:
            try:
                del sys.modules[name]
            except KeyError:
                pass


@contextlib.contextmanager
def import_state(**kwargs):
    """Context manager to manage the various importers and stored state in the
    sys module.

    The 'modules' attribute is not supported as the interpreter state stores a
    pointer to the dict that the interpreter uses internally;
    reassigning to sys.modules does not have the desired effect.

    """
    originals = {}
    try:
        for attr, default in (('meta_path', []), ('path', []),
                              ('path_hooks', []),
                              ('path_importer_cache', {})):
            originals[attr] = getattr(sys, attr)
            if attr in kwargs:
                new_value = kwargs[attr]
                del kwargs[attr]
            else:
                new_value = default
            setattr(sys, attr, new_value)
        if len(kwargs):
            raise ValueError(
                    'unrecognized arguments: {0}'.format(kwargs.keys()))
        yield
    finally:
        for attr, value in originals.items():
            setattr(sys, attr, value)


class mock_modules(object):

    """A mock importer/loader."""

    def __init__(self, *names):
        self.modules = {}
        for name in names:
            if not name.endswith('.__init__'):
                import_name = name
            else:
                import_name = name[:-len('.__init__')]
            if '.' not in name:
                package = None
            elif import_name == name:
                package = name.rsplit('.', 1)[0]
            else:
                package = import_name
            module = imp.new_module(import_name)
            module.__loader__ = self
            module.__file__ = '<mock __file__>'
            module.__package__ = package
            module.attr = name
            if import_name != name:
                module.__path__ = ['<mock __path__>']
            self.modules[import_name] = module

    def __getitem__(self, name):
        return self.modules[name]

    def find_module(self, fullname, path=None):
        if fullname not in self.modules:
            return None
        else:
            return self

    def load_module(self, fullname):
        if fullname not in self.modules:
            raise ImportError
        else:
            sys.modules[fullname] = self.modules[fullname]
            return self.modules[fullname]

    def __enter__(self):
        self._uncache = uncache(*self.modules.keys())
        self._uncache.__enter__()
        return self

    def __exit__(self, *exc_info):
        self._uncache.__exit__(None, None, None)



class ImportModuleTests(unittest.TestCase):

    """Test importlib.import_module."""

    def test_module_import(self):
        # Test importing a top-level module.
        with mock_modules('top_level') as mock:
            with import_state(meta_path=[mock]):
                module = importlib.import_module('top_level')
                self.assertEqual(module.__name__, 'top_level')

    def test_absolute_package_import(self):
        # Test importing a module from a package with an absolute name.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with mock_modules(pkg_long_name, name) as mock:
            with import_state(meta_path=[mock]):
                module = importlib.import_module(name)
                self.assertEqual(module.__name__, name)

    def test_shallow_relative_package_import(self):
        modules = ['a.__init__', 'a.b.__init__', 'a.b.c.__init__', 'a.b.c.d']
        with mock_modules(*modules) as mock:
            with import_state(meta_path=[mock]):
                module = importlib.import_module('.d', 'a.b.c')
                self.assertEqual(module.__name__, 'a.b.c.d')

    def test_deep_relative_package_import(self):
        # Test importing a module from a package through a relatve import.
        modules = ['a.__init__', 'a.b.__init__', 'a.c']
        with mock_modules(*modules) as mock:
            with import_state(meta_path=[mock]):
                module = importlib.import_module('..c', 'a.b')
                self.assertEqual(module.__name__, 'a.c')

    def test_absolute_import_with_package(self):
        # Test importing a module from a package with an absolute name with
        # the 'package' argument given.
        pkg_name = 'pkg'
        pkg_long_name = '{0}.__init__'.format(pkg_name)
        name = '{0}.mod'.format(pkg_name)
        with mock_modules(pkg_long_name, name) as mock:
            with import_state(meta_path=[mock]):
                module = importlib.import_module(name, pkg_name)
                self.assertEqual(module.__name__, name)

    def test_relative_import_wo_package(self):
        # Relative imports cannot happen without the 'package' argument being
        # set.
        self.assertRaises(TypeError, importlib.import_module, '.support')


def test_main():
    from test.test_support import run_unittest
    run_unittest(ImportModuleTests)


if __name__ == '__main__':
    test_main()
