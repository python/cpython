import importlib
import pickle
import re
import unittest
from test.support import os_helper

try:
    import pyfakefs.fake_filesystem_unittest as ffs
except ImportError:
    from .stubs import fake_filesystem_unittest as ffs

from importlib.metadata import (
    Distribution,
    EntryPoint,
    PackageNotFoundError,
    _unique,
    distributions,
    entry_points,
    metadata,
    packages_distributions,
    version,
)

from . import fixtures
from ._path import Symlink


class BasicTests(fixtures.DistInfoPkg, unittest.TestCase):
    version_pattern = r'\d+\.\d+(\.\d)?'

    def test_retrieves_version_of_self(self):
        dist = Distribution.from_name('distinfo-pkg')
        assert isinstance(dist.version, str)
        assert re.match(self.version_pattern, dist.version)

    def test_for_name_does_not_exist(self):
        with self.assertRaises(PackageNotFoundError):
            Distribution.from_name('does-not-exist')

    def test_package_not_found_mentions_metadata(self):
        """
        When a package is not found, that could indicate that the
        package is not installed or that it is installed without
        metadata. Ensure the exception mentions metadata to help
        guide users toward the cause. See #124.
        """
        with self.assertRaises(PackageNotFoundError) as ctx:
            Distribution.from_name('does-not-exist')

        assert "metadata" in str(ctx.exception)

    def test_abc_enforced(self):
        with self.assertRaises(TypeError):
            type('DistributionSubclass', (Distribution,), {})()

    @fixtures.parameterize(
        dict(name=None),
        dict(name=''),
    )
    def test_invalid_inputs_to_from_name(self, name):
        with self.assertRaises(Exception):
            Distribution.from_name(name)


class ImportTests(fixtures.DistInfoPkg, unittest.TestCase):
    def test_import_nonexistent_module(self):
        # Ensure that the MetadataPathFinder does not crash an import of a
        # non-existent module.
        with self.assertRaises(ImportError):
            importlib.import_module('does_not_exist')

    def test_resolve(self):
        ep = entry_points(group='entries')['main']
        self.assertEqual(ep.load().__name__, "main")

    def test_entrypoint_with_colon_in_name(self):
        ep = entry_points(group='entries')['ns:sub']
        self.assertEqual(ep.value, 'mod:main')

    def test_resolve_without_attr(self):
        ep = EntryPoint(
            name='ep',
            value='importlib.metadata',
            group='grp',
        )
        assert ep.load() is importlib.metadata


class NameNormalizationTests(fixtures.OnSysPath, fixtures.SiteDir, unittest.TestCase):
    @staticmethod
    def make_pkg(name):
        """
        Create minimal metadata for a dist-info package with
        the indicated name on the file system.
        """
        return {
            f'{name}.dist-info': {
                'METADATA': 'VERSION: 1.0\n',
            },
        }

    def test_dashes_in_dist_name_found_as_underscores(self):
        """
        For a package with a dash in the name, the dist-info metadata
        uses underscores in the name. Ensure the metadata loads.
        """
        fixtures.build_files(self.make_pkg('my_pkg'), self.site_dir)
        assert version('my-pkg') == '1.0'

    def test_dist_name_found_as_any_case(self):
        """
        Ensure the metadata loads when queried with any case.
        """
        pkg_name = 'CherryPy'
        fixtures.build_files(self.make_pkg(pkg_name), self.site_dir)
        assert version(pkg_name) == '1.0'
        assert version(pkg_name.lower()) == '1.0'
        assert version(pkg_name.upper()) == '1.0'

    def test_unique_distributions(self):
        """
        Two distributions varying only by non-normalized name on
        the file system should resolve as the same.
        """
        fixtures.build_files(self.make_pkg('abc'), self.site_dir)
        before = list(_unique(distributions()))

        alt_site_dir = self.fixtures.enter_context(fixtures.tmp_path())
        self.fixtures.enter_context(self.add_sys_path(alt_site_dir))
        fixtures.build_files(self.make_pkg('ABC'), alt_site_dir)
        after = list(_unique(distributions()))

        assert len(after) == len(before)


