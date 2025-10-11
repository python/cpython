import contextlib
import filecmp
import getopt
import os
import re
import shutil
import tempfile
import unittest

from test import support
from test.support import os_helper


def _create_file_shallow_equal(template_path, new_path):
    """create a file with the same size and mtime but different content."""
    shutil.copy2(template_path, new_path)
    with open(new_path, 'r+b') as f:
        next_char = bytearray(f.read(1))
        next_char[0] = (next_char[0] + 1) % 256
        f.seek(0)
        f.write(next_char)
    shutil.copystat(template_path, new_path)
    assert os.stat(new_path).st_size == os.stat(template_path).st_size
    assert os.stat(new_path).st_mtime == os.stat(template_path).st_mtime


class FileCompareTestCase(unittest.TestCase):

    def setUp(self):
        self.name = os_helper.TESTFN
        self.name_same = os_helper.TESTFN + '-same'
        self.name_diff = os_helper.TESTFN + '-diff'
        self.name_same_shallow = os_helper.TESTFN + '-same-shallow'
        data = 'Contents of file go here.\n'
        for name in [self.name, self.name_same, self.name_diff]:
            with open(name, 'w', encoding="utf-8") as output:
                output.write(data)

        with open(self.name_diff, 'a+', encoding="utf-8") as output:
            output.write('An extra line.\n')

        for name in [self.name_same, self.name_diff]:
            shutil.copystat(self.name, name)

        _create_file_shallow_equal(self.name, self.name_same_shallow)

        self.dir = tempfile.gettempdir()

    def tearDown(self):
        os.unlink(self.name)
        os.unlink(self.name_same)
        os.unlink(self.name_diff)
        os.unlink(self.name_same_shallow)

    def test_matching(self):
        self.assertTrue(filecmp.cmp(self.name, self.name),
                        "Comparing file to itself fails")
        self.assertTrue(filecmp.cmp(self.name, self.name, shallow=False),
                        "Comparing file to itself fails")
        self.assertTrue(filecmp.cmp(self.name, self.name_same),
                        "Comparing file to identical file fails")
        self.assertTrue(filecmp.cmp(self.name, self.name_same, shallow=False),
                        "Comparing file to identical file fails")
        self.assertTrue(filecmp.cmp(self.name, self.name_same_shallow),
                        "Shallow identical files should be considered equal")

    def test_different(self):
        self.assertFalse(filecmp.cmp(self.name, self.name_diff),
                    "Mismatched files compare as equal")
        self.assertFalse(filecmp.cmp(self.name, self.dir),
                    "File and directory compare as equal")
        self.assertFalse(filecmp.cmp(self.name, self.name_same_shallow,
                                     shallow=False),
                        "Mismatched file to shallow identical file compares as equal")

    def test_cache_clear(self):
        first_compare = filecmp.cmp(self.name, self.name_same, shallow=False)
        second_compare = filecmp.cmp(self.name, self.name_diff, shallow=False)
        filecmp.clear_cache()
        # Cache is internal, just verify the function works without error
        self.assertTrue(isinstance(first_compare, bool))
        self.assertTrue(isinstance(second_compare, bool))


