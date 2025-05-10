"""
Tests for copying from pathlib.types._ReadablePath to _WritablePath.
"""

import contextlib
import unittest

from .support import is_pypi
from .support.local_path import LocalPathGround
from .support.zip_path import ZipPathGround, ReadableZipPath, WritableZipPath


class CopyTestBase:
    def setUp(self):
        self.source_root = self.source_ground.setup()
        self.source_ground.create_hierarchy(self.source_root)
        self.target_root = self.target_ground.setup(local_suffix="_target")

    def tearDown(self):
        self.source_ground.teardown(self.source_root)
        self.target_ground.teardown(self.target_root)

    def test_copy_file(self):
        source = self.source_root / 'fileA'
        target = self.target_root / 'copyA'
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(self.target_ground.isfile(target))
        self.assertEqual(self.source_ground.readbytes(source),
                         self.target_ground.readbytes(result))

    def test_copy_file_empty(self):
        source = self.source_root / 'empty'
        target = self.target_root / 'copyA'
        self.source_ground.create_file(source, b'')
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(self.target_ground.isfile(target))
        self.assertEqual(self.target_ground.readbytes(result), b'')

    def test_copy_file_to_existing_file(self):
        source = self.source_root / 'fileA'
        target = self.target_root / 'copyA'
        self.target_ground.create_file(target, b'this is a copy\n')
        with contextlib.ExitStack() as stack:
            if isinstance(target, WritableZipPath):
                stack.enter_context(self.assertWarns(UserWarning))
            result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(self.target_ground.isfile(target))
        self.assertEqual(self.source_ground.readbytes(source),
                         self.target_ground.readbytes(result))

    def test_copy_file_to_directory(self):
        if isinstance(self.target_root, WritableZipPath):
            self.skipTest('needs local target')
        source = self.source_root / 'fileA'
        target = self.target_root / 'copyA'
        self.target_ground.create_dir(target)
        self.assertRaises(OSError, source.copy, target)

    def test_copy_file_to_itself(self):
        source = self.source_root / 'fileA'
        self.assertRaises(OSError, source.copy, source)
        self.assertRaises(OSError, source.copy, source, follow_symlinks=False)

    def test_copy_dir(self):
        source = self.source_root / 'dirC'
        target = self.target_root / 'copyC'
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(self.target_ground.isdir(target))
        self.assertTrue(self.target_ground.isfile(target / 'fileC'))
        self.assertEqual(self.target_ground.readtext(target / 'fileC'), 'this is file C\n')
        self.assertTrue(self.target_ground.isdir(target / 'dirD'))
        self.assertTrue(self.target_ground.isfile(target / 'dirD' / 'fileD'))
        self.assertEqual(self.target_ground.readtext(target / 'dirD' / 'fileD'), 'this is file D\n')

    def test_copy_dir_follow_symlinks_true(self):
        if not self.source_ground.can_symlink:
            self.skipTest('needs symlink support on source')
        source = self.source_root / 'dirC'
        target = self.target_root / 'copyC'
        self.source_ground.create_symlink(source / 'linkC', 'fileC')
        self.source_ground.create_symlink(source / 'linkD', 'dirD')
        result = source.copy(target)
        self.assertEqual(result, target)
        self.assertTrue(self.target_ground.isdir(target))
        self.assertFalse(self.target_ground.islink(target / 'linkC'))
        self.assertTrue(self.target_ground.isfile(target / 'linkC'))
        self.assertEqual(self.target_ground.readtext(target / 'linkC'), 'this is file C\n')
        self.assertFalse(self.target_ground.islink(target / 'linkD'))
        self.assertTrue(self.target_ground.isdir(target / 'linkD'))
        self.assertTrue(self.target_ground.isfile(target / 'linkD' / 'fileD'))
        self.assertEqual(self.target_ground.readtext(target / 'linkD' / 'fileD'), 'this is file D\n')

    def test_copy_dir_follow_symlinks_false(self):
        if not self.source_ground.can_symlink:
            self.skipTest('needs symlink support on source')
        if not self.target_ground.can_symlink:
            self.skipTest('needs symlink support on target')
        source = self.source_root / 'dirC'
        target = self.target_root / 'copyC'
        self.source_ground.create_symlink(source / 'linkC', 'fileC')
        self.source_ground.create_symlink(source / 'linkD', 'dirD')
        result = source.copy(target, follow_symlinks=False)
        self.assertEqual(result, target)
        self.assertTrue(self.target_ground.isdir(target))
        self.assertTrue(self.target_ground.islink(target / 'linkC'))
        self.assertEqual(self.target_ground.readlink(target / 'linkC'), 'fileC')
        self.assertTrue(self.target_ground.islink(target / 'linkD'))
        self.assertEqual(self.target_ground.readlink(target / 'linkD'), 'dirD')

    def test_copy_dir_to_existing_directory(self):
        if isinstance(self.target_root, WritableZipPath):
            self.skipTest('needs local target')
        source = self.source_root / 'dirC'
        target = self.target_root / 'copyC'
        self.target_ground.create_dir(target)
        self.assertRaises(FileExistsError, source.copy, target)

    def test_copy_dir_to_itself(self):
        source = self.source_root / 'dirC'
        self.assertRaises(OSError, source.copy, source)
        self.assertRaises(OSError, source.copy, source, follow_symlinks=False)

    def test_copy_dir_into_itself(self):
        source = self.source_root / 'dirC'
        target = self.source_root / 'dirC' / 'dirD' / 'copyC'
        self.assertRaises(OSError, source.copy, target)
        self.assertRaises(OSError, source.copy, target, follow_symlinks=False)

    def test_copy_into(self):
        source = self.source_root / 'fileA'
        target_dir = self.target_root / 'dirA'
        self.target_ground.create_dir(target_dir)
        result = source.copy_into(target_dir)
        self.assertEqual(result, target_dir / 'fileA')
        self.assertTrue(self.target_ground.isfile(result))
        self.assertEqual(self.source_ground.readbytes(source),
                         self.target_ground.readbytes(result))

    def test_copy_into_empty_name(self):
        source = self.source_root.with_segments()
        target_dir = self.target_root / 'dirA'
        self.target_ground.create_dir(target_dir)
        self.assertRaises(ValueError, source.copy_into, target_dir)


class ZipToZipPathCopyTest(CopyTestBase, unittest.TestCase):
    source_ground = ZipPathGround(ReadableZipPath)
    target_ground = ZipPathGround(WritableZipPath)


if not is_pypi:
    from pathlib import Path

    class ZipToLocalPathCopyTest(CopyTestBase, unittest.TestCase):
        source_ground = ZipPathGround(ReadableZipPath)
        target_ground = LocalPathGround(Path)


    class LocalToZipPathCopyTest(CopyTestBase, unittest.TestCase):
        source_ground = LocalPathGround(Path)
        target_ground = ZipPathGround(WritableZipPath)


    class LocalToLocalPathCopyTest(CopyTestBase, unittest.TestCase):
        source_ground = LocalPathGround(Path)
        target_ground = LocalPathGround(Path)


if __name__ == "__main__":
    unittest.main()
