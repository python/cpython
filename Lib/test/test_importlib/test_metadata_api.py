import re
import textwrap
import unittest
import itertools

from collections.abc import Iterator

from . import fixtures
from importlib.metadata import (
    Distribution, PackageNotFoundError, distribution,
    entry_points, files, metadata, requires, version,
    )


class APITests(
        fixtures.EggInfoPkg,
        fixtures.DistInfoPkg,
        fixtures.EggInfoFile,
        unittest.TestCase):

    version_pattern = r'\d+\.\d+(\.\d)?'

    def test_retrieves_version_of_self(self):
        pkg_version = version('egginfo-pkg')
        assert isinstance(pkg_version, str)
        assert re.match(self.version_pattern, pkg_version)

    def test_retrieves_version_of_distinfo_pkg(self):
        pkg_version = version('distinfo-pkg')
        assert isinstance(pkg_version, str)
        assert re.match(self.version_pattern, pkg_version)

    def test_for_name_does_not_exist(self):
        with self.assertRaises(PackageNotFoundError):
            distribution('does-not-exist')

    def test_for_top_level(self):
        self.assertEqual(
            distribution('egginfo-pkg').read_text('top_level.txt').strip(),
            'mod')

    def test_read_text(self):
        top_level = [
            path for path in files('egginfo-pkg')
            if path.name == 'top_level.txt'
            ][0]
        self.assertEqual(top_level.read_text(), 'mod\n')

    def test_entry_points(self):
        entries = dict(entry_points()['entries'])
        ep = entries['main']
        self.assertEqual(ep.value, 'mod:main')
        self.assertEqual(ep.extras, [])

    def test_metadata_for_this_package(self):
        md = metadata('egginfo-pkg')
        assert md['author'] == 'Steven Ma'
        assert md['LICENSE'] == 'Unknown'
        assert md['Name'] == 'egginfo-pkg'
        classifiers = md.get_all('Classifier')
        assert 'Topic :: Software Development :: Libraries' in classifiers

    @staticmethod
    def _test_files(files_iter):
        assert isinstance(files_iter, Iterator), files_iter
        files = list(files_iter)
        root = files[0].root
        for file in files:
            assert file.root == root
            assert not file.hash or file.hash.value
            assert not file.hash or file.hash.mode == 'sha256'
            assert not file.size or file.size >= 0
            assert file.locate().exists()
            assert isinstance(file.read_binary(), bytes)
            if file.name.endswith('.py'):
                file.read_text()

    def test_file_hash_repr(self):
        assertRegex = self.assertRegex

        util = [
            p for p in files('distinfo-pkg')
            if p.name == 'mod.py'
            ][0]
        assertRegex(
            repr(util.hash),
            '<FileHash mode: sha256 value: .*>')

    def test_files_dist_info(self):
        self._test_files(files('distinfo-pkg'))

    def test_files_egg_info(self):
        self._test_files(files('egginfo-pkg'))

    def test_version_egg_info_file(self):
        self.assertEqual(version('egginfo-file'), '0.1')

    def test_requires_egg_info_file(self):
        requirements = requires('egginfo-file')
        self.assertIsNone(requirements)

    def test_requires(self):
        deps = requires('egginfo-pkg')
        assert any(
            dep == 'wheel >= 1.0; python_version >= "2.7"'
            for dep in deps
            )

    def test_requires_dist_info(self):
        deps = list(requires('distinfo-pkg'))
        assert deps and all(deps)
        assert 'wheel >= 1.0' in deps
        assert "pytest; extra == 'test'" in deps

    def test_more_complex_deps_requires_text(self):
        requires = textwrap.dedent("""
            dep1
            dep2

            [:python_version < "3"]
            dep3

            [extra1]
            dep4

            [extra2:python_version < "3"]
            dep5
            """)
        deps = sorted(Distribution._deps_from_requires_text(requires))
        expected = [
            'dep1',
            'dep2',
            'dep3; python_version < "3"',
            'dep4; extra == "extra1"',
            'dep5; (python_version < "3") and extra == "extra2"',
            ]
        # It's important that the environment marker expression be
        # wrapped in parentheses to avoid the following 'and' binding more
        # tightly than some other part of the environment expression.

        assert deps == expected


class OffSysPathTests(fixtures.DistInfoPkgOffPath, unittest.TestCase):
    def test_find_distributions_specified_path(self):
        dists = itertools.chain.from_iterable(
            resolver(path=[str(self.site_dir)])
            for resolver in Distribution._discover_resolvers()
            )
        assert any(
            dist.metadata['Name'] == 'distinfo-pkg'
            for dist in dists
            )
