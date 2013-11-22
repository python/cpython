import abc
import unittest


class FinderTests(metaclass=abc.ABCMeta):

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


class LoaderTests(metaclass=abc.ABCMeta):

    @abc.abstractmethod
    def test_module(self):
        """A module should load without issue.

        After the loader returns the module should be in sys.modules.

        Attributes to verify:

            * __file__
            * __loader__
            * __name__
            * No __path__

        """
        pass

    @abc.abstractmethod
    def test_package(self):
        """Loading a package should work.

        After the loader returns the module should be in sys.modules.

        Attributes to verify:

            * __name__
            * __file__
            * __package__
            * __path__
            * __loader__

        """
        pass

    @abc.abstractmethod
    def test_lacking_parent(self):
        """A loader should not be dependent on it's parent package being
        imported."""
        pass

    @abc.abstractmethod
    def test_state_after_failure(self):
        """If a module is already in sys.modules and a reload fails
        (e.g. a SyntaxError), the module should be in the state it was before
        the reload began."""
        pass

    @abc.abstractmethod
    def test_unloadable(self):
        """Test ImportError is raised when the loader is asked to load a module
        it can't."""
        pass
