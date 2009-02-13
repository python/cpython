import sys
import os
import tempfile
import shutil
from io import StringIO

from distutils.core import Extension, Distribution
from distutils.command.build_ext import build_ext
from distutils import sysconfig

import unittest
from test import support

# http://bugs.python.org/issue4373
# Don't load the xx module more than once.
ALREADY_TESTED = False

def _get_source_filename():
    srcdir = sysconfig.get_config_var('srcdir')
    return os.path.join(srcdir, 'Modules', 'xxmodule.c')

class BuildExtTestCase(unittest.TestCase):
    def setUp(self):
        # Create a simple test environment
        # Note that we're making changes to sys.path
        self.tmp_dir = tempfile.mkdtemp(prefix="pythontest_")
        self.sys_path = sys.path[:]
        sys.path.append(self.tmp_dir)
        shutil.copy(_get_source_filename(), self.tmp_dir)

    def test_build_ext(self):
        global ALREADY_TESTED
        xx_c = os.path.join(self.tmp_dir, 'xxmodule.c')
        xx_ext = Extension('xx', [xx_c])
        dist = Distribution({'name': 'xx', 'ext_modules': [xx_ext]})
        dist.package_dir = self.tmp_dir
        cmd = build_ext(dist)
        if os.name == "nt":
            # On Windows, we must build a debug version iff running
            # a debug build of Python
            cmd.debug = sys.executable.endswith("_d.exe")
        cmd.build_lib = self.tmp_dir
        cmd.build_temp = self.tmp_dir

        old_stdout = sys.stdout
        if not support.verbose:
            # silence compiler output
            sys.stdout = StringIO()
        try:
            cmd.ensure_finalized()
            cmd.run()
        finally:
            sys.stdout = old_stdout

        if ALREADY_TESTED:
            return
        else:
            ALREADY_TESTED = True

        import xx

        for attr in ('error', 'foo', 'new', 'roj'):
            self.assert_(hasattr(xx, attr))

        self.assertEquals(xx.foo(2, 5), 7)
        self.assertEquals(xx.foo(13,15), 28)
        self.assertEquals(xx.new().demo(), None)
        doc = 'This is a template module just for instruction.'
        self.assertEquals(xx.__doc__, doc)
        self.assert_(isinstance(xx.Null(), xx.Null))
        self.assert_(isinstance(xx.Str(), xx.Str))

    def tearDown(self):
        # Get everything back to normal
        support.unload('xx')
        sys.path = self.sys_path
        # XXX on Windows the test leaves a directory with xx module in TEMP
        shutil.rmtree(self.tmp_dir, os.name == 'nt' or sys.platform == 'cygwin')

    def test_solaris_enable_shared(self):
        dist = Distribution({'name': 'xx'})
        cmd = build_ext(dist)
        old = sys.platform

        sys.platform = 'sunos' # fooling finalize_options
        from distutils.sysconfig import  _config_vars
        old_var = _config_vars.get('Py_ENABLE_SHARED')
        _config_vars['Py_ENABLE_SHARED'] = 1
        try:
            cmd.ensure_finalized()
        finally:
            sys.platform = old
            if old_var is None:
                del _config_vars['Py_ENABLE_SHARED']
            else:
                _config_vars['Py_ENABLE_SHARED'] = old_var

        # make sur we get some lobrary dirs under solaris
        self.assert_(len(cmd.library_dirs) > 0)

def test_suite():
    src = _get_source_filename()
    if not os.path.exists(src):
        if support.verbose:
            print('test_build_ext: Cannot find source code (test'
                  ' must run in python build dir)')
        return unittest.TestSuite()
    else: return unittest.makeSuite(BuildExtTestCase)

if __name__ == '__main__':
    support.run_unittest(test_suite())
