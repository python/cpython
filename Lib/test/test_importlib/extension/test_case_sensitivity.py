from importlib import _bootstrap_external
from test.support import os_helper
import unittest
import sys
from test.test_importlib import util

importlib = util.import_importlib('importlib')
machinery = util.import_importlib('importlib.machinery')


@unittest.skipIf(util.EXTENSIONS is None or util.EXTENSIONS.filename is None,
                 'dynamic loading not supported or test module not available')
@util.case_insensitive_tests
class ExtensionModuleCaseSensitivityTest(util.CASEOKTestBase):

    def find_spec(self):
        good_name = util.EXTENSIONS.name
        bad_name = good_name.upper()
        assert good_name != bad_name
        finder = self.machinery.FileFinder(util.EXTENSIONS.path,
                                          (self.machinery.ExtensionFileLoader,
                                           self.machinery.EXTENSION_SUFFIXES))
        return finder.find_spec(bad_name)

    @unittest.skipIf(sys.flags.ignore_environment, 'ignore_environment flag was set')
    def test_case_sensitive(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.unset('PYTHONCASEOK')
            self.caseok_env_changed(should_exist=False)
            spec = self.find_spec()
            self.assertIsNone(spec)

    @unittest.skipIf(sys.flags.ignore_environment, 'ignore_environment flag was set')
    def test_case_insensitivity(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.set('PYTHONCASEOK', '1')
            self.caseok_env_changed(should_exist=True)
            spec = self.find_spec()
            self.assertTrue(spec)


(Frozen_ExtensionCaseSensitivity,
 Source_ExtensionCaseSensitivity
 ) = util.test_both(ExtensionModuleCaseSensitivityTest, importlib=importlib,
                    machinery=machinery)


if __name__ == '__main__':
    unittest.main()
