import re
import textwrap
import unittest
import warnings
import importlib

from . import fixtures
from importlib.metadata import (
    Distribution,
    PackageNotFoundError,
    distribution,
    entry_points,
    files,
    metadata,
    requires,
    version,
)


class APITests(
    fixtures.EggInfoPkg,
    fixtures.DistInfoPkg,
    fixtures.DistInfoPkgWithDot,
    fixtures.EggInfoFile,
    unittest.TestCase,
):

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

    def test_name_normalization(self):
        names = 'pkg.dot', 'pkg_dot', 'pkg-dot', 'pkg..dot', 'Pkg.Dot'
        for name in names:
            with self.subTest(name):
                assert distribution(name).metadata['Name'] == 'pkg.dot'

    def test_prefix_not_matched(self):
        prefixes = 'p', 'pkg', 'pkg.'
        for prefix in prefixes:
            with self.subTest(prefix):
                with self.assertRaises(PackageNotFoundError):
                    distribution(prefix)

    def test_for_top_level(self):
        self.assertEqual(
            distribution('egginfo-pkg').read_text('top_level.txt').strip(), 'mod'
        )

    def test_read_text(self):
        top_level = [
            path for path in files('egginfo-pkg') if path.name == 'top_level.txt'
        ][0]
        self.assertEqual(top_level.read_text(), 'mod\n')

    def test_entry_points(self):
        eps = entry_points()
        assert 'entries' in eps.groups
        entries = eps.select(group='entries')
        assert 'main' in entries.names
        ep = entries['main']
        self.assertEqual(ep.value, 'mod:main')
        self.assertEqual(ep.extras, [])

    def test_entry_points_distribution(self):
        entries = entry_points(group='entries')
        for entry in ("main", "ns:sub"):
            ep = entries[entry]
            self.assertIn(ep.dist.name, ('distinfo-pkg', 'egginfo-pkg'))
            self.assertEqual(ep.dist.version, "1.0.0")

    def test_entry_points_unique_packages(self):
        # Entry points should only be exposed for the first package
        # on sys.path with a given name.
        alt_site_dir = self.fixtures.enter_context(fixtures.tempdir())
        self.fixtures.enter_context(self.add_sys_path(alt_site_dir))
        alt_pkg = {
            "distinfo_pkg-1.1.0.dist-info": {
                "METADATA": """
                Name: distinfo-pkg
                Version: 1.1.0
                """,
                "entry_points.txt": """
                [entries]
                main = mod:altmain
            """,
            },
        }
        fixtures.build_files(alt_pkg, alt_site_dir)
        entries = entry_points(group='entries')
        assert not any(
            ep.dist.name == 'distinfo-pkg' and ep.dist.version == '1.0.0'
            for ep in entries
        )
        # ns:sub doesn't exist in alt_pkg
        assert 'ns:sub' not in entries

    def test_entry_points_missing_name(self):
        with self.assertRaises(KeyError):
            entry_points(group='entries')['missing']

    def test_entry_points_missing_group(self):
        assert entry_points(group='missing') == ()

    def test_entry_points_dict_construction(self):
        # Prior versions of entry_points() returned simple lists and
        # allowed casting those lists into maps by name using ``dict()``.
        # Capture this now deprecated use-case.
        with warnings.catch_warnings(record=True) as caught:
            warnings.filterwarnings("default", category=DeprecationWarning)
            eps = dict(entry_points(group='entries'))

        assert 'main' in eps
        assert eps['main'] == entry_points(group='entries')['main']

        # check warning
        expected = next(iter(caught))
        assert expected.category is DeprecationWarning
        assert "Construction of dict of EntryPoints is deprecated" in str(expected)

    def test_entry_points_by_index(self):
        """
        Prior versions of Distribution.entry_points would return a
        tuple that allowed access by index.
        Capture this now deprecated use-case
        See python/importlib_metadata#300 and bpo-44246.
        """
        eps = distribution('distinfo-pkg').entry_points
        with warnings.catch_warnings(record=True) as caught:
            eps[0]

        # check warning
        expected = next(iter(caught))
        assert expected.category is DeprecationWarning
        assert "Accessing entry points by index is deprecated" in str(expected)

    def test_entry_points_groups_getitem(self):
        # Prior versions of entry_points() returned a dict. Ensure
        # that callers using '.__getitem__()' are supported but warned to
        # migrate.
        with warnings.catch_warnings(record=True):
            entry_points()['entries'] == entry_points(group='entries')

            with self.assertRaises(KeyError):
                entry_points()['missing']

    def test_entry_points_groups_get(self):
        # Prior versions of entry_points() returned a dict. Ensure
        # that callers using '.get()' are supported but warned to
        # migrate.
        with warnings.catch_warnings(record=True):
            entry_points().get('missing', 'default') == 'default'
            entry_points().get('entries', 'default') == entry_points()['entries']
            entry_points().get('missing', ()) == ()

    def test_metadata_for_this_package(self):
        md = metadata('egginfo-pkg')
        assert md['author'] == 'Steven Ma'
        assert md['LICENSE'] == 'Unknown'
        assert md['Name'] == 'egginfo-pkg'
        classifiers = md.get_all('Classifier')
        assert 'Topic :: Software Development :: Libraries' in classifiers

    @staticmethod
    def _test_files(files):
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

        util = [p for p in files('distinfo-pkg') if p.name == 'mod.py'][0]
        assertRegex(repr(util.hash), '<FileHash mode: sha256 value: .*>')

    def test_files_dist_info(self):
        self._test_files(files('distinfo-pkg'))

    def test_files_egg_info(self):
        self._test_files(files('egginfo-pkg'))

    def test_version_egg_info_file(self):
        self.assertEqual(version('egginfo-file'), '0.1')

    def test_requires_egg_info_file(self):
        requirements = requires('egginfo-file')
        self.assertIsNone(requirements)

    def test_requires_egg_info(self):
        deps = requires('egginfo-pkg')
        assert len(deps) == 2
        assert any(dep == 'wheel >= 1.0; python_version >= "2.7"' for dep in deps)

    def test_requires_dist_info(self):
        deps = requires('distinfo-pkg')
        assert len(deps) == 2
        assert all(deps)
        assert 'wheel >= 1.0' in deps
        assert "pytest; extra == 'test'" in deps

    def test_more_complex_deps_requires_text(self):
        requires = textwrap.dedent(
            """
            dep1
            dep2

            [:python_version < "3"]
            dep3

            [extra1]
            dep4

            [extra2:python_version < "3"]
            dep5
            """
        )
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

    def test_as_json(self):
        md = metadata('distinfo-pkg').json
        assert 'name' in md
        assert md['keywords'] == ['sample', 'package']
        desc = md['description']
        assert desc.startswith('Once upon a time\nThere was')
        assert len(md['requires_dist']) == 2

    def test_as_json_egg_info(self):
        md = metadata('egginfo-pkg').json
        assert 'name' in md
        assert md['keywords'] == ['sample', 'package']
        desc = md['description']
        assert desc.startswith('Once upon a time\nThere was')
        assert len(md['classifier']) == 2

    def test_as_json_odd_case(self):
        self.make_uppercase()
        md = metadata('distinfo-pkg').json
        assert 'name' in md
        assert len(md['requires_dist']) == 2
        assert md['keywords'] == ['SAMPLE', 'PACKAGE']


