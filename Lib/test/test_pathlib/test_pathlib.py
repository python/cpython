import os
import ntpath
import pathlib
import posixpath
import unittest

from test.test_pathlib import test_pathlib_raw
from test.test_pathlib.test_pathlib_abc import needs_posix, needs_windows


#
# Tests for the pure classes.
#

class PurePathTest(test_pathlib_raw.RawPurePathTest):
    cls = pathlib.PurePath

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

    def test_concrete_pathmod(self):
        if self.cls is pathlib.PurePosixPath:
            expected = posixpath
        elif self.cls is pathlib.PureWindowsPath:
            expected = ntpath
        else:
            expected = os.path
        p = self.cls('a')
        self.assertIs(p.pathmod, expected)

    def test_different_pathmods_unequal(self):
        p = self.cls('a')
        if p.pathmod is posixpath:
            q = pathlib.PureWindowsPath('a')
        else:
            q = pathlib.PurePosixPath('a')
        self.assertNotEqual(p, q)

    def test_different_pathmods_unordered(self):
        p = self.cls('a')
        if p.pathmod is posixpath:
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

    def _check_parse_path(self, raw_path, *expected):
        sep = self.pathmod.sep
        actual = self.cls._parse_path(raw_path.replace('/', sep))
        self.assertEqual(actual, expected)
        if altsep := self.pathmod.altsep:
            actual = self.cls._parse_path(raw_path.replace('/', altsep))
            self.assertEqual(actual, expected)

    def test_parse_path_common(self):
        check = self._check_parse_path
        sep = self.pathmod.sep
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

    def test_bytes_constructor(self):
        P = self.cls
        message = (r"argument should be a str or an os\.PathLike object "
                   r"where __fspath__ returns a str, not 'bytes'")
        with self.assertRaisesRegex(TypeError, message):
            P(b'a')
        with self.assertRaisesRegex(TypeError, message):
            P(b'a', 'b')
        with self.assertRaisesRegex(TypeError, message):
            P('a', b'b')

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

    def test_with_suffix_empty(self):
        # Path doesn't have a "filename" component.
        P = self.cls
        self.assertRaises(ValueError, P('').with_suffix, '.gz')
        self.assertRaises(ValueError, P('.').with_suffix, '.gz')
        self.assertRaises(ValueError, P('/').with_suffix, '.gz')

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
    def test_root_normalize_posix(self):
        P = self.cls
        self.assertEqual(P('///a/b').root, '/')

    @needs_posix
    def test_parse_windows_path(self):
        P = self.cls
        p = P('c:', 'a', 'b')
        pp = P(pathlib.PureWindowsPath('c:\\a\\b'))
        self.assertEqual(p, pp)

    equivalences_windows = {
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
        self.test_equivalences(self.equivalences_windows)

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

class PathTest(PurePathTest, test_pathlib_raw.RawPathTest):
    """Tests for the FS-accessing functionalities of the Path classes."""
    cls = pathlib.Path

    def test_matches_pathbase_api(self):
        our_names = {name for name in dir(self.cls) if name[0] != '_'}
        our_names.remove('is_reserved')  # only present in PurePath
        path_names = {name for name in dir(pathlib._abc.PathBase) if name[0] != '_'}
        self.assertEqual(our_names, path_names)
        for attr_name in our_names:
            if attr_name == 'pathmod':
                # On Windows, Path.pathmod is ntpath, but PathBase.pathmod is
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

    def test_unsupported_pathmod(self):
        if self.cls.pathmod is os.path:
            self.skipTest("path flavour is supported")
        else:
            self.assertRaises(pathlib.UnsupportedOperation, self.cls)

    def test_glob_empty_pattern(self):
        p = self.cls('')
        with self.assertRaisesRegex(ValueError, 'Unacceptable pattern'):
            list(p.glob(''))
        with self.assertRaisesRegex(ValueError, 'Unacceptable pattern'):
            list(p.glob('.'))


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