class InvalidMetadataTests(fixtures.OnSysPath, fixtures.SiteDir, unittest.TestCase):
    @staticmethod
    def make_pkg(name, files=dict(METADATA="VERSION: 1.0")):
        """
        Create metadata for a dist-info package with name and files.
        """
        return {
            f'{name}.dist-info': files,
        }

    def test_valid_dists_preferred(self):
        """
        Dists with metadata should be preferred when discovered by name.

        Ref python/importlib_metadata#489.
        """
        # create three dists with the valid one in the middle (lexicographically)
        # such that on most file systems, the valid one is never naturally first.
        fixtures.build_files(self.make_pkg('foo-4.0', files={}), self.site_dir)
        fixtures.build_files(self.make_pkg('foo-4.1'), self.site_dir)
        fixtures.build_files(self.make_pkg('foo-4.2', files={}), self.site_dir)
        dist = Distribution.from_name('foo')
        assert dist.version == "1.0"

    def test_missing_metadata(self):
        """
        Dists with a missing metadata file should return None.

        Ref python/importlib_metadata#493.
        """
        fixtures.build_files(self.make_pkg('foo-4.3', files={}), self.site_dir)
        assert Distribution.from_name('foo').metadata is None
        assert metadata('foo') is None


class NonASCIITests(fixtures.OnSysPath, fixtures.SiteDir, unittest.TestCase):
    @staticmethod
    def pkg_with_non_ascii_description(site_dir):
        """
        Create minimal metadata for a package with non-ASCII in
        the description.
        """
        contents = {
            'portend.dist-info': {
                'METADATA': 'Description: pôrˈtend',
            },
        }
        fixtures.build_files(contents, site_dir)
        return 'portend'

    @staticmethod
    def pkg_with_non_ascii_description_egg_info(site_dir):
        """
        Create minimal metadata for an egg-info package with
        non-ASCII in the description.
        """
        contents = {
            'portend.dist-info': {
                'METADATA': """
                Name: portend

                pôrˈtend""",
            },
        }
        fixtures.build_files(contents, site_dir)
        return 'portend'

    def test_metadata_loads(self):
        pkg_name = self.pkg_with_non_ascii_description(self.site_dir)
        meta = metadata(pkg_name)
        assert meta['Description'] == 'pôrˈtend'

    def test_metadata_loads_egg_info(self):
        pkg_name = self.pkg_with_non_ascii_description_egg_info(self.site_dir)
        meta = metadata(pkg_name)
        assert meta['Description'] == 'pôrˈtend'


class DiscoveryTests(
    fixtures.EggInfoPkg,
    fixtures.EggInfoPkgPipInstalledNoToplevel,
    fixtures.EggInfoPkgPipInstalledNoModules,
    fixtures.EggInfoPkgSourcesFallback,
    fixtures.DistInfoPkg,
    unittest.TestCase,
):
    def test_package_discovery(self):
        dists = list(distributions())
        assert all(isinstance(dist, Distribution) for dist in dists)
        assert any(dist.metadata['Name'] == 'egginfo-pkg' for dist in dists)
        assert any(dist.metadata['Name'] == 'egg_with_module-pkg' for dist in dists)
        assert any(dist.metadata['Name'] == 'egg_with_no_modules-pkg' for dist in dists)
        assert any(dist.metadata['Name'] == 'sources_fallback-pkg' for dist in dists)
        assert any(dist.metadata['Name'] == 'distinfo-pkg' for dist in dists)

    def test_invalid_usage(self):
        with self.assertRaises(ValueError):
            list(distributions(context='something', name='else'))

    def test_interleaved_discovery(self):
        """
        Ensure interleaved searches are safe.

        When the search is cached, it is possible for searches to be
        interleaved, so make sure those use-cases are safe.

        Ref #293
        """
        dists = distributions()
        next(dists)
        version('egginfo-pkg')
        next(dists)


