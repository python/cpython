import unittest
from test_support import run_unittest, TESTFN
import glob
import os

def mkdirs(fname):
    if os.path.exists(fname) or fname == '':
        return
    base, file = os.path.split(fname)
    mkdirs(base)
    os.mkdir(fname)

def touchfile(fname):
    base, file = os.path.split(fname)
    mkdirs(base)
    f = open(fname, 'w')
    f.close()

def deltree(fname):
    for f in os.listdir(fname):
        fullname = os.path.join(fname, f)
        if os.path.isdir(fullname):
            deltree(fullname)
        else:
            try:
                os.unlink(fullname)
            except:
                pass
    try:
        os.rmdir(fname)
    except:
        pass


class GlobTests(unittest.TestCase):

    def norm(self, *parts):
        return os.path.normpath(os.path.join(self.tempdir, *parts))

    def mktemp(self, *parts):
        touchfile(self.norm(*parts))

    def setUp(self):
        self.tempdir = TESTFN+"_dir"
        self.mktemp('a', 'D')
        self.mktemp('aab', 'F')
        self.mktemp('aaa', 'zzzF')
        self.mktemp('ZZZ')
        self.mktemp('a', 'bcd', 'EF')
        self.mktemp('a', 'bcd', 'efg', 'ha')

    def tearDown(self):
        deltree(self.tempdir)

    def glob(self, *parts):
        if len(parts) == 1:
            pattern = parts[0]
        else:
            pattern = os.path.join(*parts)
        p = os.path.join(self.tempdir, pattern)
        return glob.glob(p)

    def assertSequencesEqual_noorder(self, l1, l2):
        l1 = list(l1)
        l2 = list(l2)
        l1.sort()
        l2.sort()
        self.assertEqual(l1, l2)

    def test_glob_literal(self):
        eq = self.assertSequencesEqual_noorder
        np = lambda *f: norm(self.tempdir, *f)
        eq(self.glob('a'), [self.norm('a')])
        eq(self.glob('a', 'D'), [self.norm('a', 'D')])
        eq(self.glob('aab'), [self.norm('aab')])
        eq(self.glob('zymurgy'), [])

    def test_glob_one_directory(self):
        eq = self.assertSequencesEqual_noorder
        np = lambda *f: norm(self.tempdir, *f)
        eq(self.glob('a*'), map(self.norm, ['a', 'aab', 'aaa']))
        eq(self.glob('*a'), map(self.norm, ['a', 'aaa']))
        eq(self.glob('aa?'), map(self.norm, ['aaa', 'aab']))
        eq(self.glob('aa[ab]'), map(self.norm, ['aaa', 'aab']))
        eq(self.glob('*q'), [])

    def test_glob_nested_directory(self):
        eq = self.assertSequencesEqual_noorder
        np = lambda *f: norm(self.tempdir, *f)
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
        np = lambda *f: norm(self.tempdir, *f)
        eq(self.glob('*', 'D'), [self.norm('a', 'D')])
        eq(self.glob('*', '*a'), [])
        eq(self.glob('a', '*', '*', '*a'),
           [self.norm('a', 'bcd', 'efg', 'ha')])
        eq(self.glob('?a?', '*F'), map(self.norm, [os.path.join('aaa', 'zzzF'),
                                                   os.path.join('aab', 'F')]))


def test_main():
    run_unittest(GlobTests)


if __name__ == "__main__":
    test_main()
