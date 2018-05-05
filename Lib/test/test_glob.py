import glob
import os
import shutil
import sys
import unittest

from test.test_support import run_unittest, TESTFN


def fsdecode(s):
    return unicode(s, sys.getfilesystemencoding())


class GlobTests(unittest.TestCase):

    def norm(self, *parts):
        return os.path.normpath(os.path.join(self.tempdir, *parts))

    def mktemp(self, *parts):
        filename = self.norm(*parts)
        base, file = os.path.split(filename)
        if not os.path.exists(base):
            os.makedirs(base)
        f = open(filename, 'w')
        f.close()

    def setUp(self):
        self.tempdir = TESTFN + "_dir"
        self.mktemp('a', 'D')
        self.mktemp('aab', 'F')
        self.mktemp('.aa', 'G')
        self.mktemp('.bb', 'H')
        self.mktemp('aaa', 'zzzF')
        self.mktemp('ZZZ')
        self.mktemp('a', 'bcd', 'EF')
        self.mktemp('a', 'bcd', 'efg', 'ha')
        if hasattr(os, 'symlink'):
            os.symlink(self.norm('broken'), self.norm('sym1'))
            os.symlink('broken', self.norm('sym2'))
            os.symlink(os.path.join('a', 'bcd'), self.norm('sym3'))

    def tearDown(self):
        shutil.rmtree(self.tempdir)

    def glob(self, *parts):
        if len(parts) == 1:
            pattern = parts[0]
        else:
            pattern = os.path.join(*parts)
        p = os.path.join(self.tempdir, pattern)
        res = glob.glob(p)
        self.assertItemsEqual(glob.iglob(p), res)
        ures = [fsdecode(x) for x in res]
        self.assertItemsEqual(glob.glob(fsdecode(p)), ures)
        self.assertItemsEqual(glob.iglob(fsdecode(p)), ures)
        return res

    def assertSequencesEqual_noorder(self, l1, l2):
        l1 = list(l1)
        l2 = list(l2)
        self.assertEqual(set(l1), set(l2))
        self.assertEqual(sorted(l1), sorted(l2))

    def test_glob_literal(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('a'), [self.norm('a')])
        eq(self.glob('a', 'D'), [self.norm('a', 'D')])
        eq(self.glob('aab'), [self.norm('aab')])
        eq(self.glob('zymurgy'), [])

        res = glob.glob('*')
        self.assertEqual({type(r) for r in res}, {str})
        res = glob.glob(os.path.join(os.curdir, '*'))
        self.assertEqual({type(r) for r in res}, {str})

        # test return types are unicode, but only if os.listdir
        # returns unicode filenames
        tmp = os.listdir(fsdecode(os.curdir))
        if {type(x) for x in tmp} == {unicode}:
            res = glob.glob(u'*')
            self.assertEqual({type(r) for r in res}, {unicode})
            res = glob.glob(os.path.join(fsdecode(os.curdir), u'*'))
            self.assertEqual({type(r) for r in res}, {unicode})

    def test_glob_one_directory(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('a*'), map(self.norm, ['a', 'aab', 'aaa']))
        eq(self.glob('*a'), map(self.norm, ['a', 'aaa']))
        eq(self.glob('.*'), map(self.norm, ['.aa', '.bb']))
        eq(self.glob('?aa'), map(self.norm, ['aaa']))
        eq(self.glob('aa?'), map(self.norm, ['aaa', 'aab']))
        eq(self.glob('aa[ab]'), map(self.norm, ['aaa', 'aab']))
        eq(self.glob('*q'), [])

    def test_glob_nested_directory(self):
        eq = self.assertSequencesEqual_noorder
        if os.path.normcase("abCD") == "abCD":
            # case-sensitive filesystem
            eq(self.glob('a', 'bcd', 'E*'), [self.norm('a', 'bcd', 'EF')])
        else:
            # case insensitive filesystem
            eq(self.glob('a', 'bcd', 'E*'), [self.norm('a', 'bcd', 'EF'),
                                             self.norm('a', 'bcd', 'efg')])
        eq(self.glob('a', 'bcd', '*g'), [self.norm('a', 'bcd', 'efg')])

    def test_glob_directory_names(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('*', 'D'), [self.norm('a', 'D')])
        eq(self.glob('*', '*a'), [])
        eq(self.glob('a', '*', '*', '*a'),
           [self.norm('a', 'bcd', 'efg', 'ha')])
        eq(self.glob('?a?', '*F'), [self.norm('aaa', 'zzzF'),
                                    self.norm('aab', 'F')])

    def test_glob_directory_with_trailing_slash(self):
        # Patterns ending with a slash shouldn't match non-dirs
        res = glob.glob(self.norm('Z*Z') + os.sep)
        self.assertEqual(res, [])
        res = glob.glob(self.norm('ZZZ') + os.sep)
        self.assertEqual(res, [])
        # When there is a wildcard pattern which ends with os.sep, glob()
        # doesn't blow up.
        res = glob.glob(self.norm('aa*') + os.sep)
        self.assertEqual(len(res), 2)
        # either of these results is reasonable
        self.assertIn(set(res), [
                      {self.norm('aaa'), self.norm('aab')},
                      {self.norm('aaa') + os.sep, self.norm('aab') + os.sep},
                      ])

    def test_glob_unicode_directory_with_trailing_slash(self):
        # Same as test_glob_directory_with_trailing_slash, but with an
        # unicode argument.
        res = glob.glob(fsdecode(self.norm('Z*Z') + os.sep))
        self.assertEqual(res, [])
        res = glob.glob(fsdecode(self.norm('ZZZ') + os.sep))
        self.assertEqual(res, [])
        res = glob.glob(fsdecode(self.norm('aa*') + os.sep))
        self.assertEqual(len(res), 2)
        # either of these results is reasonable
        self.assertIn(set(res), [
                      {fsdecode(self.norm('aaa')), fsdecode(self.norm('aab'))},
                      {fsdecode(self.norm('aaa') + os.sep),
                       fsdecode(self.norm('aab') + os.sep)},
                      ])

    @unittest.skipUnless(hasattr(os, 'symlink'), "Requires symlink support")
    def test_glob_symlinks(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('sym3'), [self.norm('sym3')])
        eq(self.glob('sym3', '*'), [self.norm('sym3', 'EF'),
                                    self.norm('sym3', 'efg')])
        self.assertIn(self.glob('sym3' + os.sep),
                      [[self.norm('sym3')], [self.norm('sym3') + os.sep]])
        eq(self.glob('*', '*F'),
           [self.norm('aaa', 'zzzF'), self.norm('aab', 'F'),
            self.norm('sym3', 'EF')])

    @unittest.skipUnless(hasattr(os, 'symlink'), "Requires symlink support")
    def test_glob_broken_symlinks(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('sym*'), [self.norm('sym1'), self.norm('sym2'),
                               self.norm('sym3')])
        eq(self.glob('sym1'), [self.norm('sym1')])
        eq(self.glob('sym2'), [self.norm('sym2')])

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific test")
    def test_glob_magic_in_drive(self):
        eq = self.assertSequencesEqual_noorder
        eq(glob.glob('*:'), [])
        eq(glob.glob(u'*:'), [])
        eq(glob.glob('?:'), [])
        eq(glob.glob(u'?:'), [])


def test_main():
    run_unittest(GlobTests)


if __name__ == "__main__":
    test_main()
