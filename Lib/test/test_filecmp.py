import filecmp
import os
import re
import shutil
import tempfile
import unittest
import stat
import io

from test import support
from test.support import os_helper
from unittest.mock import patch


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
        self.assertTrue(len(filecmp._cache) == 0,
                        "Cache not cleared after calling clear_cache")

    def test_cache_overflow_and_hit(self):
        filecmp.clear_cache()
        s1 = filecmp._sig(os.stat(self.name))
        s2 = filecmp._sig(os.stat(self.name_same))
        for i in range(105):
            fake_path2 = f"fake_path_{i}"
            filecmp._cache[self.name, fake_path2, s1, s2] = True

        self.assertGreater(len(filecmp._cache), 100)
        res1 = filecmp.cmp(self.name, self.name_same, shallow=False)
        self.assertTrue(res1)
        self.assertEqual(len(filecmp._cache), 1)

        res2 = filecmp.cmp(self.name, self.name_same, shallow=False)
        self.assertTrue(res2)

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
        """Assert that two lists are equal, up to ordering."""
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

    def test_dircmp_no_shallow_different_file(self):
        # A non shallow different file2
        d = filecmp.dircmp(self.dir, self.dir_same_shallow, shallow=False)
        self.assertEqual(d.same_files, [])
        self.assertEqual(d.diff_files, ['file'])
        expected_report = [
            "diff {} {}".format(self.dir, self.dir_same_shallow),
            "Differing files : ['file']",
            "Common subdirectories : ['subdir']",
        ]
        self._assert_report(d.report, expected_report)

    def test_dircmp_shallow_same_file(self):
        # A non shallow different file2
        d = filecmp.dircmp(self.dir, self.dir_same_shallow)
        self.assertEqual(d.same_files, ['file'])
        self.assertEqual(d.diff_files, [])
        expected_report = [
            "diff {} {}".format(self.dir, self.dir_same_shallow),
            "Identical files : ['file']",
            "Common subdirectories : ['subdir']",
        ]
        self._assert_report(d.report, expected_report)

    def test_dircmp_shallow_is_keyword_only(self):
        with self.assertRaisesRegex(
            TypeError,
            re.escape("dircmp.__init__() takes from 3 to 5 positional arguments but 6 were given"),
        ):
            filecmp.dircmp(self.dir, self.dir_same, None, None, True)
        self.assertIsInstance(
            filecmp.dircmp(self.dir, self.dir_same, None, None, shallow=True),
            filecmp.dircmp,
        )

    def test_dircmp_subdirs_type(self):
        """Check that dircmp.subdirs respects subclassing."""
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


