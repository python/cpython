import unittest

from importlib.metadata import (
    IDistribution,
    IPackagePath,
    PackageMetadata,
    distribution,
    files,
    metadata,
)

from . import fixtures


class ProtocolTests(fixtures.DistInfoPkg, unittest.TestCase):
    """
    The objects supplied to consumers satisfy the published protocols.
    """

    def test_distribution_matches_protocol(self):
        assert isinstance(distribution('distinfo-pkg'), IDistribution)

    def test_metadata_matches_protocol(self):
        assert isinstance(metadata('distinfo-pkg'), PackageMetadata)

    def test_package_path_matches_protocol(self):
        assert all(isinstance(file, IPackagePath) for file in files('distinfo-pkg'))
