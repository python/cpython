"""Tests for distutils.command.bdist_msi."""
import unittest
import sys

from test.support import run_unittest

from distutils.tests import support

@unittest.skipUnless(sys.platform=="win32", "These tests are only for win32")
class BDistMSITestCase(support.TempdirManager,
                       support.LoggingSilencer,
                       unittest.TestCase):

    def test_minimal(self):
        # minimal test XXX need more tests
        from distutils.command.bdist_msi import bdist_msi
        pkg_pth, dist = self.create_dist()
        cmd = bdist_msi(dist)
        cmd.ensure_finalized()

def test_suite():
    return unittest.makeSuite(BDistMSITestCase)

if __name__ == '__main__':
    run_unittest(test_suite())
