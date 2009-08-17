"""Tests for distutils.filelist."""
import unittest
from distutils.filelist import glob_to_re

class FileListTestCase(unittest.TestCase):

    def test_glob_to_re(self):
        # simple cases
        self.assertEquals(glob_to_re('foo*'), 'foo[^/]*\\Z(?ms)')
        self.assertEquals(glob_to_re('foo?'), 'foo[^/]\\Z(?ms)')
        self.assertEquals(glob_to_re('foo??'), 'foo[^/][^/]\\Z(?ms)')

        # special cases
        self.assertEquals(glob_to_re(r'foo\\*'), r'foo\\\\[^/]*\Z(?ms)')
        self.assertEquals(glob_to_re(r'foo\\\*'), r'foo\\\\\\[^/]*\Z(?ms)')
        self.assertEquals(glob_to_re('foo????'), r'foo[^/][^/][^/][^/]\Z(?ms)')
        self.assertEquals(glob_to_re(r'foo\\??'), r'foo\\\\[^/][^/]\Z(?ms)')

def test_suite():
    return unittest.makeSuite(FileListTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
