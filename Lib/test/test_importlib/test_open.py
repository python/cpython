import unittest

from importlib import resources
from . import data01
from . import util


class CommonBinaryTests(util.CommonResourceTests, unittest.TestCase):
    def execute(self, package, path):
        with resources.open_binary(package, path):
            pass


class CommonTextTests(util.CommonResourceTests, unittest.TestCase):
    def execute(self, package, path):
        with resources.open_text(package, path):
            pass


class OpenTests:
    def test_open_binary(self):
        with resources.open_binary(self.data, 'utf-8.file') as fp:
            result = fp.read()
            self.assertEqual(result, b'Hello, UTF-8 world!\n')

    def test_open_text_default_encoding(self):
        with resources.open_text(self.data, 'utf-8.file') as fp:
            result = fp.read()
            self.assertEqual(result, 'Hello, UTF-8 world!\n')

    def test_open_text_given_encoding(self):
        with resources.open_text(
                self.data, 'utf-16.file', 'utf-16', 'strict') as fp:
            result = fp.read()
        self.assertEqual(result, 'Hello, UTF-16 world!\n')

    def test_open_text_with_errors(self):
        # Raises UnicodeError without the 'errors' argument.
        with resources.open_text(
                self.data, 'utf-16.file', 'utf-8', 'strict') as fp:
            self.assertRaises(UnicodeError, fp.read)
        with resources.open_text(
                self.data, 'utf-16.file', 'utf-8', 'ignore') as fp:
            result = fp.read()
        self.assertEqual(
            result,
            'H\x00e\x00l\x00l\x00o\x00,\x00 '
            '\x00U\x00T\x00F\x00-\x001\x006\x00 '
            '\x00w\x00o\x00r\x00l\x00d\x00!\x00\n\x00')

    def test_open_binary_FileNotFoundError(self):
        self.assertRaises(
            FileNotFoundError,
            resources.open_binary, self.data, 'does-not-exist')

    def test_open_text_FileNotFoundError(self):
        self.assertRaises(
            FileNotFoundError,
            resources.open_text, self.data, 'does-not-exist')


class OpenDiskTests(OpenTests, unittest.TestCase):
    def setUp(self):
        self.data = data01


class OpenZipTests(OpenTests, util.ZipSetup, unittest.TestCase):
    pass


if __name__ == '__main__':
    unittest.main()
