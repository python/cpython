import os
import tempfile
import shutil
import unittest
import filecmp


class TestFileCmp(unittest.TestCase):

    def setUp(self):
        # 创建临时目录结构
        self.tempdir = tempfile.mkdtemp()

        self.dir1 = os.path.join(self.tempdir, "dir1")
        self.dir2 = os.path.join(self.tempdir, "dir2")

        os.makedirs(self.dir1)
        os.makedirs(self.dir2)

        # 文件内容
        self.file_same_1 = os.path.join(self.dir1, "same.txt")
        self.file_same_2 = os.path.join(self.dir2, "same.txt")

        self.file_diff_1 = os.path.join(self.dir1, "diff.txt")
        self.file_diff_2 = os.path.join(self.dir2, "diff.txt")

        # 创建相同文件
        with open(self.file_same_1, "w") as f:
            f.write("hello world")
        with open(self.file_same_2, "w") as f:
            f.write("hello world")

        # 创建不同文件
        with open(self.file_diff_1, "w") as f:
            f.write("foo")
        with open(self.file_diff_2, "w") as f:
            f.write("bar")

        # 创建仅在 dir1 存在的文件
        with open(os.path.join(self.dir1, "only_in_dir1.txt"), "w") as f:
            f.write("exclusive")

        # 创建子目录
        os.makedirs(os.path.join(self.dir1, "subdir"))
        os.makedirs(os.path.join(self.dir2, "subdir"))

        with open(os.path.join(self.dir1, "subdir", "nested.txt"), "w") as f:
            f.write("nested content")
        with open(os.path.join(self.dir2, "subdir", "nested.txt"), "w") as f:
            f.write("nested content")

    def tearDown(self):
        shutil.rmtree(self.tempdir)

    def test_cmp_same_files(self):
        self.assertTrue(filecmp.cmp(self.file_same_1, self.file_same_2, shallow=False))

    def test_cmp_different_files(self):
        self.assertFalse(filecmp.cmp(self.file_diff_1, self.file_diff_2, shallow=False))

    def test_cmp_shallow_true(self):
        # 修改时间不同但内容相同
        os.utime(self.file_same_1, (0, 0))
        os.utime(self.file_same_2, (10000, 10000))
        # 由于 shallow=True，只比较 stat 信息，可能返回 False
        result = filecmp.cmp(self.file_same_1, self.file_same_2, shallow=True)
        self.assertIsInstance(result, bool)

    def test_cmpfiles_function(self):
        common_files = ["same.txt", "diff.txt"]
        match, mismatch, errors = filecmp.cmpfiles(
            self.dir1, self.dir2, common_files, shallow=False
        )
        self.assertIn("same.txt", match)
        self.assertIn("diff.txt", mismatch)
        self.assertEqual(errors, [])

    def test_dircmp_basic(self):
        dcmp = filecmp.dircmp(self.dir1, self.dir2)
        self.assertIn("only_in_dir1.txt", dcmp.left_only)
        self.assertIn("same.txt", dcmp.common_files)
        self.assertIn("diff.txt", dcmp.diff_files)
        self.assertIn("subdir", dcmp.common_dirs)

    def test_dircmp_recursive(self):
        dcmp = filecmp.dircmp(self.dir1, self.dir2)
        sub = dcmp.subdirs["subdir"]
        self.assertEqual(sub.diff_files, [])
        self.assertEqual(sub.common_files, ["nested.txt"])

    def test_report_methods(self):
        dcmp = filecmp.dircmp(self.dir1, self.dir2)
        dcmp.report()
        dcmp.report_full_closure()
        # 只要不抛出异常就通过
        self.assertTrue(True)


if __name__ == "__main__":
    unittest.main()