class DirectoryTest(fixtures.OnSysPath, fixtures.SiteDir, unittest.TestCase):
    def test_egg_info(self):
        # make an `EGG-INFO` directory that's unrelated
        self.site_dir.joinpath('EGG-INFO').mkdir()
        # used to crash with `IsADirectoryError`
        with self.assertRaises(PackageNotFoundError):
            version('unknown-package')

    def test_egg(self):
        egg = self.site_dir.joinpath('foo-3.6.egg')
        egg.mkdir()
        with self.add_sys_path(egg):
            with self.assertRaises(PackageNotFoundError):
                version('foo')


class MissingSysPath(fixtures.OnSysPath, unittest.TestCase):
    site_dir = '/does-not-exist'

    def test_discovery(self):
        """
        Discovering distributions should succeed even if
        there is an invalid path on sys.path.
        """
        importlib.metadata.distributions()


class InaccessibleSysPath(fixtures.OnSysPath, ffs.TestCase):
    site_dir = '/access-denied'

    def setUp(self):
        super().setUp()
        self.setUpPyfakefs()
        self.fs.create_dir(self.site_dir, perm_bits=000)

    def test_discovery(self):
        """
        Discovering distributions should succeed even if
        there is an invalid path on sys.path.
        """
        list(importlib.metadata.distributions())


class TestEntryPoints(unittest.TestCase):
    def __init__(self, *args):
        super().__init__(*args)
        self.ep = importlib.metadata.EntryPoint(
            name='name', value='value', group='group'
        )

    def test_entry_point_pickleable(self):
        revived = pickle.loads(pickle.dumps(self.ep))
        assert revived == self.ep

    def test_positional_args(self):
        """
        Capture legacy (namedtuple) construction, discouraged.
        """
        EntryPoint('name', 'value', 'group')

    def test_immutable(self):
        """EntryPoints should be immutable"""
        with self.assertRaises(AttributeError):
            self.ep.name = 'badactor'

    def test_repr(self):
        assert 'EntryPoint' in repr(self.ep)
        assert 'name=' in repr(self.ep)
        assert "'name'" in repr(self.ep)

    def test_hashable(self):
        """EntryPoints should be hashable"""
        hash(self.ep)

    def test_module(self):
        assert self.ep.module == 'value'

    def test_attr(self):
        assert self.ep.attr is None

    def test_sortable(self):
        """
        EntryPoint objects are sortable, but result is undefined.
        """
        sorted([
            EntryPoint(name='b', value='val', group='group'),
            EntryPoint(name='a', value='val', group='group'),
        ])


class FileSystem(
    fixtures.OnSysPath, fixtures.SiteDir, fixtures.FileBuilder, unittest.TestCase
):
    def test_unicode_dir_on_sys_path(self):
        """
        Ensure a Unicode subdirectory of a directory on sys.path
        does not crash.
        """
        fixtures.build_files(
            {self.unicode_filename(): {}},
            prefix=self.site_dir,
        )
        list(distributions())


class PackagesDistributionsPrebuiltTest(fixtures.ZipFixtures, unittest.TestCase):
    def test_packages_distributions_example(self):
        self._fixture_on_path('example-21.12-py3-none-any.whl')
        assert packages_distributions()['example'] == ['example']

    def test_packages_distributions_example2(self):
        """
        Test packages_distributions on a wheel built
        by trampolim.
        """
        self._fixture_on_path('example2-1.0.0-py3-none-any.whl')
        assert packages_distributions()['example2'] == ['example2']


