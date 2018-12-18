"""Tests for distutils.util."""
import os
import sys
import unittest
from test.test_support import run_unittest, swap_attr

from distutils.errors import DistutilsByteCompileError
from distutils.tests import support
from distutils import util # used to patch _environ_checked
from distutils.util import (byte_compile, grok_environment_error,
                            check_environ, get_platform)


class UtilTestCase(support.EnvironGuard, unittest.TestCase):

    def test_dont_write_bytecode(self):
        # makes sure byte_compile raise a DistutilsError
        # if sys.dont_write_bytecode is True
        old_dont_write_bytecode = sys.dont_write_bytecode
        sys.dont_write_bytecode = True
        try:
            self.assertRaises(DistutilsByteCompileError, byte_compile, [])
        finally:
            sys.dont_write_bytecode = old_dont_write_bytecode

    def test_grok_environment_error(self):
        # test obsolete function to ensure backward compat (#4931)
        exc = IOError("Unable to find batch file")
        msg = grok_environment_error(exc)
        self.assertEqual(msg, "error: Unable to find batch file")

    def test_check_environ(self):
        util._environ_checked = 0
        os.environ.pop('HOME', None)

        check_environ()

        self.assertEqual(os.environ['PLAT'], get_platform())
        self.assertEqual(util._environ_checked, 1)

    @unittest.skipUnless(os.name == 'posix', 'specific to posix')
    def test_check_environ_getpwuid(self):
        util._environ_checked = 0
        os.environ.pop('HOME', None)

        import pwd

        # only set pw_dir field, other fields are not used
        def mock_getpwuid(uid):
            return pwd.struct_passwd((None, None, None, None, None,
                                      '/home/distutils', None))

        with swap_attr(pwd, 'getpwuid', mock_getpwuid):
            check_environ()
            self.assertEqual(os.environ['HOME'], '/home/distutils')

        util._environ_checked = 0
        os.environ.pop('HOME', None)

        # bpo-10496: Catch pwd.getpwuid() error
        def getpwuid_err(uid):
            raise KeyError
        with swap_attr(pwd, 'getpwuid', getpwuid_err):
            check_environ()
            self.assertNotIn('HOME', os.environ)


def test_suite():
    return unittest.makeSuite(UtilTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
