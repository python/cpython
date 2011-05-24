"""Tests for 'site'.

Tests assume the initial paths in sys.path once the interpreter has begun
executing have not been removed.

"""
import unittest
import sys
import os
import subprocess
import shutil
from copy import copy, deepcopy

from test.support import (run_unittest, TESTFN, unlink, get_attribute,
                          captured_stdout, skip_unless_symlink)

import sysconfig
from sysconfig import (get_paths, get_platform, get_config_vars,
                       get_path, get_path_names, _INSTALL_SCHEMES,
                       _get_default_scheme, _expand_vars,
                       get_scheme_names, get_config_var, _main)

class TestSysConfig(unittest.TestCase):

    def setUp(self):
        """Make a copy of sys.path"""
        super(TestSysConfig, self).setUp()
        self.sys_path = sys.path[:]
        # patching os.uname
        if hasattr(os, 'uname'):
            self.uname = os.uname
            self._uname = os.uname()
        else:
            self.uname = None
            self._uname = None
        os.uname = self._get_uname
        # saving the environment
        self.name = os.name
        self.platform = sys.platform
        self.version = sys.version
        self.sep = os.sep
        self.join = os.path.join
        self.isabs = os.path.isabs
        self.splitdrive = os.path.splitdrive
        self._config_vars = copy(sysconfig._CONFIG_VARS)
        self.old_environ = deepcopy(os.environ)

    def tearDown(self):
        """Restore sys.path"""
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
        sysconfig._CONFIG_VARS = copy(self._config_vars)
        for key, value in self.old_environ.items():
            if os.environ.get(key) != value:
                os.environ[key] = value

        for key in list(os.environ.keys()):
            if key not in self.old_environ:
                del os.environ[key]

        super(TestSysConfig, self).tearDown()

    def _set_uname(self, uname):
        self._uname = uname

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
        wanted = list(wanted.items())
        wanted.sort()
        scheme = list(scheme.items())
        scheme.sort()
        self.assertEqual(scheme, wanted)

    def test_get_path(self):
        # xxx make real tests here
        for scheme in _INSTALL_SCHEMES:
            for name in _INSTALL_SCHEMES[scheme]:
                res = get_path(name, scheme)

    def test_get_config_vars(self):
        cvars = get_config_vars()
        self.assertTrue(isinstance(cvars, dict))
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
        get_config_vars()['MACOSX_DEPLOYMENT_TARGET'] = '10.3'
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
        get_config_vars()['MACOSX_DEPLOYMENT_TARGET'] = '10.4'
        get_config_vars()['CFLAGS'] = ('-arch ppc -arch i386 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')

        self.assertEqual(get_platform(), 'macosx-10.4-fat')

        get_config_vars()['CFLAGS'] = ('-arch x86_64 -arch i386 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')

        self.assertEqual(get_platform(), 'macosx-10.4-intel')

        get_config_vars()['CFLAGS'] = ('-arch x86_64 -arch ppc -arch i386 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')
        self.assertEqual(get_platform(), 'macosx-10.4-fat3')

        get_config_vars()['CFLAGS'] = ('-arch ppc64 -arch x86_64 -arch ppc -arch i386 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')
        self.assertEqual(get_platform(), 'macosx-10.4-universal')

        get_config_vars()['CFLAGS'] = ('-arch x86_64 -arch ppc64 -isysroot '
                                       '/Developer/SDKs/MacOSX10.4u.sdk  '
                                       '-fno-strict-aliasing -fno-common '
                                       '-dynamic -DNDEBUG -g -O3')

        self.assertEqual(get_platform(), 'macosx-10.4-fat64')

        for arch in ('ppc', 'i386', 'x86_64', 'ppc64'):
            get_config_vars()['CFLAGS'] = ('-arch %s -isysroot '
                                           '/Developer/SDKs/MacOSX10.4u.sdk  '
                                           '-fno-strict-aliasing -fno-common '
                                           '-dynamic -DNDEBUG -g -O3'%(arch,))

            self.assertEqual(get_platform(), 'macosx-10.4-%s'%(arch,))

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
            os.environ["Path"] = "{};{}".format(
                os.path.dirname(sys.executable), os.environ["Path"])

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
        # Issue 8759 : make sure the posix scheme for the users
        # is similar to the global posix_prefix one
        base = get_config_var('base')
        user = get_config_var('userbase')
        for name in ('stdlib', 'platstdlib', 'purelib', 'platlib'):
            global_path = get_path(name, 'posix_prefix')
            user_path = get_path(name, 'posix_user')
            self.assertEqual(user_path, global_path.replace(base, user))

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

        p = subprocess.Popen([
                sys.executable, '-c',
                'import sysconfig; print(sysconfig.get_platform())',
            ],
            stdout=subprocess.PIPE,
            stderr=open('/dev/null'),
            env=env)
        test_platform = p.communicate()[0].strip()
        test_platform = test_platform.decode('utf-8')
        status = p.wait()

        self.assertEqual(status, 0)
        self.assertEqual(my_platform, test_platform)


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
