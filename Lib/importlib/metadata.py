import io
import re
import abc
import csv
import sys
import email
import pathlib
import operator
import functools
import itertools
import collections

from configparser import ConfigParser
from contextlib import suppress
from importlib import import_module
from importlib.abc import MetaPathFinder
from itertools import starmap


__all__ = [
    'Distribution',
    'PackageNotFoundError',
    'distribution',
    'distributions',
    'entry_points',
    'files',
    'metadata',
    'requires',
    'version',
    ]


class PackageNotFoundError(ModuleNotFoundError):
    """The package was not found."""


class EntryPoint(collections.namedtuple('EntryPointBase', 'name value group')):
    """An entry point as defined by Python packaging conventions.

    See `the packaging docs on entry points
    <https://packaging.python.org/specifications/entry-points/>`_
    for more information.
    """

    pattern = re.compile(
        r'(?P<module>[\w.]+)\s*'
        r'(:\s*(?P<attr>[\w.]+))?\s*'
        r'(?P<extras>\[.*\])?\s*$'
        )
    """
    A regular expression describing the syntax for an entry point,
    which might look like:

        - module
        - package.module
        - package.module:attribute
        - package.module:object.attribute
        - package.module:attr [extra1, extra2]

    Other combinations are possible as well.

    The expression is lenient about whitespace around the ':',
    following the attr, and following any extras.
    """

    def load(self):
        """Load the entry point from its definition. If only a module
        is indicated by the value, return that module. Otherwise,
        return the named object.
        """
        match = self.pattern.match(self.value)
        module = import_module(match.group('module'))
        attrs = filter(None, (match.group('attr') or '').split('.'))
        return functools.reduce(getattr, attrs, module)

    @property
    def extras(self):
        match = self.pattern.match(self.value)
        return list(re.finditer(r'\w+', match.group('extras') or ''))

    @classmethod
    def _from_config(cls, config):
        return [
            cls(name, value, group)
            for group in config.sections()
            for name, value in config.items(group)
            ]

    @classmethod
    def _from_text(cls, text):
        config = ConfigParser(delimiters='=')
        # case sensitive: https://stackoverflow.com/q/1611799/812183
        config.optionxform = str
        try:
            config.read_string(text)
        except AttributeError:  # pragma: nocover
            # Python 2 has no read_string
            config.readfp(io.StringIO(text))
        return EntryPoint._from_config(config)

    def __iter__(self):
        """
        Supply iter so one may construct dicts of EntryPoints easily.
        """
        return iter((self.name, self))


class PackagePath(pathlib.PurePosixPath):
    """A reference to a path in a package"""

    def read_text(self, encoding='utf-8'):
        with self.locate().open(encoding=encoding) as stream:
            return stream.read()

    def read_binary(self):
        with self.locate().open('rb') as stream:
            return stream.read()

    def locate(self):
        """Return a path-like object for this path"""
        return self.dist.locate_file(self)


class FileHash:
    def __init__(self, spec):
        self.mode, _, self.value = spec.partition('=')

    def __repr__(self):
        return '<FileHash mode: {} value: {}>'.format(self.mode, self.value)


