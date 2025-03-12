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


if __name__ == "__main__":
    unittest.main()
