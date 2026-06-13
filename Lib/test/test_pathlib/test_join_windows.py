"""
Tests for Windows-flavoured pathlib.types._JoinablePath
"""

import os
import unittest

from .support import is_pypi
from .support.lexical_path import LexicalWindowsPath

if is_pypi:
    from pathlib_abc import vfspath
else:
    from pathlib._os import vfspath


class JoinTestBase:
    def test_join(self):
        P = self.cls
        p = P('C:/a/b')
        pp = p.joinpath('x/y')
        self.assertEqual(pp, P(r'C:/a/b\x/y'))
        pp = p.joinpath('/x/y')
        self.assertEqual(pp, P('C:/x/y'))
        # Joining with a different drive => the first path is ignored, even
        # if the second path is relative.
        pp = p.joinpath('D:x/y')
        self.assertEqual(pp, P('D:x/y'))
        pp = p.joinpath('D:/x/y')
        self.assertEqual(pp, P('D:/x/y'))
        pp = p.joinpath('//host/share/x/y')
        self.assertEqual(pp, P('//host/share/x/y'))
        # Joining with the same drive => the first path is appended to if
        # the second path is relative.
        pp = p.joinpath('c:x/y')
        self.assertEqual(pp, P(r'c:/a/b\x/y'))
        pp = p.joinpath('c:/x/y')
        self.assertEqual(pp, P('c:/x/y'))
        # Joining with files with NTFS data streams => the filename should
        # not be parsed as a drive letter
        pp = p.joinpath('./d:s')
        self.assertEqual(pp, P(r'C:/a/b\./d:s'))
        pp = p.joinpath('./dd:s')
        self.assertEqual(pp, P(r'C:/a/b\./dd:s'))
        pp = p.joinpath('E:d:s')
        self.assertEqual(pp, P('E:d:s'))
        # Joining onto a UNC path with no root
        pp = P('//server').joinpath('share')
        self.assertEqual(pp, P(r'//server\share'))
        pp = P('//./BootPartition').joinpath('Windows')
        self.assertEqual(pp, P(r'//./BootPartition\Windows'))

    def test_div(self):
        # Basically the same as joinpath().
        P = self.cls
        p = P('C:/a/b')
        self.assertEqual(p / 'x/y', P(r'C:/a/b\x/y'))
        self.assertEqual(p / 'x' / 'y', P(r'C:/a/b\x\y'))
        self.assertEqual(p / '/x/y', P('C:/x/y'))
        self.assertEqual(p / '/x' / 'y', P(r'C:/x\y'))
        # Joining with a different drive => the first path is ignored, even
        # if the second path is relative.
        self.assertEqual(p / 'D:x/y', P('D:x/y'))
        self.assertEqual(p / 'D:' / 'x/y', P('D:x/y'))
        self.assertEqual(p / 'D:/x/y', P('D:/x/y'))
        self.assertEqual(p / 'D:' / '/x/y', P('D:/x/y'))
        self.assertEqual(p / '//host/share/x/y', P('//host/share/x/y'))
        # Joining with the same drive => the first path is appended to if
        # the second path is relative.
        self.assertEqual(p / 'c:x/y', P(r'c:/a/b\x/y'))
        self.assertEqual(p / 'c:/x/y', P('c:/x/y'))
        # Joining with files with NTFS data streams => the filename should
        # not be parsed as a drive letter
        self.assertEqual(p / './d:s', P(r'C:/a/b\./d:s'))
        self.assertEqual(p / './dd:s', P(r'C:/a/b\./dd:s'))
        self.assertEqual(p / 'E:d:s', P('E:d:s'))

    def test_vfspath(self):
        p = self.cls(r'a\b\c')
        self.assertEqual(vfspath(p), 'a\\b\\c')
        p = self.cls(r'c:\a\b\c')
        self.assertEqual(vfspath(p), 'c:\\a\\b\\c')
        p = self.cls('\\\\a\\b\\')
        self.assertEqual(vfspath(p), '\\\\a\\b\\')
        p = self.cls(r'\\a\b\c')
        self.assertEqual(vfspath(p), '\\\\a\\b\\c')
        p = self.cls(r'\\a\b\c\d')
        self.assertEqual(vfspath(p), '\\\\a\\b\\c\\d')

    def test_parts(self):
        P = self.cls
        p = P(r'c:a\b')
        parts = p.parts
        self.assertEqual(parts, ('c:', 'a', 'b'))
        p = P(r'c:\a\b')
        parts = p.parts
        self.assertEqual(parts, ('c:\\', 'a', 'b'))
        p = P(r'\\a\b\c\d')
        parts = p.parts
        self.assertEqual(parts, ('\\\\a\\b\\', 'c', 'd'))

    def test_parent(self):
        # Anchored
        P = self.cls
        p = P('z:a/b/c')
        self.assertEqual(p.parent, P('z:a/b'))
        self.assertEqual(p.parent.parent, P('z:a'))
        self.assertEqual(p.parent.parent.parent, P('z:'))
        self.assertEqual(p.parent.parent.parent.parent, P('z:'))
        p = P('z:/a/b/c')
        self.assertEqual(p.parent, P('z:/a/b'))
        self.assertEqual(p.parent.parent, P('z:/a'))
        self.assertEqual(p.parent.parent.parent, P('z:/'))
        self.assertEqual(p.parent.parent.parent.parent, P('z:/'))
        p = P('//a/b/c/d')
        self.assertEqual(p.parent, P('//a/b/c'))
        self.assertEqual(p.parent.parent, P('//a/b/'))
        self.assertEqual(p.parent.parent.parent, P('//a/b/'))

    def test_parents(self):
        # Anchored
        P = self.cls
        p = P('z:a/b')
        par = p.parents
        self.assertEqual(len(par), 2)
        self.assertEqual(par[0], P('z:a'))
        self.assertEqual(par[1], P('z:'))
        self.assertEqual(par[0:1], (P('z:a'),))
        self.assertEqual(par[:-1], (P('z:a'),))
        self.assertEqual(par[:2], (P('z:a'), P('z:')))
        self.assertEqual(par[1:], (P('z:'),))
        self.assertEqual(par[::2], (P('z:a'),))
        self.assertEqual(par[::-1], (P('z:'), P('z:a')))
        self.assertEqual(list(par), [P('z:a'), P('z:')])
        with self.assertRaises(IndexError):
            par[2]
        p = P('z:/a/b')
        par = p.parents
        self.assertEqual(len(par), 2)
        self.assertEqual(par[0], P('z:/a'))
        self.assertEqual(par[1], P('z:/'))
        self.assertEqual(par[0:1], (P('z:/a'),))
        self.assertEqual(par[0:-1], (P('z:/a'),))
        self.assertEqual(par[:2], (P('z:/a'), P('z:/')))
        self.assertEqual(par[1:], (P('z:/'),))
        self.assertEqual(par[::2], (P('z:/a'),))
        self.assertEqual(par[::-1], (P('z:/'), P('z:/a'),))
        self.assertEqual(list(par), [P('z:/a'), P('z:/')])
        with self.assertRaises(IndexError):
            par[2]
        p = P('//a/b/c/d')
        par = p.parents
        self.assertEqual(len(par), 2)
        self.assertEqual(par[0], P('//a/b/c'))
        self.assertEqual(par[1], P('//a/b/'))
        self.assertEqual(par[0:1], (P('//a/b/c'),))
        self.assertEqual(par[0:-1], (P('//a/b/c'),))
        self.assertEqual(par[:2], (P('//a/b/c'), P('//a/b/')))
        self.assertEqual(par[1:], (P('//a/b/'),))
        self.assertEqual(par[::2], (P('//a/b/c'),))
        self.assertEqual(par[::-1], (P('//a/b/'), P('//a/b/c')))
        self.assertEqual(list(par), [P('//a/b/c'), P('//a/b/')])
        with self.assertRaises(IndexError):
            par[2]

    def test_anchor(self):
        P = self.cls
        self.assertEqual(P('c:').anchor, 'c:')
        self.assertEqual(P('c:a/b').anchor, 'c:')
        self.assertEqual(P('c:\\').anchor, 'c:\\')
        self.assertEqual(P('c:\\a\\b\\').anchor, 'c:\\')
        self.assertEqual(P('\\\\a\\b\\').anchor, '\\\\a\\b\\')
        self.assertEqual(P('\\\\a\\b\\c\\d').anchor, '\\\\a\\b\\')

    def test_name(self):
        P = self.cls
        self.assertEqual(P('c:').name, '')
        self.assertEqual(P('c:/').name, '')
        self.assertEqual(P('c:a/b').name, 'b')
        self.assertEqual(P('c:/a/b').name, 'b')
        self.assertEqual(P('c:a/b.py').name, 'b.py')
        self.assertEqual(P('c:/a/b.py').name, 'b.py')
        self.assertEqual(P('//My.py/Share.php').name, '')
        self.assertEqual(P('//My.py/Share.php/a/b').name, 'b')

    def test_stem(self):
        P = self.cls
        self.assertEqual(P('c:').stem, '')
        self.assertEqual(P('c:..').stem, '..')
        self.assertEqual(P('c:/').stem, '')
        self.assertEqual(P('c:a/b').stem, 'b')
        self.assertEqual(P('c:a/b.py').stem, 'b')
        self.assertEqual(P('c:a/.hgrc').stem, '.hgrc')
        self.assertEqual(P('c:a/.hg.rc').stem, '.hg')
        self.assertEqual(P('c:a/b.tar.gz').stem, 'b.tar')
        self.assertEqual(P('c:a/trailing.dot.').stem, 'trailing.dot')

    def test_suffix(self):
        P = self.cls
        self.assertEqual(P('c:').suffix, '')
        self.assertEqual(P('c:/').suffix, '')
        self.assertEqual(P('c:a/b').suffix, '')
        self.assertEqual(P('c:/a/b').suffix, '')
        self.assertEqual(P('c:a/b.py').suffix, '.py')
        self.assertEqual(P('c:/a/b.py').suffix, '.py')
        self.assertEqual(P('c:a/.hgrc').suffix, '')
        self.assertEqual(P('c:/a/.hgrc').suffix, '')
        self.assertEqual(P('c:a/.hg.rc').suffix, '.rc')
        self.assertEqual(P('c:/a/.hg.rc').suffix, '.rc')
        self.assertEqual(P('c:a/b.tar.gz').suffix, '.gz')
        self.assertEqual(P('c:/a/b.tar.gz').suffix, '.gz')
        self.assertEqual(P('c:a/trailing.dot.').suffix, '.')
        self.assertEqual(P('c:/a/trailing.dot.').suffix, '.')
        self.assertEqual(P('//My.py/Share.php').suffix, '')
        self.assertEqual(P('//My.py/Share.php/a/b').suffix, '')

    def test_suffixes(self):
        P = self.cls
        self.assertEqual(P('c:').suffixes, [])
        self.assertEqual(P('c:/').suffixes, [])
        self.assertEqual(P('c:a/b').suffixes, [])
        self.assertEqual(P('c:/a/b').suffixes, [])
        self.assertEqual(P('c:a/b.py').suffixes, ['.py'])
        self.assertEqual(P('c:/a/b.py').suffixes, ['.py'])
        self.assertEqual(P('c:a/.hgrc').suffixes, [])
        self.assertEqual(P('c:/a/.hgrc').suffixes, [])
        self.assertEqual(P('c:a/.hg.rc').suffixes, ['.rc'])
        self.assertEqual(P('c:/a/.hg.rc').suffixes, ['.rc'])
        self.assertEqual(P('c:a/b.tar.gz').suffixes, ['.tar', '.gz'])
        self.assertEqual(P('c:/a/b.tar.gz').suffixes, ['.tar', '.gz'])
        self.assertEqual(P('//My.py/Share.php').suffixes, [])
        self.assertEqual(P('//My.py/Share.php/a/b').suffixes, [])
        self.assertEqual(P('c:a/trailing.dot.').suffixes, ['.dot', '.'])
        self.assertEqual(P('c:/a/trailing.dot.').suffixes, ['.dot', '.'])

    def test_with_name(self):
        P = self.cls
        self.assertEqual(P(r'c:a\b').with_name('d.xml'), P(r'c:a\d.xml'))
        self.assertEqual(P(r'c:\a\b').with_name('d.xml'), P(r'c:\a\d.xml'))
        self.assertEqual(P(r'c:a\Dot ending.').with_name('d.xml'), P(r'c:a\d.xml'))
        self.assertEqual(P(r'c:\a\Dot ending.').with_name('d.xml'), P(r'c:\a\d.xml'))
        self.assertRaises(ValueError, P(r'c:a\b').with_name, r'd:\e')
        self.assertRaises(ValueError, P(r'c:a\b').with_name, r'\\My\Share')

    def test_with_stem(self):
        P = self.cls
        self.assertEqual(P('c:a/b').with_stem('d'), P('c:a/d'))
        self.assertEqual(P('c:/a/b').with_stem('d'), P('c:/a/d'))
        self.assertEqual(P('c:a/Dot ending.').with_stem('d'), P('c:a/d.'))
        self.assertEqual(P('c:/a/Dot ending.').with_stem('d'), P('c:/a/d.'))
        self.assertRaises(ValueError, P('c:a/b').with_stem, 'd:/e')
        self.assertRaises(ValueError, P('c:a/b').with_stem, '//My/Share')

    def test_with_suffix(self):
        P = self.cls
        self.assertEqual(P('c:a/b').with_suffix('.gz'), P('c:a/b.gz'))
        self.assertEqual(P('c:/a/b').with_suffix('.gz'), P('c:/a/b.gz'))
        self.assertEqual(P('c:a/b.py').with_suffix('.gz'), P('c:a/b.gz'))
        self.assertEqual(P('c:/a/b.py').with_suffix('.gz'), P('c:/a/b.gz'))
        # Path doesn't have a "filename" component.
        self.assertRaises(ValueError, P('').with_suffix, '.gz')
        self.assertRaises(ValueError, P('/').with_suffix, '.gz')
        self.assertRaises(ValueError, P('//My/Share').with_suffix, '.gz')
        # Invalid suffix.
        self.assertRaises(ValueError, P('c:a/b').with_suffix, 'gz')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, '/')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, '\\')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, 'c:')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, '/.gz')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, '\\.gz')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, 'c:.gz')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, 'c/d')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, 'c\\d')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, '.c/d')
        self.assertRaises(ValueError, P('c:a/b').with_suffix, '.c\\d')
        self.assertRaises(TypeError, P('c:a/b').with_suffix, None)


class LexicalWindowsPathJoinTest(JoinTestBase, unittest.TestCase):
    cls = LexicalWindowsPath


if not is_pypi:
    from pathlib import PureWindowsPath, WindowsPath

    class PureWindowsPathJoinTest(JoinTestBase, unittest.TestCase):
        cls = PureWindowsPath

    if os.name == 'nt':
        class WindowsPathJoinTest(JoinTestBase, unittest.TestCase):
            cls = WindowsPath


if __name__ == "__main__":
    unittest.main()
