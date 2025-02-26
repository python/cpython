import platform
import re
import unittest
import sys
import os
import subprocess
import shutil
import json
import textwrap
from copy import copy

from test.support import (
    captured_stdout,
    is_apple_mobile,
    is_wasi,
    PythonSymlink,
    requires_subprocess,
)
from test.support.import_helper import import_module
from test.support.os_helper import (TESTFN, unlink, skip_unless_symlink,
                                    change_cwd)
from test.support.venv import VirtualEnvironment

import sysconfig
from sysconfig import (get_paths, get_platform, get_config_vars,
                       get_path, get_path_names, _INSTALL_SCHEMES,
                       get_default_scheme, get_scheme_names, get_config_var,
                       _expand_vars, _get_preferred_schemes)
from sysconfig.__main__ import _main, _parse_makefile
import _imp
import _osx_support
import _sysconfig


HAS_USER_BASE = sysconfig._HAS_USER_BASE


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
        self._framework = sys._framework
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
        sys._framework = self._framework
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

    def venv(self, **venv_create_args):
        return VirtualEnvironment.from_tmpdir(
            prefix=f'{self.id()}-venv-',
            **venv_create_args,
        )

    def test_get_path_names(self):
        self.assertEqual(get_path_names(), sysconfig._SCHEME_KEYS)

    def test_get_paths(self):
        scheme = get_paths()
        default_scheme = get_default_scheme()
        wanted = _expand_vars(default_scheme, None)
        wanted = sorted(wanted.items())
        scheme = sorted(scheme.items())
        self.assertEqual(scheme, wanted)

    def test_get_path(self):
        config_vars = get_config_vars()
        if os.name == 'nt':
            # On Windows, we replace the native platlibdir name with the
            # default so that POSIX schemes resolve correctly
            config_vars = config_vars | {'platlibdir': 'lib'}
        for scheme in _INSTALL_SCHEMES:
            for name in _INSTALL_SCHEMES[scheme]:
                expected = _INSTALL_SCHEMES[scheme][name].format(**config_vars)
                self.assertEqual(
                    os.path.normpath(get_path(name, scheme)),
                    os.path.normpath(expected),
                )

    def test_get_default_scheme(self):
        self.assertIn(get_default_scheme(), _INSTALL_SCHEMES)

    def test_get_preferred_schemes(self):
        expected_schemes = {'prefix', 'home', 'user'}

        # Windows.
        os.name = 'nt'
        schemes = _get_preferred_schemes()
        self.assertIsInstance(schemes, dict)
        self.assertEqual(set(schemes), expected_schemes)

        # Mac and Linux, shared library build.
        os.name = 'posix'
        schemes = _get_preferred_schemes()
        self.assertIsInstance(schemes, dict)
        self.assertEqual(set(schemes), expected_schemes)

        # Mac, framework build.
        os.name = 'posix'
        sys.platform = 'darwin'
        sys._framework = "MyPython"
        self.assertIsInstance(schemes, dict)
        self.assertEqual(set(schemes), expected_schemes)

    def test_posix_venv_scheme(self):
        # The following directories were hardcoded in the venv module
        # before bpo-45413, here we assert the posix_venv scheme does not regress
        binpath = 'bin'
        incpath = 'include'
        libpath = os.path.join('lib',
                               f'python{sysconfig._get_python_version_abi()}',
                               'site-packages')

        # Resolve the paths in an imaginary venv/ directory
        binpath = os.path.join('venv', binpath)
        incpath = os.path.join('venv', incpath)
        libpath = os.path.join('venv', libpath)

        # Mimic the venv module, set all bases to the venv directory
        bases = ('base', 'platbase', 'installed_base', 'installed_platbase')
        vars = {base: 'venv' for base in bases}

        self.assertEqual(binpath, sysconfig.get_path('scripts', scheme='posix_venv', vars=vars))
        self.assertEqual(libpath, sysconfig.get_path('purelib', scheme='posix_venv', vars=vars))

        # The include directory on POSIX isn't exactly the same as before,
        # but it is "within"
        sysconfig_includedir = sysconfig.get_path('include', scheme='posix_venv', vars=vars)
        self.assertTrue(sysconfig_includedir.startswith(incpath + os.sep))

    def test_nt_venv_scheme(self):
        # The following directories were hardcoded in the venv module
        # before bpo-45413, here we assert the posix_venv scheme does not regress
        binpath = 'Scripts'
        incpath = 'Include'
        libpath = os.path.join('Lib', 'site-packages')

        # Resolve the paths in an imaginary venv\ directory
        venv = 'venv'
        binpath = os.path.join(venv, binpath)
        incpath = os.path.join(venv, incpath)
        libpath = os.path.join(venv, libpath)

        # Mimic the venv module, set all bases to the venv directory
        bases = ('base', 'platbase', 'installed_base', 'installed_platbase')
        vars = {base: 'venv' for base in bases}

        self.assertEqual(binpath, sysconfig.get_path('scripts', scheme='nt_venv', vars=vars))
        self.assertEqual(incpath, sysconfig.get_path('include', scheme='nt_venv', vars=vars))
        self.assertEqual(libpath, sysconfig.get_path('purelib', scheme='nt_venv', vars=vars))

    def test_venv_scheme(self):
        if sys.platform == 'win32':
            self.assertEqual(
                sysconfig.get_path('scripts', scheme='venv'),
                sysconfig.get_path('scripts', scheme='nt_venv')
            )
            self.assertEqual(
                sysconfig.get_path('include', scheme='venv'),
                sysconfig.get_path('include', scheme='nt_venv')
            )
            self.assertEqual(
                sysconfig.get_path('purelib', scheme='venv'),
                sysconfig.get_path('purelib', scheme='nt_venv')
            )
        else:
            self.assertEqual(
                sysconfig.get_path('scripts', scheme='venv'),
                sysconfig.get_path('scripts', scheme='posix_venv')
            )
            self.assertEqual(
                sysconfig.get_path('include', scheme='venv'),
                sysconfig.get_path('include', scheme='posix_venv')
            )
            self.assertEqual(
                sysconfig.get_path('purelib', scheme='venv'),
                sysconfig.get_path('purelib', scheme='posix_venv')
            )

    def test_get_config_vars(self):
        cvars = get_config_vars()
        self.assertIsInstance(cvars, dict)
        self.assertTrue(cvars)

    def test_get_platform(self):
        # Check the actual platform returns something reasonable.
        actual_platform = get_platform()
        self.assertIsInstance(actual_platform, str)
        self.assertTrue(actual_platform)

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

        # Android
        os.name = 'posix'
        sys.platform = 'android'
        get_config_vars()['ANDROID_API_LEVEL'] = 9
        for machine, abi in {
            'x86_64': 'x86_64',
            'i686': 'x86',
            'aarch64': 'arm64_v8a',
            'armv7l': 'armeabi_v7a',
        }.items():
            with self.subTest(machine):
                self._set_uname(('Linux', 'localhost', '3.18.91+',
                                '#1 Tue Jan 9 20:35:43 UTC 2018', machine))
                self.assertEqual(get_platform(), f'android-9-{abi}')

        # XXX more platforms to tests here

    @unittest.skipIf(is_wasi, "Incompatible with WASI mapdir and OOT builds")
    @unittest.skipIf(is_apple_mobile,
                     f"{sys.platform} doesn't distribute header files in the runtime environment")
    def test_get_config_h_filename(self):
        config_h = sysconfig.get_config_h_filename()
        self.assertTrue(os.path.isfile(config_h), config_h)

    def test_get_scheme_names(self):
        wanted = ['nt', 'posix_home', 'posix_prefix', 'posix_venv', 'nt_venv', 'venv']
        if HAS_USER_BASE:
            wanted.extend(['nt_user', 'osx_framework_user', 'posix_user'])
        self.assertEqual(get_scheme_names(), tuple(sorted(wanted)))

    @skip_unless_symlink
    @requires_subprocess()
    def test_symlink(self): # Issue 7880
        with PythonSymlink() as py:
            cmd = "-c", "import sysconfig; print(sysconfig.get_platform())"
            self.assertEqual(py.call_real(*cmd), py.call_link(*cmd))

    def test_user_similar(self):
        # Issue #8759: make sure the posix scheme for the users
        # is similar to the global posix_prefix one
        base = get_config_var('base')
        if HAS_USER_BASE:
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
            if HAS_USER_BASE:
                user_path = get_path(name, 'posix_user')
                expected = os.path.normpath(global_path.replace(base, user, 1))
                # bpo-44860: platlib of posix_user doesn't use sys.platlibdir,
                # whereas posix_prefix does.
                if name == 'platlib':
                    # Replace "/lib64/python3.11/site-packages" suffix
                    # with "/lib/python3.11/site-packages".
                    py_version_abi = sysconfig._get_python_version_abi()
                    suffix = f'python{py_version_abi}/site-packages'
                    expected = expected.replace(f'/{sys.platlibdir}/{suffix}',
                                                f'/lib/{suffix}')
                self.assertEqual(user_path, expected)

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

    @unittest.skipIf(not _imp.extension_suffixes(), "stub loader has no suffixes")
    def test_soabi(self):
        soabi = sysconfig.get_config_var('SOABI')
        self.assertIn(soabi, _imp.extension_suffixes()[0])

    def test_library(self):
        library = sysconfig.get_config_var('LIBRARY')
        ldlibrary = sysconfig.get_config_var('LDLIBRARY')
        major, minor = sys.version_info[:2]
        if sys.platform == 'win32':
            self.assertTrue(library.startswith(f'python{major}{minor}'))
            self.assertTrue(library.endswith('.dll'))
            self.assertEqual(library, ldlibrary)
        elif is_apple_mobile:
            framework = sysconfig.get_config_var('PYTHONFRAMEWORK')
            self.assertEqual(ldlibrary, f"{framework}.framework/{framework}")
        else:
            self.assertTrue(library.startswith(f'libpython{major}.{minor}'))
            self.assertTrue(library.endswith('.a'))
            if sys.platform == 'darwin' and sys._framework:
                self.skipTest('gh-110824: skip LDLIBRARY test for framework build')
            else:
                self.assertTrue(ldlibrary.startswith(f'libpython{major}.{minor}'))

    @unittest.skipUnless(sys.platform == "darwin", "test only relevant on MacOSX")
    @requires_subprocess()
    def test_platform_in_subprocess(self):
        my_platform = sysconfig.get_platform()

        # Test without MACOSX_DEPLOYMENT_TARGET in the environment

        env = os.environ.copy()
        if 'MACOSX_DEPLOYMENT_TARGET' in env:
            del env['MACOSX_DEPLOYMENT_TARGET']

        p = subprocess.Popen([
                sys.executable, '-c',
                'import sysconfig; print(sysconfig.get_platform())',
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
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
            stderr=subprocess.DEVNULL,
            env=env)
        test_platform = p.communicate()[0].strip()
        test_platform = test_platform.decode('utf-8')
        status = p.wait()

        self.assertEqual(status, 0)
        self.assertEqual(my_platform, test_platform)

    @unittest.skipIf(is_wasi, "Incompatible with WASI mapdir and OOT builds")
    @unittest.skipIf(is_apple_mobile,
                     f"{sys.platform} doesn't include config folder at runtime")
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
            # <srcdir>/PC/pyconfig.h.in always exists even if unused
            pyconfig_h = os.path.join(srcdir, 'PC', 'pyconfig.h.in')
            self.assertTrue(os.path.exists(pyconfig_h), pyconfig_h)
            pyconfig_h_in = os.path.join(srcdir, 'pyconfig.h.in')
            self.assertTrue(os.path.exists(pyconfig_h_in), pyconfig_h_in)
            if os.name == 'nt':
                # <executable dir>/pyconfig.h exists on Windows in a build tree
                pyconfig_h = os.path.join(sys.executable, '..', 'pyconfig.h')
                self.assertTrue(os.path.exists(pyconfig_h), pyconfig_h)
        elif os.name == 'posix':
            makefile_dir = os.path.dirname(sysconfig.get_makefile_filename())
            # Issue #19340: srcdir has been realpath'ed already
            makefile_dir = os.path.realpath(makefile_dir)
            self.assertEqual(makefile_dir, srcdir)

    def test_srcdir_independent_of_cwd(self):
        # srcdir should be independent of the current working directory
        # See Issues #15322, #15364.
        srcdir = sysconfig.get_config_var('srcdir')
        with change_cwd(os.pardir):
            srcdir2 = sysconfig.get_config_var('srcdir')
        self.assertEqual(srcdir, srcdir2)

    @unittest.skipIf(sysconfig.get_config_var('EXT_SUFFIX') is None,
                     'EXT_SUFFIX required for this test')
    @unittest.skipIf(not _imp.extension_suffixes(), "stub loader has no suffixes")
    def test_EXT_SUFFIX_in_vars(self):
        vars = sysconfig.get_config_vars()
        self.assertEqual(vars['EXT_SUFFIX'], _imp.extension_suffixes()[0])

    @unittest.skipUnless(sys.platform == 'linux', 'Linux-specific test')
    def test_linux_ext_suffix(self):
        ctypes = import_module('ctypes')
        machine = platform.machine()
        suffix = sysconfig.get_config_var('EXT_SUFFIX')
        if re.match('(aarch64|arm|mips|ppc|powerpc|s390|sparc)', machine):
            self.assertTrue('linux' in suffix, suffix)
        if re.match('(i[3-6]86|x86_64)$', machine):
            if ctypes.sizeof(ctypes.c_char_p()) == 4:
                expected_suffixes = 'i386-linux-gnu.so', 'x86_64-linux-gnux32.so', 'i386-linux-musl.so'
            else: # 8 byte pointer size
                expected_suffixes = 'x86_64-linux-gnu.so', 'x86_64-linux-musl.so'
            self.assertTrue(suffix.endswith(expected_suffixes),
                            f'unexpected suffix {suffix!r}')

    @unittest.skipUnless(sys.platform == 'android', 'Android-specific test')
    def test_android_ext_suffix(self):
        machine = platform.machine()
        suffix = sysconfig.get_config_var('EXT_SUFFIX')
        expected_triplet = {
            "x86_64": "x86_64-linux-android",
            "i686": "i686-linux-android",
            "aarch64": "aarch64-linux-android",
            "armv7l": "arm-linux-androideabi",
        }[machine]
        self.assertTrue(suffix.endswith(f"-{expected_triplet}.so"),
                        f"{machine=}, {suffix=}")

    @unittest.skipUnless(sys.platform == 'darwin', 'OS X-specific test')
    def test_osx_ext_suffix(self):
        suffix = sysconfig.get_config_var('EXT_SUFFIX')
        self.assertTrue(suffix.endswith('-darwin.so'), suffix)

    @requires_subprocess()
    def test_config_vars_depend_on_site_initialization(self):
        script = textwrap.dedent("""
            import sysconfig

            config_vars = sysconfig.get_config_vars()

            import json
            print(json.dumps(config_vars, indent=2))
        """)

        with self.venv() as venv:
            site_config_vars = json.loads(venv.run('-c', script).stdout)
            no_site_config_vars = json.loads(venv.run('-S', '-c', script).stdout)

        self.assertNotEqual(site_config_vars, no_site_config_vars)
        # With the site initialization, the virtual environment should be enabled.
        self.assertEqual(site_config_vars['base'], venv.prefix)
        self.assertEqual(site_config_vars['platbase'], venv.prefix)
        #self.assertEqual(site_config_vars['prefix'], venv.prefix)  # # FIXME: prefix gets overwriten by _init_posix
        # Without the site initialization, the virtual environment should be disabled.
        self.assertEqual(no_site_config_vars['base'], site_config_vars['installed_base'])
        self.assertEqual(no_site_config_vars['platbase'], site_config_vars['installed_platbase'])

    @requires_subprocess()
    def test_config_vars_recalculation_after_site_initialization(self):
        script = textwrap.dedent("""
            import sysconfig

            before = sysconfig.get_config_vars()

            import site
            site.main()

            after = sysconfig.get_config_vars()

            import json
            print(json.dumps({'before': before, 'after': after}, indent=2))
        """)

        with self.venv() as venv:
            config_vars = json.loads(venv.run('-S', '-c', script).stdout)

        self.assertNotEqual(config_vars['before'], config_vars['after'])
        self.assertEqual(config_vars['after']['base'], venv.prefix)
        #self.assertEqual(config_vars['after']['prefix'], venv.prefix)  # FIXME: prefix gets overwriten by _init_posix
        #self.assertEqual(config_vars['after']['exec_prefix'], venv.prefix)  # FIXME: exec_prefix gets overwriten by _init_posix

    @requires_subprocess()
    def test_paths_depend_on_site_initialization(self):
        script = textwrap.dedent("""
            import sysconfig

            paths = sysconfig.get_paths()

            import json
            print(json.dumps(paths, indent=2))
        """)

        with self.venv() as venv:
            site_paths = json.loads(venv.run('-c', script).stdout)
            no_site_paths = json.loads(venv.run('-S', '-c', script).stdout)

        self.assertNotEqual(site_paths, no_site_paths)


class MakefileTests(unittest.TestCase):

    @unittest.skipIf(sys.platform.startswith('win'),
                     'Test is not Windows compatible')
    @unittest.skipIf(is_wasi, "Incompatible with WASI mapdir and OOT builds")
    @unittest.skipIf(is_apple_mobile,
                     f"{sys.platform} doesn't include config folder at runtime")
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
            print("var6=${var3}/lib/python3.5/config-$(VAR2)$(var5)"
                  "-x86_64-linux-gnu", file=makefile)
        vars = _parse_makefile(TESTFN)
        self.assertEqual(vars, {
            'var1': 'ab42',
            'VAR2': 'b42',
            'var3': 42,
            'var4': '$/invalid',
            'var5': 'dollar$5',
            'var6': '42/lib/python3.5/config-b42dollar$5-x86_64-linux-gnu',
        })


class DeprecationTests(unittest.TestCase):
    def deprecated(self, removal_version, deprecation_msg=None, error=Exception, error_msg=None):
        if sys.version_info >= removal_version:
            return self.assertRaises(error, msg=error_msg)
        else:
            return self.assertWarns(DeprecationWarning, msg=deprecation_msg)

    def test_is_python_build_check_home(self):
        with self.deprecated(
            removal_version=(3, 15),
            deprecation_msg=(
                'The check_home argument of sysconfig.is_python_build is '
                'deprecated and its value is ignored. '
                'It will be removed in Python 3.15.'
            ),
            error=TypeError,
            error_msg="is_python_build() takes 0 positional arguments but 1 were given",
        ):
            sysconfig.is_python_build('foo')


if __name__ == "__main__":
    unittest.main()
