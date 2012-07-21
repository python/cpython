"""
Test suite for _osx_support: shared OS X support functions.
"""

import os
import platform
import shutil
import stat
import sys
import unittest

import test.support

import _osx_support

@unittest.skipUnless(sys.platform.startswith("darwin"), "requires OS X")
class Test_OSXSupport(unittest.TestCase):

    def setUp(self):
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
        expected_vars.update(('_OSX_SUPPORT_INITIAL_'+ k,
                config_vars[k]) for k in config_vars
                    if config_vars[k] != expected_vars[k])

    def test__find_executable(self):
        if self.env['PATH']:
            self.env['PATH'] = self.env['PATH'] + ':'
        self.env['PATH'] = self.env['PATH'] + os.path.abspath(self.temp_path_dir)
        test.support.unlink(self.prog_name)
        self.assertIsNone(_osx_support._find_executable(self.prog_name))
        self.addCleanup(test.support.unlink, self.prog_name)
        with open(self.prog_name, 'w') as f:
            f.write("#!/bin/sh\n/bin/echo OK\n")
        os.chmod(self.prog_name, stat.S_IRWXU)
        self.assertEqual(self.prog_name,
                            _osx_support._find_executable(self.prog_name))

    def test__read_output(self):
        if self.env['PATH']:
            self.env['PATH'] = self.env['PATH'] + ':'
        self.env['PATH'] = self.env['PATH'] + os.path.abspath(self.temp_path_dir)
        test.support.unlink(self.prog_name)
        self.addCleanup(test.support.unlink, self.prog_name)
        with open(self.prog_name, 'w') as f:
            f.write("#!/bin/sh\n/bin/echo ExpectedOutput\n")
        os.chmod(self.prog_name, stat.S_IRWXU)
        self.assertEqual('ExpectedOutput',
                            _osx_support._read_output(self.prog_name))

    def test__find_build_tool(self):
        out = _osx_support._find_build_tool('cc')
        self.assertTrue(os.path.isfile(out),
                            'cc not found - check xcode-select')

    def test__get_system_version(self):
        self.assertTrue(platform.mac_ver()[0].startswith(
                                    _osx_support._get_system_version()))

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
        self.assertEqual(platform.mac_ver()[0].split('.') >= ['10', '4'],
                            _osx_support._supports_universal_builds())

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

        suffix = (':' + self.env['PATH']) if self.env['PATH'] else ''
        self.env['PATH'] = os.path.abspath(self.temp_path_dir) + suffix
        for c_name, c_output in compilers:
            test.support.unlink(c_name)
            self.addCleanup(test.support.unlink, c_name)
            with open(c_name, 'w') as f:
                f.write("#!/bin/sh\n/bin/echo " + c_output)
            os.chmod(c_name, stat.S_IRWXU)
        self.assertEqual(expected_vars,
                            _osx_support._find_appropriate_compiler(
                                    config_vars))

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
                            _osx_support._remove_universal_flags(
                                    config_vars))

    @unittest.skipUnless(shutil.which('clang'),'test requires clang')
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

        self.assertEqual(expected_vars,
                            _osx_support._remove_unsupported_archs(
                                    config_vars))

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

        self.assertEqual(expected_vars,
                            _osx_support._override_all_archs(
                                    config_vars))

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

        self.assertEqual(expected_vars,
                            _osx_support._check_for_unavailable_sdk(
                                    config_vars))

    def test_get_platform_osx(self):
        # Note, get_platform_osx is currently tested more extensively
        # indirectly by test_sysconfig and test_distutils
        config_vars = {
        'CFLAGS': '-fno-strict-aliasing  -g -O3 -arch ppc -arch i386  '
                        '-isysroot /Developer/SDKs/MacOSX10.1.sdk',
        'MACOSX_DEPLOYMENT_TARGET': '10.6',
        }
        result = _osx_support.get_platform_osx(config_vars, ' ', ' ', ' ')
        self.assertEqual(('macosx', '10.6', 'fat'), result)

def test_main():
    if sys.platform == 'darwin':
        test.support.run_unittest(Test_OSXSupport)

if __name__ == "__main__":
    test_main()
