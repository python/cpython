"""Tests for packaging.manifest."""
import os
import re
from io import StringIO
from packaging.errors import PackagingTemplateError
from packaging.manifest import Manifest, _translate_pattern, _glob_to_re

from packaging.tests import unittest, support

_MANIFEST = """\
recursive-include foo *.py   # ok
# nothing here

#

recursive-include bar \\
  *.dat   *.txt
"""

_MANIFEST2 = """\
README
file1
"""


class ManifestTestCase(support.TempdirManager,
                       support.LoggingCatcher,
                       unittest.TestCase):

    def setUp(self):
        super(ManifestTestCase, self).setUp()
        self.cwd = os.getcwd()

    def tearDown(self):
        os.chdir(self.cwd)
        super(ManifestTestCase, self).tearDown()

    def assertNoWarnings(self):
        self.assertEqual(self.get_logs(), [])

    def assertWarnings(self):
        self.assertNotEqual(self.get_logs(), [])

    def test_manifest_reader(self):
        tmpdir = self.mkdtemp()
        MANIFEST = os.path.join(tmpdir, 'MANIFEST.in')
        with open(MANIFEST, 'w') as f:
            f.write(_MANIFEST)

        manifest = Manifest()
        manifest.read_template(MANIFEST)

        warnings = self.get_logs()
        # the manifest should have been read and 3 warnings issued
        # (we didn't provide the files)
        self.assertEqual(3, len(warnings))
        for warning in warnings:
            self.assertIn('no files found matching', warning)

        # manifest also accepts file-like objects
        with open(MANIFEST) as f:
            manifest.read_template(f)

        # the manifest should have been read and 3 warnings issued
        # (we didn't provide the files)
        self.assertEqual(3, len(warnings))

    def test_default_actions(self):
        tmpdir = self.mkdtemp()
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(tmpdir)
        self.write_file('README', 'xxx')
        self.write_file('file1', 'xxx')
        content = StringIO(_MANIFEST2)
        manifest = Manifest()
        manifest.read_template(content)
        self.assertEqual(['README', 'file1'], manifest.files)

    def test_glob_to_re(self):
        # simple cases
        self.assertEqual(_glob_to_re('foo*'), 'foo[^/]*\\Z(?ms)')
        self.assertEqual(_glob_to_re('foo?'), 'foo[^/]\\Z(?ms)')
        self.assertEqual(_glob_to_re('foo??'), 'foo[^/][^/]\\Z(?ms)')

        # special cases
        self.assertEqual(_glob_to_re(r'foo\\*'), r'foo\\\\[^/]*\Z(?ms)')
        self.assertEqual(_glob_to_re(r'foo\\\*'), r'foo\\\\\\[^/]*\Z(?ms)')
        self.assertEqual(_glob_to_re('foo????'), r'foo[^/][^/][^/][^/]\Z(?ms)')
        self.assertEqual(_glob_to_re(r'foo\\??'), r'foo\\\\[^/][^/]\Z(?ms)')

    def test_remove_duplicates(self):
        manifest = Manifest()
        manifest.files = ['a', 'b', 'a', 'g', 'c', 'g']
        # files must be sorted beforehand
        manifest.sort()
        manifest.remove_duplicates()
        self.assertEqual(manifest.files, ['a', 'b', 'c', 'g'])

    def test_translate_pattern(self):
        # blackbox test of a private function

        # not regex
        pattern = _translate_pattern('a', anchor=True, is_regex=False)
        self.assertTrue(hasattr(pattern, 'search'))

        # is a regex
        regex = re.compile('a')
        pattern = _translate_pattern(regex, anchor=True, is_regex=True)
        self.assertEqual(pattern, regex)

        # plain string flagged as regex
        pattern = _translate_pattern('a', anchor=True, is_regex=True)
        self.assertTrue(hasattr(pattern, 'search'))

        # glob support
        pattern = _translate_pattern('*.py', anchor=True, is_regex=False)
        self.assertTrue(pattern.search('filelist.py'))

    def test_exclude_pattern(self):
        # return False if no match
        manifest = Manifest()
        self.assertFalse(manifest.exclude_pattern('*.py'))

        # return True if files match
        manifest = Manifest()
        manifest.files = ['a.py', 'b.py']
        self.assertTrue(manifest.exclude_pattern('*.py'))

        # test excludes
        manifest = Manifest()
        manifest.files = ['a.py', 'a.txt']
        manifest.exclude_pattern('*.py')
        self.assertEqual(manifest.files, ['a.txt'])

    def test_include_pattern(self):
        # return False if no match
        manifest = Manifest()
        manifest.allfiles = []
        self.assertFalse(manifest._include_pattern('*.py'))

        # return True if files match
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'b.txt']
        self.assertTrue(manifest._include_pattern('*.py'))

        # test * matches all files
        manifest = Manifest()
        self.assertIsNone(manifest.allfiles)
        manifest.allfiles = ['a.py', 'b.txt']
        manifest._include_pattern('*')
        self.assertEqual(manifest.allfiles, ['a.py', 'b.txt'])

    def test_process_template(self):
        # invalid lines
        manifest = Manifest()
        for action in ('include', 'exclude', 'global-include',
                       'global-exclude', 'recursive-include',
                       'recursive-exclude', 'graft', 'prune'):
            self.assertRaises(PackagingTemplateError,
                              manifest._process_template_line, action)

        # implicit include
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'b.txt', 'd/c.py']

        manifest._process_template_line('*.py')
        self.assertEqual(manifest.files, ['a.py'])
        self.assertNoWarnings()

        # include
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'b.txt', 'd/c.py']

        manifest._process_template_line('include *.py')
        self.assertEqual(manifest.files, ['a.py'])
        self.assertNoWarnings()

        manifest._process_template_line('include *.rb')
        self.assertEqual(manifest.files, ['a.py'])
        self.assertWarnings()

        # exclude
        manifest = Manifest()
        manifest.files = ['a.py', 'b.txt', 'd/c.py']

        manifest._process_template_line('exclude *.py')
        self.assertEqual(manifest.files, ['b.txt', 'd/c.py'])
        self.assertNoWarnings()

        manifest._process_template_line('exclude *.rb')
        self.assertEqual(manifest.files, ['b.txt', 'd/c.py'])
        self.assertWarnings()

        # global-include
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'b.txt', 'd/c.py']

        manifest._process_template_line('global-include *.py')
        self.assertEqual(manifest.files, ['a.py', 'd/c.py'])
        self.assertNoWarnings()

        manifest._process_template_line('global-include *.rb')
        self.assertEqual(manifest.files, ['a.py', 'd/c.py'])
        self.assertWarnings()

        # global-exclude
        manifest = Manifest()
        manifest.files = ['a.py', 'b.txt', 'd/c.py']

        manifest._process_template_line('global-exclude *.py')
        self.assertEqual(manifest.files, ['b.txt'])
        self.assertNoWarnings()

        manifest._process_template_line('global-exclude *.rb')
        self.assertEqual(manifest.files, ['b.txt'])
        self.assertWarnings()

        # recursive-include
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'd/b.py', 'd/c.txt', 'd/d/e.py']

        manifest._process_template_line('recursive-include d *.py')
        self.assertEqual(manifest.files, ['d/b.py', 'd/d/e.py'])
        self.assertNoWarnings()

        manifest._process_template_line('recursive-include e *.py')
        self.assertEqual(manifest.files, ['d/b.py', 'd/d/e.py'])
        self.assertWarnings()

        # recursive-exclude
        manifest = Manifest()
        manifest.files = ['a.py', 'd/b.py', 'd/c.txt', 'd/d/e.py']

        manifest._process_template_line('recursive-exclude d *.py')
        self.assertEqual(manifest.files, ['a.py', 'd/c.txt'])
        self.assertNoWarnings()

        manifest._process_template_line('recursive-exclude e *.py')
        self.assertEqual(manifest.files, ['a.py', 'd/c.txt'])
        self.assertWarnings()

        # graft
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'd/b.py', 'd/d/e.py', 'f/f.py']

        manifest._process_template_line('graft d')
        self.assertEqual(manifest.files, ['d/b.py', 'd/d/e.py'])
        self.assertNoWarnings()

        manifest._process_template_line('graft e')
        self.assertEqual(manifest.files, ['d/b.py', 'd/d/e.py'])
        self.assertWarnings()

        # prune
        manifest = Manifest()
        manifest.files = ['a.py', 'd/b.py', 'd/d/e.py', 'f/f.py']

        manifest._process_template_line('prune d')
        self.assertEqual(manifest.files, ['a.py', 'f/f.py'])
        self.assertNoWarnings()

        manifest._process_template_line('prune e')
        self.assertEqual(manifest.files, ['a.py', 'f/f.py'])
        self.assertWarnings()


def test_suite():
    return unittest.makeSuite(ManifestTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
