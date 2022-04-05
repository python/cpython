import re
import json
import pickle
import textwrap
import unittest
import warnings
import importlib.metadata

try:
    import pyfakefs.fake_filesystem_unittest as ffs
except ImportError:
    from .stubs import fake_filesystem_unittest as ffs

from . import fixtures
from importlib.metadata import (
    Distribution,
    EntryPoint,
    PackageNotFoundError,
    distributions,
    entry_points,
    metadata,
    packages_distributions,
    version,
)


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
        # When a package is not found, that could indicate that the
        # packgae is not installed or that it is installed without
        # metadata. Ensure the exception mentions metadata to help
        # guide users toward the cause. See #124.
        with self.assertRaises(PackageNotFoundError) as ctx:
            Distribution.from_name('does-not-exist')

        assert "metadata" in str(ctx.exception)

    def test_new_style_classes(self):
        self.assertIsInstance(Distribution, type)


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
    def pkg_with_dashes(site_dir):
        """
        Create minimal metadata for a package with dashes
        in the name (and thus underscores in the filename).
        """
        metadata_dir = site_dir / 'my_pkg.dist-info'
        metadata_dir.mkdir()
        metadata = metadata_dir / 'METADATA'
        with metadata.open('w', encoding='utf-8') as strm:
            strm.write('Version: 1.0\n')
        return 'my-pkg'

    def test_dashes_in_dist_name_found_as_underscores(self):
        # For a package with a dash in the name, the dist-info metadata
        # uses underscores in the name. Ensure the metadata loads.
        pkg_name = self.pkg_with_dashes(self.site_dir)
        assert version(pkg_name) == '1.0'

    @staticmethod
    def pkg_with_mixed_case(site_dir):
        """
        Create minimal metadata for a package with mixed case
        in the name.
        """
        metadata_dir = site_dir / 'CherryPy.dist-info'
        metadata_dir.mkdir()
        metadata = metadata_dir / 'METADATA'
        with metadata.open('w', encoding='utf-8') as strm:
            strm.write('Version: 1.0\n')
        return 'CherryPy'

    def test_dist_name_found_as_any_case(self):
        # Ensure the metadata loads when queried with any case.
        pkg_name = self.pkg_with_mixed_case(self.site_dir)
        assert version(pkg_name) == '1.0'
        assert version(pkg_name.lower()) == '1.0'
        assert version(pkg_name.upper()) == '1.0'


class NonASCIITests(fixtures.OnSysPath, fixtures.SiteDir, unittest.TestCase):
    @staticmethod
    def pkg_with_non_ascii_description(site_dir):
        """
        Create minimal metadata for a package with non-ASCII in
        the description.
        """
        metadata_dir = site_dir / 'portend.dist-info'
        metadata_dir.mkdir()
        metadata = metadata_dir / 'METADATA'
        with metadata.open('w', encoding='utf-8') as fp:
            fp.write('Description: pôrˈtend')
        return 'portend'

    @staticmethod
    def pkg_with_non_ascii_description_egg_info(site_dir):
        """
        Create minimal metadata for an egg-info package with
        non-ASCII in the description.
        """
        metadata_dir = site_dir / 'portend.dist-info'
        metadata_dir.mkdir()
        metadata = metadata_dir / 'METADATA'
        with metadata.open('w', encoding='utf-8') as fp:
            fp.write(
                textwrap.dedent(
                    """
                Name: portend

                pôrˈtend
                """
                ).strip()
            )
        return 'portend'

    def test_metadata_loads(self):
        pkg_name = self.pkg_with_non_ascii_description(self.site_dir)
        meta = metadata(pkg_name)
        assert meta['Description'] == 'pôrˈtend'

    def test_metadata_loads_egg_info(self):
        pkg_name = self.pkg_with_non_ascii_description_egg_info(self.site_dir)
        meta = metadata(pkg_name)
        assert meta['Description'] == 'pôrˈtend'


class DiscoveryTests(fixtures.EggInfoPkg, fixtures.DistInfoPkg, unittest.TestCase):
    def test_package_discovery(self):
        dists = list(distributions())
        assert all(isinstance(dist, Distribution) for dist in dists)
        assert any(dist.metadata['Name'] == 'egginfo-pkg' for dist in dists)
        assert any(dist.metadata['Name'] == 'distinfo-pkg' for dist in dists)

    def test_invalid_usage(self):
        with self.assertRaises(ValueError):
            list(distributions(context='something', name='else'))


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
        # EntryPoints should be hashable.
        hash(self.ep)

    def test_json_dump(self):
        # json should not expect to be able to dump an EntryPoint.
        with self.assertRaises(Exception):
            with warnings.catch_warnings(record=True):
                json.dumps(self.ep)

    def test_module(self):
        assert self.ep.module == 'value'

    def test_attr(self):
        assert self.ep.attr is None

    def test_sortable(self):
        # EntryPoint objects are sortable, but result is undefined.
        sorted(
            [
                EntryPoint(name='b', value='val', group='group'),
                EntryPoint(name='a', value='val', group='group'),
            ]
        )


class FileSystem(
    fixtures.OnSysPath, fixtures.SiteDir, fixtures.FileBuilder, unittest.TestCase
):
    def test_unicode_dir_on_sys_path(self):
        # Ensure a Unicode subdirectory of a directory on sys.path
        # does not crash.
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
