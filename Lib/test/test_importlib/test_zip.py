import sys
import unittest

from contextlib import ExitStack
from importlib.metadata import distribution, entry_points, files, version
from importlib.resources import path


class TestZip(unittest.TestCase):
    root = 'test.test_importlib.data'

    def setUp(self):
        # Find the path to the example-*.whl so we can add it to the front of
        # sys.path, where we'll then try to find the metadata thereof.
        self.resources = ExitStack()
        self.addCleanup(self.resources.close)
        wheel = self.resources.enter_context(
            path(self.root, 'example-21.12-py3-none-any.whl'))
        sys.path.insert(0, str(wheel))
        self.resources.callback(sys.path.pop, 0)

    def test_zip_version(self):
        self.assertEqual(version('example'), '21.12')

    def test_zip_entry_points(self):
        scripts = dict(entry_points()['console_scripts'])
        entry_point = scripts['example']
        self.assertEqual(entry_point.value, 'example:main')

    def test_missing_metadata(self):
        self.assertIsNone(distribution('example').read_text('does not exist'))

    def test_case_insensitive(self):
        self.assertEqual(version('Example'), '21.12')

    def test_files(self):
        for file in files('example'):
            path = str(file.dist.locate_file(file))
            assert '.whl/' in path, path


class TestEgg(TestZip):
    def setUp(self):
        # Find the path to the example-*.egg so we can add it to the front of
        # sys.path, where we'll then try to find the metadata thereof.
        self.resources = ExitStack()
        self.addCleanup(self.resources.close)
        egg = self.resources.enter_context(
            path(self.root, 'example-21.12-py3.6.egg'))
        sys.path.insert(0, str(egg))
        self.resources.callback(sys.path.pop, 0)

    def test_files(self):
        for file in files('example'):
            path = str(file.dist.locate_file(file))
            assert '.egg/' in path, path
