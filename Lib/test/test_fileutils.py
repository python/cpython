import os
import os.path
import unittest
import tempfile
import shutil
from test.support import import_helper, os_helper

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
    """Tests for _Py_wrealpath used by os.path.realpath."""

    def setUp(self):
        self.base = None

    def tearDown(self):
        if self.base and os.path.exists(self.base):
            shutil.rmtree(self.base, ignore_errors=True)

    def test_realpath_long_path(self):
        """Test that realpath handles paths longer than MAXPATHLEN."""
        if os.name == 'nt':
            self.skipTest('POSIX-specific test')

        self.base = tempfile.mkdtemp()
        current_path = self.base

        for i in range(25):
            dirname = f"d{i:03d}_" + "x" * 195
            current_path = os.path.join(current_path, dirname)
            try:
                os.mkdir(current_path)
            except OSError as e:
                self.skipTest(f"Cannot create long paths on this platform: {e}")

        full_path = os.path.abspath(current_path)
        if len(full_path) <= 4096:
            self.skipTest(f"Path not long enough ({len(full_path)} bytes)")

        result = os.path.realpath(full_path)
        self.assertTrue(os.path.isabs(result))
        self.assertGreater(len(result), 4096)

    def test_realpath_nonexistent_with_strict(self):
        """Test that realpath with strict=True raises for nonexistent paths."""
        if os.name == 'nt':
            self.skipTest('POSIX-specific test')

        self.base = tempfile.mkdtemp()
        nonexistent = os.path.join(self.base, "does_not_exist", "subdir")

        result = os.path.realpath(nonexistent, strict=False)
        self.assertIsNotNone(result)

        with self.assertRaises(OSError):
            os.path.realpath(nonexistent, strict=True)

    @os_helper.skip_unless_symlink
    def test_realpath_symlink_long_path(self):
        """Test realpath with symlinks in long paths."""
        if os.name == 'nt':
            self.skipTest('POSIX-specific test')

        self.base = tempfile.mkdtemp()
        current_path = self.base

        for i in range(15):
            dirname = f"d{i:03d}_" + "x" * 195
            current_path = os.path.join(current_path, dirname)
            try:
                os.mkdir(current_path)
            except OSError as e:
                self.skipTest(f"Cannot create long paths on this platform: {e}")

        symlink = os.path.join(self.base, "link")
        try:
            os.symlink(current_path, symlink)
        except (OSError, NotImplementedError) as e:
            self.skipTest(f"Cannot create symlinks on this platform: {e}")

        result = os.path.realpath(symlink)
        self.assertEqual(os.path.normpath(result), os.path.normpath(current_path))
        self.assertGreater(len(result), 1500)


if __name__ == "__main__":
    unittest.main()
