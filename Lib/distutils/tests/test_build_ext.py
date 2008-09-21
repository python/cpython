import sys
import os
import tempfile
import shutil
from StringIO import StringIO

from distutils.core import Extension, Distribution
from distutils.command.build_ext import build_ext
from distutils import sysconfig

import unittest
from test import test_support

class BuildExtTestCase(unittest.TestCase):
    def setUp(self):
        # Create a simple test environment
        # Note that we're making changes to sys.path
        self.tmp_dir = tempfile.mkdtemp(prefix="pythontest_")
        self.sys_path = sys.path[:]
        sys.path.append(self.tmp_dir)

        xx_c = os.path.join(sysconfig.project_base, 'Modules', 'xxmodule.c')
        shutil.copy(xx_c, self.tmp_dir)

    def test_build_ext(self):
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
        if not test_support.verbose:
            # silence compiler output
            sys.stdout = StringIO()
        try:
            cmd.ensure_finalized()
            cmd.run()
        finally:
            sys.stdout = old_stdout

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
        test_support.unload('xx')
        sys.path = self.sys_path
        # XXX on Windows the test leaves a directory with xx module in TEMP
        shutil.rmtree(self.tmp_dir, os.name == 'nt' or sys.platform == 'cygwin')

def test_suite():
    if not sysconfig.python_build:
        if test_support.verbose:
            print 'test_build_ext: The test must be run in a python build dir'
        return unittest.TestSuite()
    else: return unittest.makeSuite(BuildExtTestCase)

if __name__ == '__main__':
    test_support.run_unittest(test_suite())
