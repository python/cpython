import re
import sys
import zipfile
import itertools

from .api import Distribution
from .abc import DistributionFinder
from contextlib import suppress
from pathlib import Path


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


@install
class MetadataPathFinder(NullFinder):
    """A degenerate finder for distribution packages on the file system.

    This finder supplies only a find_distributions() method for versions
    of Python that do not have a PathFinder find_distributions().
    """
    search_template = r'{pattern}(-.*)?\.(dist|egg)-info'

    def find_distributions(self, name=None, path=None):
        """Return an iterable of all Distribution instances capable of
        loading the metadata for packages matching the name
        (or all names if not supplied) along the paths in the list
        of directories ``path`` (defaults to sys.path).
        """
        if path is None:
            path = sys.path
        pattern = '.*' if name is None else re.escape(name)
        found = self._search_paths(pattern, path)
        return map(PathDistribution, found)

    @classmethod
    def _search_paths(cls, pattern, paths):
        """
        Find metadata directories in paths heuristically.
        """
        return itertools.chain.from_iterable(
            cls._search_path(path, pattern)
            for path in map(cls._switch_path, paths)
            )

    @staticmethod
    def _switch_path(path):
        with suppress(Exception):
            return zipfile.Path(path)
        return Path(path)

    @classmethod
    def _predicate(cls, pattern, root, item):
        return re.match(pattern, str(item.name), flags=re.IGNORECASE)

    @classmethod
    def _search_path(cls, root, pattern):
        if not root.is_dir():
            return ()
        normalized = pattern.replace('-', '_')
        matcher = cls.search_template.format(pattern=normalized)
        return (item for item in root.iterdir()
                if cls._predicate(matcher, root, item))


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
