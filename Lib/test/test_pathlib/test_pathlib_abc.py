import collections
import io
import os
import errno
import unittest

from pathlib._os import magic_open
from pathlib.types import _PathParser, PathInfo, _JoinablePath, _ReadablePath, _WritablePath
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


class DummyJoinablePath(_JoinablePath):
    __slots__ = ('_segments',)

    parser = posixpath

    def __init__(self, *segments):
        self._segments = segments

    def __str__(self):
        if self._segments:
            return self.parser.join(*self._segments)
        return ''

    def __eq__(self, other):
        if not isinstance(other, DummyJoinablePath):
            return NotImplemented
        return str(self) == str(other)

    def __hash__(self):
        return hash(str(self))

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, str(self))

    def with_segments(self, *pathsegments):
        return type(self)(*pathsegments)


class JoinablePathTest(unittest.TestCase):
    cls = DummyJoinablePath

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

#
# Tests for the virtual classes.
#


class DummyWritablePathIO(io.BytesIO):
    """
    Used by DummyWritablePath to implement `__open_wb__()`
    """

    def __init__(self, files, path):
        super().__init__()
        self.files = files
        self.path = path

    def close(self):
        self.files[self.path] = self.getvalue()
        super().close()


class DummyReadablePathInfo:
    __slots__ = ('_is_dir', '_is_file')

    def __init__(self, is_dir, is_file):
        self._is_dir = is_dir
        self._is_file = is_file

    def exists(self, *, follow_symlinks=True):
        return self._is_dir or self._is_file

    def is_dir(self, *, follow_symlinks=True):
        return self._is_dir

    def is_file(self, *, follow_symlinks=True):
        return self._is_file

    def is_symlink(self):
        return False


class DummyReadablePath(_ReadablePath, DummyJoinablePath):
    """
    Simple implementation of DummyReadablePath that keeps files and
    directories in memory.
    """
    __slots__ = ('_info')

    _files = {}
    _directories = {}
    parser = posixpath

    def __init__(self, *segments):
        super().__init__(*segments)
        self._info = None

    @property
    def info(self):
        if self._info is None:
            path_str = str(self)
            self._info = DummyReadablePathInfo(
                is_dir=path_str.rstrip('/') in self._directories,
                is_file=path_str in self._files)
        return self._info

    def __open_rb__(self, buffering=-1):
        path = str(self)
        if path in self._directories:
            raise IsADirectoryError(errno.EISDIR, "Is a directory", path)
        elif path not in self._files:
            raise FileNotFoundError(errno.ENOENT, "File not found", path)
        return io.BytesIO(self._files[path])

    def iterdir(self):
        path = str(self).rstrip('/')
        if path in self._files:
            raise NotADirectoryError(errno.ENOTDIR, "Not a directory", path)
        elif path in self._directories:
            return iter([self / name for name in self._directories[path]])
        else:
            raise FileNotFoundError(errno.ENOENT, "File not found", path)

    def readlink(self):
        raise NotImplementedError


class DummyWritablePath(_WritablePath, DummyJoinablePath):
    __slots__ = ()

    def __open_wb__(self, buffering=-1):
        path = str(self)
        if path in self._directories:
            raise IsADirectoryError(errno.EISDIR, "Is a directory", path)
        parent, name = posixpath.split(path)
        if parent not in self._directories:
            raise FileNotFoundError(errno.ENOENT, "File not found", parent)
        self._files[path] = b''
        self._directories[parent].add(name)
        return DummyWritablePathIO(self._files, path)

    def mkdir(self):
        path = str(self)
        parent = str(self.parent)
        if path in self._directories:
            raise FileExistsError(errno.EEXIST, "File exists", path)
        try:
            if self.name:
                self._directories[parent].add(self.name)
            self._directories[path] = set()
        except KeyError:
            raise FileNotFoundError(errno.ENOENT, "File not found", parent) from None

    def symlink_to(self, target, target_is_directory=False):
        raise NotImplementedError


