"""Tests for distutils.util."""
import sys
import unittest
from test.test_support import run_unittest

from distutils.errors import DistutilsByteCompileError
from distutils.tests import support
from distutils.util import byte_compile, grok_environment_error


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


def test_suite():
    return unittest.makeSuite(UtilTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
