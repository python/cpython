import collections
import io
import os
import errno
import stat
import unittest

from pathlib._abc import PurePathBase, PathBase
from pathlib._types import Parser
import posixpath

from test.support.os_helper import TESTFN


_tests_needing_posix = set()
_tests_needing_windows = set()


def needs_posix(fn):
    """Decorator that marks a test as requiring a POSIX-flavoured path class."""
    _tests_needing_posix.add(fn.__name__)
    return fn

def needs_windows(fn):
    """Decorator that marks a test as requiring a Windows-flavoured path class."""
    _tests_needing_windows.add(fn.__name__)
    return fn


#
# Tests for the pure classes.
#


class PurePathBaseTest(unittest.TestCase):
    cls = PurePathBase

    def test_magic_methods(self):
        P = self.cls
        self.assertFalse(hasattr(P, '__fspath__'))
        self.assertFalse(hasattr(P, '__bytes__'))
        self.assertIs(P.__reduce__, object.__reduce__)
        self.assertIs(P.__repr__, object.__repr__)
        self.assertIs(P.__hash__, object.__hash__)
        self.assertIs(P.__eq__, object.__eq__)
        self.assertIs(P.__lt__, object.__lt__)
        self.assertIs(P.__le__, object.__le__)
        self.assertIs(P.__gt__, object.__gt__)
        self.assertIs(P.__ge__, object.__ge__)

    def test_parser(self):
        self.assertIs(self.cls.parser, posixpath)


class DummyPurePath(PurePathBase):
    __slots__ = ()

    def __eq__(self, other):
        if not isinstance(other, DummyPurePath):
            return NotImplemented
        return str(self) == str(other)

    def __hash__(self):
        return hash(str(self))

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, self.as_posix())


