from test.test_importlib import util
import sys
import unittest
from test import support
from test.support import import_helper


class ParentModuleTests:

    """Importing a submodule should import the parent modules."""

    def test_import_parent(self):
        with util.mock_spec('pkg.__init__', 'pkg.module') as mock:
            with util.import_state(meta_path=[mock]):
                module = self.__import__('pkg.module')
                self.assertIn('pkg', sys.modules)

    def test_bad_parent(self):
        with util.mock_spec('pkg.module') as mock:
            with util.import_state(meta_path=[mock]):
                with self.assertRaises(ImportError) as cm:
                    self.__import__('pkg.module')
                self.assertEqual(cm.exception.name, 'pkg')

    def test_raising_parent_after_importing_child(self):
        def __init__():
            import pkg.module
            1/0
        mock = util.mock_spec('pkg.__init__', 'pkg.module',
                                 module_code={'pkg': __init__})
        with mock:
            with util.import_state(meta_path=[mock]):
                with self.assertRaises(ZeroDivisionError):
                    self.__import__('pkg')
                self.assertNotIn('pkg', sys.modules)
                self.assertIn('pkg.module', sys.modules)
                with self.assertRaises(ZeroDivisionError):
                    self.__import__('pkg.module')
                self.assertNotIn('pkg', sys.modules)
                self.assertIn('pkg.module', sys.modules)

    def test_raising_parent_after_relative_importing_child(self):
        def __init__():
            from . import module
            1/0
        mock = util.mock_spec('pkg.__init__', 'pkg.module',
                                 module_code={'pkg': __init__})
        with mock:
            with util.import_state(meta_path=[mock]):
                with self.assertRaises((ZeroDivisionError, ImportError)):
                    # This raises ImportError on the "from . import module"
                    # line, not sure why.
                    self.__import__('pkg')
                self.assertNotIn('pkg', sys.modules)
                with self.assertRaises((ZeroDivisionError, ImportError)):
                    self.__import__('pkg.module')
                self.assertNotIn('pkg', sys.modules)
                # XXX False
                #self.assertIn('pkg.module', sys.modules)

    def test_raising_parent_after_double_relative_importing_child(self):
        def __init__():
            from ..subpkg import module
            1/0
        mock = util.mock_spec('pkg.__init__', 'pkg.subpkg.__init__',
                                 'pkg.subpkg.module',
                                 module_code={'pkg.subpkg': __init__})
        with mock:
            with util.import_state(meta_path=[mock]):
                with self.assertRaises((ZeroDivisionError, ImportError)):
                    # This raises ImportError on the "from ..subpkg import module"
                    # line, not sure why.
                    self.__import__('pkg.subpkg')
                self.assertNotIn('pkg.subpkg', sys.modules)
                with self.assertRaises((ZeroDivisionError, ImportError)):
                    self.__import__('pkg.subpkg.module')
                self.assertNotIn('pkg.subpkg', sys.modules)
                # XXX False
                #self.assertIn('pkg.subpkg.module', sys.modules)

    def test_module_not_package(self):
        # Try to import a submodule from a non-package should raise ImportError.
        assert not hasattr(sys, '__path__')
        with self.assertRaises(ImportError) as cm:
            self.__import__('sys.no_submodules_here')
        self.assertEqual(cm.exception.name, 'sys.no_submodules_here')

    def test_module_not_package_but_side_effects(self):
        # If a module injects something into sys.modules as a side-effect, then
        # pick up on that fact.
        name = 'mod'
        subname = name + '.b'
        def module_injection():
            sys.modules[subname] = 'total bunk'
        mock_spec = util.mock_spec('mod',
                                         module_code={'mod': module_injection})
        with mock_spec as mock:
            with util.import_state(meta_path=[mock]):
                try:
                    submodule = self.__import__(subname)
                finally:
                    import_helper.unload(subname)


(Frozen_ParentTests,
 Source_ParentTests
 ) = util.test_both(ParentModuleTests, __import__=util.__import__)


if __name__ == '__main__':
    unittest.main()