class ReadablePathTest(JoinablePathTest):
    """Tests for ReadablePathTest methods that use stat(), open() and iterdir()."""

    cls = DummyReadablePath
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
        self.createTestHierarchy()

    def createTestHierarchy(self):
        cls = self.cls
        cls._files = {
            f'{self.base}/fileA': b'this is file A\n',
            f'{self.base}/dirB/fileB': b'this is file B\n',
            f'{self.base}/dirC/fileC': b'this is file C\n',
            f'{self.base}/dirC/dirD/fileD': b'this is file D\n',
            f'{self.base}/dirC/novel.txt': b'this is a novel\n',
        }
        cls._directories = {
            f'{self.base}': {'fileA', 'dirA', 'dirB', 'dirC', 'dirE'},
            f'{self.base}/dirA': set(),
            f'{self.base}/dirB': {'fileB'},
            f'{self.base}/dirC': {'fileC', 'dirD', 'novel.txt'},
            f'{self.base}/dirC/dirD': {'fileD'},
            f'{self.base}/dirE': set(),
        }

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

    def test_is_readable(self):
        p = self.cls(self.base)
        self.assertIsInstance(p, _ReadablePath)

    def test_magic_open(self):
        p = self.cls(self.base)
        with magic_open(p / 'fileA', 'r') as f:
            self.assertIsInstance(f, io.TextIOBase)
            self.assertEqual(f.read(), "this is file A\n")
        with magic_open(p / 'fileA', 'rb') as f:
            self.assertIsInstance(f, io.BufferedIOBase)
            self.assertEqual(f.read().strip(), b"this is file A")

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

    def test_iterdir_info(self):
        p = self.cls(self.base)
        for child in p.iterdir():
            self.assertIsInstance(child.info, PathInfo)
            self.assertTrue(child.info.exists(follow_symlinks=False))

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
        expect = {q} if q.info.exists() else set()
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
        self.assertEqual(list(p.glob("")), [p.joinpath("")])

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

    def test_info_exists(self):
        p = self.cls(self.base)
        self.assertTrue(p.info.exists())
        self.assertTrue((p / 'dirA').info.exists())
        self.assertTrue((p / 'dirA').info.exists(follow_symlinks=False))
        self.assertTrue((p / 'fileA').info.exists())
        self.assertTrue((p / 'fileA').info.exists(follow_symlinks=False))
        self.assertFalse((p / 'non-existing').info.exists())
        self.assertFalse((p / 'non-existing').info.exists(follow_symlinks=False))
        if self.can_symlink:
            self.assertTrue((p / 'linkA').info.exists())
            self.assertTrue((p / 'linkA').info.exists(follow_symlinks=False))
            self.assertTrue((p / 'linkB').info.exists())
            self.assertTrue((p / 'linkB').info.exists(follow_symlinks=True))
            self.assertFalse((p / 'brokenLink').info.exists())
            self.assertTrue((p / 'brokenLink').info.exists(follow_symlinks=False))
            self.assertFalse((p / 'brokenLinkLoop').info.exists())
            self.assertTrue((p / 'brokenLinkLoop').info.exists(follow_symlinks=False))
        self.assertFalse((p / 'fileA\udfff').info.exists())
        self.assertFalse((p / 'fileA\udfff').info.exists(follow_symlinks=False))
        self.assertFalse((p / 'fileA\x00').info.exists())
        self.assertFalse((p / 'fileA\x00').info.exists(follow_symlinks=False))

    def test_info_exists_caching(self):
        p = self.cls(self.base)
        q = p / 'myfile'
        self.assertFalse(q.info.exists())
        self.assertFalse(q.info.exists(follow_symlinks=False))
        if isinstance(self.cls, _WritablePath):
            q.write_text('hullo')
            self.assertFalse(q.info.exists())
            self.assertFalse(q.info.exists(follow_symlinks=False))

    def test_info_is_dir(self):
        p = self.cls(self.base)
        self.assertTrue((p / 'dirA').info.is_dir())
        self.assertTrue((p / 'dirA').info.is_dir(follow_symlinks=False))
        self.assertFalse((p / 'fileA').info.is_dir())
        self.assertFalse((p / 'fileA').info.is_dir(follow_symlinks=False))
        self.assertFalse((p / 'non-existing').info.is_dir())
        self.assertFalse((p / 'non-existing').info.is_dir(follow_symlinks=False))
        if self.can_symlink:
            self.assertFalse((p / 'linkA').info.is_dir())
            self.assertFalse((p / 'linkA').info.is_dir(follow_symlinks=False))
            self.assertTrue((p / 'linkB').info.is_dir())
            self.assertFalse((p / 'linkB').info.is_dir(follow_symlinks=False))
            self.assertFalse((p / 'brokenLink').info.is_dir())
            self.assertFalse((p / 'brokenLink').info.is_dir(follow_symlinks=False))
            self.assertFalse((p / 'brokenLinkLoop').info.is_dir())
            self.assertFalse((p / 'brokenLinkLoop').info.is_dir(follow_symlinks=False))
        self.assertFalse((p / 'dirA\udfff').info.is_dir())
        self.assertFalse((p / 'dirA\udfff').info.is_dir(follow_symlinks=False))
        self.assertFalse((p / 'dirA\x00').info.is_dir())
        self.assertFalse((p / 'dirA\x00').info.is_dir(follow_symlinks=False))

    def test_info_is_dir_caching(self):
        p = self.cls(self.base)
        q = p / 'mydir'
        self.assertFalse(q.info.is_dir())
        self.assertFalse(q.info.is_dir(follow_symlinks=False))
        if isinstance(self.cls, _WritablePath):
            q.mkdir()
            self.assertFalse(q.info.is_dir())
            self.assertFalse(q.info.is_dir(follow_symlinks=False))

    def test_info_is_file(self):
        p = self.cls(self.base)
        self.assertTrue((p / 'fileA').info.is_file())
        self.assertTrue((p / 'fileA').info.is_file(follow_symlinks=False))
        self.assertFalse((p / 'dirA').info.is_file())
        self.assertFalse((p / 'dirA').info.is_file(follow_symlinks=False))
        self.assertFalse((p / 'non-existing').info.is_file())
        self.assertFalse((p / 'non-existing').info.is_file(follow_symlinks=False))
        if self.can_symlink:
            self.assertTrue((p / 'linkA').info.is_file())
            self.assertFalse((p / 'linkA').info.is_file(follow_symlinks=False))
            self.assertFalse((p / 'linkB').info.is_file())
            self.assertFalse((p / 'linkB').info.is_file(follow_symlinks=False))
            self.assertFalse((p / 'brokenLink').info.is_file())
            self.assertFalse((p / 'brokenLink').info.is_file(follow_symlinks=False))
            self.assertFalse((p / 'brokenLinkLoop').info.is_file())
            self.assertFalse((p / 'brokenLinkLoop').info.is_file(follow_symlinks=False))
        self.assertFalse((p / 'fileA\udfff').info.is_file())
        self.assertFalse((p / 'fileA\udfff').info.is_file(follow_symlinks=False))
        self.assertFalse((p / 'fileA\x00').info.is_file())
        self.assertFalse((p / 'fileA\x00').info.is_file(follow_symlinks=False))

    def test_info_is_file_caching(self):
        p = self.cls(self.base)
        q = p / 'myfile'
        self.assertFalse(q.info.is_file())
        self.assertFalse(q.info.is_file(follow_symlinks=False))
        if isinstance(self.cls, _WritablePath):
            q.write_text('hullo')
            self.assertFalse(q.info.is_file())
            self.assertFalse(q.info.is_file(follow_symlinks=False))

    def test_info_is_symlink(self):
        p = self.cls(self.base)
        self.assertFalse((p / 'fileA').info.is_symlink())
        self.assertFalse((p / 'dirA').info.is_symlink())
        self.assertFalse((p / 'non-existing').info.is_symlink())
        if self.can_symlink:
            self.assertTrue((p / 'linkA').info.is_symlink())
            self.assertTrue((p / 'linkB').info.is_symlink())
            self.assertTrue((p / 'brokenLink').info.is_symlink())
            self.assertFalse((p / 'linkA\udfff').info.is_symlink())
            self.assertFalse((p / 'linkA\x00').info.is_symlink())
            self.assertTrue((p / 'brokenLinkLoop').info.is_symlink())
        self.assertFalse((p / 'fileA\udfff').info.is_symlink())
        self.assertFalse((p / 'fileA\x00').info.is_symlink())


