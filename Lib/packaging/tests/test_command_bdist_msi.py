"""Tests for distutils.command.bdist_msi."""
import sys

from packaging.tests import unittest, support


class BDistMSITestCase(support.TempdirManager,
                       support.LoggingCatcher,
                       unittest.TestCase):

    @unittest.skipUnless(sys.platform == "win32", "runs only on win32")
    def test_minimal(self):
        # minimal test XXX need more tests
        from packaging.command.bdist_msi import bdist_msi
        pkg_pth, dist = self.create_dist()
        cmd = bdist_msi(dist)
        cmd.ensure_finalized()


def test_suite():
    return unittest.makeSuite(BDistMSITestCase)


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
