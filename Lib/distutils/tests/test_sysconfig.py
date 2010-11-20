"""Tests for distutils.sysconfig."""
import os
import shutil
import test
import unittest

from distutils import sysconfig
from distutils.ccompiler import get_default_compiler
from distutils.tests import support
from test.support import TESTFN, run_unittest

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
        if os.path.isfile(TESTFN):
            os.remove(TESTFN)
        elif os.path.isdir(TESTFN):
            shutil.rmtree(TESTFN)

    def test_get_config_h_filename(self):
        config_h = sysconfig.get_config_h_filename()
        self.assertTrue(os.path.isfile(config_h), config_h)

    def test_get_python_lib(self):
        lib_dir = sysconfig.get_python_lib()
        # XXX doesn't work on Linux when Python was never installed before
        #self.assertTrue(os.path.isdir(lib_dir), lib_dir)
        # test for pythonxx.lib?
        self.assertNotEqual(sysconfig.get_python_lib(),
                            sysconfig.get_python_lib(prefix=TESTFN))

    def test_get_python_inc(self):
        inc_dir = sysconfig.get_python_inc()
        # This is not much of a test.  We make sure Python.h exists
        # in the directory returned by get_python_inc() but we don't know
        # it is the correct file.
        self.assertTrue(os.path.isdir(inc_dir), inc_dir)
        python_h = os.path.join(inc_dir, "Python.h")
        self.assertTrue(os.path.isfile(python_h), python_h)

    def test_get_config_vars(self):
        cvars = sysconfig.get_config_vars()
        self.assertTrue(isinstance(cvars, dict))
        self.assertTrue(cvars)

    def test_customize_compiler(self):

        # not testing if default compiler is not unix
        if get_default_compiler() != 'unix':
            return

        os.environ['AR'] = 'my_ar'
        os.environ['ARFLAGS'] = '-arflags'

        # make sure AR gets caught
        class compiler:
            compiler_type = 'unix'

            def set_executables(self, **kw):
                self.exes = kw

        comp = compiler()
        sysconfig.customize_compiler(comp)
        self.assertEqual(comp.exes['archiver'], 'my_ar -arflags')

    def test_parse_makefile_base(self):
        self.makefile = TESTFN
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
        self.makefile = TESTFN
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
        self.assertEqual(global_sysconfig.get_config_var('LDSHARED'),sysconfig.get_config_var('LDSHARED'))
        self.assertEqual(global_sysconfig.get_config_var('CC'), sysconfig.get_config_var('CC'))



def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(SysconfigTestCase))
    return suite


if __name__ == '__main__':
    run_unittest(test_suite())
