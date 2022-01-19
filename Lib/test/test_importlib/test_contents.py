import unittest
from importlib import resources

from test.test_importlib import data01
from test.test_importlib.resources import util


class ContentsTests:
    expected = {
        '__init__.py',
        'binary.file',
        'subdirectory',
        'utf-16.file',
        'utf-8.file',
    }

    def test_contents(self):
        contents = {path.name for path in resources.files(self.data).iterdir()}
        assert self.expected <= contents


class ContentsDiskTests(ContentsTests, unittest.TestCase):
    def setUp(self):
        self.data = data01


class ContentsZipTests(ContentsTests, util.ZipSetup, unittest.TestCase):
    pass


class ContentsNamespaceTests(ContentsTests, unittest.TestCase):
    expected = {
        # no __init__ because of namespace design
        # no subdirectory as incidental difference in fixture
        'binary.file',
        'utf-16.file',
        'utf-8.file',
    }

    def setUp(self):
        from test.test_importlib import namespacedata01

        self.data = namespacedata01


if __name__ == '__main__':
    unittest.main()
