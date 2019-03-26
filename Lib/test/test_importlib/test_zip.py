import sys
import unittest
import importlib.metadata

from importlib.resources import path

try:
    from contextlib import ExitStack
except ImportError:
    from contextlib2 import ExitStack


class BespokeLoader:
    archive = 'bespoke'


class TestZip(unittest.TestCase):
    def setUp(self):
        # Find the path to the example.*.whl so we can add it to the front of
        # sys.path, where we'll then try to find the metadata thereof.
        self.resources = ExitStack()
        self.addCleanup(self.resources.close)
        wheel = self.resources.enter_context(
            path('test.test_importlib.data',
                 'example-21.12-py3-none-any.whl'))
        sys.path.insert(0, str(wheel))
        self.resources.callback(sys.path.pop, 0)

    def test_zip_version(self):
        self.assertEqual(importlib.metadata.version('example'), '21.12')

    def test_zip_entry_points(self):
        scripts = dict(importlib.metadata.entry_points()['console_scripts'])
        entry_point = scripts['example']
        self.assertEqual(entry_point.value, 'example:main')

    def test_missing_metadata(self):
        distribution = importlib.metadata.distribution('example')
        self.assertIsNone(distribution.read_text('does not exist'))

    def test_case_insensitive(self):
        self.assertEqual(importlib.metadata.version('Example'), '21.12')

    def test_files(self):
        files = importlib.metadata.files('example')
        for file in files:
            path = str(file.dist.locate_file(file))
            assert '.whl/' in path, path
