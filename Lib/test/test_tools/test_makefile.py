"""
Tests for `Makefile`.
"""

import os
import unittest
from test import support

SPLITTER = '@-----@'
MAKEFILE = os.path.join(support.REPO_ROOT, 'Makefile')

if not support.check_impl_detail(cpython=True):
    raise unittest.SkipTest('cpython only')
if support.MS_WINDOWS:
    raise unittest.SkipTest('Windows does not support make')
if not os.path.exists(MAKEFILE) or not os.path.isfile(MAKEFILE):
    raise unittest.SkipTest('Makefile could not be found')

try:
    import _testcapi
except ImportError:
    TEST_MODULES = False
else:
    TEST_MODULES = True


@support.requires_subprocess()
class TestMakefile(unittest.TestCase):
    def list_test_dirs(self):
        import subprocess
        return subprocess.check_output(
            ['make', '-s', '-C', support.REPO_ROOT, 'listtestmodules'],
            universal_newlines=True,
            encoding='utf-8',
        )

    def check_existing_test_modules(self, result):
        test_dirs = result.split(' ')
        idle_test = 'idlelib/idle_test'
        self.assertIn(idle_test, test_dirs)

        used = [idle_test]
        for dirpath, _, _ in os.walk(support.TEST_HOME_DIR):
            dirname = os.path.basename(dirpath)
            if dirname == '__pycache__':
                continue
            with self.subTest(dirpath=dirpath):
                relpath = os.path.relpath(dirpath, support.STDLIB_DIR)
                self.assertIn(
                    relpath,
                    test_dirs,
                    msg=(
                        f"{relpath!r} is not included in the Makefile's list "
                        "of test directories to install"
                    )
                )
                used.append(relpath)

        # Check that there are no extra entries:
        unique_test_dirs = set(test_dirs)
        self.assertEqual(
            unique_test_dirs.symmetric_difference(set(used)),
            set(),
        )
        self.assertEqual(len(test_dirs), len(unique_test_dirs))

    def check_missing_test_modules(self, result):
        self.assertEqual(result, '')

    def test_makefile_test_folders(self):
        result = self.list_test_dirs().strip()
        self.assertEqual(
            result.count(SPLITTER), 1,
            msg=f'{SPLITTER} should be contained in the output exactly once',
        )
        _, actual_result = result.split(SPLITTER)
        actual_result = actual_result.strip()
        if TEST_MODULES:
            self.check_existing_test_modules(actual_result)
        else:
            self.check_missing_test_modules(actual_result)
