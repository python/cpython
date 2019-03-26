# coding: utf-8
from __future__ import unicode_literals

import re
import textwrap
import unittest
import importlib
import importlib.metadata

from . import fixtures
from importlib.metadata import _hooks


class BasicTests(fixtures.DistInfoPkg, unittest.TestCase):
    version_pattern = r'\d+\.\d+(\.\d)?'

    def test_retrieves_version_of_self(self):
        dist = importlib.metadata.Distribution.from_name('distinfo-pkg')
        assert isinstance(dist.version, str)
        assert re.match(self.version_pattern, dist.version)

    def test_for_name_does_not_exist(self):
        with self.assertRaises(importlib.metadata.PackageNotFoundError):
            importlib.metadata.Distribution.from_name('does-not-exist')

    def test_new_style_classes(self):
        self.assertIsInstance(importlib.metadata.Distribution, type)
        self.assertIsInstance(_hooks.MetadataPathFinder, type)
        self.assertIsInstance(_hooks.WheelMetadataFinder, type)
        self.assertIsInstance(_hooks.WheelDistribution, type)


class ImportTests(fixtures.DistInfoPkg, unittest.TestCase):
    def test_import_nonexistent_module(self):
        # Ensure that the MetadataPathFinder does not crash an import of a
        # non-existant module.
        with self.assertRaises(ImportError):
            importlib.import_module('does_not_exist')

    def test_resolve(self):
        entries = dict(importlib.metadata.entry_points()['entries'])
        ep = entries['main']
        self.assertEqual(ep.load().__name__, "main")

    def test_resolve_without_attr(self):
        ep = importlib.metadata.api.EntryPoint(
            name='ep',
            value='importlib.metadata.api',
            group='grp',
            )
        assert ep.load() is importlib.metadata.api


class NameNormalizationTests(fixtures.SiteDir, unittest.TestCase):
    @staticmethod
    def pkg_with_dashes(site_dir):
        """
        Create minimal metadata for a package with dashes
        in the name (and thus underscores in the filename).
        """
        metadata_dir = site_dir / 'my_pkg.dist-info'
        metadata_dir.mkdir()
        metadata = metadata_dir / 'METADATA'
        with metadata.open('w') as strm:
            strm.write('Version: 1.0\n')
        return 'my-pkg'

    def test_dashes_in_dist_name_found_as_underscores(self):
        """
        For a package with a dash in the name, the dist-info metadata
        uses underscores in the name. Ensure the metadata loads.
        """
        pkg_name = self.pkg_with_dashes(self.site_dir)
        assert importlib.metadata.version(pkg_name) == '1.0'

    @staticmethod
    def pkg_with_mixed_case(site_dir):
        """
        Create minimal metadata for a package with mixed case
        in the name.
        """
        metadata_dir = site_dir / 'CherryPy.dist-info'
        metadata_dir.mkdir()
        metadata = metadata_dir / 'METADATA'
        with metadata.open('w') as strm:
            strm.write('Version: 1.0\n')
        return 'CherryPy'

    def test_dist_name_found_as_any_case(self):
        """
        Ensure the metadata loads when queried with any case.
        """
        pkg_name = self.pkg_with_mixed_case(self.site_dir)
        assert importlib.metadata.version(pkg_name) == '1.0'
        assert importlib.metadata.version(pkg_name.lower()) == '1.0'
        assert importlib.metadata.version(pkg_name.upper()) == '1.0'


class NonASCIITests(fixtures.SiteDir, unittest.TestCase):
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
            fp.write('Description: pôrˈtend\n')
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
            fp.write(textwrap.dedent("""
                Name: portend

                pôrˈtend
                """).lstrip())
        return 'portend'

    def test_metadata_loads(self):
        pkg_name = self.pkg_with_non_ascii_description(self.site_dir)
        meta = importlib.metadata.metadata(pkg_name)
        assert meta['Description'] == 'pôrˈtend'

    def test_metadata_loads_egg_info(self):
        pkg_name = self.pkg_with_non_ascii_description_egg_info(self.site_dir)
        meta = importlib.metadata.metadata(pkg_name)
        assert meta.get_payload() == 'pôrˈtend\n'


class DiscoveryTests(fixtures.EggInfoPkg, fixtures.DistInfoPkg,
                     unittest.TestCase):

    def test_package_discovery(self):
        dists = list(importlib.metadata.api.distributions())
        assert all(
            isinstance(dist, importlib.metadata.Distribution)
            for dist in dists
            )
        assert any(
            dist.metadata['Name'] == 'egginfo-pkg'
            for dist in dists
            )
        assert any(
            dist.metadata['Name'] == 'distinfo-pkg'
            for dist in dists
            )
