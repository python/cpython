"""PEP 366 ("Main module explicit relative imports") specifies the
semantics for the __package__ attribute on modules. This attribute is
used, when available, to detect which package a module belongs to (instead
of using the typical __path__/__name__ test).

"""
import unittest
import warnings
from test.test_importlib import util


class Using__package__:

    """Use of __package__ supersedes the use of __name__/__path__ to calculate
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

    def import_module(self, globals_):
        with self.mock_modules('pkg.__init__', 'pkg.fake') as importer:
            with util.import_state(meta_path=[importer]):
                self.__import__('pkg.fake')
                module = self.__import__('',
                                         globals=globals_,
                                         fromlist=['attr'], level=2)
        return module

    def test_using___package__(self):
        # [__package__]
        module = self.import_module({'__package__': 'pkg.fake'})
        self.assertEqual(module.__name__, 'pkg')

    def test_using___name__(self):
        # [__name__]
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            module = self.import_module({'__name__': 'pkg.fake',
                                         '__path__': []})
        self.assertEqual(module.__name__, 'pkg')

    def test_warn_when_using___name__(self):
        with self.assertWarns(ImportWarning):
            self.import_module({'__name__': 'pkg.fake', '__path__': []})

    def test_None_as___package__(self):
        # [None]
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            module = self.import_module({
                '__name__': 'pkg.fake', '__path__': [], '__package__': None })
        self.assertEqual(module.__name__, 'pkg')

    def test_spec_fallback(self):
        # If __package__ isn't defined, fall back on __spec__.parent.
        module = self.import_module({'__spec__': FakeSpec('pkg.fake')})
        self.assertEqual(module.__name__, 'pkg')

    def test_warn_when_package_and_spec_disagree(self):
        # Raise an ImportWarning if __package__ != __spec__.parent.
        with self.assertWarns(ImportWarning):
            self.import_module({'__package__': 'pkg.fake',
                                '__spec__': FakeSpec('pkg.fakefake')})

    def test_bad__package__(self):
        globals = {'__package__': '<not real>'}
        with self.assertRaises(ModuleNotFoundError):
            self.__import__('', globals, {}, ['relimport'], 1)

    def test_bunk__package__(self):
        globals = {'__package__': 42}
        with self.assertRaises(TypeError):
            self.__import__('', globals, {}, ['relimport'], 1)


class FakeSpec:
    def __init__(self, parent):
        self.parent = parent


class Using__package__PEP302(Using__package__):
    mock_modules = util.mock_modules

    def test_using___package__(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ImportWarning)
            super().test_using___package__()

    def test_spec_fallback(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ImportWarning)
            super().test_spec_fallback()


(Frozen_UsingPackagePEP302,
 Source_UsingPackagePEP302
 ) = util.test_both(Using__package__PEP302, __import__=util.__import__)


class Using__package__PEP451(Using__package__):
    mock_modules = util.mock_spec


(Frozen_UsingPackagePEP451,
 Source_UsingPackagePEP451
 ) = util.test_both(Using__package__PEP451, __import__=util.__import__)


class Setting__package__:

    """Because __package__ is a new feature, it is not always set by a loader.
    Import will set it as needed to help with the transition to relying on
    __package__.

    For a top-level module, __package__ is set to None [top-level]. For a
    package __name__ is used for __package__ [package]. For submodules the
    value is __name__.rsplit('.', 1)[0] [submodule].

    """

    __import__ = util.__import__['Source']

    # [top-level]
    def test_top_level(self):
        with self.mock_modules('top_level') as mock:
            with util.import_state(meta_path=[mock]):
                del mock['top_level'].__package__
                module = self.__import__('top_level')
                self.assertEqual(module.__package__, '')

    # [package]
    def test_package(self):
        with self.mock_modules('pkg.__init__') as mock:
            with util.import_state(meta_path=[mock]):
                del mock['pkg'].__package__
                module = self.__import__('pkg')
                self.assertEqual(module.__package__, 'pkg')

    # [submodule]
    def test_submodule(self):
        with self.mock_modules('pkg.__init__', 'pkg.mod') as mock:
            with util.import_state(meta_path=[mock]):
                del mock['pkg.mod'].__package__
                pkg = self.__import__('pkg.mod')
                module = getattr(pkg, 'mod')
                self.assertEqual(module.__package__, 'pkg')

class Setting__package__PEP302(Setting__package__, unittest.TestCase):
    mock_modules = util.mock_modules

    def test_top_level(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ImportWarning)
            super().test_top_level()

    def test_package(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ImportWarning)
            super().test_package()

    def test_submodule(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ImportWarning)
            super().test_submodule()

class Setting__package__PEP451(Setting__package__, unittest.TestCase):
    mock_modules = util.mock_spec


if __name__ == '__main__':
    unittest.main()
