"""Tests for distutils.filelist."""
import re
import unittest
from os.path import join
from distutils import debug
from distutils.log import WARN
from distutils.errors import DistutilsTemplateError
from distutils.filelist import glob_to_re, translate_pattern, FileList

from test.test_support import captured_stdout, run_unittest
from distutils.tests import support

MANIFEST_IN = """\
include ok
include xo
exclude xo
include foo.tmp
global-include *.x
global-include *.txt
global-exclude *.tmp
recursive-include f *.oo
recursive-exclude global *.x
graft dir
prune dir3
"""


class FileListTestCase(support.LoggingSilencer,
                       unittest.TestCase):

    def assertNoWarnings(self):
        self.assertEqual(self.get_logs(WARN), [])
        self.clear_logs()

    def assertWarnings(self):
        self.assertGreater(len(self.get_logs(WARN)), 0)
        self.clear_logs()

    def test_glob_to_re(self):
        # simple cases
        self.assertEqual(glob_to_re('foo*'), 'foo[^/]*\\Z(?ms)')
        self.assertEqual(glob_to_re('foo?'), 'foo[^/]\\Z(?ms)')
        self.assertEqual(glob_to_re('foo??'), 'foo[^/][^/]\\Z(?ms)')

        # special cases
        self.assertEqual(glob_to_re(r'foo\\*'), r'foo\\\\[^/]*\Z(?ms)')
        self.assertEqual(glob_to_re(r'foo\\\*'), r'foo\\\\\\[^/]*\Z(?ms)')
        self.assertEqual(glob_to_re('foo????'), r'foo[^/][^/][^/][^/]\Z(?ms)')
        self.assertEqual(glob_to_re(r'foo\\??'), r'foo\\\\[^/][^/]\Z(?ms)')

    def test_process_template_line(self):
        # testing  all MANIFEST.in template patterns
        file_list = FileList()

        # simulated file list
        file_list.allfiles = ['foo.tmp', 'ok', 'xo', 'four.txt',
                              join('global', 'one.txt'),
                              join('global', 'two.txt'),
                              join('global', 'files.x'),
                              join('global', 'here.tmp'),
                              join('f', 'o', 'f.oo'),
                              join('dir', 'graft-one'),
                              join('dir', 'dir2', 'graft2'),
                              join('dir3', 'ok'),
                              join('dir3', 'sub', 'ok.txt'),
                              ]

        for line in MANIFEST_IN.split('\n'):
            if line.strip() == '':
                continue
            file_list.process_template_line(line)

        wanted = ['ok', 'four.txt', join('global', 'one.txt'),
                  join('global', 'two.txt'), join('f', 'o', 'f.oo'),
                  join('dir', 'graft-one'), join('dir', 'dir2', 'graft2')]

        self.assertEqual(file_list.files, wanted)

    def test_debug_print(self):
        file_list = FileList()
        with captured_stdout() as stdout:
            file_list.debug_print('xxx')
        self.assertEqual(stdout.getvalue(), '')

        debug.DEBUG = True
        try:
            with captured_stdout() as stdout:
                file_list.debug_print('xxx')
            self.assertEqual(stdout.getvalue(), 'xxx\n')
        finally:
            debug.DEBUG = False

    def test_set_allfiles(self):
        file_list = FileList()
        files = ['a', 'b', 'c']
        file_list.set_allfiles(files)
        self.assertEqual(file_list.allfiles, files)

    def test_remove_duplicates(self):
        file_list = FileList()
        file_list.files = ['a', 'b', 'a', 'g', 'c', 'g']
        # files must be sorted beforehand (sdist does it)
        file_list.sort()
        file_list.remove_duplicates()
        self.assertEqual(file_list.files, ['a', 'b', 'c', 'g'])

    def test_translate_pattern(self):
        # not regex
        self.assertTrue(hasattr(
            translate_pattern('a', anchor=True, is_regex=False),
            'search'))

        # is a regex
        regex = re.compile('a')
        self.assertEqual(
            translate_pattern(regex, anchor=True, is_regex=True),
            regex)

        # plain string flagged as regex
        self.assertTrue(hasattr(
            translate_pattern('a', anchor=True, is_regex=True),
            'search'))

        # glob support
        self.assertTrue(translate_pattern(
            '*.py', anchor=True, is_regex=False).search('filelist.py'))

    def test_exclude_pattern(self):
        # return False if no match
        file_list = FileList()
        self.assertFalse(file_list.exclude_pattern('*.py'))

        # return True if files match
        file_list = FileList()
        file_list.files = ['a.py', 'b.py']
        self.assertTrue(file_list.exclude_pattern('*.py'))

        # test excludes
        file_list = FileList()
        file_list.files = ['a.py', 'a.txt']
        file_list.exclude_pattern('*.py')
        self.assertEqual(file_list.files, ['a.txt'])

    def test_include_pattern(self):
        # return False if no match
        file_list = FileList()
        file_list.set_allfiles([])
        self.assertFalse(file_list.include_pattern('*.py'))

        # return True if files match
        file_list = FileList()
        file_list.set_allfiles(['a.py', 'b.txt'])
        self.assertTrue(file_list.include_pattern('*.py'))

        # test * matches all files
        file_list = FileList()
        self.assertIsNone(file_list.allfiles)
        file_list.set_allfiles(['a.py', 'b.txt'])
        file_list.include_pattern('*')
        self.assertEqual(file_list.allfiles, ['a.py', 'b.txt'])

    def test_process_template(self):
        # invalid lines
        file_list = FileList()
        for action in ('include', 'exclude', 'global-include',
                       'global-exclude', 'recursive-include',
                       'recursive-exclude', 'graft', 'prune', 'blarg'):
            self.assertRaises(DistutilsTemplateError,
                              file_list.process_template_line, action)

        # include
        file_list = FileList()
        file_list.set_allfiles(['a.py', 'b.txt', 'd/c.py'])

        file_list.process_template_line('include *.py')
        self.assertEqual(file_list.files, ['a.py'])
        self.assertNoWarnings()

        file_list.process_template_line('include *.rb')
        self.assertEqual(file_list.files, ['a.py'])
        self.assertWarnings()

        # exclude
        file_list = FileList()
        file_list.files = ['a.py', 'b.txt', 'd/c.py']

        file_list.process_template_line('exclude *.py')
        self.assertEqual(file_list.files, ['b.txt', 'd/c.py'])
        self.assertNoWarnings()

        file_list.process_template_line('exclude *.rb')
        self.assertEqual(file_list.files, ['b.txt', 'd/c.py'])
        self.assertWarnings()

        # global-include
        file_list = FileList()
        file_list.set_allfiles(['a.py', 'b.txt', 'd/c.py'])

        file_list.process_template_line('global-include *.py')
        self.assertEqual(file_list.files, ['a.py', 'd/c.py'])
        self.assertNoWarnings()

        file_list.process_template_line('global-include *.rb')
        self.assertEqual(file_list.files, ['a.py', 'd/c.py'])
        self.assertWarnings()

        # global-exclude
        file_list = FileList()
        file_list.files = ['a.py', 'b.txt', 'd/c.py']

        file_list.process_template_line('global-exclude *.py')
        self.assertEqual(file_list.files, ['b.txt'])
        self.assertNoWarnings()

        file_list.process_template_line('global-exclude *.rb')
        self.assertEqual(file_list.files, ['b.txt'])
        self.assertWarnings()

        # recursive-include
        file_list = FileList()
        file_list.set_allfiles(['a.py', 'd/b.py', 'd/c.txt', 'd/d/e.py'])

        file_list.process_template_line('recursive-include d *.py')
        self.assertEqual(file_list.files, ['d/b.py', 'd/d/e.py'])
        self.assertNoWarnings()

        file_list.process_template_line('recursive-include e *.py')
        self.assertEqual(file_list.files, ['d/b.py', 'd/d/e.py'])
        self.assertWarnings()

        # recursive-exclude
        file_list = FileList()
        file_list.files = ['a.py', 'd/b.py', 'd/c.txt', 'd/d/e.py']

        file_list.process_template_line('recursive-exclude d *.py')
        self.assertEqual(file_list.files, ['a.py', 'd/c.txt'])
        self.assertNoWarnings()

        file_list.process_template_line('recursive-exclude e *.py')
        self.assertEqual(file_list.files, ['a.py', 'd/c.txt'])
        self.assertWarnings()

        # graft
        file_list = FileList()
        file_list.set_allfiles(['a.py', 'd/b.py', 'd/d/e.py', 'f/f.py'])

        file_list.process_template_line('graft d')
        self.assertEqual(file_list.files, ['d/b.py', 'd/d/e.py'])
        self.assertNoWarnings()

        file_list.process_template_line('graft e')
        self.assertEqual(file_list.files, ['d/b.py', 'd/d/e.py'])
        self.assertWarnings()

        # prune
        file_list = FileList()
        file_list.files = ['a.py', 'd/b.py', 'd/d/e.py', 'f/f.py']

        file_list.process_template_line('prune d')
        self.assertEqual(file_list.files, ['a.py', 'f/f.py'])
        self.assertNoWarnings()

        file_list.process_template_line('prune e')
        self.assertEqual(file_list.files, ['a.py', 'f/f.py'])
        self.assertWarnings()


def test_suite():
    return unittest.makeSuite(FileListTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
