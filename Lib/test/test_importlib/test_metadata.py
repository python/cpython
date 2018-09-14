import re
import sys
import unittest
import importlib

from contextlib import ExitStack
from importlib import metadata
from importlib.resources import path
from test.support import DirsOnSysPath, unload
from types import ModuleType


class BespokeLoader:
    archive = 'bespoke'


class TestMetadata(unittest.TestCase):
    version_pattern = r'\d+\.\d+(\.\d)?'

    def setUp(self):
        # Find the path to the example.*.whl so we can add it to the front of
        # sys.path, where we'll then try to find the metadata thereof.
        self.resources = ExitStack()
        self.addCleanup(self.resources.close)
        wheel = self.resources.enter_context(
            path('test.test_importlib.data00',
                 'example-21.12-py3-none-any.whl'))
        self.resources.enter_context(DirsOnSysPath(str(wheel)))
        self.resources.callback(unload, 'example')

    def test_retrieves_version(self):
        self.assertEqual(metadata.version('example'), '21.12')

    def test_retrieves_version_from_distribution(self):
        example = importlib.import_module('example')
        dist = metadata.Distribution.from_module(example)
        assert isinstance(dist.version, str)
        assert re.match(self.version_pattern, dist.version)

    def test_for_name_does_not_exist(self):
        with self.assertRaises(metadata.PackageNotFoundError):
            metadata.distribution('does-not-exist')

    def test_for_name_does_not_exist_from_distribution(self):
        with self.assertRaises(metadata.PackageNotFoundError):
            metadata.Distribution.from_name('does-not-exist')

    def test_entry_points(self):
        parser = metadata.entry_points('example')
        entry_point = parser.get('console_scripts', 'example')
        self.assertEqual(entry_point, 'example:main')

    def test_not_a_zip(self):
        # For coverage purposes, this module is importable, but has neither a
        # location on the file system, nor a .archive attribute.
        sys.modules['bespoke'] = ModuleType('bespoke')
        self.resources.callback(unload, 'bespoke')
        self.assertRaises(ImportError, metadata.version, 'bespoke')

    def test_unversioned_dist_info(self):
        # For coverage purposes, give the module an unversioned .archive
        # attribute.
        bespoke = sys.modules['bespoke'] = ModuleType('bespoke')
        bespoke.__loader__ = BespokeLoader()
        self.resources.callback(unload, 'bespoke')
        self.assertRaises(ImportError, metadata.version, 'bespoke')

    def test_missing_metadata(self):
        distribution = metadata.distribution('example')
        self.assertIsNone(distribution.load_metadata('does not exist'))

    def test_for_module_by_name(self):
        name = 'example'
        distribution = metadata.distribution(name)
        self.assertEqual(
            distribution.load_metadata('top_level.txt').strip(),
            'example')

    def test_for_module_distribution_by_name(self):
        name = 'example'
        metadata.Distribution.from_named_module(name)

    def test_resolve(self):
        entry_points = metadata.entry_points('example')
        main = metadata.resolve(
            entry_points.get('console_scripts', 'example'))
        import example
        self.assertEqual(main, example.main)

    def test_resolve_invalid(self):
        self.assertRaises(ValueError, metadata.resolve, 'bogus.ep')
