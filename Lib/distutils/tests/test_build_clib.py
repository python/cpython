"""Tests for distutils.command.build_clib."""
import unittest

from distutils.command.build_clib import build_clib
from distutils.errors import DistutilsSetupError
from distutils.tests import support

class BuildCLibTestCase(support.TempdirManager,
                        support.LoggingSilencer,
                        unittest.TestCase):

    def test_check_library_dist(self):
        pkg_dir, dist = self.create_dist()
        cmd = build_clib(dist)

        # 'libraries' option must be a list
        self.assertRaises(DistutilsSetupError, cmd.check_library_list, 'foo')

        # each element of 'libraries' must a 2-tuple
        self.assertRaises(DistutilsSetupError, cmd.check_library_list,
                          ['foo1', 'foo2'])

        # first element of each tuple in 'libraries'
        # must be a string (the library name)
        self.assertRaises(DistutilsSetupError, cmd.check_library_list,
                          [(1, 'foo1'), ('name', 'foo2')])

        # library name may not contain directory separators
        self.assertRaises(DistutilsSetupError, cmd.check_library_list,
                          [('name', 'foo1'),
                           ('another/name', 'foo2')])

        # second element of each tuple must be a dictionary (build info)
        self.assertRaises(DistutilsSetupError, cmd.check_library_list,
                          [('name', {}),
                           ('another', 'foo2')])

        # those work
        libs = [('name', {}), ('name', {'ok': 'good'})]
        cmd.check_library_list(libs)


def test_suite():
    return unittest.makeSuite(BuildCLibTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