class LegacyDots(fixtures.DistInfoPkgWithDotLegacy, unittest.TestCase):
    def test_name_normalization(self):
        names = 'pkg.dot', 'pkg_dot', 'pkg-dot', 'pkg..dot', 'Pkg.Dot'
        for name in names:
            with self.subTest(name):
                assert distribution(name).metadata['Name'] == 'pkg.dot'

    def test_name_normalization_versionless_egg_info(self):
        names = 'pkg.lot', 'pkg_lot', 'pkg-lot', 'pkg..lot', 'Pkg.Lot'
        for name in names:
            with self.subTest(name):
                assert distribution(name).metadata['Name'] == 'pkg.lot'


class OffSysPathTests(fixtures.DistInfoPkgOffPath, unittest.TestCase):
    def test_find_distributions_specified_path(self):
        dists = Distribution.discover(path=[str(self.site_dir)])
        assert any(dist.metadata['Name'] == 'distinfo-pkg' for dist in dists)

    def test_distribution_at_pathlib(self):
        # Demonstrate how to load metadata direct from a directory.
        dist_info_path = self.site_dir / 'distinfo_pkg-1.0.0.dist-info'
        dist = Distribution.at(dist_info_path)
        assert dist.version == '1.0.0'

    def test_distribution_at_str(self):
        dist_info_path = self.site_dir / 'distinfo_pkg-1.0.0.dist-info'
        dist = Distribution.at(str(dist_info_path))
        assert dist.version == '1.0.0'


class InvalidateCache(unittest.TestCase):
    def test_invalidate_cache(self):
        # No externally observable behavior, but ensures test coverage...
        importlib.invalidate_caches()
