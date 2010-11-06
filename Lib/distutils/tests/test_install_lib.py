"""Tests for distutils.command.install_data."""
import sys
import os
import unittest

from distutils.command.install_lib import install_lib
from distutils.extension import Extension
from distutils.tests import support
from distutils.errors import DistutilsOptionError
from test.support import run_unittest

class InstallLibTestCase(support.TempdirManager,
                         support.LoggingSilencer,
                         support.EnvironGuard,
                         unittest.TestCase):

    def test_finalize_options(self):
        pkg_dir, dist = self.create_dist()
        cmd = install_lib(dist)

        cmd.finalize_options()
        self.assertEquals(cmd.compile, 1)
        self.assertEquals(cmd.optimize, 0)

        # optimize must be 0, 1, or 2
        cmd.optimize = 'foo'
        self.assertRaises(DistutilsOptionError, cmd.finalize_options)
        cmd.optimize = '4'
        self.assertRaises(DistutilsOptionError, cmd.finalize_options)

        cmd.optimize = '2'
        cmd.finalize_options()
        self.assertEquals(cmd.optimize, 2)

    @unittest.skipUnless(not sys.dont_write_bytecode,
                         'byte-compile not supported')
    def test_byte_compile(self):
        pkg_dir, dist = self.create_dist()
        cmd = install_lib(dist)
        cmd.compile = cmd.optimize = 1

        f = os.path.join(pkg_dir, 'foo.py')
        self.write_file(f, '# python file')
        cmd.byte_compile([f])
        self.assertTrue(os.path.exists(os.path.join(pkg_dir, 'foo.pyc')))
        self.assertTrue(os.path.exists(os.path.join(pkg_dir, 'foo.pyo')))

    def test_get_outputs(self):
        pkg_dir, dist = self.create_dist()
        cmd = install_lib(dist)

        # setting up a dist environment
        cmd.compile = cmd.optimize = 1
        cmd.install_dir = pkg_dir
        f = os.path.join(pkg_dir, 'foo.py')
        self.write_file(f, '# python file')
        cmd.distribution.py_modules = [pkg_dir]
        cmd.distribution.ext_modules = [Extension('foo', ['xxx'])]
        cmd.distribution.packages = [pkg_dir]
        cmd.distribution.script_name = 'setup.py'

        # get_output should return 4 elements
        self.assertTrue(len(cmd.get_outputs()) >= 2)

    def test_get_inputs(self):
        pkg_dir, dist = self.create_dist()
        cmd = install_lib(dist)

        # setting up a dist environment
        cmd.compile = cmd.optimize = 1
        cmd.install_dir = pkg_dir
        f = os.path.join(pkg_dir, 'foo.py')
        self.write_file(f, '# python file')
        cmd.distribution.py_modules = [pkg_dir]
        cmd.distribution.ext_modules = [Extension('foo', ['xxx'])]
        cmd.distribution.packages = [pkg_dir]
        cmd.distribution.script_name = 'setup.py'

        # get_input should return 2 elements
        self.assertEquals(len(cmd.get_inputs()), 2)

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
    run_unittest(test_suite())
