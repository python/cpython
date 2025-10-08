"""
Tests for pathlib.types._WritablePath
"""

import io
import os
import sys
import unittest

from .support import is_pypi
from .support.local_path import WritableLocalPath, LocalPathGround
from .support.zip_path import WritableZipPath, ZipPathGround

if is_pypi:
    from pathlib_abc import _WritablePath
    from pathlib_abc._os import vfsopen
else:
    from pathlib.types import _WritablePath
    from pathlib._os import vfsopen


class WriteTestBase:
    def setUp(self):
        self.root = self.ground.setup()

    def tearDown(self):
        self.ground.teardown(self.root)

    def test_is_writable(self):
        self.assertIsInstance(self.root, _WritablePath)

    def test_open_w(self):
        p = self.root / 'fileA'
        with vfsopen(p, 'w', encoding='utf-8') as f:
            self.assertIsInstance(f, io.TextIOBase)
            f.write('this is file A\n')
        self.assertEqual(self.ground.readtext(p), 'this is file A\n')

    def test_open_w_buffering_error(self):
        p = self.root / 'fileA'
        self.assertRaises(ValueError, vfsopen, p, 'w', buffering=0)
        self.assertRaises(ValueError, vfsopen, p, 'w', buffering=1)
        self.assertRaises(ValueError, vfsopen, p, 'w', buffering=1024)

    @unittest.skipIf(
        not getattr(sys.flags, 'warn_default_encoding', 0),
        "Requires warn_default_encoding",
    )
    def test_open_w_encoding_warning(self):
        p = self.root / 'fileA'
        with self.assertWarns(EncodingWarning) as wc:
            with vfsopen(p, 'w'):
                pass
        self.assertEqual(wc.filename, __file__)

    def test_open_wb(self):
        p = self.root / 'fileA'
        with vfsopen(p, 'wb') as f:
            #self.assertIsInstance(f, io.BufferedWriter)
            f.write(b'this is file A\n')
        self.assertEqual(self.ground.readbytes(p), b'this is file A\n')
        self.assertRaises(ValueError, vfsopen, p, 'wb', encoding='utf8')
        self.assertRaises(ValueError, vfsopen, p, 'wb', errors='strict')
        self.assertRaises(ValueError, vfsopen, p, 'wb', newline='')

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
        self.assertRaises(TypeError, p.write_text, b'somebytes', encoding='utf-8')
        self.assertEqual(self.ground.readbytes(p), b'\xe4bcdefg')

    @unittest.skipIf(
        not getattr(sys.flags, 'warn_default_encoding', 0),
        "Requires warn_default_encoding",
    )
    def test_write_text_encoding_warning(self):
        p = self.root / 'fileA'
        with self.assertWarns(EncodingWarning) as wc:
            p.write_text('abcdefg')
        self.assertEqual(wc.filename, __file__)

    def test_write_text_with_newlines(self):
        # Check that `\n` character change nothing
        p = self.root / 'fileA'
        p.write_text('abcde\r\nfghlk\n\rmnopq', encoding='utf-8', newline='\n')
        self.assertEqual(self.ground.readbytes(p), b'abcde\r\nfghlk\n\rmnopq')

        # Check that `\r` character replaces `\n`
        p = self.root / 'fileB'
        p.write_text('abcde\r\nfghlk\n\rmnopq', encoding='utf-8', newline='\r')
        self.assertEqual(self.ground.readbytes(p), b'abcde\r\rfghlk\r\rmnopq')

        # Check that `\r\n` character replaces `\n`
        p = self.root / 'fileC'
        p.write_text('abcde\r\nfghlk\n\rmnopq', encoding='utf-8', newline='\r\n')
        self.assertEqual(self.ground.readbytes(p), b'abcde\r\r\nfghlk\r\n\rmnopq')

        # Check that no argument passed will change `\n` to `os.linesep`
        os_linesep_byte = bytes(os.linesep, encoding='ascii')
        p = self.root / 'fileD'
        p.write_text('abcde\nfghlk\n\rmnopq', encoding='utf-8')
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
