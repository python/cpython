from .. import util
import sys
import unittest
import importlib


class ParentModuleTests(unittest.TestCase):

    """Importing a submodule should import the parent modules."""

    def test_import_parent(self):
        with util.mock_modules('pkg.__init__', 'pkg.module') as mock:
            with util.import_state(meta_path=[mock]):
                module = util.import_('pkg.module')
                self.assert_('pkg' in sys.modules)

    def test_bad_parent(self):
        with util.mock_modules('pkg.module') as mock:
            with util.import_state(meta_path=[mock]):
                self.assertRaises(ImportError, util.import_, 'pkg.module')


def test_main():
    from test.support import run_unittest
    run_unittest(ParentModuleTests)


if __name__ == '__main__':
    test_main()
