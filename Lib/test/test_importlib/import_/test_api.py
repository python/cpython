from .. import util
from . import util as import_util
import sys
import types
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


class APITest:

    """Test API-specific details for __import__ (e.g. raising the right
    exception when passing in an int for the module name)."""

    def test_name_requires_rparition(self):
        # Raise TypeError if a non-string is passed in for the module name.
        with self.assertRaises(TypeError):
            self.__import__(42)

    def test_negative_level(self):
        # Raise ValueError when a negative level is specified.
        # PEP 328 did away with sys.module None entries and the ambiguity of
        # absolute/relative imports.
        with self.assertRaises(ValueError):
            self.__import__('os', globals(), level=-1)

    def test_nonexistent_fromlist_entry(self):
        # If something in fromlist doesn't exist, that's okay.
        # issue15715
        mod = types.ModuleType('fine')
        mod.__path__ = ['XXX']
        with util.import_state(meta_path=[BadLoaderFinder]):
            with util.uncache('fine'):
                sys.modules['fine'] = mod
                self.__import__('fine', fromlist=['not here'])

    def test_fromlist_load_error_propagates(self):
        # If something in fromlist triggers an exception not related to not
        # existing, let that exception propagate.
        # issue15316
        mod = types.ModuleType('fine')
        mod.__path__ = ['XXX']
        with util.import_state(meta_path=[BadLoaderFinder]):
            with util.uncache('fine'):
                sys.modules['fine'] = mod
                with self.assertRaises(ImportError):
                    self.__import__('fine', fromlist=['bogus'])

Frozen_APITests, Source_APITests = util.test_both(
        APITest, __import__=import_util.__import__)


if __name__ == '__main__':
    unittest.main()
