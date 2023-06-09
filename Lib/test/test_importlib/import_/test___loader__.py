from importlib import machinery
import sys
import types
import unittest
import warnings

from test.test_importlib import util


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


if __name__ == '__main__':
    unittest.main()
