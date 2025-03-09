import io
import unittest

from pathlib import Path
from pathlib.types import _ReadablePath
from pathlib._os import magic_open

from test.test_pathlib.support.local_path import ReadableLocalPath, LocalPathGround
from test.test_pathlib.support.zip_path import ReadableZipPath, ZipPathGround


class ReadablePathTestBase:
    def setUp(self):
        self.root = self.ground.setup()
        self.ground.create_hierarchy(self.root)

    def tearDown(self):
        self.ground.teardown(self.root)

    def test_is_readable(self):
        self.assertIsInstance(self.root, _ReadablePath)

    def test_open_r(self):
        p = self.root / 'fileA'
        with magic_open(p, 'r') as f:
            self.assertIsInstance(f, io.TextIOBase)
            self.assertEqual(f.read(), 'this is file A\n')

    def test_open_rb(self):
        p = self.root / 'fileA'
        with magic_open(p, 'rb') as f:
            self.assertEqual(f.read(), b'this is file A\n')

    def test_read_bytes(self):
        p = self.root / 'fileA'
        self.assertEqual(p.read_bytes(), b'this is file A\n')

    def test_read_text(self):
        p = self.root / 'fileA'
        self.assertEqual(p.read_text(), 'this is file A\n')
        q = self.root / 'abc'
        self.ground.create_file(q, b'\xe4bcdefg')
        self.assertEqual(q.read_text(encoding='latin-1'), 'äbcdefg')
        self.assertEqual(q.read_text(encoding='utf-8', errors='ignore'), 'bcdefg')

    def test_read_text_with_newlines(self):
        p = self.root / 'abc'
        self.ground.create_file(p, b'abcde\r\nfghlk\n\rmnopq')
        # Check that `\n` character change nothing
        self.assertEqual(p.read_text(newline='\n'), 'abcde\r\nfghlk\n\rmnopq')
        # Check that `\r` character replaces `\n`
        self.assertEqual(p.read_text(newline='\r'), 'abcde\r\nfghlk\n\rmnopq')
        # Check that `\r\n` character replaces `\n`
        self.assertEqual(p.read_text(newline='\r\n'), 'abcde\r\nfghlk\n\rmnopq')


class ZipPathTest(ReadablePathTestBase, unittest.TestCase):
    ground = ZipPathGround(ReadableZipPath)


class LocalPathTest(ReadablePathTestBase, unittest.TestCase):
    ground = LocalPathGround(ReadableLocalPath)


class PathTest(ReadablePathTestBase, unittest.TestCase):
    ground = LocalPathGround(Path)


if __name__ == "__main__":
    unittest.main()
