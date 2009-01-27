import abc
import unittest


class LoaderTests(unittest.TestCase, metaclass=abc.ABCMeta):

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

            * __file__
            * __loader__
            * __name__
            * __path__

        """
        pass

    @abc.abstractmethod
    def test_lacking_parent(self):
        """A loader should not be dependent on it's parent package being
        imported."""
        pass

    @abc.abstractmethod
    def test_module_reuse(self):
        """If a module is already in sys.modules, it should be reused."""
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
