import collections.abc
import io
import os
import sys
import errno
import pathlib
import pickle
import socket
import stat
import tempfile
import unittest
from unittest import mock

from test.support import import_helper
from test.support import os_helper
from test.support.os_helper import TESTFN, FakePath

try:
    import grp, pwd
except ImportError:
    grp = pwd = None


class _BaseFlavourTest(object):

    def _check_parse_parts(self, arg, expected):
        f = self.flavour.parse_parts
        sep = self.flavour.sep
        altsep = self.flavour.altsep
        actual = f([x.replace('/', sep) for x in arg])
        self.assertEqual(actual, expected)
        if altsep:
            actual = f([x.replace('/', altsep) for x in arg])
            self.assertEqual(actual, expected)

    def test_parse_parts_common(self):
        check = self._check_parse_parts
        sep = self.flavour.sep
        # Unanchored parts.
        check([],                   ('', '', []))
        check(['a'],                ('', '', ['a']))
        check(['a/'],               ('', '', ['a']))
        check(['a', 'b'],           ('', '', ['a', 'b']))
        # Expansion.
        check(['a/b'],              ('', '', ['a', 'b']))
        check(['a/b/'],             ('', '', ['a', 'b']))
        check(['a', 'b/c', 'd'],    ('', '', ['a', 'b', 'c', 'd']))
        # Collapsing and stripping excess slashes.
        check(['a', 'b//c', 'd'],   ('', '', ['a', 'b', 'c', 'd']))
        check(['a', 'b/c/', 'd'],   ('', '', ['a', 'b', 'c', 'd']))
        # Eliminating standalone dots.
        check(['.'],                ('', '', []))
        check(['.', '.', 'b'],      ('', '', ['b']))
        check(['a', '.', 'b'],      ('', '', ['a', 'b']))
        check(['a', '.', '.'],      ('', '', ['a']))
        # The first part is anchored.
        check(['/a/b'],             ('', sep, [sep, 'a', 'b']))
        check(['/a', 'b'],          ('', sep, [sep, 'a', 'b']))
        check(['/a/', 'b'],         ('', sep, [sep, 'a', 'b']))
        # Ignoring parts before an anchored part.
        check(['a', '/b', 'c'],     ('', sep, [sep, 'b', 'c']))
        check(['a', '/b', '/c'],    ('', sep, [sep, 'c']))


class PosixFlavourTest(_BaseFlavourTest, unittest.TestCase):
    flavour = pathlib._posix_flavour

    def test_parse_parts(self):
        check = self._check_parse_parts
        # Collapsing of excess leading slashes, except for the double-slash
        # special case.
        check(['//a', 'b'],             ('', '//', ['//', 'a', 'b']))
        check(['///a', 'b'],            ('', '/', ['/', 'a', 'b']))
        check(['////a', 'b'],           ('', '/', ['/', 'a', 'b']))
        # Paths which look like NT paths aren't treated specially.
        check(['c:a'],                  ('', '', ['c:a']))
        check(['c:\\a'],                ('', '', ['c:\\a']))
        check(['\\a'],                  ('', '', ['\\a']))

    def test_splitroot(self):
        f = self.flavour.splitroot
        self.assertEqual(f(''), ('', '', ''))
        self.assertEqual(f('a'), ('', '', 'a'))
        self.assertEqual(f('a/b'), ('', '', 'a/b'))
        self.assertEqual(f('a/b/'), ('', '', 'a/b/'))
        self.assertEqual(f('/a'), ('', '/', 'a'))
        self.assertEqual(f('/a/b'), ('', '/', 'a/b'))
        self.assertEqual(f('/a/b/'), ('', '/', 'a/b/'))
        # The root is collapsed when there are redundant slashes
        # except when there are exactly two leading slashes, which
        # is a special case in POSIX.
        self.assertEqual(f('//a'), ('', '//', 'a'))
        self.assertEqual(f('///a'), ('', '/', 'a'))
        self.assertEqual(f('///a/b'), ('', '/', 'a/b'))
        # Paths which look like NT paths aren't treated specially.
        self.assertEqual(f('c:/a/b'), ('', '', 'c:/a/b'))
        self.assertEqual(f('\\/a/b'), ('', '', '\\/a/b'))
        self.assertEqual(f('\\a\\b'), ('', '', '\\a\\b'))


class NTFlavourTest(_BaseFlavourTest, unittest.TestCase):
    flavour = pathlib._windows_flavour

    def test_parse_parts(self):
        check = self._check_parse_parts
        # First part is anchored.
        check(['c:'],                   ('c:', '', ['c:']))
        check(['c:/'],                  ('c:', '\\', ['c:\\']))
        check(['/'],                    ('', '\\', ['\\']))
        check(['c:a'],                  ('c:', '', ['c:', 'a']))
        check(['c:/a'],                 ('c:', '\\', ['c:\\', 'a']))
        check(['/a'],                   ('', '\\', ['\\', 'a']))
        # UNC paths.
        check(['//a/b'],                ('\\\\a\\b', '\\', ['\\\\a\\b\\']))
        check(['//a/b/'],               ('\\\\a\\b', '\\', ['\\\\a\\b\\']))
        check(['//a/b/c'],              ('\\\\a\\b', '\\', ['\\\\a\\b\\', 'c']))
        # Second part is anchored, so that the first part is ignored.
        check(['a', 'Z:b', 'c'],        ('Z:', '', ['Z:', 'b', 'c']))
        check(['a', 'Z:/b', 'c'],       ('Z:', '\\', ['Z:\\', 'b', 'c']))
        # UNC paths.
        check(['a', '//b/c', 'd'],      ('\\\\b\\c', '\\', ['\\\\b\\c\\', 'd']))
        # Collapsing and stripping excess slashes.
        check(['a', 'Z://b//c/', 'd/'], ('Z:', '\\', ['Z:\\', 'b', 'c', 'd']))
        # UNC paths.
        check(['a', '//b/c//', 'd'],    ('\\\\b\\c', '\\', ['\\\\b\\c\\', 'd']))
        # Extended paths.
        check(['//?/c:/'],              ('\\\\?\\c:', '\\', ['\\\\?\\c:\\']))
        check(['//?/c:/a'],             ('\\\\?\\c:', '\\', ['\\\\?\\c:\\', 'a']))
        check(['//?/c:/a', '/b'],       ('\\\\?\\c:', '\\', ['\\\\?\\c:\\', 'b']))
        # Extended UNC paths (format is "\\?\UNC\server\share").
        check(['//?/UNC/b/c'],          ('\\\\?\\UNC\\b\\c', '\\', ['\\\\?\\UNC\\b\\c\\']))
        check(['//?/UNC/b/c/d'],        ('\\\\?\\UNC\\b\\c', '\\', ['\\\\?\\UNC\\b\\c\\', 'd']))
        # Second part has a root but not drive.
        check(['a', '/b', 'c'],         ('', '\\', ['\\', 'b', 'c']))
        check(['Z:/a', '/b', 'c'],      ('Z:', '\\', ['Z:\\', 'b', 'c']))
        check(['//?/Z:/a', '/b', 'c'],  ('\\\\?\\Z:', '\\', ['\\\\?\\Z:\\', 'b', 'c']))

    def test_splitroot(self):
        f = self.flavour.splitroot
        self.assertEqual(f(''), ('', '', ''))
        self.assertEqual(f('a'), ('', '', 'a'))
        self.assertEqual(f('a\\b'), ('', '', 'a\\b'))
        self.assertEqual(f('\\a'), ('', '\\', 'a'))
        self.assertEqual(f('\\a\\b'), ('', '\\', 'a\\b'))
        self.assertEqual(f('c:a\\b'), ('c:', '', 'a\\b'))
        self.assertEqual(f('c:\\a\\b'), ('c:', '\\', 'a\\b'))
        # Redundant slashes in the root are collapsed.
        self.assertEqual(f('\\\\a'), ('', '\\', 'a'))
        self.assertEqual(f('\\\\\\a/b'), ('', '\\', 'a/b'))
        self.assertEqual(f('c:\\\\a'), ('c:', '\\', 'a'))
        self.assertEqual(f('c:\\\\\\a/b'), ('c:', '\\', 'a/b'))
        # Valid UNC paths.
        self.assertEqual(f('\\\\a\\b'), ('\\\\a\\b', '\\', ''))
        self.assertEqual(f('\\\\a\\b\\'), ('\\\\a\\b', '\\', ''))
        self.assertEqual(f('\\\\a\\b\\c\\d'), ('\\\\a\\b', '\\', 'c\\d'))
        # These are non-UNC paths (according to ntpath.py and test_ntpath).
        # However, command.com says such paths are invalid, so it's
        # difficult to know what the right semantics are.
        self.assertEqual(f('\\\\\\a\\b'), ('', '\\', 'a\\b'))
        self.assertEqual(f('\\\\a'), ('', '\\', 'a'))


#
# Tests for the pure classes.
#