class Distribution:
    """A Python distribution package."""

    @abc.abstractmethod
    def read_text(self, filename):
        """Attempt to load metadata file given by the name.

        :param filename: The name of the file in the distribution info.
        :return: The text if found, otherwise None.
        """

    @abc.abstractmethod
    def locate_file(self, path):
        """
        Given a path to a file in this distribution, return a path
        to it.
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
            dists = resolver(name)
            dist = next(dists, None)
            if dist is not None:
                return dist
        else:
            raise PackageNotFoundError(name)

    @classmethod
    def discover(cls):
        """Return an iterable of Distribution objects for all packages.

        :return: Iterable of Distribution objects for all packages.
        """
        return itertools.chain.from_iterable(
            resolver()
            for resolver in cls._discover_resolvers()
            )

    @staticmethod
    def _discover_resolvers():
        """Search the meta_path for resolvers."""
        declared = (
            getattr(finder, 'find_distributions', None)
            for finder in sys.meta_path
            )
        return filter(None, declared)

    @property
    def metadata(self):
        """Return the parsed metadata for this Distribution.

        The returned object will have keys that name the various bits of
        metadata.  See PEP 566 for details.
        """
        text = (
            self.read_text('METADATA')
            or self.read_text('PKG-INFO')
            # This last clause is here to support old egg-info files.  Its
            # effect is to just end up using the PathDistribution's self._path
            # (which points to the egg-info file) attribute unchanged.
            or self.read_text('')
            )
        return email.message_from_string(text)

    @property
    def version(self):
        """Return the 'Version' metadata for the distribution package."""
        return self.metadata['Version']

    @property
    def entry_points(self):
        return EntryPoint._from_text(self.read_text('entry_points.txt'))

    @property
    def files(self):
        file_lines = self._read_files_distinfo() or self._read_files_egginfo()

        def make_file(name, hash=None, size_str=None):
            result = PackagePath(name)
            result.hash = FileHash(hash) if hash else None
            result.size = int(size_str) if size_str else None
            result.dist = self
            return result

        return file_lines and starmap(make_file, csv.reader(file_lines))

    def _read_files_distinfo(self):
        """
        Read the lines of RECORD
        """
        text = self.read_text('RECORD')
        return text and text.splitlines()

    def _read_files_egginfo(self):
        """
        SOURCES.txt might contain literal commas, so wrap each line
        in quotes.
        """
        text = self.read_text('SOURCES.txt')
        return text and map('"{}"'.format, text.splitlines())

    @property
    def requires(self):
        """Generated requirements specified for this Distribution"""
        return self._read_dist_info_reqs() or self._read_egg_info_reqs()

    def _read_dist_info_reqs(self):
        spec = self.metadata['Requires-Dist']
        return spec and filter(None, spec.splitlines())

    def _read_egg_info_reqs(self):
        source = self.read_text('requires.txt')
        return source and self._deps_from_requires_text(source)

    @classmethod
    def _deps_from_requires_text(cls, source):
        section_pairs = cls._read_sections(source.splitlines())
        sections = {
            section: list(map(operator.itemgetter('line'), results))
            for section, results in
            itertools.groupby(section_pairs, operator.itemgetter('section'))
            }
        return cls._convert_egg_info_reqs_to_simple_reqs(sections)

    @staticmethod
    def _read_sections(lines):
        section = None
        for line in filter(None, lines):
            section_match = re.match(r'\[(.*)\]$', line)
            if section_match:
                section = section_match.group(1)
                continue
            yield locals()

    @staticmethod
    def _convert_egg_info_reqs_to_simple_reqs(sections):
        """
        Historically, setuptools would solicit and store 'extra'
        requirements, including those with environment markers,
        in separate sections. More modern tools expect each
        dependency to be defined separately, with any relevant
        extras and environment markers attached directly to that
        requirement. This method converts the former to the
        latter. See _test_deps_from_requires_text for an example.
        """
        def make_condition(name):
            return name and 'extra == "{name}"'.format(name=name)

        def parse_condition(section):
            section = section or ''
            extra, sep, markers = section.partition(':')
            if extra and markers:
                markers = '({markers})'.format(markers=markers)
            conditions = list(filter(None, [markers, make_condition(extra)]))
            return '; ' + ' and '.join(conditions) if conditions else ''

        for section, deps in sections.items():
            for dep in deps:
                yield dep + parse_condition(section)


class DistributionFinder(MetaPathFinder):
    """
    A MetaPathFinder capable of discovering installed distributions.
    """

    @abc.abstractmethod
    def find_distributions(self, name=None, path=None):
        """
        Find distributions.

        Return an iterable of all Distribution instances capable of
        loading the metadata for packages matching the ``name``
        (or all names if not supplied) along the paths in the list
        of directories ``path`` (defaults to sys.path).
        """


class PathDistribution(Distribution):
    def __init__(self, path):
        """Construct a distribution from a path to the metadata directory."""
        self._path = path

    def read_text(self, filename):
        with suppress(FileNotFoundError, IsADirectoryError, KeyError,
                      NotADirectoryError, PermissionError):
            return self._path.joinpath(filename).read_text(encoding='utf-8')
    read_text.__doc__ = Distribution.read_text.__doc__

    def locate_file(self, path):
        return self._path.parent / path


def distribution(package):
    """Get the ``Distribution`` instance for the given package.

    :param package: The name of the package as a string.
    :return: A ``Distribution`` instance (or subclass thereof).
    """
    return Distribution.from_name(package)


def distributions():
    """Get all ``Distribution`` instances in the current environment.

    :return: An iterable of ``Distribution`` instances.
    """
    return Distribution.discover()


def metadata(package):
    """Get the metadata for the package.

    :param package: The name of the distribution package to query.
    :return: An email.Message containing the parsed metadata.
    """
    return Distribution.from_name(package).metadata


def version(package):
    """Get the version string for the named package.

    :param package: The name of the distribution package to query.
    :return: The version string for the package as defined in the package's
        "Version" metadata key.
    """
    return distribution(package).version


def entry_points():
    """Return EntryPoint objects for all installed packages.

    :return: EntryPoint objects for all installed packages.
    """
    eps = itertools.chain.from_iterable(
        dist.entry_points for dist in distributions())
    by_group = operator.attrgetter('group')
    ordered = sorted(eps, key=by_group)
    grouped = itertools.groupby(ordered, by_group)
    return {
        group: tuple(eps)
        for group, eps in grouped
        }


def files(package):
    return distribution(package).files


def requires(package):
    """
    Return a list of requirements for the indicated distribution.

    :return: An iterator of requirements, suitable for
    packaging.requirement.Requirement.
    """
    return distribution(package).requires
