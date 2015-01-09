from importlib import machinery
import sys
import types
import unittest

from .. import util


class SpecLoaderMock:

    def find_spec(self, fullname, path=None, target=None):
        return machinery.ModuleSpec(fullname, self)

    def create_module(self, spec):
        return None

    def exec_module(self, module):
        pass


class SpecLoaderAttributeTests:

    def test___loader__(self):
        loader = SpecLoaderMock()
        with util.uncache('blah'), util.import_state(meta_path=[loader]):
            module = self.__import__('blah')
        self.assertEqual(loader, module.__loader__)


(Frozen_SpecTests,
 Source_SpecTests
 ) = util.test_both(SpecLoaderAttributeTests, __import__=util.__import__)


class LoaderMock:

    def find_module(self, fullname, path=None):
        return self

    def load_module(self, fullname):
        sys.modules[fullname] = self.module
        return self.module


class LoaderAttributeTests:

    def test___loader___missing(self):
        module = types.ModuleType('blah')
        try:
            del module.__loader__
        except AttributeError:
            pass
        loader = LoaderMock()
        loader.module = module
        with util.uncache('blah'), util.import_state(meta_path=[loader]):
            module = self.__import__('blah')
        self.assertEqual(loader, module.__loader__)

    def test___loader___is_None(self):
        module = types.ModuleType('blah')
        module.__loader__ = None
        loader = LoaderMock()
        loader.module = module
        with util.uncache('blah'), util.import_state(meta_path=[loader]):
            returned_module = self.__import__('blah')
        self.assertEqual(loader, module.__loader__)


(Frozen_Tests,
 Source_Tests
 ) = util.test_both(LoaderAttributeTests, __import__=util.__import__)


if __name__ == '__main__':
    unittest.main()
