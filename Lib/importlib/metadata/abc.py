import abc

from importlib.abc import MetaPathFinder


class DistributionFinder(MetaPathFinder):
    """
    A MetaPathFinder capable of discovering installed distributions.
    """

    @abc.abstractmethod
    def find_distributions(self, name=None, path=None):
        """
        Return an iterable of all Distribution instances capable of
        loading the metadata for packages matching the name
        (or all names if not supplied) along the paths in the list
        of directories ``path`` (defaults to sys.path).
        """