class DirCmpImplementationTests(unittest.TestCase):
    def setUp(self):
        tmpdir = tempfile.gettempdir()
        self.base_test_dir = os.path.join(tmpdir, 'test_filecmp_working_dir')
        os.makedirs(self.base_test_dir, exist_ok=True)
        self.addCleanup(shutil.rmtree, self.base_test_dir, ignore_errors=True)

        self.dir = os.path.join(self.base_test_dir, 'dir')
        self.dir_same = os.path.join(self.base_test_dir, 'dir-same')
        self.dir_diff = os.path.join(self.base_test_dir, 'dir-diff')
        self.dir_diff_file = os.path.join(self.base_test_dir, 'dir-diff-file')
        self.dir_same_shallow = os.path.join(
            self.base_test_dir, 'dir-same-shallow'
        )

        self.dir_ignored = os.path.join(self.dir_same, '.hg')
        self.caseinsensitive = os.path.normcase('A') == os.path.normcase('a')
        data = 'Contents of file go here.\n'

        os.mkdir(self.dir)
        subdir_path = os.path.join(self.dir, 'subdir')
        os.mkdir(subdir_path)
        dir_file_path = os.path.join(self.dir, "file")
        with open(dir_file_path, 'w', encoding="utf-8") as output:
            output.write(data)

        for d in (self.dir_same, self.dir_same_shallow,
                self.dir_diff, self.dir_diff_file):
            os.mkdir(d)
            subdir_path = os.path.join(d, 'subdir')
            os.mkdir(subdir_path)
            fn = ('FiLe' if (self.caseinsensitive and d is self.dir_same)
                else 'file')
            file_path = os.path.join(d, fn)

            if d is self.dir_same_shallow:
                _create_file_shallow_equal(dir_file_path, file_path)
            else:
                shutil.copy2(dir_file_path, file_path)

        with open(os.path.join(self.dir_diff, 'file2'), 'w',
                    encoding="utf-8") as output:
            output.write('An extra file.\n')

        with open(os.path.join(self.dir_diff_file, 'file2'), 'w',
                    encoding="utf-8") as output:
            output.write('Different contents.\n')

    def test_dircmp_os_errors_in_processing(self):
        trigger_file = "error_trigger_file.txt"
        for d in (self.dir, self.dir_same):
            with open(os.path.join(d, trigger_file), "w",
                        encoding="utf-8") as f:
                f.write("trigger")

        original_stat = os.stat

        def fake_stat(path, *args, **kwargs):
            if trigger_file in str(path):
                raise OSError
            return original_stat(path, *args, **kwargs)

        with patch("os.stat", side_effect=fake_stat):
            dc = filecmp.dircmp(self.dir, self.dir_same)
            self.assertIn(trigger_file, dc.common_funny)

    def test_dircmp_different_types_and_funny_types(self):
        file_diff_type = "diff_type.txt"
        file_funny_type = "funny_type.txt"

        for f_name in (file_diff_type, file_funny_type):
            for d in (self.dir, self.dir_same):
                with open(os.path.join(d, f_name), "w",
                            encoding="utf-8") as f:
                    f.write("data")

        original_stat = os.stat

        def fake_stat_types(path, *args, **kwargs):
            st = original_stat(path, *args, **kwargs)
            path_str = str(path)

            if file_diff_type in path_str:
                is_dir_same = "dir-same" in path_str
                target_type = stat.S_IFREG if is_dir_same else stat.S_IFDIR
                new_mode = (st.st_mode & ~stat.S_IFREG) | target_type
                return os.stat_result((new_mode,) + st[1:])

            if file_funny_type in path_str:
                new_mode = (st.st_mode & ~stat.S_IFREG) | stat.S_IFSOCK
                return os.stat_result((new_mode,) + st[1:])

            return st

        with patch("os.stat", side_effect=fake_stat_types):
            dc = filecmp.dircmp(self.dir, self.dir_same)
            self.assertIn(file_diff_type, dc.common_funny)
            self.assertIn(file_funny_type, dc.common_funny)

    def test_dircmp_closure_and_report_funnies(self):
        deep_dir_1 = os.path.join(self.dir, "subdir", "deep_folder")
        deep_dir_2 = os.path.join(self.dir_same, "subdir", "deep_folder")
        os.mkdir(deep_dir_1)
        os.mkdir(deep_dir_2)

        funny_trigger = "funny_report_file.txt"
        funny_file_trigger = "funny_file_trigger.txt"

        for f_name in (funny_trigger, funny_file_trigger):
            for d in (self.dir, self.dir_same):
                with open(os.path.join(d, f_name), "w",
                            encoding="utf-8") as f:
                    f.write("data")

        original_stat = os.stat
        original_cmp = filecmp._cmp

        def fake_stat_for_report(path, *args, **kwargs):
            if funny_trigger in str(path):
                st = original_stat(path, *args, **kwargs)
                new_mode = (st.st_mode & ~stat.S_IFREG) | stat.S_IFSOCK
                return os.stat_result((new_mode,) + st[1:])
            return original_stat(path, *args, **kwargs)

        def fake_private_cmp(a, b, sh):
            if funny_file_trigger in str(a) or funny_file_trigger in str(b):
                return 2
            return original_cmp(a, b, sh)

        with patch("os.stat", side_effect=fake_stat_for_report), \
            patch("filecmp._cmp", side_effect=fake_private_cmp):

            dc = filecmp.dircmp(self.dir, self.dir_same)
            _ = dc.subdirs
            dc.phase4_closure()

            with patch("sys.stdout", new=io.StringIO()) as mock_stdout:
                dc.report()
                output = mock_stdout.getvalue()

                self.assertIn("Common funny cases", output)
                self.assertIn("Trouble with common files", output)


if __name__ == "__main__":
    unittest.main()