class WritablePathTest(JoinablePathTest):
    cls = DummyWritablePath

    def test_is_writable(self):
        p = self.cls(self.base)
        self.assertIsInstance(p, _WritablePath)


class DummyRWPath(DummyWritablePath, DummyReadablePath):
    __slots__ = ()


class RWPathTest(WritablePathTest, ReadablePathTest):
    cls = DummyRWPath
    can_symlink = False

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
        self.assertTrue(result.info.exists())
        self.assertEqual(source.read_text(), result.read_text())

    def test_copy_file_to_existing_file(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        target = base / 'dirB' / 'fileB'
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(result.info.exists())
        self.assertEqual(source.read_text(), result.read_text())

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
        self.assertTrue(result.info.exists())
        self.assertEqual(result.read_bytes(), b'')

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
        self.assertTrue(result.info.is_dir())
        self.assertTrue(result.joinpath('dirD').info.is_dir())
        self.assertTrue(result.joinpath('dirD', 'fileD').info.is_file())
        self.assertEqual(result.joinpath('dirD', 'fileD').read_text(),
                         "this is file D\n")
        self.assertTrue(result.joinpath('fileC').info.is_file())
        self.assertTrue(result.joinpath('fileC').read_text(),
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
        target_walk = ordered_walk(result)
        for source_item, target_item in zip(source_walk, target_walk, strict=True):
            self.assertEqual(source_item[0].parts[len(source.parts):],
                             target_item[0].parts[len(target.parts):])  # dirpath
            self.assertEqual(source_item[1], target_item[1])  # dirnames
            self.assertEqual(source_item[2], target_item[2])  # filenames
            # Compare files and symlinks
            for filename in source_item[2]:
                source_file = source_item[0].joinpath(filename)
                target_file = target_item[0].joinpath(filename)
                if follow_symlinks or not source_file.info.is_symlink():
                    # Regular file.
                    self.assertEqual(source_file.read_bytes(), target_file.read_bytes())
                elif source_file.info.is_dir():
                    # Symlink to directory.
                    self.assertTrue(target_file.info.is_dir())
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
        self.assertFalse(target.info.exists())

    def test_copy_into(self):
        base = self.cls(self.base)
        source = base / 'fileA'
        target_dir = base / 'dirA'
        result = source.copy_into(target_dir)
        self.assertEqual(result, target_dir / 'fileA')
        self.assertTrue(result.info.exists())
        self.assertEqual(source.read_text(), result.read_text())

    def test_copy_into_empty_name(self):
        source = self.cls('')
        target_dir = self.base
        self.assertRaises(ValueError, source.copy_into, target_dir)


class ReadablePathWalkTest(unittest.TestCase):
    cls = DummyReadablePath
    base = ReadablePathTest.base
    can_symlink = False

    def setUp(self):
        self.walk_path = self.cls(self.base, "TEST1")
        self.sub1_path = self.walk_path / "SUB1"
        self.sub11_path = self.sub1_path / "SUB11"
        self.sub2_path = self.walk_path / "SUB2"
        self.link_path = self.sub2_path / "link"
        self.sub2_tree = (self.sub2_path, [], ["tmp3"])
        self.createTestHierarchy()

    def createTestHierarchy(self):
        cls = self.cls
        cls._files = {
            f'{self.base}/TEST1/tmp1': b'this is tmp1\n',
            f'{self.base}/TEST1/SUB1/tmp2': b'this is tmp2\n',
            f'{self.base}/TEST1/SUB2/tmp3': b'this is tmp3\n',
            f'{self.base}/TEST2/tmp4': b'this is tmp4\n',
        }
        cls._directories = {
            f'{self.base}': {'TEST1', 'TEST2'},
            f'{self.base}/TEST1': {'SUB1', 'SUB2', 'tmp1'},
            f'{self.base}/TEST1/SUB1': {'SUB11', 'tmp2'},
            f'{self.base}/TEST1/SUB1/SUB11': set(),
            f'{self.base}/TEST1/SUB2': {'tmp3'},
            f'{self.base}/TEST2': {'tmp4'},
        }

    def tearDown(self):
        cls = self.cls
        cls._files.clear()
        cls._directories.clear()

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
