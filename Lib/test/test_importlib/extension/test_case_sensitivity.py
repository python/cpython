from importlib import _bootstrap
import sys
from test import support
import unittest

from .. import util
from . import util as ext_util

frozen_machinery, source_machinery = util.import_importlib('importlib.machinery')


# XXX find_spec tests

@unittest.skipIf(ext_util.FILENAME is None, '_testcapi not available')
@util.case_insensitive_tests
class ExtensionModuleCaseSensitivityTest:

    def find_module(self):
        good_name = ext_util.NAME
        bad_name = good_name.upper()
        assert good_name != bad_name
        finder = self.machinery.FileFinder(ext_util.PATH,
                                          (self.machinery.ExtensionFileLoader,
                                           self.machinery.EXTENSION_SUFFIXES))
        return finder.find_module(bad_name)

    def test_case_sensitive(self):
        with support.EnvironmentVarGuard() as env:
            env.unset('PYTHONCASEOK')
            if b'PYTHONCASEOK' in _bootstrap._os.environ:
                self.skipTest('os.environ changes not reflected in '
                              '_os.environ')
            loader = self.find_module()
            self.assertIsNone(loader)

    def test_case_insensitivity(self):
        with support.EnvironmentVarGuard() as env:
            env.set('PYTHONCASEOK', '1')
            if b'PYTHONCASEOK' not in _bootstrap._os.environ:
                self.skipTest('os.environ changes not reflected in '
                              '_os.environ')
            loader = self.find_module()
            self.assertTrue(hasattr(loader, 'load_module'))

Frozen_ExtensionCaseSensitivity, Source_ExtensionCaseSensitivity = util.test_both(
        ExtensionModuleCaseSensitivityTest,
        machinery=[frozen_machinery, source_machinery])


if __name__ == '__main__':
    unittest.main()
