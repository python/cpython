import glob
import os
import shutil
import sys
import unittest

from test.support import (TESTFN, skip_unless_symlink,
                          can_symlink, create_empty_file, change_cwd)


class GlobTests(unittest.TestCase):
    dir_fd = None

    def norm(self, *parts):
        return os.path.normpath(os.path.join(self.tempdir, *parts))

    def joins(self, *tuples):
        return [os.path.join(self.tempdir, *parts) for parts in tuples]

    def mktemp(self, *parts):
        filename = self.norm(*parts)
        base, file = os.path.split(filename)
        if not os.path.exists(base):
            os.makedirs(base)
        create_empty_file(filename)

    def setUp(self):
        self.tempdir = TESTFN + "_dir"
        self.mktemp('a', 'D')
        self.mktemp('aab', 'F')
        self.mktemp('.aa', 'G')
        self.mktemp('.bb', 'H')
        self.mktemp('aaa', 'zzzF')
        self.mktemp('ZZZ')
        self.mktemp('EF')
        self.mktemp('a', 'bcd', 'EF')
        self.mktemp('a', 'bcd', 'efg', 'ha')
        if can_symlink():
            os.symlink(self.norm('broken'), self.norm('sym1'))
            os.symlink('broken', self.norm('sym2'))
            os.symlink(os.path.join('a', 'bcd'), self.norm('sym3'))
        if {os.open, os.stat} <= os.supports_dir_fd and os.scandir in os.supports_fd:
            self.dir_fd = os.open(self.tempdir, os.O_RDONLY | os.O_DIRECTORY)
        else:
            self.dir_fd = None

    def tearDown(self):
        if self.dir_fd is not None:
            os.close(self.dir_fd)
        shutil.rmtree(self.tempdir)

    def glob(self, *parts, **kwargs):
        if len(parts) == 1:
            pattern = parts[0]
        else:
            pattern = os.path.join(*parts)
        p = os.path.join(self.tempdir, pattern)
        res = glob.glob(p, **kwargs)
        self.assertCountEqual(glob.iglob(p, **kwargs), res)
        bres = [os.fsencode(x) for x in res]
        self.assertCountEqual(glob.glob(os.fsencode(p), **kwargs), bres)
        self.assertCountEqual(glob.iglob(os.fsencode(p), **kwargs), bres)

        with change_cwd(self.tempdir):
            res2 = glob.glob(pattern, **kwargs)
            for x in res2:
                self.assertFalse(os.path.isabs(x), x)
            if pattern == '**' or pattern == '**' + os.sep:
                expected = res[1:]
            else:
                expected = res
            self.assertCountEqual([os.path.join(self.tempdir, x) for x in res2],
                                  expected)
            self.assertCountEqual(glob.iglob(pattern, **kwargs), res2)
            bpattern = os.fsencode(pattern)
            bres2 = [os.fsencode(x) for x in res2]
            self.assertCountEqual(glob.glob(bpattern, **kwargs), bres2)
            self.assertCountEqual(glob.iglob(bpattern, **kwargs), bres2)

        self.assertCountEqual(glob.glob(pattern, root_dir=self.tempdir, **kwargs), res2)
        self.assertCountEqual(glob.iglob(pattern, root_dir=self.tempdir, **kwargs), res2)
        btempdir = os.fsencode(self.tempdir)
        self.assertCountEqual(
            glob.glob(bpattern, root_dir=btempdir, **kwargs), bres2)
        self.assertCountEqual(
            glob.iglob(bpattern, root_dir=btempdir, **kwargs), bres2)

        if self.dir_fd is not None:
            self.assertCountEqual(
                glob.glob(pattern, dir_fd=self.dir_fd, **kwargs), res2)
            self.assertCountEqual(
                glob.iglob(pattern, dir_fd=self.dir_fd, **kwargs), res2)
            self.assertCountEqual(
                glob.glob(bpattern, dir_fd=self.dir_fd, **kwargs), bres2)
            self.assertCountEqual(
                glob.iglob(bpattern, dir_fd=self.dir_fd, **kwargs), bres2)

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

        res = glob.glob(b'*')
        self.assertEqual({type(r) for r in res}, {bytes})
        res = glob.glob(os.path.join(os.fsencode(os.curdir), b'*'))
        self.assertEqual({type(r) for r in res}, {bytes})

    def test_glob_empty_pattern(self):
        self.assertEqual(glob.glob(''), [])
        self.assertEqual(glob.glob(b''), [])
        self.assertEqual(glob.glob('', root_dir=self.tempdir), [])
        self.assertEqual(glob.glob(b'', root_dir=os.fsencode(self.tempdir)), [])
        self.assertEqual(glob.glob('', dir_fd=self.dir_fd), [])
        self.assertEqual(glob.glob(b'', dir_fd=self.dir_fd), [])

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

    def test_glob_bytes_directory_with_trailing_slash(self):
        # Same as test_glob_directory_with_trailing_slash, but with a
        # bytes argument.
        res = glob.glob(os.fsencode(self.norm('Z*Z') + os.sep))
        self.assertEqual(res, [])
        res = glob.glob(os.fsencode(self.norm('ZZZ') + os.sep))
        self.assertEqual(res, [])
        res = glob.glob(os.fsencode(self.norm('aa*') + os.sep))
        self.assertEqual(len(res), 2)
        # either of these results is reasonable
        self.assertIn(set(res), [
                      {os.fsencode(self.norm('aaa')),
                       os.fsencode(self.norm('aab'))},
                      {os.fsencode(self.norm('aaa') + os.sep),
                       os.fsencode(self.norm('aab') + os.sep)},
                      ])

    @skip_unless_symlink
    def test_glob_symlinks(self):
        eq = self.assertSequencesEqual_noorder
        eq(self.glob('sym3'), [self.norm('sym3')])
        eq(self.glob('sym3', '*'), [self.norm('sym3', 'EF'),
                                    self.norm('sym3', 'efg')])
        self.assertIn(self.glob('sym3' + os.sep),
                      [[self.norm('sym3')], [self.norm('sym3') + os.sep]])
        eq(self.glob('*', '*F'),
           [self.norm('aaa', 'zzzF'),
            self.norm('aab', 'F'), self.norm('sym3', 'EF')])

    @skip_unless_symlink
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
        eq(glob.glob(b'*:'), [])
        eq(glob.glob('?:'), [])
        eq(glob.glob(b'?:'), [])
        eq(glob.glob('\\\\?\\c:\\'), ['\\\\?\\c:\\'])
        eq(glob.glob(b'\\\\?\\c:\\'), [b'\\\\?\\c:\\'])
        eq(glob.glob('\\\\*\\*\\'), [])
        eq(glob.glob(b'\\\\*\\*\\'), [])

    def check_escape(self, arg, expected):
        self.assertEqual(glob.escape(arg), expected)
        self.assertEqual(glob.escape(os.fsencode(arg)), os.fsencode(expected))

    def test_escape(self):
        check = self.check_escape
        check('abc', 'abc')
        check('[', '[[]')
        check('?', '[?]')
        check('*', '[*]')
        check('[[_/*?*/_]]', '[[][[]_/[*][?][*]/_]]')
        check('/[[_/*?*/_]]/', '/[[][[]_/[*][?][*]/_]]/')

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific test")
    def test_escape_windows(self):
        check = self.check_escape
        check('?:?', '?:[?]')
        check('*:*', '*:[*]')
        check(r'\\?\c:\?', r'\\?\c:\[?]')
        check(r'\\*\*\*', r'\\*\*\[*]')
        check('//?/c:/?', '//?/c:/[?]')
        check('//*/*/*', '//*/*/[*]')

    def rglob(self, *parts, **kwargs):
        return self.glob(*parts, recursive=True, **kwargs)

    def test_recursive_glob(self):
        eq = self.assertSequencesEqual_noorder
        full = [('EF',), ('ZZZ',),
                ('a',), ('a', 'D'),
                ('a', 'bcd'),
                ('a', 'bcd', 'EF'),
                ('a', 'bcd', 'efg'),
                ('a', 'bcd', 'efg', 'ha'),
                ('aaa',), ('aaa', 'zzzF'),
                ('aab',), ('aab', 'F'),
               ]
        if can_symlink():
            full += [('sym1',), ('sym2',),
                     ('sym3',),
                     ('sym3', 'EF'),
                     ('sym3', 'efg'),
                     ('sym3', 'efg', 'ha'),
                    ]
        eq(self.rglob('**'), self.joins(('',), *full))
        eq(self.rglob(os.curdir, '**'),
            self.joins((os.curdir, ''), *((os.curdir,) + i for i in full)))
        dirs = [('a', ''), ('a', 'bcd', ''), ('a', 'bcd', 'efg', ''),
                ('aaa', ''), ('aab', '')]
        if can_symlink():
            dirs += [('sym3', ''), ('sym3', 'efg', '')]
        eq(self.rglob('**', ''), self.joins(('',), *dirs))

        eq(self.rglob('a', '**'), self.joins(
            ('a', ''), ('a', 'D'), ('a', 'bcd'), ('a', 'bcd', 'EF'),
            ('a', 'bcd', 'efg'), ('a', 'bcd', 'efg', 'ha')))
        eq(self.rglob('a**'), self.joins(('a',), ('aaa',), ('aab',)))
        expect = [('a', 'bcd', 'EF'), ('EF',)]
        if can_symlink():
            expect += [('sym3', 'EF')]
        eq(self.rglob('**', 'EF'), self.joins(*expect))
        expect = [('a', 'bcd', 'EF'), ('aaa', 'zzzF'), ('aab', 'F'), ('EF',)]
        if can_symlink():
            expect += [('sym3', 'EF')]
        eq(self.rglob('**', '*F'), self.joins(*expect))
        eq(self.rglob('**', '*F', ''), [])
        eq(self.rglob('**', 'bcd', '*'), self.joins(
            ('a', 'bcd', 'EF'), ('a', 'bcd', 'efg')))
        eq(self.rglob('a', '**', 'bcd'), self.joins(('a', 'bcd')))

        with change_cwd(self.tempdir):
            join = os.path.join
            eq(glob.glob('**', recursive=True), [join(*i) for i in full])
            eq(glob.glob(join('**', ''), recursive=True),
                [join(*i) for i in dirs])
            eq(glob.glob(join('**', '*'), recursive=True),
                [join(*i) for i in full])
            eq(glob.glob(join(os.curdir, '**'), recursive=True),
                [join(os.curdir, '')] + [join(os.curdir, *i) for i in full])
            eq(glob.glob(join(os.curdir, '**', ''), recursive=True),
                [join(os.curdir, '')] + [join(os.curdir, *i) for i in dirs])
            eq(glob.glob(join(os.curdir, '**', '*'), recursive=True),
                [join(os.curdir, *i) for i in full])
            eq(glob.glob(join('**','zz*F'), recursive=True),
                [join('aaa', 'zzzF')])
            eq(glob.glob('**zz*F', recursive=True), [])
            expect = [join('a', 'bcd', 'EF'), 'EF']
            if can_symlink():
                expect += [join('sym3', 'EF')]
            eq(glob.glob(join('**', 'EF'), recursive=True), expect)

    def test_glob_many_open_files(self):
        depth = 30
        base = os.path.join(self.tempdir, 'deep')
        p = os.path.join(base, *(['d']*depth))
        os.makedirs(p)
        pattern = os.path.join(base, *(['*']*depth))
        iters = [glob.iglob(pattern, recursive=True) for j in range(100)]
        for it in iters:
            self.assertEqual(next(it), p)
        pattern = os.path.join(base, '**', 'd')
        iters = [glob.iglob(pattern, recursive=True) for j in range(100)]
        p = base
        for i in range(depth):
            p = os.path.join(p, 'd')
            for it in iters:
                self.assertEqual(next(it), p)


