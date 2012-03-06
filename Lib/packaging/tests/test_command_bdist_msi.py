"""Tests for distutils.command.bdist_msi."""
import os
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
        cmd.run()

        bdists = os.listdir(os.path.join(project_dir, 'dist'))
        self.assertEqual(bdists, ['foo-0.1.msi'])

        # bug #13719: upload ignores bdist_msi files
        self.assertEqual(dist.dist_files,
                         [('bdist_msi', 'any', 'dist/foo-0.1.msi')])


def test_suite():
    return unittest.makeSuite(BDistMSITestCase)


if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
