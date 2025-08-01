"""
Tests for pathlib.types._ReadablePath
"""

import collections.abc
import io
import sys
import unittest

from .support import is_pypi
from .support.local_path import ReadableLocalPath, LocalPathGround
from .support.zip_path import ReadableZipPath, ZipPathGround

if is_pypi:
    from pathlib_abc import PathInfo, _ReadablePath
    from pathlib_abc._os import magic_open
else:
    from pathlib.types import PathInfo, _ReadablePath
    from pathlib._os import magic_open


class ReadTestBase:
    def setUp(self):
        self.root = self.ground.setup()
        self.ground.create_hierarchy(self.root)

    def tearDown(self):
        self.ground.teardown(self.root)

    def test_is_readable(self):
        self.assertIsInstance(self.root, _ReadablePath)

    def test_open_r(self):
        p = self.root / 'fileA'
        with magic_open(p, 'r', encoding='utf-8') as f:
            self.assertIsInstance(f, io.TextIOBase)
            self.assertEqual(f.read(), 'this is file A\n')

    @unittest.skipIf(
        not getattr(sys.flags, 'warn_default_encoding', 0),
        "Requires warn_default_encoding",
    )
    def test_open_r_encoding_warning(self):
        p = self.root / 'fileA'
        with self.assertWarns(EncodingWarning) as wc:
            with magic_open(p, 'r'):
                pass
        self.assertEqual(wc.filename, __file__)

    def test_open_rb(self):
        p = self.root / 'fileA'
        with magic_open(p, 'rb') as f:
            self.assertEqual(f.read(), b'this is file A\n')
        self.assertRaises(ValueError, magic_open, p, 'rb', encoding='utf8')
        self.assertRaises(ValueError, magic_open, p, 'rb', errors='strict')
        self.assertRaises(ValueError, magic_open, p, 'rb', newline='')

    def test_read_bytes(self):
        p = self.root / 'fileA'
        self.assertEqual(p.read_bytes(), b'this is file A\n')

    def test_read_text(self):
        p = self.root / 'fileA'
        self.assertEqual(p.read_text(encoding='utf-8'), 'this is file A\n')
        q = self.root / 'abc'
        self.ground.create_file(q, b'\xe4bcdefg')
        self.assertEqual(q.read_text(encoding='latin-1'), 'äbcdefg')
        self.assertEqual(q.read_text(encoding='utf-8', errors='ignore'), 'bcdefg')

    @unittest.skipIf(
        not getattr(sys.flags, 'warn_default_encoding', 0),
        "Requires warn_default_encoding",
    )
    def test_read_text_encoding_warning(self):
        p = self.root / 'fileA'
        with self.assertWarns(EncodingWarning) as wc:
            p.read_text()
        self.assertEqual(wc.filename, __file__)

    def test_read_text_with_newlines(self):
        p = self.root / 'abc'
        self.ground.create_file(p, b'abcde\r\nfghlk\n\rmnopq')
        # Check that `\n` character change nothing
        self.assertEqual(p.read_text(encoding='utf-8', newline='\n'), 'abcde\r\nfghlk\n\rmnopq')
        # Check that `\r` character replaces `\n`
        self.assertEqual(p.read_text(encoding='utf-8', newline='\r'), 'abcde\r\nfghlk\n\rmnopq')
        # Check that `\r\n` character replaces `\n`
        self.assertEqual(p.read_text(encoding='utf-8', newline='\r\n'), 'abcde\r\nfghlk\n\rmnopq')

    def test_iterdir(self):
        expected = ['dirA', 'dirB', 'dirC', 'fileA']
        if self.ground.can_symlink:
            expected += ['linkA', 'linkB', 'brokenLink', 'brokenLinkLoop']
        expected = {self.root.joinpath(name) for name in expected}
        actual = set(self.root.iterdir())
        self.assertEqual(actual, expected)

    def test_iterdir_nodir(self):
        p = self.root / 'fileA'
        self.assertRaises(OSError, p.iterdir)

    def test_iterdir_info(self):
        for child in self.root.iterdir():
            self.assertIsInstance(child.info, PathInfo)
            self.assertTrue(child.info.exists(follow_symlinks=False))

    def test_glob(self):
        if not self.ground.can_symlink:
            self.skipTest("requires symlinks")

        p = self.root
        sep = self.root.parser.sep
        altsep = self.root.parser.altsep
        def check(pattern, expected):
            if altsep:
                expected = {name.replace(altsep, sep) for name in expected}
            expected = {p.joinpath(name) for name in expected}
            actual = set(p.glob(pattern, recurse_symlinks=True))
            self.assertEqual(actual, expected)

        it = p.glob("fileA")
        self.assertIsInstance(it, collections.abc.Iterator)
        self.assertEqual(list(it), [p.joinpath("fileA")])
        check("*A", ["dirA", "fileA", "linkA"])
        check("*A", ['dirA', 'fileA', 'linkA'])
        check("*B/*", ["dirB/fileB", "linkB/fileB"])
        check("*B/*", ['dirB/fileB', 'linkB/fileB'])
        check("brokenLink", ['brokenLink'])
        check("brokenLinkLoop", ['brokenLinkLoop'])
        check("**/", ["", "dirA/", "dirA/linkC/", "dirB/", "dirC/", "dirC/dirD/", "linkB/"])
        check("**/*/", ["dirA/", "dirA/linkC/", "dirB/", "dirC/", "dirC/dirD/", "linkB/"])
        check("*/", ["dirA/", "dirB/", "dirC/", "linkB/"])
        check("*/dirD/**/", ["dirC/dirD/"])
        check("*/dirD/**", ["dirC/dirD/", "dirC/dirD/fileD"])
        check("dir*/**", ["dirA/", "dirA/linkC", "dirA/linkC/fileB", "dirB/", "dirB/fileB", "dirC/",
                          "dirC/fileC", "dirC/dirD", "dirC/dirD/fileD", "dirC/novel.txt"])
        check("dir*/**/", ["dirA/", "dirA/linkC/", "dirB/", "dirC/", "dirC/dirD/"])
        check("dir*/**/..", ["dirA/..", "dirA/linkC/..", "dirB/..", "dirC/..", "dirC/dirD/.."])
        check("dir*/*/**", ["dirA/linkC/", "dirA/linkC/fileB", "dirC/dirD/", "dirC/dirD/fileD"])
        check("dir*/*/**/", ["dirA/linkC/", "dirC/dirD/"])
        check("dir*/*/**/..", ["dirA/linkC/..", "dirC/dirD/.."])
        check("dir*/*/..", ["dirC/dirD/..", "dirA/linkC/.."])
        check("dir*/*/../dirD/**/", ["dirC/dirD/../dirD/"])
        check("dir*/**/fileC", ["dirC/fileC"])
        check("dir*/file*", ["dirB/fileB", "dirC/fileC"])
        check("**/*/fileA", [])
        check("fileB", [])
        check("**/*/fileB", ["dirB/fileB", "dirA/linkC/fileB", "linkB/fileB"])
        check("**/fileB", ["dirB/fileB", "dirA/linkC/fileB", "linkB/fileB"])
        check("*/fileB", ["dirB/fileB", "linkB/fileB"])
        check("*/fileB", ['dirB/fileB', 'linkB/fileB'])
        check("**/file*",
              ["fileA", "dirA/linkC/fileB", "dirB/fileB", "dirC/fileC", "dirC/dirD/fileD",
               "linkB/fileB"])
        with self.assertRaisesRegex(ValueError, 'Unacceptable pattern'):
            list(p.glob(''))

    def test_walk_top_down(self):
        it = self.root.walk()

        path, dirnames, filenames = next(it)
        dirnames.sort()
        filenames.sort()
        self.assertEqual(path, self.root)
        self.assertEqual(dirnames, ['dirA', 'dirB', 'dirC'])
        self.assertEqual(filenames, ['brokenLink', 'brokenLinkLoop', 'fileA', 'linkA', 'linkB']
                                    if self.ground.can_symlink else ['fileA'])

        path, dirnames, filenames = next(it)
        self.assertEqual(path, self.root / 'dirA')
        self.assertEqual(dirnames, [])
        self.assertEqual(filenames, ['linkC'] if self.ground.can_symlink else [])

        path, dirnames, filenames = next(it)
        self.assertEqual(path, self.root / 'dirB')
        self.assertEqual(dirnames, [])
        self.assertEqual(filenames, ['fileB'])

        path, dirnames, filenames = next(it)
        filenames.sort()
        self.assertEqual(path, self.root / 'dirC')
        self.assertEqual(dirnames, ['dirD'])
        self.assertEqual(filenames, ['fileC', 'novel.txt'])

        path, dirnames, filenames = next(it)
        self.assertEqual(path, self.root / 'dirC' / 'dirD')
        self.assertEqual(dirnames, [])
        self.assertEqual(filenames, ['fileD'])

        self.assertRaises(StopIteration, next, it)

    def test_walk_prune(self):
        expected = {self.root, self.root / 'dirA', self.root / 'dirC', self.root / 'dirC' / 'dirD'}
        actual = set()
        for path, dirnames, filenames in self.root.walk():
            actual.add(path)
            if path == self.root:
                dirnames.remove('dirB')
        self.assertEqual(actual, expected)

    def test_walk_bottom_up(self):
        seen_root = seen_dira = seen_dirb = seen_dirc = seen_dird = False
        for path, dirnames, filenames in self.root.walk(top_down=False):
            if path == self.root:
                self.assertFalse(seen_root)
                self.assertTrue(seen_dira)
                self.assertTrue(seen_dirb)
                self.assertTrue(seen_dirc)
                self.assertEqual(sorted(dirnames), ['dirA', 'dirB', 'dirC'])
                self.assertEqual(sorted(filenames),
                                 ['brokenLink', 'brokenLinkLoop', 'fileA', 'linkA', 'linkB']
                                 if self.ground.can_symlink else ['fileA'])
                seen_root = True
            elif path == self.root / 'dirA':
                self.assertFalse(seen_root)
                self.assertFalse(seen_dira)
                self.assertEqual(dirnames, [])
                self.assertEqual(filenames, ['linkC'] if self.ground.can_symlink else [])
                seen_dira = True
            elif path == self.root / 'dirB':
                self.assertFalse(seen_root)
                self.assertFalse(seen_dirb)
                self.assertEqual(dirnames, [])
                self.assertEqual(filenames, ['fileB'])
                seen_dirb = True
            elif path == self.root / 'dirC':
                self.assertFalse(seen_root)
                self.assertFalse(seen_dirc)
                self.assertTrue(seen_dird)
                self.assertEqual(dirnames, ['dirD'])
                self.assertEqual(sorted(filenames), ['fileC', 'novel.txt'])
                seen_dirc = True
            elif path == self.root / 'dirC' / 'dirD':
                self.assertFalse(seen_root)
                self.assertFalse(seen_dirc)
                self.assertFalse(seen_dird)
                self.assertEqual(dirnames, [])
                self.assertEqual(filenames, ['fileD'])
                seen_dird = True
            else:
                raise AssertionError(f"Unexpected path: {path}")
        self.assertTrue(seen_root)

    def test_info_exists(self):
        p = self.root
        self.assertTrue(p.info.exists())
        self.assertTrue((p / 'dirA').info.exists())
        self.assertTrue((p / 'dirA').info.exists(follow_symlinks=False))
        self.assertTrue((p / 'fileA').info.exists())
        self.assertTrue((p / 'fileA').info.exists(follow_symlinks=False))
        self.assertFalse((p / 'non-existing').info.exists())
        self.assertFalse((p / 'non-existing').info.exists(follow_symlinks=False))
        if self.ground.can_symlink:
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

    def test_info_is_dir(self):
        p = self.root
        self.assertTrue((p / 'dirA').info.is_dir())
        self.assertTrue((p / 'dirA').info.is_dir(follow_symlinks=False))
        self.assertFalse((p / 'fileA').info.is_dir())
        self.assertFalse((p / 'fileA').info.is_dir(follow_symlinks=False))
        self.assertFalse((p / 'non-existing').info.is_dir())
        self.assertFalse((p / 'non-existing').info.is_dir(follow_symlinks=False))
        if self.ground.can_symlink:
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

    def test_info_is_file(self):
        p = self.root
        self.assertTrue((p / 'fileA').info.is_file())
        self.assertTrue((p / 'fileA').info.is_file(follow_symlinks=False))
        self.assertFalse((p / 'dirA').info.is_file())
        self.assertFalse((p / 'dirA').info.is_file(follow_symlinks=False))
        self.assertFalse((p / 'non-existing').info.is_file())
        self.assertFalse((p / 'non-existing').info.is_file(follow_symlinks=False))
        if self.ground.can_symlink:
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

    def test_info_is_symlink(self):
        p = self.root
        self.assertFalse((p / 'fileA').info.is_symlink())
        self.assertFalse((p / 'dirA').info.is_symlink())
        self.assertFalse((p / 'non-existing').info.is_symlink())
        if self.ground.can_symlink:
            self.assertTrue((p / 'linkA').info.is_symlink())
            self.assertTrue((p / 'linkB').info.is_symlink())
            self.assertTrue((p / 'brokenLink').info.is_symlink())
            self.assertFalse((p / 'linkA\udfff').info.is_symlink())
            self.assertFalse((p / 'linkA\x00').info.is_symlink())
            self.assertTrue((p / 'brokenLinkLoop').info.is_symlink())
        self.assertFalse((p / 'fileA\udfff').info.is_symlink())
        self.assertFalse((p / 'fileA\x00').info.is_symlink())


class ZipPathReadTest(ReadTestBase, unittest.TestCase):
    ground = ZipPathGround(ReadableZipPath)


class LocalPathReadTest(ReadTestBase, unittest.TestCase):
    ground = LocalPathGround(ReadableLocalPath)


if not is_pypi:
    from pathlib import Path

    class PathReadTest(ReadTestBase, unittest.TestCase):
        ground = LocalPathGround(Path)


if __name__ == "__main__":
    unittest.main()
