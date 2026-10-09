"""Test Tools/build/generate_getbuildinfo.py."""

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
        getcompiler = os.path.join('Programs', '_getcompiler')
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
                    compiler = generate_getbuildinfo._get_compiler()
                    self.assertEqual(compiler, MOCK_COMPILER)
                    if hostrunner:
                        cmd = [hostrunner, getcompiler]
                    else:
                        cmd = [getcompiler]
                    if use_cc:
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

        with support.captured_stdout() as stdout:
            compiler = get_compiler('Clang 21.0.0 (clang-2100.1.1.101)')
            self.assertEqual(compiler, '[Clang 21.0.0]')
        self.assertEqual(stdout.getvalue(),
            f"{SCRIPT_NAME}: Truncate compiler string 'Clang 21.0.0 "
            "(clang-2100.1.1.101)' to 'Clang 21.0.0'\n")

    def test_functional(self):
        old_locale = locale.setlocale(locale.LC_ALL)
        self.addCleanup(locale.setlocale, locale.LC_ALL, old_locale)

        filename = 'output.txt'
        self.addCleanup(os.unlink, filename)

        argv = [
            'program',
            '--output', filename,
            '--platform', 'SET_PLATFORM',
            '--git-version', 'VERSION',
            '--git-tag', 'TAG',
            '--git-branch', 'BRANCH',
            '--compiler', 'COMPILER',
            '--free-threading', '1',
        ]
        with support.swap_attr(sys, 'argv', argv):
            with os_helper.EnvironmentVarGuard() as env:
                env['SOURCE_DATE_EPOCH'] = str(1791419852)
                with support.captured_stdout() as stdout:
                    generate_getbuildinfo.main()
                self.assertEqual(stdout.getvalue(), f'{SCRIPT_NAME}: {filename} updated\n')

        with open(filename) as fp:
            output = fp.read()
        expected = textwrap.dedent('''
            // Header file auto-generated by Tools/build/generate_getbuildinfo.py

            #define PLATFORM "SET_PLATFORM"
            #define COMPILER "[COMPILER]"
            #define GIT_VERSION "VERSION"
            #define GIT_IDENTIFIER "TAG"
            #define BUILD_INFO "TAG:VERSION, Oct  8 2026, 00:37:32"
            #define VERSION "3.16.0a0 free-threading build (TAG:VERSION, Oct  8 2026, 00:37:32) [COMPILER]"
        ''').lstrip()
        self.assertEqual(output, expected)


if __name__ == "__main__":
    unittest.main()
