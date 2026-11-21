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

            # Skip empty dirs (ignoring hidden files and __pycache__):
            files = [
                filename for filename in files
                if not filename.startswith('.')
            ]
            dirs = [
                dirname for dirname in dirs
                if not dirname.startswith('.') and dirname != "__pycache__"
            ]
            if not dirs and not files:
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

    def test_makefile_module_check_command(self):
        vars = {}
        with open(MAKEFILE, 'r', encoding='utf-8') as f:
            for line in f:
                if '=' in line and not line.startswith(('\t', ' ', '#')):
                    key, _, val = line.partition('=')
                    vars[key.strip()] = val.strip()

        self.assertIn('MODULE_CHECK_COMMAND', vars)
        cmd = vars['MODULE_CHECK_COMMAND']
        tokens = cmd.split()
        if vars.get('_PYTHON_HOST_PLATFORM'):
            self.assertIn('--cross-compiling', tokens)
            self.assertNotIn('--no-cross-compiling', tokens)
            self.assertIn('PYTHON_FOR_BUILD', cmd)
        else:
            self.assertIn('--no-cross-compiling', tokens)
            self.assertNotIn('--cross-compiling', tokens)
            self.assertIn('BUILDPYTHON', cmd)

    def test_makefile_checksharedmods_recipe(self):
        """Ensure checksharedmods recipe uses $(RUNSHARED_FOR_BUILD) $(MODULE_CHECK_COMMAND)."""
        found_target = False
        found_recipe = False
        with open(MAKEFILE, 'r', encoding='utf-8') as f:
            for line in f:
                if line.startswith('checksharedmods:'):
                    found_target = True
                elif found_target:
                    if line.startswith(('\t', ' ')):
                        if 'MODULE_CHECK_COMMAND' in line:
                            found_recipe = True
                            self.assertIn('$(RUNSHARED_FOR_BUILD) $(MODULE_CHECK_COMMAND)', line)
                            self.assertNotIn('$(RUNSHARED) $(MODULE_CHECK_COMMAND)', line)
                    else:
                        break
        self.assertTrue(found_target, "checksharedmods target not found in Makefile")
        self.assertTrue(found_recipe, "MODULE_CHECK_COMMAND not invoked in checksharedmods recipe")
