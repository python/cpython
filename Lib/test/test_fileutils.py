# Run tests for functions in Python/fileutils.c.

import os
import os.path
import unittest
import tempfile
import shutil
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testinternalcapi')


class PathTests(unittest.TestCase):

    def test_capi_normalize_path(self):
        if os.name == 'nt':
            raise unittest.SkipTest('Windows has its own helper for this')
        else:
            from test.test_posixpath import PosixPathTest as posixdata
            tests = posixdata.NORMPATH_CASES
        for filename, expected in tests:
            if not os.path.isabs(filename):
                continue
            with self.subTest(filename):
                result = _testcapi.normalize_path(filename)
                self.assertEqual(result, expected,
                    msg=f'input: {filename!r} expected output: {expected!r}')


class RealpathTests(unittest.TestCase):
    """Tests for _Py_wrealpath used by os.path.realpath"""

    def test_realpath_long_path(self):
        """Test that realpath handles paths longer than MAXPATHLEN (4096)"""
        if os.name == 'nt':
            raise unittest.SkipTest('POSIX-specific test')

        base = tempfile.mkdtemp()
        original_cwd = os.getcwd()
        try:
            os.chdir(base)

            for i in range(85):
                dirname = f"d{i:03d}_" + "x" * 44
                os.mkdir(dirname)
                os.chdir(dirname)

            full_path = os.getcwd()

            self.assertGreater(len(full_path), 4096,
                              f"Path should exceed MAXPATHLEN, got {len(full_path)}")

            # Main test: realpath should not crash on long paths
            result = os.path.realpath(full_path)

            self.assertTrue(os.path.isabs(result))
            self.assertGreater(len(result), 4096)
            # Note: os.path.exists() may fail on very long paths
            # The important thing is realpath() doesn't crash

        finally:
            os.chdir(original_cwd)
            shutil.rmtree(base, ignore_errors=True)

    def test_realpath_nonexistent_with_strict(self):
        """Test that realpath with strict=True raises for nonexistent paths"""
        if os.name == 'nt':
            raise unittest.SkipTest('POSIX-specific test')

        base = tempfile.mkdtemp()
        try:
            nonexistent = os.path.join(base, "does_not_exist", "subdir")

            # Without strict, should return the path
            result = os.path.realpath(nonexistent, strict=False)
            self.assertIsNotNone(result)

            # With strict=True, should raise an error
            with self.assertRaises(OSError):
                os.path.realpath(nonexistent, strict=True)

        finally:
            shutil.rmtree(base, ignore_errors=True)

    def test_realpath_symlink_long_path(self):
        """Test realpath with symlinks in long paths"""
        if os.name == 'nt':
            raise unittest.SkipTest('POSIX-specific test')

        base = tempfile.mkdtemp()
        try:
            # Create a long path
            current = base
            for i in range(30):
                dirname = f"d{i:03d}_" + "x" * 44
                current = os.path.join(current, dirname)
                os.mkdir(current)

            # Create a symlink pointing to the long path
            symlink = os.path.join(base, "link")
            os.symlink(current, symlink)

            # Resolve the symlink
            result = os.path.realpath(symlink)

            self.assertEqual(os.path.normpath(result), os.path.normpath(current))
            self.assertGreater(len(result), 1500)

        finally:
            shutil.rmtree(base, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
