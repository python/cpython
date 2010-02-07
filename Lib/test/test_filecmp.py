
import os, filecmp, shutil, tempfile
import unittest
from test import test_support

class FileCompareTestCase(unittest.TestCase):
    def setUp(self):
        self.name = test_support.TESTFN
        self.name_same = test_support.TESTFN + '-same'
        self.name_diff = test_support.TESTFN + '-diff'
        data = 'Contents of file go here.\n'
        for name in [self.name, self.name_same, self.name_diff]:
            output = open(name, 'w')
            output.write(data)
            output.close()

        output = open(self.name_diff, 'a+')
        output.write('An extra line.\n')
        output.close()
        self.dir = tempfile.gettempdir()

    def tearDown(self):
        os.unlink(self.name)
        os.unlink(self.name_same)
        os.unlink(self.name_diff)

    def test_matching(self):
        self.assertTrue(filecmp.cmp(self.name, self.name_same),
                        "Comparing file to itself fails")
        self.assertTrue(filecmp.cmp(self.name, self.name_same, shallow=False),
                        "Comparing file to itself fails")
        self.assertTrue(filecmp.cmp(self.name, self.name, shallow=False),
                        "Comparing file to identical file fails")
        self.assertTrue(filecmp.cmp(self.name, self.name),
                        "Comparing file to identical file fails")

    def test_different(self):
        self.assertFalse(filecmp.cmp(self.name, self.name_diff),
                    "Mismatched files compare as equal")
        self.assertFalse(filecmp.cmp(self.name, self.dir),
                    "File and directory compare as equal")

class DirCompareTestCase(unittest.TestCase):
    def setUp(self):
        tmpdir = tempfile.gettempdir()
        self.dir = os.path.join(tmpdir, 'dir')
        self.dir_same = os.path.join(tmpdir, 'dir-same')
        self.dir_diff = os.path.join(tmpdir, 'dir-diff')
        self.caseinsensitive = os.path.normcase('A') == os.path.normcase('a')
        data = 'Contents of file go here.\n'
        for dir in [self.dir, self.dir_same, self.dir_diff]:
            shutil.rmtree(dir, True)
            os.mkdir(dir)
            if self.caseinsensitive and dir is self.dir_same:
                fn = 'FiLe'     # Verify case-insensitive comparison
            else:
                fn = 'file'
            output = open(os.path.join(dir, fn), 'w')
            output.write(data)
            output.close()

        output = open(os.path.join(self.dir_diff, 'file2'), 'w')
        output.write('An extra file.\n')
        output.close()

    def tearDown(self):
        shutil.rmtree(self.dir)
        shutil.rmtree(self.dir_same)
        shutil.rmtree(self.dir_diff)

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

        # Add different file2
        output = open(os.path.join(self.dir, 'file2'), 'w')
        output.write('Different contents.\n')
        output.close()

        self.assertFalse(filecmp.cmpfiles(self.dir, self.dir_same,
                                     ['file', 'file2']) ==
                    (['file'], ['file2'], []),
                    "Comparing mismatched directories fails")


    def test_dircmp(self):
        # Check attributes for comparison of two identical directories
        d = filecmp.dircmp(self.dir, self.dir_same)
        if self.caseinsensitive:
            self.assertEqual([d.left_list, d.right_list],[['file'], ['FiLe']])
        else:
            self.assertEqual([d.left_list, d.right_list],[['file'], ['file']])
        self.assertTrue(d.common == ['file'])
        self.assertTrue(d.left_only == d.right_only == [])
        self.assertTrue(d.same_files == ['file'])
        self.assertTrue(d.diff_files == [])

        # Check attributes for comparison of two different directories
        d = filecmp.dircmp(self.dir, self.dir_diff)
        self.assertTrue(d.left_list == ['file'])
        self.assertTrue(d.right_list == ['file', 'file2'])
        self.assertTrue(d.common == ['file'])
        self.assertTrue(d.left_only == [])
        self.assertTrue(d.right_only == ['file2'])
        self.assertTrue(d.same_files == ['file'])
        self.assertTrue(d.diff_files == [])

        # Add different file2
        output = open(os.path.join(self.dir, 'file2'), 'w')
        output.write('Different contents.\n')
        output.close()
        d = filecmp.dircmp(self.dir, self.dir_diff)
        self.assertTrue(d.same_files == ['file'])
        self.assertTrue(d.diff_files == ['file2'])


def test_main():
    test_support.run_unittest(FileCompareTestCase, DirCompareTestCase)

if __name__ == "__main__":
    test_main()
