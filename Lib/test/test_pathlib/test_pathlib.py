import collections
import contextlib
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
from test.support import swap_attr
from test.support import os_helper
from test.support.os_helper import TESTFN, FakePath
from test.test_pathlib import test_pathlib_abc
from test.test_pathlib.test_pathlib_abc import needs_posix, needs_windows

try:
    import fcntl
except ImportError:
    fcntl = None
try:
    import grp, pwd
except ImportError:
    grp = pwd = None
try:
    import posix
except ImportError:
    posix = None


root_in_posix = False
if hasattr(os, 'geteuid'):
    root_in_posix = (os.geteuid() == 0)


def patch_replace(old_test):
    def new_replace(self, target):
        raise OSError(errno.EXDEV, "Cross-device link", self, target)

    def new_test(self):
        old_replace = self.cls.replace
        self.cls.replace = new_replace
        try:
            old_test(self)
        finally:
            self.cls.replace = old_replace
    return new_test


_tests_needing_symlinks = set()
def needs_symlinks(fn):
    """Decorator that marks a test as requiring a path class that supports symlinks."""
    _tests_needing_symlinks.add(fn.__name__)
    return fn



class UnsupportedOperationTest(unittest.TestCase):
    def test_is_notimplemented(self):
        self.assertTrue(issubclass(pathlib.UnsupportedOperation, NotImplementedError))
        self.assertTrue(isinstance(pathlib.UnsupportedOperation(), NotImplementedError))


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
        name = self.id().split('.')[-1]
        if name in _tests_needing_symlinks and not self.can_symlink:
            self.skipTest('requires symlinks')
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

    def test_matches_pathbase_docstrings(self):
        path_names = {name for name in dir(pathlib._abc.PathBase) if name[0] != '_'}
        for attr_name in path_names:
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

    def test_copy_file_preserve_metadata(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        if hasattr(os, 'chmod'):
            os.chmod(source, stat.S_IRWXU | stat.S_IRWXO)
        if hasattr(os, 'chflags') and hasattr(stat, 'UF_NODUMP'):
            os.chflags(source, stat.UF_NODUMP)
        source_st = source.stat()
        target = base / 'copyA'
        source.copy(target, preserve_metadata=True)
        self.assertTrue(target.exists())
        self.assertEqual(source.read_text(), target.read_text())
        target_st = target.stat()
        self.assertLessEqual(source_st.st_atime, target_st.st_atime)
        self.assertLessEqual(source_st.st_mtime, target_st.st_mtime)
        self.assertEqual(source_st.st_mode, target_st.st_mode)
        if hasattr(source_st, 'st_flags'):
            self.assertEqual(source_st.st_flags, target_st.st_flags)

    @needs_symlinks
    def test_copy_file_to_existing_symlink(self):
        base = self.cls(self.base)
        source = base / 'dirB' / 'fileB'
        target = base / 'linkA'
        real_target = base / 'fileA'
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(target.exists())
        self.assertTrue(target.is_symlink())
        self.assertTrue(real_target.exists())
        self.assertFalse(real_target.is_symlink())
        self.assertEqual(source.read_text(), real_target.read_text())

    @needs_symlinks
    def test_copy_file_to_existing_symlink_follow_symlinks_false(self):
        base = self.cls(self.base)
        source = base / 'dirB' / 'fileB'
        target = base / 'linkA'
        real_target = base / 'fileA'
        result = source.copy(target, follow_symlinks=False)
        self.assertEqual(result, target)
        self.assertTrue(target.exists())
        self.assertTrue(target.is_symlink())
        self.assertTrue(real_target.exists())
        self.assertFalse(real_target.is_symlink())
        self.assertEqual(source.read_text(), real_target.read_text())

    @os_helper.skip_unless_xattr
    def test_copy_file_preserve_metadata_xattrs(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        os.setxattr(source, b'user.foo', b'42')
        target = base / 'copyA'
        source.copy(target, preserve_metadata=True)
        self.assertEqual(os.getxattr(target, b'user.foo'), b'42')

    @needs_symlinks
    def test_copy_symlink_follow_symlinks_true(self):
        base = self.cls(self.base)
        source = base / 'linkA'
        target = base / 'copyA'
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(target.exists())
        self.assertFalse(target.is_symlink())
        self.assertEqual(source.read_text(), target.read_text())

    @needs_symlinks
    def test_copy_symlink_follow_symlinks_false(self):
        base = self.cls(self.base)
        source = base / 'linkA'
        target = base / 'copyA'
        result = source.copy(target, follow_symlinks=False)
        self.assertEqual(result, target)
        self.assertTrue(target.exists())
        self.assertTrue(target.is_symlink())
        self.assertEqual(source.readlink(), target.readlink())

    @needs_symlinks
    def test_copy_symlink_to_itself(self):
        base = self.cls(self.base)
        source = base / 'linkA'
        self.assertRaises(OSError, source.copy, source)

    @needs_symlinks
    def test_copy_symlink_to_existing_symlink(self):
        base = self.cls(self.base)
        source = base / 'copySource'
        target = base / 'copyTarget'
        source.symlink_to(base / 'fileA')
        target.symlink_to(base / 'dirC')
        self.assertRaises(OSError, source.copy, target)
        self.assertRaises(OSError, source.copy, target, follow_symlinks=False)

    @needs_symlinks
    def test_copy_symlink_to_existing_directory_symlink(self):
        base = self.cls(self.base)
        source = base / 'copySource'
        target = base / 'copyTarget'
        source.symlink_to(base / 'fileA')
        target.symlink_to(base / 'dirC')
        self.assertRaises(OSError, source.copy, target)
        self.assertRaises(OSError, source.copy, target, follow_symlinks=False)

    @needs_symlinks
    def test_copy_directory_symlink_follow_symlinks_false(self):
        base = self.cls(self.base)
        source = base / 'linkB'
        target = base / 'copyA'
        result = source.copy(target, follow_symlinks=False)
        self.assertEqual(result, target)
        self.assertTrue(target.exists())
        self.assertTrue(target.is_symlink())
        self.assertEqual(source.readlink(), target.readlink())

    @needs_symlinks
    def test_copy_directory_symlink_to_itself(self):
        base = self.cls(self.base)
        source = base / 'linkB'
        self.assertRaises(OSError, source.copy, source)
        self.assertRaises(OSError, source.copy, source, follow_symlinks=False)

    @needs_symlinks
    def test_copy_directory_symlink_into_itself(self):
        base = self.cls(self.base)
        source = base / 'linkB'
        target = base / 'linkB' / 'copyB'
        self.assertRaises(OSError, source.copy, target)
        self.assertRaises(OSError, source.copy, target, follow_symlinks=False)
        self.assertFalse(target.exists())

    @needs_symlinks
    def test_copy_directory_symlink_to_existing_symlink(self):
        base = self.cls(self.base)
        source = base / 'copySource'
        target = base / 'copyTarget'
        source.symlink_to(base / 'dirC')
        target.symlink_to(base / 'fileA')
        self.assertRaises(FileExistsError, source.copy, target)
        self.assertRaises(FileExistsError, source.copy, target, follow_symlinks=False)

    @needs_symlinks
    def test_copy_directory_symlink_to_existing_directory_symlink(self):
        base = self.cls(self.base)
        source = base / 'copySource'
        target = base / 'copyTarget'
        source.symlink_to(base / 'dirC' / 'dirD')
        target.symlink_to(base / 'dirC')
        self.assertRaises(FileExistsError, source.copy, target)
        self.assertRaises(FileExistsError, source.copy, target, follow_symlinks=False)

    @needs_symlinks
    def test_copy_dangling_symlink(self):
        base = self.cls(self.base)
        source = base / 'source'
        target = base / 'target'

        source.mkdir()
        source.joinpath('link').symlink_to('nonexistent')

        self.assertRaises(FileNotFoundError, source.copy, target)

        target2 = base / 'target2'
        result = source.copy(target2, follow_symlinks=False)
        self.assertEqual(result, target2)
        self.assertTrue(target2.joinpath('link').is_symlink())
        self.assertEqual(target2.joinpath('link').readlink(), self.cls('nonexistent'))

    @needs_symlinks
    def test_copy_link_preserve_metadata(self):
        base = self.cls(self.base)
        source = base / 'linkA'
        if hasattr(os, 'lchmod'):
            os.lchmod(source, stat.S_IRWXU | stat.S_IRWXO)
        if hasattr(os, 'lchflags') and hasattr(stat, 'UF_NODUMP'):
            os.lchflags(source, stat.UF_NODUMP)
        source_st = source.lstat()
        target = base / 'copyA'
        source.copy(target, follow_symlinks=False, preserve_metadata=True)
        self.assertTrue(target.exists())
        self.assertTrue(target.is_symlink())
        self.assertEqual(source.readlink(), target.readlink())
        target_st = target.lstat()
        self.assertLessEqual(source_st.st_atime, target_st.st_atime)
        self.assertLessEqual(source_st.st_mtime, target_st.st_mtime)
        self.assertEqual(source_st.st_mode, target_st.st_mode)
        if hasattr(source_st, 'st_flags'):
            self.assertEqual(source_st.st_flags, target_st.st_flags)

    def test_copy_error_handling(self):
        def make_raiser(err):
            def raiser(*args, **kwargs):
                raise OSError(err, os.strerror(err))
            return raiser

        base = self.cls(self.base)
        source = base / 'fileA'
        target = base / 'copyA'

        # Raise non-fatal OSError from all available fast copy functions.
        with contextlib.ExitStack() as ctx:
            if fcntl and hasattr(fcntl, 'FICLONE'):
                ctx.enter_context(mock.patch('fcntl.ioctl', make_raiser(errno.EXDEV)))
            if posix and hasattr(posix, '_fcopyfile'):
                ctx.enter_context(mock.patch('posix._fcopyfile', make_raiser(errno.ENOTSUP)))
            if hasattr(os, 'copy_file_range'):
                ctx.enter_context(mock.patch('os.copy_file_range', make_raiser(errno.EXDEV)))
            if hasattr(os, 'sendfile'):
                ctx.enter_context(mock.patch('os.sendfile', make_raiser(errno.ENOTSOCK)))

            source.copy(target)
            self.assertTrue(target.exists())
            self.assertEqual(source.read_text(), target.read_text())

        # Raise fatal OSError from first available fast copy function.
        if fcntl and hasattr(fcntl, 'FICLONE'):
            patchpoint = 'fcntl.ioctl'
        elif posix and hasattr(posix, '_fcopyfile'):
            patchpoint = 'posix._fcopyfile'
        elif hasattr(os, 'copy_file_range'):
            patchpoint = 'os.copy_file_range'
        elif hasattr(os, 'sendfile'):
            patchpoint = 'os.sendfile'
        else:
            return
        with mock.patch(patchpoint, make_raiser(errno.ENOENT)):
            self.assertRaises(FileNotFoundError, source.copy, target)

    @unittest.skipIf(sys.platform == "win32" or sys.platform == "wasi", "directories are always readable on Windows and WASI")
    @unittest.skipIf(root_in_posix, "test fails with root privilege")
    def test_copy_dir_no_read_permission(self):
        base = self.cls(self.base)
        source = base / 'dirE'
        target = base / 'copyE'
        self.assertRaises(PermissionError, source.copy, target)
        self.assertFalse(target.exists())

    def test_copy_dir_preserve_metadata(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        if hasattr(os, 'chmod'):
            os.chmod(source / 'dirD', stat.S_IRWXU | stat.S_IRWXO)
        if hasattr(os, 'chflags') and hasattr(stat, 'UF_NODUMP'):
            os.chflags(source / 'fileC', stat.UF_NODUMP)
        target = base / 'copyA'
        source.copy(target, preserve_metadata=True)

        for subpath in ['.', 'fileC', 'dirD', 'dirD/fileD']:
            source_st = source.joinpath(subpath).stat()
            target_st = target.joinpath(subpath).stat()
            self.assertLessEqual(source_st.st_atime, target_st.st_atime)
            self.assertLessEqual(source_st.st_mtime, target_st.st_mtime)
            self.assertEqual(source_st.st_mode, target_st.st_mode)
            if hasattr(source_st, 'st_flags'):
                self.assertEqual(source_st.st_flags, target_st.st_flags)

    @os_helper.skip_unless_xattr
    def test_copy_dir_preserve_metadata_xattrs(self):
        base = self.cls(self.base)
        source = base / 'dirC'
        source_file = source.joinpath('dirD', 'fileD')
        os.setxattr(source_file, b'user.foo', b'42')
        target = base / 'copyA'
        source.copy(target, preserve_metadata=True)
        target_file = target.joinpath('dirD', 'fileD')
        self.assertEqual(os.getxattr(target_file, b'user.foo'), b'42')

    @needs_symlinks
    def test_move_file_symlink(self):
        base = self.cls(self.base)
        source = base / 'linkA'
        source_readlink = source.readlink()
        target = base / 'linkA_moved'
        result = source.move(target)
        self.assertEqual(result, target)
        self.assertFalse(source.exists())
        self.assertTrue(target.is_symlink())
        self.assertEqual(source_readlink, target.readlink())

    @needs_symlinks
    def test_move_file_symlink_to_itself(self):
        base = self.cls(self.base)
        source = base / 'linkA'
        self.assertRaises(OSError, source.move, source)

    @needs_symlinks
    def test_move_dir_symlink(self):
        base = self.cls(self.base)
        source = base / 'linkB'
        source_readlink = source.readlink()
        target = base / 'linkB_moved'
        result = source.move(target)
        self.assertEqual(result, target)
        self.assertFalse(source.exists())
        self.assertTrue(target.is_symlink())
        self.assertEqual(source_readlink, target.readlink())

    @needs_symlinks
    def test_move_dir_symlink_to_itself(self):
        base = self.cls(self.base)
        source = base / 'linkB'
        self.assertRaises(OSError, source.move, source)

    @needs_symlinks
    def test_move_dangling_symlink(self):
        base = self.cls(self.base)
        source = base / 'brokenLink'
        source_readlink = source.readlink()
        target = base / 'brokenLink_moved'
        result = source.move(target)
        self.assertEqual(result, target)
        self.assertFalse(source.exists())
        self.assertTrue(target.is_symlink())
        self.assertEqual(source_readlink, target.readlink())

    @patch_replace
    def test_move_file_other_fs(self):
        self.test_move_file()

    @patch_replace
    def test_move_file_to_file_other_fs(self):
        self.test_move_file_to_file()

    @patch_replace
    def test_move_file_to_dir_other_fs(self):
        self.test_move_file_to_dir()

    @patch_replace
    def test_move_dir_other_fs(self):
        self.test_move_dir()

    @patch_replace
    def test_move_dir_to_dir_other_fs(self):
        self.test_move_dir_to_dir()

    @patch_replace
    def test_move_dir_into_itself_other_fs(self):
        self.test_move_dir_into_itself()

    @patch_replace
    @needs_symlinks
    def test_move_file_symlink_other_fs(self):
        self.test_move_file_symlink()

    @patch_replace
    @needs_symlinks
    def test_move_file_symlink_to_itself_other_fs(self):
        self.test_move_file_symlink_to_itself()

    @patch_replace
    @needs_symlinks
    def test_move_dir_symlink_other_fs(self):
        self.test_move_dir_symlink()

    @patch_replace
    @needs_symlinks
    def test_move_dir_symlink_to_itself_other_fs(self):
        self.test_move_dir_symlink_to_itself()

    @patch_replace
    @needs_symlinks
    def test_move_dangling_symlink_other_fs(self):
        self.test_move_dangling_symlink()

    @patch_replace
    def test_move_into_other_os(self):
        self.test_move_into()

    @patch_replace
    def test_move_into_empty_name_other_os(self):
        self.test_move_into_empty_name()

    @needs_symlinks
    def test_complex_symlinks_absolute(self):
        self._check_complex_symlinks(self.base)

    @needs_symlinks
    def test_complex_symlinks_relative(self):
        self._check_complex_symlinks('.')

    @needs_symlinks
    def test_complex_symlinks_relative_dot_dot(self):
        self._check_complex_symlinks(self.parser.join('dirA', '..'))

    def _check_complex_symlinks(self, link0_target):
        # Test solving a non-looping chain of symlinks (issue #19887).
        parser = self.parser
        P = self.cls(self.base)
        P.joinpath('link1').symlink_to(parser.join('link0', 'link0'), target_is_directory=True)
        P.joinpath('link2').symlink_to(parser.join('link1', 'link1'), target_is_directory=True)
        P.joinpath('link3').symlink_to(parser.join('link2', 'link2'), target_is_directory=True)
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

    def _check_resolve(self, p, expected, strict=True):
        q = p.resolve(strict)
        self.assertEqual(q, expected)

    # This can be used to check both relative and absolute resolutions.
    _check_resolve_relative = _check_resolve_absolute = _check_resolve

    @needs_symlinks
    def test_resolve_common(self):
        P = self.cls
        p = P(self.base, 'foo')
        with self.assertRaises(OSError) as cm:
            p.resolve(strict=True)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        # Non-strict
        parser = self.parser
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 parser.join(self.base, 'foo'))
        p = P(self.base, 'foo', 'in', 'spam')
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 parser.join(self.base, 'foo', 'in', 'spam'))
        p = P(self.base, '..', 'foo', 'in', 'spam')
        self.assertEqualNormCase(str(p.resolve(strict=False)),
                                 parser.join(parser.dirname(self.base), 'foo', 'in', 'spam'))
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
        if self.cls.parser is not posixpath:
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
        P(self.base, str(d), 'linkY').symlink_to(self.parser.join(self.base, 'dirB'))
        p = P(self.base, 'dirA', 'linkX', 'linkY', 'fileB')
        self._check_resolve_absolute(p, P(self.base, 'dirB', 'fileB'))
        # Non-strict
        p = P(self.base, 'dirA', 'linkX', 'linkY', 'foo', 'in', 'spam')
        self._check_resolve_relative(p, P(self.base, 'dirB', 'foo', 'in', 'spam'),
                                     False)
        p = P(self.base, 'dirA', 'linkX', 'linkY', '..', 'foo', 'in', 'spam')
        if self.cls.parser is not posixpath:
            # In Windows, if linkY points to dirB, 'dirA\linkY\..'
            # resolves to 'dirA' without resolving linkY first.
            self._check_resolve_relative(p, P(d, 'foo', 'in', 'spam'), False)
        else:
            # In Posix, if linkY points to dirB, 'dirA/linkY/..'
            # resolves to 'dirB/..' first before resolving to parent of dirB.
            self._check_resolve_relative(p, P(self.base, 'foo', 'in', 'spam'), False)

    @needs_symlinks
    def test_resolve_dot(self):
        # See http://web.archive.org/web/20200623062557/https://bitbucket.org/pitrou/pathlib/issues/9/
        parser = self.parser
        p = self.cls(self.base)
        p.joinpath('0').symlink_to('.', target_is_directory=True)
        p.joinpath('1').symlink_to(parser.join('0', '0'), target_is_directory=True)
        p.joinpath('2').symlink_to(parser.join('1', '1'), target_is_directory=True)
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

    @needs_posix
    @needs_symlinks
    def test_resolve_loop(self):
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
        self.cls(self.base, 'linkU').symlink_to(self.parser.join(self.base, 'linkU/inside'))
        self._check_symlink_loop(self.base, 'linkU')
        self.cls(self.base, 'linkV').symlink_to(self.parser.join(self.base, 'linkV'))
        self._check_symlink_loop(self.base, 'linkV')
        self.cls(self.base, 'linkW').symlink_to(self.parser.join(self.base, 'linkW/../linkW'))
        self._check_symlink_loop(self.base, 'linkW')
        # Non-strict
        q = self.cls(self.base, 'linkW', 'foo')
        self.assertEqual(q.resolve(strict=False), q)

    def test_resolve_nonexist_relative_issue38671(self):
        p = self.cls('non', 'exist')

        old_cwd = os.getcwd()
        os.chdir(self.base)
        try:
            self.assertEqual(p.resolve(), self.cls(self.base, p))
        finally:
            os.chdir(old_cwd)

    @needs_symlinks
    def test_readlink(self):
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
        with self.assertRaises(pathlib.UnsupportedOperation):
            q.readlink(p)

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

    @needs_symlinks
    def test_delete_symlink(self):
        tmp = self.cls(self.base, 'delete')
        tmp.mkdir()
        dir_ = tmp / 'dir'
        dir_.mkdir()
        link = tmp / 'link'
        link.symlink_to(dir_)
        link._delete()
        self.assertTrue(dir_.exists())
        self.assertFalse(link.exists(follow_symlinks=False))

    @needs_symlinks
    def test_delete_inner_symlink(self):
        tmp = self.cls(self.base, 'delete')
        tmp.mkdir()
        dir1 = tmp / 'dir1'
        dir2 = dir1 / 'dir2'
        dir3 = tmp / 'dir3'
        for d in dir1, dir2, dir3:
            d.mkdir()
        file1 = tmp / 'file1'
        file1.write_text('foo')
        link1 = dir1 / 'link1'
        link1.symlink_to(dir2)
        link2 = dir1 / 'link2'
        link2.symlink_to(dir3)
        link3 = dir1 / 'link3'
        link3.symlink_to(file1)
        # make sure symlinks are removed but not followed
        dir1._delete()
        self.assertFalse(dir1.exists())
        self.assertTrue(dir3.exists())
        self.assertTrue(file1.exists())

    @unittest.skipIf(sys.platform[:6] == 'cygwin',
                     "This test can't be run on Cygwin (issue #1071513).")
    @os_helper.skip_if_dac_override
    @os_helper.skip_unless_working_chmod
    def test_delete_unwritable(self):
        tmp = self.cls(self.base, 'delete')
        tmp.mkdir()
        child_file_path = tmp / 'a'
        child_dir_path = tmp / 'b'
        child_file_path.write_text("")
        child_dir_path.mkdir()
        old_dir_mode = tmp.stat().st_mode
        old_child_file_mode = child_file_path.stat().st_mode
        old_child_dir_mode = child_dir_path.stat().st_mode
        # Make unwritable.
        new_mode = stat.S_IREAD | stat.S_IEXEC
        try:
            child_file_path.chmod(new_mode)
            child_dir_path.chmod(new_mode)
            tmp.chmod(new_mode)

            self.assertRaises(PermissionError, tmp._delete)
        finally:
            tmp.chmod(old_dir_mode)
            child_file_path.chmod(old_child_file_mode)
            child_dir_path.chmod(old_child_dir_mode)

    @needs_windows
    def test_delete_inner_junction(self):
        import _winapi
        tmp = self.cls(self.base, 'delete')
        tmp.mkdir()
        dir1 = tmp / 'dir1'
        dir2 = dir1 / 'dir2'
        dir3 = tmp / 'dir3'
        for d in dir1, dir2, dir3:
            d.mkdir()
        file1 = tmp / 'file1'
        file1.write_text('foo')
        link1 = dir1 / 'link1'
        _winapi.CreateJunction(str(dir2), str(link1))
        link2 = dir1 / 'link2'
        _winapi.CreateJunction(str(dir3), str(link2))
        link3 = dir1 / 'link3'
        _winapi.CreateJunction(str(file1), str(link3))
        # make sure junctions are removed but not followed
        dir1._delete()
        self.assertFalse(dir1.exists())
        self.assertTrue(dir3.exists())
        self.assertTrue(file1.exists())

    @needs_windows
    def test_delete_outer_junction(self):
        import _winapi
        tmp = self.cls(self.base, 'delete')
        tmp.mkdir()
        src = tmp / 'cheese'
        dst = tmp / 'shop'
        src.mkdir()
        spam = src / 'spam'
        spam.write_text('')
        _winapi.CreateJunction(str(src), str(dst))
        dst._delete()
        self.assertFalse(dst.exists())
        self.assertTrue(spam.exists())
        self.assertTrue(src.exists())

    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
    @unittest.skipIf(sys.platform == "vxworks",
                     "fifo requires special path on VxWorks")
    def test_delete_on_named_pipe(self):
        p = self.cls(self.base, 'pipe')
        os.mkfifo(p)
        p._delete()
        self.assertFalse(p.exists())

        p = self.cls(self.base, 'dir')
        p.mkdir()
        os.mkfifo(p / 'mypipe')
        p._delete()
        self.assertFalse(p.exists())

    def test_delete_does_not_choke_on_failing_lstat(self):
        try:
            orig_lstat = os.lstat
            tmp = self.cls(self.base, 'delete')

            def raiser(fn, *args, **kwargs):
                if fn != tmp:
                    raise OSError()
                else:
                    return orig_lstat(fn)

            os.lstat = raiser

            tmp.mkdir()
            foo = tmp / 'foo'
            foo.write_text('')
            tmp._delete()
        finally:
            os.lstat = orig_lstat

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

    @needs_symlinks
    def test_stat_no_follow_symlinks(self):
        p = self.cls(self.base) / 'linkA'
        st = p.stat()
        self.assertNotEqual(st, p.stat(follow_symlinks=False))

    @needs_symlinks
    def test_lstat(self):
        p = self.cls(self.base)/ 'linkA'
        st = p.stat()
        self.assertNotEqual(st, p.lstat())

    def test_lstat_nosymlink(self):
        p = self.cls(self.base) / 'fileA'
        st = p.stat()
        self.assertEqual(st, p.lstat())

    def test_is_junction_false(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_junction())
        self.assertFalse((P / 'dirA').is_junction())
        self.assertFalse((P / 'non-existing').is_junction())
        self.assertFalse((P / 'fileA' / 'bah').is_junction())
        self.assertFalse((P / 'fileA\udfff').is_junction())
        self.assertFalse((P / 'fileA\x00').is_junction())

    def test_is_junction_true(self):
        P = self.cls(self.base)

        with mock.patch.object(P.parser, 'isjunction'):
            self.assertEqual(P.is_junction(), P.parser.isjunction.return_value)
            P.parser.isjunction.assert_called_once_with(P)

    def test_is_fifo_false(self):
        P = self.cls(self.base)
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

    def test_is_socket_false(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_socket())
        self.assertFalse((P / 'dirA').is_socket())
        self.assertFalse((P / 'non-existing').is_socket())
        self.assertFalse((P / 'fileA' / 'bah').is_socket())
        self.assertIs((P / 'fileA\udfff').is_socket(), False)
        self.assertIs((P / 'fileA\x00').is_socket(), False)

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

    def test_is_mount(self):
        P = self.cls(self.base)
        self.assertFalse((P / 'fileA').is_mount())
        self.assertFalse((P / 'dirA').is_mount())
        self.assertFalse((P / 'non-existing').is_mount())
        self.assertFalse((P / 'fileA' / 'bah').is_mount())
        if self.can_symlink:
            self.assertFalse((P / 'linkA').is_mount())
        if os.name == 'nt':
            R = self.cls('c:\\')
        else:
            R = self.cls('/')
        self.assertTrue(R.is_mount())
        self.assertFalse((R / '\udfff').is_mount())

    def test_samefile(self):
        parser = self.parser
        fileA_path = parser.join(self.base, 'fileA')
        fileB_path = parser.join(self.base, 'dirB', 'fileB')
        p = self.cls(fileA_path)
        pp = self.cls(fileA_path)
        q = self.cls(fileB_path)
        self.assertTrue(p.samefile(fileA_path))
        self.assertTrue(p.samefile(pp))
        self.assertFalse(p.samefile(fileB_path))
        self.assertFalse(p.samefile(q))
        # Test the non-existent file case
        non_existent = parser.join(self.base, 'foo')
        r = self.cls(non_existent)
        self.assertRaises(FileNotFoundError, p.samefile, r)
        self.assertRaises(FileNotFoundError, p.samefile, non_existent)
        self.assertRaises(FileNotFoundError, r.samefile, p)
        self.assertRaises(FileNotFoundError, r.samefile, non_existent)
        self.assertRaises(FileNotFoundError, r.samefile, r)
        self.assertRaises(FileNotFoundError, r.samefile, non_existent)

    def test_passing_kwargs_errors(self):
        with self.assertRaises(TypeError):
            self.cls(foo="bar")

    @needs_symlinks
    def test_iterdir_symlink(self):
        # __iter__ on a symlink to a directory.
        P = self.cls
        p = P(self.base, 'linkB')
        paths = set(p.iterdir())
        expected = { P(self.base, 'linkB', q) for q in ['fileB', 'linkD'] }
        self.assertEqual(paths, expected)

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

    # See https://github.com/WebAssembly/wasi-filesystem/issues/26
    @unittest.skipIf(is_wasi, "WASI resolution of '..' parts doesn't match POSIX")
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
        if self.cls.parser is posixpath:
            self.assertEqual(set(p.glob("xyzzy/..")), set())
        else:
            # ".." segments are normalized first on Windows, so this path is stat()able.
            self.assertEqual(set(p.glob("xyzzy/..")), { P(self.base, "xyzzy", "..") })
        self.assertEqual(set(p.glob("/".join([".."] * 50))), { P(self.base, *[".."] * 50)})

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

    @needs_symlinks
    @unittest.skipIf(is_emscripten, "Hangs")
    def test_glob_recurse_symlinks_common(self):
        def _check(path, glob, expected):
            actual = {path for path in path.glob(glob, recurse_symlinks=True)
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
        _check(p, "dir*/**", [
            "dirA/", "dirA/linkC", "dirA/linkC/fileB", "dirA/linkC/linkD", "dirA/linkC/linkD/fileB",
            "dirB/", "dirB/fileB", "dirB/linkD", "dirB/linkD/fileB",
            "dirC/", "dirC/fileC", "dirC/dirD",  "dirC/dirD/fileD", "dirC/novel.txt",
            "dirE/"])
        _check(p, "dir*/**/", ["dirA/", "dirA/linkC/", "dirA/linkC/linkD/", "dirB/", "dirB/linkD/",
                               "dirC/", "dirC/dirD/", "dirE/"])
        _check(p, "dir*/**/..", ["dirA/..", "dirA/linkC/..", "dirB/..",
                                 "dirB/linkD/..", "dirA/linkC/linkD/..",
                                 "dirC/..", "dirC/dirD/..", "dirE/.."])
        _check(p, "dir*/*/**", [
            "dirA/linkC/", "dirA/linkC/linkD", "dirA/linkC/fileB", "dirA/linkC/linkD/fileB",
            "dirB/linkD/", "dirB/linkD/fileB",
            "dirC/dirD/", "dirC/dirD/fileD"])
        _check(p, "dir*/*/**/", ["dirA/linkC/", "dirA/linkC/linkD/", "dirB/linkD/", "dirC/dirD/"])
        _check(p, "dir*/*/**/..", ["dirA/linkC/..", "dirA/linkC/linkD/..",
                                   "dirB/linkD/..", "dirC/dirD/.."])
        _check(p, "dir*/**/fileC", ["dirC/fileC"])
        _check(p, "dir*/*/../dirD/**/", ["dirC/dirD/../dirD/"])
        _check(p, "*/dirD/**", ["dirC/dirD/", "dirC/dirD/fileD"])
        _check(p, "*/dirD/**/", ["dirC/dirD/"])

    @needs_symlinks
    @unittest.skipIf(is_emscripten, "Hangs")
    def test_rglob_recurse_symlinks_common(self):
        def _check(path, glob, expected):
            actual = {path for path in path.rglob(glob, recurse_symlinks=True)
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

    @needs_symlinks
    def test_rglob_symlink_loop(self):
        # Don't get fooled by symlink loops (Issue #26012).
        P = self.cls
        p = P(self.base)
        given = set(p.rglob('*', recurse_symlinks=False))
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

    @needs_symlinks
    def test_glob_permissions(self):
        # See bpo-38894
        P = self.cls
        base = P(self.base) / 'permissions'
        base.mkdir()

        for i in range(100):
            link = base / f"link{i}"
            if i % 2:
                link.symlink_to(P(self.base, "dirE", "nonexistent"))
            else:
                link.symlink_to(P(self.base, "dirC"), target_is_directory=True)

        self.assertEqual(len(set(base.glob("*"))), 100)
        self.assertEqual(len(set(base.glob("*/"))), 50)
        self.assertEqual(len(set(base.glob("*/fileC"))), 50)
        self.assertEqual(len(set(base.glob("*/file*"))), 50)

    @needs_symlinks
    def test_glob_long_symlink(self):
        # See gh-87695
        base = self.cls(self.base) / 'long_symlink'
        base.mkdir()
        bad_link = base / 'bad_link'
        bad_link.symlink_to("bad" * 200)
        self.assertEqual(sorted(base.glob('**/*')), [bad_link])

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


class PathWalkTest(test_pathlib_abc.DummyPathWalkTest):
    cls = pathlib.Path
    base = PathTest.base
    can_symlink = PathTest.can_symlink

    def setUp(self):
        name = self.id().split('.')[-1]
        if name in _tests_needing_symlinks and not self.can_symlink:
            self.skipTest('requires symlinks')
        super().setUp()
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

    def tearDown(self):
        if 'SUB21' in self.sub2_tree[1]:
            os.chmod(self.sub2_path / "SUB21", stat.S_IRWXU)
        super().tearDown()

    def test_walk_bad_dir(self):
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

    @needs_symlinks
    def test_walk_follow_symlinks(self):
        walk_it = self.walk_path.walk(follow_symlinks=True)
        for root, dirs, files in walk_it:
            if root == self.link_path:
                self.assertEqual(dirs, [])
                self.assertEqual(files, ["tmp4"])
                break
        else:
            self.fail("Didn't follow symlink with follow_symlinks=True")

    @needs_symlinks
    def test_walk_symlink_location(self):
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
