# top-level.
# Package.
# module in pacakge.
# Package within a package.
# At least one tests with 'path'.
# Module that is not handled.

import unittest


class FinderTests(unittest.TestCase):

    """Basic tests for a finder to pass."""

    def test_module(self):
        # Test importing a top-level module.
        raise NotImplementedError

    def test_package(self):
        # Test importing a package.
        raise NotImplementedError

    def test_module_in_package(self):
        # Test importing a module contained within a package.
        # A value for 'path' should be used if for a meta_path finder.
        raise NotImplementedError

    def test_package_in_package(self):
        # Test importing a subpackage.
        # A value for 'path' should be used if for a meta_path finder.
        raise NotImplementedError

    def test_package_over_module(self):
        # Test that packages are chosen over modules.
        raise NotImplementedError

    def test_failure(self):
        # Test trying to find a module that cannot be handled.
        raise NotImplementedError
