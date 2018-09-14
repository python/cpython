import os
import re
import abc
import sys
import email
import itertools
import contextlib

from configparser import ConfigParser
from importlib import import_module
from pathlib import Path
from types import ModuleType
from zipfile import ZipFile


__all__ = [
    'Distribution',
    'PackageNotFoundError',
    'distribution',
    'entry_points',
    'resolve',
    'version',
    ]


class PackageNotFoundError(ModuleNotFoundError):
    """The package was not found."""


def distribution(package):
    """Get the ``Distribution`` instance for the given package.

    :param package: The module object for the package or the name of the
        package as a string.
    :return: A ``Distribution`` instance (or subclass thereof).
    """
    if isinstance(package, ModuleType):
        return Distribution.from_module(package)
    else:
        return Distribution.from_name(package)


def entry_points(name):
    """Return the entry points for the named distribution package.

    :param name: The name of the distribution package to query.
    :return: A ConfigParser instance where the sections and keys are taken
        from the entry_points.txt ini-style contents.
    """
    as_string = distribution(name).load_metadata('entry_points.txt')
    # 2018-09-10(barry): Should we provide any options here, or let the caller
    # send options to the underlying ConfigParser?   For now, YAGNI.
    config = ConfigParser()
    config.read_string(as_string)
    return config


def resolve(entry_point):
    """Resolve an entry point string into the named callable.

    :param entry_point: An entry point string of the form
        `path.to.module:callable`.
    :return: The actual callable object `path.to.module.callable`
    :raises ValueError: When `entry_point` doesn't have the proper format.
    """
    path, colon, name = entry_point.rpartition(':')
    if colon != ':':
        raise ValueError('Not an entry point: {}'.format(entry_point))
    module = import_module(path)
    return getattr(module, name)


def version(package):
    """Get the version string for the named package.

    :param package: The module object for the package or the name of the
        package as a string.
    :return: The version string for the package as defined in the package's
        "Version" metadata key.
    """
    return distribution(package).version


class Distribution:
    """A Python distribution package."""

    @abc.abstractmethod
    def load_metadata(self, name):
        """Attempt to load metadata given by the name.

        :param name: The name of the distribution package.
        :return: The metadata string if found, otherwise None.
        """

    @classmethod
    def from_name(cls, name):
        """Return the Distribution for the given package name.

        :param name: The name of the distribution package to search for.
        :return: The Distribution instance (or subclass thereof) for the named
            package, if found.
        :raises PackageNotFoundError: When the named package's distribution
            metadata cannot be found.
        """
        for resolver in cls._discover_resolvers():
            resolved = resolver(name)
            if resolved is not None:
                return resolved
        else:
            raise PackageNotFoundError(name)

    @staticmethod
    def _discover_resolvers():
        """Search the meta_path for resolvers."""
        declared = (
            getattr(finder, 'find_distribution', None)
            for finder in sys.meta_path
            )
        return filter(None, declared)

    @classmethod
    def from_module(cls, module):
        """Discover the Distribution package for a module."""
        return cls.from_name(cls.name_for_module(module))

    @classmethod
    def from_named_module(cls, mod_name):
        return cls.from_module(import_module(mod_name))

    @staticmethod
    def name_for_module(module):
        """Given an imported module, infer the distribution package name."""
        return getattr(module, '__dist_name__', module.__name__)

    @property
    def metadata(self):
        """Return the parsed metadata for this Distribution.

        The returned object will have keys that name the various bits of
        metadata.  See PEP 566 for details.
        """
        return email.message_from_string(
            self.load_metadata('METADATA') or self.load_metadata('PKG-INFO')
            )

    @property
    def version(self):
        """Return the 'Version' metadata for the distribution package."""
        return self.metadata['Version']


class MetadataPathFinder:
    """A degenerate finder for distribution packages on the file system.

    This finder supplies only a find_distribution() method for versions
    of Python that do not have a PathFinder find_distribution().
    """
    @staticmethod
    def find_spec(*args, **kwargs):
        return None

    @classmethod
    def find_distribution(cls, name):
        paths = cls._search_paths(name)
        dists = map(PathDistribution, paths)
        return next(dists, None)

    @classmethod
    def _search_paths(cls, name):
        """
        Find metadata directories in sys.path heuristically.
        """
        return itertools.chain.from_iterable(
            cls._search_path(path, name)
            for path in map(Path, sys.path)
            )

    @classmethod
    def _search_path(cls, root, name):
        if not root.is_dir():
            return ()
        return (
            item
            for item in root.iterdir()
            if item.is_dir()
            and str(item.name).startswith(name)
            and re.match(rf'{name}(-.*)?\.(dist|egg)-info', str(item.name))
            )


class PathDistribution(Distribution):
    def __init__(self, path):
        """Construct a distribution from a path to the metadata directory."""
        self._path = path

    def load_metadata(self, name):
        """Attempt to load metadata given by the name.

        :param name: The name of the distribution package.
        :return: The metadata string if found, otherwise None.
        """
        filename = os.path.join(self._path, name)
        with contextlib.suppress(FileNotFoundError):
            with open(filename, encoding='utf-8') as fp:
                return fp.read()
        return None


class WheelMetadataFinder:
    """A degenerate finder for distribution packages in wheels.

    This finder supplies only a find_distribution() method for versions
    of Python that do not have a PathFinder find_distribution().
    """
    @staticmethod
    def find_spec(*args, **kwargs):
        return None

    @classmethod
    def find_distribution(cls, name):
        try:
            module = import_module(name)
        except ImportError:
            return None
        archive = getattr(module.__loader__, 'archive', None)
        if archive is None:
            return None
        try:
            name, version = Path(archive).name.split('-')[0:2]
        except ValueError:
            return None
        dist_info = '{}-{}.dist-info'.format(name, version)
        with ZipFile(archive) as zf:
            # Since we're opening the zip file anyway to see if there's a
            # METADATA file in the .dist-info directory, we might as well
            # read it and cache it here.
            zi = zf.getinfo('{}/{}'.format(dist_info, 'METADATA'))
            metadata = zf.read(zi).decode('utf-8')
        return WheelDistribution(archive, dist_info, metadata)


class WheelDistribution(Distribution):
    def __init__(self, archive, dist_info, metadata):
        self._archive = archive
        self._dist_info = dist_info
        self._metadata = metadata

    def load_metadata(self, name):
        """Attempt to load metadata given by the name.

        :param name: The name of the distribution package.
        :return: The metadata string if found, otherwise None.
        """
        if name == 'METADATA':
            return self._metadata
        with ZipFile(self._archive) as zf:
            with contextlib.suppress(KeyError):
                as_bytes = zf.read('{}/{}'.format(self._dist_info, name))
                return as_bytes.decode('utf-8')
        return None


def _install():                                     # pragma: nocover
    """Install the appropriate sys.meta_path finder for the Python version."""
    sys.meta_path.append(MetadataPathFinder)
    sys.meta_path.append(WheelMetadataFinder)


_install()