@skip_unless_symlink
class SymlinkLoopGlobTests(unittest.TestCase):

    def test_selflink(self):
        tempdir = TESTFN + "_dir"
        os.makedirs(tempdir)
        self.addCleanup(shutil.rmtree, tempdir)
        with change_cwd(tempdir):
            os.makedirs('dir')
            create_empty_file(os.path.join('dir', 'file'))
            os.symlink(os.curdir, os.path.join('dir', 'link'))

            results = glob.glob('**', recursive=True)
            self.assertEqual(len(results), len(set(results)))
            results = set(results)
            depth = 0
            while results:
                path = os.path.join(*(['dir'] + ['link'] * depth))
                self.assertIn(path, results)
                results.remove(path)
                if not results:
                    break
                path = os.path.join(path, 'file')
                self.assertIn(path, results)
                results.remove(path)
                depth += 1

            results = glob.glob(os.path.join('**', 'file'), recursive=True)
            self.assertEqual(len(results), len(set(results)))
            results = set(results)
            depth = 0
            while results:
                path = os.path.join(*(['dir'] + ['link'] * depth + ['file']))
                self.assertIn(path, results)
                results.remove(path)
                depth += 1

            results = glob.glob(os.path.join('**', ''), recursive=True)
            self.assertEqual(len(results), len(set(results)))
            results = set(results)
            depth = 0
            while results:
                path = os.path.join(*(['dir'] + ['link'] * depth + ['']))
                self.assertIn(path, results)
                results.remove(path)
                depth += 1


if __name__ == "__main__":
    unittest.main()
