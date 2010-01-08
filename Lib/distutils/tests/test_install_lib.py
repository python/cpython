"""Tests for distutils.command.install_data."""
import sys
import os
import unittest

from distutils.command.install_lib import install_lib
from distutils.extension import Extension
from distutils.tests import support
from distutils.errors import DistutilsOptionError

class InstallLibTestCase(support.TempdirManager,
                         support.LoggingSilencer,
                         unittest.TestCase):

    def test_dont_write_bytecode(self):
        # makes sure byte_compile is not used
        pkg_dir, dist = self.create_dist()
        cmd = install_lib(dist)
        cmd.compile = 1
        cmd.optimize = 1

        old_dont_write_bytecode = sys.dont_write_bytecode
        sys.dont_write_bytecode = True
        try:
            cmd.byte_compile([])
        finally:
            sys.dont_write_bytecode = old_dont_write_bytecode

        self.assertTrue('byte-compiling is disabled' in self.logs[0][1])

def test_suite():
    return unittest.makeSuite(InstallLibTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