class _BasePurePathTest(object):

    # Keys are canonical paths, values are list of tuples of arguments
    # supposed to produce equal paths.
    equivalences = {
        'a/b': [
            ('a', 'b'), ('a/', 'b'), ('a', 'b/'), ('a/', 'b/'),
            ('a/b/',), ('a//b',), ('a//b//',),
            # Empty components get removed.
            ('', 'a', 'b'), ('a', '', 'b'), ('a', 'b', ''),
            ],
        '/b/c/d': [
            ('a', '/b/c', 'd'), ('a', '///b//c', 'd/'),
            ('/a', '/b/c', 'd'),
            # Empty components get removed.
            ('/', 'b', '', 'c/d'), ('/', '', 'b/c/d'), ('', '/b/c/d'),
            ],
    }

    def setUp(self):
        p = self.cls('a')
        self.flavour = p._flavour
        self.sep = self.flavour.sep
        self.altsep = self.flavour.altsep

    def test_constructor_common(self):
        P = self.cls
        p = P('a')
        self.assertIsInstance(p, P)
        P('a', 'b', 'c')
        P('/a', 'b', 'c')
        P('a/b/c')
        P('/a/b/c')
        P(FakePath("a/b/c"))
        self.assertEqual(P(P('a')), P('a'))
        self.assertEqual(P(P('a'), 'b'), P('a/b'))
        self.assertEqual(P(P('a'), P('b')), P('a/b'))
        self.assertEqual(P(P('a'), P('b'), P('c')), P(FakePath("a/b/c")))

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

    def test_join_common(self):
        P = self.cls
        p = P('a/b')
        pp = p.joinpath('c')
        self.assertEqual(pp, P('a/b/c'))
        self.assertIs(type(pp), type(p))
        pp = p.joinpath('c', 'd')
        self.assertEqual(pp, P('a/b/c/d'))
        pp = p.joinpath(P('c'))
        self.assertEqual(pp, P('a/b/c'))
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
        pp = p / P('c')
        self.assertEqual(pp, P('a/b/c'))
        pp = p/ '/c'
        self.assertEqual(pp, P('/c'))

    def _check_str(self, expected, args):
        p = self.cls(*args)
        self.assertEqual(str(p), expected.replace('/', self.sep))

    def test_str_common(self):
        # Canonicalized paths roundtrip.
        for pathstr in ('a', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c'):
            self._check_str(pathstr, (pathstr,))
        # Special case for the empty path.
        self._check_str('.', ('',))
        # Other tests for str() are in test_equivalences().

    def test_as_posix_common(self):
        P = self.cls
        for pathstr in ('a', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c'):
            self.assertEqual(P(pathstr).as_posix(), pathstr)
        # Other tests for as_posix() are in test_equivalences().

    def test_as_bytes_common(self):
        sep = os.fsencode(self.sep)
        P = self.cls
        self.assertEqual(bytes(P('a/b')), b'a' + sep + b'b')

    def test_as_uri_common(self):
        P = self.cls
        with self.assertRaises(ValueError):
            P('a').as_uri()
        with self.assertRaises(ValueError):
            P().as_uri()

    def test_repr_common(self):
        for pathstr in ('a', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c'):
            p = self.cls(pathstr)
            clsname = p.__class__.__name__
            r = repr(p)
            # The repr() is in the form ClassName("forward-slashes path").
            self.assertTrue(r.startswith(clsname + '('), r)
            self.assertTrue(r.endswith(')'), r)
            inner = r[len(clsname) + 1 : -1]
            self.assertEqual(eval(inner), p.as_posix())
            # The repr() roundtrips.
            q = eval(r, pathlib.__dict__)
            self.assertIs(q.__class__, p.__class__)
            self.assertEqual(q, p)
            self.assertEqual(repr(q), r)

    def test_eq_common(self):
        P = self.cls
        self.assertEqual(P('a/b'), P('a/b'))
        self.assertEqual(P('a/b'), P('a', 'b'))
        self.assertNotEqual(P('a/b'), P('a'))
        self.assertNotEqual(P('a/b'), P('/a/b'))
        self.assertNotEqual(P('a/b'), P())
        self.assertNotEqual(P('/a/b'), P('/'))
        self.assertNotEqual(P(), P('/'))
        self.assertNotEqual(P(), "")
        self.assertNotEqual(P(), {})
        self.assertNotEqual(P(), int)

    def test_match_common(self):
        P = self.cls
        self.assertRaises(ValueError, P('a').match, '')
        self.assertRaises(ValueError, P('a').match, '.')
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

    def test_ordering_common(self):
        # Ordering is tuple-alike.
        def assertLess(a, b):
            self.assertLess(a, b)
            self.assertGreater(b, a)
        P = self.cls
        a = P('a')
        b = P('a/b')
        c = P('abc')
        d = P('b')
        assertLess(a, b)
        assertLess(a, c)
        assertLess(a, d)
        assertLess(b, c)
        assertLess(c, d)
        P = self.cls
        a = P('/a')
        b = P('/a/b')
        c = P('/abc')
        d = P('/b')
        assertLess(a, b)
        assertLess(a, c)
        assertLess(a, d)
        assertLess(b, c)
        assertLess(c, d)
        with self.assertRaises(TypeError):
            P() < {}

    def test_parts_common(self):
        # `parts` returns a tuple.
        sep = self.sep
        P = self.cls
        p = P('a/b')
        parts = p.parts
        self.assertEqual(parts, ('a', 'b'))
        # The object gets reused.
        self.assertIs(parts, p.parts)
        # When the path is absolute, the anchor is a separate part.
        p = P('/a/b')
        parts = p.parts
        self.assertEqual(parts, (sep, 'a', 'b'))

    def test_fspath_common(self):
        P = self.cls
        p = P('a/b')
        self._check_str(p.__fspath__(), ('a/b',))
        self._check_str(os.fspath(p), ('a/b',))

    def test_equivalences(self):
        for k, tuples in self.equivalences.items():
            canon = k.replace('/', self.sep)
            posix = k.replace(self.sep, '/')
            if canon != posix:
                tuples = tuples + [
                    tuple(part.replace('/', self.sep) for part in t)
                    for t in tuples
                    ]
                tuples.append((posix, ))
            pcanon = self.cls(canon)
            for t in tuples:
                p = self.cls(*t)
                self.assertEqual(p, pcanon, "failed with args {}".format(t))
                self.assertEqual(hash(p), hash(pcanon))
                self.assertEqual(str(p), canon)
                self.assertEqual(p.as_posix(), posix)

    def test_parent_common(self):
        # Relative
        P = self.cls
        p = P('a/b/c')
        self.assertEqual(p.parent, P('a/b'))
        self.assertEqual(p.parent.parent, P('a'))
        self.assertEqual(p.parent.parent.parent, P())
        self.assertEqual(p.parent.parent.parent.parent, P())
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
        self.assertEqual(par[2], P('.'))
        self.assertEqual(par[-1], P('.'))
        self.assertEqual(par[-2], P('a'))
        self.assertEqual(par[-3], P('a/b'))
        self.assertEqual(par[0:1], (P('a/b'),))
        self.assertEqual(par[:2], (P('a/b'), P('a')))
        self.assertEqual(par[:-1], (P('a/b'), P('a')))
        self.assertEqual(par[1:], (P('a'), P('.')))
        self.assertEqual(par[::2], (P('a/b'), P('.')))
        self.assertEqual(par[::-1], (P('.'), P('a'), P('a/b')))
        self.assertEqual(list(par), [P('a/b'), P('a'), P('.')])
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
        self.assertEqual(par[0:1], (P('/a/b'),))
        self.assertEqual(par[:2], (P('/a/b'), P('/a')))
        self.assertEqual(par[:-1], (P('/a/b'), P('/a')))
        self.assertEqual(par[1:], (P('/a'), P('/')))
        self.assertEqual(par[::2], (P('/a/b'), P('/')))
        self.assertEqual(par[::-1], (P('/'), P('/a'), P('/a/b')))
        self.assertEqual(list(par), [P('/a/b'), P('/a'), P('/')])
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

    def test_name_common(self):
        P = self.cls
        self.assertEqual(P('').name, '')
        self.assertEqual(P('.').name, '')
        self.assertEqual(P('/').name, '')
        self.assertEqual(P('a/b').name, 'b')
        self.assertEqual(P('/a/b').name, 'b')
        self.assertEqual(P('/a/b/.').name, 'b')
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

    def test_stem_common(self):
        P = self.cls
        self.assertEqual(P('').stem, '')
        self.assertEqual(P('.').stem, '')
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
        self.assertRaises(ValueError, P('').with_name, 'd.xml')
        self.assertRaises(ValueError, P('.').with_name, 'd.xml')
        self.assertRaises(ValueError, P('/').with_name, 'd.xml')
        self.assertRaises(ValueError, P('a/b').with_name, '')
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
        self.assertRaises(ValueError, P('').with_stem, 'd')
        self.assertRaises(ValueError, P('.').with_stem, 'd')
        self.assertRaises(ValueError, P('/').with_stem, 'd')
        self.assertRaises(ValueError, P('a/b').with_stem, '')
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
        # Path doesn't have a "filename" component.
        self.assertRaises(ValueError, P('').with_suffix, '.gz')
        self.assertRaises(ValueError, P('.').with_suffix, '.gz')
        self.assertRaises(ValueError, P('/').with_suffix, '.gz')
        # Invalid suffix.
        self.assertRaises(ValueError, P('a/b').with_suffix, 'gz')
        self.assertRaises(ValueError, P('a/b').with_suffix, '/')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.')
        self.assertRaises(ValueError, P('a/b').with_suffix, '/.gz')
        self.assertRaises(ValueError, P('a/b').with_suffix, 'c/d')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.c/.d')
        self.assertRaises(ValueError, P('a/b').with_suffix, './.d')
        self.assertRaises(ValueError, P('a/b').with_suffix, '.d/.')
        self.assertRaises(ValueError, P('a/b').with_suffix,
                          (self.flavour.sep, 'd'))

    def test_relative_to_common(self):
        P = self.cls
        p = P('a/b')
        self.assertRaises(TypeError, p.relative_to)
        self.assertRaises(TypeError, p.relative_to, b'a')
        self.assertEqual(p.relative_to(P()), P('a/b'))
        self.assertEqual(p.relative_to(''), P('a/b'))
        self.assertEqual(p.relative_to(P('a')), P('b'))
        self.assertEqual(p.relative_to('a'), P('b'))
        self.assertEqual(p.relative_to('a/'), P('b'))
        self.assertEqual(p.relative_to(P('a/b')), P())
        self.assertEqual(p.relative_to('a/b'), P())
        # With several args.
        self.assertEqual(p.relative_to('a', 'b'), P())
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, P('c'))
        self.assertRaises(ValueError, p.relative_to, P('a/b/c'))
        self.assertRaises(ValueError, p.relative_to, P('a/c'))
        self.assertRaises(ValueError, p.relative_to, P('/a'))
        p = P('/a/b')
        self.assertEqual(p.relative_to(P('/')), P('a/b'))
        self.assertEqual(p.relative_to('/'), P('a/b'))
        self.assertEqual(p.relative_to(P('/a')), P('b'))
        self.assertEqual(p.relative_to('/a'), P('b'))
        self.assertEqual(p.relative_to('/a/'), P('b'))
        self.assertEqual(p.relative_to(P('/a/b')), P())
        self.assertEqual(p.relative_to('/a/b'), P())
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, P('/c'))
        self.assertRaises(ValueError, p.relative_to, P('/a/b/c'))
        self.assertRaises(ValueError, p.relative_to, P('/a/c'))
        self.assertRaises(ValueError, p.relative_to, P())
        self.assertRaises(ValueError, p.relative_to, '')
        self.assertRaises(ValueError, p.relative_to, P('a'))

    def test_is_relative_to_common(self):
        P = self.cls
        p = P('a/b')
        self.assertRaises(TypeError, p.is_relative_to)
        self.assertRaises(TypeError, p.is_relative_to, b'a')
        self.assertTrue(p.is_relative_to(P()))
        self.assertTrue(p.is_relative_to(''))
        self.assertTrue(p.is_relative_to(P('a')))
        self.assertTrue(p.is_relative_to('a/'))
        self.assertTrue(p.is_relative_to(P('a/b')))
        self.assertTrue(p.is_relative_to('a/b'))
        # With several args.
        self.assertTrue(p.is_relative_to('a', 'b'))
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
        self.assertFalse(p.is_relative_to(P()))
        self.assertFalse(p.is_relative_to(''))
        self.assertFalse(p.is_relative_to(P('a')))

    def test_pickling_common(self):
        P = self.cls
        p = P('/a/b')
        for proto in range(0, pickle.HIGHEST_PROTOCOL + 1):
            dumped = pickle.dumps(p, proto)
            pp = pickle.loads(dumped)
            self.assertIs(pp.__class__, p.__class__)
            self.assertEqual(pp, p)
            self.assertEqual(hash(pp), hash(p))
            self.assertEqual(str(pp), str(p))


class PurePosixPathTest(_BasePurePathTest, unittest.TestCase):
    cls = pathlib.PurePosixPath

    def test_root(self):
        P = self.cls
        self.assertEqual(P('/a/b').root, '/')
        self.assertEqual(P('///a/b').root, '/')
        # POSIX special case for two leading slashes.
        self.assertEqual(P('//a/b').root, '//')

    def test_eq(self):
        P = self.cls
        self.assertNotEqual(P('a/b'), P('A/b'))
        self.assertEqual(P('/a'), P('///a'))
        self.assertNotEqual(P('/a'), P('//a'))

    def test_as_uri(self):
        P = self.cls
        self.assertEqual(P('/').as_uri(), 'file:///')
        self.assertEqual(P('/a/b.c').as_uri(), 'file:///a/b.c')
        self.assertEqual(P('/a/b%#c').as_uri(), 'file:///a/b%25%23c')

    def test_as_uri_non_ascii(self):
        from urllib.parse import quote_from_bytes
        P = self.cls
        try:
            os.fsencode('\xe9')
        except UnicodeEncodeError:
            self.skipTest("\\xe9 cannot be encoded to the filesystem encoding")
        self.assertEqual(P('/a/b\xe9').as_uri(),
                         'file:///a/b' + quote_from_bytes(os.fsencode('\xe9')))

    def test_match(self):
        P = self.cls
        self.assertFalse(P('A.py').match('a.PY'))

    def test_is_absolute(self):
        P = self.cls
        self.assertFalse(P().is_absolute())
        self.assertFalse(P('a').is_absolute())
        self.assertFalse(P('a/b/').is_absolute())
        self.assertTrue(P('/').is_absolute())
        self.assertTrue(P('/a').is_absolute())
        self.assertTrue(P('/a/b/').is_absolute())
        self.assertTrue(P('//a').is_absolute())
        self.assertTrue(P('//a/b').is_absolute())

    def test_is_reserved(self):
        P = self.cls
        self.assertIs(False, P('').is_reserved())
        self.assertIs(False, P('/').is_reserved())
        self.assertIs(False, P('/foo/bar').is_reserved())
        self.assertIs(False, P('/dev/con/PRN/NUL').is_reserved())

    def test_join(self):
        P = self.cls
        p = P('//a')
        pp = p.joinpath('b')
        self.assertEqual(pp, P('//a/b'))
        pp = P('/a').joinpath('//c')
        self.assertEqual(pp, P('//c'))
        pp = P('//a').joinpath('/c')
        self.assertEqual(pp, P('/c'))

    def test_div(self):
        # Basically the same as joinpath().
        P = self.cls
        p = P('//a')
        pp = p / 'b'
        self.assertEqual(pp, P('//a/b'))
        pp = P('/a') / '//c'
        self.assertEqual(pp, P('//c'))
        pp = P('//a') / '/c'
        self.assertEqual(pp, P('/c'))


class PureWindowsPathTest(_BasePurePathTest, unittest.TestCase):
    cls = pathlib.PureWindowsPath

    equivalences = _BasePurePathTest.equivalences.copy()
    equivalences.update({
        'c:a': [ ('c:', 'a'), ('c:', 'a/'), ('/', 'c:', 'a') ],
        'c:/a': [
            ('c:/', 'a'), ('c:', '/', 'a'), ('c:', '/a'),
            ('/z', 'c:/', 'a'), ('//x/y', 'c:/', 'a'),
            ],
        '//a/b/': [ ('//a/b',) ],
        '//a/b/c': [
            ('//a/b', 'c'), ('//a/b/', 'c'),
            ],
    })

    def test_str(self):
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

    def test_str_subclass(self):
        self._check_str_subclass('c:')
        self._check_str_subclass('c:a')
        self._check_str_subclass('c:a\\b.txt')
        self._check_str_subclass('c:\\')
        self._check_str_subclass('c:\\a')
        self._check_str_subclass('c:\\a\\b.txt')
        self._check_str_subclass('\\\\some\\share')
        self._check_str_subclass('\\\\some\\share\\a')
        self._check_str_subclass('\\\\some\\share\\a\\b.txt')

    def test_eq(self):
        P = self.cls
        self.assertEqual(P('c:a/b'), P('c:a/b'))
        self.assertEqual(P('c:a/b'), P('c:', 'a', 'b'))
        self.assertNotEqual(P('c:a/b'), P('d:a/b'))
        self.assertNotEqual(P('c:a/b'), P('c:/a/b'))
        self.assertNotEqual(P('/a/b'), P('c:/a/b'))
        # Case-insensitivity.
        self.assertEqual(P('a/B'), P('A/b'))
        self.assertEqual(P('C:a/B'), P('c:A/b'))
        self.assertEqual(P('//Some/SHARE/a/B'), P('//somE/share/A/b'))

    def test_as_uri(self):
        P = self.cls
        with self.assertRaises(ValueError):
            P('/a/b').as_uri()
        with self.assertRaises(ValueError):
            P('c:a/b').as_uri()
        self.assertEqual(P('c:/').as_uri(), 'file:///c:/')
        self.assertEqual(P('c:/a/b.c').as_uri(), 'file:///c:/a/b.c')
        self.assertEqual(P('c:/a/b%#c').as_uri(), 'file:///c:/a/b%25%23c')
        self.assertEqual(P('c:/a/b\xe9').as_uri(), 'file:///c:/a/b%C3%A9')
        self.assertEqual(P('//some/share/').as_uri(), 'file://some/share/')
        self.assertEqual(P('//some/share/a/b.c').as_uri(),
                         'file://some/share/a/b.c')
        self.assertEqual(P('//some/share/a/b%#c\xe9').as_uri(),
                         'file://some/share/a/b%25%23c%C3%A9')

    def test_match_common(self):
        P = self.cls
        # Absolute patterns.
        self.assertTrue(P('c:/b.py').match('/*.py'))
        self.assertTrue(P('c:/b.py').match('c:*.py'))
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
        self.assertTrue(P('//some/share/a.py').match('/*.py'))
        self.assertTrue(P('//some/share/a.py').match('//some/share/*.py'))
        self.assertFalse(P('//other/share/a.py').match('//some/share/*.py'))
        self.assertFalse(P('//some/share/a/b.py').match('//some/share/*.py'))
        # Case-insensitivity.
        self.assertTrue(P('B.py').match('b.PY'))
        self.assertTrue(P('c:/a/B.Py').match('C:/A/*.pY'))
        self.assertTrue(P('//Some/Share/B.Py').match('//somE/sharE/*.pY'))

    def test_ordering_common(self):
        # Case-insensitivity.
        def assertOrderedEqual(a, b):
            self.assertLessEqual(a, b)
            self.assertGreaterEqual(b, a)
        P = self.cls
        p = P('c:A/b')
        q = P('C:a/B')
        assertOrderedEqual(p, q)
        self.assertFalse(p < q)
        self.assertFalse(p > q)
        p = P('//some/Share/A/b')
        q = P('//Some/SHARE/a/B')
        assertOrderedEqual(p, q)
        self.assertFalse(p < q)
        self.assertFalse(p > q)

    def test_parts(self):
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
        self.assertEqual(p.parent.parent, P('//a/b'))
        self.assertEqual(p.parent.parent.parent, P('//a/b'))

    def test_parents(self):
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

    def test_drive(self):
        P = self.cls
        self.assertEqual(P('c:').drive, 'c:')
        self.assertEqual(P('c:a/b').drive, 'c:')
        self.assertEqual(P('c:/').drive, 'c:')
        self.assertEqual(P('c:/a/b/').drive, 'c:')
        self.assertEqual(P('//a/b').drive, '\\\\a\\b')
        self.assertEqual(P('//a/b/').drive, '\\\\a\\b')
        self.assertEqual(P('//a/b/c/d').drive, '\\\\a\\b')

    def test_root(self):
        P = self.cls
        self.assertEqual(P('c:').root, '')
        self.assertEqual(P('c:a/b').root, '')
        self.assertEqual(P('c:/').root, '\\')
        self.assertEqual(P('c:/a/b/').root, '\\')
        self.assertEqual(P('//a/b').root, '\\')
        self.assertEqual(P('//a/b/').root, '\\')
        self.assertEqual(P('//a/b/c/d').root, '\\')

    def test_anchor(self):
        P = self.cls
        self.assertEqual(P('c:').anchor, 'c:')
        self.assertEqual(P('c:a/b').anchor, 'c:')
        self.assertEqual(P('c:/').anchor, 'c:\\')
        self.assertEqual(P('c:/a/b/').anchor, 'c:\\')
        self.assertEqual(P('//a/b').anchor, '\\\\a\\b\\')
        self.assertEqual(P('//a/b/').anchor, '\\\\a\\b\\')
        self.assertEqual(P('//a/b/c/d').anchor, '\\\\a\\b\\')

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
        self.assertEqual(P('c:a/Some name. Ending with a dot.').suffix, '')
        self.assertEqual(P('c:/a/Some name. Ending with a dot.').suffix, '')
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
        self.assertEqual(P('c:a/Some name. Ending with a dot.').suffixes, [])
        self.assertEqual(P('c:/a/Some name. Ending with a dot.').suffixes, [])

    def test_stem(self):
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
        self.assertEqual(P('c:a/Some name. Ending with a dot.').stem,
                         'Some name. Ending with a dot.')

    def test_with_name(self):
        P = self.cls
        self.assertEqual(P('c:a/b').with_name('d.xml'), P('c:a/d.xml'))
        self.assertEqual(P('c:/a/b').with_name('d.xml'), P('c:/a/d.xml'))
        self.assertEqual(P('c:a/Dot ending.').with_name('d.xml'), P('c:a/d.xml'))
        self.assertEqual(P('c:/a/Dot ending.').with_name('d.xml'), P('c:/a/d.xml'))
        self.assertRaises(ValueError, P('c:').with_name, 'd.xml')
        self.assertRaises(ValueError, P('c:/').with_name, 'd.xml')
        self.assertRaises(ValueError, P('//My/Share').with_name, 'd.xml')
        self.assertRaises(ValueError, P('c:a/b').with_name, 'd:')
        self.assertRaises(ValueError, P('c:a/b').with_name, 'd:e')
        self.assertRaises(ValueError, P('c:a/b').with_name, 'd:/e')
        self.assertRaises(ValueError, P('c:a/b').with_name, '//My/Share')

    def test_with_stem(self):
        P = self.cls
        self.assertEqual(P('c:a/b').with_stem('d'), P('c:a/d'))
        self.assertEqual(P('c:/a/b').with_stem('d'), P('c:/a/d'))
        self.assertEqual(P('c:a/Dot ending.').with_stem('d'), P('c:a/d'))
        self.assertEqual(P('c:/a/Dot ending.').with_stem('d'), P('c:/a/d'))
        self.assertRaises(ValueError, P('c:').with_stem, 'd')
        self.assertRaises(ValueError, P('c:/').with_stem, 'd')
        self.assertRaises(ValueError, P('//My/Share').with_stem, 'd')
        self.assertRaises(ValueError, P('c:a/b').with_stem, 'd:')
        self.assertRaises(ValueError, P('c:a/b').with_stem, 'd:e')
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

    def test_relative_to(self):
        P = self.cls
        p = P('C:Foo/Bar')
        self.assertEqual(p.relative_to(P('c:')), P('Foo/Bar'))
        self.assertEqual(p.relative_to('c:'), P('Foo/Bar'))
        self.assertEqual(p.relative_to(P('c:foO')), P('Bar'))
        self.assertEqual(p.relative_to('c:foO'), P('Bar'))
        self.assertEqual(p.relative_to('c:foO/'), P('Bar'))
        self.assertEqual(p.relative_to(P('c:foO/baR')), P())
        self.assertEqual(p.relative_to('c:foO/baR'), P())
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
        p = P('C:/Foo/Bar')
        self.assertEqual(p.relative_to(P('c:')), P('/Foo/Bar'))
        self.assertEqual(p.relative_to('c:'), P('/Foo/Bar'))
        self.assertEqual(str(p.relative_to(P('c:'))), '\\Foo\\Bar')
        self.assertEqual(str(p.relative_to('c:')), '\\Foo\\Bar')
        self.assertEqual(p.relative_to(P('c:/')), P('Foo/Bar'))
        self.assertEqual(p.relative_to('c:/'), P('Foo/Bar'))
        self.assertEqual(p.relative_to(P('c:/foO')), P('Bar'))
        self.assertEqual(p.relative_to('c:/foO'), P('Bar'))
        self.assertEqual(p.relative_to('c:/foO/'), P('Bar'))
        self.assertEqual(p.relative_to(P('c:/foO/baR')), P())
        self.assertEqual(p.relative_to('c:/foO/baR'), P())
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, P('C:/Baz'))
        self.assertRaises(ValueError, p.relative_to, P('C:/Foo/Bar/Baz'))
        self.assertRaises(ValueError, p.relative_to, P('C:/Foo/Baz'))
        self.assertRaises(ValueError, p.relative_to, P('C:Foo'))
        self.assertRaises(ValueError, p.relative_to, P('d:'))
        self.assertRaises(ValueError, p.relative_to, P('d:/'))
        self.assertRaises(ValueError, p.relative_to, P('/'))
        self.assertRaises(ValueError, p.relative_to, P('/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('//C/Foo'))
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
        # Unrelated paths.
        self.assertRaises(ValueError, p.relative_to, P('/Server/Share/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('c:/Server/Share/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('//z/Share/Foo'))
        self.assertRaises(ValueError, p.relative_to, P('//Server/z/Foo'))

    def test_is_relative_to(self):
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
        self.assertTrue(p.is_relative_to('c:'))
        self.assertTrue(p.is_relative_to(P('c:/')))
        self.assertTrue(p.is_relative_to(P('c:/foO')))
        self.assertTrue(p.is_relative_to('c:/foO/'))
        self.assertTrue(p.is_relative_to(P('c:/foO/baR')))
        self.assertTrue(p.is_relative_to('c:/foO/baR'))
        # Unrelated paths.
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

    def test_is_absolute(self):
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
        self.assertTrue(P('//a/b').is_absolute())
        self.assertTrue(P('//a/b/').is_absolute())
        self.assertTrue(P('//a/b/c').is_absolute())
        self.assertTrue(P('//a/b/c/d').is_absolute())

    def test_join(self):
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

    def test_div(self):
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

    def test_is_reserved(self):
        P = self.cls
        self.assertIs(False, P('').is_reserved())
        self.assertIs(False, P('/').is_reserved())
        self.assertIs(False, P('/foo/bar').is_reserved())
        # UNC paths are never reserved.
        self.assertIs(False, P('//my/share/nul/con/aux').is_reserved())
        # Case-insensitive DOS-device names are reserved.
        self.assertIs(True, P('nul').is_reserved())
        self.assertIs(True, P('aux').is_reserved())
        self.assertIs(True, P('prn').is_reserved())
        self.assertIs(True, P('con').is_reserved())
        self.assertIs(True, P('conin$').is_reserved())
        self.assertIs(True, P('conout$').is_reserved())
        # COM/LPT + 1-9 or + superscript 1-3 are reserved.
        self.assertIs(True, P('COM1').is_reserved())
        self.assertIs(True, P('LPT9').is_reserved())
        self.assertIs(True, P('com\xb9').is_reserved())
        self.assertIs(True, P('com\xb2').is_reserved())
        self.assertIs(True, P('lpt\xb3').is_reserved())
        # DOS-device name mataching ignores characters after a dot or
        # a colon and also ignores trailing spaces.
        self.assertIs(True, P('NUL.txt').is_reserved())
        self.assertIs(True, P('PRN  ').is_reserved())
        self.assertIs(True, P('AUX  .txt').is_reserved())
        self.assertIs(True, P('COM1:bar').is_reserved())
        self.assertIs(True, P('LPT9   :bar').is_reserved())
        # DOS-device names are only matched at the beginning
        # of a path component.
        self.assertIs(False, P('bar.com9').is_reserved())
        self.assertIs(False, P('bar.lpt9').is_reserved())
        # Only the last path component matters.
        self.assertIs(True, P('c:/baz/con/NUL').is_reserved())
        self.assertIs(False, P('c:/NUL/con/baz').is_reserved())

class PurePathTest(_BasePurePathTest, unittest.TestCase):
    cls = pathlib.PurePath

    def test_concrete_class(self):
        p = self.cls('a')
        self.assertIs(type(p),
            pathlib.PureWindowsPath if os.name == 'nt' else pathlib.PurePosixPath)

    def test_different_flavours_unequal(self):
        p = pathlib.PurePosixPath('a')
        q = pathlib.PureWindowsPath('a')
        self.assertNotEqual(p, q)

    def test_different_flavours_unordered(self):
        p = pathlib.PurePosixPath('a')
        q = pathlib.PureWindowsPath('a')
        with self.assertRaises(TypeError):
            p < q
        with self.assertRaises(TypeError):
            p <= q
        with self.assertRaises(TypeError):
            p > q
        with self.assertRaises(TypeError):
            p >= q


#
# Tests for the concrete classes.
#

# Make sure any symbolic links in the base test path are resolved.
BASE = os.path.realpath(TESTFN)
join = lambda *x: os.path.join(BASE, *x)
rel_join = lambda *x: os.path.join(TESTFN, *x)

only_nt = unittest.skipIf(os.name != 'nt',
                          'test requires a Windows-compatible system')
only_posix = unittest.skipIf(os.name == 'nt',
                             'test requires a POSIX-compatible system')

@only_posix
class PosixPathAsPureTest(PurePosixPathTest):
    cls = pathlib.PosixPath

@only_nt
class WindowsPathAsPureTest(PureWindowsPathTest):
    cls = pathlib.WindowsPath

    def test_owner(self):
        P = self.cls
        with self.assertRaises(NotImplementedError):
            P('c:/').owner()

    def test_group(self):
        P = self.cls
        with self.assertRaises(NotImplementedError):
            P('c:/').group()


class _BasePathTest(object):
    """Tests for the FS-accessing functionalities of the Path classes."""

    # (BASE)
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
    #  |-- dirE  # No permissions
    #  |-- fileA
    #  |-- linkA -> fileA
    #  |-- linkB -> dirB
    #  `-- brokenLinkLoop -> brokenLinkLoop
    #

    def setUp(self):
        def cleanup():
            os.chmod(join('dirE'), 0o777)
            os_helper.rmtree(BASE)
        self.addCleanup(cleanup)
        os.mkdir(BASE)
        os.mkdir(join('dirA'))
        os.mkdir(join('dirB'))
        os.mkdir(join('dirC'))
        os.mkdir(join('dirC', 'dirD'))
        os.mkdir(join('dirE'))
        with open(join('fileA'), 'wb') as f:
            f.write(b"this is file A\n")
        with open(join('dirB', 'fileB'), 'wb') as f:
            f.write(b"this is file B\n")
        with open(join('dirC', 'fileC'), 'wb') as f:
            f.write(b"this is file C\n")
        with open(join('dirC', 'dirD', 'fileD'), 'wb') as f:
            f.write(b"this is file D\n")
        os.chmod(join('dirE'), 0)
        if os_helper.can_symlink():
            # Relative symlinks.
            os.symlink('fileA', join('linkA'))
            os.symlink('non-existing', join('brokenLink'))
            self.dirlink('dirB', join('linkB'))
            self.dirlink(os.path.join('..', 'dirB'), join('dirA', 'linkC'))
            # This one goes upwards, creating a loop.
            self.dirlink(os.path.join('..', 'dirB'), join('dirB', 'linkD'))
            # Broken symlink (pointing to itself).
            os.symlink('brokenLinkLoop',  join('brokenLinkLoop'))

    if os.name == 'nt':
        # Workaround for http://bugs.python.org/issue13772.
        def dirlink(self, src, dest):
            os.symlink(src, dest, target_is_directory=True)
    else:
        def dirlink(self, src, dest):
            os.symlink(src, dest)

    def assertSame(self, path_a, path_b):
        self.assertTrue(os.path.samefile(str(path_a), str(path_b)),
                        "%r and %r don't point to the same file" %
                        (path_a, path_b))

    def assertFileNotFound(self, func, *args, **kwargs):
        with self.assertRaises(FileNotFoundError) as cm:
            func(*args, **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENOENT)

    def assertEqualNormCase(self, path_a, path_b):
        self.assertEqual(os.path.normcase(path_a), os.path.normcase(path_b))

    def _test_cwd(self, p):
        q = self.cls(os.getcwd())
        self.assertEqual(p, q)
        self.assertEqualNormCase(str(p), str(q))
        self.assertIs(type(p), type(q))
        self.assertTrue(p.is_absolute())

    def test_cwd(self):
        p = self.cls.cwd()
        self._test_cwd(p)

    def _test_home(self, p):
        q = self.cls(os.path.expanduser('~'))
        self.assertEqual(p, q)
        self.assertEqualNormCase(str(p), str(q))
        self.assertIs(type(p), type(q))
        self.assertTrue(p.is_absolute())

    def test_home(self):
        with os_helper.EnvironmentVarGuard() as env:
            self._test_home(self.cls.home())

            env.clear()
            env['USERPROFILE'] = os.path.join(BASE, 'userprofile')
            self._test_home(self.cls.home())

            # bpo-38883: ignore `HOME` when set on windows
            env['HOME'] = os.path.join(BASE, 'home')
            self._test_home(self.cls.home())

    def test_samefile(self):
        fileA_path = os.path.join(BASE, 'fileA')
        fileB_path = os.path.join(BASE, 'dirB', 'fileB')
        p = self.cls(fileA_path)
        pp = self.cls(fileA_path)
        q = self.cls(fileB_path)
        self.assertTrue(p.samefile(fileA_path))
        self.assertTrue(p.samefile(pp))
        self.assertFalse(p.samefile(fileB_path))
        self.assertFalse(p.samefile(q))
        # Test the non-existent file case
        non_existent = os.path.join(BASE, 'foo')
        r = self.cls(non_existent)
        self.assertRaises(FileNotFoundError, p.samefile, r)
        self.assertRaises(FileNotFoundError, p.samefile, non_existent)
        self.assertRaises(FileNotFoundError, r.samefile, p)
        self.assertRaises(FileNotFoundError, r.samefile, non_existent)
        self.assertRaises(FileNotFoundError, r.samefile, r)
        self.assertRaises(FileNotFoundError, r.samefile, non_existent)

    def test_empty_path(self):
        # The empty path points to '.'
        p = self.cls('')
        self.assertEqual(p.stat(), os.stat('.'))

    def test_expanduser_common(self):
        P = self.cls
        p = P('~')
        self.assertEqual(p.expanduser(), P(os.path.expanduser('~')))
        p = P('foo')
        self.assertEqual(p.expanduser(), p)
        p = P('/~')
        self.assertEqual(p.expanduser(), p)
        p = P('../~')
        self.assertEqual(p.expanduser(), p)
        p = P(P('').absolute().anchor) / '~'
        self.assertEqual(p.expanduser(), p)

    def test_exists(self):
        P = self.cls
        p = P(BASE)
        self.assertIs(True, p.exists())
        self.assertIs(True, (p / 'dirA').exists())
        self.assertIs(True, (p / 'fileA').exists())
        self.assertIs(False, (p / 'fileA' / 'bah').exists())
        if os_helper.can_symlink():
            self.assertIs(True, (p / 'linkA').exists())
            self.assertIs(True, (p / 'linkB').exists())
            self.assertIs(True, (p / 'linkB' / 'fileB').exists())
            self.assertIs(False, (p / 'linkA' / 'bah').exists())
        self.assertIs(False, (p / 'foo').exists())
        self.assertIs(False, P('/xyzzy').exists())
        self.assertIs(False, P(BASE + '\udfff').exists())
        self.assertIs(False, P(BASE + '\x00').exists())

    def test_open_common(self):
        p = self.cls(BASE)
        with (p / 'fileA').open('r') as f:
            self.assertIsInstance(f, io.TextIOBase)
            self.assertEqual(f.read(), "this is file A\n")
        with (p / 'fileA').open('rb') as f:
            self.assertIsInstance(f, io.BufferedIOBase)
            self.assertEqual(f.read().strip(), b"this is file A")
        with (p / 'fileA').open('rb', buffering=0) as f:
            self.assertIsInstance(f, io.RawIOBase)
            self.assertEqual(f.read().strip(), b"this is file A")

    def test_read_write_bytes(self):
        p = self.cls(BASE)
        (p / 'fileA').write_bytes(b'abcdefg')
        self.assertEqual((p / 'fileA').read_bytes(), b'abcdefg')
        # Check that trying to write str does not truncate the file.
        self.assertRaises(TypeError, (p / 'fileA').write_bytes, 'somestr')
        self.assertEqual((p / 'fileA').read_bytes(), b'abcdefg')

    def test_read_write_text(self):
        p = self.cls(BASE)
        (p / 'fileA').write_text('äbcdefg', encoding='latin-1')
        self.assertEqual((p / 'fileA').read_text(
            encoding='utf-8', errors='ignore'), 'bcdefg')
        # Check that trying to write bytes does not truncate the file.
        self.assertRaises(TypeError, (p / 'fileA').write_text, b'somebytes')
        self.assertEqual((p / 'fileA').read_text(encoding='latin-1'), 'äbcdefg')

    def test_write_text_with_newlines(self):
        p = self.cls(BASE)
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
        p = P(BASE)
        it = p.iterdir()
        paths = set(it)
        expected = ['dirA', 'dirB', 'dirC', 'dirE', 'fileA']
        if os_helper.can_symlink():
            expected += ['linkA', 'linkB', 'brokenLink', 'brokenLinkLoop']
        self.assertEqual(paths, { P(BASE, q) for q in expected })

    @os_helper.skip_unless_symlink
    def test_iterdir_symlink(self):
        # __iter__ on a symlink to a directory.
        P = self.cls
        p = P(BASE, 'linkB')
        paths = set(p.iterdir())
        expected = { P(BASE, 'linkB', q) for q in ['fileB', 'linkD'] }
        self.assertEqual(paths, expected)

    def test_iterdir_nodir(self):
        # __iter__ on something that is not a directory.
        p = self.cls(BASE, 'fileA')
        with self.assertRaises(OSError) as cm:
            next(p.iterdir())
        # ENOENT or EINVAL under Windows, ENOTDIR otherwise
        # (see issue #12802).
        self.assertIn(cm.exception.errno, (errno.ENOTDIR,
                                           errno.ENOENT, errno.EINVAL))

    def test_glob_common(self):
        def _check(glob, expected):
            self.assertEqual(set(glob), { P(BASE, q) for q in expected })
        P = self.cls
        p = P(BASE)
        it = p.glob("fileA")
        self.assertIsInstance(it, collections.abc.Iterator)
        _check(it, ["fileA"])
        _check(p.glob("fileB"), [])
        _check(p.glob("dir*/file*"), ["dirB/fileB", "dirC/fileC"])
        if not os_helper.can_symlink():
            _check(p.glob("*A"), ['dirA', 'fileA'])
        else:
            _check(p.glob("*A"), ['dirA', 'fileA', 'linkA'])
        if not os_helper.can_symlink():
            _check(p.glob("*B/*"), ['dirB/fileB'])
        else:
            _check(p.glob("*B/*"), ['dirB/fileB', 'dirB/linkD',
                                    'linkB/fileB', 'linkB/linkD'])
        if not os_helper.can_symlink():
            _check(p.glob("*/fileB"), ['dirB/fileB'])
        else:
            _check(p.glob("*/fileB"), ['dirB/fileB', 'linkB/fileB'])

    def test_rglob_common(self):
        def _check(glob, expected):
            self.assertEqual(set(glob), { P(BASE, q) for q in expected })
        P = self.cls
        p = P(BASE)
        it = p.rglob("fileA")
        self.assertIsInstance(it, collections.abc.Iterator)
        _check(it, ["fileA"])
        _check(p.rglob("fileB"), ["dirB/fileB"])
        _check(p.rglob("*/fileA"), [])
        if not os_helper.can_symlink():
            _check(p.rglob("*/fileB"), ["dirB/fileB"])
        else:
            _check(p.rglob("*/fileB"), ["dirB/fileB", "dirB/linkD/fileB",
                                        "linkB/fileB", "dirA/linkC/fileB"])
        _check(p.rglob("file*"), ["fileA", "dirB/fileB",
                                  "dirC/fileC", "dirC/dirD/fileD"])
        p = P(BASE, "dirC")
        _check(p.rglob("file*"), ["dirC/fileC", "dirC/dirD/fileD"])
        _check(p.rglob("*/*"), ["dirC/dirD/fileD"])

    @os_helper.skip_unless_symlink
    def test_rglob_symlink_loop(self):
        # Don't get fooled by symlink loops (Issue #26012).
        P = self.cls
        p = P(BASE)
        given = set(p.rglob('*'))
        expect = {'brokenLink',
                  'dirA', 'dirA/linkC',
                  'dirB', 'dirB/fileB', 'dirB/linkD',
                  'dirC', 'dirC/dirD', 'dirC/dirD/fileD', 'dirC/fileC',
                  'dirE',
                  'fileA',
                  'linkA',
                  'linkB',
                  'brokenLinkLoop',
                  }
        self.assertEqual(given, {p / x for x in expect})

    def test_glob_many_open_files(self):
        depth = 30
        P = self.cls
        base = P(BASE) / 'deep'
        p = P(base, *(['d']*depth))
        p.mkdir(parents=True)
        pattern = '/'.join(['*'] * depth)
        iters = [base.glob(pattern) for j in range(100)]
        for it in iters:
            self.assertEqual(next(it), p)
        iters = [base.rglob('d') for j in range(100)]
        p = base
        for i in range(depth):
            p = p / 'd'
            for it in iters:
                self.assertEqual(next(it), p)

    def test_glob_dotdot(self):
        # ".." is not special in globs.
        P = self.cls
        p = P(BASE)
        self.assertEqual(set(p.glob("..")), { P(BASE, "..") })
        self.assertEqual(set(p.glob("dirA/../file*")), { P(BASE, "dirA/../fileA") })
        self.assertEqual(set(p.glob("../xyzzy")), set())

    @os_helper.skip_unless_symlink
    def test_glob_permissions(self):
        # See bpo-38894
        P = self.cls
        base = P(BASE) / 'permissions'
        base.mkdir()

        file1 = base / "file1"
        file1.touch()
        file2 = base / "file2"
        file2.touch()

        subdir = base / "subdir"

        file3 = base / "file3"
        file3.symlink_to(subdir / "other")

        # Patching is needed to avoid relying on the filesystem
        # to return the order of the files as the error will not
        # happen if the symlink is the last item.

        with mock.patch("os.scandir") as scandir:
            scandir.return_value = sorted(os.scandir(base))
            self.assertEqual(len(set(base.glob("*"))), 3)

        subdir.mkdir()

        with mock.patch("os.scandir") as scandir:
            scandir.return_value = sorted(os.scandir(base))
            self.assertEqual(len(set(base.glob("*"))), 4)

        subdir.chmod(000)

        with mock.patch("os.scandir") as scandir:
            scandir.return_value = sorted(os.scandir(base))
            self.assertEqual(len(set(base.glob("*"))), 4)

    def _check_resolve(self, p, expected, strict=True):
        q = p.resolve(strict)
        self.assertEqual(q, expected)

    # This can be used to check both relative and absolute resolutions.
    _check_resolve_relative = _check_resolve_absolute = _check_resolve

    @os_helper.skip_unless_symlink
    def test_resolve_common(self):
        P = self.cls
        p = P(BASE, 'foo')
        with self.assertRaises(OSError) as cm:
            p.resolve(strict=True)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        # Non-strict
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 os.path.join(BASE, 'foo'))
        p = P(BASE, 'foo', 'in', 'spam')
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 os.path.join(BASE, 'foo', 'in', 'spam'))
        p = P(BASE, '..', 'foo', 'in', 'spam')
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 os.path.abspath(os.path.join('foo', 'in', 'spam')))
        # These are all relative symlinks.
        p = P(BASE, 'dirB', 'fileB')
        self._check_resolve_relative(p, p)
        p = P(BASE, 'linkA')
        self._check_resolve_relative(p, P(BASE, 'fileA'))
        p = P(BASE, 'dirA', 'linkC', 'fileB')
        self._check_resolve_relative(p, P(BASE, 'dirB', 'fileB'))
        p = P(BASE, 'dirB', 'linkD', 'fileB')
        self._check_resolve_relative(p, P(BASE, 'dirB', 'fileB'))
        # Non-strict
        p = P(BASE, 'dirA', 'linkC', 'fileB', 'foo', 'in', 'spam')
        self._check_resolve_relative(p, P(BASE, 'dirB', 'fileB', 'foo', 'in',
                                          'spam'), False)
        p = P(BASE, 'dirA', 'linkC', '..', 'foo', 'in', 'spam')
        if os.name == 'nt':
            # In Windows, if linkY points to dirB, 'dirA\linkY\..'
            # resolves to 'dirA' without resolving linkY first.
            self._check_resolve_relative(p, P(BASE, 'dirA', 'foo', 'in',
                                              'spam'), False)
        else:
            # In Posix, if linkY points to dirB, 'dirA/linkY/..'
            # resolves to 'dirB/..' first before resolving to parent of dirB.
            self._check_resolve_relative(p, P(BASE, 'foo', 'in', 'spam'), False)
        # Now create absolute symlinks.
        d = os_helper._longpath(tempfile.mkdtemp(suffix='-dirD',
                                                 dir=os.getcwd()))
        self.addCleanup(os_helper.rmtree, d)
        os.symlink(os.path.join(d), join('dirA', 'linkX'))
        os.symlink(join('dirB'), os.path.join(d, 'linkY'))
        p = P(BASE, 'dirA', 'linkX', 'linkY', 'fileB')
        self._check_resolve_absolute(p, P(BASE, 'dirB', 'fileB'))
        # Non-strict
        p = P(BASE, 'dirA', 'linkX', 'linkY', 'foo', 'in', 'spam')
        self._check_resolve_relative(p, P(BASE, 'dirB', 'foo', 'in', 'spam'),
                                     False)
        p = P(BASE, 'dirA', 'linkX', 'linkY', '..', 'foo', 'in', 'spam')
        if os.name == 'nt':
            # In Windows, if linkY points to dirB, 'dirA\linkY\..'
            # resolves to 'dirA' without resolving linkY first.
            self._check_resolve_relative(p, P(d, 'foo', 'in', 'spam'), False)
        else:
            # In Posix, if linkY points to dirB, 'dirA/linkY/..'
            # resolves to 'dirB/..' first before resolving to parent of dirB.
            self._check_resolve_relative(p, P(BASE, 'foo', 'in', 'spam'), False)

    @os_helper.skip_unless_symlink
    def test_resolve_dot(self):
        # See https://bitbucket.org/pitrou/pathlib/issue/9/pathresolve-fails-on-complex-symlinks
        p = self.cls(BASE)
        self.dirlink('.', join('0'))
        self.dirlink(os.path.join('0', '0'), join('1'))
        self.dirlink(os.path.join('1', '1'), join('2'))
        q = p / '2'
        self.assertEqual(q.resolve(strict=True), p)
        r = q / '3' / '4'
        self.assertRaises(FileNotFoundError, r.resolve, strict=True)
        # Non-strict
        self.assertEqual(r.resolve(strict=False), p / '3' / '4')

    def test_resolve_nonexist_relative_issue38671(self):
        p = self.cls('non', 'exist')

        old_cwd = os.getcwd()
        os.chdir(BASE)
        try:
            self.assertEqual(p.resolve(), self.cls(BASE, p))
        finally:
            os.chdir(old_cwd)

    def test_with(self):
        p = self.cls(BASE)
        it = p.iterdir()
        it2 = p.iterdir()
        next(it2)
        with p:
            pass
        # Using a path as a context manager is a no-op, thus the following
        # operations should still succeed after the context manage exits.
        next(it)
        next(it2)
        p.exists()
        p.resolve()
        p.absolute()
        with p:
            pass

    def test_chmod(self):
        p = self.cls(BASE) / 'fileA'
        mode = p.stat().st_mode
        # Clear writable bit.
        new_mode = mode & ~0o222
        p.chmod(new_mode)
        self.assertEqual(p.stat().st_mode, new_mode)
        # Set writable bit.
        new_mode = mode | 0o222
        p.chmod(new_mode)
        self.assertEqual(p.stat().st_mode, new_mode)

    # On Windows, os.chmod does not follow symlinks (issue #15411)
    @only_posix
    def test_chmod_follow_symlinks_true(self):
        p = self.cls(BASE) / 'linkA'
        q = p.resolve()
        mode = q.stat().st_mode
        # Clear writable bit.
        new_mode = mode & ~0o222
        p.chmod(new_mode, follow_symlinks=True)
        self.assertEqual(q.stat().st_mode, new_mode)
        # Set writable bit
        new_mode = mode | 0o222
        p.chmod(new_mode, follow_symlinks=True)
        self.assertEqual(q.stat().st_mode, new_mode)

    # XXX also need a test for lchmod.

    def test_stat(self):
        p = self.cls(BASE) / 'fileA'
        st = p.stat()
        self.assertEqual(p.stat(), st)
        # Change file mode by flipping write bit.
        p.chmod(st.st_mode ^ 0o222)
        self.addCleanup(p.chmod, st.st_mode)
        self.assertNotEqual(p.stat(), st)

    @os_helper.skip_unless_symlink
    def test_stat_no_follow_symlinks(self):
        p = self.cls(BASE) / 'linkA'
        st = p.stat()
        self.assertNotEqual(st, p.stat(follow_symlinks=False))

    def test_stat_no_follow_symlinks_nosymlink(self):
        p = self.cls(BASE) / 'fileA'
        st = p.stat()
        self.assertEqual(st, p.stat(follow_symlinks=False))

    @os_helper.skip_unless_symlink
    def test_lstat(self):
        p = self.cls(BASE)/ 'linkA'
        st = p.stat()
        self.assertNotEqual(st, p.lstat())

    def test_lstat_nosymlink(self):
        p = self.cls(BASE) / 'fileA'
        st = p.stat()
        self.assertEqual(st, p.lstat())

    @unittest.skipUnless(pwd, "the pwd module is needed for this test")
    def test_owner(self):
        p = self.cls(BASE) / 'fileA'
        uid = p.stat().st_uid
        try:
            name = pwd.getpwuid(uid).pw_name
        except KeyError:
            self.skipTest(
                "user %d doesn't have an entry in the system database" % uid)
        self.assertEqual(name, p.owner())

    @unittest.skipUnless(grp, "the grp module is needed for this test")
    def test_group(self):
        p = self.cls(BASE) / 'fileA'
        gid = p.stat().st_gid
        try:
            name = grp.getgrgid(gid).gr_name
        except KeyError:
            self.skipTest(
                "group %d doesn't have an entry in the system database" % gid)
        self.assertEqual(name, p.group())

    def test_unlink(self):
        p = self.cls(BASE) / 'fileA'
        p.unlink()
        self.assertFileNotFound(p.stat)
        self.assertFileNotFound(p.unlink)

    def test_unlink_missing_ok(self):
        p = self.cls(BASE) / 'fileAAA'
        self.assertFileNotFound(p.unlink)
        p.unlink(missing_ok=True)

    def test_rmdir(self):
        p = self.cls(BASE) / 'dirA'
        for q in p.iterdir():
            q.unlink()
        p.rmdir()
        self.assertFileNotFound(p.stat)
        self.assertFileNotFound(p.unlink)

    @unittest.skipUnless(hasattr(os, "link"), "os.link() is not present")
    def test_link_to(self):
        P = self.cls(BASE)
        p = P / 'fileA'
        size = p.stat().st_size
        # linking to another path.
        q = P / 'dirA' / 'fileAA'
        try:
            with self.assertWarns(DeprecationWarning):
                p.link_to(q)
        except PermissionError as e:
            self.skipTest('os.link(): %s' % e)
        self.assertEqual(q.stat().st_size, size)
        self.assertEqual(os.path.samefile(p, q), True)
        self.assertTrue(p.stat)
        # Linking to a str of a relative path.
        r = rel_join('fileAAA')
        with self.assertWarns(DeprecationWarning):
            q.link_to(r)
        self.assertEqual(os.stat(r).st_size, size)
        self.assertTrue(q.stat)

    @unittest.skipUnless(hasattr(os, "link"), "os.link() is not present")
    def test_hardlink_to(self):
        P = self.cls(BASE)
        target = P / 'fileA'
        size = target.stat().st_size
        # linking to another path.
        link = P / 'dirA' / 'fileAA'
        link.hardlink_to(target)
        self.assertEqual(link.stat().st_size, size)
        self.assertTrue(os.path.samefile(target, link))
        self.assertTrue(target.exists())
        # Linking to a str of a relative path.
        link2 = P / 'dirA' / 'fileAAA'
        target2 = rel_join('fileA')
        link2.hardlink_to(target2)
        self.assertEqual(os.stat(target2).st_size, size)
        self.assertTrue(link2.exists())

    @unittest.skipIf(hasattr(os, "link"), "os.link() is present")
    def test_link_to_not_implemented(self):
        P = self.cls(BASE)
        p = P / 'fileA'
        # linking to another path.
        q = P / 'dirA' / 'fileAA'
        with self.assertRaises(NotImplementedError):
            p.link_to(q)

    def test_rename(self):
        P = self.cls(BASE)
        p = P / 'fileA'
        size = p.stat().st_size
        # Renaming to another path.
        q = P / 'dirA' / 'fileAA'
        renamed_p = p.rename(q)
        self.assertEqual(renamed_p, q)
        self.assertEqual(q.stat().st_size, size)
        self.assertFileNotFound(p.stat)
        # Renaming to a str of a relative path.
        r = rel_join('fileAAA')
        renamed_q = q.rename(r)
        self.assertEqual(renamed_q, self.cls(r))
        self.assertEqual(os.stat(r).st_size, size)
        self.assertFileNotFound(q.stat)

    def test_replace(self):
        P = self.cls(BASE)
        p = P / 'fileA'
        size = p.stat().st_size
        # Replacing a non-existing path.
        q = P / 'dirA' / 'fileAA'
        replaced_p = p.replace(q)
        self.assertEqual(replaced_p, q)
        self.assertEqual(q.stat().st_size, size)
        self.assertFileNotFound(p.stat)
        # Replacing another (existing) path.
        r = rel_join('dirB', 'fileB')
        replaced_q = q.replace(r)
        self.assertEqual(replaced_q, self.cls(r))
        self.assertEqual(os.stat(r).st_size, size)
        self.assertFileNotFound(q.stat)

    @os_helper.skip_unless_symlink
    def test_readlink(self):
        P = self.cls(BASE)
        self.assertEqual((P / 'linkA').readlink(), self.cls('fileA'))
        self.assertEqual((P / 'brokenLink').readlink(),
                         self.cls('non-existing'))
        self.assertEqual((P / 'linkB').readlink(), self.cls('dirB'))
        with self.assertRaises(OSError):
            (P / 'fileA').readlink()

    def test_touch_common(self):
        P = self.cls(BASE)
        p = P / 'newfileA'
        self.assertFalse(p.exists())
        p.touch()
        self.assertTrue(p.exists())
        st = p.stat()
        old_mtime = st.st_mtime
        old_mtime_ns = st.st_mtime_ns
        # Rewind the mtime sufficiently far in the past to work around
        # filesystem-specific timestamp granularity.
        os.utime(str(p), (old_mtime - 10, old_mtime - 10))
        # The file mtime should be refreshed by calling touch() again.
        p.touch()
        st = p.stat()
        self.assertGreaterEqual(st.st_mtime_ns, old_mtime_ns)
        self.assertGreaterEqual(st.st_mtime, old_mtime)
        # Now with exist_ok=False.
        p = P / 'newfileB'
        self.assertFalse(p.exists())
        p.touch(mode=0o700, exist_ok=False)
        self.assertTrue(p.exists())
        self.assertRaises(OSError, p.touch, exist_ok=False)

    def test_touch_nochange(self):
        P = self.cls(BASE)
        p = P / 'fileA'
        p.touch()
        with p.open('rb') as f:
            self.assertEqual(f.read().strip(), b"this is file A")

    def test_mkdir(self):
        P = self.cls(BASE)
        p = P / 'newdirA'
        self.assertFalse(p.exists())
        p.mkdir()
        self.assertTrue(p.exists())
        self.assertTrue(p.is_dir())
        with self.assertRaises(OSError) as cm:
            p.mkdir()
        self.assertEqual(cm.exception.errno, errno.EEXIST)

    def test_mkdir_parents(self):
        # Creating a chain of directories.
        p = self.cls(BASE, 'newdirB', 'newdirC')
        self.assertFalse(p.exists())
        with self.assertRaises(OSError) as cm:
            p.mkdir()
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        p.mkdir(parents=True)
        self.assertTrue(p.exists())
        self.assertTrue(p.is_dir())
        with self.assertRaises(OSError) as cm:
            p.mkdir(parents=True)
        self.assertEqual(cm.exception.errno, errno.EEXIST)
        # Test `mode` arg.
        mode = stat.S_IMODE(p.stat().st_mode)  # Default mode.
        p = self.cls(BASE, 'newdirD', 'newdirE')
        p.mkdir(0o555, parents=True)
        self.assertTrue(p.exists())
        self.assertTrue(p.is_dir())
        if os.name != 'nt':
            # The directory's permissions follow the mode argument.
            self.assertEqual(stat.S_IMODE(p.stat().st_mode), 0o7555 & mode)
        # The parent's permissions follow the default process settings.
        self.assertEqual(stat.S_IMODE(p.parent.stat().st_mode), mode)

    def test_mkdir_exist_ok(self):
        p = self.cls(BASE, 'dirB')
        st_ctime_first = p.stat().st_ctime
        self.assertTrue(p.exists())
        self.assertTrue(p.is_dir())
        with self.assertRaises(FileExistsError) as cm:
            p.mkdir()
        self.assertEqual(cm.exception.errno, errno.EEXIST)
        p.mkdir(exist_ok=True)
        self.assertTrue(p.exists())
        self.assertEqual(p.stat().st_ctime, st_ctime_first)

    def test_mkdir_exist_ok_with_parent(self):
        p = self.cls(BASE, 'dirC')
        self.assertTrue(p.exists())
        with self.assertRaises(FileExistsError) as cm:
            p.mkdir()
        self.assertEqual(cm.exception.errno, errno.EEXIST)
        p = p / 'newdirC'
        p.mkdir(parents=True)
        st_ctime_first = p.stat().st_ctime
        self.assertTrue(p.exists())
        with self.assertRaises(FileExistsError) as cm:
            p.mkdir(parents=True)
        self.assertEqual(cm.exception.errno, errno.EEXIST)
        p.mkdir(parents=True, exist_ok=True)
        self.assertTrue(p.exists())
        self.assertEqual(p.stat().st_ctime, st_ctime_first)

    def test_mkdir_exist_ok_root(self):
        # Issue #25803: A drive root could raise PermissionError on Windows.
        self.cls('/').resolve().mkdir(exist_ok=True)
        self.cls('/').resolve().mkdir(parents=True, exist_ok=True)

    @only_nt  # XXX: not sure how to test this on POSIX.
    def test_mkdir_with_unknown_drive(self):
        for d in 'ZYXWVUTSRQPONMLKJIHGFEDCBA':
            p = self.cls(d + ':\\')
            if not p.is_dir():
                break
        else:
            self.skipTest("cannot find a drive that doesn't exist")
        with self.assertRaises(OSError):
            (p / 'child' / 'path').mkdir(parents=True)

    def test_mkdir_with_child_file(self):
        p = self.cls(BASE, 'dirB', 'fileB')
        self.assertTrue(p.exists())
        # An exception is raised when the last path component is an existing
        # regular file, regardless of whether exist_ok is true or not.
        with self.assertRaises(FileExistsError) as cm:
            p.mkdir(parents=True)
        self.assertEqual(cm.exception.errno, errno.EEXIST)
        with self.assertRaises(FileExistsError) as cm:
            p.mkdir(parents=True, exist_ok=True)
        self.assertEqual(cm.exception.errno, errno.EEXIST)

    def test_mkdir_no_parents_file(self):
        p = self.cls(BASE, 'fileA')
        self.assertTrue(p.exists())
        # An exception is raised when the last path component is an existing
        # regular file, regardless of whether exist_ok is true or not.
        with self.assertRaises(FileExistsError) as cm:
            p.mkdir()
        self.assertEqual(cm.exception.errno, errno.EEXIST)
        with self.assertRaises(FileExistsError) as cm:
            p.mkdir(exist_ok=True)
        self.assertEqual(cm.exception.errno, errno.EEXIST)

    def test_mkdir_concurrent_parent_creation(self):
        for pattern_num in range(32):
            p = self.cls(BASE, 'dirCPC%d' % pattern_num)
            self.assertFalse(p.exists())

            def my_mkdir(path, mode=0o777):
                path = str(path)
                # Emulate another process that would create the directory
                # just before we try to create it ourselves.  We do it
                # in all possible pattern combinations, assuming that this
                # function is called at most 5 times (dirCPC/dir1/dir2,
                # dirCPC/dir1, dirCPC, dirCPC/dir1, dirCPC/dir1/dir2).
                if pattern.pop():
                    os.mkdir(path, mode)  # From another process.
                    concurrently_created.add(path)
                os.mkdir(path, mode)  # Our real call.

            pattern = [bool(pattern_num & (1 << n)) for n in range(5)]
            concurrently_created = set()
            p12 = p / 'dir1' / 'dir2'
            try:
                with mock.patch("pathlib._normal_accessor.mkdir", my_mkdir):
                    p12.mkdir(parents=True, exist_ok=False)
            except FileExistsError:
                self.assertIn(str(p12), concurrently_created)
            else:
                self.assertNotIn(str(p12), concurrently_created)
            self.assertTrue(p.exists())

    @os_helper.skip_unless_symlink
    def test_symlink_to(self):
        P = self.cls(BASE)
        target = P / 'fileA'
        # Symlinking a path target.
        link = P / 'dirA' / 'linkAA'
        link.symlink_to(target)
        self.assertEqual(link.stat(), target.stat())
        self.assertNotEqual(link.lstat(), target.stat())
        # Symlinking a str target.
        link = P / 'dirA' / 'linkAAA'
        link.symlink_to(str(target))
        self.assertEqual(link.stat(), target.stat())
        self.assertNotEqual(link.lstat(), target.stat())
        self.assertFalse(link.is_dir())
        # Symlinking to a directory.
        target = P / 'dirB'
        link = P / 'dirA' / 'linkAAAA'
        link.symlink_to(target, target_is_directory=True)
        self.assertEqual(link.stat(), target.stat())
        self.assertNotEqual(link.lstat(), target.stat())
        self.assertTrue(link.is_dir())
        self.assertTrue(list(link.iterdir()))

    def test_is_dir(self):
        P = self.cls(BASE)
        self.assertTrue((P / 'dirA').is_dir())
        self.assertFalse((P / 'fileA').is_dir())
        self.assertFalse((P / 'non-existing').is_dir())
        self.assertFalse((P / 'fileA' / 'bah').is_dir())
        if os_helper.can_symlink():
            self.assertFalse((P / 'linkA').is_dir())
            self.assertTrue((P / 'linkB').is_dir())
            self.assertFalse((P/ 'brokenLink').is_dir(), False)
        self.assertIs((P / 'dirA\udfff').is_dir(), False)
        self.assertIs((P / 'dirA\x00').is_dir(), False)

    def test_is_file(self):
        P = self.cls(BASE)
        self.assertTrue((P / 'fileA').is_file())
        self.assertFalse((P / 'dirA').is_file())
        self.assertFalse((P / 'non-existing').is_file())
        self.assertFalse((P / 'fileA' / 'bah').is_file())
        if os_helper.can_symlink():
            self.assertTrue((P / 'linkA').is_file())
            self.assertFalse((P / 'linkB').is_file())
            self.assertFalse((P/ 'brokenLink').is_file())
        self.assertIs((P / 'fileA\udfff').is_file(), False)
        self.assertIs((P / 'fileA\x00').is_file(), False)

    @only_posix
    def test_is_mount(self):
        P = self.cls(BASE)
        R = self.cls('/')  # TODO: Work out Windows.
        self.assertFalse((P / 'fileA').is_mount())
        self.assertFalse((P / 'dirA').is_mount())
        self.assertFalse((P / 'non-existing').is_mount())
        self.assertFalse((P / 'fileA' / 'bah').is_mount())
        self.assertTrue(R.is_mount())
        if os_helper.can_symlink():
            self.assertFalse((P / 'linkA').is_mount())
        self.assertIs(self.cls('/\udfff').is_mount(), False)
        self.assertIs(self.cls('/\x00').is_mount(), False)

    def test_is_symlink(self):
        P = self.cls(BASE)
        self.assertFalse((P / 'fileA').is_symlink())
        self.assertFalse((P / 'dirA').is_symlink())
        self.assertFalse((P / 'non-existing').is_symlink())
        self.assertFalse((P / 'fileA' / 'bah').is_symlink())
        if os_helper.can_symlink():
            self.assertTrue((P / 'linkA').is_symlink())
            self.assertTrue((P / 'linkB').is_symlink())
            self.assertTrue((P/ 'brokenLink').is_symlink())
        self.assertIs((P / 'fileA\udfff').is_file(), False)
        self.assertIs((P / 'fileA\x00').is_file(), False)
        if os_helper.can_symlink():
            self.assertIs((P / 'linkA\udfff').is_file(), False)
            self.assertIs((P / 'linkA\x00').is_file(), False)

    def test_is_fifo_false(self):
        P = self.cls(BASE)
        self.assertFalse((P / 'fileA').is_fifo())
        self.assertFalse((P / 'dirA').is_fifo())
        self.assertFalse((P / 'non-existing').is_fifo())
        self.assertFalse((P / 'fileA' / 'bah').is_fifo())
        self.assertIs((P / 'fileA\udfff').is_fifo(), False)
        self.assertIs((P / 'fileA\x00').is_fifo(), False)

    @unittest.skipUnless(hasattr(os, "mkfifo"), "os.mkfifo() required")
    @unittest.skipIf(sys.platform == "vxworks",
                    "fifo requires special path on VxWorks")
    def test_is_fifo_true(self):
        P = self.cls(BASE, 'myfifo')
        try:
            os.mkfifo(str(P))
        except PermissionError as e:
            self.skipTest('os.mkfifo(): %s' % e)
        self.assertTrue(P.is_fifo())
        self.assertFalse(P.is_socket())
        self.assertFalse(P.is_file())
        self.assertIs(self.cls(BASE, 'myfifo\udfff').is_fifo(), False)
        self.assertIs(self.cls(BASE, 'myfifo\x00').is_fifo(), False)

    def test_is_socket_false(self):
        P = self.cls(BASE)
        self.assertFalse((P / 'fileA').is_socket())
        self.assertFalse((P / 'dirA').is_socket())
        self.assertFalse((P / 'non-existing').is_socket())
        self.assertFalse((P / 'fileA' / 'bah').is_socket())
        self.assertIs((P / 'fileA\udfff').is_socket(), False)
        self.assertIs((P / 'fileA\x00').is_socket(), False)

    @unittest.skipUnless(hasattr(socket, "AF_UNIX"), "Unix sockets required")
    def test_is_socket_true(self):
        P = self.cls(BASE, 'mysock')
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.addCleanup(sock.close)
        try:
            sock.bind(str(P))
        except OSError as e:
            if (isinstance(e, PermissionError) or
                    "AF_UNIX path too long" in str(e)):
                self.skipTest("cannot bind Unix socket: " + str(e))
        self.assertTrue(P.is_socket())
        self.assertFalse(P.is_fifo())
        self.assertFalse(P.is_file())
        self.assertIs(self.cls(BASE, 'mysock\udfff').is_socket(), False)
        self.assertIs(self.cls(BASE, 'mysock\x00').is_socket(), False)

    def test_is_block_device_false(self):
        P = self.cls(BASE)
        self.assertFalse((P / 'fileA').is_block_device())
        self.assertFalse((P / 'dirA').is_block_device())
        self.assertFalse((P / 'non-existing').is_block_device())
        self.assertFalse((P / 'fileA' / 'bah').is_block_device())
        self.assertIs((P / 'fileA\udfff').is_block_device(), False)
        self.assertIs((P / 'fileA\x00').is_block_device(), False)

    def test_is_char_device_false(self):
        P = self.cls(BASE)
        self.assertFalse((P / 'fileA').is_char_device())
        self.assertFalse((P / 'dirA').is_char_device())
        self.assertFalse((P / 'non-existing').is_char_device())
        self.assertFalse((P / 'fileA' / 'bah').is_char_device())
        self.assertIs((P / 'fileA\udfff').is_char_device(), False)
        self.assertIs((P / 'fileA\x00').is_char_device(), False)

    def test_is_char_device_true(self):
        # Under Unix, /dev/null should generally be a char device.
        P = self.cls('/dev/null')
        if not P.exists():
            self.skipTest("/dev/null required")
        self.assertTrue(P.is_char_device())
        self.assertFalse(P.is_block_device())
        self.assertFalse(P.is_file())
        self.assertIs(self.cls('/dev/null\udfff').is_char_device(), False)
        self.assertIs(self.cls('/dev/null\x00').is_char_device(), False)

    def test_pickling_common(self):
        p = self.cls(BASE, 'fileA')
        for proto in range(0, pickle.HIGHEST_PROTOCOL + 1):
            dumped = pickle.dumps(p, proto)
            pp = pickle.loads(dumped)
            self.assertEqual(pp.stat(), p.stat())

    def test_parts_interning(self):
        P = self.cls
        p = P('/usr/bin/foo')
        q = P('/usr/local/bin')
        # 'usr'
        self.assertIs(p.parts[1], q.parts[1])
        # 'bin'
        self.assertIs(p.parts[2], q.parts[3])

    def _check_complex_symlinks(self, link0_target):
        # Test solving a non-looping chain of symlinks (issue #19887).
        P = self.cls(BASE)
        self.dirlink(os.path.join('link0', 'link0'), join('link1'))
        self.dirlink(os.path.join('link1', 'link1'), join('link2'))
        self.dirlink(os.path.join('link2', 'link2'), join('link3'))
        self.dirlink(link0_target, join('link0'))

        # Resolve absolute paths.
        p = (P / 'link0').resolve()
        self.assertEqual(p, P)
        self.assertEqualNormCase(str(p), BASE)
        p = (P / 'link1').resolve()
        self.assertEqual(p, P)
        self.assertEqualNormCase(str(p), BASE)
        p = (P / 'link2').resolve()
        self.assertEqual(p, P)
        self.assertEqualNormCase(str(p), BASE)
        p = (P / 'link3').resolve()
        self.assertEqual(p, P)
        self.assertEqualNormCase(str(p), BASE)

        # Resolve relative paths.
        old_path = os.getcwd()
        os.chdir(BASE)
        try:
            p = self.cls('link0').resolve()
            self.assertEqual(p, P)
            self.assertEqualNormCase(str(p), BASE)
            p = self.cls('link1').resolve()
            self.assertEqual(p, P)
            self.assertEqualNormCase(str(p), BASE)
            p = self.cls('link2').resolve()
            self.assertEqual(p, P)
            self.assertEqualNormCase(str(p), BASE)
            p = self.cls('link3').resolve()
            self.assertEqual(p, P)
            self.assertEqualNormCase(str(p), BASE)
        finally:
            os.chdir(old_path)

    @os_helper.skip_unless_symlink
    def test_complex_symlinks_absolute(self):
        self._check_complex_symlinks(BASE)

    @os_helper.skip_unless_symlink
    def test_complex_symlinks_relative(self):
        self._check_complex_symlinks('.')

    @os_helper.skip_unless_symlink
    def test_complex_symlinks_relative_dot_dot(self):
        self._check_complex_symlinks(os.path.join('dirA', '..'))