class DirCompareTestCase(unittest.TestCase):
    def setUp(self):
        tmpdir = tempfile.gettempdir()
        self.dir = os.path.join(tmpdir, 'dir')
        self.dir_same = os.path.join(tmpdir, 'dir-same')
        self.dir_diff = os.path.join(tmpdir, 'dir-diff')
        self.dir_diff_file = os.path.join(tmpdir, 'dir-diff-file')
        self.dir_same_shallow = os.path.join(tmpdir, 'dir-same-shallow')

        # Another dir is created under dir_same, but it has a name from the
        # ignored list so it should not affect testing results.
        self.dir_ignored = os.path.join(self.dir_same, '.hg')

        self.caseinsensitive = os.path.normcase('A') == os.path.normcase('a')
        data = 'Contents of file go here.\n'

        shutil.rmtree(self.dir, True)
        os.mkdir(self.dir)
        subdir_path = os.path.join(self.dir, 'subdir')
        os.mkdir(subdir_path)
        dir_file_path = os.path.join(self.dir, "file")
        with open(dir_file_path, 'w', encoding="utf-8") as output:
            output.write(data)

        for dir in (self.dir_same, self.dir_same_shallow,
                    self.dir_diff, self.dir_diff_file):
            shutil.rmtree(dir, True)
            os.mkdir(dir)
            subdir_path = os.path.join(dir, 'subdir')
            os.mkdir(subdir_path)
            if self.caseinsensitive and dir is self.dir_same:
                fn = 'FiLe'     # Verify case-insensitive comparison
            else:
                fn = 'file'

            file_path = os.path.join(dir, fn)

            if dir is self.dir_same_shallow:
                _create_file_shallow_equal(dir_file_path, file_path)
            else:
                shutil.copy2(dir_file_path, file_path)

        with open(os.path.join(self.dir_diff, 'file2'), 'w', encoding="utf-8") as output:
            output.write('An extra file.\n')

        # Add different file2 with respect to dir_diff
        with open(os.path.join(self.dir_diff_file, 'file2'), 'w', encoding="utf-8") as output:
            output.write('Different contents.\n')


    def tearDown(self):
        for dir in (self.dir, self.dir_same, self.dir_diff,
                    self.dir_same_shallow, self.dir_diff_file):
            shutil.rmtree(dir)

    def test_default_ignores(self):
        self.assertIn('.hg', filecmp.DEFAULT_IGNORES)

    def test_cmpfiles(self):
        self.assertTrue(filecmp.cmpfiles(self.dir, self.dir, ['file']) ==
                        (['file'], [], []),
                        "Comparing directory to itself fails")
        self.assertTrue(filecmp.cmpfiles(self.dir, self.dir_same, ['file']) ==
                        (['file'], [], []),
                        "Comparing directory to same fails")

        # Try it with shallow=False
        self.assertTrue(filecmp.cmpfiles(self.dir, self.dir, ['file'],
                                         shallow=False) ==
                        (['file'], [], []),
                        "Comparing directory to itself fails")
        self.assertTrue(filecmp.cmpfiles(self.dir, self.dir_same, ['file'],
                                         shallow=False),
                        "Comparing directory to same fails")

        self.assertFalse(filecmp.cmpfiles(self.dir, self.dir_diff_file,
                                     ['file', 'file2']) ==
                    (['file'], ['file2'], []),
                    "Comparing mismatched directories fails")

    def test_cmpfiles_invalid_names(self):
        # See https://github.com/python/cpython/issues/122400.
        for file, desc in [
            ('\x00', 'NUL bytes filename'),
            (__file__ + '\x00', 'filename with embedded NUL bytes'),
            ("\uD834\uDD1E.py", 'surrogate codes (MUSICAL SYMBOL G CLEF)'),
            ('a' * 1_000_000, 'very long filename'),
        ]:
            for other_dir in [self.dir, self.dir_same, self.dir_diff]:
                with self.subTest(f'cmpfiles: {desc}', other_dir=other_dir):
                    res = filecmp.cmpfiles(self.dir, other_dir, [file])
                    self.assertTupleEqual(res, ([], [], [file]))

    def test_dircmp_invalid_names(self):
        for bad_dir, desc in [
            ('\x00', 'NUL bytes dirname'),
            (f'Top{os.sep}Mid\x00', 'dirname with embedded NUL bytes'),
            ("\uD834\uDD1E", 'surrogate codes (MUSICAL SYMBOL G CLEF)'),
            ('a' * 1_000_000, 'very long dirname'),
        ]:
            d1 = filecmp.dircmp(self.dir, bad_dir)
            d2 = filecmp.dircmp(bad_dir, self.dir)
            for target in [
                # attributes where os.listdir() raises OSError or ValueError
                'left_list', 'right_list',
                'left_only', 'right_only', 'common',
            ]:
                with self.subTest(f'dircmp(ok, bad): {desc}', target=target):
                    with self.assertRaises((OSError, ValueError)):
                        getattr(d1, target)
                with self.subTest(f'dircmp(bad, ok): {desc}', target=target):
                    with self.assertRaises((OSError, ValueError)):
                        getattr(d2, target)

    def _assert_lists(self, actual, expected):
        self.assertEqual(sorted(actual), sorted(expected))

    def test_dircmp_identical_directories(self):
        self._assert_dircmp_identical_directories()
        self._assert_dircmp_identical_directories(shallow=False)

    def test_dircmp_different_file(self):
        self._assert_dircmp_different_file()
        self._assert_dircmp_different_file(shallow=False)

    def test_dircmp_different_directories(self):
        self._assert_dircmp_different_directories()
        self._assert_dircmp_different_directories(shallow=False)

    def _assert_dircmp_identical_directories(self, **options):
        # Check attributes for comparison of two identical directories
        left_dir, right_dir = self.dir, self.dir_same
        d = filecmp.dircmp(left_dir, right_dir, **options)
        self.assertEqual(d.left, left_dir)
        self.assertEqual(d.right, right_dir)
        if self.caseinsensitive:
            self._assert_lists(d.left_list, ['file', 'subdir'])
            self._assert_lists(d.right_list, ['FiLe', 'subdir'])
        else:
            self._assert_lists(d.left_list, ['file', 'subdir'])
            self._assert_lists(d.right_list, ['file', 'subdir'])
        self._assert_lists(d.common, ['file', 'subdir'])
        self._assert_lists(d.common_dirs, ['subdir'])
        self.assertEqual(d.left_only, [])
        self.assertEqual(d.right_only, [])
        self.assertEqual(d.same_files, ['file'])
        self.assertEqual(d.diff_files, [])
        expected_report = [
            "diff {} {}".format(self.dir, self.dir_same),
            "Identical files : ['file']",
            "Common subdirectories : ['subdir']",
        ]
        self._assert_report(d.report, expected_report)

    def _assert_dircmp_different_directories(self, **options):
        # Check attributes for comparison of two different directories (right)
        left_dir, right_dir = self.dir, self.dir_diff
        d = filecmp.dircmp(left_dir, right_dir, **options)
        self.assertEqual(d.left, left_dir)
        self.assertEqual(d.right, right_dir)
        self._assert_lists(d.left_list, ['file', 'subdir'])
        self._assert_lists(d.right_list, ['file', 'file2', 'subdir'])
        self._assert_lists(d.common, ['file', 'subdir'])
        self._assert_lists(d.common_dirs, ['subdir'])
        self.assertEqual(d.left_only, [])
        self.assertEqual(d.right_only, ['file2'])
        self.assertEqual(d.same_files, ['file'])
        self.assertEqual(d.diff_files, [])
        expected_report = [
            "diff {} {}".format(self.dir, self.dir_diff),
            "Only in {} : ['file2']".format(self.dir_diff),
            "Identical files : ['file']",
            "Common subdirectories : ['subdir']",
        ]
        self._assert_report(d.report, expected_report)

        # Check attributes for comparison of two different directories (left)
        left_dir, right_dir = self.dir_diff, self.dir
        d = filecmp.dircmp(left_dir, right_dir, **options)
        self.assertEqual(d.left, left_dir)
        self.assertEqual(d.right, right_dir)
        self._assert_lists(d.left_list, ['file', 'file2', 'subdir'])
        self._assert_lists(d.right_list, ['file', 'subdir'])
        self._assert_lists(d.common, ['file', 'subdir'])
        self.assertEqual(d.left_only, ['file2'])
        self.assertEqual(d.right_only, [])
        self.assertEqual(d.same_files, ['file'])
        self.assertEqual(d.diff_files, [])
        expected_report = [
            "diff {} {}".format(self.dir_diff, self.dir),
            "Only in {} : ['file2']".format(self.dir_diff),
            "Identical files : ['file']",
            "Common subdirectories : ['subdir']",
        ]
        self._assert_report(d.report, expected_report)


    def _assert_dircmp_different_file(self, **options):
        # A different file2
        d = filecmp.dircmp(self.dir_diff, self.dir_diff_file, **options)
        self.assertEqual(d.same_files, ['file'])
        self.assertEqual(d.diff_files, ['file2'])
        expected_report = [
            "diff {} {}".format(self.dir_diff, self.dir_diff_file),
            "Identical files : ['file']",
            "Differing files : ['file2']",
            "Common subdirectories : ['subdir']",
        ]
        self._assert_report(d.report, expected_report)

    def test_dircmp_shallow_same_file(self):
        # A shallow identical file
        d = filecmp.dircmp(self.dir, self.dir_same_shallow)
        self.assertEqual(d.same_files, ['file'])
        self.assertEqual(d.diff_files, [])
        expected_report = [
            "diff {} {}".format(self.dir, self.dir_same_shallow),
            "Identical files : ['file']",
            "Common subdirectories : ['subdir']",
        ]
        self._assert_report(d.report, expected_report)

    def test_dircmp_too_many_args(self):
        with self.assertRaisesRegex(
            TypeError,
            re.escape("__init__() takes from 3 to 5 positional arguments but 6 were given"),
        ):
            filecmp.dircmp(self.dir, self.dir_same, None, None, 'extra')

    def test_dircmp_subdirs_type(self):
        class MyDirCmp(filecmp.dircmp):
            pass
        d = MyDirCmp(self.dir, self.dir_diff)
        sub_dirs = d.subdirs
        self.assertEqual(list(sub_dirs.keys()), ['subdir'])
        sub_dcmp = sub_dirs['subdir']
        self.assertEqual(type(sub_dcmp), MyDirCmp)

    def test_report_partial_closure(self):
        left_dir, right_dir = self.dir, self.dir_same
        d = filecmp.dircmp(left_dir, right_dir)
        left_subdir = os.path.join(left_dir, 'subdir')
        right_subdir = os.path.join(right_dir, 'subdir')
        expected_report = [
            "diff {} {}".format(self.dir, self.dir_same),
            "Identical files : ['file']",
            "Common subdirectories : ['subdir']",
            '',
            "diff {} {}".format(left_subdir, right_subdir),
        ]
        self._assert_report(d.report_partial_closure, expected_report)

    def test_report_full_closure(self):
        left_dir, right_dir = self.dir, self.dir_same
        d = filecmp.dircmp(left_dir, right_dir)
        left_subdir = os.path.join(left_dir, 'subdir')
        right_subdir = os.path.join(right_dir, 'subdir')
        expected_report = [
            "diff {} {}".format(self.dir, self.dir_same),
            "Identical files : ['file']",
            "Common subdirectories : ['subdir']",
            '',
            "diff {} {}".format(left_subdir, right_subdir),
        ]
        self._assert_report(d.report_full_closure, expected_report)

    def _assert_report(self, dircmp_report, expected_report_lines):
        with support.captured_stdout() as stdout:
            dircmp_report()
            report_lines = stdout.getvalue().strip().split('\n')
            self.assertEqual(report_lines, expected_report_lines)


