"""Test Tools/build/generate_getbuildinfo.py."""

import contextlib
import locale
import os
import subprocess
import sys
import textwrap
import unittest
from unittest import mock
from test import support
from test.support import os_helper
from test import test_tools


test_tools.skip_if_missing("build")

with test_tools.imports_under_tool('build'):
    import generate_getbuildinfo

SRC_DIR = os.path.dirname(test_tools.toolsdir)
SCRIPT_NAME = 'generate_getbuildinfo.py'


class TestGetBuildInfo(unittest.TestCase):
    @contextlib.contextmanager
    def check_log(self, log):
        try:
            with support.captured_stdout() as stdout:
                yield
        finally:
            self.assertEqual(stdout.getvalue(),
                             f'{SCRIPT_NAME}: {log}\n')

    @contextlib.contextmanager
    def check_exit_fail(self, errmsg):
        with self.check_log(errmsg):
            try:
                yield
            except SystemExit as exc:
                self.assertEqual(exc.code, 1)

    def test_get_py_version(self):
        with os_helper.change_cwd(SRC_DIR):
            PY_VERSION = generate_getbuildinfo.get_py_version()

        # "./python -V" writes "Python {PY_VERSION}"
        proc = subprocess.run([sys.executable, '-V'],
                              stdout=subprocess.PIPE,
                              text=True, check=True)
        python_version = proc.stdout.rstrip()
        self.assertEqual(f'Python {PY_VERSION}', python_version)

    def test_get_date_time(self):
        # Test get_date_time() with SOURCE_DATE_EPOCH env var
        def check(timestamp, date, time):
            with os_helper.EnvironmentVarGuard() as env:
                env['SOURCE_DATE_EPOCH'] = str(timestamp)
                result = generate_getbuildinfo.get_date_time()

            self.assertEqual(result, (date, time))

        check(0, 'Jan  1 1970', '00:00:00')
        check(1791419852, 'Oct  8 2026', '00:37:32')

    def test_get_build_info(self):
        get_build_info = generate_getbuildinfo.get_build_info

        with os_helper.EnvironmentVarGuard() as env:
            env['SOURCE_DATE_EPOCH'] = '0'
            date_time = 'Jan  1 1970, 00:00:00'
            # all set
            self.assertEqual(get_build_info('tag', 'branch', 'version'),
                             (f'tag:version, {date_time}', 'tag'))
            # missing version
            self.assertEqual(get_build_info('tag', 'branch', ''),
                             (f'tag, {date_time}', 'tag'))
            # Missing tag
            self.assertEqual(get_build_info('', 'branch', 'version'),
                             (f'branch:version, {date_time}', 'branch'))
            # undefined tag
            self.assertEqual(get_build_info('undefined', 'branch', 'version'),
                             (f'branch:version, {date_time}', 'branch'))
            # all empty
            self.assertEqual(get_build_info('', '', ''),
                             (f'main, {date_time}', 'main'))

    def test_get_compiler(self):
        GETCOMPILER = 'GETCOMPILER'
        MOCK_COMPILER = 'MOCK_COMPILER'
        CC = 'gcc -std=c11'

        for hostrunner in ('', 'node'):
            for use_cc in (False, True):
                with (mock.patch.object(generate_getbuildinfo,
                                       'get_hostrunner',
                                       return_value=hostrunner),
                     mock.patch.object(generate_getbuildinfo,
                                       'get_makefile_cc',
                                       return_value=CC),
                     mock.patch.object(generate_getbuildinfo,
                                       'run_command') as mock_run_command):
                    if use_cc:
                        mock_run_command.side_effect = ('', MOCK_COMPILER)
                    else:
                        mock_run_command.return_value = MOCK_COMPILER
                    compiler = generate_getbuildinfo._get_compiler(GETCOMPILER)
                    if use_cc and support.MS_WINDOWS:
                        self.assertIsNone(compiler)
                    else:
                        self.assertEqual(compiler, MOCK_COMPILER)

                    if hostrunner and not support.MS_WINDOWS:
                        cmd = [hostrunner, GETCOMPILER]
                    else:
                        cmd = [GETCOMPILER]
                    if use_cc and not support.MS_WINDOWS:
                        self.assertEqual(mock_run_command.call_args_list,
                            [
                                mock.call(cmd, check=False),
                                mock.call(['gcc', '-std=c11', '--version'], check=False),
                            ]
                        )
                    else:
                        mock_run_command.assert_called_once_with(cmd, check=False)

    def test_compact_compiler(self):
        get_compiler = generate_getbuildinfo.get_compiler
        compact_compiler = generate_getbuildinfo.compact_compiler

        def check(compiler, expected):
            self.assertEqual(compact_compiler(compiler), expected)

        def check_unchanged(compiler):
            self.assertEqual(compact_compiler(compiler), compiler)

        compiler = (
            'Android (13691557, +pgo, +bolt, +lto, +mlgo, based on r522817d) '
            'clang version 18.0.4 '
            '(https://android.googlesource.com/toolchain/llvm-project'
            ' d8003a456d14a3deb8054cdaa529ffbf02d9b262)'
        )
        check(compiler, 'Android Clang 18.0.4')

        compiler = (
            'Clang 18.0.4 '
            '(https://android.googlesource.com/toolchain/llvm-project'
            ' d8003a456d14a3deb8054cdaa529ffbf02d9b262)'
        )
        check(compiler, 'Clang 18.0.4')

        compiler = (
            'Clang 24.0.0git '
            '(https:/github.com/llvm/llvm-project'
            ' a06d9165905ce89b5ffef2bbb84c886d60a9b8bf)'
        )
        check(compiler, 'Clang 24.0.0git')

        check('Clang 21.0.0 (clang-2100.1.1.101)',
              'Clang 21.0.0')
        check('Apple clang version 21.0.0 (clang-2100.1.1.101)',
              'Apple Clang 21.0.0')
        check('Custom Vendor (something) Clang 22.1.0+dev',
              'Custom Vendor Clang 22.1.0+dev')

        check_unchanged('Clang 22.1.3 64 bit (AMD64) with MSC v.1951 CRT')
        check_unchanged('GCC 16.2.1 20260819 (Red Hat 16.2.1-2)')
        check_unchanged('GCC 15.2.0')
        check_unchanged('MSC v.1951 64 bit (AMD64)')
        check_unchanged('MSC v.1951 32 bit (Intel)')

        log = ("Truncate compiler string 'Clang 21.0.0 "
               "(clang-2100.1.1.101)' to 'Clang 21.0.0'")
        with self.check_log(log):
            compiler = get_compiler('Clang 21.0.0 (clang-2100.1.1.101)')
            self.assertEqual(compiler, '[Clang 21.0.0]')

    def test_get_c_compiler_version_windows(self):
        get_compiler_version = generate_getbuildinfo.get_c_compiler_version_windows
        C_COMPILER = 'CL.EXE'

        tests = []

        # Test a full output
        output = textwrap.dedent('''
            Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36260 for ARM64
            Copyright (C) Microsoft Corporation.  All rights reserved.

            usage: cl [ option... ] filename... [ /link linkoption... ]
        ''').strip()
        tests.append((output, 'MSC v.1951 64 bit (ARM64)'))

        output = 'Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36260 for x64'
        tests.append((output, 'MSC v.1951 64 bit (AMD64)'))

        output = 'Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36260 for x86'
        tests.append((output, 'MSC v.1951 32 bit (Intel)'))

        output = 'Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36260 for ARM'
        tests.append((output, 'MSC v.1951 32 bit (ARM)'))

        def check_version(output, expected):
            with mock.patch.object(generate_getbuildinfo,
                                   'run_command') as mock_run_command:
                mock_run_command.return_value = output
                version = get_compiler_version(C_COMPILER)
            self.assertEqual(version, expected)
            mock_run_command.assert_called_once_with([C_COMPILER],
                                                     check=False, stderr=True)

        for output, expected in tests:
            with self.subTest(output=output, expected=expected):
                check_version(output, expected)

        def check_error(output, errmsg):
            with mock.patch.object(generate_getbuildinfo,
                                   'run_command') as mock_run_command:
                mock_run_command.return_value = output
                with self.check_exit_fail(errmsg):
                    get_compiler_version(C_COMPILER)

        output = 'xxx'
        check_error(output, f'Unable to parse cl.exe version: {output!r}')

        platform = 'PLATFORM'
        output = ('Microsoft (R) C/C++ Optimizing Compiler '
                  f'Version 19.51.36260 for {platform}')
        check_error(output, f'Unknown cl.exe platform: {platform!r}')

    def test_functional(self):
        old_locale = locale.setlocale(locale.LC_ALL)
        self.addCleanup(locale.setlocale, locale.LC_ALL, old_locale)

        filename = 'output.txt'
        self.addCleanup(os.unlink, filename)

        argv = [
            'program',
            '--output', filename,
            # Inject a quote to check that it's escaped
            '--platform', 'SET"PLATFORM',
            '--git-version', 'VERSION',
            '--git-tag', 'TAG',
            '--git-branch', 'BRANCH',
            '--compiler', 'COMPILER',
            '--free-threading', '1',
        ]
        with support.swap_attr(sys, 'argv', argv):
            with os_helper.EnvironmentVarGuard() as env:
                env['SOURCE_DATE_EPOCH'] = str(1791419852)
                with self.check_log(f'{filename} updated'):
                    generate_getbuildinfo.main()

        with open(filename) as fp:
            output = fp.read()
        expected = textwrap.dedent(r'''
            // Header file auto-generated by Tools/build/generate_getbuildinfo.py

            #define PLATFORM "SET\"PLATFORM"
            #define COMPILER "[COMPILER]"
            #define GIT_VERSION "VERSION"
            #define GIT_IDENTIFIER "TAG"
            #define BUILD_INFO "TAG:VERSION, Oct  8 2026, 00:37:32"
            #define VERSION "3.16.0a0 free-threading build (TAG:VERSION, Oct  8 2026, 00:37:32) [COMPILER]"
        ''').lstrip()
        self.assertEqual(output, expected)


if __name__ == "__main__":
    unittest.main()
