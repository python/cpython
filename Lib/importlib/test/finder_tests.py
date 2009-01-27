import abc
import unittest


class FinderTests(unittest.TestCase, metaclass=abc.ABCMeta):

    """Basic tests for a finder to pass."""

    @abc.abstractmethod
    def test_module(self):
        # Test importing a top-level module.
        pass

    @abc.abstractmethod
    def test_package(self):
        # Test importing a package.
        pass

    @abc.abstractmethod
    def test_module_in_package(self):
        # Test importing a module contained within a package.
        # A value for 'path' should be used if for a meta_path finder.
        pass

    @abc.abstractmethod
    def test_package_in_package(self):
        # Test importing a subpackage.
        # A value for 'path' should be used if for a meta_path finder.
        pass

    @abc.abstractmethod
    def test_package_over_module(self):
        # Test that packages are chosen over modules.
        pass

    @abc.abstractmethod
    def test_failure(self):
        # Test trying to find a module that cannot be handled.
        pass
