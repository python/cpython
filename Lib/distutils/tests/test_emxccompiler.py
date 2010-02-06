"""Tests for distutils.emxccompiler."""
import unittest
import sys
import os
import warnings

from test.test_support import check_warnings, run_unittest
from test.test_support import captured_stdout

from distutils.emxccompiler import get_versions
from distutils.util import get_compiler_versions
from distutils.tests import support

class EmxCCompilerTestCase(support.TempdirManager,
                           unittest.TestCase):

    def test_get_version_deprecated(self):
        with check_warnings() as w:
            warnings.simplefilter("always")
            # make sure get_compiler_versions and get_versions
            # returns the same gcc
            gcc, ld, dllwrap = get_compiler_versions()
            emx_gcc, emx_ld = get_versions()
            self.assertEquals(gcc, emx_gcc)

            # make sure using get_version() generated a warning
            self.assertEquals(len(w.warnings), 1)

def test_suite():
    return unittest.makeSuite(EmxCCompilerTestCase)

if __name__ == '__main__':
    run_unittest(test_suite())
