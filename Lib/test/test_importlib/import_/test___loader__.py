import imp
import sys
import unittest

from .. import util
from . import util as import_util


class LoaderMock:

    def find_module(self, fullname, path=None):
        return self

    def load_module(self, fullname):
        sys.modules[fullname] = self.module
        return self.module


class LoaderAttributeTests(unittest.TestCase):

    def test___loader___missing(self):
        module = imp.new_module('blah')
        try:
            del module.__loader__
        except AttributeError:
            pass
        loader = LoaderMock()
        loader.module = module
        with util.uncache('blah'), util.import_state(meta_path=[loader]):
            module = import_util.import_('blah')
        self.assertEqual(loader, module.__loader__)

    def test___loader___is_None(self):
        module = imp.new_module('blah')
        module.__loader__ = None
        loader = LoaderMock()
        loader.module = module
        with util.uncache('blah'), util.import_state(meta_path=[loader]):
            returned_module = import_util.import_('blah')
        self.assertEqual(loader, module.__loader__)


if __name__ == '__main__':
    unittest.main()
