"""Tests for distutils.filelist."""
import unittest
from distutils.filelist import glob_to_re

class FileListTestCase(unittest.TestCase):

    def test_glob_to_re(self):
        # simple cases
        self.assertEquals(glob_to_re('foo*'), 'foo[^/]*$')
        self.assertEquals(glob_to_re('foo?'), 'foo[^/]$')
        self.assertEquals(glob_to_re('foo??'), 'foo[^/][^/]$')

        # special cases
        self.assertEquals(glob_to_re(r'foo\\*'), r'foo\\\\[^/]*$')
        self.assertEquals(glob_to_re(r'foo\\\*'), r'foo\\\\\\[^/]*$')
        self.assertEquals(glob_to_re('foo????'), r'foo[^/][^/][^/][^/]$')
        self.assertEquals(glob_to_re(r'foo\\??'), r'foo\\\\[^/][^/]$')

def test_suite():
    return unittest.makeSuite(FileListTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
