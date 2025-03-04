import unittest

from pathlib import Path

from test.test_pathlib.support.local_path import LocalPathGround
from test.test_pathlib.support.zip_path import ZipPathGround, ReadableZipPath, WritableZipPath


class CopyPathTestBase:
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


class CopyZipPathToZipPathTest(CopyPathTestBase, unittest.TestCase):
    source_ground = ZipPathGround(ReadableZipPath)
    target_ground = ZipPathGround(WritableZipPath)


class CopyZipPathToPathTest(CopyPathTestBase, unittest.TestCase):
    source_ground = ZipPathGround(ReadableZipPath)
    target_ground = LocalPathGround(Path)


class CopyPathToZipPathTest(CopyPathTestBase, unittest.TestCase):
    source_ground = LocalPathGround(Path)
    target_ground = ZipPathGround(WritableZipPath)


class CopyPathToPathTest(CopyPathTestBase, unittest.TestCase):
    source_ground = LocalPathGround(Path)
    target_ground = LocalPathGround(Path)


if __name__ == "__main__":
    unittest.main()
