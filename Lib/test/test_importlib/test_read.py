import unittest

from importlib import import_module, resources
from . import data01
from . import util


class CommonBinaryTests(util.CommonResourceTests, unittest.TestCase):
    def execute(self, package, path):
        resources.read_binary(package, path)


class CommonTextTests(util.CommonResourceTests, unittest.TestCase):
    def execute(self, package, path):
        resources.read_text(package, path)


class ReadTests:
    def test_read_binary(self):
        result = resources.read_binary(self.data, 'binary.file')
        self.assertEqual(result, b'\0\1\2\3')

    def test_read_text_default_encoding(self):
        result = resources.read_text(self.data, 'utf-8.file')
        self.assertEqual(result, 'Hello, UTF-8 world!\n')

    def test_read_text_given_encoding(self):
        result = resources.read_text(self.data, 'utf-16.file', encoding='utf-16')
        self.assertEqual(result, 'Hello, UTF-16 world!\n')

    def test_read_text_with_errors(self):
        # Raises UnicodeError without the 'errors' argument.
        self.assertRaises(UnicodeError, resources.read_text, self.data, 'utf-16.file')
        result = resources.read_text(self.data, 'utf-16.file', errors='ignore')
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
        result = resources.read_binary(submodule, 'binary.file')
        self.assertEqual(result, b'\0\1\2\3')

    def test_read_submodule_resource_by_name(self):
        result = resources.read_binary('ziptestdata.subdirectory', 'binary.file')
        self.assertEqual(result, b'\0\1\2\3')


if __name__ == '__main__':
    unittest.main()
