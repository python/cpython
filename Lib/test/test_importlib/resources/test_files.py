import typing
import unittest

from importlib import resources
from importlib.abc import Traversable
from . import data01
from . import util


class FilesTests:
    def test_read_bytes(self):
        files = resources.files(self.data)
        actual = files.joinpath('utf-8.file').read_bytes()
        assert actual == b'Hello, UTF-8 world!\n'

    def test_read_text(self):
        files = resources.files(self.data)
        actual = files.joinpath('utf-8.file').read_text(encoding='utf-8')
        assert actual == 'Hello, UTF-8 world!\n'

    @unittest.skipUnless(
        hasattr(typing, 'runtime_checkable'),
        "Only suitable when typing supports runtime_checkable",
    )
    def test_traversable(self):
        assert isinstance(resources.files(self.data), Traversable)


class OpenDiskTests(FilesTests, unittest.TestCase):
    def setUp(self):
        self.data = data01


class OpenZipTests(FilesTests, util.ZipSetup, unittest.TestCase):
    pass


class OpenNamespaceTests(FilesTests, unittest.TestCase):
    def setUp(self):
        from . import namespacedata01

        self.data = namespacedata01


if __name__ == '__main__':
    unittest.main()
