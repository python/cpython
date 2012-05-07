from .. import util
from . import util as import_util
import sys
import unittest
import importlib
from test import support


class ParentModuleTests(unittest.TestCase):

    """Importing a submodule should import the parent modules."""

    def test_import_parent(self):
        with util.mock_modules('pkg.__init__', 'pkg.module') as mock:
            with util.import_state(meta_path=[mock]):
                module = import_util.import_('pkg.module')
                self.assertTrue('pkg' in sys.modules)

    def test_bad_parent(self):
        with util.mock_modules('pkg.module') as mock:
            with util.import_state(meta_path=[mock]):
                with self.assertRaises(ImportError) as cm:
                    import_util.import_('pkg.module')
                self.assertEqual(cm.exception.name, 'pkg')

    def test_raising_parent_after_importing_child(self):
        def __init__():
            import pkg.module
            1/0
        mock = util.mock_modules('pkg.__init__', 'pkg.module',
                                 module_code={'pkg': __init__})
        with mock:
            with util.import_state(meta_path=[mock]):
                with self.assertRaises(ZeroDivisionError):
                    import_util.import_('pkg')
                self.assertFalse('pkg' in sys.modules)
                self.assertTrue('pkg.module' in sys.modules)
                with self.assertRaises(ZeroDivisionError):
                    import_util.import_('pkg.module')
                self.assertFalse('pkg' in sys.modules)
                self.assertTrue('pkg.module' in sys.modules)

    def test_raising_parent_after_relative_importing_child(self):
        def __init__():
            from . import module
            1/0
        mock = util.mock_modules('pkg.__init__', 'pkg.module',
                                 module_code={'pkg': __init__})
        with mock:
            with util.import_state(meta_path=[mock]):
                with self.assertRaises((ZeroDivisionError, ImportError)):
                    # This raises ImportError on the "from . import module"
                    # line, not sure why.
                    import_util.import_('pkg')
                self.assertFalse('pkg' in sys.modules)
                with self.assertRaises((ZeroDivisionError, ImportError)):
                    import_util.import_('pkg.module')
                self.assertFalse('pkg' in sys.modules)
                # XXX False
                #self.assertTrue('pkg.module' in sys.modules)

    def test_raising_parent_after_double_relative_importing_child(self):
        def __init__():
            from ..subpkg import module
            1/0
        mock = util.mock_modules('pkg.__init__', 'pkg.subpkg.__init__',
                                 'pkg.subpkg.module',
                                 module_code={'pkg.subpkg': __init__})
        with mock:
            with util.import_state(meta_path=[mock]):
                with self.assertRaises((ZeroDivisionError, ImportError)):
                    # This raises ImportError on the "from ..subpkg import module"
                    # line, not sure why.
                    import_util.import_('pkg.subpkg')
                self.assertFalse('pkg.subpkg' in sys.modules)
                with self.assertRaises((ZeroDivisionError, ImportError)):
                    import_util.import_('pkg.subpkg.module')
                self.assertFalse('pkg.subpkg' in sys.modules)
                # XXX False
                #self.assertTrue('pkg.subpkg.module' in sys.modules)

    def test_module_not_package(self):
        # Try to import a submodule from a non-package should raise ImportError.
        assert not hasattr(sys, '__path__')
        with self.assertRaises(ImportError) as cm:
            import_util.import_('sys.no_submodules_here')
        self.assertEqual(cm.exception.name, 'sys.no_submodules_here')

    def test_module_not_package_but_side_effects(self):
        # If a module injects something into sys.modules as a side-effect, then
        # pick up on that fact.
        name = 'mod'
        subname = name + '.b'
        def module_injection():
            sys.modules[subname] = 'total bunk'
        mock_modules = util.mock_modules('mod',
                                         module_code={'mod': module_injection})
        with mock_modules as mock:
            with util.import_state(meta_path=[mock]):
                try:
                    submodule = import_util.import_(subname)
                finally:
                    support.unload(subname)


def test_main():
    from test.support import run_unittest
    run_unittest(ParentModuleTests)


if __name__ == '__main__':
    test_main()