class DummyPurePathTest(unittest.TestCase):
    cls = DummyPurePath

    # Use a base path that's unrelated to any real filesystem path.
    base = f'/this/path/kills/fascists/{TESTFN}'

    def setUp(self):
        name = self.id().split('.')[-1]
        if name in _tests_needing_posix and self.cls.parser is not posixpath:
            self.skipTest('requires POSIX-flavoured path class')
        if name in _tests_needing_windows and self.cls.parser is posixpath:
            self.skipTest('requires Windows-flavoured path class')
        p = self.cls('a')
        self.parser = p.parser
        self.sep = self.parser.sep
        self.altsep = self.parser.altsep

    def test_parser(self):
        self.assertIsInstance(self.cls.parser, Parser)

    def test_constructor_common(self):
        P = self.cls
        p = P('a')
        self.assertIsInstance(p, P)
        P()
        P('a', 'b', 'c')
        P('/a', 'b', 'c')
        P('a/b/c')
        P('/a/b/c')

    def test_bytes(self):
        P = self.cls
        with self.assertRaises(TypeError):
            P(b'a')
        with self.assertRaises(TypeError):
            P(b'a', 'b')
        with self.assertRaises(TypeError):
            P('a', b'b')
        with self.assertRaises(TypeError):
            P('a').joinpath(b'b')
        with self.assertRaises(TypeError):
            P('a') / b'b'
        with self.assertRaises(TypeError):
            b'a' / P('b')
        with self.assertRaises(TypeError):
            P('a').match(b'b')
        with self.assertRaises(TypeError):
            P('a').relative_to(b'b')
        with self.assertRaises(TypeError):
            P('a').with_name(b'b')
        with self.assertRaises(TypeError):
            P('a').with_stem(b'b')
        with self.assertRaises(TypeError):
            P('a').with_suffix(b'b')

    def _check_str_subclass(self, *args):
        # Issue #21127: it should be possible to construct a PurePath object
        # from a str subclass instance, and it then gets converted to
        # a pure str object.
        class StrSubclass(str):
            pass
        P = self.cls
        p = P(*(StrSubclass(x) for x in args))
        self.assertEqual(p, P(*args))
        for part in p.parts:
            self.assertIs(type(part), str)

    def test_str_subclass_common(self):
        self._check_str_subclass('')
        self._check_str_subclass('.')
        self._check_str_subclass('a')
        self._check_str_subclass('a/b.txt')
        self._check_str_subclass('/a/b.txt')

    @needs_windows
    def test_str_subclass_windows(self):
        self._check_str_subclass('.\\a:b')
        self._check_str_subclass('c:')
        self._check_str_subclass('c:a')
        self._check_str_subclass('c:a\\b.txt')
        self._check_str_subclass('c:\\')
        self._check_str_subclass('c:\\a')
        self._check_str_subclass('c:\\a\\b.txt')
        self._check_str_subclass('\\\\some\\share')
        self._check_str_subclass('\\\\some\\share\\a')
        self._check_str_subclass('\\\\some\\share\\a\\b.txt')

    def test_with_segments_common(self):
        class P(self.cls):
            def __init__(self, *pathsegments, session_id):
                super().__init__(*pathsegments)
                self.session_id = session_id

            def with_segments(self, *pathsegments):
                return type(self)(*pathsegments, session_id=self.session_id)
        p = P('foo', 'bar', session_id=42)
        self.assertEqual(42, (p / 'foo').session_id)
        self.assertEqual(42, ('foo' / p).session_id)
        self.assertEqual(42, p.joinpath('foo').session_id)
        self.assertEqual(42, p.with_name('foo').session_id)
        self.assertEqual(42, p.with_stem('foo').session_id)
        self.assertEqual(42, p.with_suffix('.foo').session_id)
        self.assertEqual(42, p.with_segments('foo').session_id)
        self.assertEqual(42, p.relative_to('foo').session_id)
        self.assertEqual(42, p.parent.session_id)
        for parent in p.parents:
            self.assertEqual(42, parent.session_id)

    def test_join_common(self):
        P = self.cls
        p = P('a/b')
        pp = p.joinpath('c')
        self.assertEqual(pp, P('a/b/c'))
        self.assertIs(type(pp), type(p))
        pp = p.joinpath('c', 'd')
        self.assertEqual(pp, P('a/b/c/d'))
        pp = p.joinpath('/c')
        self.assertEqual(pp, P('/c'))

    @needs_posix
    def test_join_posix(self):
        P = self.cls
        p = P('//a')
        pp = p.joinpath('b')
        self.assertEqual(pp, P('//a/b'))
        pp = P('/a').joinpath('//c')
        self.assertEqual(pp, P('//c'))
        pp = P('//a').joinpath('/c')
        self.assertEqual(pp, P('/c'))

    @needs_windows
    def test_join_windows(self):
        P = self.cls
        p = P('C:/a/b')
        pp = p.joinpath('x/y')
        self.assertEqual(pp, P('C:/a/b/x/y'))
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
        self.assertEqual(pp, P('C:/a/b/x/y'))
        pp = p.joinpath('c:/x/y')
        self.assertEqual(pp, P('C:/x/y'))
        # Joining with files with NTFS data streams => the filename should
        # not be parsed as a drive letter
        pp = p.joinpath(P('./d:s'))
        self.assertEqual(pp, P('C:/a/b/d:s'))
        pp = p.joinpath(P('./dd:s'))
        self.assertEqual(pp, P('C:/a/b/dd:s'))
        pp = p.joinpath(P('E:d:s'))
        self.assertEqual(pp, P('E:d:s'))
        # Joining onto a UNC path with no root
        pp = P('//').joinpath('server')
        self.assertEqual(pp, P('//server'))
        pp = P('//server').joinpath('share')
        self.assertEqual(pp, P('//server/share'))
        pp = P('//./BootPartition').joinpath('Windows')
        self.assertEqual(pp, P('//./BootPartition/Windows'))

    def test_div_common(self):
        # Basically the same as joinpath().
        P = self.cls
        p = P('a/b')
        pp = p / 'c'
        self.assertEqual(pp, P('a/b/c'))
        self.assertIs(type(pp), type(p))
        pp = p / 'c/d'
        self.assertEqual(pp, P('a/b/c/d'))
        pp = p / 'c' / 'd'
        self.assertEqual(pp, P('a/b/c/d'))
        pp = 'c' / p / 'd'
        self.assertEqual(pp, P('c/a/b/d'))
        pp = p/ '/c'
        self.assertEqual(pp, P('/c'))

    @needs_posix
    def test_div_posix(self):
        # Basically the same as joinpath().
        P = self.cls
        p = P('//a')
        pp = p / 'b'
        self.assertEqual(pp, P('//a/b'))
        pp = P('/a') / '//c'
        self.assertEqual(pp, P('//c'))
        pp = P('//a') / '/c'
        self.assertEqual(pp, P('/c'))

    @needs_windows
    def test_div_windows(self):
        # Basically the same as joinpath().
        P = self.cls
        p = P('C:/a/b')
        self.assertEqual(p / 'x/y', P('C:/a/b/x/y'))
        self.assertEqual(p / 'x' / 'y', P('C:/a/b/x/y'))
        self.assertEqual(p / '/x/y', P('C:/x/y'))
        self.assertEqual(p / '/x' / 'y', P('C:/x/y'))
        # Joining with a different drive => the first path is ignored, even
        # if the second path is relative.
        self.assertEqual(p / 'D:x/y', P('D:x/y'))
        self.assertEqual(p / 'D:' / 'x/y', P('D:x/y'))
        self.assertEqual(p / 'D:/x/y', P('D:/x/y'))
        self.assertEqual(p / 'D:' / '/x/y', P('D:/x/y'))
        self.assertEqual(p / '//host/share/x/y', P('//host/share/x/y'))
        # Joining with the same drive => the first path is appended to if
        # the second path is relative.
        self.assertEqual(p / 'c:x/y', P('C:/a/b/x/y'))
        self.assertEqual(p / 'c:/x/y', P('C:/x/y'))
        # Joining with files with NTFS data streams => the filename should
        # not be parsed as a drive letter
        self.assertEqual(p / P('./d:s'), P('C:/a/b/d:s'))
        self.assertEqual(p / P('./dd:s'), P('C:/a/b/dd:s'))
        self.assertEqual(p / P('E:d:s'), P('E:d:s'))

    def _check_str(self, expected, args):
        p = self.cls(*args)
        self.assertEqual(str(p), expected.replace('/', self.sep))

    def test_str_common(self):
        # Canonicalized paths roundtrip.
        for pathstr in ('a', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c'):
            self._check_str(pathstr, (pathstr,))
        # Other tests for str() are in test_equivalences().

    @needs_windows
    def test_str_windows(self):
        p = self.cls('a/b/c')
        self.assertEqual(str(p), 'a\\b\\c')
        p = self.cls('c:/a/b/c')
        self.assertEqual(str(p), 'c:\\a\\b\\c')
        p = self.cls('//a/b')
        self.assertEqual(str(p), '\\\\a\\b\\')
        p = self.cls('//a/b/c')
        self.assertEqual(str(p), '\\\\a\\b\\c')
        p = self.cls('//a/b/c/d')
        self.assertEqual(str(p), '\\\\a\\b\\c\\d')

    def test_as_posix_common(self):
        P = self.cls
        for pathstr in ('a', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c'):
            self.assertEqual(P(pathstr).as_posix(), pathstr)
        # Other tests for as_posix() are in test_equivalences().

    def test_match_empty(self):
        P = self.cls
        self.assertRaises(ValueError, P('a').match, '')

    def test_match_common(self):
        P = self.cls
        # Simple relative pattern.
        self.assertTrue(P('b.py').match('b.py'))
        self.assertTrue(P('a/b.py').match('b.py'))
        self.assertTrue(P('/a/b.py').match('b.py'))
        self.assertFalse(P('a.py').match('b.py'))
        self.assertFalse(P('b/py').match('b.py'))
        self.assertFalse(P('/a.py').match('b.py'))
        self.assertFalse(P('b.py/c').match('b.py'))
        # Wildcard relative pattern.
        self.assertTrue(P('b.py').match('*.py'))
        self.assertTrue(P('a/b.py').match('*.py'))
        self.assertTrue(P('/a/b.py').match('*.py'))
        self.assertFalse(P('b.pyc').match('*.py'))
        self.assertFalse(P('b./py').match('*.py'))
        self.assertFalse(P('b.py/c').match('*.py'))
        # Multi-part relative pattern.
        self.assertTrue(P('ab/c.py').match('a*/*.py'))
        self.assertTrue(P('/d/ab/c.py').match('a*/*.py'))
        self.assertFalse(P('a.py').match('a*/*.py'))
        self.assertFalse(P('/dab/c.py').match('a*/*.py'))
        self.assertFalse(P('ab/c.py/d').match('a*/*.py'))
        # Absolute pattern.
        self.assertTrue(P('/b.py').match('/*.py'))
        self.assertFalse(P('b.py').match('/*.py'))
        self.assertFalse(P('a/b.py').match('/*.py'))
        self.assertFalse(P('/a/b.py').match('/*.py'))
        # Multi-part absolute pattern.
        self.assertTrue(P('/a/b.py').match('/a/*.py'))
        self.assertFalse(P('/ab.py').match('/a/*.py'))
        self.assertFalse(P('/a/b/c.py').match('/a/*.py'))
        # Multi-part glob-style pattern.
        self.assertFalse(P('/a/b/c.py').match('/**/*.py'))
        self.assertTrue(P('/a/b/c.py').match('/a/**/*.py'))
        # Case-sensitive flag
        self.assertFalse(P('A.py').match('a.PY', case_sensitive=True))
        self.assertTrue(P('A.py').match('a.PY', case_sensitive=False))
        self.assertFalse(P('c:/a/B.Py').match('C:/A/*.pY', case_sensitive=True))
        self.assertTrue(P('/a/b/c.py').match('/A/*/*.Py', case_sensitive=False))
        # Matching against empty path
        self.assertFalse(P('').match('*'))
        self.assertFalse(P('').match('**'))
        self.assertFalse(P('').match('**/*'))

    @needs_posix
    def test_match_posix(self):
        P = self.cls
        self.assertFalse(P('A.py').match('a.PY'))

    @needs_windows
    def test_match_windows(self):
        P = self.cls
        # Absolute patterns.
        self.assertTrue(P('c:/b.py').match('*:/*.py'))
        self.assertTrue(P('c:/b.py').match('c:/*.py'))
        self.assertFalse(P('d:/b.py').match('c:/*.py'))  # wrong drive
        self.assertFalse(P('b.py').match('/*.py'))
        self.assertFalse(P('b.py').match('c:*.py'))
        self.assertFalse(P('b.py').match('c:/*.py'))
        self.assertFalse(P('c:b.py').match('/*.py'))
        self.assertFalse(P('c:b.py').match('c:/*.py'))
        self.assertFalse(P('/b.py').match('c:*.py'))
        self.assertFalse(P('/b.py').match('c:/*.py'))
        # UNC patterns.
        self.assertTrue(P('//some/share/a.py').match('//*/*/*.py'))
        self.assertTrue(P('//some/share/a.py').match('//some/share/*.py'))
        self.assertFalse(P('//other/share/a.py').match('//some/share/*.py'))
        self.assertFalse(P('//some/share/a/b.py').match('//some/share/*.py'))
        # Case-insensitivity.
        self.assertTrue(P('B.py').match('b.PY'))
        self.assertTrue(P('c:/a/B.Py').match('C:/A/*.pY'))
        self.assertTrue(P('//Some/Share/B.Py').match('//somE/sharE/*.pY'))
        # Path anchor doesn't match pattern anchor
        self.assertFalse(P('c:/b.py').match('/*.py'))  # 'c:/' vs '/'
        self.assertFalse(P('c:/b.py').match('c:*.py'))  # 'c:/' vs 'c:'
        self.assertFalse(P('//some/share/a.py').match('/*.py'))  # '//some/share/' vs '/'

    def test_full_match_common(self):
        P = self.cls
        # Simple relative pattern.
        self.assertTrue(P('b.py').full_match('b.py'))
        self.assertFalse(P('a/b.py').full_match('b.py'))
        self.assertFalse(P('/a/b.py').full_match('b.py'))
        self.assertFalse(P('a.py').full_match('b.py'))
        self.assertFalse(P('b/py').full_match('b.py'))
        self.assertFalse(P('/a.py').full_match('b.py'))
        self.assertFalse(P('b.py/c').full_match('b.py'))
        # Wildcard relative pattern.
        self.assertTrue(P('b.py').full_match('*.py'))
        self.assertFalse(P('a/b.py').full_match('*.py'))
        self.assertFalse(P('/a/b.py').full_match('*.py'))
        self.assertFalse(P('b.pyc').full_match('*.py'))
        self.assertFalse(P('b./py').full_match('*.py'))
        self.assertFalse(P('b.py/c').full_match('*.py'))
        # Multi-part relative pattern.
        self.assertTrue(P('ab/c.py').full_match('a*/*.py'))
        self.assertFalse(P('/d/ab/c.py').full_match('a*/*.py'))
        self.assertFalse(P('a.py').full_match('a*/*.py'))
        self.assertFalse(P('/dab/c.py').full_match('a*/*.py'))
        self.assertFalse(P('ab/c.py/d').full_match('a*/*.py'))
        # Absolute pattern.
        self.assertTrue(P('/b.py').full_match('/*.py'))
        self.assertFalse(P('b.py').full_match('/*.py'))
        self.assertFalse(P('a/b.py').full_match('/*.py'))
        self.assertFalse(P('/a/b.py').full_match('/*.py'))
        # Multi-part absolute pattern.
        self.assertTrue(P('/a/b.py').full_match('/a/*.py'))
        self.assertFalse(P('/ab.py').full_match('/a/*.py'))
        self.assertFalse(P('/a/b/c.py').full_match('/a/*.py'))
        # Multi-part glob-style pattern.
        self.assertTrue(P('a').full_match('**'))
        self.assertTrue(P('c.py').full_match('**'))
        self.assertTrue(P('a/b/c.py').full_match('**'))
        self.assertTrue(P('/a/b/c.py').full_match('**'))
        self.assertTrue(P('/a/b/c.py').full_match('/**'))
        self.assertTrue(P('/a/b/c.py').full_match('/a/**'))
        self.assertTrue(P('/a/b/c.py').full_match('**/*.py'))
        self.assertTrue(P('/a/b/c.py').full_match('/**/*.py'))
        self.assertTrue(P('/a/b/c.py').full_match('/a/**/*.py'))
        self.assertTrue(P('/a/b/c.py').full_match('/a/b/**/*.py'))
        self.assertTrue(P('/a/b/c.py').full_match('/**/**/**/**/*.py'))
        self.assertFalse(P('c.py').full_match('**/a.py'))
        self.assertFalse(P('c.py').full_match('c/**'))
        self.assertFalse(P('a/b/c.py').full_match('**/a'))
        self.assertFalse(P('a/b/c.py').full_match('**/a/b'))
        self.assertFalse(P('a/b/c.py').full_match('**/a/b/c'))
        self.assertFalse(P('a/b/c.py').full_match('**/a/b/c.'))
        self.assertFalse(P('a/b/c.py').full_match('**/a/b/c./**'))
        self.assertFalse(P('a/b/c.py').full_match('**/a/b/c./**'))
        self.assertFalse(P('a/b/c.py').full_match('/a/b/c.py/**'))
        self.assertFalse(P('a/b/c.py').full_match('/**/a/b/c.py'))
        # Case-sensitive flag
        self.assertFalse(P('A.py').full_match('a.PY', case_sensitive=True))
        self.assertTrue(P('A.py').full_match('a.PY', case_sensitive=False))
        self.assertFalse(P('c:/a/B.Py').full_match('C:/A/*.pY', case_sensitive=True))
        self.assertTrue(P('/a/b/c.py').full_match('/A/*/*.Py', case_sensitive=False))
        # Matching against empty path
        self.assertFalse(P('').full_match('*'))
        self.assertTrue(P('').full_match('**'))
        self.assertFalse(P('').full_match('**/*'))
        # Matching with empty pattern
        self.assertTrue(P('').full_match(''))
        self.assertTrue(P('.').full_match('.'))
        self.assertFalse(P('/').full_match(''))
        self.assertFalse(P('/').full_match('.'))
        self.assertFalse(P('foo').full_match(''))
        self.assertFalse(P('foo').full_match('.'))

    def test_parts_common(self):
        # `parts` returns a tuple.
        sep = self.sep
        P = self.cls
        p = P('a/b')
        parts = p.parts
        self.assertEqual(parts, ('a', 'b'))
        # When the path is absolute, the anchor is a separate part.
        p = P('/a/b')
        parts = p.parts
        self.assertEqual(parts, (sep, 'a', 'b'))

    @needs_windows
    def test_parts_windows(self):
        P = self.cls
        p = P('c:a/b')
        parts = p.parts
        self.assertEqual(parts, ('c:', 'a', 'b'))
        p = P('c:/a/b')
        parts = p.parts
        self.assertEqual(parts, ('c:\\', 'a', 'b'))
        p = P('//a/b/c/d')
        parts = p.parts
        self.assertEqual(parts, ('\\\\a\\b\\', 'c', 'd'))

    def test_parent_common(self):
        # Relative
        P = self.cls
        p = P('a/b/c')
        self.assertEqual(p.parent, P('a/b'))
        self.assertEqual(p.parent.parent, P('a'))
        self.assertEqual(p.parent.parent.parent, P(''))
        self.assertEqual(p.parent.parent.parent.parent, P(''))
        # Anchored
        p = P('/a/b/c')
        self.assertEqual(p.parent, P('/a/b'))
        self.assertEqual(p.parent.parent, P('/a'))
        self.assertEqual(p.parent.parent.parent, P('/'))
        self.assertEqual(p.parent.parent.parent.parent, P('/'))

    @needs_windows
    def test_parent_windows(self):
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
        self.assertEqual(p.parent.parent, P('//a/b'))
        self.assertEqual(p.parent.parent.parent, P('//a/b'))

    def test_parents_common(self):
        # Relative
        P = self.cls
        p = P('a/b/c')
        par = p.parents
        self.assertEqual(len(par), 3)
        self.assertEqual(par[0], P('a/b'))
        self.assertEqual(par[1], P('a'))
        self.assertEqual(par[2], P(''))
        self.assertEqual(par[-1], P(''))
        self.assertEqual(par[-2], P('a'))
        self.assertEqual(par[-3], P('a/b'))
        self.assertEqual(par[0:1], (P('a/b'),))
        self.assertEqual(par[:2], (P('a/b'), P('a')))
        self.assertEqual(par[:-1], (P('a/b'), P('a')))
        self.assertEqual(par[1:], (P('a'), P('')))
        self.assertEqual(par[::2], (P('a/b'), P('')))
        self.assertEqual(par[::-1], (P(''), P('a'), P('a/b')))
        self.assertEqual(list(par), [P('a/b'), P('a'), P('')])
        with self.assertRaises(IndexError):
            par[-4]
        with self.assertRaises(IndexError):
            par[3]
        with self.assertRaises(TypeError):
            par[0] = p
        # Anchored
        p = P('/a/b/c')
        par = p.parents
        self.assertEqual(len(par), 3)
        self.assertEqual(par[0], P('/a/b'))
        self.assertEqual(par[1], P('/a'))
        self.assertEqual(par[2], P('/'))
        self.assertEqual(par[-1], P('/'))
        self.assertEqual(par[-2], P('/a'))
        self.assertEqual(par[-3], P('/a/b'))
        self.assertEqual(par[0:1], (P('/a/b'),))
        self.assertEqual(par[:2], (P('/a/b'), P('/a')))
        self.assertEqual(par[:-1], (P('/a/b'), P('/a')))
        self.assertEqual(par[1:], (P('/a'), P('/')))
        self.assertEqual(par[::2], (P('/a/b'), P('/')))
        self.assertEqual(par[::-1], (P('/'), P('/a'), P('/a/b')))
        self.assertEqual(list(par), [P('/a/b'), P('/a'), P('/')])
        with self.assertRaises(IndexError):
            par[-4]
        with self.assertRaises(IndexError):
            par[3]

    @needs_windows
    def test_parents_windows(self):
        # Anchored
        P = self.cls
        p = P('z:a/b/')
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
        p = P('z:/a/b/')
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
        self.assertEqual(par[1], P('//a/b'))
        self.assertEqual(par[0:1], (P('//a/b/c'),))
        self.assertEqual(par[0:-1], (P('//a/b/c'),))
        self.assertEqual(par[:2], (P('//a/b/c'), P('//a/b')))
        self.assertEqual(par[1:], (P('//a/b'),))
        self.assertEqual(par[::2], (P('//a/b/c'),))
        self.assertEqual(par[::-1], (P('//a/b'), P('//a/b/c')))
        self.assertEqual(list(par), [P('//a/b/c'), P('//a/b')])
        with self.assertRaises(IndexError):
            par[2]

    def test_drive_common(self):
        P = self.cls
        self.assertEqual(P('a/b').drive, '')
        self.assertEqual(P('/a/b').drive, '')
        self.assertEqual(P('').drive, '')

    @needs_windows
    def test_drive_windows(self):
        P = self.cls
        self.assertEqual(P('c:').drive, 'c:')
        self.assertEqual(P('c:a/b').drive, 'c:')
        self.assertEqual(P('c:/').drive, 'c:')
        self.assertEqual(P('c:/a/b/').drive, 'c:')
        self.assertEqual(P('//a/b').drive, '\\\\a\\b')
        self.assertEqual(P('//a/b/').drive, '\\\\a\\b')
        self.assertEqual(P('//a/b/c/d').drive, '\\\\a\\b')
        self.assertEqual(P('./c:a').drive, '')

    def test_root_common(self):
        P = self.cls
        sep = self.sep
        self.assertEqual(P('').root, '')
        self.assertEqual(P('a/b').root, '')
        self.assertEqual(P('/').root, sep)
        self.assertEqual(P('/a/b').root, sep)

    @needs_posix
    def test_root_posix(self):
        P = self.cls
        self.assertEqual(P('/a/b').root, '/')
        # POSIX special case for two leading slashes.
        self.assertEqual(P('//a/b').root, '//')

    @needs_windows
    def test_root_windows(self):
        P = self.cls
        self.assertEqual(P('c:').root, '')
        self.assertEqual(P('c:a/b').root, '')
        self.assertEqual(P('c:/').root, '\\')
        self.assertEqual(P('c:/a/b/').root, '\\')
        self.assertEqual(P('//a/b').root, '\\')
        self.assertEqual(P('//a/b/').root, '\\')
        self.assertEqual(P('//a/b/c/d').root, '\\')

    def test_anchor_common(self):
        P = self.cls
        sep = self.sep
        self.assertEqual(P('').anchor, '')
        self.assertEqual(P('a/b').anchor, '')
        self.assertEqual(P('/').anchor, sep)
        self.assertEqual(P('/a/b').anchor, sep)

    @needs_windows
    def test_anchor_windows(self):
        P = self.cls
        self.assertEqual(P('c:').anchor, 'c:')
        self.assertEqual(P('c:a/b').anchor, 'c:')
        self.assertEqual(P('c:/').anchor, 'c:\\')
        self.assertEqual(P('c:/a/b/').anchor, 'c:\\')
        self.assertEqual(P('//a/b').anchor, '\\\\a\\b\\')
        self.assertEqual(P('//a/b/').anchor, '\\\\a\\b\\')
        self.assertEqual(P('//a/b/c/d').anchor, '\\\\a\\b\\')

    def test_name_empty(self):
        P = self.cls
        self.assertEqual(P('').name, '')
        self.assertEqual(P('.').name, '.')
        self.assertEqual(P('/a/b/.').name, '.')

    def test_name_common(self):
        P = self.cls
        self.assertEqual(P('/').name, '')
        self.assertEqual(P('a/b').name, 'b')
        self.assertEqual(P('/a/b').name, 'b')
        self.assertEqual(P('a/b.py').name, 'b.py')
        self.assertEqual(P('/a/b.py').name, 'b.py')

    @needs_windows
    def test_name_windows(self):
        P = self.cls
        self.assertEqual(P('c:').name, '')
        self.assertEqual(P('c:/').name, '')
        self.assertEqual(P('c:a/b').name, 'b')
        self.assertEqual(P('c:/a/b').name, 'b')
        self.assertEqual(P('c:a/b.py').name, 'b.py')
        self.assertEqual(P('c:/a/b.py').name, 'b.py')
        self.assertEqual(P('//My.py/Share.php').name, '')
        self.assertEqual(P('//My.py/Share.php/a/b').name, 'b')

    def test_suffix_common(self):
        P = self.cls
        self.assertEqual(P('').suffix, '')
        self.assertEqual(P('.').suffix, '')
        self.assertEqual(P('..').suffix, '')
        self.assertEqual(P('/').suffix, '')
        self.assertEqual(P('a/b').suffix, '')
        self.assertEqual(P('/a/b').suffix, '')
        self.assertEqual(P('/a/b/.').suffix, '')
        self.assertEqual(P('a/b.py').suffix, '.py')
        self.assertEqual(P('/a/b.py').suffix, '.py')
        self.assertEqual(P('a/.hgrc').suffix, '')
        self.assertEqual(P('/a/.hgrc').suffix, '')
        self.assertEqual(P('a/.hg.rc').suffix, '.rc')
        self.assertEqual(P('/a/.hg.rc').suffix, '.rc')
        self.assertEqual(P('a/b.tar.gz').suffix, '.gz')
        self.assertEqual(P('/a/b.tar.gz').suffix, '.gz')
        self.assertEqual(P('a/trailing.dot.').suffix, '.')
        self.assertEqual(P('/a/trailing.dot.').suffix, '.')
        self.assertEqual(P('a/..d.o.t..').suffix, '.')
        self.assertEqual(P('a/inn.er..dots').suffix, '.dots')
        self.assertEqual(P('photo').suffix, '')
        self.assertEqual(P('photo.jpg').suffix, '.jpg')

    @needs_windows
    def test_suffix_windows(self):
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

    def test_suffixes_common(self):
        P = self.cls
        self.assertEqual(P('').suffixes, [])
        self.assertEqual(P('.').suffixes, [])
        self.assertEqual(P('/').suffixes, [])
        self.assertEqual(P('a/b').suffixes, [])
        self.assertEqual(P('/a/b').suffixes, [])
        self.assertEqual(P('/a/b/.').suffixes, [])
        self.assertEqual(P('a/b.py').suffixes, ['.py'])
        self.assertEqual(P('/a/b.py').suffixes, ['.py'])
        self.assertEqual(P('a/.hgrc').suffixes, [])
        self.assertEqual(P('/a/.hgrc').suffixes, [])
        self.assertEqual(P('a/.hg.rc').suffixes, ['.rc'])
        self.assertEqual(P('/a/.hg.rc').suffixes, ['.rc'])
        self.assertEqual(P('a/b.tar.gz').suffixes, ['.tar', '.gz'])
        self.assertEqual(P('/a/b.tar.gz').suffixes, ['.tar', '.gz'])
        self.assertEqual(P('a/trailing.dot.').suffixes, ['.dot', '.'])
        self.assertEqual(P('/a/trailing.dot.').suffixes, ['.dot', '.'])
        self.assertEqual(P('a/..d.o.t..').suffixes, ['.o', '.t', '.', '.'])
        self.assertEqual(P('a/inn.er..dots').suffixes, ['.er', '.', '.dots'])
        self.assertEqual(P('photo').suffixes, [])
        self.assertEqual(P('photo.jpg').suffixes, ['.jpg'])

    @needs_windows
    def test_suffixes_windows(self):
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

    def test_stem_empty(self):
        P = self.cls
        self.assertEqual(P('').stem, '')
        self.assertEqual(P('.').stem, '.')

    def test_stem_common(self):
        P = self.cls
        self.assertEqual(P('..').stem, '..')
        self.assertEqual(P('/').stem, '')
        self.assertEqual(P('a/b').stem, 'b')
        self.assertEqual(P('a/b.py').stem, 'b')
        self.assertEqual(P('a/.hgrc').stem, '.hgrc')
        self.assertEqual(P('a/.hg.rc').stem, '.hg')
        self.assertEqual(P('a/b.tar.gz').stem, 'b.tar')
        self.assertEqual(P('a/trailing.dot.').stem, 'trailing.dot')
        self.assertEqual(P('a/..d.o.t..').stem, '..d.o.t.')
        self.assertEqual(P('a/inn.er..dots').stem, 'inn.er.')
        self.assertEqual(P('photo').stem, 'photo')
        self.assertEqual(P('photo.jpg').stem, 'photo')

    @needs_windows
    def test_stem_windows(self):
        P = self.cls
        self.assertEqual(P('c:').stem, '')
        self.assertEqual(P('c:.').stem, '')
        self.assertEqual(P('c:..').stem, '..')
        self.assertEqual(P('c:/').stem, '')
        self.assertEqual(P('c:a/b').stem, 'b')
        self.assertEqual(P('c:a/b.py').stem, 'b')
        self.assertEqual(P('c:a/.hgrc').stem, '.hgrc')
        self.assertEqual(P('c:a/.hg.rc').stem, '.hg')
        self.assertEqual(P('c:a/b.tar.gz').stem, 'b.tar')
        self.assertEqual(P('c:a/trailing.dot.').stem, 'trailing.dot')

    def test_with_name_common(self):
        P = self.cls
        self.assertEqual(P('a/b').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/b').with_name('d.xml'), P('/a/d.xml'))
        self.assertEqual(P('a/b.py').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/b.py').with_name('d.xml'), P('/a/d.xml'))
        self.assertEqual(P('a/Dot ending.').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/Dot ending.').with_name('d.xml'), P('/a/d.xml'))

    @needs_windows
    def test_with_name_windows(self):
        P = self.cls
        self.assertEqual(P('c:a/b').with_name('d.xml'), P('c:a/d.xml'))
        self.assertEqual(P('c:/a/b').with_name('d.xml'), P('c:/a/d.xml'))
        self.assertEqual(P('c:a/Dot ending.').with_name('d.xml'), P('c:a/d.xml'))
        self.assertEqual(P('c:/a/Dot ending.').with_name('d.xml'), P('c:/a/d.xml'))
        self.assertRaises(ValueError, P('c:').with_name, 'd.xml')
        self.assertRaises(ValueError, P('c:/').with_name, 'd.xml')
        self.assertRaises(ValueError, P('//My/Share').with_name, 'd.xml')
        self.assertEqual(str(P('a').with_name('d:')), '.\\d:')
        self.assertEqual(str(P('a').with_name('d:e')), '.\\d:e')
        self.assertEqual(P('c:a/b').with_name('d:'), P('c:a/d:'))
        self.assertEqual(P('c:a/b').with_name('d:e'), P('c:a/d:e'))
        self.assertRaises(ValueError, P('c:a/b').with_name, 'd:/e')
        self.assertRaises(ValueError, P('c:a/b').with_name, '//My/Share')

    def test_with_name_empty(self):
        P = self.cls
        self.assertEqual(P('').with_name('d.xml'), P('d.xml'))
        self.assertEqual(P('.').with_name('d.xml'), P('d.xml'))
        self.assertEqual(P('/').with_name('d.xml'), P('/d.xml'))
        self.assertEqual(P('a/b').with_name(''), P('a/'))
        self.assertEqual(P('a/b').with_name('.'), P('a/.'))

    def test_with_name_seps(self):
        P = self.cls
        self.assertRaises(ValueError, P('a/b').with_name, '/c')
        self.assertRaises(ValueError, P('a/b').with_name, 'c/')
        self.assertRaises(ValueError, P('a/b').with_name, 'c/d')

    def test_with_stem_common(self):
        P = self.cls
        self.assertEqual(P('a/b').with_stem('d'), P('a/d'))
        self.assertEqual(P('/a/b').with_stem('d'), P('/a/d'))
        self.assertEqual(P('a/b.py').with_stem('d'), P('a/d.py'))
        self.assertEqual(P('/a/b.py').with_stem('d'), P('/a/d.py'))
        self.assertEqual(P('/a/b.tar.gz').with_stem('d'), P('/a/d.gz'))
        self.assertEqual(P('a/Dot ending.').with_stem('d'), P('a/d.'))
        self.assertEqual(P('/a/Dot ending.').with_stem('d'), P('/a/d.'))

    @needs_windows
    def test_with_stem_windows(self):
        P = self.cls
        self.assertEqual(P('c:a/b').with_stem('d'), P('c:a/d'))
        self.assertEqual(P('c:/a/b').with_stem('d'), P('c:/a/d'))
        self.assertEqual(P('c:a/Dot ending.').with_stem('d'), P('c:a/d.'))
        self.assertEqual(P('c:/a/Dot ending.').with_stem('d'), P('c:/a/d.'))
        self.assertRaises(ValueError, P('c:').with_stem, 'd')
        self.assertRaises(ValueError, P('c:/').with_stem, 'd')
        self.assertRaises(ValueError, P('//My/Share').with_stem, 'd')
        self.assertEqual(str(P('a').with_stem('d:')), '.\\d:')
        self.assertEqual(str(P('a').with_stem('d:e')), '.\\d:e')
        self.assertEqual(P('c:a/b').with_stem('d:'), P('c:a/d:'))
        self.assertEqual(P('c:a/b').with_stem('d:e'), P('c:a/d:e'))
        self.assertRaises(ValueError, P('c:a/b').with_stem, 'd:/e')
        self.assertRaises(ValueError, P('c:a/b').with_stem, '//My/Share')

    def test_with_stem_empty(self):
        P = self.cls
        self.assertEqual(P('').with_stem('d'), P('d'))
        self.assertEqual(P('.').with_stem('d'), P('d'))
        self.assertEqual(P('/').with_stem('d'), P('/d'))
        self.assertEqual(P('a/b').with_stem(''), P('a/'))
        self.assertEqual(P('a/b').with_stem('.'), P('a/.'))
        self.assertRaises(ValueError, P('foo.gz').with_stem, '')
        self.assertRaises(ValueError, P('/a/b/foo.gz').with_stem, '')

    def test_with_stem_seps(self):
        P = self.cls
        self.assertRaises(ValueError, P('a/b').with_stem, '/c')
        self.assertRaises(ValueError, P('a/b').with_stem, 'c/')
        self.assertRaises(ValueError, P('a/b').with_stem, 'c/d')

    def test_with_suffix_common(self):
        P = self.cls
        self.assertEqual(P('a/b').with_suffix('.gz'), P('a/b.gz'))
        self.assertEqual(P('/a/b').with_suffix('.gz'), P('/a/b.gz'))
        self.assertEqual(P('a/b.py').with_suffix('.gz'), P('a/b.gz'))
        self.assertEqual(P('/a/b.py').with_suffix('.gz'), P('/a/b.gz'))
        # Stripping suffix.
        self.assertEqual(P('a/b.py').with_suffix(''), P('a/b'))
        self.assertEqual(P('/a/b').with_suffix(''), P('/a/b'))
        # Single dot
        self.assertEqual(P('a/b').with_suffix('.'), P('a/b.'))
        self.assertEqual(P('/a/b').with_suffix('.'), P('/a/b.'))
        self.assertEqual(P('a/b.py').with_suffix('.'), P('a/b.'))
        self.assertEqual(P('/a/b.py').with_suffix('.'), P('/a/b.'))

    @needs_windows
    def test_with_suffix_windows(self):
        P = self.cls
        self.assertEqual(P('c:a/b').with_suffix('.gz'), P('c:a/b.gz'))
        self.assertEqual(P('c:/a/b').with_suffix('.gz'), P('c:/a/b.gz'))
        self.assertEqual(P('c:a/b.py').with_suffix('.gz'), P('c:a/b.gz'))
        self.assertEqual(P('c:/a/b.py').with_suffix('.gz'), P('c:/a/b.gz'))
        # Path doesn't have a "filename" component.
        self.assertRaises(ValueError, P('').with_suffix, '.gz')
        self.assertRaises(ValueError, P('.').with_suffix, '.gz')
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

    def test_with_suffix_empty(self):
        P = self.cls
        # Path doesn't have a "filename" component.
        self.assertRaises(ValueError, P('').with_suffix, '.gz')
        self.assertRaises(ValueError, P('/').with_suffix, '.gz')

    def test_with_suffix_invalid(self):
        P = self.cls
        # Invalid suffix.
        self.assertRaises(ValueError, P('a/b').with_suffix, 'gz')
        self.assertRaises(ValueError, P('a/b').with_suffix, '/')
        self.assertRaises(ValueError, P('a/b').with_suffix, '/.gz')
        self.assertRaises(ValueError, P('a/b').with_suffix, 'c/d')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.c/.d')
        self.assertRaises(ValueError, P('a/b').with_suffix, './.d')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.d/.')
        self.assertRaises(TypeError, P('a/b').with_suffix, None)

    def test_relative_to_common(self):
        P = self.cls
        p = P('a/b')
        self.assertRaises(TypeError, p.relative_to)
        self.assertRaises(TypeError, p.relative_to, b'a')
        self.assertEqual(p.relative_to(P('')), P('a/b'))
        self.assertEqual(p.relative_to(''), P('a/b'))
        self.assertEqual(p.relative_to(P('a')), P('b'))
        self.assertEqual(p.relative_to('a'), P('b'))
        self.assertEqual(p.relative_to('a/'), P('b'))
        self.assertEqual(p.relative_to(P('a/b')), P(''))
        self.assertEqual(p.relative_to('a/b'), P(''))
        self.assertEqual(p.relative_to(P(''), walk_up=True), P('a/b'))
        self.assertEqual(p.relative_to('', walk_up=True), P('a/b'))
        self.assertEqual(p.relative_to(P('a'), walk_up=True), P('b'))
        self.assertEqual(p.relative_to('a', walk_up=True), P('b'))
        self.assertEqual(p.relative_to('a/', walk_up=True), P('b'))
        self.assertEqual(p.relative_to(P('a/b'), walk_up=True), P(''))
        self.assertEqual(p.relative_to('a/b', walk_up=True), P(''))
        self.assertEqual(p.relative_to(P('a/c'), walk_up=True), P('../b'))
        self.assertEqual(p.relative_to('a/c', walk_up=True), P('../b'))
        self.assertEqual(p.relative_to(P('a/b/c'), walk_up=True), P('..'))
        self.assertEqual(p.relative_to('a/b/c', walk_up=True), P('..'))
        self.assertEqual(p.relative_to(P('c'), walk_up=True), P('../a/b'))
        self.assertEqual(p.relative_to('c', walk_up=True), P('../a/b'))
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, P('c'))
        self.assertRaises(ValueError, p.relative_to, P('a/b/c'))
        self.assertRaises(ValueError, p.relative_to, P('a/c'))
        self.assertRaises(ValueError, p.relative_to, P('/a'))
        self.assertRaises(ValueError, p.relative_to, P("../a"))
        self.assertRaises(ValueError, p.relative_to, P("a/.."))
        self.assertRaises(ValueError, p.relative_to, P("/a/.."))
        self.assertRaises(ValueError, p.relative_to, P('/'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('/a'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P("../a"), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P("a/.."), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P("/a/.."), walk_up=True)
        p = P('/a/b')
        self.assertEqual(p.relative_to(P('/')), P('a/b'))
        self.assertEqual(p.relative_to('/'), P('a/b'))
        self.assertEqual(p.relative_to(P('/a')), P('b'))
        self.assertEqual(p.relative_to('/a'), P('b'))
        self.assertEqual(p.relative_to('/a/'), P('b'))
        self.assertEqual(p.relative_to(P('/a/b')), P(''))
        self.assertEqual(p.relative_to('/a/b'), P(''))
        self.assertEqual(p.relative_to(P('/'), walk_up=True), P('a/b'))
        self.assertEqual(p.relative_to('/', walk_up=True), P('a/b'))
        self.assertEqual(p.relative_to(P('/a'), walk_up=True), P('b'))
        self.assertEqual(p.relative_to('/a', walk_up=True), P('b'))
        self.assertEqual(p.relative_to('/a/', walk_up=True), P('b'))
        self.assertEqual(p.relative_to(P('/a/b'), walk_up=True), P(''))
        self.assertEqual(p.relative_to('/a/b', walk_up=True), P(''))
        self.assertEqual(p.relative_to(P('/a/c'), walk_up=True), P('../b'))
        self.assertEqual(p.relative_to('/a/c', walk_up=True), P('../b'))
        self.assertEqual(p.relative_to(P('/a/b/c'), walk_up=True), P('..'))
        self.assertEqual(p.relative_to('/a/b/c', walk_up=True), P('..'))
        self.assertEqual(p.relative_to(P('/c'), walk_up=True), P('../a/b'))
        self.assertEqual(p.relative_to('/c', walk_up=True), P('../a/b'))
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, P('/c'))
        self.assertRaises(ValueError, p.relative_to, P('/a/b/c'))
        self.assertRaises(ValueError, p.relative_to, P('/a/c'))
        self.assertRaises(ValueError, p.relative_to, P(''))
        self.assertRaises(ValueError, p.relative_to, '')
        self.assertRaises(ValueError, p.relative_to, P('a'))
        self.assertRaises(ValueError, p.relative_to, P("../a"))
        self.assertRaises(ValueError, p.relative_to, P("a/.."))
        self.assertRaises(ValueError, p.relative_to, P("/a/.."))
        self.assertRaises(ValueError, p.relative_to, P(''), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('a'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P("../a"), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P("a/.."), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P("/a/.."), walk_up=True)

    @needs_windows
    def test_relative_to_windows(self):
        P = self.cls
        p = P('C:Foo/Bar')
        self.assertEqual(p.relative_to(P('c:')), P('Foo/Bar'))
        self.assertEqual(p.relative_to('c:'), P('Foo/Bar'))
        self.assertEqual(p.relative_to(P('c:foO')), P('Bar'))
        self.assertEqual(p.relative_to('c:foO'), P('Bar'))
        self.assertEqual(p.relative_to('c:foO/'), P('Bar'))
        self.assertEqual(p.relative_to(P('c:foO/baR')), P())
        self.assertEqual(p.relative_to('c:foO/baR'), P())
        self.assertEqual(p.relative_to(P('c:'), walk_up=True), P('Foo/Bar'))
        self.assertEqual(p.relative_to('c:', walk_up=True), P('Foo/Bar'))
        self.assertEqual(p.relative_to(P('c:foO'), walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to('c:foO', walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to('c:foO/', walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to(P('c:foO/baR'), walk_up=True), P())
        self.assertEqual(p.relative_to('c:foO/baR', walk_up=True), P())
        self.assertEqual(p.relative_to(P('C:Foo/Bar/Baz'), walk_up=True), P('..'))
        self.assertEqual(p.relative_to(P('C:Foo/Baz'), walk_up=True), P('../Bar'))
        self.assertEqual(p.relative_to(P('C:Baz/Bar'), walk_up=True), P('../../Foo/Bar'))
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, P())
        self.assertRaises(ValueError, p.relative_to, '')
        self.assertRaises(ValueError, p.relative_to, P('d:'))
        self.assertRaises(ValueError, p.relative_to, P('/'))
        self.assertRaises(ValueError, p.relative_to, P('Foo'))
        self.assertRaises(ValueError, p.relative_to, P('/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('C:/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('C:Foo/Bar/Baz'))
        self.assertRaises(ValueError, p.relative_to, P('C:Foo/Baz'))
        self.assertRaises(ValueError, p.relative_to, P(), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, '', walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('d:'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('/'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('Foo'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('/Foo'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('C:/Foo'), walk_up=True)
        p = P('C:/Foo/Bar')
        self.assertEqual(p.relative_to(P('c:/')), P('Foo/Bar'))
        self.assertEqual(p.relative_to('c:/'), P('Foo/Bar'))
        self.assertEqual(p.relative_to(P('c:/foO')), P('Bar'))
        self.assertEqual(p.relative_to('c:/foO'), P('Bar'))
        self.assertEqual(p.relative_to('c:/foO/'), P('Bar'))
        self.assertEqual(p.relative_to(P('c:/foO/baR')), P())
        self.assertEqual(p.relative_to('c:/foO/baR'), P())
        self.assertEqual(p.relative_to(P('c:/'), walk_up=True), P('Foo/Bar'))
        self.assertEqual(p.relative_to('c:/', walk_up=True), P('Foo/Bar'))
        self.assertEqual(p.relative_to(P('c:/foO'), walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to('c:/foO', walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to('c:/foO/', walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to(P('c:/foO/baR'), walk_up=True), P())
        self.assertEqual(p.relative_to('c:/foO/baR', walk_up=True), P())
        self.assertEqual(p.relative_to('C:/Baz', walk_up=True), P('../Foo/Bar'))
        self.assertEqual(p.relative_to('C:/Foo/Bar/Baz', walk_up=True), P('..'))
        self.assertEqual(p.relative_to('C:/Foo/Baz', walk_up=True), P('../Bar'))
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, 'c:')
        self.assertRaises(ValueError, p.relative_to, P('c:'))
        self.assertRaises(ValueError, p.relative_to, P('C:/Baz'))
        self.assertRaises(ValueError, p.relative_to, P('C:/Foo/Bar/Baz'))
        self.assertRaises(ValueError, p.relative_to, P('C:/Foo/Baz'))
        self.assertRaises(ValueError, p.relative_to, P('C:Foo'))
        self.assertRaises(ValueError, p.relative_to, P('d:'))
        self.assertRaises(ValueError, p.relative_to, P('d:/'))
        self.assertRaises(ValueError, p.relative_to, P('/'))
        self.assertRaises(ValueError, p.relative_to, P('/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('//C/Foo'))
        self.assertRaises(ValueError, p.relative_to, 'c:', walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('c:'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('C:Foo'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('d:'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('d:/'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('/'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('/Foo'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('//C/Foo'), walk_up=True)
        # UNC paths.
        p = P('//Server/Share/Foo/Bar')
        self.assertEqual(p.relative_to(P('//sErver/sHare')), P('Foo/Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare'), P('Foo/Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare/'), P('Foo/Bar'))
        self.assertEqual(p.relative_to(P('//sErver/sHare/Foo')), P('Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare/Foo'), P('Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare/Foo/'), P('Bar'))
        self.assertEqual(p.relative_to(P('//sErver/sHare/Foo/Bar')), P())
        self.assertEqual(p.relative_to('//sErver/sHare/Foo/Bar'), P())
        self.assertEqual(p.relative_to(P('//sErver/sHare'), walk_up=True), P('Foo/Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare', walk_up=True), P('Foo/Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare/', walk_up=True), P('Foo/Bar'))
        self.assertEqual(p.relative_to(P('//sErver/sHare/Foo'), walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare/Foo', walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare/Foo/', walk_up=True), P('Bar'))
        self.assertEqual(p.relative_to(P('//sErver/sHare/Foo/Bar'), walk_up=True), P())
        self.assertEqual(p.relative_to('//sErver/sHare/Foo/Bar', walk_up=True), P())
        self.assertEqual(p.relative_to(P('//sErver/sHare/bar'), walk_up=True), P('../Foo/Bar'))
        self.assertEqual(p.relative_to('//sErver/sHare/bar', walk_up=True), P('../Foo/Bar'))
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, P('/Server/Share/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('c:/Server/Share/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('//z/Share/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('//Server/z/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('/Server/Share/Foo'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('c:/Server/Share/Foo'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('//z/Share/Foo'), walk_up=True)
        self.assertRaises(ValueError, p.relative_to, P('//Server/z/Foo'), walk_up=True)

    def test_is_relative_to_common(self):
        P = self.cls
        p = P('a/b')
        self.assertRaises(TypeError, p.is_relative_to)
        self.assertRaises(TypeError, p.is_relative_to, b'a')
        self.assertTrue(p.is_relative_to(P('')))
        self.assertTrue(p.is_relative_to(''))
        self.assertTrue(p.is_relative_to(P('a')))
        self.assertTrue(p.is_relative_to('a/'))
        self.assertTrue(p.is_relative_to(P('a/b')))
        self.assertTrue(p.is_relative_to('a/b'))
        # Unrelated paths.
        self.assertFalse(p.is_relative_to(P('c')))
        self.assertFalse(p.is_relative_to(P('a/b/c')))
        self.assertFalse(p.is_relative_to(P('a/c')))
        self.assertFalse(p.is_relative_to(P('/a')))
        p = P('/a/b')
        self.assertTrue(p.is_relative_to(P('/')))
        self.assertTrue(p.is_relative_to('/'))
        self.assertTrue(p.is_relative_to(P('/a')))
        self.assertTrue(p.is_relative_to('/a'))
        self.assertTrue(p.is_relative_to('/a/'))
        self.assertTrue(p.is_relative_to(P('/a/b')))
        self.assertTrue(p.is_relative_to('/a/b'))
        # Unrelated paths.
        self.assertFalse(p.is_relative_to(P('/c')))
        self.assertFalse(p.is_relative_to(P('/a/b/c')))
        self.assertFalse(p.is_relative_to(P('/a/c')))
        self.assertFalse(p.is_relative_to(P('')))
        self.assertFalse(p.is_relative_to(''))
        self.assertFalse(p.is_relative_to(P('a')))

    @needs_windows
    def test_is_relative_to_windows(self):
        P = self.cls
        p = P('C:Foo/Bar')
        self.assertTrue(p.is_relative_to(P('c:')))
        self.assertTrue(p.is_relative_to('c:'))
        self.assertTrue(p.is_relative_to(P('c:foO')))
        self.assertTrue(p.is_relative_to('c:foO'))
        self.assertTrue(p.is_relative_to('c:foO/'))
        self.assertTrue(p.is_relative_to(P('c:foO/baR')))
        self.assertTrue(p.is_relative_to('c:foO/baR'))
        # Unrelated paths.
        self.assertFalse(p.is_relative_to(P()))
        self.assertFalse(p.is_relative_to(''))
        self.assertFalse(p.is_relative_to(P('d:')))
        self.assertFalse(p.is_relative_to(P('/')))
        self.assertFalse(p.is_relative_to(P('Foo')))
        self.assertFalse(p.is_relative_to(P('/Foo')))
        self.assertFalse(p.is_relative_to(P('C:/Foo')))
        self.assertFalse(p.is_relative_to(P('C:Foo/Bar/Baz')))
        self.assertFalse(p.is_relative_to(P('C:Foo/Baz')))
        p = P('C:/Foo/Bar')
        self.assertTrue(p.is_relative_to(P('c:/')))
        self.assertTrue(p.is_relative_to(P('c:/foO')))
        self.assertTrue(p.is_relative_to('c:/foO/'))
        self.assertTrue(p.is_relative_to(P('c:/foO/baR')))
        self.assertTrue(p.is_relative_to('c:/foO/baR'))
        # Unrelated paths.
        self.assertFalse(p.is_relative_to('c:'))
        self.assertFalse(p.is_relative_to(P('C:/Baz')))
        self.assertFalse(p.is_relative_to(P('C:/Foo/Bar/Baz')))
        self.assertFalse(p.is_relative_to(P('C:/Foo/Baz')))
        self.assertFalse(p.is_relative_to(P('C:Foo')))
        self.assertFalse(p.is_relative_to(P('d:')))
        self.assertFalse(p.is_relative_to(P('d:/')))
        self.assertFalse(p.is_relative_to(P('/')))
        self.assertFalse(p.is_relative_to(P('/Foo')))
        self.assertFalse(p.is_relative_to(P('//C/Foo')))
        # UNC paths.
        p = P('//Server/Share/Foo/Bar')
        self.assertTrue(p.is_relative_to(P('//sErver/sHare')))
        self.assertTrue(p.is_relative_to('//sErver/sHare'))
        self.assertTrue(p.is_relative_to('//sErver/sHare/'))
        self.assertTrue(p.is_relative_to(P('//sErver/sHare/Foo')))
        self.assertTrue(p.is_relative_to('//sErver/sHare/Foo'))
        self.assertTrue(p.is_relative_to('//sErver/sHare/Foo/'))
        self.assertTrue(p.is_relative_to(P('//sErver/sHare/Foo/Bar')))
        self.assertTrue(p.is_relative_to('//sErver/sHare/Foo/Bar'))
        # Unrelated paths.
        self.assertFalse(p.is_relative_to(P('/Server/Share/Foo')))
        self.assertFalse(p.is_relative_to(P('c:/Server/Share/Foo')))
        self.assertFalse(p.is_relative_to(P('//z/Share/Foo')))
        self.assertFalse(p.is_relative_to(P('//Server/z/Foo')))

    @needs_posix
    def test_is_absolute_posix(self):
        P = self.cls
        self.assertFalse(P('').is_absolute())
        self.assertFalse(P('a').is_absolute())
        self.assertFalse(P('a/b/').is_absolute())
        self.assertTrue(P('/').is_absolute())
        self.assertTrue(P('/a').is_absolute())
        self.assertTrue(P('/a/b/').is_absolute())
        self.assertTrue(P('//a').is_absolute())
        self.assertTrue(P('//a/b').is_absolute())

    @needs_windows
    def test_is_absolute_windows(self):
        P = self.cls
        # Under NT, only paths with both a drive and a root are absolute.
        self.assertFalse(P().is_absolute())
        self.assertFalse(P('a').is_absolute())
        self.assertFalse(P('a/b/').is_absolute())
        self.assertFalse(P('/').is_absolute())
        self.assertFalse(P('/a').is_absolute())
        self.assertFalse(P('/a/b/').is_absolute())
        self.assertFalse(P('c:').is_absolute())
        self.assertFalse(P('c:a').is_absolute())
        self.assertFalse(P('c:a/b/').is_absolute())
        self.assertTrue(P('c:/').is_absolute())
        self.assertTrue(P('c:/a').is_absolute())
        self.assertTrue(P('c:/a/b/').is_absolute())
        # UNC paths are absolute by definition.
        self.assertTrue(P('//').is_absolute())
        self.assertTrue(P('//a').is_absolute())
        self.assertTrue(P('//a/b').is_absolute())
        self.assertTrue(P('//a/b/').is_absolute())
        self.assertTrue(P('//a/b/c').is_absolute())
        self.assertTrue(P('//a/b/c/d').is_absolute())
        self.assertTrue(P('//?/UNC/').is_absolute())
        self.assertTrue(P('//?/UNC/spam').is_absolute())


#
# Tests for the virtual classes.
#

class PathBaseTest(PurePathBaseTest):
    cls = PathBase

    def test_not_implemented_error(self):
        p = self.cls('')
        e = NotImplementedError
        self.assertRaises(e, p.stat)
        self.assertRaises(e, p.exists)
        self.assertRaises(e, p.is_dir)
        self.assertRaises(e, p.is_file)
        self.assertRaises(e, p.is_symlink)
        self.assertRaises(e, p.open)
        self.assertRaises(e, p.read_bytes)
        self.assertRaises(e, p.read_text)
        self.assertRaises(e, p.write_bytes, b'foo')
        self.assertRaises(e, p.write_text, 'foo')
        self.assertRaises(e, p.iterdir)
        self.assertRaises(e, lambda: list(p.glob('*')))
        self.assertRaises(e, lambda: list(p.rglob('*')))
        self.assertRaises(e, lambda: list(p.walk()))
        self.assertRaises(e, p.readlink)
        self.assertRaises(e, p.symlink_to, 'foo')
        self.assertRaises(e, p.mkdir)

    def test_fspath_common(self):
        self.assertRaises(TypeError, os.fspath, self.cls(''))

    def test_as_bytes_common(self):
        self.assertRaises(TypeError, bytes, self.cls(''))


class DummyPathIO(io.BytesIO):
    """
    Used by DummyPath to implement `open('w')`
    """

    def __init__(self, files, path):
        super().__init__()
        self.files = files
        self.path = path

    def close(self):
        self.files[self.path] = self.getvalue()
        super().close()


DummyPathStatResult = collections.namedtuple(
    'DummyPathStatResult',
    'st_mode st_ino st_dev st_nlink st_uid st_gid st_size st_atime st_mtime st_ctime')


class DummyPath(PathBase):
    """
    Simple implementation of PathBase that keeps files and directories in
    memory.
    """
    __slots__ = ()

    _files = {}
    _directories = {}

    def __eq__(self, other):
        if not isinstance(other, DummyPath):
            return NotImplemented
        return str(self) == str(other)

    def __hash__(self):
        return hash(str(self))

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, self.as_posix())

    def stat(self, *, follow_symlinks=True):
        path = str(self).rstrip('/')
        if path in self._files:
            st_mode = stat.S_IFREG
        elif path in self._directories:
            st_mode = stat.S_IFDIR
        else:
            raise FileNotFoundError(errno.ENOENT, "Not found", str(self))
        return DummyPathStatResult(st_mode, hash(str(self)), 0, 0, 0, 0, 0, 0, 0, 0)

    def open(self, mode='r', buffering=-1, encoding=None,
             errors=None, newline=None):
        if buffering != -1 and not (buffering == 0 and 'b' in mode):
            raise NotImplementedError
        path = str(self)
        if path in self._directories:
            raise IsADirectoryError(errno.EISDIR, "Is a directory", path)

        text = 'b' not in mode
        mode = ''.join(c for c in mode if c not in 'btU')
        if mode == 'r':
            if path not in self._files:
                raise FileNotFoundError(errno.ENOENT, "File not found", path)
            stream = io.BytesIO(self._files[path])
        elif mode == 'w':
            parent, name = posixpath.split(path)
            if parent not in self._directories:
                raise FileNotFoundError(errno.ENOENT, "File not found", parent)
            stream = DummyPathIO(self._files, path)
            self._files[path] = b''
            self._directories[parent].add(name)
        else:
            raise NotImplementedError
        if text:
            stream = io.TextIOWrapper(stream, encoding=encoding, errors=errors, newline=newline)
        return stream

    def iterdir(self):
        path = str(self).rstrip('/')
        if path in self._files:
            raise NotADirectoryError(errno.ENOTDIR, "Not a directory", path)
        elif path in self._directories:
            return iter([self / name for name in self._directories[path]])
        else:
            raise FileNotFoundError(errno.ENOENT, "File not found", path)

    def mkdir(self, mode=0o777, parents=False, exist_ok=False):
        path = str(self)
        parent = str(self.parent)
        if path in self._directories:
            if exist_ok:
                return
            else:
                raise FileExistsError(errno.EEXIST, "File exists", path)
        try:
            if self.name:
                self._directories[parent].add(self.name)
            self._directories[path] = set()
        except KeyError:
            if not parents:
                raise FileNotFoundError(errno.ENOENT, "File not found", parent) from None
            self.parent.mkdir(parents=True, exist_ok=True)
            self.mkdir(mode, parents=False, exist_ok=exist_ok)

    def _delete(self):
        path = str(self)
        if path in self._files:
            del self._files[path]
        elif path in self._directories:
            for name in list(self._directories[path]):
                self.joinpath(name)._delete()
            del self._directories[path]
        else:
            raise FileNotFoundError(errno.ENOENT, "File not found", path)
        parent = str(self.parent)
        self._directories[parent].remove(self.name)


class DummyPathTest(DummyPurePathTest):
    """Tests for PathBase methods that use stat(), open() and iterdir()."""

    cls = DummyPath
    can_symlink = False

    # (self.base)
    #  |
    #  |-- brokenLink -> non-existing
    #  |-- dirA
    #  |   `-- linkC -> ../dirB
    #  |-- dirB
    #  |   |-- fileB
    #  |   `-- linkD -> ../dirB
    #  |-- dirC
    #  |   |-- dirD
    #  |   |   `-- fileD
    #  |   `-- fileC
    #  |   `-- novel.txt
    #  |-- dirE  # No permissions
    #  |-- fileA
    #  |-- linkA -> fileA
    #  |-- linkB -> dirB
    #  `-- brokenLinkLoop -> brokenLinkLoop
    #

    def setUp(self):
        super().setUp()
        parser = self.cls.parser
        p = self.cls(self.base)
        p.mkdir(parents=True)
        p.joinpath('dirA').mkdir()
        p.joinpath('dirB').mkdir()
        p.joinpath('dirC').mkdir()
        p.joinpath('dirC', 'dirD').mkdir()
        p.joinpath('dirE').mkdir()
        with p.joinpath('fileA').open('wb') as f:
            f.write(b"this is file A\n")
        with p.joinpath('dirB', 'fileB').open('wb') as f:
            f.write(b"this is file B\n")
        with p.joinpath('dirC', 'fileC').open('wb') as f:
            f.write(b"this is file C\n")
        with p.joinpath('dirC', 'novel.txt').open('wb') as f:
            f.write(b"this is a novel\n")
        with p.joinpath('dirC', 'dirD', 'fileD').open('wb') as f:
            f.write(b"this is file D\n")
        if self.can_symlink:
            p.joinpath('linkA').symlink_to('fileA')
            p.joinpath('brokenLink').symlink_to('non-existing')
            p.joinpath('linkB').symlink_to('dirB', target_is_directory=True)
            p.joinpath('dirA', 'linkC').symlink_to(
                parser.join('..', 'dirB'), target_is_directory=True)
            p.joinpath('dirB', 'linkD').symlink_to(
                parser.join('..', 'dirB'), target_is_directory=True)
            p.joinpath('brokenLinkLoop').symlink_to('brokenLinkLoop')

    def tearDown(self):
        cls = self.cls
        cls._files.clear()
        cls._directories.clear()

    def tempdir(self):
        path = self.cls(self.base).with_name('tmp-dirD')
        path.mkdir()
        return path

    def assertFileNotFound(self, func, *args, **kwargs):
        with self.assertRaises(FileNotFoundError) as cm:
            func(*args, **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENOENT)

    def assertEqualNormCase(self, path_a, path_b):
        normcase = self.parser.normcase
        self.assertEqual(normcase(path_a), normcase(path_b))

    def test_exists(self):
        P = self.cls
        p = P(self.base)
        self.assertIs(True, p.exists())
        self.assertIs(True, (p / 'dirA').exists())
        self.assertIs(True, (p / 'fileA').exists())
        self.assertIs(False, (p / 'fileA' / 'bah').exists())
        if self.can_symlink:
            self.assertIs(True, (p / 'linkA').exists())
            self.assertIs(True, (p / 'linkB').exists())
            self.assertIs(True, (p / 'linkB' / 'fileB').exists())
            self.assertIs(False, (p / 'linkA' / 'bah').exists())
            self.assertIs(False, (p / 'brokenLink').exists())
            self.assertIs(True, (p / 'brokenLink').exists(follow_symlinks=False))
        self.assertIs(False, (p / 'foo').exists())
        self.assertIs(False, P('/xyzzy').exists())
        self.assertIs(False, P(self.base + '\udfff').exists())
        self.assertIs(False, P(self.base + '\x00').exists())

    def test_open_common(self):
        p = self.cls(self.base)
        with (p / 'fileA').open('r') as f:
            self.assertIsInstance(f, io.TextIOBase)
            self.assertEqual(f.read(), "this is file A\n")
        with (p / 'fileA').open('rb') as f:
            self.assertIsInstance(f, io.BufferedIOBase)
            self.assertEqual(f.read().strip(), b"this is file A")

    def test_read_write_bytes(self):
        p = self.cls(self.base)
        (p / 'fileA').write_bytes(b'abcdefg')
        self.assertEqual((p / 'fileA').read_bytes(), b'abcdefg')
        # Check that trying to write str does not truncate the file.
        self.assertRaises(TypeError, (p / 'fileA').write_bytes, 'somestr')
        self.assertEqual((p / 'fileA').read_bytes(), b'abcdefg')

    def test_read_write_text(self):
        p = self.cls(self.base)
        (p / 'fileA').write_text('äbcdefg', encoding='latin-1')
        self.assertEqual((p / 'fileA').read_text(
            encoding='utf-8', errors='ignore'), 'bcdefg')
        # Check that trying to write bytes does not truncate the file.
        self.assertRaises(TypeError, (p / 'fileA').write_text, b'somebytes')
        self.assertEqual((p / 'fileA').read_text(encoding='latin-1'), 'äbcdefg')

    def test_read_text_with_newlines(self):
        p = self.cls(self.base)
        # Check that `\n` character change nothing
        (p / 'fileA').write_bytes(b'abcde\r\nfghlk\n\rmnopq')
        self.assertEqual((p / 'fileA').read_text(newline='\n'),
                         'abcde\r\nfghlk\n\rmnopq')
        # Check that `\r` character replaces `\n`
        (p / 'fileA').write_bytes(b'abcde\r\nfghlk\n\rmnopq')
        self.assertEqual((p / 'fileA').read_text(newline='\r'),
                         'abcde\r\nfghlk\n\rmnopq')
        # Check that `\r\n` character replaces `\n`
        (p / 'fileA').write_bytes(b'abcde\r\nfghlk\n\rmnopq')
        self.assertEqual((p / 'fileA').read_text(newline='\r\n'),
                             'abcde\r\nfghlk\n\rmnopq')

    def test_write_text_with_newlines(self):
        p = self.cls(self.base)
        # Check that `\n` character change nothing
        (p / 'fileA').write_text('abcde\r\nfghlk\n\rmnopq', newline='\n')
        self.assertEqual((p / 'fileA').read_bytes(),
                         b'abcde\r\nfghlk\n\rmnopq')
        # Check that `\r` character replaces `\n`
        (p / 'fileA').write_text('abcde\r\nfghlk\n\rmnopq', newline='\r')
        self.assertEqual((p / 'fileA').read_bytes(),
                         b'abcde\r\rfghlk\r\rmnopq')
        # Check that `\r\n` character replaces `\n`
        (p / 'fileA').write_text('abcde\r\nfghlk\n\rmnopq', newline='\r\n')
        self.assertEqual((p / 'fileA').read_bytes(),
                         b'abcde\r\r\nfghlk\r\n\rmnopq')
        # Check that no argument passed will change `\n` to `os.linesep`
        os_linesep_byte = bytes(os.linesep, encoding='ascii')
        (p / 'fileA').write_text('abcde\nfghlk\n\rmnopq')
        self.assertEqual((p / 'fileA').read_bytes(),
                          b'abcde' + os_linesep_byte + b'fghlk' + os_linesep_byte + b'\rmnopq')

    def test_copy_file(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        target = base / 'copyA'
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(target.exists())
        self.assertEqual(source.read_text(), target.read_text())

    def test_copy_file_to_existing_file(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        target = base / 'dirB' / 'fileB'
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(target.exists())
        self.assertEqual(source.read_text(), target.read_text())

    def test_copy_file_to_existing_directory(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        target = base / 'dirA'
        self.assertRaises(OSError, source.copy, target)

    def test_copy_file_empty(self):
        base = self.cls(self.base)
        source = base / 'empty'
        target = base / 'copyA'
        source.write_bytes(b'')
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(target.exists())
        self.assertEqual(target.read_bytes(), b'')

    def test_copy_file_to_itself(self):
        base = self.cls(self.base)
        source = base / 'empty'
        source.write_bytes(b'')
        self.assertRaises(OSError, source.copy, source)
        self.assertRaises(OSError, source.copy, source, follow_symlinks=False)

    def test_copy_dir_simple(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        target = base / 'copyC'
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(target.is_dir())
        self.assertTrue(target.joinpath('dirD').is_dir())
        self.assertTrue(target.joinpath('dirD', 'fileD').is_file())
        self.assertEqual(target.joinpath('dirD', 'fileD').read_text(),
                         "this is file D\n")
        self.assertTrue(target.joinpath('fileC').is_file())
        self.assertTrue(target.joinpath('fileC').read_text(),
                        "this is file C\n")

    def test_copy_dir_complex(self, follow_symlinks=True):
        def ordered_walk(path):
            for dirpath, dirnames, filenames in path.walk(follow_symlinks=follow_symlinks):
                dirnames.sort()
                filenames.sort()
                yield dirpath, dirnames, filenames
        base = self.cls(self.base)
        source = base / 'dirC'

        if self.can_symlink:
            # Add some symlinks
            source.joinpath('linkC').symlink_to('fileC')
            source.joinpath('linkD').symlink_to('dirD', target_is_directory=True)

        # Perform the copy
        target = base / 'copyC'
        result = source.copy(target, follow_symlinks=follow_symlinks)
        self.assertEqual(result, target)

        # Compare the source and target trees
        source_walk = ordered_walk(source)
        target_walk = ordered_walk(target)
        for source_item, target_item in zip(source_walk, target_walk, strict=True):
            self.assertEqual(source_item[0].relative_to(source),
                             target_item[0].relative_to(target))  # dirpath
            self.assertEqual(source_item[1], target_item[1])  # dirnames
            self.assertEqual(source_item[2], target_item[2])  # filenames
            # Compare files and symlinks
            for filename in source_item[2]:
                source_file = source_item[0].joinpath(filename)
                target_file = target_item[0].joinpath(filename)
                if follow_symlinks or not source_file.is_symlink():
                    # Regular file.
                    self.assertEqual(source_file.read_bytes(), target_file.read_bytes())
                elif source_file.is_dir():
                    # Symlink to directory.
                    self.assertTrue(target_file.is_dir())
                    self.assertEqual(source_file.readlink(), target_file.readlink())
                else:
                    # Symlink to file.
                    self.assertEqual(source_file.read_bytes(), target_file.read_bytes())
                    self.assertEqual(source_file.readlink(), target_file.readlink())

    def test_copy_dir_complex_follow_symlinks_false(self):
        self.test_copy_dir_complex(follow_symlinks=False)

    def test_copy_dir_to_existing_directory(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        target = base / 'copyC'
        target.mkdir()
        target.joinpath('dirD').mkdir()
        self.assertRaises(FileExistsError, source.copy, target)

    def test_copy_dir_to_existing_directory_dirs_exist_ok(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        target = base / 'copyC'
        target.mkdir()
        target.joinpath('dirD').mkdir()
        result = source.copy(target, dirs_exist_ok=True)
        self.assertEqual(result, target)
        self.assertTrue(target.is_dir())
        self.assertTrue(target.joinpath('dirD').is_dir())
        self.assertTrue(target.joinpath('dirD', 'fileD').is_file())
        self.assertEqual(target.joinpath('dirD', 'fileD').read_text(),
                         "this is file D\n")
        self.assertTrue(target.joinpath('fileC').is_file())
        self.assertTrue(target.joinpath('fileC').read_text(),
                        "this is file C\n")

    def test_copy_dir_to_itself(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        self.assertRaises(OSError, source.copy, source)
        self.assertRaises(OSError, source.copy, source, follow_symlinks=False)

    def test_copy_dir_into_itself(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        target = base / 'dirC' / 'dirD' / 'copyC'
        self.assertRaises(OSError, source.copy, target)
        self.assertRaises(OSError, source.copy, target, follow_symlinks=False)
        self.assertFalse(target.exists())

    def test_copy_into(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        target_dir = base / 'dirA'
        result = source.copy_into(target_dir)
        self.assertEqual(result, target_dir / 'fileA')
        self.assertTrue(result.exists())
        self.assertEqual(source.read_text(), result.read_text())

    def test_copy_into_empty_name(self):
        source = self.cls('')
        target_dir = self.base
        self.assertRaises(ValueError, source.copy_into, target_dir)

    def test_move_file(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        source_text = source.read_text()
        target = base / 'fileA_moved'
        result = source.move(target)
        self.assertEqual(result, target)
        self.assertFalse(source.exists())
        self.assertTrue(target.exists())
        self.assertEqual(source_text, target.read_text())

    def test_move_file_to_file(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        source_text = source.read_text()
        target = base / 'dirB' / 'fileB'
        result = source.move(target)
        self.assertEqual(result, target)
        self.assertFalse(source.exists())
        self.assertTrue(target.exists())
        self.assertEqual(source_text, target.read_text())

    def test_move_file_to_dir(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        target = base / 'dirB'
        self.assertRaises(OSError, source.move, target)

    def test_move_file_to_itself(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        self.assertRaises(OSError, source.move, source)

    def test_move_dir(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        target = base / 'dirC_moved'
        result = source.move(target)
        self.assertEqual(result, target)
        self.assertFalse(source.exists())
        self.assertTrue(target.is_dir())
        self.assertTrue(target.joinpath('dirD').is_dir())
        self.assertTrue(target.joinpath('dirD', 'fileD').is_file())
        self.assertEqual(target.joinpath('dirD', 'fileD').read_text(),
                         "this is file D\n")
        self.assertTrue(target.joinpath('fileC').is_file())
        self.assertTrue(target.joinpath('fileC').read_text(),
                        "this is file C\n")

    def test_move_dir_to_dir(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        target = base / 'dirB'
        self.assertRaises(OSError, source.move, target)
        self.assertTrue(source.exists())
        self.assertTrue(target.exists())

    def test_move_dir_to_itself(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        self.assertRaises(OSError, source.move, source)
        self.assertTrue(source.exists())

    def test_move_dir_into_itself(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        target = base / 'dirC' / 'bar'
        self.assertRaises(OSError, source.move, target)
        self.assertTrue(source.exists())
        self.assertFalse(target.exists())

    def test_move_into(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        source_text = source.read_text()
        target_dir = base / 'dirA'
        result = source.move_into(target_dir)
        self.assertEqual(result, target_dir / 'fileA')
        self.assertFalse(source.exists())
        self.assertTrue(result.exists())
        self.assertEqual(source_text, result.read_text())

    def test_move_into_empty_name(self):
        source = self.cls('')
        target_dir = self.base
        self.assertRaises(ValueError, source.move_into, target_dir)

    def test_iterdir(self):
        P = self.cls
        p = P(self.base)
        it = p.iterdir()
        paths = set(it)
        expected = ['dirA', 'dirB', 'dirC', 'dirE', 'fileA']
        if self.can_symlink:
            expected += ['linkA', 'linkB', 'brokenLink', 'brokenLinkLoop']
        self.assertEqual(paths, { P(self.base, q) for q in expected })

    def test_iterdir_nodir(self):
        # __iter__ on something that is not a directory.
        p = self.cls(self.base, 'fileA')
        with self.assertRaises(OSError) as cm:
            p.iterdir()
        # ENOENT or EINVAL under Windows, ENOTDIR otherwise
        # (see issue #12802).
        self.assertIn(cm.exception.errno, (errno.ENOTDIR,
                                           errno.ENOENT, errno.EINVAL))

    def test_scandir(self):
        p = self.cls(self.base)
        with p._scandir() as entries:
            self.assertTrue(list(entries))
        with p._scandir() as entries:
            for entry in entries:
                child = p / entry.name
                self.assertIsNotNone(entry)
                self.assertEqual(entry.name, child.name)
                self.assertEqual(entry.is_symlink(),
                                 child.is_symlink())
                self.assertEqual(entry.is_dir(follow_symlinks=False),
                                 child.is_dir(follow_symlinks=False))
                if entry.name != 'brokenLinkLoop':
                    self.assertEqual(entry.is_dir(), child.is_dir())

    def test_glob_common(self):
        def _check(glob, expected):
            self.assertEqual(set(glob), { P(self.base, q) for q in expected })
        P = self.cls
        p = P(self.base)
        it = p.glob("fileA")
        self.assertIsInstance(it, collections.abc.Iterator)
        _check(it, ["fileA"])
        _check(p.glob("fileB"), [])
        _check(p.glob("dir*/file*"), ["dirB/fileB", "dirC/fileC"])
        if not self.can_symlink:
            _check(p.glob("*A"), ['dirA', 'fileA'])
        else:
            _check(p.glob("*A"), ['dirA', 'fileA', 'linkA'])
        if not self.can_symlink:
            _check(p.glob("*B/*"), ['dirB/fileB'])
        else:
            _check(p.glob("*B/*"), ['dirB/fileB', 'dirB/linkD',
                                    'linkB/fileB', 'linkB/linkD'])
        if not self.can_symlink:
            _check(p.glob("*/fileB"), ['dirB/fileB'])
        else:
            _check(p.glob("*/fileB"), ['dirB/fileB', 'linkB/fileB'])
        if self.can_symlink:
            _check(p.glob("brokenLink"), ['brokenLink'])

        if not self.can_symlink:
            _check(p.glob("*/"), ["dirA/", "dirB/", "dirC/", "dirE/"])
        else:
            _check(p.glob("*/"), ["dirA/", "dirB/", "dirC/", "dirE/", "linkB/"])

    @needs_posix
    def test_glob_posix(self):
        P = self.cls
        p = P(self.base)
        q = p / "FILEa"
        given = set(p.glob("FILEa"))
        expect = {q} if q.exists() else set()
        self.assertEqual(given, expect)
        self.assertEqual(set(p.glob("FILEa*")), set())

    @needs_windows
    def test_glob_windows(self):
        P = self.cls
        p = P(self.base)
        self.assertEqual(set(p.glob("FILEa")), { P(self.base, "fileA") })
        self.assertEqual(set(p.glob("*a\\")), { P(self.base, "dirA/") })
        self.assertEqual(set(p.glob("F*a")), { P(self.base, "fileA") })

    def test_glob_empty_pattern(self):
        P = self.cls
        p = P(self.base)
        self.assertEqual(list(p.glob("")), [p])

    def test_glob_case_sensitive(self):
        P = self.cls
        def _check(path, pattern, case_sensitive, expected):
            actual = {str(q) for q in path.glob(pattern, case_sensitive=case_sensitive)}
            expected = {str(P(self.base, q)) for q in expected}
            self.assertEqual(actual, expected)
        path = P(self.base)
        _check(path, "DIRB/FILE*", True, [])
        _check(path, "DIRB/FILE*", False, ["dirB/fileB"])
        _check(path, "dirb/file*", True, [])
        _check(path, "dirb/file*", False, ["dirB/fileB"])

    def test_rglob_recurse_symlinks_false(self):
        def _check(path, glob, expected):
            actual = set(path.rglob(glob, recurse_symlinks=False))
            self.assertEqual(actual, { P(self.base, q) for q in expected })
        P = self.cls
        p = P(self.base)
        it = p.rglob("fileA")
        self.assertIsInstance(it, collections.abc.Iterator)
        _check(p, "fileA", ["fileA"])
        _check(p, "fileB", ["dirB/fileB"])
        _check(p, "**/fileB", ["dirB/fileB"])
        _check(p, "*/fileA", [])

        if self.can_symlink:
            _check(p, "*/fileB", ["dirB/fileB", "dirB/linkD/fileB",
                                  "linkB/fileB", "dirA/linkC/fileB"])
            _check(p, "*/", [
                "dirA/", "dirA/linkC/", "dirB/", "dirB/linkD/", "dirC/",
                "dirC/dirD/", "dirE/", "linkB/"])
        else:
            _check(p, "*/fileB", ["dirB/fileB"])
            _check(p, "*/", ["dirA/", "dirB/", "dirC/", "dirC/dirD/", "dirE/"])

        _check(p, "file*", ["fileA", "dirB/fileB", "dirC/fileC", "dirC/dirD/fileD"])
        _check(p, "", ["", "dirA/", "dirB/", "dirC/", "dirE/", "dirC/dirD/"])
        p = P(self.base, "dirC")
        _check(p, "*", ["dirC/fileC", "dirC/novel.txt",
                              "dirC/dirD", "dirC/dirD/fileD"])
        _check(p, "file*", ["dirC/fileC", "dirC/dirD/fileD"])
        _check(p, "**/file*", ["dirC/fileC", "dirC/dirD/fileD"])
        _check(p, "dir*/**", ["dirC/dirD/", "dirC/dirD/fileD"])
        _check(p, "dir*/**/", ["dirC/dirD/"])
        _check(p, "*/*", ["dirC/dirD/fileD"])
        _check(p, "*/", ["dirC/dirD/"])
        _check(p, "", ["dirC/", "dirC/dirD/"])
        _check(p, "**", ["dirC/", "dirC/fileC", "dirC/dirD", "dirC/dirD/fileD", "dirC/novel.txt"])
        _check(p, "**/", ["dirC/", "dirC/dirD/"])
        # gh-91616, a re module regression
        _check(p, "*.txt", ["dirC/novel.txt"])
        _check(p, "*.*", ["dirC/novel.txt"])

    @needs_posix
    def test_rglob_posix(self):
        P = self.cls
        p = P(self.base, "dirC")
        q = p / "dirD" / "FILEd"
        given = set(p.rglob("FILEd"))
        expect = {q} if q.exists() else set()
        self.assertEqual(given, expect)
        self.assertEqual(set(p.rglob("FILEd*")), set())

    @needs_windows
    def test_rglob_windows(self):
        P = self.cls
        p = P(self.base, "dirC")
        self.assertEqual(set(p.rglob("FILEd")), { P(self.base, "dirC/dirD/fileD") })
        self.assertEqual(set(p.rglob("*\\")), { P(self.base, "dirC/dirD/") })

    def test_stat(self):
        statA = self.cls(self.base).joinpath('fileA').stat()
        statB = self.cls(self.base).joinpath('dirB', 'fileB').stat()
        statC = self.cls(self.base).joinpath('dirC').stat()
        # st_mode: files are the same, directory differs.
        self.assertIsInstance(statA.st_mode, int)
        self.assertEqual(statA.st_mode, statB.st_mode)
        self.assertNotEqual(statA.st_mode, statC.st_mode)
        self.assertNotEqual(statB.st_mode, statC.st_mode)
        # st_ino: all different,
        self.assertIsInstance(statA.st_ino, int)
        self.assertNotEqual(statA.st_ino, statB.st_ino)
        self.assertNotEqual(statA.st_ino, statC.st_ino)
        self.assertNotEqual(statB.st_ino, statC.st_ino)
        # st_dev: all the same.
        self.assertIsInstance(statA.st_dev, int)
        self.assertEqual(statA.st_dev, statB.st_dev)
        self.assertEqual(statA.st_dev, statC.st_dev)
        # other attributes not used by pathlib.

    def test_stat_no_follow_symlinks_nosymlink(self):
        p = self.cls(self.base) / 'fileA'
        st = p.stat()
        self.assertEqual(st, p.stat(follow_symlinks=False))

    def test_is_dir(self):
        P = self.cls(self.base)
        self.assertTrue((P / 'dirA').is_dir())
        self.assertFalse((P / 'fileA').is_dir())
        self.assertFalse((P / 'non-existing').is_dir())
        self.assertFalse((P / 'fileA' / 'bah').is_dir())
        if self.can_symlink:
            self.assertFalse((P / 'linkA').is_dir())
            self.assertTrue((P / 'linkB').is_dir())
            self.assertFalse((P/ 'brokenLink').is_dir())
        self.assertFalse((P / 'dirA\udfff').is_dir())
        self.assertFalse((P / 'dirA\x00').is_dir())

    def test_is_dir_no_follow_symlinks(self):
        P = self.cls(self.base)
        self.assertTrue((P / 'dirA').is_dir(follow_symlinks=False))
        self.assertFalse((P / 'fileA').is_dir(follow_symlinks=False))
        self.assertFalse((P / 'non-existing').is_dir(follow_symlinks=False))
        self.assertFalse((P / 'fileA' / 'bah').is_dir(follow_symlinks=False))
        if self.can_symlink:
            self.assertFalse((P / 'linkA').is_dir(follow_symlinks=False))
            self.assertFalse((P / 'linkB').is_dir(follow_symlinks=False))
            self.assertFalse((P/ 'brokenLink').is_dir(follow_symlinks=False))
        self.assertFalse((P / 'dirA\udfff').is_dir(follow_symlinks=False))
        self.assertFalse((P / 'dirA\x00').is_dir(follow_symlinks=False))

    def test_is_file(self):
        P = self.cls(self.base)
        self.assertTrue((P / 'fileA').is_file())
        self.assertFalse((P / 'dirA').is_file())
        self.assertFalse((P / 'non-existing').is_file())
        self.assertFalse((P / 'fileA' / 'bah').is_file())
        if self.can_symlink:
            self.assertTrue((P / 'linkA').is_file())
            self.assertFalse((P / 'linkB').is_file())
            self.assertFalse((P/ 'brokenLink').is_file())
        self.assertFalse((P / 'fileA\udfff').is_file())
        self.assertFalse((P / 'fileA\x00').is_file())

    def test_is_file_no_follow_symlinks(self):
        P = self.cls(self.base)
        self.assertTrue((P / 'fileA').is_file(follow_symlinks=False))
        self.assertFalse((P / 'dirA').is_file(follow_symlinks=False))
        self.assertFalse((P / 'non-existing').is_file(follow_symlinks=False))
        self.assertFalse((P / 'fileA' / 'bah').is_file(follow_symlinks=False))
        if self.can_symlink:
            self.assertFalse((P / 'linkA').is_file(follow_symlinks=False))
            self.assertFalse((P / 'linkB').is_file(follow_symlinks=False))
            self.assertFalse((P/ 'brokenLink').is_file(follow_symlinks=False))
        self.assertFalse((P / 'fileA\udfff').is_file(follow_symlinks=False))
        self.assertFalse((P / 'fileA\x00').is_file(follow_symlinks=False))

    def test_is_symlink(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_symlink())
        self.assertFalse((P / 'dirA').is_symlink())
        self.assertFalse((P / 'non-existing').is_symlink())
        self.assertFalse((P / 'fileA' / 'bah').is_symlink())
        if self.can_symlink:
            self.assertTrue((P / 'linkA').is_symlink())
            self.assertTrue((P / 'linkB').is_symlink())
            self.assertTrue((P/ 'brokenLink').is_symlink())
        self.assertIs((P / 'fileA\udfff').is_file(), False)
        self.assertIs((P / 'fileA\x00').is_file(), False)
        if self.can_symlink:
            self.assertIs((P / 'linkA\udfff').is_file(), False)
            self.assertIs((P / 'linkA\x00').is_file(), False)

    def test_delete_file(self):
        p = self.cls(self.base) / 'fileA'
        p._delete()
        self.assertFileNotFound(p.stat)
        self.assertFileNotFound(p._delete)

    def test_delete_dir(self):
        base = self.cls(self.base)
        base.joinpath('dirA')._delete()
        self.assertRaises(FileNotFoundError, base.joinpath('dirA').stat)
        self.assertRaises(FileNotFoundError, base.joinpath('dirA', 'linkC').stat,
                          follow_symlinks=False)
        base.joinpath('dirB')._delete()
        self.assertRaises(FileNotFoundError, base.joinpath('dirB').stat)
        self.assertRaises(FileNotFoundError, base.joinpath('dirB', 'fileB').stat)
        self.assertRaises(FileNotFoundError, base.joinpath('dirB', 'linkD').stat,
                          follow_symlinks=False)
        base.joinpath('dirC')._delete()
        self.assertRaises(FileNotFoundError, base.joinpath('dirC').stat)
        self.assertRaises(FileNotFoundError, base.joinpath('dirC', 'dirD').stat)
        self.assertRaises(FileNotFoundError, base.joinpath('dirC', 'dirD', 'fileD').stat)
        self.assertRaises(FileNotFoundError, base.joinpath('dirC', 'fileC').stat)
        self.assertRaises(FileNotFoundError, base.joinpath('dirC', 'novel.txt').stat)

    def test_delete_missing(self):
        tmp = self.cls(self.base, 'delete')
        tmp.mkdir()
        # filename is guaranteed not to exist
        filename = tmp / 'foo'
        self.assertRaises(FileNotFoundError, filename._delete)


class DummyPathWalkTest(unittest.TestCase):
    cls = DummyPath
    base = DummyPathTest.base
    can_symlink = False

    def setUp(self):
        # Build:
        #     TESTFN/
        #       TEST1/              a file kid and two directory kids
        #         tmp1
        #         SUB1/             a file kid and a directory kid
        #           tmp2
        #           SUB11/          no kids
        #         SUB2/             a file kid and a dirsymlink kid
        #           tmp3
        #           link/           a symlink to TEST2
        #           broken_link
        #           broken_link2
        #       TEST2/
        #         tmp4              a lone file
        self.walk_path = self.cls(self.base, "TEST1")
        self.sub1_path = self.walk_path / "SUB1"
        self.sub11_path = self.sub1_path / "SUB11"
        self.sub2_path = self.walk_path / "SUB2"
        tmp1_path = self.walk_path / "tmp1"
        tmp2_path = self.sub1_path / "tmp2"
        tmp3_path = self.sub2_path / "tmp3"
        self.link_path = self.sub2_path / "link"
        t2_path = self.cls(self.base, "TEST2")
        tmp4_path = self.cls(self.base, "TEST2", "tmp4")
        broken_link_path = self.sub2_path / "broken_link"
        broken_link2_path = self.sub2_path / "broken_link2"

        self.sub11_path.mkdir(parents=True)
        self.sub2_path.mkdir(parents=True)
        t2_path.mkdir(parents=True)

        for path in tmp1_path, tmp2_path, tmp3_path, tmp4_path:
            with path.open("w", encoding='utf-8') as f:
                f.write(f"I'm {path} and proud of it.  Blame test_pathlib.\n")

        if self.can_symlink:
            self.link_path.symlink_to(t2_path, target_is_directory=True)
            broken_link_path.symlink_to('broken')
            broken_link2_path.symlink_to(self.cls('tmp3', 'broken'))
            self.sub2_tree = (self.sub2_path, [], ["broken_link", "broken_link2", "link", "tmp3"])
        else:
            self.sub2_tree = (self.sub2_path, [], ["tmp3"])

    def tearDown(self):
        base = self.cls(self.base)
        base._delete()

    def test_walk_topdown(self):
        walker = self.walk_path.walk()
        entry = next(walker)
        entry[1].sort()  # Ensure we visit SUB1 before SUB2
        self.assertEqual(entry, (self.walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        entry = next(walker)
        self.assertEqual(entry, (self.sub1_path, ["SUB11"], ["tmp2"]))
        entry = next(walker)
        self.assertEqual(entry, (self.sub11_path, [], []))
        entry = next(walker)
        entry[1].sort()
        entry[2].sort()
        self.assertEqual(entry, self.sub2_tree)
        with self.assertRaises(StopIteration):
            next(walker)

    def test_walk_prune(self):
        # Prune the search.
        all = []
        for root, dirs, files in self.walk_path.walk():
            all.append((root, dirs, files))
            if 'SUB1' in dirs:
                # Note that this also mutates the dirs we appended to all!
                dirs.remove('SUB1')

        self.assertEqual(len(all), 2)
        self.assertEqual(all[0], (self.walk_path, ["SUB2"], ["tmp1"]))

        all[1][-1].sort()
        all[1][1].sort()
        self.assertEqual(all[1], self.sub2_tree)

    def test_walk_bottom_up(self):
        seen_testfn = seen_sub1 = seen_sub11 = seen_sub2 = False
        for path, dirnames, filenames in self.walk_path.walk(top_down=False):
            if path == self.walk_path:
                self.assertFalse(seen_testfn)
                self.assertTrue(seen_sub1)
                self.assertTrue(seen_sub2)
                self.assertEqual(sorted(dirnames), ["SUB1", "SUB2"])
                self.assertEqual(filenames, ["tmp1"])
                seen_testfn = True
            elif path == self.sub1_path:
                self.assertFalse(seen_testfn)
                self.assertFalse(seen_sub1)
                self.assertTrue(seen_sub11)
                self.assertEqual(dirnames, ["SUB11"])
                self.assertEqual(filenames, ["tmp2"])
                seen_sub1 = True
            elif path == self.sub11_path:
                self.assertFalse(seen_sub1)
                self.assertFalse(seen_sub11)
                self.assertEqual(dirnames, [])
                self.assertEqual(filenames, [])
                seen_sub11 = True
            elif path == self.sub2_path:
                self.assertFalse(seen_testfn)
                self.assertFalse(seen_sub2)
                self.assertEqual(sorted(dirnames), sorted(self.sub2_tree[1]))
                self.assertEqual(sorted(filenames), sorted(self.sub2_tree[2]))
                seen_sub2 = True
            else:
                raise AssertionError(f"Unexpected path: {path}")
        self.assertTrue(seen_testfn)



if __name__ == "__main__":
    unittest.main()
