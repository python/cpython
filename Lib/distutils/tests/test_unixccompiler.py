"""Tests for distutils.unixccompiler."""
import sys
import unittest
from test.support import run_unittest

from distutils import sysconfig
from distutils.unixccompiler import UnixCCompiler

class UnixCCompilerTestCase(unittest.TestCase):

    def setUp(self):
        self._backup_platform = sys.platform
        self._backup_get_config_var = sysconfig.get_config_var
        class CompilerWrapper(UnixCCompiler):
            def rpath_foo(self):
                return self.runtime_library_dir_option('/foo')
        self.cc = CompilerWrapper()

    def tearDown(self):
        sys.platform = self._backup_platform
        sysconfig.get_config_var = self._backup_get_config_var

    def test_runtime_libdir_option(self):

        # not tested under windows
        if sys.platform == 'win32':
            return

        # Issue#5900
        #
        # Ensure RUNPATH is added to extension modules with RPATH if
        # GNU ld is used

        # darwin
        sys.platform = 'darwin'
        self.assertEqual(self.cc.rpath_foo(), '-L/foo')

        # hp-ux
        sys.platform = 'hp-ux'
        old_gcv = sysconfig.get_config_var
        def gcv(v):
            return 'xxx'
        sysconfig.get_config_var = gcv
        self.assertEqual(self.cc.rpath_foo(), ['+s', '-L/foo'])

        def gcv(v):
            return 'gcc'
        sysconfig.get_config_var = gcv
        self.assertEqual(self.cc.rpath_foo(), ['-Wl,+s', '-L/foo'])

        def gcv(v):
            return 'g++'
        sysconfig.get_config_var = gcv
        self.assertEqual(self.cc.rpath_foo(), ['-Wl,+s', '-L/foo'])

        sysconfig.get_config_var = old_gcv

        # irix646
        sys.platform = 'irix646'
        self.assertEqual(self.cc.rpath_foo(), ['-rpath', '/foo'])

        # osf1V5
        sys.platform = 'osf1V5'
        self.assertEqual(self.cc.rpath_foo(), ['-rpath', '/foo'])

        # GCC GNULD
        sys.platform = 'bar'
        def gcv(v):
            if v == 'CC':
                return 'gcc'
            elif v == 'GNULD':
                return 'yes'
        sysconfig.get_config_var = gcv
        self.assertEqual(self.cc.rpath_foo(), '-Wl,--enable-new-dtags,-R/foo')

        # GCC non-GNULD
        sys.platform = 'bar'
        def gcv(v):
            if v == 'CC':
                return 'gcc'
            elif v == 'GNULD':
                return 'no'
        sysconfig.get_config_var = gcv
        self.assertEqual(self.cc.rpath_foo(), '-Wl,-R/foo')

        # GCC GNULD with fully qualified configuration prefix
        # see #7617
        sys.platform = 'bar'
        def gcv(v):
            if v == 'CC':
                return 'x86_64-pc-linux-gnu-gcc-4.4.2'
            elif v == 'GNULD':
                return 'yes'
        sysconfig.get_config_var = gcv
        self.assertEqual(self.cc.rpath_foo(), '-Wl,--enable-new-dtags,-R/foo')


        # non-GCC GNULD
        sys.platform = 'bar'
        def gcv(v):
            if v == 'CC':
                return 'cc'
            elif v == 'GNULD':
                return 'yes'
        sysconfig.get_config_var = gcv
        self.assertEqual(self.cc.rpath_foo(), '-R/foo')

        # non-GCC non-GNULD
        sys.platform = 'bar'
        def gcv(v):
            if v == 'CC':
                return 'cc'
            elif v == 'GNULD':
                return 'no'
        sysconfig.get_config_var = gcv
        self.assertEqual(self.cc.rpath_foo(), '-R/foo')

def test_suite():
    return unittest.makeSuite(UnixCCompilerTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
