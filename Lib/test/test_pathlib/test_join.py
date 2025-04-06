"""
Tests for pathlib.types._JoinablePath
"""

import unittest

from .support import is_pypi
from .support.lexical_path import LexicalPath

if is_pypi:
    from pathlib_abc import _PathParser, _JoinablePath
else:
    from pathlib.types import _PathParser, _JoinablePath


class JoinTestBase:
    def test_is_joinable(self):
        p = self.cls()
        self.assertIsInstance(p, _JoinablePath)

    def test_parser(self):
        self.assertIsInstance(self.cls.parser, _PathParser)

    def test_constructor(self):
        P = self.cls
        p = P('a')
        self.assertIsInstance(p, P)
        P()
        P('a', 'b', 'c')
        P('/a', 'b', 'c')
        P('a/b/c')
        P('/a/b/c')

    def test_with_segments(self):
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
        self.assertEqual(42, p.parent.session_id)
        for parent in p.parents:
            self.assertEqual(42, parent.session_id)

    def test_join(self):
        P = self.cls
        sep = self.cls.parser.sep
        p = P(f'a{sep}b')
        pp = p.joinpath('c')
        self.assertEqual(pp, P(f'a{sep}b{sep}c'))
        self.assertIs(type(pp), type(p))
        pp = p.joinpath('c', 'd')
        self.assertEqual(pp, P(f'a{sep}b{sep}c{sep}d'))
        pp = p.joinpath(f'{sep}c')
        self.assertEqual(pp, P(f'{sep}c'))

    def test_div(self):
        # Basically the same as joinpath().
        P = self.cls
        sep = self.cls.parser.sep
        p = P(f'a{sep}b')
        pp = p / 'c'
        self.assertEqual(pp, P(f'a{sep}b{sep}c'))
        self.assertIs(type(pp), type(p))
        pp = p / f'c{sep}d'
        self.assertEqual(pp, P(f'a{sep}b{sep}c{sep}d'))
        pp = p / 'c' / 'd'
        self.assertEqual(pp, P(f'a{sep}b{sep}c{sep}d'))
        pp = 'c' / p / 'd'
        self.assertEqual(pp, P(f'c{sep}a{sep}b{sep}d'))
        pp = p/ f'{sep}c'
        self.assertEqual(pp, P(f'{sep}c'))

    def test_full_match(self):
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

    def test_parts(self):
        # `parts` returns a tuple.
        sep = self.cls.parser.sep
        P = self.cls
        p = P(f'a{sep}b')
        parts = p.parts
        self.assertEqual(parts, ('a', 'b'))
        # When the path is absolute, the anchor is a separate part.
        p = P(f'{sep}a{sep}b')
        parts = p.parts
        self.assertEqual(parts, (sep, 'a', 'b'))

    def test_parent(self):
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

    def test_parents(self):
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

    def test_anchor(self):
        P = self.cls
        sep = self.cls.parser.sep
        self.assertEqual(P('').anchor, '')
        self.assertEqual(P(f'a{sep}b').anchor, '')
        self.assertEqual(P(sep).anchor, sep)
        self.assertEqual(P(f'{sep}a{sep}b').anchor, sep)

    def test_name(self):
        P = self.cls
        self.assertEqual(P('').name, '')
        self.assertEqual(P('/').name, '')
        self.assertEqual(P('a/b').name, 'b')
        self.assertEqual(P('/a/b').name, 'b')
        self.assertEqual(P('a/b.py').name, 'b.py')
        self.assertEqual(P('/a/b.py').name, 'b.py')

    def test_suffix(self):
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

    def test_suffixes(self):
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

    def test_stem(self):
        P = self.cls
        self.assertEqual(P('..').stem, '..')
        self.assertEqual(P('').stem, '')
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

    def test_with_name(self):
        P = self.cls
        self.assertEqual(P('a/b').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/b').with_name('d.xml'), P('/a/d.xml'))
        self.assertEqual(P('a/b.py').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/b.py').with_name('d.xml'), P('/a/d.xml'))
        self.assertEqual(P('a/Dot ending.').with_name('d.xml'), P('a/d.xml'))
        self.assertEqual(P('/a/Dot ending.').with_name('d.xml'), P('/a/d.xml'))
        self.assertRaises(ValueError, P('a/b').with_name, '/c')
        self.assertRaises(ValueError, P('a/b').with_name, 'c/')
        self.assertRaises(ValueError, P('a/b').with_name, 'c/d')

    def test_with_stem(self):
        P = self.cls
        self.assertEqual(P('a/b').with_stem('d'), P('a/d'))
        self.assertEqual(P('/a/b').with_stem('d'), P('/a/d'))
        self.assertEqual(P('a/b.py').with_stem('d'), P('a/d.py'))
        self.assertEqual(P('/a/b.py').with_stem('d'), P('/a/d.py'))
        self.assertEqual(P('/a/b.tar.gz').with_stem('d'), P('/a/d.gz'))
        self.assertEqual(P('a/Dot ending.').with_stem('d'), P('a/d.'))
        self.assertEqual(P('/a/Dot ending.').with_stem('d'), P('/a/d.'))
        self.assertRaises(ValueError, P('foo.gz').with_stem, '')
        self.assertRaises(ValueError, P('/a/b/foo.gz').with_stem, '')
        self.assertRaises(ValueError, P('a/b').with_stem, '/c')
        self.assertRaises(ValueError, P('a/b').with_stem, 'c/')
        self.assertRaises(ValueError, P('a/b').with_stem, 'c/d')

    def test_with_suffix(self):
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
        # Path doesn't have a "filename" component.
        self.assertRaises(ValueError, P('').with_suffix, '.gz')
        self.assertRaises(ValueError, P('/').with_suffix, '.gz')
        # Invalid suffix.
        self.assertRaises(ValueError, P('a/b').with_suffix, 'gz')
        self.assertRaises(ValueError, P('a/b').with_suffix, '/')
        self.assertRaises(ValueError, P('a/b').with_suffix, '/.gz')
        self.assertRaises(ValueError, P('a/b').with_suffix, 'c/d')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.c/.d')
        self.assertRaises(ValueError, P('a/b').with_suffix, './.d')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.d/.')
        self.assertRaises(TypeError, P('a/b').with_suffix, None)


class LexicalPathJoinTest(JoinTestBase, unittest.TestCase):
    cls = LexicalPath


if not is_pypi:
    from pathlib import PurePath, Path

    class PurePathJoinTest(JoinTestBase, unittest.TestCase):
        cls = PurePath

    class PathJoinTest(JoinTestBase, unittest.TestCase):
        cls = Path


if __name__ == "__main__":
    unittest.main()
