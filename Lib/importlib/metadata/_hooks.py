import sys

from .api import Distribution
from .abc import DistributionFinder
from contextlib import suppress


def install(cls):
    """Class decorator for installation on sys.meta_path."""
    sys.meta_path.append(cls())
    return cls


class NullFinder(DistributionFinder):
    """
    A "Finder" (aka "MetaClassFinder") that never finds any modules,
    but may find distributions.
    """
    @staticmethod
    def find_spec(*args, **kwargs):
        return None


class PathDistribution(Distribution):
    def __init__(self, path):
        """Construct a distribution from a path to the metadata directory."""
        self._path = path

    def read_text(self, filename):
        with suppress(FileNotFoundError, NotADirectoryError, KeyError):
            return self._path.joinpath(filename).read_text(encoding='utf-8')
    read_text.__doc__ = Distribution.read_text.__doc__

    def locate_file(self, path):
        return self._path.parent / path
