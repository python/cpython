"""Tests for distutils.command.config."""
import os
import sys
import logging

from packaging.command.config import dump_file, config
from packaging.tests import unittest, support


class ConfigTestCase(support.LoggingCatcher,
                     support.TempdirManager,
                     unittest.TestCase):

    def test_dump_file(self):
        this_file = __file__.rstrip('co')
        with open(this_file) as f:
            numlines = len(f.readlines())

        dump_file(this_file, 'I am the header')

        logs = []
        for log in self.get_logs(logging.INFO):
            logs.extend(line for line in log.split('\n'))
        self.assertEqual(len(logs), numlines + 2)

    @unittest.skipIf(sys.platform == 'win32', 'disabled on win32')
    def test_search_cpp(self):
        pkg_dir, dist = self.create_dist()
        cmd = config(dist)

        # simple pattern searches
        match = cmd.search_cpp(pattern='xxx', body='// xxx')
        self.assertEqual(match, 0)

        match = cmd.search_cpp(pattern='_configtest', body='// xxx')
        self.assertEqual(match, 1)

    def test_finalize_options(self):
        # finalize_options does a bit of transformation
        # on options
        pkg_dir, dist = self.create_dist()
        cmd = config(dist)
        cmd.include_dirs = 'one%stwo' % os.pathsep
        cmd.libraries = 'one'
        cmd.library_dirs = 'three%sfour' % os.pathsep
        cmd.ensure_finalized()

        self.assertEqual(cmd.include_dirs, ['one', 'two'])
        self.assertEqual(cmd.libraries, ['one'])
        self.assertEqual(cmd.library_dirs, ['three', 'four'])

    def test_clean(self):
        # _clean removes files
        tmp_dir = self.mkdtemp()
        f1 = os.path.join(tmp_dir, 'one')
        f2 = os.path.join(tmp_dir, 'two')

        self.write_file(f1, 'xxx')
        self.write_file(f2, 'xxx')

        for f in (f1, f2):
            self.assertTrue(os.path.exists(f))

        pkg_dir, dist = self.create_dist()
        cmd = config(dist)
        cmd._clean(f1, f2)

        for f in (f1, f2):
            self.assertFalse(os.path.exists(f))


def test_suite():
    return unittest.makeSuite(ConfigTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
