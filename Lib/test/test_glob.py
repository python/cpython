import unittest
from test.support import (run_unittest, TESTFN, skip_unless_symlink,
    can_symlink, create_empty_file)
import glob
import os
import shutil

class GlobTestsBase(unittest.TestCase):

    def norm(self, *parts):
        return os.path.normpath(os.path.join(self.tempdir, *parts))

    def mktemp(self, *parts):
        filename = self.norm(*parts)
        base, file = os.path.split(filename)
        if not os.path.exists(base):
            os.makedirs(base)
        create_empty_file(filename)

    def setUp(self):
        self.tempdir = TESTFN+"_dir"
        self.mktemp('a', 'D')
        self.mktemp('aab', 'F')
        self.mktemp('aaa', 'zzzF')
        self.mktemp('ZZZ')
        self.mktemp('a', 'bcd', 'EF')
        self.mktemp('a', 'bcd', 'efg', 'ha')
        if can_symlink():
            os.symlink(self.norm('broken'), self.norm('sym1'))
            os.symlink(self.norm('broken'), self.norm('sym2'))

    def tearDown(self):
        shutil.rmtree(self.tempdir)

    def glob(self, *parts):
        if len(parts) == 1:
            pattern = parts[0]
        else:
            pattern = os.path.join(*parts)
        p = os.path.join(self.tempdir, pattern)
        res = glob.glob(p)
        self.assertEqual(list(glob.iglob(p)), res)
        return res

    def assertSequencesEqual_noorder(self, l1, l2):
        self.assertEqual(set(l1), set(l2))

class GlobTests(GlobTestsBase):

    def test_glob_literal(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('a'), [self.norm('a')])
        eq(self.glob('a', 'D'), [self.norm('a', 'D')])
        eq(self.glob('aab'), [self.norm('aab')])
        eq(self.glob('zymurgy'), [])

        # test return types are unicode, but only if os.listdir
        # returns unicode filenames
        uniset = set([str])
        tmp = os.listdir('.')
        if set(type(x) for x in tmp) == uniset:
            u1 = glob.glob('*')
            u2 = glob.glob('./*')
            self.assertEqual(set(type(r) for r in u1), uniset)
            self.assertEqual(set(type(r) for r in u2), uniset)

    def test_glob_one_directory(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('a*'), map(self.norm, ['a', 'aab', 'aaa']))
        eq(self.glob('*a'), map(self.norm, ['a', 'aaa']))
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
        eq(self.glob('?a?', '*F'), map(self.norm, [os.path.join('aaa', 'zzzF'),
                                                   os.path.join('aab', 'F')]))

    def test_glob_directory_with_trailing_slash(self):
        # We are verifying that when there is wildcard pattern which
        # ends with os.sep doesn't blow up.
        res = glob.glob(self.tempdir + '*' + os.sep)
        self.assertEqual(len(res), 1)
        # either of these results are reasonable
        self.assertIn(res[0], [self.tempdir, self.tempdir + os.sep])

    @skip_unless_symlink
    def test_glob_broken_symlinks(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('sym*'), [self.norm('sym1'), self.norm('sym2')])
        eq(self.glob('sym1'), [self.norm('sym1')])
        eq(self.glob('sym2'), [self.norm('sym2')])

class GlobBracesTests(GlobTestsBase):

    def setUp(self):
        super(GlobBracesTests, self).setUp()
        self.mktemp('c{}d')
        self.mktemp('c{deg')
        self.mktemp('c{dfg')
        self.mktemp('cd{f}g')
        self.mktemp('ce{f}g')
        self.mktemp('cdf}g')
        self.mktemp('cef}g')

    def match_pattern_with_results(self, patterns, paths):
        expected = [self.norm(path) for path in [os.path.join(*parts) for parts in paths]]
        actual = [os.path.normpath(g) for g in self.glob(*patterns)]
        self.assertSequencesEqual_noorder(actual, expected)

    def test_two_terms(self):
        self.match_pattern_with_results(['a{aa,ab}'], [["aaa"], ["aab"]])

    def test_missing_first_plus_nested(self):
        self.match_pattern_with_results(['a{,a{a,b}}'], [['a'], ['aaa'], ['aab']])

    def test_one_subpath_with_two_file_terms(self):
        self.match_pattern_with_results(['a', '{D,bcd}'], [['a', 'D'], ['a', 'bcd']])

    def test_two_subpath_terms_with_two_file_terms(self):
        self.match_pattern_with_results(['{aaa,aab}', '{F,zzzF}'], [('aaa', 'zzzF'), ('aab', 'F')])

    def test_two_subpath_terms_with_wildcard_file_term(self):
        self.match_pattern_with_results(['aa{a,b}', '*F'], [('aaa', 'zzzF'), ('aab', 'F')])

    def test_wildcard_subpath_with_file_missing_first_term(self):
        self.match_pattern_with_results(['aa?', '{,zzz}F'], [('aaa', 'zzzF'), ('aab', 'F')])

    #
    # Edge cases where braces should not be expanded
    #
    def test_empty_braces(self):
        self.assertSequencesEqual_noorder(self.glob('c{}d'), [self.norm('c{}d')])

    def test_missing_end_brace(self):
        self.assertSequencesEqual_noorder(self.glob('c{d{e,f}g'), map(self.norm, ['c{deg', 'c{dfg']))

    def test_second_brace_one_term(self):
        self.assertSequencesEqual_noorder(self.glob('c{d,e}{f}g'), map(self.norm, ['cd{f}g', 'ce{f}g']))

    def test_outer_term_missing_first_brace(self):
        self.assertSequencesEqual_noorder(self.glob('c{d,e}f}g'), map(self.norm, ['cdf}g', 'cef}g']))

    #
    # Braces containing folder separators
    #
    def test_embedded_separator1(self):
        self.match_pattern_with_results(['a/{D,bcd/{EF,efg}}'], [('a', 'D'), ('a', 'bcd', 'EF'), ('a', 'bcd', 'efg')])

    def test_embedded_separator2(self):
        self.match_pattern_with_results(['aa{a/zzz,b/}F'], [('aaa', 'zzzF'), ('aab', 'F')])

def test_main():
    run_unittest(GlobTests, GlobBracesTests)


if __name__ == "__main__":
    test_main()
