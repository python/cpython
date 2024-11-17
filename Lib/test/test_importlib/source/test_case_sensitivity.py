"""Test case-sensitivity (PEP 235)."""
import sys

from test.test_importlib import util

importlib = util.import_importlib('importlib')
machinery = util.import_importlib('importlib.machinery')

import os
from test.support import os_helper
import unittest


@util.case_insensitive_tests
class CaseSensitivityTest(util.CASEOKTestBase):

    """PEP 235 dictates that on case-preserving, case-insensitive file systems
    that imports are case-sensitive unless the PYTHONCASEOK environment
    variable is set."""

    name = 'MoDuLe'
    assert name != name.lower()

    def finder(self, path):
        return self.machinery.FileFinder(path,
                                      (self.machinery.SourceFileLoader,
                                            self.machinery.SOURCE_SUFFIXES),
                                        (self.machinery.SourcelessFileLoader,
                                            self.machinery.BYTECODE_SUFFIXES))

    def sensitivity_test(self):
        """Look for a module with matching and non-matching sensitivity."""
        sensitive_pkg = 'sensitive.{0}'.format(self.name)
        insensitive_pkg = 'insensitive.{0}'.format(self.name.lower())
        context = util.create_modules(insensitive_pkg, sensitive_pkg)
        with context as mapping:
            sensitive_path = os.path.join(mapping['.root'], 'sensitive')
            insensitive_path = os.path.join(mapping['.root'], 'insensitive')
            sensitive_finder = self.finder(sensitive_path)
            insensitive_finder = self.finder(insensitive_path)
            return self.find(sensitive_finder), self.find(insensitive_finder)

    @unittest.skipIf(sys.flags.ignore_environment, 'ignore_environment flag was set')
    def test_sensitive(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.unset('PYTHONCASEOK')
            self.caseok_env_changed(should_exist=False)
            sensitive, insensitive = self.sensitivity_test()
            self.assertIsNotNone(sensitive)
            self.assertIn(self.name, sensitive.get_filename(self.name))
            self.assertIsNone(insensitive)

    @unittest.skipIf(sys.flags.ignore_environment, 'ignore_environment flag was set')
    def test_insensitive(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.set('PYTHONCASEOK', '1')
            self.caseok_env_changed(should_exist=True)
            sensitive, insensitive = self.sensitivity_test()
            self.assertIsNotNone(sensitive)
            self.assertIn(self.name, sensitive.get_filename(self.name))
            self.assertIsNotNone(insensitive)
            self.assertIn(self.name, insensitive.get_filename(self.name))


class CaseSensitivityTestPEP451(CaseSensitivityTest):
    def find(self, finder):
        found = finder.find_spec(self.name)
        return found.loader if found is not None else found


(Frozen_CaseSensitivityTestPEP451,
 Source_CaseSensitivityTestPEP451
 ) = util.test_both(CaseSensitivityTestPEP451, importlib=importlib,
                    machinery=machinery)


if __name__ == '__main__':
    unittest.main()
