"""Tests for distutils.dist."""

from distutils import sysconfig
import os
import test
import unittest

from test.test_support import TESTFN

class SysconfigTestCase(unittest.TestCase):
    def setUp(self):
        super(SysconfigTestCase, self).setUp()
        self.makefile = None

    def tearDown(self):
        if self.makefile is not None:
            os.unlink(self.makefile)
        super(SysconfigTestCase, self).tearDown()

    def test_get_config_h_filename(self):
        config_h = sysconfig.get_config_h_filename()
        self.assert_(os.path.isfile(config_h), config_h)

    def test_get_python_lib(self):
        lib_dir = sysconfig.get_python_lib()
        # XXX doesn't work on Linux when Python was never installed before
        #self.assert_(os.path.isdir(lib_dir), lib_dir)
        # test for pythonxx.lib?
        self.assertNotEqual(sysconfig.get_python_lib(),
                            sysconfig.get_python_lib(prefix=TESTFN))

    def test_get_python_inc(self):
        # The check for srcdir is copied from Python's setup.py,
        # and is necessary to make this test pass when building
        # Python in a directory other than the source directory.
        (srcdir,) = sysconfig.get_config_vars('srcdir')
        if not srcdir:
            inc_dir = sysconfig.get_python_inc()
        else:
            # This test is not really a proper test: when building
            # Python from source, even in the same directory,
            # we won't be testing the same thing as when running
            # distutils' tests on an installed Python. Nevertheless,
            # let's try to do our best: if we are running Python's
            # unittests from a build directory that is not the source
            # directory, the normal inc_dir will exist, it will just not
            # contain anything of interest.
            inc_dir = sysconfig.get_python_inc()
            self.assert_(os.path.isdir(inc_dir))
            # Now test the source location, to make sure Python.h does
            # exist.
            inc_dir = os.path.join(os.getcwd(), srcdir, 'Include')
            inc_dir = os.path.normpath(inc_dir)
        self.assert_(os.path.isdir(inc_dir), inc_dir)
        python_h = os.path.join(inc_dir, "Python.h")
        self.assert_(os.path.isfile(python_h), python_h)

    def test_get_config_vars(self):
        cvars = sysconfig.get_config_vars()
        self.assert_(isinstance(cvars, dict))
        self.assert_(cvars)

    def test_parse_makefile_literal_dollar(self):
        self.makefile = test.test_support.TESTFN
        fd = open(self.makefile, 'w')
        fd.write(r"CONFIG_ARGS=  '--arg1=optarg1' 'ENV=\$$LIB'" '\n')
        fd.write('VAR=$OTHER\nOTHER=foo')
        fd.close()
        d = sysconfig.parse_makefile(self.makefile)
        self.assertEquals(d, {'CONFIG_ARGS': r"'--arg1=optarg1' 'ENV=\$LIB'",
                              'OTHER': 'foo'})


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(SysconfigTestCase))
    return suite


if __name__ == '__main__':
    test.test_support.run_unittest(test_suite())
