"""Tests for distutils.command.bdist_wininst."""
import unittest

from test.test_support import run_unittest

from distutils.command.bdist_wininst import bdist_wininst
from distutils.tests import support

class BuildWinInstTestCase(support.TempdirManager,
                           support.LoggingSilencer,
                           unittest.TestCase):

    def test_get_exe_bytes(self):

        # issue5731: command was broken on non-windows platforms
        # this test makes sure it works now for every platform
        # let's create a command
        pkg_pth, dist = self.create_dist()
        cmd = bdist_wininst(dist)
        cmd.ensure_finalized()

        # let's run the code that finds the right wininst*.exe file
        # and make sure it finds it and returns its content
        # no matter what platform we have
        exe_file = cmd.get_exe_bytes()
        self.assertTrue(len(exe_file) > 10)

def test_suite():
    return unittest.makeSuite(BuildWinInstTestCase)

if __name__ == '__main__':
    run_unittest(test_suite())
