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
        import distutils._msvccompiler as _msvccompiler
        # makes sure query_vcvarsall raises
        # a DistutilsPlatformError if the compiler
        # is not found
        def _find_vcvarsall(plat_spec):
            return None, None

        old_find_vcvarsall = _msvccompiler._find_vcvarsall
        _msvccompiler._find_vcvarsall = _find_vcvarsall
        try:
            self.assertRaises(DistutilsPlatformError,
                              _msvccompiler._get_vc_env,
                             'wont find this version')
        finally:
            _msvccompiler._find_vcvarsall = old_find_vcvarsall

    def test_compiler_options(self):
        import distutils._msvccompiler as _msvccompiler
        # suppress path to vcruntime from _find_vcvarsall to
        # check that /MT is added to compile options
        old_find_vcvarsall = _msvccompiler._find_vcvarsall
        def _find_vcvarsall(plat_spec):
            return old_find_vcvarsall(plat_spec)[0], None
        _msvccompiler._find_vcvarsall = _find_vcvarsall
        try:
            compiler = _msvccompiler.MSVCCompiler()
            compiler.initialize()

            self.assertIn('/MT', compiler.compile_options)
            self.assertNotIn('/MD', compiler.compile_options)
        finally:
            _msvccompiler._find_vcvarsall = old_find_vcvarsall

    def test_vcruntime_copy(self):
        import distutils._msvccompiler as _msvccompiler
        # force path to a known file - it doesn't matter
        # what we copy as long as its name is not in
        # _msvccompiler._BUNDLED_DLLS
        old_find_vcvarsall = _msvccompiler._find_vcvarsall
        def _find_vcvarsall(plat_spec):
            return old_find_vcvarsall(plat_spec)[0], __file__
        _msvccompiler._find_vcvarsall = _find_vcvarsall
        try:
            tempdir = self.mkdtemp()
            compiler = _msvccompiler.MSVCCompiler()
            compiler.initialize()
            compiler._copy_vcruntime(tempdir)

            self.assertTrue(os.path.isfile(os.path.join(
                tempdir, os.path.basename(__file__))))
        finally:
            _msvccompiler._find_vcvarsall = old_find_vcvarsall

    def test_vcruntime_skip_copy(self):
        import distutils._msvccompiler as _msvccompiler

        tempdir = self.mkdtemp()
        compiler = _msvccompiler.MSVCCompiler()
        compiler.initialize()
        dll = compiler._vcruntime_redist
        self.assertTrue(os.path.isfile(dll))

        compiler._copy_vcruntime(tempdir)

        self.assertFalse(os.path.isfile(os.path.join(
            tempdir, os.path.basename(dll))))

    def test_get_vc_env_unicode(self):
        import distutils._msvccompiler as _msvccompiler

        test_var = 'ṰḖṤṪ┅ṼẨṜ'
        test_value = '₃⁴₅'

        # Ensure we don't early exit from _get_vc_env
        old_distutils_use_sdk = os.environ.pop('DISTUTILS_USE_SDK', None)
        os.environ[test_var] = test_value
        try:
            env = _msvccompiler._get_vc_env('x86')
            self.assertIn(test_var.lower(), env)
            self.assertEqual(test_value, env[test_var.lower()])
        finally:
            os.environ.pop(test_var)
            if old_distutils_use_sdk:
                os.environ['DISTUTILS_USE_SDK'] = old_distutils_use_sdk

def test_suite():
    return unittest.makeSuite(msvccompilerTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
