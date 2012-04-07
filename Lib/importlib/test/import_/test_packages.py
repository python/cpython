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
                with self.assertRaises(ImportError):
                    import_util.import_('pkg.module')

    def test_module_not_package(self):
        # Try to import a submodule from a non-package should raise ImportError.
        assert not hasattr(sys, '__path__')
        with self.assertRaises(ImportError):
            import_util.import_('sys.no_submodules_here')

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
