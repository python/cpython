import collections
import io
import os
import errno
import stat
import unittest

from pathlib._abc import UnsupportedOperation, PurePathBase, PathBase
import posixpath

from test.support.os_helper import TESTFN


class UnsupportedOperationTest(unittest.TestCase):
    def test_is_notimplemented(self):
        self.assertTrue(issubclass(UnsupportedOperation, NotImplementedError))
        self.assertTrue(isinstance(UnsupportedOperation(), NotImplementedError))


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

    def test_pathmod(self):
        self.assertIs(self.cls.pathmod, posixpath)


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
        p = self.cls('a')
        self.pathmod = p.pathmod
        self.sep = self.pathmod.sep
        self.altsep = self.pathmod.altsep

    def test_constructor_common(self):
        P = self.cls
        p = P('a')
        self.assertIsInstance(p, P)
        P('a', 'b', 'c')
        P('/a', 'b', 'c')
        P('a/b/c')
        P('/a/b/c')

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

    def _check_str(self, expected, args):
        p = self.cls(*args)
        self.assertEqual(str(p), expected.replace('/', self.sep))

    def test_str_common(self):
        # Canonicalized paths roundtrip.
        for pathstr in ('a', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c'):
            self._check_str(pathstr, (pathstr,))
        # Other tests for str() are in test_equivalences().

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
        self.assertTrue(P('a').match('**'))
        self.assertTrue(P('c.py').match('**'))
        self.assertTrue(P('a/b/c.py').match('**'))
        self.assertTrue(P('/a/b/c.py').match('**'))
        self.assertTrue(P('/a/b/c.py').match('/**'))
        self.assertTrue(P('/a/b/c.py').match('/a/**'))
        self.assertTrue(P('/a/b/c.py').match('**/*.py'))
        self.assertTrue(P('/a/b/c.py').match('/**/*.py'))
        self.assertTrue(P('/a/b/c.py').match('/a/**/*.py'))
        self.assertTrue(P('/a/b/c.py').match('/a/b/**/*.py'))
        self.assertTrue(P('/a/b/c.py').match('/**/**/**/**/*.py'))
        self.assertFalse(P('c.py').match('**/a.py'))
        self.assertFalse(P('c.py').match('c/**'))
        self.assertFalse(P('a/b/c.py').match('**/a'))
        self.assertFalse(P('a/b/c.py').match('**/a/b'))
        self.assertFalse(P('a/b/c.py').match('**/a/b/c'))
        self.assertFalse(P('a/b/c.py').match('**/a/b/c.'))
        self.assertFalse(P('a/b/c.py').match('**/a/b/c./**'))
        self.assertFalse(P('a/b/c.py').match('**/a/b/c./**'))
        self.assertFalse(P('a/b/c.py').match('/a/b/c.py/**'))
        self.assertFalse(P('a/b/c.py').match('/**/a/b/c.py'))
        self.assertRaises(ValueError, P('a').match, '**a/b/c')
        self.assertRaises(ValueError, P('a').match, 'a/b/c**')
        # Case-sensitive flag
        self.assertFalse(P('A.py').match('a.PY', case_sensitive=True))
        self.assertTrue(P('A.py').match('a.PY', case_sensitive=False))
        self.assertFalse(P('c:/a/B.Py').match('C:/A/*.pY', case_sensitive=True))
        self.assertTrue(P('/a/b/c.py').match('/A/*/*.Py', case_sensitive=False))
        # Matching against empty path
        self.assertFalse(P('').match('*'))
        self.assertTrue(P('').match('**'))
        self.assertFalse(P('').match('**/*'))

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

    def test_drive_common(self):
        P = self.cls
        self.assertEqual(P('a/b').drive, '')
        self.assertEqual(P('/a/b').drive, '')
        self.assertEqual(P('').drive, '')

    def test_root_common(self):
        P = self.cls
        sep = self.sep
        self.assertEqual(P('').root, '')
        self.assertEqual(P('a/b').root, '')
        self.assertEqual(P('/').root, sep)
        self.assertEqual(P('/a/b').root, sep)

    def test_anchor_common(self):
        P = self.cls
        sep = self.sep
        self.assertEqual(P('').anchor, '')
        self.assertEqual(P('a/b').anchor, '')
        self.assertEqual(P('/').anchor, sep)
        self.assertEqual(P('/a/b').anchor, sep)

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
        self.assertEqual(P('a/Some name. Ending with a dot.').suffix, '')
        self.assertEqual(P('/a/Some name. Ending with a dot.').suffix, '')

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
        self.assertEqual(P('a/Some name. Ending with a dot.').suffixes, [])
        self.assertEqual(P('/a/Some name. Ending with a dot.').suffixes, [])

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
        self.assertEqual(P('a/Some name. Ending with a dot.').stem,
                         'Some name. Ending with a dot.')

    def test_with_name_common(self):
        P = self.cls
        self.assertEqual(P('a/b').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/b').with_name('d.xml'), P('/a/d.xml'))
        self.assertEqual(P('a/b.py').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/b.py').with_name('d.xml'), P('/a/d.xml'))
        self.assertEqual(P('a/Dot ending.').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/Dot ending.').with_name('d.xml'), P('/a/d.xml'))

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
        self.assertEqual(P('a/Dot ending.').with_stem('d'), P('a/d'))
        self.assertEqual(P('/a/Dot ending.').with_stem('d'), P('/a/d'))

    def test_with_stem_empty(self):
        P = self.cls
        self.assertEqual(P('').with_stem('d'), P('d'))
        self.assertEqual(P('.').with_stem('d'), P('d'))
        self.assertEqual(P('/').with_stem('d'), P('/d'))
        self.assertEqual(P('a/b').with_stem(''), P('a/'))
        self.assertEqual(P('a/b').with_stem('.'), P('a/.'))

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

    def test_with_suffix_empty(self):
        P = self.cls
        # Path doesn't have a "filename" component.
        self.assertEqual(P('').with_suffix('.gz'), P('.gz'))
        self.assertEqual(P('.').with_suffix('.gz'), P('..gz'))
        self.assertEqual(P('/').with_suffix('.gz'), P('/.gz'))

    def test_with_suffix_seps(self):
        P = self.cls
        # Invalid suffix.
        self.assertRaises(ValueError, P('a/b').with_suffix, 'gz')
        self.assertRaises(ValueError, P('a/b').with_suffix, '/')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.')
        self.assertRaises(ValueError, P('a/b').with_suffix, '/.gz')
        self.assertRaises(ValueError, P('a/b').with_suffix, 'c/d')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.c/.d')
        self.assertRaises(ValueError, P('a/b').with_suffix, './.d')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.d/.')

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


#
# Tests for the virtual classes.
#

class PathBaseTest(PurePathBaseTest):
    cls = PathBase

    def test_unsupported_operation(self):
        P = self.cls
        p = self.cls('')
        e = UnsupportedOperation
        self.assertRaises(e, p.stat)
        self.assertRaises(e, p.lstat)
        self.assertRaises(e, p.exists)
        self.assertRaises(e, p.samefile, 'foo')
        self.assertRaises(e, p.is_dir)
        self.assertRaises(e, p.is_file)
        self.assertRaises(e, p.is_mount)
        self.assertRaises(e, p.is_symlink)
        self.assertRaises(e, p.is_block_device)
        self.assertRaises(e, p.is_char_device)
        self.assertRaises(e, p.is_fifo)
        self.assertRaises(e, p.is_socket)
        self.assertRaises(e, p.open)
        self.assertRaises(e, p.read_bytes)
        self.assertRaises(e, p.read_text)
        self.assertRaises(e, p.write_bytes, b'foo')
        self.assertRaises(e, p.write_text, 'foo')
        self.assertRaises(e, p.iterdir)
        self.assertRaises(e, p.glob, '*')
        self.assertRaises(e, p.rglob, '*')
        self.assertRaises(e, lambda: list(p.walk()))
        self.assertRaises(e, p.absolute)
        self.assertRaises(e, P.cwd)
        self.assertRaises(e, p.expanduser)
        self.assertRaises(e, p.home)
        self.assertRaises(e, p.readlink)
        self.assertRaises(e, p.symlink_to, 'foo')
        self.assertRaises(e, p.hardlink_to, 'foo')
        self.assertRaises(e, p.mkdir)
        self.assertRaises(e, p.touch)
        self.assertRaises(e, p.rename, 'foo')
        self.assertRaises(e, p.replace, 'foo')
        self.assertRaises(e, p.chmod, 0o755)
        self.assertRaises(e, p.lchmod, 0o755)
        self.assertRaises(e, p.unlink)
        self.assertRaises(e, p.rmdir)
        self.assertRaises(e, p.owner)
        self.assertRaises(e, p.group)
        self.assertRaises(e, p.as_uri)

    def test_as_uri_common(self):
        e = UnsupportedOperation
        self.assertRaises(e, self.cls('').as_uri)

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
    _symlinks = {}

    def __eq__(self, other):
        if not isinstance(other, DummyPath):
            return NotImplemented
        return str(self) == str(other)

    def __hash__(self):
        return hash(str(self))

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, self.as_posix())

    def stat(self, *, follow_symlinks=True):
        if follow_symlinks:
            path = str(self.resolve())
        else:
            path = str(self.parent.resolve() / self.name)
        if path in self._files:
            st_mode = stat.S_IFREG
        elif path in self._directories:
            st_mode = stat.S_IFDIR
        elif path in self._symlinks:
            st_mode = stat.S_IFLNK
        else:
            raise FileNotFoundError(errno.ENOENT, "Not found", str(self))
        return DummyPathStatResult(st_mode, hash(str(self)), 0, 0, 0, 0, 0, 0, 0, 0)

    def open(self, mode='r', buffering=-1, encoding=None,
             errors=None, newline=None):
        if buffering != -1:
            raise NotImplementedError
        path_obj = self.resolve()
        path = str(path_obj)
        name = path_obj.name
        parent = str(path_obj.parent)
        if path in self._directories:
            raise IsADirectoryError(errno.EISDIR, "Is a directory", path)

        text = 'b' not in mode
        mode = ''.join(c for c in mode if c not in 'btU')
        if mode == 'r':
            if path not in self._files:
                raise FileNotFoundError(errno.ENOENT, "File not found", path)
            stream = io.BytesIO(self._files[path])
        elif mode == 'w':
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
        path = str(self.resolve())
        if path in self._files:
            raise NotADirectoryError(errno.ENOTDIR, "Not a directory", path)
        elif path in self._directories:
            return (self / name for name in self._directories[path])
        else:
            raise FileNotFoundError(errno.ENOENT, "File not found", path)

    def mkdir(self, mode=0o777, parents=False, exist_ok=False):
        path = str(self.resolve())
        if path in self._directories:
            if exist_ok:
                return
            else:
                raise FileExistsError(errno.EEXIST, "File exists", path)
        try:
            if self.name:
                self._directories[str(self.parent)].add(self.name)
            self._directories[path] = set()
        except KeyError:
            if not parents:
                raise FileNotFoundError(errno.ENOENT, "File not found", str(self.parent)) from None
            self.parent.mkdir(parents=True, exist_ok=True)
            self.mkdir(mode, parents=False, exist_ok=exist_ok)


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
        pathmod = self.cls.pathmod
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
            p.joinpath('linkB').symlink_to('dirB')
            p.joinpath('dirA', 'linkC').symlink_to(pathmod.join('..', 'dirB'))
            p.joinpath('dirB', 'linkD').symlink_to(pathmod.join('..', 'dirB'))
            p.joinpath('brokenLinkLoop').symlink_to('brokenLinkLoop')

    def tearDown(self):
        cls = self.cls
        cls._files.clear()
        cls._directories.clear()
        cls._symlinks.clear()

    def tempdir(self):
        path = self.cls(self.base).with_name('tmp-dirD')
        path.mkdir()
        return path

    def assertFileNotFound(self, func, *args, **kwargs):
        with self.assertRaises(FileNotFoundError) as cm:
            func(*args, **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENOENT)

    def assertEqualNormCase(self, path_a, path_b):
        normcase = self.pathmod.normcase
        self.assertEqual(normcase(path_a), normcase(path_b))

    def test_samefile(self):
        pathmod = self.pathmod
        fileA_path = pathmod.join(self.base, 'fileA')
        fileB_path = pathmod.join(self.base, 'dirB', 'fileB')
        p = self.cls(fileA_path)
        pp = self.cls(fileA_path)
        q = self.cls(fileB_path)
        self.assertTrue(p.samefile(fileA_path))
        self.assertTrue(p.samefile(pp))
        self.assertFalse(p.samefile(fileB_path))
        self.assertFalse(p.samefile(q))
        # Test the non-existent file case
        non_existent = pathmod.join(self.base, 'foo')
        r = self.cls(non_existent)
        self.assertRaises(FileNotFoundError, p.samefile, r)
        self.assertRaises(FileNotFoundError, p.samefile, non_existent)
        self.assertRaises(FileNotFoundError, r.samefile, p)
        self.assertRaises(FileNotFoundError, r.samefile, non_existent)
        self.assertRaises(FileNotFoundError, r.samefile, r)
        self.assertRaises(FileNotFoundError, r.samefile, non_existent)

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

    def test_iterdir(self):
        P = self.cls
        p = P(self.base)
        it = p.iterdir()
        paths = set(it)
        expected = ['dirA', 'dirB', 'dirC', 'dirE', 'fileA']
        if self.can_symlink:
            expected += ['linkA', 'linkB', 'brokenLink', 'brokenLinkLoop']
        self.assertEqual(paths, { P(self.base, q) for q in expected })

    def test_iterdir_symlink(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        # __iter__ on a symlink to a directory.
        P = self.cls
        p = P(self.base, 'linkB')
        paths = set(p.iterdir())
        expected = { P(self.base, 'linkB', q) for q in ['fileB', 'linkD'] }
        self.assertEqual(paths, expected)

    def test_iterdir_nodir(self):
        # __iter__ on something that is not a directory.
        p = self.cls(self.base, 'fileA')
        with self.assertRaises(OSError) as cm:
            p.iterdir()
        # ENOENT or EINVAL under Windows, ENOTDIR otherwise
        # (see issue #12802).
        self.assertIn(cm.exception.errno, (errno.ENOTDIR,
                                           errno.ENOENT, errno.EINVAL))

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

    def test_glob_empty_pattern(self):
        p = self.cls('')
        with self.assertRaisesRegex(ValueError, 'Unacceptable pattern'):
            list(p.glob(''))

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

    def test_glob_follow_symlinks_common(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        def _check(path, glob, expected):
            actual = {path for path in path.glob(glob, follow_symlinks=True)
                      if path.parts.count("linkD") <= 1}  # exclude symlink loop.
            self.assertEqual(actual, { P(self.base, q) for q in expected })
        P = self.cls
        p = P(self.base)
        _check(p, "fileB", [])
        _check(p, "dir*/file*", ["dirB/fileB", "dirC/fileC"])
        _check(p, "*A", ["dirA", "fileA", "linkA"])
        _check(p, "*B/*", ["dirB/fileB", "dirB/linkD", "linkB/fileB", "linkB/linkD"])
        _check(p, "*/fileB", ["dirB/fileB", "linkB/fileB"])
        _check(p, "*/", ["dirA/", "dirB/", "dirC/", "dirE/", "linkB/"])
        _check(p, "dir*/*/..", ["dirC/dirD/..", "dirA/linkC/..", "dirB/linkD/.."])
        _check(p, "dir*/**/", ["dirA/", "dirA/linkC/", "dirA/linkC/linkD/", "dirB/", "dirB/linkD/",
                               "dirC/", "dirC/dirD/", "dirE/"])
        _check(p, "dir*/**/..", ["dirA/..", "dirA/linkC/..", "dirB/..",
                                 "dirB/linkD/..", "dirA/linkC/linkD/..",
                                 "dirC/..", "dirC/dirD/..", "dirE/.."])
        _check(p, "dir*/*/**/", ["dirA/linkC/", "dirA/linkC/linkD/", "dirB/linkD/", "dirC/dirD/"])
        _check(p, "dir*/*/**/..", ["dirA/linkC/..", "dirA/linkC/linkD/..",
                                   "dirB/linkD/..", "dirC/dirD/.."])
        _check(p, "dir*/**/fileC", ["dirC/fileC"])
        _check(p, "dir*/*/../dirD/**/", ["dirC/dirD/../dirD/"])
        _check(p, "*/dirD/**/", ["dirC/dirD/"])

    def test_glob_no_follow_symlinks_common(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        def _check(path, glob, expected):
            actual = {path for path in path.glob(glob, follow_symlinks=False)}
            self.assertEqual(actual, { P(self.base, q) for q in expected })
        P = self.cls
        p = P(self.base)
        _check(p, "fileB", [])
        _check(p, "dir*/file*", ["dirB/fileB", "dirC/fileC"])
        _check(p, "*A", ["dirA", "fileA", "linkA"])
        _check(p, "*B/*", ["dirB/fileB", "dirB/linkD"])
        _check(p, "*/fileB", ["dirB/fileB"])
        _check(p, "*/", ["dirA/", "dirB/", "dirC/", "dirE/"])
        _check(p, "dir*/*/..", ["dirC/dirD/.."])
        _check(p, "dir*/**/", ["dirA/", "dirB/", "dirC/", "dirC/dirD/", "dirE/"])
        _check(p, "dir*/**/..", ["dirA/..", "dirB/..", "dirC/..", "dirC/dirD/..", "dirE/.."])
        _check(p, "dir*/*/**/", ["dirC/dirD/"])
        _check(p, "dir*/*/**/..", ["dirC/dirD/.."])
        _check(p, "dir*/**/fileC", ["dirC/fileC"])
        _check(p, "dir*/*/../dirD/**/", ["dirC/dirD/../dirD/"])
        _check(p, "*/dirD/**/", ["dirC/dirD/"])

    def test_rglob_common(self):
        def _check(glob, expected):
            self.assertEqual(set(glob), {P(self.base, q) for q in expected})
        P = self.cls
        p = P(self.base)
        it = p.rglob("fileA")
        self.assertIsInstance(it, collections.abc.Iterator)
        _check(it, ["fileA"])
        _check(p.rglob("fileB"), ["dirB/fileB"])
        _check(p.rglob("**/fileB"), ["dirB/fileB"])
        _check(p.rglob("*/fileA"), [])
        if not self.can_symlink:
            _check(p.rglob("*/fileB"), ["dirB/fileB"])
        else:
            _check(p.rglob("*/fileB"), ["dirB/fileB", "dirB/linkD/fileB",
                                        "linkB/fileB", "dirA/linkC/fileB"])
        _check(p.rglob("file*"), ["fileA", "dirB/fileB",
                                  "dirC/fileC", "dirC/dirD/fileD"])
        if not self.can_symlink:
            _check(p.rglob("*/"), [
                "dirA/", "dirB/", "dirC/", "dirC/dirD/", "dirE/",
            ])
        else:
            _check(p.rglob("*/"), [
                "dirA/", "dirA/linkC/", "dirB/", "dirB/linkD/", "dirC/",
                "dirC/dirD/", "dirE/", "linkB/",
            ])
        _check(p.rglob(""), ["", "dirA/", "dirB/", "dirC/", "dirE/", "dirC/dirD/"])

        p = P(self.base, "dirC")
        _check(p.rglob("*"), ["dirC/fileC", "dirC/novel.txt",
                              "dirC/dirD", "dirC/dirD/fileD"])
        _check(p.rglob("file*"), ["dirC/fileC", "dirC/dirD/fileD"])
        _check(p.rglob("**/file*"), ["dirC/fileC", "dirC/dirD/fileD"])
        _check(p.rglob("dir*/**/"), ["dirC/dirD/"])
        _check(p.rglob("*/*"), ["dirC/dirD/fileD"])
        _check(p.rglob("*/"), ["dirC/dirD/"])
        _check(p.rglob(""), ["dirC/", "dirC/dirD/"])
        _check(p.rglob("**/"), ["dirC/", "dirC/dirD/"])
        # gh-91616, a re module regression
        _check(p.rglob("*.txt"), ["dirC/novel.txt"])
        _check(p.rglob("*.*"), ["dirC/novel.txt"])

    def test_rglob_follow_symlinks_common(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        def _check(path, glob, expected):
            actual = {path for path in path.rglob(glob, follow_symlinks=True)
                      if path.parts.count("linkD") <= 1}  # exclude symlink loop.
            self.assertEqual(actual, { P(self.base, q) for q in expected })
        P = self.cls
        p = P(self.base)
        _check(p, "fileB", ["dirB/fileB", "dirA/linkC/fileB", "linkB/fileB",
                            "dirA/linkC/linkD/fileB", "dirB/linkD/fileB", "linkB/linkD/fileB"])
        _check(p, "*/fileA", [])
        _check(p, "*/fileB", ["dirB/fileB", "dirA/linkC/fileB", "linkB/fileB",
                              "dirA/linkC/linkD/fileB", "dirB/linkD/fileB", "linkB/linkD/fileB"])
        _check(p, "file*", ["fileA", "dirA/linkC/fileB", "dirB/fileB",
                            "dirA/linkC/linkD/fileB", "dirB/linkD/fileB", "linkB/linkD/fileB",
                            "dirC/fileC", "dirC/dirD/fileD", "linkB/fileB"])
        _check(p, "*/", ["dirA/", "dirA/linkC/", "dirA/linkC/linkD/", "dirB/", "dirB/linkD/",
                         "dirC/", "dirC/dirD/", "dirE/", "linkB/", "linkB/linkD/"])
        _check(p, "", ["", "dirA/", "dirA/linkC/", "dirA/linkC/linkD/", "dirB/", "dirB/linkD/",
                       "dirC/", "dirE/", "dirC/dirD/", "linkB/", "linkB/linkD/"])

        p = P(self.base, "dirC")
        _check(p, "*", ["dirC/fileC", "dirC/novel.txt",
                        "dirC/dirD", "dirC/dirD/fileD"])
        _check(p, "file*", ["dirC/fileC", "dirC/dirD/fileD"])
        _check(p, "*/*", ["dirC/dirD/fileD"])
        _check(p, "*/", ["dirC/dirD/"])
        _check(p, "", ["dirC/", "dirC/dirD/"])
        # gh-91616, a re module regression
        _check(p, "*.txt", ["dirC/novel.txt"])
        _check(p, "*.*", ["dirC/novel.txt"])

    def test_rglob_no_follow_symlinks_common(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        def _check(path, glob, expected):
            actual = {path for path in path.rglob(glob, follow_symlinks=False)}
            self.assertEqual(actual, { P(self.base, q) for q in expected })
        P = self.cls
        p = P(self.base)
        _check(p, "fileB", ["dirB/fileB"])
        _check(p, "*/fileA", [])
        _check(p, "*/fileB", ["dirB/fileB"])
        _check(p, "file*", ["fileA", "dirB/fileB", "dirC/fileC", "dirC/dirD/fileD", ])
        _check(p, "*/", ["dirA/", "dirB/", "dirC/", "dirC/dirD/", "dirE/"])
        _check(p, "", ["", "dirA/", "dirB/", "dirC/", "dirE/", "dirC/dirD/"])

        p = P(self.base, "dirC")
        _check(p, "*", ["dirC/fileC", "dirC/novel.txt",
                        "dirC/dirD", "dirC/dirD/fileD"])
        _check(p, "file*", ["dirC/fileC", "dirC/dirD/fileD"])
        _check(p, "*/*", ["dirC/dirD/fileD"])
        _check(p, "*/", ["dirC/dirD/"])
        _check(p, "", ["dirC/", "dirC/dirD/"])
        # gh-91616, a re module regression
        _check(p, "*.txt", ["dirC/novel.txt"])
        _check(p, "*.*", ["dirC/novel.txt"])

    def test_rglob_symlink_loop(self):
        # Don't get fooled by symlink loops (Issue #26012).
        if not self.can_symlink:
            self.skipTest("symlinks required")
        P = self.cls
        p = P(self.base)
        given = set(p.rglob('*'))
        expect = {'brokenLink',
                  'dirA', 'dirA/linkC',
                  'dirB', 'dirB/fileB', 'dirB/linkD',
                  'dirC', 'dirC/dirD', 'dirC/dirD/fileD',
                  'dirC/fileC', 'dirC/novel.txt',
                  'dirE',
                  'fileA',
                  'linkA',
                  'linkB',
                  'brokenLinkLoop',
                  }
        self.assertEqual(given, {p / x for x in expect})

    def test_glob_dotdot(self):
        # ".." is not special in globs.
        P = self.cls
        p = P(self.base)
        self.assertEqual(set(p.glob("..")), { P(self.base, "..") })
        self.assertEqual(set(p.glob("../..")), { P(self.base, "..", "..") })
        self.assertEqual(set(p.glob("dirA/..")), { P(self.base, "dirA", "..") })
        self.assertEqual(set(p.glob("dirA/../file*")), { P(self.base, "dirA/../fileA") })
        self.assertEqual(set(p.glob("dirA/../file*/..")), set())
        self.assertEqual(set(p.glob("../xyzzy")), set())
        self.assertEqual(set(p.glob("xyzzy/..")), set())
        self.assertEqual(set(p.glob("/".join([".."] * 50))), { P(self.base, *[".."] * 50)})

    def test_glob_permissions(self):
        # See bpo-38894
        if not self.can_symlink:
            self.skipTest("symlinks required")
        P = self.cls
        base = P(self.base) / 'permissions'
        base.mkdir()

        for i in range(100):
            link = base / f"link{i}"
            if i % 2:
                link.symlink_to(P(self.base, "dirE", "nonexistent"))
            else:
                link.symlink_to(P(self.base, "dirC"))

        self.assertEqual(len(set(base.glob("*"))), 100)
        self.assertEqual(len(set(base.glob("*/"))), 50)
        self.assertEqual(len(set(base.glob("*/fileC"))), 50)
        self.assertEqual(len(set(base.glob("*/file*"))), 50)

    def test_glob_long_symlink(self):
        # See gh-87695
        if not self.can_symlink:
            self.skipTest("symlinks required")
        base = self.cls(self.base) / 'long_symlink'
        base.mkdir()
        bad_link = base / 'bad_link'
        bad_link.symlink_to("bad" * 200)
        self.assertEqual(sorted(base.glob('**/*')), [bad_link])

    def test_readlink(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        P = self.cls(self.base)
        self.assertEqual((P / 'linkA').readlink(), self.cls('fileA'))
        self.assertEqual((P / 'brokenLink').readlink(),
                         self.cls('non-existing'))
        self.assertEqual((P / 'linkB').readlink(), self.cls('dirB'))
        self.assertEqual((P / 'linkB' / 'linkD').readlink(), self.cls('../dirB'))
        with self.assertRaises(OSError):
            (P / 'fileA').readlink()

    @unittest.skipIf(hasattr(os, "readlink"), "os.readlink() is present")
    def test_readlink_unsupported(self):
        P = self.cls(self.base)
        p = P / 'fileA'
        with self.assertRaises(UnsupportedOperation):
            q.readlink(p)

    def _check_resolve(self, p, expected, strict=True):
        q = p.resolve(strict)
        self.assertEqual(q, expected)

    # This can be used to check both relative and absolute resolutions.
    _check_resolve_relative = _check_resolve_absolute = _check_resolve

    def test_resolve_common(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        P = self.cls
        p = P(self.base, 'foo')
        with self.assertRaises(OSError) as cm:
            p.resolve(strict=True)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        # Non-strict
        pathmod = self.pathmod
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 pathmod.join(self.base, 'foo'))
        p = P(self.base, 'foo', 'in', 'spam')
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 pathmod.join(self.base, 'foo', 'in', 'spam'))
        p = P(self.base, '..', 'foo', 'in', 'spam')
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 pathmod.join(pathmod.dirname(self.base), 'foo', 'in', 'spam'))
        # These are all relative symlinks.
        p = P(self.base, 'dirB', 'fileB')
        self._check_resolve_relative(p, p)
        p = P(self.base, 'linkA')
        self._check_resolve_relative(p, P(self.base, 'fileA'))
        p = P(self.base, 'dirA', 'linkC', 'fileB')
        self._check_resolve_relative(p, P(self.base, 'dirB', 'fileB'))
        p = P(self.base, 'dirB', 'linkD', 'fileB')
        self._check_resolve_relative(p, P(self.base, 'dirB', 'fileB'))
        # Non-strict
        p = P(self.base, 'dirA', 'linkC', 'fileB', 'foo', 'in', 'spam')
        self._check_resolve_relative(p, P(self.base, 'dirB', 'fileB', 'foo', 'in',
                                          'spam'), False)
        p = P(self.base, 'dirA', 'linkC', '..', 'foo', 'in', 'spam')
        if self.cls.pathmod is not posixpath:
            # In Windows, if linkY points to dirB, 'dirA\linkY\..'
            # resolves to 'dirA' without resolving linkY first.
            self._check_resolve_relative(p, P(self.base, 'dirA', 'foo', 'in',
                                              'spam'), False)
        else:
            # In Posix, if linkY points to dirB, 'dirA/linkY/..'
            # resolves to 'dirB/..' first before resolving to parent of dirB.
            self._check_resolve_relative(p, P(self.base, 'foo', 'in', 'spam'), False)
        # Now create absolute symlinks.
        d = self.tempdir()
        P(self.base, 'dirA', 'linkX').symlink_to(d)
        P(self.base, str(d), 'linkY').symlink_to(self.pathmod.join(self.base, 'dirB'))
        p = P(self.base, 'dirA', 'linkX', 'linkY', 'fileB')
        self._check_resolve_absolute(p, P(self.base, 'dirB', 'fileB'))
        # Non-strict
        p = P(self.base, 'dirA', 'linkX', 'linkY', 'foo', 'in', 'spam')
        self._check_resolve_relative(p, P(self.base, 'dirB', 'foo', 'in', 'spam'),
                                     False)
        p = P(self.base, 'dirA', 'linkX', 'linkY', '..', 'foo', 'in', 'spam')
        if self.cls.pathmod is not posixpath:
            # In Windows, if linkY points to dirB, 'dirA\linkY\..'
            # resolves to 'dirA' without resolving linkY first.
            self._check_resolve_relative(p, P(d, 'foo', 'in', 'spam'), False)
        else:
            # In Posix, if linkY points to dirB, 'dirA/linkY/..'
            # resolves to 'dirB/..' first before resolving to parent of dirB.
            self._check_resolve_relative(p, P(self.base, 'foo', 'in', 'spam'), False)

    def test_resolve_dot(self):
        # See http://web.archive.org/web/20200623062557/https://bitbucket.org/pitrou/pathlib/issues/9/
        if not self.can_symlink:
            self.skipTest("symlinks required")
        pathmod = self.pathmod
        p = self.cls(self.base)
        p.joinpath('0').symlink_to('.', target_is_directory=True)
        p.joinpath('1').symlink_to(pathmod.join('0', '0'), target_is_directory=True)
        p.joinpath('2').symlink_to(pathmod.join('1', '1'), target_is_directory=True)
        q = p / '2'
        self.assertEqual(q.resolve(strict=True), p)
        r = q / '3' / '4'
        self.assertRaises(FileNotFoundError, r.resolve, strict=True)
        # Non-strict
        self.assertEqual(r.resolve(strict=False), p / '3' / '4')

    def _check_symlink_loop(self, *args):
        path = self.cls(*args)
        with self.assertRaises(OSError) as cm:
            path.resolve(strict=True)
        self.assertEqual(cm.exception.errno, errno.ELOOP)

    def test_resolve_loop(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        if self.cls.pathmod is not posixpath:
            self.skipTest("symlink loops work differently with concrete Windows paths")
        # Loops with relative symlinks.
        self.cls(self.base, 'linkX').symlink_to('linkX/inside')
        self._check_symlink_loop(self.base, 'linkX')
        self.cls(self.base, 'linkY').symlink_to('linkY')
        self._check_symlink_loop(self.base, 'linkY')
        self.cls(self.base, 'linkZ').symlink_to('linkZ/../linkZ')
        self._check_symlink_loop(self.base, 'linkZ')
        # Non-strict
        p = self.cls(self.base, 'linkZ', 'foo')
        self.assertEqual(p.resolve(strict=False), p)
        # Loops with absolute symlinks.
        self.cls(self.base, 'linkU').symlink_to(self.pathmod.join(self.base, 'linkU/inside'))
        self._check_symlink_loop(self.base, 'linkU')
        self.cls(self.base, 'linkV').symlink_to(self.pathmod.join(self.base, 'linkV'))
        self._check_symlink_loop(self.base, 'linkV')
        self.cls(self.base, 'linkW').symlink_to(self.pathmod.join(self.base, 'linkW/../linkW'))
        self._check_symlink_loop(self.base, 'linkW')
        # Non-strict
        q = self.cls(self.base, 'linkW', 'foo')
        self.assertEqual(q.resolve(strict=False), q)

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

    def test_stat_no_follow_symlinks(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        p = self.cls(self.base) / 'linkA'
        st = p.stat()
        self.assertNotEqual(st, p.stat(follow_symlinks=False))

    def test_stat_no_follow_symlinks_nosymlink(self):
        p = self.cls(self.base) / 'fileA'
        st = p.stat()
        self.assertEqual(st, p.stat(follow_symlinks=False))

    def test_lstat(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        p = self.cls(self.base)/ 'linkA'
        st = p.stat()
        self.assertNotEqual(st, p.lstat())

    def test_lstat_nosymlink(self):
        p = self.cls(self.base) / 'fileA'
        st = p.stat()
        self.assertEqual(st, p.lstat())

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

    def test_is_mount(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_mount())
        self.assertFalse((P / 'dirA').is_mount())
        self.assertFalse((P / 'non-existing').is_mount())
        self.assertFalse((P / 'fileA' / 'bah').is_mount())
        if self.can_symlink:
            self.assertFalse((P / 'linkA').is_mount())

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

    def test_is_junction_false(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_junction())
        self.assertFalse((P / 'dirA').is_junction())
        self.assertFalse((P / 'non-existing').is_junction())
        self.assertFalse((P / 'fileA' / 'bah').is_junction())
        self.assertFalse((P / 'fileA\udfff').is_junction())
        self.assertFalse((P / 'fileA\x00').is_junction())

    def test_is_fifo_false(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_fifo())
        self.assertFalse((P / 'dirA').is_fifo())
        self.assertFalse((P / 'non-existing').is_fifo())
        self.assertFalse((P / 'fileA' / 'bah').is_fifo())
        self.assertIs((P / 'fileA\udfff').is_fifo(), False)
        self.assertIs((P / 'fileA\x00').is_fifo(), False)

    def test_is_socket_false(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_socket())
        self.assertFalse((P / 'dirA').is_socket())
        self.assertFalse((P / 'non-existing').is_socket())
        self.assertFalse((P / 'fileA' / 'bah').is_socket())
        self.assertIs((P / 'fileA\udfff').is_socket(), False)
        self.assertIs((P / 'fileA\x00').is_socket(), False)

    def test_is_block_device_false(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_block_device())
        self.assertFalse((P / 'dirA').is_block_device())
        self.assertFalse((P / 'non-existing').is_block_device())
        self.assertFalse((P / 'fileA' / 'bah').is_block_device())
        self.assertIs((P / 'fileA\udfff').is_block_device(), False)
        self.assertIs((P / 'fileA\x00').is_block_device(), False)

    def test_is_char_device_false(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_char_device())
        self.assertFalse((P / 'dirA').is_char_device())
        self.assertFalse((P / 'non-existing').is_char_device())
        self.assertFalse((P / 'fileA' / 'bah').is_char_device())
        self.assertIs((P / 'fileA\udfff').is_char_device(), False)
        self.assertIs((P / 'fileA\x00').is_char_device(), False)

    def _check_complex_symlinks(self, link0_target):
        if not self.can_symlink:
            self.skipTest("symlinks required")

        # Test solving a non-looping chain of symlinks (issue #19887).
        pathmod = self.pathmod
        P = self.cls(self.base)
        P.joinpath('link1').symlink_to(pathmod.join('link0', 'link0'), target_is_directory=True)
        P.joinpath('link2').symlink_to(pathmod.join('link1', 'link1'), target_is_directory=True)
        P.joinpath('link3').symlink_to(pathmod.join('link2', 'link2'), target_is_directory=True)
        P.joinpath('link0').symlink_to(link0_target, target_is_directory=True)

        # Resolve absolute paths.
        p = (P / 'link0').resolve()
        self.assertEqual(p, P)
        self.assertEqualNormCase(str(p), self.base)
        p = (P / 'link1').resolve()
        self.assertEqual(p, P)
        self.assertEqualNormCase(str(p), self.base)
        p = (P / 'link2').resolve()
        self.assertEqual(p, P)
        self.assertEqualNormCase(str(p), self.base)
        p = (P / 'link3').resolve()
        self.assertEqual(p, P)
        self.assertEqualNormCase(str(p), self.base)

        # Resolve relative paths.
        try:
            self.cls('').absolute()
        except UnsupportedOperation:
            return
        old_path = os.getcwd()
        os.chdir(self.base)
        try:
            p = self.cls('link0').resolve()
            self.assertEqual(p, P)
            self.assertEqualNormCase(str(p), self.base)
            p = self.cls('link1').resolve()
            self.assertEqual(p, P)
            self.assertEqualNormCase(str(p), self.base)
            p = self.cls('link2').resolve()
            self.assertEqual(p, P)
            self.assertEqualNormCase(str(p), self.base)
            p = self.cls('link3').resolve()
            self.assertEqual(p, P)
            self.assertEqualNormCase(str(p), self.base)
        finally:
            os.chdir(old_path)

    def test_complex_symlinks_absolute(self):
        self._check_complex_symlinks(self.base)

    def test_complex_symlinks_relative(self):
        self._check_complex_symlinks('.')

    def test_complex_symlinks_relative_dot_dot(self):
        self._check_complex_symlinks(self.pathmod.join('dirA', '..'))

    def setUpWalk(self):
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
            self.link_path.symlink_to(t2_path)
            broken_link_path.symlink_to('broken')
            broken_link2_path.symlink_to(self.cls('tmp3', 'broken'))
            self.sub2_tree = (self.sub2_path, [], ["broken_link", "broken_link2", "link", "tmp3"])
        else:
            self.sub2_tree = (self.sub2_path, [], ["tmp3"])

    def test_walk_topdown(self):
        self.setUpWalk()
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
        self.setUpWalk()
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
        self.setUpWalk()
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

    def test_walk_follow_symlinks(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        self.setUpWalk()
        walk_it = self.walk_path.walk(follow_symlinks=True)
        for root, dirs, files in walk_it:
            if root == self.link_path:
                self.assertEqual(dirs, [])
                self.assertEqual(files, ["tmp4"])
                break
        else:
            self.fail("Didn't follow symlink with follow_symlinks=True")

    def test_walk_symlink_location(self):
        if not self.can_symlink:
            self.skipTest("symlinks required")
        self.setUpWalk()
        # Tests whether symlinks end up in filenames or dirnames depending
        # on the `follow_symlinks` argument.
        walk_it = self.walk_path.walk(follow_symlinks=False)
        for root, dirs, files in walk_it:
            if root == self.sub2_path:
                self.assertIn("link", files)
                break
        else:
            self.fail("symlink not found")

        walk_it = self.walk_path.walk(follow_symlinks=True)
        for root, dirs, files in walk_it:
            if root == self.sub2_path:
                self.assertIn("link", dirs)
                break
        else:
            self.fail("symlink not found")


class DummyPathWithSymlinks(DummyPath):
    __slots__ = ()

    # Reduce symlink traversal limit to make tests run faster.
    _max_symlinks = 20

    def readlink(self):
        path = str(self.parent.resolve() / self.name)
        if path in self._symlinks:
            return self.with_segments(self._symlinks[path])
        elif path in self._files or path in self._directories:
            raise OSError(errno.EINVAL, "Not a symlink", path)
        else:
            raise FileNotFoundError(errno.ENOENT, "File not found", path)

    def symlink_to(self, target, target_is_directory=False):
        self._directories[str(self.parent)].add(self.name)
        self._symlinks[str(self)] = str(target)


class DummyPathWithSymlinksTest(DummyPathTest):
    cls = DummyPathWithSymlinks
    can_symlink = True


if __name__ == "__main__":
    unittest.main()
