import sys
import unittest

from contextlib import ExitStack
from importlib.metadata import (
    distribution, entry_points, files, PackageNotFoundError,
    version, distributions,
)
from importlib import resources

from test.support import requires_zlib


@requires_zlib()
class TestZip(unittest.TestCase):
    root = 'test.test_importlib.data'

    def _fixture_on_path(self, filename):
        pkg_file = resources.files(self.root).joinpath(filename)
        file = self.resources.enter_context(resources.as_file(pkg_file))
        assert file.name.startswith('example-'), file.name
        sys.path.insert(0, str(file))
        self.resources.callback(sys.path.pop, 0)

    def setUp(self):
        # Find the path to the example-*.whl so we can add it to the front of
        # sys.path, where we'll then try to find the metadata thereof.
        self.resources = ExitStack()
        self.addCleanup(self.resources.close)
        self._fixture_on_path('example-21.12-py3-none-any.whl')

    def test_zip_version(self):
        self.assertEqual(version('example'), '21.12')

    def test_zip_version_does_not_match(self):
        with self.assertRaises(PackageNotFoundError):
            version('definitely-not-installed')

    def test_zip_entry_points(self):
        scripts = dict(entry_points()['console_scripts'])
        entry_point = scripts['example']
        self.assertEqual(entry_point.value, 'example:main')
        entry_point = scripts['Example']
        self.assertEqual(entry_point.value, 'example:main')

    def test_missing_metadata(self):
        self.assertIsNone(distribution('example').read_text('does not exist'))

    def test_case_insensitive(self):
        self.assertEqual(version('Example'), '21.12')

    def test_files(self):
        for file in files('example'):
            path = str(file.dist.locate_file(file))
            assert '.whl/' in path, path

    def test_one_distribution(self):
        dists = list(distributions(path=sys.path[:1]))
        assert len(dists) == 1


@requires_zlib()
class TestEgg(TestZip):
    def setUp(self):
        # Find the path to the example-*.egg so we can add it to the front of
        # sys.path, where we'll then try to find the metadata thereof.
        self.resources = ExitStack()
        self.addCleanup(self.resources.close)
        self._fixture_on_path('example-21.12-py3.6.egg')

    def test_files(self):
        for file in files('example'):
            path = str(file.dist.locate_file(file))
            assert '.egg/' in path, path
