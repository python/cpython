"""Tests for distutils.sysconfig."""
import os
import test
import unittest
import shutil
import subprocess
import sys
import textwrap

from distutils import sysconfig
from distutils.ccompiler import get_default_compiler
from distutils.tests import support
from test.test_support import TESTFN, swap_item

class SysconfigTestCase(support.EnvironGuard,
                        unittest.TestCase):
    def setUp(self):
        super(SysconfigTestCase, self).setUp()
        self.makefile = None

    def tearDown(self):
        if self.makefile is not None:
            os.unlink(self.makefile)
        self.cleanup_testfn()
        super(SysconfigTestCase, self).tearDown()

    def cleanup_testfn(self):
        path = test.test_support.TESTFN
        if os.path.isfile(path):
            os.remove(path)
        elif os.path.isdir(path):
            shutil.rmtree(path)

    def test_get_python_lib(self):
        lib_dir = sysconfig.get_python_lib()
        # XXX doesn't work on Linux when Python was never installed before
        #self.assertTrue(os.path.isdir(lib_dir), lib_dir)
        # test for pythonxx.lib?
        self.assertNotEqual(sysconfig.get_python_lib(),
                            sysconfig.get_python_lib(prefix=TESTFN))
        _sysconfig = __import__('sysconfig')
        res = sysconfig.get_python_lib(True, True)
        self.assertEqual(_sysconfig.get_path('platstdlib'), res)

    def test_get_python_inc(self):
        inc_dir = sysconfig.get_python_inc()
        # This is not much of a test.  We make sure Python.h exists
        # in the directory returned by get_python_inc() but we don't know
        # it is the correct file.
        self.assertTrue(os.path.isdir(inc_dir), inc_dir)
        python_h = os.path.join(inc_dir, "Python.h")
        self.assertTrue(os.path.isfile(python_h), python_h)

    def customize_compiler(self):
        # make sure AR gets caught
        class compiler:
            compiler_type = 'unix'

            def set_executables(self, **kw):
                self.exes = kw

        sysconfig_vars = {
            'AR': 'sc_ar',
            'CC': 'sc_cc',
            'CXX': 'sc_cxx',
            'ARFLAGS': '--sc-arflags',
            'CFLAGS': '--sc-cflags',
            'CCSHARED': '--sc-ccshared',
            'LDSHARED': 'sc_ldshared',
            'SO': 'sc_shutil_suffix',
        }

        comp = compiler()
        old_vars = dict(sysconfig._config_vars)
        try:
            # On macOS, disable _osx_support.customize_compiler()
            sysconfig._config_vars['CUSTOMIZED_OSX_COMPILER'] = 'True'

            for key, value in sysconfig_vars.items():
                sysconfig._config_vars[key] = value
            sysconfig.customize_compiler(comp)
        finally:
            sysconfig._config_vars.clear()
            sysconfig._config_vars.update(old_vars)

        return comp

    @unittest.skipUnless(get_default_compiler() == 'unix',
                         'not testing if default compiler is not unix')
    def test_customize_compiler(self):
        # Make sure that sysconfig._config_vars is initialized
        sysconfig.get_config_vars()

        os.environ['AR'] = 'env_ar'
        os.environ['CC'] = 'env_cc'
        os.environ['CPP'] = 'env_cpp'
        os.environ['CXX'] = 'env_cxx --env-cxx-flags'
        os.environ['LDSHARED'] = 'env_ldshared'
        os.environ['LDFLAGS'] = '--env-ldflags'
        os.environ['ARFLAGS'] = '--env-arflags'
        os.environ['CFLAGS'] = '--env-cflags'
        os.environ['CPPFLAGS'] = '--env-cppflags'

        comp = self.customize_compiler()
        self.assertEqual(comp.exes['archiver'],
                         'env_ar --env-arflags')
        self.assertEqual(comp.exes['preprocessor'],
                         'env_cpp --env-cppflags')
        self.assertEqual(comp.exes['compiler'],
                         'env_cc --sc-cflags --env-cflags --env-cppflags')
        self.assertEqual(comp.exes['compiler_so'],
                         ('env_cc --sc-cflags '
                          '--env-cflags ''--env-cppflags --sc-ccshared'))
        self.assertEqual(comp.exes['compiler_cxx'],
                         'env_cxx --env-cxx-flags')
        self.assertEqual(comp.exes['linker_exe'],
                         'env_cc')
        self.assertEqual(comp.exes['linker_so'],
                         ('env_ldshared --env-ldflags --env-cflags'
                          ' --env-cppflags'))
        self.assertEqual(comp.shared_lib_extension, 'sc_shutil_suffix')

        del os.environ['AR']
        del os.environ['CC']
        del os.environ['CPP']
        del os.environ['CXX']
        del os.environ['LDSHARED']
        del os.environ['LDFLAGS']
        del os.environ['ARFLAGS']
        del os.environ['CFLAGS']
        del os.environ['CPPFLAGS']

        comp = self.customize_compiler()
        self.assertEqual(comp.exes['archiver'],
                         'sc_ar --sc-arflags')
        self.assertEqual(comp.exes['preprocessor'],
                         'sc_cc -E')
        self.assertEqual(comp.exes['compiler'],
                         'sc_cc --sc-cflags')
        self.assertEqual(comp.exes['compiler_so'],
                         'sc_cc --sc-cflags --sc-ccshared')
        self.assertEqual(comp.exes['compiler_cxx'],
                         'sc_cxx')
        self.assertEqual(comp.exes['linker_exe'],
                         'sc_cc')
        self.assertEqual(comp.exes['linker_so'],
                         'sc_ldshared')
        self.assertEqual(comp.shared_lib_extension, 'sc_shutil_suffix')

    def test_parse_makefile_base(self):
        self.makefile = test.test_support.TESTFN
        fd = open(self.makefile, 'w')
        try:
            fd.write(r"CONFIG_ARGS=  '--arg1=optarg1' 'ENV=LIB'" '\n')
            fd.write('VAR=$OTHER\nOTHER=foo')
        finally:
            fd.close()
        d = sysconfig.parse_makefile(self.makefile)
        self.assertEqual(d, {'CONFIG_ARGS': "'--arg1=optarg1' 'ENV=LIB'",
                             'OTHER': 'foo'})

    def test_parse_makefile_literal_dollar(self):
        self.makefile = test.test_support.TESTFN
        fd = open(self.makefile, 'w')
        try:
            fd.write(r"CONFIG_ARGS=  '--arg1=optarg1' 'ENV=\$$LIB'" '\n')
            fd.write('VAR=$OTHER\nOTHER=foo')
        finally:
            fd.close()
        d = sysconfig.parse_makefile(self.makefile)
        self.assertEqual(d, {'CONFIG_ARGS': r"'--arg1=optarg1' 'ENV=\$LIB'",
                             'OTHER': 'foo'})


    def test_sysconfig_module(self):
        import sysconfig as global_sysconfig
        self.assertEqual(global_sysconfig.get_config_var('CFLAGS'), sysconfig.get_config_var('CFLAGS'))
        self.assertEqual(global_sysconfig.get_config_var('LDFLAGS'), sysconfig.get_config_var('LDFLAGS'))

    @unittest.skipIf(sysconfig.get_config_var('CUSTOMIZED_OSX_COMPILER'),'compiler flags customized')
    def test_sysconfig_compiler_vars(self):
        # On OS X, binary installers support extension module building on
        # various levels of the operating system with differing Xcode
        # configurations.  This requires customization of some of the
        # compiler configuration directives to suit the environment on
        # the installed machine.  Some of these customizations may require
        # running external programs and, so, are deferred until needed by
        # the first extension module build.  With Python 3.3, only
        # the Distutils version of sysconfig is used for extension module
        # builds, which happens earlier in the Distutils tests.  This may
        # cause the following tests to fail since no tests have caused
        # the global version of sysconfig to call the customization yet.
        # The solution for now is to simply skip this test in this case.
        # The longer-term solution is to only have one version of sysconfig.

        import sysconfig as global_sysconfig
        if sysconfig.get_config_var('CUSTOMIZED_OSX_COMPILER'):
            self.skipTest('compiler flags customized')
        self.assertEqual(global_sysconfig.get_config_var('LDSHARED'), sysconfig.get_config_var('LDSHARED'))
        self.assertEqual(global_sysconfig.get_config_var('CC'), sysconfig.get_config_var('CC'))

    def test_customize_compiler_before_get_config_vars(self):
        # Issue #21923: test that a Distribution compiler
        # instance can be called without an explicit call to
        # get_config_vars().
        with open(TESTFN, 'w') as f:
            f.writelines(textwrap.dedent('''\
                from distutils.core import Distribution
                config = Distribution().get_command_obj('config')
                # try_compile may pass or it may fail if no compiler
                # is found but it should not raise an exception.
                rc = config.try_compile('int x;')
                '''))
        p = subprocess.Popen([str(sys.executable), TESTFN],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True)
        outs, errs = p.communicate()
        self.assertEqual(0, p.returncode, "Subprocess failed: " + outs)


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(SysconfigTestCase))
    return suite


if __name__ == '__main__':
    test.test_support.run_unittest(test_suite())
