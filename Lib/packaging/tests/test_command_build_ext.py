import os
import sys
import site
import sysconfig
import textwrap
from packaging.dist import Distribution
from packaging.errors import (UnknownFileError, CompileError,
                              PackagingPlatformError)
from packaging.command.build_ext import build_ext
from packaging.compiler.extension import Extension

from test.script_helper import assert_python_ok
from packaging.tests import support, unittest


class BuildExtTestCase(support.TempdirManager,
                       support.LoggingCatcher,
                       unittest.TestCase):
    def setUp(self):
        super(BuildExtTestCase, self).setUp()
        self.tmp_dir = self.mkdtemp()
        self.old_user_base = site.USER_BASE
        site.USER_BASE = self.mkdtemp()

    def tearDown(self):
        site.USER_BASE = self.old_user_base
        super(BuildExtTestCase, self).tearDown()

    def test_build_ext(self):
        support.copy_xxmodule_c(self.tmp_dir)
        xx_c = os.path.join(self.tmp_dir, 'xxmodule.c')
        xx_ext = Extension('xx', [xx_c])
        dist = Distribution({'name': 'xx', 'ext_modules': [xx_ext]})
        dist.package_dir = self.tmp_dir
        cmd = build_ext(dist)
        support.fixup_build_ext(cmd)
        cmd.build_lib = self.tmp_dir
        cmd.build_temp = self.tmp_dir
        cmd.ensure_finalized()
        cmd.run()

        code = textwrap.dedent("""\
            import sys
            sys.path.insert(0, %r)

            import xx

            for attr in ('error', 'foo', 'new', 'roj'):
                assert hasattr(xx, attr)

            assert xx.foo(2, 5) == 7
            assert xx.foo(13, 15) == 28
            assert xx.new().demo() is None
            doc = 'This is a template module just for instruction.'
            assert xx.__doc__ == doc
            assert isinstance(xx.Null(), xx.Null)
            assert isinstance(xx.Str(), xx.Str)
            """)
        code = code % self.tmp_dir
        assert_python_ok('-c', code)

    def test_solaris_enable_shared(self):
        dist = Distribution({'name': 'xx'})
        cmd = build_ext(dist)
        old = sys.platform

        sys.platform = 'sunos'  # fooling finalize_options

        old_var = sysconfig.get_config_var('Py_ENABLE_SHARED')
        sysconfig._CONFIG_VARS['Py_ENABLE_SHARED'] = 1
        try:
            cmd.ensure_finalized()
        finally:
            sys.platform = old
            if old_var is None:
                del sysconfig._CONFIG_VARS['Py_ENABLE_SHARED']
            else:
                sysconfig._CONFIG_VARS['Py_ENABLE_SHARED'] = old_var

        # make sure we get some library dirs under solaris
        self.assertGreater(len(cmd.library_dirs), 0)

    def test_user_site(self):
        dist = Distribution({'name': 'xx'})
        cmd = build_ext(dist)

        # making sure the user option is there
        options = [name for name, short, label in
                   cmd.user_options]
        self.assertIn('user', options)

        # setting a value
        cmd.user = True

        # setting user based lib and include
        lib = os.path.join(site.USER_BASE, 'lib')
        incl = os.path.join(site.USER_BASE, 'include')
        os.mkdir(lib)
        os.mkdir(incl)

        # let's run finalize
        cmd.ensure_finalized()

        # see if include_dirs and library_dirs
        # were set
        self.assertIn(lib, cmd.library_dirs)
        self.assertIn(lib, cmd.rpath)
        self.assertIn(incl, cmd.include_dirs)

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

    def test_finalize_options(self):
        # Make sure Python's include directories (for Python.h, pyconfig.h,
        # etc.) are in the include search path.
        modules = [Extension('foo', ['xxx'], optional=False)]
        dist = Distribution({'name': 'xx', 'ext_modules': modules})
        cmd = build_ext(dist)
        cmd.finalize_options()

        py_include = sysconfig.get_path('include')
        self.assertIn(py_include, cmd.include_dirs)

        plat_py_include = sysconfig.get_path('platinclude')
        self.assertIn(plat_py_include, cmd.include_dirs)

        # make sure cmd.libraries is turned into a list
        # if it's a string
        cmd = build_ext(dist)
        cmd.libraries = 'my_lib'
        cmd.finalize_options()
        self.assertEqual(cmd.libraries, ['my_lib'])

        # make sure cmd.library_dirs is turned into a list
        # if it's a string
        cmd = build_ext(dist)
        cmd.library_dirs = 'my_lib_dir'
        cmd.finalize_options()
        self.assertIn('my_lib_dir', cmd.library_dirs)

        # make sure rpath is turned into a list
        # if it's a list of os.pathsep's paths
        cmd = build_ext(dist)
        cmd.rpath = os.pathsep.join(['one', 'two'])
        cmd.finalize_options()
        self.assertEqual(cmd.rpath, ['one', 'two'])

        # XXX more tests to perform for win32

        # make sure define is turned into 2-tuples
        # strings if they are ','-separated strings
        cmd = build_ext(dist)
        cmd.define = 'one,two'
        cmd.finalize_options()
        self.assertEqual(cmd.define, [('one', '1'), ('two', '1')])

        # make sure undef is turned into a list of
        # strings if they are ','-separated strings
        cmd = build_ext(dist)
        cmd.undef = 'one,two'
        cmd.finalize_options()
        self.assertEqual(cmd.undef, ['one', 'two'])

        # make sure swig_opts is turned into a list
        cmd = build_ext(dist)
        cmd.swig_opts = None
        cmd.finalize_options()
        self.assertEqual(cmd.swig_opts, [])

        cmd = build_ext(dist)
        cmd.swig_opts = '1 2'
        cmd.finalize_options()
        self.assertEqual(cmd.swig_opts, ['1', '2'])

    def test_get_source_files(self):
        modules = [Extension('foo', ['xxx'], optional=False)]
        dist = Distribution({'name': 'xx', 'ext_modules': modules})
        cmd = build_ext(dist)
        cmd.ensure_finalized()
        self.assertEqual(cmd.get_source_files(), ['xxx'])

    def test_compiler_option(self):
        # cmd.compiler is an option and
        # should not be overriden by a compiler instance
        # when the command is run
        dist = Distribution()
        cmd = build_ext(dist)
        cmd.compiler = 'unix'
        cmd.ensure_finalized()
        cmd.run()
        self.assertEqual(cmd.compiler, 'unix')

    def test_get_outputs(self):
        tmp_dir = self.mkdtemp()
        c_file = os.path.join(tmp_dir, 'foo.c')
        self.write_file(c_file, 'void PyInit_foo(void) {}\n')
        ext = Extension('foo', [c_file], optional=False)
        dist = Distribution({'name': 'xx',
                             'ext_modules': [ext]})
        cmd = build_ext(dist)
        support.fixup_build_ext(cmd)
        cmd.ensure_finalized()
        self.assertEqual(len(cmd.get_outputs()), 1)

        cmd.build_lib = os.path.join(self.tmp_dir, 'build')
        cmd.build_temp = os.path.join(self.tmp_dir, 'tempt')

        # issue #5977 : distutils build_ext.get_outputs
        # returns wrong result with --inplace
        other_tmp_dir = os.path.realpath(self.mkdtemp())
        old_wd = os.getcwd()
        os.chdir(other_tmp_dir)
        try:
            cmd.inplace = True
            cmd.run()
            so_file = cmd.get_outputs()[0]
        finally:
            os.chdir(old_wd)
        self.assertTrue(os.path.exists(so_file))
        so_ext = sysconfig.get_config_var('SO')
        self.assertTrue(so_file.endswith(so_ext))
        so_dir = os.path.dirname(so_file)
        self.assertEqual(so_dir, other_tmp_dir)

        cmd.inplace = False
        cmd.run()
        so_file = cmd.get_outputs()[0]
        self.assertTrue(os.path.exists(so_file))
        self.assertTrue(so_file.endswith(so_ext))
        so_dir = os.path.dirname(so_file)
        self.assertEqual(so_dir, cmd.build_lib)

        # inplace = False, cmd.package = 'bar'
        build_py = cmd.get_finalized_command('build_py')
        build_py.package_dir = 'bar'
        path = cmd.get_ext_fullpath('foo')
        # checking that the last directory is the build_dir
        path = os.path.split(path)[0]
        self.assertEqual(path, cmd.build_lib)

        # inplace = True, cmd.package = 'bar'
        cmd.inplace = True
        other_tmp_dir = os.path.realpath(self.mkdtemp())
        old_wd = os.getcwd()
        os.chdir(other_tmp_dir)
        try:
            path = cmd.get_ext_fullpath('foo')
        finally:
            os.chdir(old_wd)
        # checking that the last directory is bar
        path = os.path.split(path)[0]
        lastdir = os.path.split(path)[-1]
        self.assertEqual(lastdir, 'bar')

    def test_ext_fullpath(self):
        ext = sysconfig.get_config_vars()['SO']
        # building lxml.etree inplace
        #etree_c = os.path.join(self.tmp_dir, 'lxml.etree.c')
        #etree_ext = Extension('lxml.etree', [etree_c])
        #dist = Distribution({'name': 'lxml', 'ext_modules': [etree_ext]})
        dist = Distribution()
        cmd = build_ext(dist)
        cmd.inplace = True
        cmd.distribution.package_dir = 'src'
        cmd.distribution.packages = ['lxml', 'lxml.html']
        curdir = os.getcwd()
        wanted = os.path.join(curdir, 'src', 'lxml', 'etree' + ext)
        path = cmd.get_ext_fullpath('lxml.etree')
        self.assertEqual(wanted, path)

        # building lxml.etree not inplace
        cmd.inplace = False
        cmd.build_lib = os.path.join(curdir, 'tmpdir')
        wanted = os.path.join(curdir, 'tmpdir', 'lxml', 'etree' + ext)
        path = cmd.get_ext_fullpath('lxml.etree')
        self.assertEqual(wanted, path)

        # building twisted.runner.portmap not inplace
        build_py = cmd.get_finalized_command('build_py')
        build_py.package_dir = None
        cmd.distribution.packages = ['twisted', 'twisted.runner.portmap']
        path = cmd.get_ext_fullpath('twisted.runner.portmap')
        wanted = os.path.join(curdir, 'tmpdir', 'twisted', 'runner',
                              'portmap' + ext)
        self.assertEqual(wanted, path)

        # building twisted.runner.portmap inplace
        cmd.inplace = True
        path = cmd.get_ext_fullpath('twisted.runner.portmap')
        wanted = os.path.join(curdir, 'twisted', 'runner', 'portmap' + ext)
        self.assertEqual(wanted, path)

    @unittest.skipUnless(sys.platform == 'darwin',
                         'test only relevant for Mac OS X')
    def test_deployment_target_default(self):
        # Issue 9516: Test that, in the absence of the environment variable,
        # an extension module is compiled with the same deployment target as
        #  the interpreter.
        self._try_compile_deployment_target('==', None)

    @unittest.skipUnless(sys.platform == 'darwin',
                         'test only relevant for Mac OS X')
    def test_deployment_target_too_low(self):
        # Issue 9516: Test that an extension module is not allowed to be
        # compiled with a deployment target less than that of the interpreter.
        self.assertRaises(PackagingPlatformError,
            self._try_compile_deployment_target, '>', '10.1')

    @unittest.skipUnless(sys.platform == 'darwin',
                         'test only relevant for Mac OS X')
    def test_deployment_target_higher_ok(self):
        # Issue 9516: Test that an extension module can be compiled with a
        # deployment target higher than that of the interpreter: the ext
        # module may depend on some newer OS feature.
        deptarget = sysconfig.get_config_var('MACOSX_DEPLOYMENT_TARGET')
        if deptarget:
            # increment the minor version number (i.e. 10.6 -> 10.7)
            deptarget = [int(x) for x in deptarget.split('.')]
            deptarget[-1] += 1
            deptarget = '.'.join(str(i) for i in deptarget)
            self._try_compile_deployment_target('<', deptarget)

    def _try_compile_deployment_target(self, operator, target):
        orig_environ = os.environ
        os.environ = orig_environ.copy()
        self.addCleanup(setattr, os, 'environ', orig_environ)

        if target is None:
            if os.environ.get('MACOSX_DEPLOYMENT_TARGET'):
                del os.environ['MACOSX_DEPLOYMENT_TARGET']
        else:
            os.environ['MACOSX_DEPLOYMENT_TARGET'] = target

        deptarget_c = os.path.join(self.tmp_dir, 'deptargetmodule.c')

        with open(deptarget_c, 'w') as fp:
            fp.write(textwrap.dedent('''\
                #include <AvailabilityMacros.h>

                int dummy;

                #if TARGET %s MAC_OS_X_VERSION_MIN_REQUIRED
                #else
                #error "Unexpected target"
                #endif

            ''' % operator))

        # get the deployment target that the interpreter was built with
        target = sysconfig.get_config_var('MACOSX_DEPLOYMENT_TARGET')
        target = tuple(map(int, target.split('.')))
        target = '%02d%01d0' % target

        deptarget_ext = Extension(
            'deptarget',
            [deptarget_c],
            extra_compile_args=['-DTARGET=%s' % (target,)],
        )
        dist = Distribution({
            'name': 'deptarget',
            'ext_modules': [deptarget_ext],
        })
        dist.package_dir = self.tmp_dir
        cmd = build_ext(dist)
        cmd.build_lib = self.tmp_dir
        cmd.build_temp = self.tmp_dir

        try:
            cmd.ensure_finalized()
            cmd.run()
        except CompileError:
            self.fail("Wrong deployment target during compilation")


def test_suite():
    return unittest.makeSuite(BuildExtTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
