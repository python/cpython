import sys
import os
import tempfile
import shutil
from StringIO import StringIO

from distutils.core import Extension, Distribution
from distutils.command.build_ext import build_ext
from distutils import sysconfig
from distutils.tests import support
from distutils.extension import Extension
from distutils.errors import UnknownFileError
from distutils.errors import CompileError

import unittest
from test import test_support

# http://bugs.python.org/issue4373
# Don't load the xx module more than once.
ALREADY_TESTED = False

def _get_source_filename():
    srcdir = sysconfig.get_config_var('srcdir')
    return os.path.join(srcdir, 'Modules', 'xxmodule.c')

class BuildExtTestCase(support.TempdirManager,
                       support.LoggingSilencer,
                       unittest.TestCase):
    def setUp(self):
        # Create a simple test environment
        # Note that we're making changes to sys.path
        super(BuildExtTestCase, self).setUp()
        self.tmp_dir = self.mkdtemp()
        self.sys_path = sys.path[:]
        sys.path.append(self.tmp_dir)
        shutil.copy(_get_source_filename(), self.tmp_dir)
        if sys.version > "2.6":
            import site
            self.old_user_base = site.USER_BASE
            site.USER_BASE = self.mkdtemp()
            from distutils.command import build_ext
            build_ext.USER_BASE = site.USER_BASE

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
        if not test_support.verbose:
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
        test_support.unload('xx')
        sys.path = self.sys_path
        if sys.version > "2.6":
            import site
            site.USER_BASE = self.old_user_base
            from distutils.command import build_ext
            build_ext.USER_BASE = self.old_user_base

        super(BuildExtTestCase, self).tearDown()

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

    def test_user_site(self):
        # site.USER_SITE was introduced in 2.6
        if sys.version < '2.6':
            return

        import site
        dist = Distribution({'name': 'xx'})
        cmd = build_ext(dist)

        # making sure the suer option is there
        options = [name for name, short, lable in
                   cmd.user_options]
        self.assert_('user' in options)

        # setting a value
        cmd.user = 1

        # setting user based lib and include
        lib = os.path.join(site.USER_BASE, 'lib')
        incl = os.path.join(site.USER_BASE, 'include')
        os.mkdir(lib)
        os.mkdir(incl)

        # let's run finalize
        cmd.ensure_finalized()

        # see if include_dirs and library_dirs
        # were set
        self.assert_(lib in cmd.library_dirs)
        self.assert_(incl in cmd.include_dirs)

    def test_optional_extension(self):

        # this extension will fail, but let's ignore this failure
        # with the optional argument.
        modules = [Extension('foo', ['xxx'], optional=False)]
        dist = Distribution({'name': 'xx', 'ext_modules': modules})
        cmd = build_ext(dist)
        cmd.ensure_finalized()
        self.assertRaises((UnknownFileError, CompileError),
                          cmd.run)  # should raise an error

        modules = [Extension('foo', ['xxx'], optional=True)]
        dist = Distribution({'name': 'xx', 'ext_modules': modules})
        cmd = build_ext(dist)
        cmd.ensure_finalized()
        cmd.run()  # should pass

def test_suite():
    src = _get_source_filename()
    if not os.path.exists(src):
        if test_support.verbose:
            print ('test_build_ext: Cannot find source code (test'
                   ' must run in python build dir)')
        return unittest.TestSuite()
    else: return unittest.makeSuite(BuildExtTestCase)

if __name__ == '__main__':
    test_support.run_unittest(test_suite())
