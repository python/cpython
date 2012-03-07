"""Tests for distutils.command.bdist_msi."""
import sys

from packaging.tests import unittest, support


@unittest.skipUnless(sys.platform == 'win32', 'these tests require Windows')
class BDistMSITestCase(support.TempdirManager,
                       support.LoggingCatcher,
                       unittest.TestCase):

    def test_minimal(self):
        # minimal test XXX need more tests
        from packaging.command.bdist_msi import bdist_msi
        project_dir, dist = self.create_dist()
        cmd = bdist_msi(dist)
        cmd.ensure_finalized()


def test_suite():
    return unittest.makeSuite(BDistMSITestCase)


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
