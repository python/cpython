"""Tests for distutils.command.bdist."""

from packaging import util
from packaging.command.bdist import bdist, show_formats

from packaging.tests import unittest, support, captured_stdout


class BuildTestCase(support.TempdirManager,
                    support.LoggingCatcher,
                    unittest.TestCase):

    def _mock_get_platform(self):
        self._get_platform_called = True
        return self._get_platform()

    def setUp(self):
        super(BuildTestCase, self).setUp()

        # mock util.get_platform
        self._get_platform_called = False
        self._get_platform = util.get_platform
        util.get_platform = self._mock_get_platform

    def tearDown(self):
        super(BuildTestCase, self).tearDown()
        util.get_platform = self._get_platform

    def test_formats(self):

        # let's create a command and make sure
        # we can fix the format
        pkg_pth, dist = self.create_dist()
        cmd = bdist(dist)
        cmd.formats = ['msi']
        cmd.ensure_finalized()
        self.assertEqual(cmd.formats, ['msi'])

        # what format bdist offers ?
        # XXX an explicit list in bdist is
        # not the best way to  bdist_* commands
        # we should add a registry
        formats = sorted(('zip', 'gztar', 'bztar', 'ztar',
                          'tar', 'wininst', 'msi'))
        found = sorted(cmd.format_command)
        self.assertEqual(found, formats)

    def test_skip_build(self):
        pkg_pth, dist = self.create_dist()
        cmd = bdist(dist)
        cmd.skip_build = False
        cmd.formats = ['ztar']
        cmd.ensure_finalized()
        self.assertFalse(self._get_platform_called)

        pkg_pth, dist = self.create_dist()
        cmd = bdist(dist)
        cmd.skip_build = True
        cmd.formats = ['ztar']
        cmd.ensure_finalized()
        self.assertTrue(self._get_platform_called)

    def test_show_formats(self):
        __, stdout = captured_stdout(show_formats)

        # the output should be a header line + one line per format
        num_formats = len(bdist.format_commands)
        output = [line for line in stdout.split('\n')
                  if line.strip().startswith('--formats=')]
        self.assertEqual(len(output), num_formats)


def test_suite():
    return unittest.makeSuite(BuildTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
