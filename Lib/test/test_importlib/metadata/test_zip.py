import multiprocessing
import os
import sys
import unittest

from test.support import requires_fork, warnings_helper

from importlib.metadata import (
    FastPath,
    PackageNotFoundError,
    distribution,
    distributions,
    entry_points,
    files,
    version,
)

from . import fixtures


class TestZip(fixtures.ZipFixtures, unittest.TestCase):
    def setUp(self):
        super().setUp()
        self._fixture_on_path('example-21.12-py3-none-any.whl')

    def test_zip_version(self):
        self.assertEqual(version('example'), '21.12')

    def test_zip_version_does_not_match(self):
        with self.assertRaises(PackageNotFoundError):
            version('definitely-not-installed')

    def test_zip_entry_points(self):
        scripts = entry_points(group='console_scripts')
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

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_fork()
    @unittest.skipUnless(
        hasattr(os, 'register_at_fork')
        and 'fork' in multiprocessing.get_all_start_methods(),
        'requires fork-based multiprocessing support',
    )
    def test_fastpath_cache_cleared_in_forked_child(self):
        zip_path = sys.path[0]

        FastPath(zip_path)
        assert FastPath.__new__.cache_info().currsize >= 1

        ctx = multiprocessing.get_context('fork')
        parent_conn, child_conn = ctx.Pipe()

        def child(conn, root):
            try:
                before = FastPath.__new__.cache_info().currsize
                FastPath(root)
                after = FastPath.__new__.cache_info().currsize
                conn.send((before, after))
            finally:
                conn.close()

        proc = ctx.Process(target=child, args=(child_conn, zip_path))
        proc.start()
        child_conn.close()
        cache_sizes = parent_conn.recv()
        proc.join()

        self.assertEqual(cache_sizes, (0, 1))


class TestEgg(TestZip):
    def setUp(self):
        super().setUp()
        self._fixture_on_path('example-21.12-py3.6.egg')

    def test_files(self):
        for file in files('example'):
            path = str(file.dist.locate_file(file))
            assert '.egg/' in path, path

    def test_normalized_name(self):
        dist = distribution('example')
        assert dist._normalized_name == 'example'
