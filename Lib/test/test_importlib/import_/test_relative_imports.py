"""Test relative imports (PEP 328)."""
from .. import util
import unittest
import warnings


class RelativeImports:

    """PEP 328 introduced relative imports. This allows for imports to occur
    from within a package without having to specify the actual package name.

    A simple example is to import another module within the same package
    [module from module]::

      # From pkg.mod1 with pkg.mod2 being a module.
      from . import mod2

    This also works for getting an attribute from a module that is specified
    in a relative fashion [attr from module]::

      # From pkg.mod1.
      from .mod2 import attr

    But this is in no way restricted to working between modules; it works
    from [package to module],::

      # From pkg, importing pkg.module which is a module.
      from . import module

    [module to package],::

      # Pull attr from pkg, called from pkg.module which is a module.
      from . import attr

    and [package to package]::

      # From pkg.subpkg1 (both pkg.subpkg[1,2] are packages).
      from .. import subpkg2

    The number of dots used is in no way restricted [deep import]::

      # Import pkg.attr from pkg.pkg1.pkg2.pkg3.pkg4.pkg5.
      from ...... import attr

    To prevent someone from accessing code that is outside of a package, one
    cannot reach the location containing the root package itself::

      # From pkg.__init__ [too high from package]
      from .. import top_level

      # From pkg.module [too high from module]
      from .. import top_level

     Relative imports are the only type of import that allow for an empty
     module name for an import [empty name].

    """

    def relative_import_test(self, create, globals_, callback):
        """Abstract out boilerplace for setting up for an import test."""
        uncache_names = []
        for name in create:
            if not name.endswith('.__init__'):
                uncache_names.append(name)
            else:
                uncache_names.append(name[:-len('.__init__')])
        with util.mock_spec(*create) as importer:
            with util.import_state(meta_path=[importer]):
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore")
                    for global_ in globals_:
                        with util.uncache(*uncache_names):
                            callback(global_)


    def test_module_from_module(self):
        # [module from module]
        create = 'pkg.__init__', 'pkg.mod2'
        globals_ = {'__package__': 'pkg'}, {'__name__': 'pkg.mod1'}
        def callback(global_):
            self.__import__('pkg')  # For __import__().
            module = self.__import__('', global_, fromlist=['mod2'], level=1)
            self.assertEqual(module.__name__, 'pkg')
            self.assertTrue(hasattr(module, 'mod2'))
            self.assertEqual(module.mod2.attr, 'pkg.mod2')
        self.relative_import_test(create, globals_, callback)

    def test_attr_from_module(self):
        # [attr from module]
        create = 'pkg.__init__', 'pkg.mod2'
        globals_ = {'__package__': 'pkg'}, {'__name__': 'pkg.mod1'}
        def callback(global_):
            self.__import__('pkg')  # For __import__().
            module = self.__import__('mod2', global_, fromlist=['attr'],
                                            level=1)
            self.assertEqual(module.__name__, 'pkg.mod2')
            self.assertEqual(module.attr, 'pkg.mod2')
        self.relative_import_test(create, globals_, callback)

    def test_package_to_module(self):
        # [package to module]
        create = 'pkg.__init__', 'pkg.module'
        globals_ = ({'__package__': 'pkg'},
                    {'__name__': 'pkg', '__path__': ['blah']})
        def callback(global_):
            self.__import__('pkg')  # For __import__().
            module = self.__import__('', global_, fromlist=['module'],
                             level=1)
            self.assertEqual(module.__name__, 'pkg')
            self.assertTrue(hasattr(module, 'module'))
            self.assertEqual(module.module.attr, 'pkg.module')
        self.relative_import_test(create, globals_, callback)

    def test_module_to_package(self):
        # [module to package]
        create = 'pkg.__init__', 'pkg.module'
        globals_ = {'__package__': 'pkg'}, {'__name__': 'pkg.module'}
        def callback(global_):
            self.__import__('pkg')  # For __import__().
            module = self.__import__('', global_, fromlist=['attr'], level=1)
            self.assertEqual(module.__name__, 'pkg')
        self.relative_import_test(create, globals_, callback)

    def test_package_to_package(self):
        # [package to package]
        create = ('pkg.__init__', 'pkg.subpkg1.__init__',
                    'pkg.subpkg2.__init__')
        globals_ =  ({'__package__': 'pkg.subpkg1'},
                     {'__name__': 'pkg.subpkg1', '__path__': ['blah']})
        def callback(global_):
            module = self.__import__('', global_, fromlist=['subpkg2'],
                                            level=2)
            self.assertEqual(module.__name__, 'pkg')
            self.assertTrue(hasattr(module, 'subpkg2'))
            self.assertEqual(module.subpkg2.attr, 'pkg.subpkg2.__init__')

    def test_deep_import(self):
        # [deep import]
        create = ['pkg.__init__']
        for count in range(1,6):
            create.append('{0}.pkg{1}.__init__'.format(
                            create[-1][:-len('.__init__')], count))
        globals_ = ({'__package__': 'pkg.pkg1.pkg2.pkg3.pkg4.pkg5'},
                    {'__name__': 'pkg.pkg1.pkg2.pkg3.pkg4.pkg5',
                        '__path__': ['blah']})
        def callback(global_):
            self.__import__(globals_[0]['__package__'])
            module = self.__import__('', global_, fromlist=['attr'], level=6)
            self.assertEqual(module.__name__, 'pkg')
        self.relative_import_test(create, globals_, callback)

    def test_too_high_from_package(self):
        # [too high from package]
        create = ['top_level', 'pkg.__init__']
        globals_ = ({'__package__': 'pkg'},
                    {'__name__': 'pkg', '__path__': ['blah']})
        def callback(global_):
            self.__import__('pkg')
            with self.assertRaises(ValueError):
                self.__import__('', global_, fromlist=['top_level'],
                                    level=2)
        self.relative_import_test(create, globals_, callback)

    def test_too_high_from_module(self):
        # [too high from module]
        create = ['top_level', 'pkg.__init__', 'pkg.module']
        globals_ = {'__package__': 'pkg'}, {'__name__': 'pkg.module'}
        def callback(global_):
            self.__import__('pkg')
            with self.assertRaises(ValueError):
                self.__import__('', global_, fromlist=['top_level'],
                                    level=2)
        self.relative_import_test(create, globals_, callback)

    def test_empty_name_w_level_0(self):
        # [empty name]
        with self.assertRaises(ValueError):
            self.__import__('')

    def test_import_from_different_package(self):
        # Test importing from a different package than the caller.
        # in pkg.subpkg1.mod
        # from ..subpkg2 import mod
        create = ['__runpy_pkg__.__init__',
                    '__runpy_pkg__.__runpy_pkg__.__init__',
                    '__runpy_pkg__.uncle.__init__',
                    '__runpy_pkg__.uncle.cousin.__init__',
                    '__runpy_pkg__.uncle.cousin.nephew']
        globals_ = {'__package__': '__runpy_pkg__.__runpy_pkg__'}
        def callback(global_):
            self.__import__('__runpy_pkg__.__runpy_pkg__')
            module = self.__import__('uncle.cousin', globals_, {},
                                    fromlist=['nephew'],
                                level=2)
            self.assertEqual(module.__name__, '__runpy_pkg__.uncle.cousin')
        self.relative_import_test(create, globals_, callback)

    def test_import_relative_import_no_fromlist(self):
        # Import a relative module w/ no fromlist.
        create = ['crash.__init__', 'crash.mod']
        globals_ = [{'__package__': 'crash', '__name__': 'crash'}]
        def callback(global_):
            self.__import__('crash')
            mod = self.__import__('mod', global_, {}, [], 1)
            self.assertEqual(mod.__name__, 'crash.mod')
        self.relative_import_test(create, globals_, callback)

    def test_relative_import_no_globals(self):
        # No globals for a relative import is an error.
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            with self.assertRaises(KeyError):
                self.__import__('sys', level=1)

    def test_relative_import_no_package(self):
        with self.assertRaises(ImportError):
            self.__import__('a', {'__package__': '', '__spec__': None},
                            level=1)

    def test_relative_import_no_package_exists_absolute(self):
        with self.assertRaises(ImportError):
            self.__import__('sys', {'__package__': '', '__spec__': None},
                            level=1)


(Frozen_RelativeImports,
 Source_RelativeImports
 ) = util.test_both(RelativeImports, __import__=util.__import__)


if __name__ == '__main__':
    unittest.main()
