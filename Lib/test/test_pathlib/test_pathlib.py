import io
import os
import sys
import errno
import ntpath
import pathlib
import pickle
import posixpath
import socket
import stat
import tempfile
import unittest
from unittest import mock
from urllib.request import pathname2url

from test.support import import_helper
from test.support import is_emscripten, is_wasi
from test.support import infinite_recursion
from test.support import os_helper
from test.support.os_helper import TESTFN, FakePath
from test.test_pathlib import test_pathlib_abc
from test.test_pathlib.test_pathlib_abc import needs_posix, needs_windows, needs_symlinks

try:
    import grp, pwd
except ImportError:
    grp = pwd = None


root_in_posix = False
if hasattr(os, 'geteuid'):
    root_in_posix = (os.geteuid() == 0)

#
# Tests for the pure classes.
#

class PurePathTest(test_pathlib_abc.DummyPurePathTest):
    cls = pathlib.PurePath

    # Make sure any symbolic links in the base test path are resolved.
    base = os.path.realpath(TESTFN)

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
            ('a', '/b/c', 'd'), ('/a', '/b/c', 'd'),
            # Empty components get removed.
            ('/', 'b', '', 'c/d'), ('/', '', 'b/c/d'), ('', '/b/c/d'),
            ],
    }

    def test_concrete_class(self):
        if self.cls is pathlib.PurePath:
            expected = pathlib.PureWindowsPath if os.name == 'nt' else pathlib.PurePosixPath
        else:
            expected = self.cls
        p = self.cls('a')
        self.assertIs(type(p), expected)

    def test_concrete_parser(self):
        if self.cls is pathlib.PurePosixPath:
            expected = posixpath
        elif self.cls is pathlib.PureWindowsPath:
            expected = ntpath
        else:
            expected = os.path
        p = self.cls('a')
        self.assertIs(p.parser, expected)

    def test_different_parsers_unequal(self):
        p = self.cls('a')
        if p.parser is posixpath:
            q = pathlib.PureWindowsPath('a')
        else:
            q = pathlib.PurePosixPath('a')
        self.assertNotEqual(p, q)

    def test_different_parsers_unordered(self):
        p = self.cls('a')
        if p.parser is posixpath:
            q = pathlib.PureWindowsPath('a')
        else:
            q = pathlib.PurePosixPath('a')
        with self.assertRaises(TypeError):
            p < q
        with self.assertRaises(TypeError):
            p <= q
        with self.assertRaises(TypeError):
            p > q
        with self.assertRaises(TypeError):
            p >= q

    def test_constructor_nested(self):
        P = self.cls
        P(FakePath("a/b/c"))
        self.assertEqual(P(P('a')), P('a'))
        self.assertEqual(P(P('a'), 'b'), P('a/b'))
        self.assertEqual(P(P('a'), P('b')), P('a/b'))
        self.assertEqual(P(P('a'), P('b'), P('c')), P(FakePath("a/b/c")))
        self.assertEqual(P(P('./a:b')), P('./a:b'))

    @needs_windows
    def test_constructor_nested_foreign_flavour(self):
        # See GH-125069.
        p1 = pathlib.PurePosixPath('b/c:\\d')
        p2 = pathlib.PurePosixPath('b/', 'c:\\d')
        self.assertEqual(p1, p2)
        self.assertEqual(self.cls(p1), self.cls('b/c:/d'))
        self.assertEqual(self.cls(p2), self.cls('b/c:/d'))

    def _check_parse_path(self, raw_path, *expected):
        sep = self.parser.sep
        actual = self.cls._parse_path(raw_path.replace('/', sep))
        self.assertEqual(actual, expected)
        if altsep := self.parser.altsep:
            actual = self.cls._parse_path(raw_path.replace('/', altsep))
            self.assertEqual(actual, expected)

    def test_parse_path_common(self):
        check = self._check_parse_path
        sep = self.parser.sep
        check('',         '', '', [])
        check('a',        '', '', ['a'])
        check('a/',       '', '', ['a'])
        check('a/b',      '', '', ['a', 'b'])
        check('a/b/',     '', '', ['a', 'b'])
        check('a/b/c/d',  '', '', ['a', 'b', 'c', 'd'])
        check('a/b//c/d', '', '', ['a', 'b', 'c', 'd'])
        check('a/b/c/d',  '', '', ['a', 'b', 'c', 'd'])
        check('.',        '', '', [])
        check('././b',    '', '', ['b'])
        check('a/./b',    '', '', ['a', 'b'])
        check('a/./.',    '', '', ['a'])
        check('/a/b',     '', sep, ['a', 'b'])

    def test_empty_path(self):
        # The empty path points to '.'
        p = self.cls('')
        self.assertEqual(str(p), '.')
        # Special case for the empty path.
        self._check_str('.', ('',))

    def test_parts_interning(self):
        P = self.cls
        p = P('/usr/bin/foo')
        q = P('/usr/local/bin')
        # 'usr'
        self.assertIs(p.parts[1], q.parts[1])
        # 'bin'
        self.assertIs(p.parts[2], q.parts[3])

    def test_join_nested(self):
        P = self.cls
        p = P('a/b').joinpath(P('c'))
        self.assertEqual(p, P('a/b/c'))

    def test_div_nested(self):
        P = self.cls
        p = P('a/b') / P('c')
        self.assertEqual(p, P('a/b/c'))

    def test_pickling_common(self):
        P = self.cls
        for pathstr in ('a', 'a/', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c', 'a/b/c/'):
            with self.subTest(pathstr=pathstr):
                p = P(pathstr)
                for proto in range(0, pickle.HIGHEST_PROTOCOL + 1):
                    dumped = pickle.dumps(p, proto)
                    pp = pickle.loads(dumped)
                    self.assertIs(pp.__class__, p.__class__)
                    self.assertEqual(pp, p)
                    self.assertEqual(hash(pp), hash(p))
                    self.assertEqual(str(pp), str(p))

    def test_repr_common(self):
        for pathstr in ('a', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c'):
            with self.subTest(pathstr=pathstr):
                p = self.cls(pathstr)
                clsname = p.__class__.__name__
                r = repr(p)
                # The repr() is in the form ClassName("forward-slashes path").
                self.assertTrue(r.startswith(clsname + '('), r)
                self.assertTrue(r.endswith(')'), r)
                inner = r[len(clsname) + 1 : -1]
                self.assertEqual(eval(inner), p.as_posix())

    def test_fspath_common(self):
        P = self.cls
        p = P('a/b')
        self._check_str(p.__fspath__(), ('a/b',))
        self._check_str(os.fspath(p), ('a/b',))

    def test_bytes_exc_message(self):
        P = self.cls
        message = (r"argument should be a str or an os\.PathLike object "
                   r"where __fspath__ returns a str, not 'bytes'")
        with self.assertRaisesRegex(TypeError, message):
            P(b'a')
        with self.assertRaisesRegex(TypeError, message):
            P(b'a', 'b')
        with self.assertRaisesRegex(TypeError, message):
            P('a', b'b')

    def test_as_bytes_common(self):
        sep = os.fsencode(self.sep)
        P = self.cls
        self.assertEqual(bytes(P('a/b')), b'a' + sep + b'b')

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

    def test_equivalences(self, equivalences=None):
        if equivalences is None:
            equivalences = self.equivalences
        for k, tuples in equivalences.items():
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

    def test_as_uri_common(self):
        P = self.cls
        with self.assertRaises(ValueError):
            P('a').as_uri()
        with self.assertRaises(ValueError):
            P().as_uri()

    def test_repr_roundtrips(self):
        for pathstr in ('a', 'a/b', 'a/b/c', '/', '/a/b', '/a/b/c'):
            with self.subTest(pathstr=pathstr):
                p = self.cls(pathstr)
                r = repr(p)
                # The repr() roundtrips.
                q = eval(r, pathlib.__dict__)
                self.assertIs(q.__class__, p.__class__)
                self.assertEqual(q, p)
                self.assertEqual(repr(q), r)

    def test_name_empty(self):
        P = self.cls
        self.assertEqual(P('').name, '')
        self.assertEqual(P('.').name, '')
        self.assertEqual(P('/a/b/.').name, 'b')

    def test_stem_empty(self):
        P = self.cls
        self.assertEqual(P('').stem, '')
        self.assertEqual(P('.').stem, '')

    def test_with_name_empty(self):
        P = self.cls
        self.assertRaises(ValueError, P('').with_name, 'd.xml')
        self.assertRaises(ValueError, P('.').with_name, 'd.xml')
        self.assertRaises(ValueError, P('/').with_name, 'd.xml')
        self.assertRaises(ValueError, P('a/b').with_name, '')
        self.assertRaises(ValueError, P('a/b').with_name, '.')

    def test_with_stem_empty(self):
        P = self.cls
        self.assertRaises(ValueError, P('').with_stem, 'd')
        self.assertRaises(ValueError, P('.').with_stem, 'd')
        self.assertRaises(ValueError, P('/').with_stem, 'd')
        self.assertRaises(ValueError, P('a/b').with_stem, '')
        self.assertRaises(ValueError, P('a/b').with_stem, '.')

    def test_relative_to_several_args(self):
        P = self.cls
        p = P('a/b')
        with self.assertWarns(DeprecationWarning):
            p.relative_to('a', 'b')
            p.relative_to('a', 'b', walk_up=True)

    def test_is_relative_to_several_args(self):
        P = self.cls
        p = P('a/b')
        with self.assertWarns(DeprecationWarning):
            p.is_relative_to('a', 'b')

    def test_is_reserved_deprecated(self):
        P = self.cls
        p = P('a/b')
        with self.assertWarns(DeprecationWarning):
            p.is_reserved()

    def test_match_empty(self):
        P = self.cls
        self.assertRaises(ValueError, P('a').match, '')
        self.assertRaises(ValueError, P('a').match, '.')

    @needs_posix
    def test_parse_path_posix(self):
        check = self._check_parse_path
        # Collapsing of excess leading slashes, except for the double-slash
        # special case.
        check('//a/b',   '', '//', ['a', 'b'])
        check('///a/b',  '', '/', ['a', 'b'])
        check('////a/b', '', '/', ['a', 'b'])
        # Paths which look like NT paths aren't treated specially.
        check('c:a',     '', '', ['c:a',])
        check('c:\\a',   '', '', ['c:\\a',])
        check('\\a',     '', '', ['\\a',])

    @needs_posix
    def test_eq_posix(self):
        P = self.cls
        self.assertNotEqual(P('a/b'), P('A/b'))
        self.assertEqual(P('/a'), P('///a'))
        self.assertNotEqual(P('/a'), P('//a'))

    @needs_posix
    def test_as_uri_posix(self):
        P = self.cls
        self.assertEqual(P('/').as_uri(), 'file:///')
        self.assertEqual(P('/a/b.c').as_uri(), 'file:///a/b.c')
        self.assertEqual(P('/a/b%#c').as_uri(), 'file:///a/b%25%23c')

    @needs_posix
    def test_as_uri_non_ascii(self):
        from urllib.parse import quote_from_bytes
        P = self.cls
        try:
            os.fsencode('\xe9')
        except UnicodeEncodeError:
            self.skipTest("\\xe9 cannot be encoded to the filesystem encoding")
        self.assertEqual(P('/a/b\xe9').as_uri(),
                         'file:///a/b' + quote_from_bytes(os.fsencode('\xe9')))

    @needs_posix
    def test_parse_windows_path(self):
        P = self.cls
        p = P('c:', 'a', 'b')
        pp = P(pathlib.PureWindowsPath('c:\\a\\b'))
        self.assertEqual(p, pp)

    windows_equivalences = {
        './a:b': [ ('./a:b',) ],
        'c:a': [ ('c:', 'a'), ('c:', 'a/'), ('.', 'c:', 'a') ],
        'c:/a': [
            ('c:/', 'a'), ('c:', '/', 'a'), ('c:', '/a'),
            ('/z', 'c:/', 'a'), ('//x/y', 'c:/', 'a'),
            ],
        '//a/b/': [ ('//a/b',) ],
        '//a/b/c': [
            ('//a/b', 'c'), ('//a/b/', 'c'),
            ],
    }

    @needs_windows
    def test_equivalences_windows(self):
        self.test_equivalences(self.windows_equivalences)

    @needs_windows
    def test_parse_path_windows(self):
        check = self._check_parse_path
        # First part is anchored.
        check('c:',                  'c:', '', [])
        check('c:/',                 'c:', '\\', [])
        check('/',                   '', '\\', [])
        check('c:a',                 'c:', '', ['a'])
        check('c:/a',                'c:', '\\', ['a'])
        check('/a',                  '', '\\', ['a'])
        # UNC paths.
        check('//',                  '\\\\', '', [])
        check('//a',                 '\\\\a', '', [])
        check('//a/',                '\\\\a\\', '', [])
        check('//a/b',               '\\\\a\\b', '\\', [])
        check('//a/b/',              '\\\\a\\b', '\\', [])
        check('//a/b/c',             '\\\\a\\b', '\\', ['c'])
        # Collapsing and stripping excess slashes.
        check('Z://b//c/d/',         'Z:', '\\', ['b', 'c', 'd'])
        # UNC paths.
        check('//b/c//d',            '\\\\b\\c', '\\', ['d'])
        # Extended paths.
        check('//./c:',              '\\\\.\\c:', '', [])
        check('//?/c:/',             '\\\\?\\c:', '\\', [])
        check('//?/c:/a',            '\\\\?\\c:', '\\', ['a'])
        # Extended UNC paths (format is "\\?\UNC\server\share").
        check('//?',                 '\\\\?', '', [])
        check('//?/',                '\\\\?\\', '', [])
        check('//?/UNC',             '\\\\?\\UNC', '', [])
        check('//?/UNC/',            '\\\\?\\UNC\\', '', [])
        check('//?/UNC/b',           '\\\\?\\UNC\\b', '', [])
        check('//?/UNC/b/',          '\\\\?\\UNC\\b\\', '', [])
        check('//?/UNC/b/c',         '\\\\?\\UNC\\b\\c', '\\', [])
        check('//?/UNC/b/c/',        '\\\\?\\UNC\\b\\c', '\\', [])
        check('//?/UNC/b/c/d',       '\\\\?\\UNC\\b\\c', '\\', ['d'])
        # UNC device paths
        check('//./BootPartition/',  '\\\\.\\BootPartition', '\\', [])
        check('//?/BootPartition/',  '\\\\?\\BootPartition', '\\', [])
        check('//./PhysicalDrive0',  '\\\\.\\PhysicalDrive0', '', [])
        check('//?/Volume{}/',       '\\\\?\\Volume{}', '\\', [])
        check('//./nul',             '\\\\.\\nul', '', [])
        # Paths to files with NTFS alternate data streams
        check('./c:s',               '', '', ['c:s'])
        check('cc:s',                '', '', ['cc:s'])
        check('C:c:s',               'C:', '', ['c:s'])
        check('C:/c:s',              'C:', '\\', ['c:s'])
        check('D:a/c:b',             'D:', '', ['a', 'c:b'])
        check('D:/a/c:b',            'D:', '\\', ['a', 'c:b'])

    @needs_windows
    def test_eq_windows(self):
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
        self.assertEqual(P('\u0130'), P('i\u0307'))

    @needs_windows
    def test_as_uri_windows(self):
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

    @needs_windows
    def test_ordering_windows(self):
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


class PurePosixPathTest(PurePathTest):
    cls = pathlib.PurePosixPath


class PureWindowsPathTest(PurePathTest):
    cls = pathlib.PureWindowsPath


class PurePathSubclassTest(PurePathTest):
    class cls(pathlib.PurePath):
        pass

    # repr() roundtripping is not supported in custom subclass.
    test_repr_roundtrips = None


#
# Tests for the concrete classes.
#

class PathTest(test_pathlib_abc.DummyPathTest, PurePathTest):
    """Tests for the FS-accessing functionalities of the Path classes."""
    cls = pathlib.Path
    can_symlink = os_helper.can_symlink()

    def setUp(self):
        super().setUp()
        os.chmod(self.parser.join(self.base, 'dirE'), 0)

    def tearDown(self):
        os.chmod(self.parser.join(self.base, 'dirE'), 0o777)
        os_helper.rmtree(self.base)

    def tempdir(self):
        d = os_helper._longpath(tempfile.mkdtemp(suffix='-dirD',
                                                 dir=os.getcwd()))
        self.addCleanup(os_helper.rmtree, d)
        return d

    def test_matches_pathbase_api(self):
        our_names = {name for name in dir(self.cls) if name[0] != '_'}
        our_names.remove('is_reserved')  # only present in PurePath
        path_names = {name for name in dir(pathlib._abc.PathBase) if name[0] != '_'}
        self.assertEqual(our_names, path_names)
        for attr_name in our_names:
            if attr_name == 'parser':
                # On Windows, Path.parser is ntpath, but PathBase.parser is
                # posixpath, and so their docstrings differ.
                continue
            our_attr = getattr(self.cls, attr_name)
            path_attr = getattr(pathlib._abc.PathBase, attr_name)
            self.assertEqual(our_attr.__doc__, path_attr.__doc__)

    def test_concrete_class(self):
        if self.cls is pathlib.Path:
            expected = pathlib.WindowsPath if os.name == 'nt' else pathlib.PosixPath
        else:
            expected = self.cls
        p = self.cls('a')
        self.assertIs(type(p), expected)

    def test_unsupported_parser(self):
        if self.cls.parser is os.path:
            self.skipTest("path parser is supported")
        else:
            self.assertRaises(pathlib.UnsupportedOperation, self.cls)

    def _test_cwd(self, p):
        q = self.cls(os.getcwd())
        self.assertEqual(p, q)
        self.assertEqualNormCase(str(p), str(q))
        self.assertIs(type(p), type(q))
        self.assertTrue(p.is_absolute())

    def test_cwd(self):
        p = self.cls.cwd()
        self._test_cwd(p)

    def test_absolute_common(self):
        P = self.cls

        with mock.patch("os.getcwd") as getcwd:
            getcwd.return_value = self.base

            # Simple relative paths.
            self.assertEqual(str(P().absolute()), self.base)
            self.assertEqual(str(P('.').absolute()), self.base)
            self.assertEqual(str(P('a').absolute()), os.path.join(self.base, 'a'))
            self.assertEqual(str(P('a', 'b', 'c').absolute()), os.path.join(self.base, 'a', 'b', 'c'))

            # Symlinks should not be resolved.
            self.assertEqual(str(P('linkB', 'fileB').absolute()), os.path.join(self.base, 'linkB', 'fileB'))
            self.assertEqual(str(P('brokenLink').absolute()), os.path.join(self.base, 'brokenLink'))
            self.assertEqual(str(P('brokenLinkLoop').absolute()), os.path.join(self.base, 'brokenLinkLoop'))

            # '..' entries should be preserved and not normalised.
            self.assertEqual(str(P('..').absolute()), os.path.join(self.base, '..'))
            self.assertEqual(str(P('a', '..').absolute()), os.path.join(self.base, 'a', '..'))
            self.assertEqual(str(P('..', 'b').absolute()), os.path.join(self.base, '..', 'b'))

    def _test_home(self, p):
        q = self.cls(os.path.expanduser('~'))
        self.assertEqual(p, q)
        self.assertEqualNormCase(str(p), str(q))
        self.assertIs(type(p), type(q))
        self.assertTrue(p.is_absolute())

    @unittest.skipIf(
        pwd is None, reason="Test requires pwd module to get homedir."
    )
    def test_home(self):
        with os_helper.EnvironmentVarGuard() as env:
            self._test_home(self.cls.home())

            env.clear()
            env['USERPROFILE'] = os.path.join(self.base, 'userprofile')
            self._test_home(self.cls.home())

            # bpo-38883: ignore `HOME` when set on windows
            env['HOME'] = os.path.join(self.base, 'home')
            self._test_home(self.cls.home())

    @unittest.skipIf(is_wasi, "WASI has no user accounts.")
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
        p = P('~/a:b')
        self.assertEqual(p.expanduser(), P(os.path.expanduser('~'), './a:b'))

    def test_with_segments(self):
        class P(self.cls):
            def __init__(self, *pathsegments, session_id):
                super().__init__(*pathsegments)
                self.session_id = session_id

            def with_segments(self, *pathsegments):
                return type(self)(*pathsegments, session_id=self.session_id)
        p = P(self.base, session_id=42)
        self.assertEqual(42, p.absolute().session_id)
        self.assertEqual(42, p.resolve().session_id)
        if not is_wasi:  # WASI has no user accounts.
            self.assertEqual(42, p.with_segments('~').expanduser().session_id)
        self.assertEqual(42, (p / 'fileA').rename(p / 'fileB').session_id)
        self.assertEqual(42, (p / 'fileB').replace(p / 'fileA').session_id)
        if self.can_symlink:
            self.assertEqual(42, (p / 'linkA').readlink().session_id)
        for path in p.iterdir():
            self.assertEqual(42, path.session_id)
        for path in p.glob('*'):
            self.assertEqual(42, path.session_id)
        for path in p.rglob('*'):
            self.assertEqual(42, path.session_id)
        for dirpath, dirnames, filenames in p.walk():
            self.assertEqual(42, dirpath.session_id)

    def test_open_unbuffered(self):
        p = self.cls(self.base)
        with (p / 'fileA').open('rb', buffering=0) as f:
            self.assertIsInstance(f, io.RawIOBase)
            self.assertEqual(f.read().strip(), b"this is file A")

    def test_resolve_nonexist_relative_issue38671(self):
        p = self.cls('non', 'exist')

        old_cwd = os.getcwd()
        os.chdir(self.base)
        try:
            self.assertEqual(p.resolve(), self.cls(self.base, p))
        finally:
            os.chdir(old_cwd)

    @os_helper.skip_unless_working_chmod
    def test_chmod(self):
        p = self.cls(self.base) / 'fileA'
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
    @needs_posix
    @os_helper.skip_unless_working_chmod
    def test_chmod_follow_symlinks_true(self):
        p = self.cls(self.base) / 'linkA'
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

    def _get_pw_name_or_skip_test(self, uid):
        try:
            return pwd.getpwuid(uid).pw_name
        except KeyError:
            self.skipTest(
                "user %d doesn't have an entry in the system database" % uid)

    @unittest.skipUnless(pwd, "the pwd module is needed for this test")
    def test_owner(self):
        p = self.cls(self.base) / 'fileA'
        expected_uid = p.stat().st_uid
        expected_name = self._get_pw_name_or_skip_test(expected_uid)

        self.assertEqual(expected_name, p.owner())

    @unittest.skipUnless(pwd, "the pwd module is needed for this test")
    @unittest.skipUnless(root_in_posix, "test needs root privilege")
    def test_owner_no_follow_symlinks(self):
        all_users = [u.pw_uid for u in pwd.getpwall()]
        if len(all_users) < 2:
            self.skipTest("test needs more than one user")

        target = self.cls(self.base) / 'fileA'
        link = self.cls(self.base) / 'linkA'

        uid_1, uid_2 = all_users[:2]
        os.chown(target, uid_1, -1)
        os.chown(link, uid_2, -1, follow_symlinks=False)

        expected_uid = link.stat(follow_symlinks=False).st_uid
        expected_name = self._get_pw_name_or_skip_test(expected_uid)

        self.assertEqual(expected_uid, uid_2)
        self.assertEqual(expected_name, link.owner(follow_symlinks=False))

    def _get_gr_name_or_skip_test(self, gid):
        try:
            return grp.getgrgid(gid).gr_name
        except KeyError:
            self.skipTest(
                "group %d doesn't have an entry in the system database" % gid)

    @unittest.skipUnless(grp, "the grp module is needed for this test")
    def test_group(self):
        p = self.cls(self.base) / 'fileA'
        expected_gid = p.stat().st_gid
        expected_name = self._get_gr_name_or_skip_test(expected_gid)

        self.assertEqual(expected_name, p.group())

    @unittest.skipUnless(grp, "the grp module is needed for this test")
    @unittest.skipUnless(root_in_posix, "test needs root privilege")
    def test_group_no_follow_symlinks(self):
        all_groups = [g.gr_gid for g in grp.getgrall()]
        if len(all_groups) < 2:
            self.skipTest("test needs more than one group")

        target = self.cls(self.base) / 'fileA'
        link = self.cls(self.base) / 'linkA'

        gid_1, gid_2 = all_groups[:2]
        os.chown(target, -1, gid_1)
        os.chown(link, -1, gid_2, follow_symlinks=False)

        expected_gid = link.stat(follow_symlinks=False).st_gid
        expected_name = self._get_pw_name_or_skip_test(expected_gid)

        self.assertEqual(expected_gid, gid_2)
        self.assertEqual(expected_name, link.group(follow_symlinks=False))

    def test_unlink(self):
        p = self.cls(self.base) / 'fileA'
        p.unlink()
        self.assertFileNotFound(p.stat)
        self.assertFileNotFound(p.unlink)

    def test_unlink_missing_ok(self):
        p = self.cls(self.base) / 'fileAAA'
        self.assertFileNotFound(p.unlink)
        p.unlink(missing_ok=True)

    def test_rmdir(self):
        p = self.cls(self.base) / 'dirA'
        for q in p.iterdir():
            q.unlink()
        p.rmdir()
        self.assertFileNotFound(p.stat)
        self.assertFileNotFound(p.unlink)

    @os_helper.skip_unless_hardlink
    def test_hardlink_to(self):
        P = self.cls(self.base)
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
        target2 = self.parser.join(TESTFN, 'fileA')
        link2.hardlink_to(target2)
        self.assertEqual(os.stat(target2).st_size, size)
        self.assertTrue(link2.exists())

    @unittest.skipIf(hasattr(os, "link"), "os.link() is present")
    def test_hardlink_to_unsupported(self):
        P = self.cls(self.base)
        p = P / 'fileA'
        # linking to another path.
        q = P / 'dirA' / 'fileAA'
        with self.assertRaises(pathlib.UnsupportedOperation):
            q.hardlink_to(p)

    def test_rename(self):
        P = self.cls(self.base)
        p = P / 'fileA'
        size = p.stat().st_size
        # Renaming to another path.
        q = P / 'dirA' / 'fileAA'
        renamed_p = p.rename(q)
        self.assertEqual(renamed_p, q)
        self.assertEqual(q.stat().st_size, size)
        self.assertFileNotFound(p.stat)
        # Renaming to a str of a relative path.
        r = self.parser.join(TESTFN, 'fileAAA')
        renamed_q = q.rename(r)
        self.assertEqual(renamed_q, self.cls(r))
        self.assertEqual(os.stat(r).st_size, size)
        self.assertFileNotFound(q.stat)

    def test_replace(self):
        P = self.cls(self.base)
        p = P / 'fileA'
        size = p.stat().st_size
        # Replacing a non-existing path.
        q = P / 'dirA' / 'fileAA'
        replaced_p = p.replace(q)
        self.assertEqual(replaced_p, q)
        self.assertEqual(q.stat().st_size, size)
        self.assertFileNotFound(p.stat)
        # Replacing another (existing) path.
        r = self.parser.join(TESTFN, 'dirB', 'fileB')
        replaced_q = q.replace(r)
        self.assertEqual(replaced_q, self.cls(r))
        self.assertEqual(os.stat(r).st_size, size)
        self.assertFileNotFound(q.stat)

    def test_touch_common(self):
        P = self.cls(self.base)
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
        P = self.cls(self.base)
        p = P / 'fileA'
        p.touch()
        with p.open('rb') as f:
            self.assertEqual(f.read().strip(), b"this is file A")

    def test_mkdir(self):
        P = self.cls(self.base)
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
        p = self.cls(self.base, 'newdirB', 'newdirC')
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
        p = self.cls(self.base, 'newdirD', 'newdirE')
        p.mkdir(0o555, parents=True)
        self.assertTrue(p.exists())
        self.assertTrue(p.is_dir())
        if os.name != 'nt':
            # The directory's permissions follow the mode argument.
            self.assertEqual(stat.S_IMODE(p.stat().st_mode), 0o7555 & mode)
        # The parent's permissions follow the default process settings.
        self.assertEqual(stat.S_IMODE(p.parent.stat().st_mode), mode)

    def test_mkdir_exist_ok(self):
        p = self.cls(self.base, 'dirB')
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
        p = self.cls(self.base, 'dirC')
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

    @unittest.skipIf(is_emscripten, "FS root cannot be modified on Emscripten.")
    def test_mkdir_exist_ok_root(self):
        # Issue #25803: A drive root could raise PermissionError on Windows.
        self.cls('/').resolve().mkdir(exist_ok=True)
        self.cls('/').resolve().mkdir(parents=True, exist_ok=True)

    @needs_windows  # XXX: not sure how to test this on POSIX.
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
        p = self.cls(self.base, 'dirB', 'fileB')
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
        p = self.cls(self.base, 'fileA')
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
            p = self.cls(self.base, 'dirCPC%d' % pattern_num)
            self.assertFalse(p.exists())

            real_mkdir = os.mkdir
            def my_mkdir(path, mode=0o777):
                path = str(path)
                # Emulate another process that would create the directory
                # just before we try to create it ourselves.  We do it
                # in all possible pattern combinations, assuming that this
                # function is called at most 5 times (dirCPC/dir1/dir2,
                # dirCPC/dir1, dirCPC, dirCPC/dir1, dirCPC/dir1/dir2).
                if pattern.pop():
                    real_mkdir(path, mode)  # From another process.
                    concurrently_created.add(path)
                real_mkdir(path, mode)  # Our real call.

            pattern = [bool(pattern_num & (1 << n)) for n in range(5)]
            concurrently_created = set()
            p12 = p / 'dir1' / 'dir2'
            try:
                with mock.patch("os.mkdir", my_mkdir):
                    p12.mkdir(parents=True, exist_ok=False)
            except FileExistsError:
                self.assertIn(str(p12), concurrently_created)
            else:
                self.assertNotIn(str(p12), concurrently_created)
            self.assertTrue(p.exists())

    @needs_symlinks
    def test_symlink_to(self):
        P = self.cls(self.base)
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

    @unittest.skipIf(hasattr(os, "symlink"), "os.symlink() is present")
    def test_symlink_to_unsupported(self):
        P = self.cls(self.base)
        p = P / 'fileA'
        # linking to another path.
        q = P / 'dirA' / 'fileAA'
        with self.assertRaises(pathlib.UnsupportedOperation):
            q.symlink_to(p)

    def test_is_junction(self):
        P = self.cls(self.base)

        with mock.patch.object(P.parser, 'isjunction'):
            self.assertEqual(P.is_junction(), P.parser.isjunction.return_value)
            P.parser.isjunction.assert_called_once_with(P)

    @unittest.skipUnless(hasattr(os, "mkfifo"), "os.mkfifo() required")
    @unittest.skipIf(sys.platform == "vxworks",
                    "fifo requires special path on VxWorks")
    def test_is_fifo_true(self):
        P = self.cls(self.base, 'myfifo')
        try:
            os.mkfifo(str(P))
        except PermissionError as e:
            self.skipTest('os.mkfifo(): %s' % e)
        self.assertTrue(P.is_fifo())
        self.assertFalse(P.is_socket())
        self.assertFalse(P.is_file())
        self.assertIs(self.cls(self.base, 'myfifo\udfff').is_fifo(), False)
        self.assertIs(self.cls(self.base, 'myfifo\x00').is_fifo(), False)

    @unittest.skipUnless(hasattr(socket, "AF_UNIX"), "Unix sockets required")
    @unittest.skipIf(
        is_emscripten, "Unix sockets are not implemented on Emscripten."
    )
    @unittest.skipIf(
        is_wasi, "Cannot create socket on WASI."
    )
    def test_is_socket_true(self):
        P = self.cls(self.base, 'mysock')
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
        self.assertIs(self.cls(self.base, 'mysock\udfff').is_socket(), False)
        self.assertIs(self.cls(self.base, 'mysock\x00').is_socket(), False)

    def test_is_char_device_true(self):
        # os.devnull should generally be a char device.
        P = self.cls(os.devnull)
        if not P.exists():
            self.skipTest("null device required")
        self.assertTrue(P.is_char_device())
        self.assertFalse(P.is_block_device())
        self.assertFalse(P.is_file())
        self.assertIs(self.cls(f'{os.devnull}\udfff').is_char_device(), False)
        self.assertIs(self.cls(f'{os.devnull}\x00').is_char_device(), False)

    def test_is_mount_root(self):
        if os.name == 'nt':
            R = self.cls('c:\\')
        else:
            R = self.cls('/')
        self.assertTrue(R.is_mount())
        self.assertFalse((R / '\udfff').is_mount())

    def test_passing_kwargs_deprecated(self):
        with self.assertWarns(DeprecationWarning):
            self.cls(foo="bar")

    def setUpWalk(self):
        super().setUpWalk()
        sub21_path= self.sub2_path / "SUB21"
        tmp5_path = sub21_path / "tmp3"
        broken_link3_path = self.sub2_path / "broken_link3"

        os.makedirs(sub21_path)
        tmp5_path.write_text("I am tmp5, blame test_pathlib.")
        if self.can_symlink:
            os.symlink(tmp5_path, broken_link3_path)
            self.sub2_tree[2].append('broken_link3')
            self.sub2_tree[2].sort()
        if not is_emscripten:
            # Emscripten fails with inaccessible directories.
            os.chmod(sub21_path, 0)
        try:
            os.listdir(sub21_path)
        except PermissionError:
            self.sub2_tree[1].append('SUB21')
        else:
            os.chmod(sub21_path, stat.S_IRWXU)
            os.unlink(tmp5_path)
            os.rmdir(sub21_path)

    def test_walk_bad_dir(self):
        self.setUpWalk()
        errors = []
        walk_it = self.walk_path.walk(on_error=errors.append)
        root, dirs, files = next(walk_it)
        self.assertEqual(errors, [])
        dir1 = 'SUB1'
        path1 = root / dir1
        path1new = (root / dir1).with_suffix(".new")
        path1.rename(path1new)
        try:
            roots = [r for r, _, _ in walk_it]
            self.assertTrue(errors)
            self.assertNotIn(path1, roots)
            self.assertNotIn(path1new, roots)
            for dir2 in dirs:
                if dir2 != dir1:
                    self.assertIn(root / dir2, roots)
        finally:
            path1new.rename(path1)

    def test_walk_many_open_files(self):
        depth = 30
        base = self.cls(self.base, 'deep')
        path = self.cls(base, *(['d']*depth))
        path.mkdir(parents=True)

        iters = [base.walk(top_down=False) for _ in range(100)]
        for i in range(depth + 1):
            expected = (path, ['d'] if i else [], [])
            for it in iters:
                self.assertEqual(next(it), expected)
            path = path.parent

        iters = [base.walk(top_down=True) for _ in range(100)]
        path = base
        for i in range(depth + 1):
            expected = (path, ['d'] if i < depth else [], [])
            for it in iters:
                self.assertEqual(next(it), expected)
            path = path / 'd'

    def test_walk_above_recursion_limit(self):
        recursion_limit = 40
        # directory_depth > recursion_limit
        directory_depth = recursion_limit + 10
        base = self.cls(self.base, 'deep')
        path = base.joinpath(*(['d'] * directory_depth))
        path.mkdir(parents=True)

        with infinite_recursion(recursion_limit):
            list(base.walk())
            list(base.walk(top_down=False))

    def test_glob_empty_pattern(self):
        p = self.cls('')
        with self.assertRaisesRegex(ValueError, 'Unacceptable pattern'):
            list(p.glob(''))
        with self.assertRaisesRegex(ValueError, 'Unacceptable pattern'):
            list(p.glob('.'))
        with self.assertRaisesRegex(ValueError, 'Unacceptable pattern'):
            list(p.glob('./'))

    def test_glob_many_open_files(self):
        depth = 30
        P = self.cls
        p = base = P(self.base) / 'deep'
        p.mkdir()
        for _ in range(depth):
            p /= 'd'
            p.mkdir()
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

    def test_glob_above_recursion_limit(self):
        recursion_limit = 50
        # directory_depth > recursion_limit
        directory_depth = recursion_limit + 10
        base = self.cls(self.base, 'deep')
        path = base.joinpath(*(['d'] * directory_depth))
        path.mkdir(parents=True)

        with infinite_recursion(recursion_limit):
            list(base.glob('**/'))

    def test_glob_pathlike(self):
        P = self.cls
        p = P(self.base)
        pattern = "dir*/file*"
        expect = {p / "dirB/fileB", p / "dirC/fileC"}
        self.assertEqual(expect, set(p.glob(P(pattern))))
        self.assertEqual(expect, set(p.glob(FakePath(pattern))))

    @needs_symlinks
    def test_glob_dot(self):
        P = self.cls
        with os_helper.change_cwd(P(self.base, "dirC")):
            self.assertEqual(
                set(P('.').glob('*')), {P("fileC"), P("novel.txt"), P("dirD")})
            self.assertEqual(
                set(P('.').glob('**')), {P("fileC"), P("novel.txt"), P("dirD"), P("dirD/fileD"), P(".")})
            self.assertEqual(
                set(P('.').glob('**/*')), {P("fileC"), P("novel.txt"), P("dirD"), P("dirD/fileD")})
            self.assertEqual(
                set(P('.').glob('**/*/*')), {P("dirD/fileD")})

    def test_glob_inaccessible(self):
        P = self.cls
        p = P(self.base, "mydir1", "mydir2")
        p.mkdir(parents=True)
        p.parent.chmod(0)
        self.assertEqual(set(p.glob('*')), set())

    def test_rglob_pathlike(self):
        P = self.cls
        p = P(self.base, "dirC")
        pattern = "**/file*"
        expect = {p / "fileC", p / "dirD/fileD"}
        self.assertEqual(expect, set(p.rglob(P(pattern))))
        self.assertEqual(expect, set(p.rglob(FakePath(pattern))))

    @needs_posix
    def test_absolute_posix(self):
        P = self.cls
        self.assertEqual(str(P('/').absolute()), '/')
        self.assertEqual(str(P('/a').absolute()), '/a')
        self.assertEqual(str(P('/a/b').absolute()), '/a/b')

        # '//'-prefixed absolute path (supported by POSIX).
        self.assertEqual(str(P('//').absolute()), '//')
        self.assertEqual(str(P('//a').absolute()), '//a')
        self.assertEqual(str(P('//a/b').absolute()), '//a/b')

    @unittest.skipIf(
        is_emscripten or is_wasi,
        "umask is not implemented on Emscripten/WASI."
    )
    @needs_posix
    def test_open_mode(self):
        # Unmask all permissions except world-write, which may
        # not be supported on some filesystems (see GH-85633.)
        old_mask = os.umask(0o002)
        self.addCleanup(os.umask, old_mask)
        p = self.cls(self.base)
        with (p / 'new_file').open('wb'):
            pass
        st = os.stat(self.parser.join(self.base, 'new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o664)
        os.umask(0o026)
        with (p / 'other_new_file').open('wb'):
            pass
        st = os.stat(self.parser.join(self.base, 'other_new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o640)

    @needs_posix
    def test_resolve_root(self):
        current_directory = os.getcwd()
        try:
            os.chdir('/')
            p = self.cls('spam')
            self.assertEqual(str(p.resolve()), '/spam')
        finally:
            os.chdir(current_directory)

    @unittest.skipIf(
        is_emscripten or is_wasi,
        "umask is not implemented on Emscripten/WASI."
    )
    @needs_posix
    def test_touch_mode(self):
        # Unmask all permissions except world-write, which may
        # not be supported on some filesystems (see GH-85633.)
        old_mask = os.umask(0o002)
        self.addCleanup(os.umask, old_mask)
        p = self.cls(self.base)
        (p / 'new_file').touch()
        st = os.stat(self.parser.join(self.base, 'new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o664)
        os.umask(0o026)
        (p / 'other_new_file').touch()
        st = os.stat(self.parser.join(self.base, 'other_new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o640)
        (p / 'masked_new_file').touch(mode=0o750)
        st = os.stat(self.parser.join(self.base, 'masked_new_file'))
        self.assertEqual(stat.S_IMODE(st.st_mode), 0o750)

    @unittest.skipUnless(hasattr(pwd, 'getpwall'),
                         'pwd module does not expose getpwall()')
    @unittest.skipIf(sys.platform == "vxworks",
                     "no home directory on VxWorks")
    @needs_posix
    def test_expanduser_posix(self):
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

        fakename = 'fakeuser'
        # This user can theoretically exist on a test runner. Create unique name:
        try:
            while pwd.getpwnam(fakename):
                fakename += '1'
        except KeyError:
            pass  # Non-existent name found

        p1 = P('~/Documents')
        p2 = P(f'~{username}/Documents')
        p3 = P(f'~{othername}/Documents')
        p4 = P(f'../~{username}/Documents')
        p5 = P(f'/~{username}/Documents')
        p6 = P('')
        p7 = P(f'~{fakename}/Documents')

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
    @needs_posix
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

    @needs_posix
    def test_from_uri_posix(self):
        P = self.cls
        self.assertEqual(P.from_uri('file:/foo/bar'), P('/foo/bar'))
        self.assertEqual(P.from_uri('file://foo/bar'), P('//foo/bar'))
        self.assertEqual(P.from_uri('file:///foo/bar'), P('/foo/bar'))
        self.assertEqual(P.from_uri('file:////foo/bar'), P('//foo/bar'))
        self.assertEqual(P.from_uri('file://localhost/foo/bar'), P('/foo/bar'))
        self.assertRaises(ValueError, P.from_uri, 'foo/bar')
        self.assertRaises(ValueError, P.from_uri, '/foo/bar')
        self.assertRaises(ValueError, P.from_uri, '//foo/bar')
        self.assertRaises(ValueError, P.from_uri, 'file:foo/bar')
        self.assertRaises(ValueError, P.from_uri, 'http://foo/bar')

    @needs_posix
    def test_from_uri_pathname2url_posix(self):
        P = self.cls
        self.assertEqual(P.from_uri('file:' + pathname2url('/foo/bar')), P('/foo/bar'))
        self.assertEqual(P.from_uri('file:' + pathname2url('//foo/bar')), P('//foo/bar'))

    @needs_windows
    def test_absolute_windows(self):
        P = self.cls

        # Simple absolute paths.
        self.assertEqual(str(P('c:\\').absolute()), 'c:\\')
        self.assertEqual(str(P('c:\\a').absolute()), 'c:\\a')
        self.assertEqual(str(P('c:\\a\\b').absolute()), 'c:\\a\\b')

        # UNC absolute paths.
        share = '\\\\server\\share\\'
        self.assertEqual(str(P(share).absolute()), share)
        self.assertEqual(str(P(share + 'a').absolute()), share + 'a')
        self.assertEqual(str(P(share + 'a\\b').absolute()), share + 'a\\b')

        # UNC relative paths.
        with mock.patch("os.getcwd") as getcwd:
            getcwd.return_value = share

            self.assertEqual(str(P().absolute()), share)
            self.assertEqual(str(P('.').absolute()), share)
            self.assertEqual(str(P('a').absolute()), os.path.join(share, 'a'))
            self.assertEqual(str(P('a', 'b', 'c').absolute()),
                             os.path.join(share, 'a', 'b', 'c'))

        drive = os.path.splitdrive(self.base)[0]
        with os_helper.change_cwd(self.base):
            # Relative path with root
            self.assertEqual(str(P('\\').absolute()), drive + '\\')
            self.assertEqual(str(P('\\foo').absolute()), drive + '\\foo')

            # Relative path on current drive
            self.assertEqual(str(P(drive).absolute()), self.base)
            self.assertEqual(str(P(drive + 'foo').absolute()), os.path.join(self.base, 'foo'))

        with os_helper.subst_drive(self.base) as other_drive:
            # Set the working directory on the substitute drive
            saved_cwd = os.getcwd()
            other_cwd = f'{other_drive}\\dirA'
            os.chdir(other_cwd)
            os.chdir(saved_cwd)

            # Relative path on another drive
            self.assertEqual(str(P(other_drive).absolute()), other_cwd)
            self.assertEqual(str(P(other_drive + 'foo').absolute()), other_cwd + '\\foo')

    @needs_windows
    def test_expanduser_windows(self):
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

    @needs_windows
    def test_from_uri_windows(self):
        P = self.cls
        # DOS drive paths
        self.assertEqual(P.from_uri('file:c:/path/to/file'), P('c:/path/to/file'))
        self.assertEqual(P.from_uri('file:c|/path/to/file'), P('c:/path/to/file'))
        self.assertEqual(P.from_uri('file:/c|/path/to/file'), P('c:/path/to/file'))
        self.assertEqual(P.from_uri('file:///c|/path/to/file'), P('c:/path/to/file'))
        # UNC paths
        self.assertEqual(P.from_uri('file://server/path/to/file'), P('//server/path/to/file'))
        self.assertEqual(P.from_uri('file:////server/path/to/file'), P('//server/path/to/file'))
        self.assertEqual(P.from_uri('file://///server/path/to/file'), P('//server/path/to/file'))
        # Localhost paths
        self.assertEqual(P.from_uri('file://localhost/c:/path/to/file'), P('c:/path/to/file'))
        self.assertEqual(P.from_uri('file://localhost/c|/path/to/file'), P('c:/path/to/file'))
        # Invalid paths
        self.assertRaises(ValueError, P.from_uri, 'foo/bar')
        self.assertRaises(ValueError, P.from_uri, 'c:/foo/bar')
        self.assertRaises(ValueError, P.from_uri, '//foo/bar')
        self.assertRaises(ValueError, P.from_uri, 'file:foo/bar')
        self.assertRaises(ValueError, P.from_uri, 'http://foo/bar')

    @needs_windows
    def test_from_uri_pathname2url_windows(self):
        P = self.cls
        self.assertEqual(P.from_uri('file:' + pathname2url(r'c:\path\to\file')), P('c:/path/to/file'))
        self.assertEqual(P.from_uri('file:' + pathname2url(r'\\server\path\to\file')), P('//server/path/to/file'))

    @needs_windows
    def test_owner_windows(self):
        P = self.cls
        with self.assertRaises(pathlib.UnsupportedOperation):
            P('c:/').owner()

    @needs_windows
    def test_group_windows(self):
        P = self.cls
        with self.assertRaises(pathlib.UnsupportedOperation):
            P('c:/').group()


@unittest.skipIf(os.name == 'nt', 'test requires a POSIX-compatible system')
class PosixPathTest(PathTest, PurePosixPathTest):
    cls = pathlib.PosixPath


@unittest.skipIf(os.name != 'nt', 'test requires a Windows-compatible system')
class WindowsPathTest(PathTest, PureWindowsPathTest):
    cls = pathlib.WindowsPath


class PathSubclassTest(PathTest):
    class cls(pathlib.Path):
        pass

    # repr() roundtripping is not supported in custom subclass.
    test_repr_roundtrips = None


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
