"""Tests for packaging.manifest."""
import os
import re
from io import StringIO
from packaging.errors import PackagingTemplateError
from packaging.manifest import Manifest, _translate_pattern, _glob_to_re

from packaging.tests import unittest, support

MANIFEST_IN = """\
include ok
include xo
exclude xo
include foo.tmp
include buildout.cfg
global-include *.x
global-include *.txt
global-exclude *.tmp
recursive-include f *.oo
recursive-exclude global *.x
graft dir
prune dir3
"""

MANIFEST_IN_2 = """\
recursive-include foo *.py   # ok
# nothing here

#

recursive-include bar \\
  *.dat   *.txt
"""

MANIFEST_IN_3 = """\
README
file1
"""


def make_local_path(s):
    """Converts '/' in a string to os.sep"""
    return s.replace('/', os.sep)


class ManifestTestCase(support.TempdirManager,
                       support.LoggingCatcher,
                       unittest.TestCase):

    def assertNoWarnings(self):
        self.assertEqual(self.get_logs(), [])

    def assertWarnings(self):
        self.assertNotEqual(self.get_logs(), [])

    def test_manifest_reader(self):
        tmpdir = self.mkdtemp()
        MANIFEST = os.path.join(tmpdir, 'MANIFEST.in')
        with open(MANIFEST, 'w') as f:
            f.write(MANIFEST_IN_2)

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
        content = StringIO(MANIFEST_IN_3)
        manifest = Manifest()
        manifest.read_template(content)
        self.assertEqual(['README', 'file1'], manifest.files)

    def test_glob_to_re(self):
        sep = os.sep
        if os.sep == '\\':
            sep = r'\\'

        for glob, regex in (
            # simple cases
            ('foo*', r'foo[^%(sep)s]*\Z(?ms)'),
            ('foo?', r'foo[^%(sep)s]\Z(?ms)'),
            ('foo??', r'foo[^%(sep)s][^%(sep)s]\Z(?ms)'),
            # special cases
            (r'foo\\*', r'foo\\\\[^%(sep)s]*\Z(?ms)'),
            (r'foo\\\*', r'foo\\\\\\[^%(sep)s]*\Z(?ms)'),
            ('foo????', r'foo[^%(sep)s][^%(sep)s][^%(sep)s][^%(sep)s]\Z(?ms)'),
            (r'foo\\??', r'foo\\\\[^%(sep)s][^%(sep)s]\Z(?ms)'),
        ):
            regex = regex % {'sep': sep}
            self.assertEqual(_glob_to_re(glob), regex)

    def test_process_template_line(self):
        # testing  all MANIFEST.in template patterns
        manifest = Manifest()
        l = make_local_path

        # simulated file list
        manifest.allfiles = ['foo.tmp', 'ok', 'xo', 'four.txt',
                              'buildout.cfg',
                              # filelist does not filter out VCS directories,
                              # it's sdist that does
                              l('.hg/last-message.txt'),
                              l('global/one.txt'),
                              l('global/two.txt'),
                              l('global/files.x'),
                              l('global/here.tmp'),
                              l('f/o/f.oo'),
                              l('dir/graft-one'),
                              l('dir/dir2/graft2'),
                              l('dir3/ok'),
                              l('dir3/sub/ok.txt'),
                             ]

        for line in MANIFEST_IN.split('\n'):
            if line.strip() == '':
                continue
            manifest._process_template_line(line)

        wanted = ['ok',
                  'buildout.cfg',
                  'four.txt',
                  l('.hg/last-message.txt'),
                  l('global/one.txt'),
                  l('global/two.txt'),
                  l('f/o/f.oo'),
                  l('dir/graft-one'),
                  l('dir/dir2/graft2'),
                 ]

        self.assertEqual(manifest.files, wanted)

    def test_remove_duplicates(self):
        manifest = Manifest()
        manifest.files = ['a', 'b', 'a', 'g', 'c', 'g']
        # files must be sorted beforehand (like sdist does)
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
        l = make_local_path
        # invalid lines
        manifest = Manifest()
        for action in ('include', 'exclude', 'global-include',
                       'global-exclude', 'recursive-include',
                       'recursive-exclude', 'graft', 'prune'):
            self.assertRaises(PackagingTemplateError,
                              manifest._process_template_line, action)

        # implicit include
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'b.txt', l('d/c.py')]

        manifest._process_template_line('*.py')
        self.assertEqual(manifest.files, ['a.py'])
        self.assertNoWarnings()

        # include
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'b.txt', l('d/c.py')]

        manifest._process_template_line('include *.py')
        self.assertEqual(manifest.files, ['a.py'])
        self.assertNoWarnings()

        manifest._process_template_line('include *.rb')
        self.assertEqual(manifest.files, ['a.py'])
        self.assertWarnings()

        # exclude
        manifest = Manifest()
        manifest.files = ['a.py', 'b.txt', l('d/c.py')]

        manifest._process_template_line('exclude *.py')
        self.assertEqual(manifest.files, ['b.txt', l('d/c.py')])
        self.assertNoWarnings()

        manifest._process_template_line('exclude *.rb')
        self.assertEqual(manifest.files, ['b.txt', l('d/c.py')])
        self.assertWarnings()

        # global-include
        manifest = Manifest()
        manifest.allfiles = ['a.py', 'b.txt', l('d/c.py')]

        manifest._process_template_line('global-include *.py')
        self.assertEqual(manifest.files, ['a.py', l('d/c.py')])
        self.assertNoWarnings()

        manifest._process_template_line('global-include *.rb')
        self.assertEqual(manifest.files, ['a.py', l('d/c.py')])
        self.assertWarnings()

        # global-exclude
        manifest = Manifest()
        manifest.files = ['a.py', 'b.txt', l('d/c.py')]

        manifest._process_template_line('global-exclude *.py')
        self.assertEqual(manifest.files, ['b.txt'])
        self.assertNoWarnings()

        manifest._process_template_line('global-exclude *.rb')
        self.assertEqual(manifest.files, ['b.txt'])
        self.assertWarnings()

        # recursive-include
        manifest = Manifest()
        manifest.allfiles = ['a.py', l('d/b.py'), l('d/c.txt'), l('d/d/e.py')]

        manifest._process_template_line('recursive-include d *.py')
        self.assertEqual(manifest.files, [l('d/b.py'), l('d/d/e.py')])
        self.assertNoWarnings()

        manifest._process_template_line('recursive-include e *.py')
        self.assertEqual(manifest.files, [l('d/b.py'), l('d/d/e.py')])
        self.assertWarnings()

        # recursive-exclude
        manifest = Manifest()
        manifest.files = ['a.py', l('d/b.py'), l('d/c.txt'), l('d/d/e.py')]

        manifest._process_template_line('recursive-exclude d *.py')
        self.assertEqual(manifest.files, ['a.py', l('d/c.txt')])
        self.assertNoWarnings()

        manifest._process_template_line('recursive-exclude e *.py')
        self.assertEqual(manifest.files, ['a.py', l('d/c.txt')])
        self.assertWarnings()

        # graft
        manifest = Manifest()
        manifest.allfiles = ['a.py', l('d/b.py'), l('d/d/e.py'), l('f/f.py')]

        manifest._process_template_line('graft d')
        self.assertEqual(manifest.files, [l('d/b.py'), l('d/d/e.py')])
        self.assertNoWarnings()

        manifest._process_template_line('graft e')
        self.assertEqual(manifest.files, [l('d/b.py'), l('d/d/e.py')])
        self.assertWarnings()

        # prune
        manifest = Manifest()
        manifest.files = ['a.py', l('d/b.py'), l('d/d/e.py'), l('f/f.py')]

        manifest._process_template_line('prune d')
        self.assertEqual(manifest.files, ['a.py', l('f/f.py')])
        self.assertNoWarnings()

        manifest._process_template_line('prune e')
        self.assertEqual(manifest.files, ['a.py', l('f/f.py')])
        self.assertWarnings()


def test_suite():
    return unittest.makeSuite(ManifestTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
