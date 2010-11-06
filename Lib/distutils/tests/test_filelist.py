"""Tests for distutils.filelist."""
import unittest

from distutils.filelist import glob_to_re, FileList
from test.support import captured_stdout, run_unittest
from distutils import debug

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

    def test_debug_print(self):
        file_list = FileList()
        with captured_stdout() as stdout:
            file_list.debug_print('xxx')
        stdout.seek(0)
        self.assertEquals(stdout.read(), '')

        debug.DEBUG = True
        try:
            with captured_stdout() as stdout:
                file_list.debug_print('xxx')
            stdout.seek(0)
            self.assertEquals(stdout.read(), 'xxx\n')
        finally:
            debug.DEBUG = False

def test_suite():
    return unittest.makeSuite(FileListTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