class PathTest(_BasePathTest, unittest.TestCase):
    cls = pathlib.Path

    def test_class_getitem(self):
        self.assertIs(self.cls[str], self.cls)

    def test_concrete_class(self):
        p = self.cls('a')
        self.assertIs(type(p),
            pathlib.WindowsPath if os.name == 'nt' else pathlib.PosixPath)

    def test_unsupported_flavour(self):
        if os.name == 'nt':
            self.assertRaises(NotImplementedError, pathlib.PosixPath)
        else:
            self.assertRaises(NotImplementedError, pathlib.WindowsPath)

    def test_glob_empty_pattern(self):
        p = self.cls()
        with self.assertRaisesRegex(ValueError, 'Unacceptable pattern'):
            list(p.glob(''))


@only_posix
class PosixPathTest(_BasePathTest, unittest.TestCase):
    cls = pathlib.PosixPath

    def _check_symlink_loop(self, *args, strict=True):
        path = self.cls(*args)
        with self.assertRaises(RuntimeError):
            print(path.resolve(strict))

    def test_open_mode(self):
        old_mask = os.umask(0)
        self.addCleanup(os.umask, old_mask)
        p = self.cls(BASE)
        with (p / 'new_file').open('wb'):
            pass
        st = os.stat(join('new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o666)
        os.umask(0o022)
        with (p / 'other_new_file').open('wb'):
            pass
        st = os.stat(join('other_new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o644)

    def test_resolve_root(self):
        current_directory = os.getcwd()
        try:
            os.chdir('/')
            p = self.cls('spam')
            self.assertEqual(str(p.resolve()), '/spam')
        finally:
            os.chdir(current_directory)

    def test_touch_mode(self):
        old_mask = os.umask(0)
        self.addCleanup(os.umask, old_mask)
        p = self.cls(BASE)
        (p / 'new_file').touch()
        st = os.stat(join('new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o666)
        os.umask(0o022)
        (p / 'other_new_file').touch()
        st = os.stat(join('other_new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o644)
        (p / 'masked_new_file').touch(mode=0o750)
        st = os.stat(join('masked_new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o750)

    @os_helper.skip_unless_symlink
    def test_resolve_loop(self):
        # Loops with relative symlinks.
        os.symlink('linkX/inside', join('linkX'))
        self._check_symlink_loop(BASE, 'linkX')
        os.symlink('linkY', join('linkY'))
        self._check_symlink_loop(BASE, 'linkY')
        os.symlink('linkZ/../linkZ', join('linkZ'))
        self._check_symlink_loop(BASE, 'linkZ')
        # Non-strict
        self._check_symlink_loop(BASE, 'linkZ', 'foo', strict=False)
        # Loops with absolute symlinks.
        os.symlink(join('linkU/inside'), join('linkU'))
        self._check_symlink_loop(BASE, 'linkU')
        os.symlink(join('linkV'), join('linkV'))
        self._check_symlink_loop(BASE, 'linkV')
        os.symlink(join('linkW/../linkW'), join('linkW'))
        self._check_symlink_loop(BASE, 'linkW')
        # Non-strict
        self._check_symlink_loop(BASE, 'linkW', 'foo', strict=False)

    def test_glob(self):
        P = self.cls
        p = P(BASE)
        given = set(p.glob("FILEa"))
        expect = set() if not os_helper.fs_is_case_insensitive(BASE) else given
        self.assertEqual(given, expect)
        self.assertEqual(set(p.glob("FILEa*")), set())

    def test_rglob(self):
        P = self.cls
        p = P(BASE, "dirC")
        given = set(p.rglob("FILEd"))
        expect = set() if not os_helper.fs_is_case_insensitive(BASE) else given
        self.assertEqual(given, expect)
        self.assertEqual(set(p.rglob("FILEd*")), set())

    @unittest.skipUnless(hasattr(pwd, 'getpwall'),
                         'pwd module does not expose getpwall()')
    @unittest.skipIf(sys.platform == "vxworks",
                     "no home directory on VxWorks")
    def test_expanduser(self):
        P = self.cls
        import_helper.import_module('pwd')
        import pwd
        pwdent = pwd.getpwuid(os.getuid())
        username = pwdent.pw_name
        userhome = pwdent.pw_dir.rstrip('/') or '/'
        # Find arbitrary different user (if exists).
        for pwdent in pwd.getpwall():
            othername = pwdent.pw_name
            otherhome = pwdent.pw_dir.rstrip('/')
            if othername != username and otherhome:
                break
        else:
            othername = username
            otherhome = userhome

        p1 = P('~/Documents')
        p2 = P('~' + username + '/Documents')
        p3 = P('~' + othername + '/Documents')
        p4 = P('../~' + username + '/Documents')
        p5 = P('/~' + username + '/Documents')
        p6 = P('')
        p7 = P('~fakeuser/Documents')

        with os_helper.EnvironmentVarGuard() as env:
            env.pop('HOME', None)

            self.assertEqual(p1.expanduser(), P(userhome) / 'Documents')
            self.assertEqual(p2.expanduser(), P(userhome) / 'Documents')
            self.assertEqual(p3.expanduser(), P(otherhome) / 'Documents')
            self.assertEqual(p4.expanduser(), p4)
            self.assertEqual(p5.expanduser(), p5)
            self.assertEqual(p6.expanduser(), p6)
            self.assertRaises(RuntimeError, p7.expanduser)

            env['HOME'] = '/tmp'
            self.assertEqual(p1.expanduser(), P('/tmp/Documents'))
            self.assertEqual(p2.expanduser(), P(userhome) / 'Documents')
            self.assertEqual(p3.expanduser(), P(otherhome) / 'Documents')
            self.assertEqual(p4.expanduser(), p4)
            self.assertEqual(p5.expanduser(), p5)
            self.assertEqual(p6.expanduser(), p6)
            self.assertRaises(RuntimeError, p7.expanduser)

    @unittest.skipIf(sys.platform != "darwin",
                     "Bad file descriptor in /dev/fd affects only macOS")
    def test_handling_bad_descriptor(self):
        try:
            file_descriptors = list(pathlib.Path('/dev/fd').rglob("*"))[3:]
            if not file_descriptors:
                self.skipTest("no file descriptors - issue was not reproduced")
            # Checking all file descriptors because there is no guarantee
            # which one will fail.
            for f in file_descriptors:
                f.exists()
                f.is_dir()
                f.is_file()
                f.is_symlink()
                f.is_block_device()
                f.is_char_device()
                f.is_fifo()
                f.is_socket()
        except OSError as e:
            if e.errno == errno.EBADF:
                self.fail("Bad file descriptor not handled.")
            raise


@only_nt
class WindowsPathTest(_BasePathTest, unittest.TestCase):
    cls = pathlib.WindowsPath

    def test_glob(self):
        P = self.cls
        p = P(BASE)
        self.assertEqual(set(p.glob("FILEa")), { P(BASE, "fileA") })
        self.assertEqual(set(p.glob("F*a")), { P(BASE, "fileA") })
        self.assertEqual(set(map(str, p.glob("FILEa"))), {f"{p}\\FILEa"})
        self.assertEqual(set(map(str, p.glob("F*a"))), {f"{p}\\fileA"})

    def test_rglob(self):
        P = self.cls
        p = P(BASE, "dirC")
        self.assertEqual(set(p.rglob("FILEd")), { P(BASE, "dirC/dirD/fileD") })
        self.assertEqual(set(map(str, p.rglob("FILEd"))), {f"{p}\\dirD\\FILEd"})

    def test_expanduser(self):
        P = self.cls
        with os_helper.EnvironmentVarGuard() as env:
            env.pop('HOME', None)
            env.pop('USERPROFILE', None)
            env.pop('HOMEPATH', None)
            env.pop('HOMEDRIVE', None)
            env['USERNAME'] = 'alice'

            # test that the path returns unchanged
            p1 = P('~/My Documents')
            p2 = P('~alice/My Documents')
            p3 = P('~bob/My Documents')
            p4 = P('/~/My Documents')
            p5 = P('d:~/My Documents')
            p6 = P('')
            self.assertRaises(RuntimeError, p1.expanduser)
            self.assertRaises(RuntimeError, p2.expanduser)
            self.assertRaises(RuntimeError, p3.expanduser)
            self.assertEqual(p4.expanduser(), p4)
            self.assertEqual(p5.expanduser(), p5)
            self.assertEqual(p6.expanduser(), p6)

            def check():
                env.pop('USERNAME', None)
                self.assertEqual(p1.expanduser(),
                                 P('C:/Users/alice/My Documents'))
                self.assertRaises(RuntimeError, p2.expanduser)
                env['USERNAME'] = 'alice'
                self.assertEqual(p2.expanduser(),
                                 P('C:/Users/alice/My Documents'))
                self.assertEqual(p3.expanduser(),
                                 P('C:/Users/bob/My Documents'))
                self.assertEqual(p4.expanduser(), p4)
                self.assertEqual(p5.expanduser(), p5)
                self.assertEqual(p6.expanduser(), p6)

            env['HOMEPATH'] = 'C:\\Users\\alice'
            check()

            env['HOMEDRIVE'] = 'C:\\'
            env['HOMEPATH'] = 'Users\\alice'
            check()

            env.pop('HOMEDRIVE', None)
            env.pop('HOMEPATH', None)
            env['USERPROFILE'] = 'C:\\Users\\alice'
            check()

            # bpo-38883: ignore `HOME` when set on windows
            env['HOME'] = 'C:\\Users\\eve'
            check()


class CompatiblePathTest(unittest.TestCase):
    """
    Test that a type can be made compatible with PurePath
    derivatives by implementing division operator overloads.
    """

    class CompatPath:
        """
        Minimum viable class to test PurePath compatibility.
        Simply uses the division operator to join a given
        string and the string value of another object with
        a forward slash.
        """
        def __init__(self, string):
            self.string = string

        def __truediv__(self, other):
            return type(self)(f"{self.string}/{other}")

        def __rtruediv__(self, other):
            return type(self)(f"{other}/{self.string}")

    def test_truediv(self):
        result = pathlib.PurePath("test") / self.CompatPath("right")
        self.assertIsInstance(result, self.CompatPath)
        self.assertEqual(result.string, "test/right")

        with self.assertRaises(TypeError):
            # Verify improper operations still raise a TypeError
            pathlib.PurePath("test") / 10

    def test_rtruediv(self):
        result = self.CompatPath("left") / pathlib.PurePath("test")
        self.assertIsInstance(result, self.CompatPath)
        self.assertEqual(result.string, "left/test")

        with self.assertRaises(TypeError):
            # Verify improper operations still raise a TypeError
            10 / pathlib.PurePath("test")


if __name__ == "__main__":
    unittest.main()
