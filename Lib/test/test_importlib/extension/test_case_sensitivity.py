from importlib import _bootstrap_external
from test import support
import unittest

from .. import util

importlib = util.import_importlib('importlib')
machinery = util.import_importlib('importlib.machinery')


@unittest.skipIf(util.EXTENSIONS.filename is None, '_testcapi not available')
@util.case_insensitive_tests
class ExtensionModuleCaseSensitivityTest(util.CASEOKTestBase):

    def find_module(self):
        good_name = util.EXTENSIONS.name
        bad_name = good_name.upper()
        assert good_name != bad_name
        finder = self.machinery.FileFinder(util.EXTENSIONS.path,
                                          (self.machinery.ExtensionFileLoader,
                                           self.machinery.EXTENSION_SUFFIXES))
        return finder.find_module(bad_name)

    def test_case_sensitive(self):
        with support.EnvironmentVarGuard() as env:
            env.unset('PYTHONCASEOK')
            self.caseok_env_changed(should_exist=False)
            loader = self.find_module()
            self.assertIsNone(loader)

    def test_case_insensitivity(self):
        with support.EnvironmentVarGuard() as env:
            env.set('PYTHONCASEOK', '1')
            self.caseok_env_changed(should_exist=True)
            loader = self.find_module()
            self.assertTrue(hasattr(loader, 'load_module'))


(Frozen_ExtensionCaseSensitivity,
 Source_ExtensionCaseSensitivity
 ) = util.test_both(ExtensionModuleCaseSensitivityTest, importlib=importlib,
                    machinery=machinery)


if __name__ == '__main__':
    unittest.main()
