"""Test that the semantics relating to the 'fromlist' argument are correct."""
from .. import util
from . import util as import_util
import unittest

class ReturnValue(unittest.TestCase):

    """The use of fromlist influences what import returns.

    If direct ``import ...`` statement is used, the root module or package is
    returned [import return]. But if fromlist is set, then the specified module
    is actually returned (whether it is a relative import or not)
    [from return].

    """

    def test_return_from_import(self):
        # [import return]
        with util.mock_modules('pkg.__init__', 'pkg.module') as importer:
            with util.import_state(meta_path=[importer]):
                module = import_util.import_('pkg.module')
                self.assertEqual(module.__name__, 'pkg')

    def test_return_from_from_import(self):
        # [from return]
        with util.mock_modules('pkg.__init__', 'pkg.module')as importer:
            with util.import_state(meta_path=[importer]):
                module = import_util.import_('pkg.module', fromlist=['attr'])
                self.assertEqual(module.__name__, 'pkg.module')


class HandlingFromlist(unittest.TestCase):

    """Using fromlist triggers different actions based on what is being asked
    of it.

    If fromlist specifies an object on a module, nothing special happens
    [object case]. This is even true if the object does not exist [bad object].

    If a package is being imported, then what is listed in fromlist may be
    treated as a module to be imported [module]. But once again, even if
    something in fromlist does not exist as a module, no error is thrown
    [no module]. And this extends to what is contained in __all__ when '*' is
    imported [using *]. And '*' does not need to be the only name in the
    fromlist [using * with others].

    """

    def test_object(self):
        # [object case]
        with util.mock_modules('module') as importer:
            with util.import_state(meta_path=[importer]):
                module = import_util.import_('module', fromlist=['attr'])
                self.assertEqual(module.__name__, 'module')

    def test_unexistent_object(self):
        # [bad object]
        with util.mock_modules('module') as importer:
            with util.import_state(meta_path=[importer]):
                module = import_util.import_('module', fromlist=['non_existent'])
                self.assertEqual(module.__name__, 'module')
                self.assertTrue(not hasattr(module, 'non_existent'))

    def test_module_from_package(self):
        # [module]
        with util.mock_modules('pkg.__init__', 'pkg.module') as importer:
            with util.import_state(meta_path=[importer]):
                module = import_util.import_('pkg', fromlist=['module'])
                self.assertEqual(module.__name__, 'pkg')
                self.assertTrue(hasattr(module, 'module'))
                self.assertEqual(module.module.__name__, 'pkg.module')

    def test_no_module_from_package(self):
        # [no module]
        with util.mock_modules('pkg.__init__') as importer:
            with util.import_state(meta_path=[importer]):
                module = import_util.import_('pkg', fromlist='non_existent')
                self.assertEqual(module.__name__, 'pkg')
                self.assertTrue(not hasattr(module, 'non_existent'))

    def test_empty_string(self):
        with util.mock_modules('pkg.__init__', 'pkg.mod') as importer:
            with util.import_state(meta_path=[importer]):
                module = import_util.import_('pkg.mod', fromlist=[''])
                self.assertEqual(module.__name__, 'pkg.mod')

    def test_using_star(self):
        # [using *]
        with util.mock_modules('pkg.__init__', 'pkg.module') as mock:
            with util.import_state(meta_path=[mock]):
                mock['pkg'].__all__ = ['module']
                module = import_util.import_('pkg', fromlist=['*'])
                self.assertEqual(module.__name__, 'pkg')
                self.assertTrue(hasattr(module, 'module'))
                self.assertEqual(module.module.__name__, 'pkg.module')

    def test_star_with_others(self):
        # [using * with others]
        context = util.mock_modules('pkg.__init__', 'pkg.module1', 'pkg.module2')
        with context as mock:
            with util.import_state(meta_path=[mock]):
                mock['pkg'].__all__ = ['module1']
                module = import_util.import_('pkg', fromlist=['module2', '*'])
                self.assertEqual(module.__name__, 'pkg')
                self.assertTrue(hasattr(module, 'module1'))
                self.assertTrue(hasattr(module, 'module2'))
                self.assertEqual(module.module1.__name__, 'pkg.module1')
                self.assertEqual(module.module2.__name__, 'pkg.module2')


def test_main():
    from test.support import run_unittest
    run_unittest(ReturnValue, HandlingFromlist)

if __name__ == '__main__':
    test_main()