class TestFilecmpCLI(unittest.TestCase):
    """Test the command line interface of filecmp module"""

    @classmethod
    def setUpClass(cls):
        cls.temp_dir = tempfile.mkdtemp()
        cls.addClassCleanup(shutil.rmtree, cls.temp_dir)
        cls.dir1 = os.path.join(cls.temp_dir, 'dir1')
        cls.dir2 = os.path.join(cls.temp_dir, 'dir2')

        # Pre-compute common paths
        cls.dir1_same = os.path.join(cls.dir1, 'same.txt')
        cls.dir2_same = os.path.join(cls.dir2, 'same.txt')
        cls.dir1_different = os.path.join(cls.dir1, 'different.txt')
        cls.dir2_different = os.path.join(cls.dir2, 'different.txt')
        cls.dir1_only = os.path.join(cls.dir1, 'only_in_dir1.txt')
        cls.dir2_only = os.path.join(cls.dir2, 'only_in_dir2.txt')
        cls.dir1_subdir = os.path.join(cls.dir1, 'subdir')
        cls.dir2_subdir = os.path.join(cls.dir2, 'subdir')
        cls.dir1_subfile = os.path.join(cls.dir1_subdir, 'subfile.txt')
        cls.dir2_subfile = os.path.join(cls.dir2_subdir, 'subfile.txt')

        os.makedirs(cls.dir1)
        os.makedirs(cls.dir2)

    def setUp(self):
        # Clean up any files from previous tests
        for directory in (self.dir1, self.dir2):
            for item in os.listdir(directory):
                path = os.path.join(directory, item)
                if os.path.isfile(path):
                    os.unlink(path)
                elif os.path.isdir(path):
                    shutil.rmtree(path)

    def write_file(self, path, content):
        """Helper method to write content to a file."""
        with open(path, 'w') as f:
            f.write(content)

    @contextlib.contextmanager
    def subfile(self, subdir, filename, content):
        """Context manager for creating a temporary file in a subdirectory."""
        subdir_path = os.path.join(self.dir1, subdir)
        os.makedirs(subdir_path, exist_ok=True)
        file_path = os.path.join(subdir_path, filename)
        self.write_file(file_path, content)
        try:
            yield file_path
        finally:
            if os.path.exists(file_path):
                os.unlink(file_path)

    @contextlib.contextmanager
    def subdir(self, name):
        """Context manager for creating a temporary subdirectory."""
        subdir_path = os.path.join(self.dir1, name)
        os.makedirs(subdir_path, exist_ok=True)
        try:
            yield subdir_path
        finally:
            if os.path.exists(subdir_path):
                shutil.rmtree(subdir_path)

    def test_demo_basic_comparison(self):
        # Create test files
        self.write_file(self.dir1_same, 'same content')
        self.write_file(self.dir2_same, 'same content')
        self.write_file(self.dir1_different, 'content1')
        self.write_file(self.dir2_different, 'content2')
        self.write_file(self.dir1_only, 'only in dir1')
        self.write_file(self.dir2_only, 'only in dir2')

        with support.captured_stdout() as stdout:
            filecmp.__dict__['demo']([self.dir1, self.dir2])
            output = stdout.getvalue()

        # Check that output contains expected comparison results
        self.assertIn('diff', output)
        self.assertIn('same.txt', output)
        self.assertIn('different.txt', output)
        self.assertIn('only_in_dir1.txt', output)
        self.assertIn('only_in_dir2.txt', output)

    def test_demo_recursive_comparison(self):
        # Create test files including subdirectories
        self.write_file(self.dir1_same, 'same content')
        self.write_file(self.dir2_same, 'same content')
        os.makedirs(self.dir1_subdir, exist_ok=True)
        os.makedirs(self.dir2_subdir, exist_ok=True)
        self.write_file(self.dir1_subfile, 'subfile content')
        self.write_file(self.dir2_subfile, 'subfile content')

        with support.captured_stdout() as stdout:
            filecmp.__dict__['demo'](['-r', self.dir1, self.dir2])
            output = stdout.getvalue()

        # Check that output contains subdirectory comparison
        self.assertIn('subdir', output)
        self.assertIn('subfile.txt', output)

    def test_demo_long_flag(self):
        with self.assertRaises(getopt.GetoptError):
            filecmp.__dict__['demo'](['--recursive', self.dir1, self.dir2])

    def test_demo_no_arguments(self):
        with self.assertRaises(getopt.GetoptError) as cm:
            filecmp.__dict__['demo']([])
        self.assertIn('need exactly two args', str(cm.exception))

    def test_demo_one_argument(self):
        with self.assertRaises(getopt.GetoptError) as cm:
            filecmp.__dict__['demo']([self.dir1])
        self.assertIn('need exactly two args', str(cm.exception))

    def test_demo_three_arguments(self):
        with self.assertRaises(getopt.GetoptError) as cm:
            filecmp.__dict__['demo']([self.dir1, self.dir2, 'extra'])
        self.assertIn('need exactly two args', str(cm.exception))

    def test_demo_nonexistent_directory(self):
        with self.assertRaises((FileNotFoundError, OSError)):
            filecmp.__dict__['demo']([self.dir1, '/nonexistent/path'])

    def test_demo_identical_directories(self):
        dir3 = os.path.join(self.temp_dir, 'dir3')
        dir4 = os.path.join(self.temp_dir, 'dir4')
        self.addCleanup(shutil.rmtree, dir3, ignore_errors=True)
        self.addCleanup(shutil.rmtree, dir4, ignore_errors=True)
        os.makedirs(dir3)
        os.makedirs(dir4)

        self.write_file(os.path.join(dir3, 'file1.txt'), 'same content')
        self.write_file(os.path.join(dir4, 'file1.txt'), 'same content')

        with support.captured_stdout() as stdout:
            filecmp.__dict__['demo']([dir3, dir4])
            output = stdout.getvalue()

        # Should indicate identical files
        self.assertIn('Identical files', output)
        self.assertIn('file1.txt', output)

    def test_demo_empty_directories(self):
        dir3 = os.path.join(self.temp_dir, 'empty1')
        dir4 = os.path.join(self.temp_dir, 'empty2')
        self.addCleanup(shutil.rmtree, dir3, ignore_errors=True)
        self.addCleanup(shutil.rmtree, dir4, ignore_errors=True)
        os.makedirs(dir3)
        os.makedirs(dir4)

        with support.captured_stdout() as stdout:
            filecmp.__dict__['demo']([dir3, dir4])
            output = stdout.getvalue()

        # Should handle empty directories gracefully
        self.assertTrue(len(output) > 0)

    def test_demo_with_files_instead_of_directories(self):
        self.write_file(self.dir1_same, 'content')
        self.write_file(self.dir2_same, 'content')

        # This should raise an exception as files are not directories
        with self.assertRaises((NotADirectoryError, OSError)):
            filecmp.__dict__['demo']([self.dir1_same, self.dir2_same])


if __name__ == "__main__":
    unittest.main()