class PackagesDistributionsTest(
    fixtures.OnSysPath, fixtures.SiteDir, unittest.TestCase
):
    def test_packages_distributions_neither_toplevel_nor_files(self):
        """
        Test a package built without 'top-level.txt' or a file list.
        """
        fixtures.build_files(
            {
                'trim_example-1.0.0.dist-info': {
                    'METADATA': """
                Name: trim_example
                Version: 1.0.0
                """,
                }
            },
            prefix=self.site_dir,
        )
        packages_distributions()

    def test_packages_distributions_all_module_types(self):
        """
        Test top-level modules detected on a package without 'top-level.txt'.
        """
        suffixes = importlib.machinery.all_suffixes()
        metadata = dict(
            METADATA="""
                Name: all_distributions
                Version: 1.0.0
                """,
        )
        files = {
            'all_distributions-1.0.0.dist-info': metadata,
        }
        for i, suffix in enumerate(suffixes):
            files.update({
                f'importable-name {i}{suffix}': '',
                f'in_namespace_{i}': {
                    f'mod{suffix}': '',
                },
                f'in_package_{i}': {
                    '__init__.py': '',
                    f'mod{suffix}': '',
                },
            })
        metadata.update(RECORD=fixtures.build_record(files))
        fixtures.build_files(files, prefix=self.site_dir)

        distributions = packages_distributions()

        for i in range(len(suffixes)):
            assert distributions[f'importable-name {i}'] == ['all_distributions']
            assert distributions[f'in_namespace_{i}'] == ['all_distributions']
            assert distributions[f'in_package_{i}'] == ['all_distributions']

        assert not any(name.endswith('.dist-info') for name in distributions)

    @os_helper.skip_unless_symlink
    def test_packages_distributions_symlinked_top_level(self) -> None:
        """
        Distribution is resolvable from a simple top-level symlink in RECORD.
        See #452.
        """

        files: fixtures.FilesSpec = {
            "symlinked_pkg-1.0.0.dist-info": {
                "METADATA": """
                    Name: symlinked-pkg
                    Version: 1.0.0
                    """,
                "RECORD": "symlinked,,\n",
            },
            ".symlink.target": {},
            "symlinked": Symlink(".symlink.target"),
        }

        fixtures.build_files(files, self.site_dir)
        assert packages_distributions()['symlinked'] == ['symlinked-pkg']


class PackagesDistributionsEggTest(
    fixtures.EggInfoPkg,
    fixtures.EggInfoPkgPipInstalledNoToplevel,
    fixtures.EggInfoPkgPipInstalledNoModules,
    fixtures.EggInfoPkgSourcesFallback,
    unittest.TestCase,
):
    def test_packages_distributions_on_eggs(self):
        """
        Test old-style egg packages with a variation of 'top_level.txt',
        'SOURCES.txt', and 'installed-files.txt', available.
        """
        distributions = packages_distributions()

        def import_names_from_package(package_name):
            return {
                import_name
                for import_name, package_names in distributions.items()
                if package_name in package_names
            }

        # egginfo-pkg declares one import ('mod') via top_level.txt
        assert import_names_from_package('egginfo-pkg') == {'mod'}

        # egg_with_module-pkg has one import ('egg_with_module') inferred from
        # installed-files.txt (top_level.txt is missing)
        assert import_names_from_package('egg_with_module-pkg') == {'egg_with_module'}

        # egg_with_no_modules-pkg should not be associated with any import names
        # (top_level.txt is empty, and installed-files.txt has no .py files)
        assert import_names_from_package('egg_with_no_modules-pkg') == set()

        # sources_fallback-pkg has one import ('sources_fallback') inferred from
        # SOURCES.txt (top_level.txt and installed-files.txt is missing)
        assert import_names_from_package('sources_fallback-pkg') == {'sources_fallback'}


class EditableDistributionTest(fixtures.DistInfoPkgEditable, unittest.TestCase):
    def test_origin(self):
        dist = Distribution.from_name('distinfo-pkg')
        assert dist.origin.url.endswith('.whl')
        assert dist.origin.archive_info.hashes.sha256
