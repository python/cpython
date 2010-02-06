"""Tests for distutils.cygwinccompiler."""
import unittest
import sys
import os
import warnings
import sysconfig

from test.test_support import check_warnings, run_unittest
from test.test_support import captured_stdout

from distutils import cygwinccompiler
from distutils.cygwinccompiler import (CygwinCCompiler, check_config_h,
                                       CONFIG_H_OK, CONFIG_H_NOTOK,
                                       CONFIG_H_UNCERTAIN, get_versions,
                                       get_msvcr, RE_VERSION)
from distutils.util import get_compiler_versions
from distutils.tests import support

class CygwinCCompilerTestCase(support.TempdirManager,
                              unittest.TestCase):

    def setUp(self):
        super(CygwinCCompilerTestCase, self).setUp()
        self.version = sys.version
        self.python_h = os.path.join(self.mkdtemp(), 'python.h')
        self.old_get_config_h_filename = sysconfig.get_config_h_filename
        sysconfig.get_config_h_filename = self._get_config_h_filename

    def tearDown(self):
        sys.version = self.version
        sysconfig.get_config_h_filename = self.old_get_config_h_filename
        super(CygwinCCompilerTestCase, self).tearDown()

    def _get_config_h_filename(self):
        return self.python_h

    def test_check_config_h(self):

        # check_config_h looks for "GCC" in sys.version first
        # returns CONFIG_H_OK if found
        sys.version = ('2.6.1 (r261:67515, Dec  6 2008, 16:42:21) \n[GCC '
                       '4.0.1 (Apple Computer, Inc. build 5370)]')

        self.assertEquals(check_config_h()[0], CONFIG_H_OK)

        # then it tries to see if it can find "__GNUC__" in pyconfig.h
        sys.version = 'something without the *CC word'

        # if the file doesn't exist it returns  CONFIG_H_UNCERTAIN
        self.assertEquals(check_config_h()[0], CONFIG_H_UNCERTAIN)

        # if it exists but does not contain __GNUC__, it returns CONFIG_H_NOTOK
        self.write_file(self.python_h, 'xxx')
        self.assertEquals(check_config_h()[0], CONFIG_H_NOTOK)

        # and CONFIG_H_OK if __GNUC__ is found
        self.write_file(self.python_h, 'xxx __GNUC__ xxx')
        self.assertEquals(check_config_h()[0], CONFIG_H_OK)

    def test_get_msvcr(self):

        # none
        sys.version  = ('2.6.1 (r261:67515, Dec  6 2008, 16:42:21) '
                        '\n[GCC 4.0.1 (Apple Computer, Inc. build 5370)]')
        self.assertEquals(get_msvcr(), None)

        # MSVC 7.0
        sys.version = ('2.5.1 (r251:54863, Apr 18 2007, 08:51:08) '
                       '[MSC v.1300 32 bits (Intel)]')
        self.assertEquals(get_msvcr(), ['msvcr70'])

        # MSVC 7.1
        sys.version = ('2.5.1 (r251:54863, Apr 18 2007, 08:51:08) '
                       '[MSC v.1310 32 bits (Intel)]')
        self.assertEquals(get_msvcr(), ['msvcr71'])

        # VS2005 / MSVC 8.0
        sys.version = ('2.5.1 (r251:54863, Apr 18 2007, 08:51:08) '
                       '[MSC v.1400 32 bits (Intel)]')
        self.assertEquals(get_msvcr(), ['msvcr80'])

        # VS2008 / MSVC 9.0
        sys.version = ('2.5.1 (r251:54863, Apr 18 2007, 08:51:08) '
                       '[MSC v.1500 32 bits (Intel)]')
        self.assertEquals(get_msvcr(), ['msvcr90'])

        # unknown
        sys.version = ('2.5.1 (r251:54863, Apr 18 2007, 08:51:08) '
                       '[MSC v.1999 32 bits (Intel)]')
        self.assertRaises(ValueError, get_msvcr)


    def test_get_version_deprecated(self):
        with check_warnings() as w:
            warnings.simplefilter("always")
            # make sure get_compiler_versions and get_versions
            # returns the same thing
            self.assertEquals(get_compiler_versions(), get_versions())
            # make sure using get_version() generated a warning
            self.assertEquals(len(w.warnings), 1)
            # make sure any usage of RE_VERSION will also
            # generate a warning, but till works
            version = RE_VERSION.search('1.2').group(1)
            self.assertEquals(version, '1.2')
            self.assertEquals(len(w.warnings), 2)

def test_suite():
    return unittest.makeSuite(CygwinCCompilerTestCase)

if __name__ == '__main__':
    run_unittest(test_suite())
