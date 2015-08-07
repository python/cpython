"""Tests for distutils._msvccompiler."""
import sys
import unittest
import os

from distutils.errors import DistutilsPlatformError
from distutils.tests import support
from test.support import run_unittest


SKIP_MESSAGE = (None if sys.platform == "win32" else
                "These tests are only for win32")

@unittest.skipUnless(SKIP_MESSAGE is None, SKIP_MESSAGE)
class msvccompilerTestCase(support.TempdirManager,
                            unittest.TestCase):

    def test_no_compiler(self):
        # makes sure query_vcvarsall raises
        # a DistutilsPlatformError if the compiler
        # is not found
        from distutils._msvccompiler import _get_vc_env
        def _find_vcvarsall():
            return None

        import distutils._msvccompiler as _msvccompiler
        old_find_vcvarsall = _msvccompiler._find_vcvarsall
        _msvccompiler._find_vcvarsall = _find_vcvarsall
        try:
            self.assertRaises(DistutilsPlatformError, _get_vc_env,
                             'wont find this version')
        finally:
            _msvccompiler._find_vcvarsall = old_find_vcvarsall

def test_suite():
    return unittest.makeSuite(msvccompilerTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
