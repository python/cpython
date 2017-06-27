"""
Test suite for _osx_support: shared OS X support functions.
"""

import os
import platform
import stat
import sys
import unittest
import test.support
import _osx_support

from unittest import mock
from distutils.tests import support


@unittest.skipUnless(sys.platform.startswith("darwin"), "requires OS X")
class Test_OSXSupport(support.LoggingSilencer,
                      unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.maxDiff = None
        self.prog_name = 'bogus_program_xxxx'
        self.temp_path_dir = os.path.abspath(os.getcwd())
        self.env = test.support.EnvironmentVarGuard()
        self.addCleanup(self.env.__exit__)
        for cv in ('CFLAGS', 'LDFLAGS', 'CPPFLAGS',
                   'BASECFLAGS', 'BLDSHARED', 'LDSHARED', 'CC',
                   'CXX', 'PY_CFLAGS', 'PY_LDFLAGS', 'PY_CPPFLAGS',
                   'PY_CORE_CFLAGS'):
            if cv in self.env:
                self.env.unset(cv)

    def add_expected_saved_initial_values(self, config_vars, expected_vars):
        # Ensure that the initial values for all modified config vars
        # are also saved with modified keys.
        expected_vars.update(
            ('_OSX_SUPPORT_INITIAL_' + k, config_vars[k])
            for k in config_vars
            if config_vars[k] != expected_vars[k]
        )

    def construct_env_path(self):
        path = '{env_path}{delim}{temp_path_dir}'.format(
            env_path=self.env['PATH'],
            delim=self.env['PATH'] and ':' or '',
            temp_path_dir=os.path.abspath(self.temp_path_dir),
        )
        return path

    def test__all__(self):
        test.support.check__all__(self, _osx_support)

    def test__find_executable(self):
        self.env['PATH'] = path = self.construct_env_path()
        prog_name_exe = '{}.exe'.format(self.prog_name)
        for prog_name in [self.prog_name, prog_name_exe]:
            test.support.unlink(prog_name)
            self.assertIsNone(_osx_support._find_executable(prog_name))
            self.addCleanup(test.support.unlink, prog_name)
            with open(prog_name, 'w') as f:
                f.write("#!/bin/sh\n/bin/echo OK\n")
            os.chmod(prog_name, stat.S_IRWXU)
            if prog_name.endswith('.exe'):
                with mock.patch('sys.platform', new='win32'):
                    self.assertEqual(
                        prog_name,
                        _osx_support._find_executable(self.prog_name)
                    )
            else:
                self.assertEqual(
                    prog_name, _osx_support._find_executable(self.prog_name)
                )
        self.assertEqual(
            self.prog_name,
            _osx_support._find_executable(self.prog_name, path=path)
        )

    def test__read_output(self):
        self.env['PATH'] = self.construct_env_path()
        test.support.unlink(self.prog_name)
        self.addCleanup(test.support.unlink, self.prog_name)
        with open(self.prog_name, 'w') as f:
            f.write("#!/bin/sh\n/bin/echo ExpectedOutput\n")
        os.chmod(self.prog_name, stat.S_IRWXU)
        self.assertEqual('ExpectedOutput',
                         _osx_support._read_output(self.prog_name))

        with mock.patch('tempfile.NamedTemporaryFile',
                        side_effect=ImportError):
            self.assertEqual('ExpectedOutput',
                             _osx_support._read_output(self.prog_name))

    def test__find_build_tool(self):
        out = _osx_support._find_build_tool('cc')
        self.assertTrue(os.path.isfile(out),
                        'cc not found - check xcode-select')

    def test__get_system_version(self):
        mac_ver = platform.mac_ver()[0]
        self.assertTrue(mac_ver.startswith(
                                    _osx_support._get_system_version()))
        with mock.patch('_osx_support._SYSTEM_VERSION', new=None):
            with mock.patch('re.search', return_value=None):
                self.assertEqual('', _osx_support._get_system_version())
            self.assertEqual('', _osx_support._get_system_version())
        with mock.patch('_osx_support._SYSTEM_VERSION', new=None):
            with mock.patch('builtins.open', side_effect=OSError):
                self.assertEqual('', _osx_support._get_system_version())

    def test__remove_original_values(self):
        config_vars = {
            'CC': 'gcc-test -pthreads',
        }
        expected_vars = {
            'CC': 'clang -pthreads',
        }
        cv = 'CC'
        newvalue = 'clang -pthreads'
        _osx_support._save_modified_value(config_vars, cv, newvalue)
        self.assertNotEqual(expected_vars, config_vars)
        _osx_support._remove_original_values(config_vars)
        self.assertEqual(expected_vars, config_vars)

    def test__save_modified_value(self):
        config_vars = {
            'CC': 'gcc-test -pthreads',
        }
        expected_vars = {
            'CC': 'clang -pthreads',
        }
        self.add_expected_saved_initial_values(config_vars, expected_vars)
        cv = 'CC'
        newvalue = 'clang -pthreads'
        _osx_support._save_modified_value(config_vars, cv, newvalue)
        self.assertEqual(expected_vars, config_vars)

    def test__save_modified_value_unchanged(self):
        config_vars = {
            'CC': 'gcc-test -pthreads',
        }
        expected_vars = config_vars.copy()
        cv = 'CC'
        newvalue = 'gcc-test -pthreads'
        _osx_support._save_modified_value(config_vars, cv, newvalue)
        self.assertEqual(expected_vars, config_vars)

    def test__supports_universal_builds(self):
        import platform
        mac_ver_tuple = tuple(
            int(i) for i in platform.mac_ver()[0].split('.')[0:2]
        )
        self.assertEqual(mac_ver_tuple >= (10, 4),
                         _osx_support._supports_universal_builds())
        with mock.patch('_osx_support._get_system_version', return_value=None):
            self.assertFalse(_osx_support._supports_universal_builds())

        with mock.patch('_osx_support._get_system_version',
                        return_value='some.invalid.value'):
            self.assertFalse(_osx_support._supports_universal_builds())

    def test__find_appropriate_compiler(self):
        compilers = (
            ('gcc-test', 'i686-apple-darwin11-llvm-gcc-4.2'),
            ('clang', 'clang version 3.1'),
        )
        config_vars = {
            'CC': 'gcc-test -pthreads',
            'CXX': 'cc++-test',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  ',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'gcc-test -bundle -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-test -bundle -arch ppc -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.4u.sdk -g',
        }
        expected_vars = {
            'CC': 'clang -pthreads',
            'CXX': 'clang++',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  ',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'clang -bundle -arch ppc -arch i386 -g',
            'LDSHARED': 'clang -bundle -arch ppc -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.4u.sdk -g',
        }
        self.add_expected_saved_initial_values(config_vars, expected_vars)

        self.env['PATH'] = self.construct_env_path()
        for c_name, c_output in compilers:
            test.support.unlink(c_name)
            self.addCleanup(test.support.unlink, c_name)
            with open(c_name, 'w') as f:
                f.write("#!/bin/sh\n/bin/echo " + c_output)
            os.chmod(c_name, stat.S_IRWXU)

        self.assertEqual(
            expected_vars, _osx_support._find_appropriate_compiler(config_vars)
        )

        with mock.patch('_osx_support._read_output',
                        return_value='bogus_data'):
            with mock.patch('os.path.basename', return_value='gcc'):
                self.assertEqual(
                    expected_vars,
                    _osx_support._find_appropriate_compiler(config_vars)
                )

        with mock.patch.dict(os.environ, {'CC': 'gcc-test'}, clear=True):
            self.assertEqual(
                expected_vars,
                _osx_support._find_appropriate_compiler(config_vars)
            )

        with mock.patch('os.path.basename',
                        return_value='not_starts_with_gcc'):
            self.assertEqual(
                expected_vars,
                _osx_support._find_appropriate_compiler(config_vars)
            )

        with mock.patch('_osx_support._find_executable', return_value=None):
            result = _osx_support._find_appropriate_compiler(config_vars)
            for key, value in expected_vars.items():
                self.assertTrue(result[key].endswith(value))

            with mock.patch('_osx_support._find_build_tool',
                            return_value=None):
                with self.assertRaises(SystemError) as cm:
                    _osx_support._find_appropriate_compiler(config_vars)

                self.assertEqual(str(cm.exception),
                                 'Cannot locate working compiler')

    def test__remove_universal_flags(self):
        config_vars = {
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  ',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.4u.sdk -g',
        }
        expected_vars = {
            'CFLAGS': '-fno-strict-aliasing  -g -O3    ',
            'LDFLAGS': '    -g',
            'CPPFLAGS': '-I.  ',
            'BLDSHARED': 'gcc-4.0 -bundle    -g',
            'LDSHARED': 'gcc-4.0 -bundle      -g',
        }
        self.add_expected_saved_initial_values(config_vars, expected_vars)

        self.assertEqual(expected_vars,
                         _osx_support._remove_universal_flags(config_vars))

    def test__remove_unsupported_archs(self):
        config_vars = {
            'CC': 'clang',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  ',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.4u.sdk -g',
        }
        expected_vars = {
            'CC': 'clang',
            'CFLAGS': '-fno-strict-aliasing  -g -O3  -arch i386  ',
            'LDFLAGS': ' -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'gcc-4.0 -bundle   -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle   -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.4u.sdk -g',
        }
        self.add_expected_saved_initial_values(config_vars, expected_vars)

        self.env['PATH'] = self.construct_env_path()
        c_name = 'clang'
        test.support.unlink(c_name)
        self.addCleanup(test.support.unlink, c_name)
        # exit status 255 means no PPC support in this compiler chain
        with open(c_name, 'w') as f:
            f.write("#!/bin/sh\nexit 255")
        os.chmod(c_name, stat.S_IRWXU)

        with mock.patch('os.system', return_value=None):
            self.assertEqual(
                config_vars,
                _osx_support._remove_unsupported_archs(config_vars)
            )

        self.assertEqual(
            expected_vars, _osx_support._remove_unsupported_archs(config_vars)
        )

        with mock.patch.dict(os.environ, {'CC': 'gcc-test'}, clear=True):
            self.assertEqual(
                expected_vars,
                _osx_support._remove_unsupported_archs(config_vars)
            )

        with mock.patch('re.search', return_value=None):
            self.assertEqual(
                expected_vars,
                _osx_support._remove_unsupported_archs(config_vars)
            )

    def test__override_all_archs(self):
        self.env['ARCHFLAGS'] = '-arch x86_64'
        config_vars = {
            'CC': 'clang',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  ',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle -arch ppc -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.4u.sdk -g',
        }
        expected_vars = {
            'CC': 'clang',
            'CFLAGS': '-fno-strict-aliasing  -g -O3     -arch x86_64',
            'LDFLAGS': '    -g -arch x86_64',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'gcc-4.0 -bundle    -g -arch x86_64',
            'LDSHARED': 'gcc-4.0 -bundle   -isysroot '
                        '/Developer/SDKs/MacOSX10.4u.sdk -g -arch x86_64',
        }
        self.add_expected_saved_initial_values(config_vars, expected_vars)

        self.assertEqual(
            expected_vars, _osx_support._override_all_archs(config_vars)
        )

        with mock.patch.dict(os.environ, {}, clear=True):
            self.assertEqual(
                config_vars, _osx_support._override_all_archs(config_vars)
            )

    def test__check_for_unavailable_sdk(self):
        config_vars = {
            'CC': 'clang',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  '
                      '-isysroot /Developer/SDKs/MacOSX10.1.sdk',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.1.sdk',
            'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.1.sdk -g',
        }
        expected_vars = {
            'CC': 'clang',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  '
                      ' ',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I.  ',
            'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 '
                        ' -g',
        }
        self.add_expected_saved_initial_values(config_vars, expected_vars)

        with mock.patch('re.search', return_value=None):
            self.assertEqual(
                config_vars, _osx_support._check_for_unavailable_sdk(
                    config_vars
                )
            )

        with mock.patch('os.path.exists', return_value=True):
            self.assertEqual(
                config_vars, _osx_support._check_for_unavailable_sdk(
                    config_vars
                )
            )

        self.assertEqual(
            expected_vars, _osx_support._check_for_unavailable_sdk(config_vars)
        )

    def test_get_platform_osx(self):
        # Note, get_platform_osx is also tested
        # indirectly by test_sysconfig and test_distutils
        config_vars = {
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  '
                      '-isysroot /Developer/SDKs/MacOSX10.1.sdk',
            'MACOSX_DEPLOYMENT_TARGET': '10.6',
        }
        result_fat = _osx_support.get_platform_osx(config_vars, ' ', ' ', ' ')

        with mock.patch('_osx_support._get_system_version',
                        return_value='10.3'):
            with mock.patch('sys.maxsize', new=65536):
                result_ppc = _osx_support.get_platform_osx(
                    config_vars, ' ', ' ', 'PowerPC'
                )
                result_i386 = _osx_support.get_platform_osx(
                    config_vars, ' ', ' ', 'i386'
                )
            with mock.patch('sys.maxsize', new=4294967296):
                result_ppc64 = _osx_support.get_platform_osx(
                    config_vars, ' ', ' ', 'PowerPC'
                )

                result_x86_64 = _osx_support.get_platform_osx(
                    config_vars, ' ', ' ', 'i386'
                )

        with mock.patch('_osx_support._get_system_version', return_value='TT'):
            self.assertEqual(
                ('macosx', '10.6', ' '),
                _osx_support.get_platform_osx(config_vars, ' ', ' ', ' ')
            )

        archs = (
            (('i386',), 'i386'),
            (('i386', 'ppc'), 'fat'),
            (('i386', 'x86_64'), 'intel'),
            (('i386', 'ppc', 'x86_64'), 'fat3'),
            (('ppc64', 'x86_64'), 'fat64'),
            (('i386', 'ppc', 'ppc64', 'x86_64'), 'universal'),
        )

        for arch, machine in archs:
            with mock.patch('re.findall', return_value=arch):
                self.assertEqual(
                    ('macosx', '10.6', machine),
                    _osx_support.get_platform_osx(config_vars, ' ', ' ', ' ')
                )

        with mock.patch('re.findall', return_value=('exception', 'value')):
            with self.assertRaises(ValueError) as cm:
                _osx_support.get_platform_osx(config_vars, ' ', ' ', ' ')
            self.assertEqual(
                str(cm.exception),
                "Don't know machine value for archs=('exception', 'value')"
            )

        config_vars['MACOSX_DEPLOYMENT_TARGET'] = None
        with mock.patch('_osx_support._get_system_version', return_value=None):
            result_empty = _osx_support.get_platform_osx(config_vars,
                                                         ' ', ' ', ' ')

        assertions = (
            (('macosx', '10.6', 'fat'), result_fat),
            (('macosx', '10.6', 'ppc'), result_ppc),
            (('macosx', '10.6', 'i386'), result_i386),
            (('macosx', '10.6', 'ppc64'), result_ppc64),
            (('macosx', '10.6', 'x86_64'), result_x86_64),
            ((' ', ' ', ' '), result_empty),
        )
        for expected, result in assertions:
            self.assertEqual(expected, result)

    def test_customize_config_vars(self):
        config_vars = {
            'CC': 'clang',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  '
                      '-isysroot /Developer/SDKs/MacOSX10.1.sdk',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.1.sdk',
            'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle -arch ppc -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.1.sdk -g',
        }

        expected_vars = {
            'CC': 'clang',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386   ',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I.  ',
            'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle -arch ppc -arch i386  -g',
        }

        self.add_expected_saved_initial_values(config_vars, expected_vars)

        self.assertEqual(expected_vars,
                         _osx_support.customize_config_vars(config_vars))

        with mock.patch('_osx_support._supports_universal_builds',
                        return_value=None):
            config_vars = {
                'CC': 'clang',
                'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  '
                          '-isysroot /Developer/SDKs/MacOSX10.1.sdk',
                'LDFLAGS': '-arch ppc -arch i386   -g',
                'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.1.sdk',
                'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
                'LDSHARED': 'gcc-4.0 -bundle -arch ppc -arch i386 '
                            '-isysroot /Developer/SDKs/MacOSX10.1.sdk -g',
            }
            expected_vars = {
                'CC': 'clang',
                'CFLAGS': '-fno-strict-aliasing  -g -O3     ',
                'LDFLAGS': '    -g',
                'CPPFLAGS': '-I.  ',
                'BLDSHARED': 'gcc-4.0 -bundle    -g',
                'LDSHARED': 'gcc-4.0 -bundle     -g',
            }

            self.add_expected_saved_initial_values(config_vars, expected_vars)
            self.assertEqual(expected_vars,
                             _osx_support.customize_config_vars(config_vars))

    def test_customize_compiler(self):
        compilers = (
            ('gcc-test', 'i686-apple-darwin11-llvm-gcc-4.2'),
            ('clang', 'clang version 3.1'),
        )
        config_vars = {
            'CC': 'gcc-test -pthreads',
            'CXX': 'cc++-test',
            'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  ',
            'LDFLAGS': '-arch ppc -arch i386   -g',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 -g',
            'LDSHARED': 'gcc-4.0 -bundle  -arch ppc -arch i386 '
                        '-isysroot /Developer/SDKs/MacOSX10.4u.sdk -g',
        }
        expected_vars = {
            'CC': 'clang -pthreads',
            'CXX': 'clang++',
            'CFLAGS': '-fno-strict-aliasing  -g -O3     -arch x86_64',
            'LDFLAGS': '    -g -arch x86_64',
            'CPPFLAGS': '-I. -isysroot /Developer/SDKs/MacOSX10.4u.sdk',
            'BLDSHARED': 'clang -bundle   -g -arch x86_64',
            'LDSHARED': 'clang -bundle   -isysroot '
                        '/Developer/SDKs/MacOSX10.4u.sdk -g -arch x86_64',
        }
        self.add_expected_saved_initial_values(config_vars, expected_vars)

        self.env['PATH'] = self.construct_env_path()
        self.env['ARCHFLAGS'] = '-arch x86_64'
        for c_name, c_output in compilers:
            test.support.unlink(c_name)
            self.addCleanup(test.support.unlink, c_name)
            with open(c_name, 'w') as f:
                f.write("#!/bin/sh\n/bin/echo " + c_output)
            os.chmod(c_name, stat.S_IRWXU)

        self.assertEqual(expected_vars,
                         _osx_support.customize_compiler(config_vars))

    def test_compiler_fixup(self):
        compiler_so = (
            '-isysroot', 'PATH', '-arch',
            'ARCH', 'i386', 'ppc', '-isysroot'
        )
        cc_args = '-isysroot PATH -isysroot'
        expected = ['-arch', 'ARCH', 'i386', 'ppc']
        self.assertEqual(expected,
                         _osx_support.compiler_fixup(compiler_so, cc_args))
        self.env['ARCHFLAGS'] = '-arch x86_64'
        expected = ['i386', 'ppc', 'x86_64']
        self.assertEqual(expected,
                         _osx_support.compiler_fixup(compiler_so, cc_args))

        cc_args = '-arch ARCH'
        expected = ['-isysroot', 'PATH', 'i386', 'ppc', '-isysroot']
        self.assertEqual(expected,
                         _osx_support.compiler_fixup(compiler_so, cc_args))

        cc_args = '-isysroot PATH -arch ARCH'
        expected = ['i386', 'ppc']
        self.assertEqual(expected,
                         _osx_support.compiler_fixup(compiler_so, cc_args))

        with mock.patch('_osx_support._supports_universal_builds',
                        return_value=False):
            self.assertEqual(
                expected,
                _osx_support.compiler_fixup(compiler_so, cc_args)
            )

        compiler_so = ('PATH', '-arch', 'ARCH', 'i386', 'ppc')
        cc_args = 'PATH'
        expected = ['PATH', 'i386', 'ppc', '-arch', 'x86_64']
        self.assertEqual(expected,
                         _osx_support.compiler_fixup(compiler_so, cc_args))

if __name__ == "__main__":
    unittest.main()
