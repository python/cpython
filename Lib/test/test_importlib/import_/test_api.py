from .. import util as importlib_test_util
from . import util
import imp
import sys
import unittest


class BadLoaderFinder:
    bad = 'fine.bogus'
    @classmethod
    def find_module(cls, fullname, path):
        if fullname == cls.bad:
            return cls
    @classmethod
    def load_module(cls, fullname):
        if fullname == cls.bad:
            raise ImportError('I cannot be loaded!')


class APITest(unittest.TestCase):

    """Test API-specific details for __import__ (e.g. raising the right
    exception when passing in an int for the module name)."""

    def test_name_requires_rparition(self):
        # Raise TypeError if a non-string is passed in for the module name.
        with self.assertRaises(TypeError):
            util.import_(42)

    def test_negative_level(self):
        # Raise ValueError when a negative level is specified.
        # PEP 328 did away with sys.module None entries and the ambiguity of
        # absolute/relative imports.
        with self.assertRaises(ValueError):
            util.import_('os', globals(), level=-1)

    def test_nonexistent_fromlist_entry(self):
        # If something in fromlist doesn't exist, that's okay.
        # issue15715
        mod = imp.new_module('fine')
        mod.__path__ = ['XXX']
        with importlib_test_util.import_state(meta_path=[BadLoaderFinder]):
            with importlib_test_util.uncache('fine'):
                sys.modules['fine'] = mod
                util.import_('fine', fromlist=['not here'])

    def test_fromlist_load_error_propagates(self):
        # If something in fromlist triggers an exception not related to not
        # existing, let that exception propagate.
        # issue15316
        mod = imp.new_module('fine')
        mod.__path__ = ['XXX']
        with importlib_test_util.import_state(meta_path=[BadLoaderFinder]):
            with importlib_test_util.uncache('fine'):
                sys.modules['fine'] = mod
                with self.assertRaises(ImportError):
                    util.import_('fine', fromlist=['bogus'])



def test_main():
    from test.support import run_unittest
    run_unittest(APITest)


if __name__ == '__main__':
    test_main()
