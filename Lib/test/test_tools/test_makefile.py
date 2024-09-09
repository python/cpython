"""
Tests for `Makefile`.
"""

import os
import unittest
from test import support
import sysconfig

MAKEFILE = sysconfig.get_makefile_filename()

if not support.check_impl_detail(cpython=True):
    raise unittest.SkipTest('cpython only')
if not os.path.exists(MAKEFILE) or not os.path.isfile(MAKEFILE):
    raise unittest.SkipTest('Makefile could not be found')


class TestMakefile(unittest.TestCase):
    def list_test_dirs(self):
        result = []
        found_testsubdirs = False
        with open(MAKEFILE, 'r', encoding='utf-8') as f:
            for line in f:
                if line.startswith('TESTSUBDIRS='):
                    found_testsubdirs = True
                    result.append(
                        line.removeprefix('TESTSUBDIRS=').replace(
                            '\\', '',
                        ).strip(),
                    )
                    continue
                if found_testsubdirs:
                    if '\t' not in line:
                        break
                    result.append(line.replace('\\', '').strip())
        return result

    @unittest.skipUnless(support.TEST_MODULES_ENABLED, "requires test modules")
    def test_makefile_test_folders(self):
        test_dirs = self.list_test_dirs()
        idle_test = 'idlelib/idle_test'
        self.assertIn(idle_test, test_dirs)

        used = set([idle_test])
        for dirpath, dirs, files in os.walk(support.TEST_HOME_DIR):
            dirname = os.path.basename(dirpath)
            # Skip temporary dirs:
            if dirname == '__pycache__' or dirname.startswith('.'):
                dirs.clear()  # do not process subfolders
                continue
            # Skip empty dirs:
            if not dirs and not files:
                continue
            # Skip dirs with hidden-only files:
            if files and all(filename.startswith('.') for filename in files):
                continue

            relpath = os.path.relpath(dirpath, support.STDLIB_DIR)
            with self.subTest(relpath=relpath):
                self.assertIn(
                    relpath,
                    test_dirs,
                    msg=(
                        f"{relpath!r} is not included in the Makefile's list "
                        "of test directories to install"
                    )
                )
                used.add(relpath)

        # Don't check the wheel dir when Python is built --with-wheel-pkg-dir
        if sysconfig.get_config_var('WHEEL_PKG_DIR'):
            test_dirs.remove('test/wheeldata')
            used.discard('test/wheeldata')

        # Check that there are no extra entries:
        unique_test_dirs = set(test_dirs)
        self.assertSetEqual(unique_test_dirs, used)
        self.assertEqual(len(test_dirs), len(unique_test_dirs))
