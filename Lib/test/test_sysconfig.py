import unittest
import sys
import os
import subprocess
import shutil
from copy import copy

from test.support import (run_unittest, TESTFN, unlink,
                          captured_stdout, skip_unless_symlink)

import sysconfig
from sysconfig import (get_paths, get_platform, get_config_vars,
                       get_path, get_path_names, _INSTALL_SCHEMES,
                       _get_default_scheme, _expand_vars,
                       get_scheme_names, get_config_var, _main)
import _osx_support

class TestSysConfig(unittest.TestCase):

    def setUp(self):
        super(TestSysConfig, self).setUp()
        self.sys_path = sys.path[:]
        # patching os.uname
        if hasattr(os, 'uname'):
            self.uname = os.uname
            self._uname = os.uname()
        else:
            self.uname = None
            self._set_uname(('',)*5)
        os.uname = self._get_uname
        # saving the environment
        self.name = os.name
        self.platform = sys.platform
        self.version = sys.version
        self.sep = os.sep
        self.join = os.path.join
        self.isabs = os.path.isabs
        self.splitdrive = os.path.splitdrive
        self._config_vars = sysconfig._CONFIG_VARS, copy(sysconfig._CONFIG_VARS)
        self._added_envvars = []
        self._changed_envvars = []
        for var in ('MACOSX_DEPLOYMENT_TARGET', 'PATH'):
            if var in os.environ:
                self._changed_envvars.append((var, os.environ[var]))
            else:
                self._added_envvars.append(var)

    def tearDown(self):
        sys.path[:] = self.sys_path
        self._cleanup_testfn()
        if self.uname is not None:
            os.uname = self.uname
        else:
            del os.uname
        os.name = self.name
        sys.platform = self.platform
        sys.version = self.version
        os.sep = self.sep
        os.path.join = self.join
        os.path.isabs = self.isabs
        os.path.splitdrive = self.splitdrive
        sysconfig._CONFIG_VARS = self._config_vars[0]
        sysconfig._CONFIG_VARS.clear()
        sysconfig._CONFIG_VARS.update(self._config_vars[1])
        for var, value in self._changed_envvars:
            os.environ[var] = value
        for var in self._added_envvars:
            os.environ.pop(var, None)

        super(TestSysConfig, self).tearDown()

    def _set_uname(self, uname):
        self._uname = os.uname_result(uname)

    def _get_uname(self):
        return self._uname

    def _cleanup_testfn(self):
        path = TESTFN
        if os.path.isfile(path):
            os.remove(path)
        elif os.path.isdir(path):
            shutil.rmtree(path)

    def test_get_path_names(self):
        self.assertEqual(get_path_names(), sysconfig._SCHEME_KEYS)

    def test_get_paths(self):
        scheme = get_paths()
        default_scheme = _get_default_scheme()
        wanted = _expand_vars(default_scheme, None)
        wanted = sorted(wanted.items())
        scheme = sorted(scheme.items())
        self.assertEqual(scheme, wanted)

    def test_get_path(self):
        # XXX make real tests here
        for scheme in _INSTALL_SCHEMES:
            for name in _INSTALL_SCHEMES[scheme]:
                res = get_path(name, scheme)

    def test_get_config_vars(self):
        cvars = get_config_vars()
        self.assertIsInstance(cvars, dict)
        self.assertTrue(cvars)

    def test_get_platform(self):
        # windows XP, 32bits
        os.name = 'nt'
        sys.version = ('2.4.4 (#71, Oct 18 2006, 08:34:43) '
                       '[MSC v.1310 32 bit (Intel)]')
        sys.platform = 'win32'
        self.assertEqual(get_platform(), 'win32')

        # windows XP, amd64
        os.name = 'nt'
        sys.version = ('2.4.4 (#71, Oct 18 2006, 08:34:43) '
                       '[MSC v.1310 32 bit (Amd64)]')
        sys.platform = 'win32'
        self.assertEqual(get_platform(), 'win-amd64')

        # windows XP, itanium
        os.name = 'nt'
        sys.version = ('2.4.4 (#71, Oct 18 2006, 08:34:43) '
                       '[MSC v.1310 32 bit (Itanium)]')
        sys.platform = 'win32'
        self.assertEqual(get_platform(), 'win-ia64')

        # macbook
        os.name = 'posix'
        sys.version = ('2.5 (r25:51918, Sep 19 2006, 08:49:13) '
                       '\n[GCC 4.0.1 (Apple Computer, Inc. build 5341)]')
        sys.platform = 'darwin'
        self._set_uname(('Darwin', 'macziade', '8.11.1',
                   ('Darwin Kernel Version 8.11.1: '
                    'Wed Oct 10 18:23:28 PDT 2007; '
                    'root:xnu-792.25.20~1/RELEASE_I386'), 'PowerPC'))
        _osx_support._remove_original_values(get_config_vars())
        get_config_vars()['MACOSX_DEPLOYMENT_TARGET'] = '10.3'

        get_config_vars()['CFLAGS'] = ('-fno-strict-aliasing -DNDEBUG -g '
                                       '-fwrapv -O3 -Wall -Wstrict-prototypes')

        maxint = sys.maxsize
        try:
            sys.maxsize = 2147483647
            self.assertEqual(get_platform(), 'macosx-10.3-ppc')
            sys.maxsize = 9223372036854775807
            self.assertEqual(get_platform(), 'macosx-10.3-ppc64')
        finally:
            sys.maxsize = maxint

        self._set_uname(('Darwin', 'macziade', '8.11.1',
                   ('Darwin Kernel Version 8.11.1: '
                    'Wed Oct 10 18:23:28 PDT 2007; '
                    'root:xnu-792.25.20~1/RELEASE_I386'), 'i386'))
        _osx_support._remove_original_values(get_config_vars())
        get_config_vars()['MACOSX_DEPLOYMENT_TARGET'] = '10.3'

        get_config_vars()['CFLAGS'] = ('-fno-strict-aliasing -DNDEBUG -g '
                                       '-fwrapv -O3 -Wall -Wstrict-prototypes')
        maxint = sys.maxsize
        try:
            sys.maxsize = 2147483647
            self.assertEqual(get_platform(), 'macosx-10.3-i386')
            sys.maxsize = 9223372036854775807
            self.assertEqual(get_platform(), 'macosx-10.3-x86_64')
        finally:
            sys.maxsize = maxint

        # macbook with fat binaries (fat, universal or fat64)
        _osx_support._remove_original_values(get_config_vars())
        get_config_vars()['MACOSX_DEPLOYMENT_TARGET'] = '10.4'
        get_config_vars()['CFLAGS'] = ('-arch ppc -arch i386 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')

        self.assertEqual(get_platform(), 'macosx-10.4-fat')

        _osx_support._remove_original_values(get_config_vars())
        get_config_vars()['CFLAGS'] = ('-arch x86_64 -arch i386 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')

        self.assertEqual(get_platform(), 'macosx-10.4-intel')

        _osx_support._remove_original_values(get_config_vars())
        get_config_vars()['CFLAGS'] = ('-arch x86_64 -arch ppc -arch i386 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')
        self.assertEqual(get_platform(), 'macosx-10.4-fat3')

        _osx_support._remove_original_values(get_config_vars())
        get_config_vars()['CFLAGS'] = ('-arch ppc64 -arch x86_64 -arch ppc -arch i386 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')
        self.assertEqual(get_platform(), 'macosx-10.4-universal')

        _osx_support._remove_original_values(get_config_vars())
        get_config_vars()['CFLAGS'] = ('-arch x86_64 -arch ppc64 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')

        self.assertEqual(get_platform(), 'macosx-10.4-fat64')

        for arch in ('ppc', 'i386', 'x86_64', 'ppc64'):
            _osx_support._remove_original_values(get_config_vars())
            get_config_vars()['CFLAGS'] = ('-arch %s -isysroot '
                                           '/Developer/SDKs/MacOSX10.4u.sdk  '
                                           '-fno-strict-aliasing -fno-common '
                                           '-dynamic -DNDEBUG -g -O3' % arch)

            self.assertEqual(get_platform(), 'macosx-10.4-%s' % arch)

        # linux debian sarge
        os.name = 'posix'
        sys.version = ('2.3.5 (#1, Jul  4 2007, 17:28:59) '
                       '\n[GCC 4.1.2 20061115 (prerelease) (Debian 4.1.1-21)]')
        sys.platform = 'linux2'
        self._set_uname(('Linux', 'aglae', '2.6.21.1dedibox-r7',
                    '#1 Mon Apr 30 17:25:38 CEST 2007', 'i686'))

        self.assertEqual(get_platform(), 'linux-i686')

        # XXX more platforms to tests here

    def test_get_config_h_filename(self):
        config_h = sysconfig.get_config_h_filename()
        self.assertTrue(os.path.isfile(config_h), config_h)

    def test_get_scheme_names(self):
        wanted = ('nt', 'nt_user', 'os2', 'os2_home', 'osx_framework_user',
                  'posix_home', 'posix_prefix', 'posix_user')
        self.assertEqual(get_scheme_names(), wanted)

    @skip_unless_symlink
    def test_symlink(self):
        # On Windows, the EXE needs to know where pythonXY.dll is at so we have
        # to add the directory to the path.
        if sys.platform == "win32":
            os.environ["PATH"] = "{};{}".format(
                os.path.dirname(sys.executable), os.environ["PATH"])

        # Issue 7880
        def get(python):
            cmd = [python, '-c',
                   'import sysconfig; print(sysconfig.get_platform())']
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, env=os.environ)
            return p.communicate()
        real = os.path.realpath(sys.executable)
        link = os.path.abspath(TESTFN)
        os.symlink(real, link)
        try:
            self.assertEqual(get(real), get(link))
        finally:
            unlink(link)

    def test_user_similar(self):
        # Issue #8759: make sure the posix scheme for the users
        # is similar to the global posix_prefix one
        base = get_config_var('base')
        user = get_config_var('userbase')
        # the global scheme mirrors the distinction between prefix and
        # exec-prefix but not the user scheme, so we have to adapt the paths
        # before comparing (issue #9100)
        adapt = sys.base_prefix != sys.base_exec_prefix
        for name in ('stdlib', 'platstdlib', 'purelib', 'platlib'):
            global_path = get_path(name, 'posix_prefix')
            if adapt:
                global_path = global_path.replace(sys.exec_prefix, sys.base_prefix)
                base = base.replace(sys.exec_prefix, sys.base_prefix)
            elif sys.base_prefix != sys.prefix:
                # virtual environment? Likewise, we have to adapt the paths
                # before comparing
                global_path = global_path.replace(sys.base_prefix, sys.prefix)
                base = base.replace(sys.base_prefix, sys.prefix)
            user_path = get_path(name, 'posix_user')
            self.assertEqual(user_path, global_path.replace(base, user, 1))

    def test_main(self):
        # just making sure _main() runs and returns things in the stdout
        with captured_stdout() as output:
            _main()
        self.assertTrue(len(output.getvalue().split('\n')) > 0)

    @unittest.skipIf(sys.platform == "win32", "Does not apply to Windows")
    def test_ldshared_value(self):
        ldflags = sysconfig.get_config_var('LDFLAGS')
        ldshared = sysconfig.get_config_var('LDSHARED')

        self.assertIn(ldflags, ldshared)

    @unittest.skipUnless(sys.platform == "darwin", "test only relevant on MacOSX")
    def test_platform_in_subprocess(self):
        my_platform = sysconfig.get_platform()

        # Test without MACOSX_DEPLOYMENT_TARGET in the environment

        env = os.environ.copy()
        if 'MACOSX_DEPLOYMENT_TARGET' in env:
            del env['MACOSX_DEPLOYMENT_TARGET']

        with open('/dev/null', 'w') as devnull_fp:
            p = subprocess.Popen([
                    sys.executable, '-c',
                    'import sysconfig; print(sysconfig.get_platform())',
                ],
                stdout=subprocess.PIPE,
                stderr=devnull_fp,
                env=env)
        test_platform = p.communicate()[0].strip()
        test_platform = test_platform.decode('utf-8')
        status = p.wait()

        self.assertEqual(status, 0)
        self.assertEqual(my_platform, test_platform)

        # Test with MACOSX_DEPLOYMENT_TARGET in the environment, and
        # using a value that is unlikely to be the default one.
        env = os.environ.copy()
        env['MACOSX_DEPLOYMENT_TARGET'] = '10.1'

        with open('/dev/null') as dev_null:
            p = subprocess.Popen([
                    sys.executable, '-c',
                    'import sysconfig; print(sysconfig.get_platform())',
                ],
                stdout=subprocess.PIPE,
                stderr=dev_null,
                env=env)
            test_platform = p.communicate()[0].strip()
            test_platform = test_platform.decode('utf-8')
            status = p.wait()

            self.assertEqual(status, 0)
            self.assertEqual(my_platform, test_platform)

    def test_srcdir(self):
        # See Issues #15322, #15364.
        srcdir = sysconfig.get_config_var('srcdir')

        self.assertTrue(os.path.isabs(srcdir), srcdir)
        self.assertTrue(os.path.isdir(srcdir), srcdir)

        if sysconfig._PYTHON_BUILD:
            # The python executable has not been installed so srcdir
            # should be a full source checkout.
            Python_h = os.path.join(srcdir, 'Include', 'Python.h')
            self.assertTrue(os.path.exists(Python_h), Python_h)
            self.assertTrue(sysconfig._is_python_source_dir(srcdir))
        elif os.name == 'posix':
            self.assertEqual(os.path.dirname(sysconfig.get_makefile_filename()),
                                srcdir)

    def test_srcdir_independent_of_cwd(self):
        # srcdir should be independent of the current working directory
        # See Issues #15322, #15364.
        srcdir = sysconfig.get_config_var('srcdir')
        cwd = os.getcwd()
        try:
            os.chdir('..')
            srcdir2 = sysconfig.get_config_var('srcdir')
        finally:
            os.chdir(cwd)
        self.assertEqual(srcdir, srcdir2)


class MakefileTests(unittest.TestCase):

    @unittest.skipIf(sys.platform.startswith('win'),
                     'Test is not Windows compatible')
    def test_get_makefile_filename(self):
        makefile = sysconfig.get_makefile_filename()
        self.assertTrue(os.path.isfile(makefile), makefile)

    def test_parse_makefile(self):
        self.addCleanup(unlink, TESTFN)
        with open(TESTFN, "w") as makefile:
            print("var1=a$(VAR2)", file=makefile)
            print("VAR2=b$(var3)", file=makefile)
            print("var3=42", file=makefile)
            print("var4=$/invalid", file=makefile)
            print("var5=dollar$$5", file=makefile)
        vars = sysconfig._parse_makefile(TESTFN)
        self.assertEqual(vars, {
            'var1': 'ab42',
            'VAR2': 'b42',
            'var3': 42,
            'var4': '$/invalid',
            'var5': 'dollar$5',
        })


def test_main():
    run_unittest(TestSysConfig, MakefileTests)

if __name__ == "__main__":
    test_main()
