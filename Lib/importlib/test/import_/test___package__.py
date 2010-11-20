"""PEP 366 ("Main module explicit relative imports") specifies the
semantics for the __package__ attribute on modules. This attribute is
used, when available, to detect which package a module belongs to (instead
of using the typical __path__/__name__ test).

"""
import unittest
from .. import util
from . import util as import_util


class Using__package__(unittest.TestCase):

    """Use of __package__ supercedes the use of __name__/__path__ to calculate
    what package a module belongs to. The basic algorithm is [__package__]::

      def resolve_name(name, package, level):
          level -= 1
          base = package.rsplit('.', level)[0]
          return '{0}.{1}'.format(base, name)

    But since there is no guarantee that __package__ has been set (or not been
    set to None [None]), there has to be a way to calculate the attribute's value
    [__name__]::

      def calc_package(caller_name, has___path__):
          if has__path__:
              return caller_name
          else:
              return caller_name.rsplit('.', 1)[0]

    Then the normal algorithm for relative name imports can proceed as if
    __package__ had been set.

    """

    def test_using___package__(self):
        # [__package__]
        with util.mock_modules('pkg.__init__', 'pkg.fake') as importer:
            with util.import_state(meta_path=[importer]):
                import_util.import_('pkg.fake')
                module = import_util.import_('',
                                            globals={'__package__': 'pkg.fake'},
                                            fromlist=['attr'], level=2)
        self.assertEqual(module.__name__, 'pkg')

    def test_using___name__(self, package_as_None=False):
        # [__name__]
        globals_ = {'__name__': 'pkg.fake', '__path__': []}
        if package_as_None:
            globals_['__package__'] = None
        with util.mock_modules('pkg.__init__', 'pkg.fake') as importer:
            with util.import_state(meta_path=[importer]):
                import_util.import_('pkg.fake')
                module = import_util.import_('', globals= globals_,
                                                fromlist=['attr'], level=2)
            self.assertEqual(module.__name__, 'pkg')

    def test_None_as___package__(self):
        # [None]
        self.test_using___name__(package_as_None=True)

    def test_bad__package__(self):
        globals = {'__package__': '<not real>'}
        with self.assertRaises(SystemError):
            import_util.import_('', globals, {}, ['relimport'], 1)

    def test_bunk__package__(self):
        globals = {'__package__': 42}
        with self.assertRaises(ValueError):
            import_util.import_('', globals, {}, ['relimport'], 1)


@import_util.importlib_only
class Setting__package__(unittest.TestCase):

    """Because __package__ is a new feature, it is not always set by a loader.
    Import will set it as needed to help with the transition to relying on
    __package__.

    For a top-level module, __package__ is set to None [top-level]. For a
    package __name__ is used for __package__ [package]. For submodules the
    value is __name__.rsplit('.', 1)[0] [submodule].

    """

    # [top-level]
    def test_top_level(self):
        with util.mock_modules('top_level') as mock:
            with util.import_state(meta_path=[mock]):
                del mock['top_level'].__package__
                module = import_util.import_('top_level')
                self.assertEqual(module.__package__, '')

    # [package]
    def test_package(self):
        with util.mock_modules('pkg.__init__') as mock:
            with util.import_state(meta_path=[mock]):
                del mock['pkg'].__package__
                module = import_util.import_('pkg')
                self.assertEqual(module.__package__, 'pkg')

    # [submodule]
    def test_submodule(self):
        with util.mock_modules('pkg.__init__', 'pkg.mod') as mock:
            with util.import_state(meta_path=[mock]):
                del mock['pkg.mod'].__package__
                pkg = import_util.import_('pkg.mod')
                module = getattr(pkg, 'mod')
                self.assertEqual(module.__package__, 'pkg')


def test_main():
    from test.support import run_unittest
    run_unittest(Using__package__, Setting__package__)


if __name__ == '__main__':
    test_main()
