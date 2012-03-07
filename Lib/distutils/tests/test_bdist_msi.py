"""Tests for distutils.command.bdist_msi."""
import sys
import unittest
from test.test_support import run_unittest
from distutils.tests import support


@unittest.skipUnless(sys.platform == 'win32', 'these tests require Windows')
class BDistMSITestCase(support.TempdirManager,
                       support.LoggingSilencer,
                       unittest.TestCase):

    def test_minimal(self):
        # minimal test XXX need more tests
        from distutils.command.bdist_msi import bdist_msi
        project_dir, dist = self.create_dist()
        cmd = bdist_msi(dist)
        cmd.ensure_finalized()


def test_suite():
    return unittest.makeSuite(BDistMSITestCase)

if __name__ == '__main__':
    run_unittest(test_suite())
