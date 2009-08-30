from .. import util
from . import util as import_util
import sys
import unittest
import importlib


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


def test_main():
    from test.support import run_unittest
    run_unittest(ParentModuleTests)


if __name__ == '__main__':
    test_main()
