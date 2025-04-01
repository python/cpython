"""
Tests for pathlib.types._WritablePath
"""

import io
import os
import unittest

from .support import is_pypi
from .support.local_path import WritableLocalPath, LocalPathGround
from .support.zip_path import WritableZipPath, ZipPathGround

if is_pypi:
    from pathlib_abc import _WritablePath
    from pathlib_abc._os import magic_open
else:
    from pathlib.types import _WritablePath
    from pathlib._os import magic_open


class WriteTestBase:
    def setUp(self):
        self.root = self.ground.setup()

    def tearDown(self):
        self.ground.teardown(self.root)

    def test_is_writable(self):
        self.assertIsInstance(self.root, _WritablePath)

    def test_open_w(self):
        p = self.root / 'fileA'
        with magic_open(p, 'w') as f:
            self.assertIsInstance(f, io.TextIOBase)
            f.write('this is file A\n')
        self.assertEqual(self.ground.readtext(p), 'this is file A\n')

    def test_open_wb(self):
        p = self.root / 'fileA'
        with magic_open(p, 'wb') as f:
            #self.assertIsInstance(f, io.BufferedWriter)
            f.write(b'this is file A\n')
        self.assertEqual(self.ground.readbytes(p), b'this is file A\n')
        self.assertRaises(ValueError, magic_open, p, 'wb', encoding='utf8')
        self.assertRaises(ValueError, magic_open, p, 'wb', errors='strict')
        self.assertRaises(ValueError, magic_open, p, 'wb', newline='')

    def test_write_bytes(self):
        p = self.root / 'fileA'
        p.write_bytes(b'abcdefg')
        self.assertEqual(self.ground.readbytes(p), b'abcdefg')
        # Check that trying to write str does not truncate the file.
        self.assertRaises(TypeError, p.write_bytes, 'somestr')
        self.assertEqual(self.ground.readbytes(p), b'abcdefg')

    def test_write_text(self):
        p = self.root / 'fileA'
        p.write_text('äbcdefg', encoding='latin-1')
        self.assertEqual(self.ground.readbytes(p), b'\xe4bcdefg')
        # Check that trying to write bytes does not truncate the file.
        self.assertRaises(TypeError, p.write_text, b'somebytes')
        self.assertEqual(self.ground.readbytes(p), b'\xe4bcdefg')

    def test_write_text_with_newlines(self):
        # Check that `\n` character change nothing
        p = self.root / 'fileA'
        p.write_text('abcde\r\nfghlk\n\rmnopq', newline='\n')
        self.assertEqual(self.ground.readbytes(p), b'abcde\r\nfghlk\n\rmnopq')

        # Check that `\r` character replaces `\n`
        p = self.root / 'fileB'
        p.write_text('abcde\r\nfghlk\n\rmnopq', newline='\r')
        self.assertEqual(self.ground.readbytes(p), b'abcde\r\rfghlk\r\rmnopq')

        # Check that `\r\n` character replaces `\n`
        p = self.root / 'fileC'
        p.write_text('abcde\r\nfghlk\n\rmnopq', newline='\r\n')
        self.assertEqual(self.ground.readbytes(p), b'abcde\r\r\nfghlk\r\n\rmnopq')

        # Check that no argument passed will change `\n` to `os.linesep`
        os_linesep_byte = bytes(os.linesep, encoding='ascii')
        p = self.root / 'fileD'
        p.write_text('abcde\nfghlk\n\rmnopq')
        self.assertEqual(self.ground.readbytes(p),
                         b'abcde' + os_linesep_byte +
                         b'fghlk' + os_linesep_byte + b'\rmnopq')

    def test_mkdir(self):
        p = self.root / 'newdirA'
        self.assertFalse(self.ground.isdir(p))
        p.mkdir()
        self.assertTrue(self.ground.isdir(p))

    def test_symlink_to(self):
        if not self.ground.can_symlink:
            self.skipTest('needs symlinks')
        link = self.root.joinpath('linkA')
        link.symlink_to('fileA')
        self.assertTrue(self.ground.islink(link))
        self.assertEqual(self.ground.readlink(link), 'fileA')


class ZipPathWriteTest(WriteTestBase, unittest.TestCase):
    ground = ZipPathGround(WritableZipPath)


class LocalPathWriteTest(WriteTestBase, unittest.TestCase):
    ground = LocalPathGround(WritableLocalPath)


if not is_pypi:
    from pathlib import Path

    class PathWriteTest(WriteTestBase, unittest.TestCase):
        ground = LocalPathGround(Path)


if __name__ == "__main__":
    unittest.main()
