"""Tests for distutils.msvc9compiler."""
import sys
import unittest

from distutils.errors import DistutilsPlatformError

class msvc9compilerTestCase(unittest.TestCase):

    def test_no_compiler(self):
        # makes sure query_vcvarsall throws
        # a DistutilsPlatformError if the compiler
        # is not found
        if sys.platform != 'win32':
            # this test is only for win32
            return
        from distutils.msvccompiler import get_build_version
        if get_build_version() < 8.0:
            # this test is only for MSVC8.0 or above
            return
        from distutils.msvc9compiler import query_vcvarsall
        def _find_vcvarsall(version):
            return None

        from distutils import msvc9compiler
        old_find_vcvarsall = msvc9compiler.find_vcvarsall
        msvc9compiler.find_vcvarsall = _find_vcvarsall
        try:
            self.assertRaises(DistutilsPlatformError, query_vcvarsall,
                             'wont find this version')
        finally:
            msvc9compiler.find_vcvarsall = old_find_vcvarsall

def test_suite():
    return unittest.makeSuite(msvc9compilerTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
