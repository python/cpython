import unittest

from importlib import import_module, resources
from . import data01
from . import util


class CommonBinaryTests(util.CommonTests, unittest.TestCase):
    def execute(self, package, path):
        resources.files(package).joinpath(path).read_bytes()


class CommonTextTests(util.CommonTests, unittest.TestCase):
    def execute(self, package, path):
        resources.files(package).joinpath(path).read_text()


class ReadTests:
    def test_read_bytes(self):
        result = resources.files(self.data).joinpath('binary.file').read_bytes()
        self.assertEqual(result, b'\0\1\2\3')

    def test_read_text_default_encoding(self):
        result = resources.files(self.data).joinpath('utf-8.file').read_text()
        self.assertEqual(result, 'Hello, UTF-8 world!\n')

    def test_read_text_given_encoding(self):
        result = (
            resources.files(self.data)
            .joinpath('utf-16.file')
            .read_text(encoding='utf-16')
        )
        self.assertEqual(result, 'Hello, UTF-16 world!\n')

    def test_read_text_with_errors(self):
        # Raises UnicodeError without the 'errors' argument.
        target = resources.files(self.data) / 'utf-16.file'
        self.assertRaises(UnicodeError, target.read_text, encoding='utf-8')
        result = target.read_text(encoding='utf-8', errors='ignore')
        self.assertEqual(
            result,
            'H\x00e\x00l\x00l\x00o\x00,\x00 '
            '\x00U\x00T\x00F\x00-\x001\x006\x00 '
            '\x00w\x00o\x00r\x00l\x00d\x00!\x00\n\x00',
        )


class ReadDiskTests(ReadTests, unittest.TestCase):
    data = data01


class ReadZipTests(ReadTests, util.ZipSetup, unittest.TestCase):
    def test_read_submodule_resource(self):
        submodule = import_module('ziptestdata.subdirectory')
        result = resources.files(submodule).joinpath('binary.file').read_bytes()
        self.assertEqual(result, b'\0\1\2\3')

    def test_read_submodule_resource_by_name(self):
        result = (
            resources.files('ziptestdata.subdirectory')
            .joinpath('binary.file')
            .read_bytes()
        )
        self.assertEqual(result, b'\0\1\2\3')


class ReadNamespaceTests(ReadTests, unittest.TestCase):
    def setUp(self):
        from . import namespacedata01

        self.data = namespacedata01


if __name__ == '__main__':
    unittest.main()
