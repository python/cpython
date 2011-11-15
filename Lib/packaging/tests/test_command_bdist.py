"""Tests for distutils.command.bdist."""
import os
from test.support import captured_stdout
from packaging.command.bdist import bdist, show_formats
from packaging.tests import unittest, support


class BuildTestCase(support.TempdirManager,
                    support.LoggingCatcher,
                    unittest.TestCase):

    def test_formats(self):
        # let's create a command and make sure
        # we can set the format
        dist = self.create_dist()[1]
        cmd = bdist(dist)
        cmd.formats = ['msi']
        cmd.ensure_finalized()
        self.assertEqual(cmd.formats, ['msi'])

        # what formats does bdist offer?
        # XXX hard-coded lists are not the best way to find available bdist_*
        # commands; we should add a registry
        formats = ['bztar', 'gztar', 'msi', 'tar', 'wininst', 'zip']
        found = sorted(cmd.format_command)
        self.assertEqual(found, formats)

    def test_skip_build(self):
        # bug #10946: bdist --skip-build should trickle down to subcommands
        dist = self.create_dist()[1]
        cmd = bdist(dist)
        cmd.skip_build = True
        cmd.ensure_finalized()
        dist.command_obj['bdist'] = cmd

        names = ['bdist_dumb', 'bdist_wininst']
        if os.name == 'nt':
            names.append('bdist_msi')

        for name in names:
            subcmd = cmd.get_finalized_command(name)
            self.assertTrue(subcmd.skip_build,
                            '%s should take --skip-build from bdist' % name)

    def test_show_formats(self):
        with captured_stdout() as stdout:
            show_formats()
        stdout = stdout.getvalue()

        # the output should be a header line + one line per format
        num_formats = len(bdist.format_commands)
        output = [line for line in stdout.split('\n')
                  if line.strip().startswith('--formats=')]
        self.assertEqual(len(output), num_formats)


def test_suite():
    return unittest.makeSuite(BuildTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
