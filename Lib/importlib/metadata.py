import os
import re
import abc
import csv
import sys
import email
import inspect
import pathlib
import zipfile
import operator
import warnings
import functools
import itertools
import posixpath
import collections.abc

from ._itertools import unique_everseen

from configparser import ConfigParser
from contextlib import suppress
from importlib import import_module
from importlib.abc import MetaPathFinder
from itertools import starmap
from typing import Any, List, Mapping, Optional, Protocol, TypeVar, Union


__all__ = [
    'Distribution',
    'DistributionFinder',
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

    def __str__(self):
        tmpl = "No package metadata was found for {self.name}"
        return tmpl.format(**locals())

    @property
    def name(self):
        (name,) = self.args
        return name


class EntryPoint(
        collections.namedtuple('EntryPointBase', 'name value group')):
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

    dist: Optional['Distribution'] = None

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
    def module(self):
        match = self.pattern.match(self.value)
        return match.group('module')

    @property
    def attr(self):
        match = self.pattern.match(self.value)
        return match.group('attr')

    @property
    def extras(self):
        match = self.pattern.match(self.value)
        return list(re.finditer(r'\w+', match.group('extras') or ''))

    @classmethod
    def _from_config(cls, config):
        return (
            cls(name, value, group)
            for group in config.sections()
            for name, value in config.items(group)
        )

    @classmethod
    def _from_text(cls, text):
        config = ConfigParser(delimiters='=')
        # case sensitive: https://stackoverflow.com/q/1611799/812183
        config.optionxform = str
        config.read_string(text)
        return cls._from_config(config)

    def _for(self, dist):
        self.dist = dist
        return self

    def __iter__(self):
        """
        Supply iter so one may construct dicts of EntryPoints by name.
        """
        msg = (
            "Construction of dict of EntryPoints is deprecated in "
            "favor of EntryPoints."
        )
        warnings.warn(msg, DeprecationWarning)
        return iter((self.name, self))

    def __reduce__(self):
        return (
            self.__class__,
            (self.name, self.value, self.group),
        )

    def matches(self, **params):
        attrs = (getattr(self, param) for param in params)
        return all(map(operator.eq, params.values(), attrs))


class EntryPoints(tuple):
    """
    An immutable collection of selectable EntryPoint objects.
    """

    __slots__ = ()

    def __getitem__(self, name):  # -> EntryPoint:
        try:
            return next(iter(self.select(name=name)))
        except StopIteration:
            raise KeyError(name)

    def select(self, **params):
        return EntryPoints(ep for ep in self if ep.matches(**params))

    @property
    def names(self):
        return set(ep.name for ep in self)

    @property
    def groups(self):
        """
        For coverage while SelectableGroups is present.
        >>> EntryPoints().groups
        set()
        """
        return set(ep.group for ep in self)

    @classmethod
    def _from_text_for(cls, text, dist):
        return cls(ep._for(dist) for ep in EntryPoint._from_text(text))


def flake8_bypass(func):
    is_flake8 = any('flake8' in str(frame.filename) for frame in inspect.stack()[:5])
    return func if not is_flake8 else lambda: None


class Deprecated:
    """
    Compatibility add-in for mapping to indicate that
    mapping behavior is deprecated.

    >>> recwarn = getfixture('recwarn')
    >>> class DeprecatedDict(Deprecated, dict): pass
    >>> dd = DeprecatedDict(foo='bar')
    >>> dd.get('baz', None)
    >>> dd['foo']
    'bar'
    >>> list(dd)
    ['foo']
    >>> list(dd.keys())
    ['foo']
    >>> 'foo' in dd
    True
    >>> list(dd.values())
    ['bar']
    >>> len(recwarn)
    1
    """

    _warn = functools.partial(
        warnings.warn,
        "SelectableGroups dict interface is deprecated. Use select.",
        DeprecationWarning,
        stacklevel=2,
    )

    def __getitem__(self, name):
        self._warn()
        return super().__getitem__(name)

    def get(self, name, default=None):
        flake8_bypass(self._warn)()
        return super().get(name, default)

    def __iter__(self):
        self._warn()
        return super().__iter__()

    def __contains__(self, *args):
        self._warn()
        return super().__contains__(*args)

    def keys(self):
        self._warn()
        return super().keys()

    def values(self):
        self._warn()
        return super().values()


class SelectableGroups(dict):
    """
    A backward- and forward-compatible result from
    entry_points that fully implements the dict interface.
    """

    @classmethod
    def load(cls, eps):
        by_group = operator.attrgetter('group')
        ordered = sorted(eps, key=by_group)
        grouped = itertools.groupby(ordered, by_group)
        return cls((group, EntryPoints(eps)) for group, eps in grouped)

    @property
    def _all(self):
        """
        Reconstruct a list of all entrypoints from the groups.
        """
        return EntryPoints(itertools.chain.from_iterable(self.values()))

    @property
    def groups(self):
        return self._all.groups

    @property
    def names(self):
        """
        for coverage:
        >>> SelectableGroups().names
        set()
        """
        return self._all.names

    def select(self, **params):
        if not params:
            return self
        return self._all.select(**params)


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


_T = TypeVar("_T")


class PackageMetadata(Protocol):
    def __len__(self) -> int:
        ...  # pragma: no cover

    def __contains__(self, item: str) -> bool:
        ...  # pragma: no cover

    def __getitem__(self, key: str) -> str:
        ...  # pragma: no cover

    def get_all(self, name: str, failobj: _T = ...) -> Union[List[Any], _T]:
        """
        Return all values associated with a possibly multi-valued key.
        """


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
            dists = resolver(DistributionFinder.Context(name=name))
            dist = next(iter(dists), None)
            if dist is not None:
                return dist
        else:
            raise PackageNotFoundError(name)

    @classmethod
    def discover(cls, **kwargs):
        """Return an iterable of Distribution objects for all packages.

        Pass a ``context`` or pass keyword arguments for constructing
        a context.

        :context: A ``DistributionFinder.Context`` object.
        :return: Iterable of Distribution objects for all packages.
        """
        context = kwargs.pop('context', None)
        if context and kwargs:
            raise ValueError("cannot accept context and kwargs")
        context = context or DistributionFinder.Context(**kwargs)
        return itertools.chain.from_iterable(
            resolver(context) for resolver in cls._discover_resolvers()
        )

    @staticmethod
    def at(path):
        """Return a Distribution for the indicated metadata path

        :param path: a string or path-like object
        :return: a concrete Distribution instance for the path
        """
        return PathDistribution(pathlib.Path(path))

    @staticmethod
    def _discover_resolvers():
        """Search the meta_path for resolvers."""
        declared = (
            getattr(finder, 'find_distributions', None) for finder in sys.meta_path
        )
        return filter(None, declared)

    @classmethod
    def _local(cls, root='.'):
        from pep517 import build, meta

        system = build.compat_system(root)
        builder = functools.partial(
            meta.build,
            source_dir=root,
            system=system,
        )
        return PathDistribution(zipfile.Path(meta.build_as_zip(builder)))

    @property
    def metadata(self) -> PackageMetadata:
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
    def name(self):
        """Return the 'Name' metadata for the distribution package."""
        return self.metadata['Name']

    @property
    def version(self):
        """Return the 'Version' metadata for the distribution package."""
        return self.metadata['Version']

    @property
    def entry_points(self):
        return EntryPoints._from_text_for(self.read_text('entry_points.txt'), self)

    @property
    def files(self):
        """Files in this distribution.

        :return: List of PackagePath for this distribution or None

        Result is `None` if the metadata file that enumerates files
        (i.e. RECORD for dist-info or SOURCES.txt for egg-info) is
        missing.
        Result may be empty if the metadata exists but is empty.
        """
        file_lines = self._read_files_distinfo() or self._read_files_egginfo()

        def make_file(name, hash=None, size_str=None):
            result = PackagePath(name)
            result.hash = FileHash(hash) if hash else None
            result.size = int(size_str) if size_str else None
            result.dist = self
            return result

        return file_lines and list(starmap(make_file, csv.reader(file_lines)))

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
        reqs = self._read_dist_info_reqs() or self._read_egg_info_reqs()
        return reqs and list(reqs)

    def _read_dist_info_reqs(self):
        return self.metadata.get_all('Requires-Dist')

    def _read_egg_info_reqs(self):
        source = self.read_text('requires.txt')
        return source and self._deps_from_requires_text(source)

    @classmethod
    def _deps_from_requires_text(cls, source):
        section_pairs = cls._read_sections(source.splitlines())
        sections = {
            section: list(map(operator.itemgetter('line'), results))
            for section, results in itertools.groupby(
                section_pairs, operator.itemgetter('section')
            )
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

    class Context:
        """
        Keyword arguments presented by the caller to
        ``distributions()`` or ``Distribution.discover()``
        to narrow the scope of a search for distributions
        in all DistributionFinders.

        Each DistributionFinder may expect any parameters
        and should attempt to honor the canonical
        parameters defined below when appropriate.
        """

        name = None
        """
        Specific name for which a distribution finder should match.
        A name of ``None`` matches all distributions.
        """

        def __init__(self, **kwargs):
            vars(self).update(kwargs)

        @property
        def path(self):
            """
            The path that a distribution finder should search.

            Typically refers to Python package paths and defaults
            to ``sys.path``.
            """
            return vars(self).get('path', sys.path)

    @abc.abstractmethod
    def find_distributions(self, context=Context()):
        """
        Find distributions.

        Return an iterable of all Distribution instances capable of
        loading the metadata for packages matching the ``context``,
        a DistributionFinder.Context instance.
        """


class FastPath:
    """
    Micro-optimized class for searching a path for
    children.
    """

    def __init__(self, root):
        self.root = root
        self.base = os.path.basename(self.root).lower()

    def joinpath(self, child):
        return pathlib.Path(self.root, child)

    def children(self):
        with suppress(Exception):
            return os.listdir(self.root or '')
        with suppress(Exception):
            return self.zip_children()
        return []

    def zip_children(self):
        zip_path = zipfile.Path(self.root)
        names = zip_path.root.namelist()
        self.joinpath = zip_path.joinpath

        return dict.fromkeys(child.split(posixpath.sep, 1)[0] for child in names)

    def search(self, name):
        return (
            self.joinpath(child)
            for child in self.children()
            if name.matches(child, self.base)
        )


class Prepared:
    """
    A prepared search for metadata on a possibly-named package.
    """

    normalized = None
    suffixes = 'dist-info', 'egg-info'
    exact_matches = [''][:0]
    egg_prefix = ''
    versionless_egg_name = ''

    def __init__(self, name):
        self.name = name
        if name is None:
            return
        self.normalized = self.normalize(name)
        self.exact_matches = [
            self.normalized + '.' + suffix for suffix in self.suffixes
        ]
        legacy_normalized = self.legacy_normalize(self.name)
        self.egg_prefix = legacy_normalized + '-'
        self.versionless_egg_name = legacy_normalized + '.egg'

    @staticmethod
    def normalize(name):
        """
        PEP 503 normalization plus dashes as underscores.
        """
        return re.sub(r"[-_.]+", "-", name).lower().replace('-', '_')

    @staticmethod
    def legacy_normalize(name):
        """
        Normalize the package name as found in the convention in
        older packaging tools versions and specs.
        """
        return name.lower().replace('-', '_')

    def matches(self, cand, base):
        low = cand.lower()
        # rpartition is faster than splitext and suitable for this purpose.
        pre, _, ext = low.rpartition('.')
        name, _, rest = pre.partition('-')
        return (
            low in self.exact_matches
            or ext in self.suffixes
            and (not self.normalized or name.replace('.', '_') == self.normalized)
            # legacy case:
            or self.is_egg(base)
            and low == 'egg-info'
        )

    def is_egg(self, base):
        return (
            base == self.versionless_egg_name
            or base.startswith(self.egg_prefix)
            and base.endswith('.egg')
        )


class MetadataPathFinder(DistributionFinder):
    @classmethod
    def find_distributions(cls, context=DistributionFinder.Context()):
        """
        Find distributions.

        Return an iterable of all Distribution instances capable of
        loading the metadata for packages matching ``context.name``
        (or all names if ``None`` indicated) along the paths in the list
        of directories ``context.path``.
        """
        found = cls._search_paths(context.name, context.path)
        return map(PathDistribution, found)

    @classmethod
    def _search_paths(cls, name, paths):
        """Find metadata directories in paths heuristically."""
        prepared = Prepared(name)
        return itertools.chain.from_iterable(
            path.search(prepared) for path in map(FastPath, paths)
        )


class PathDistribution(Distribution):
    def __init__(self, path):
        """Construct a distribution from a path to the metadata directory.

        :param path: A pathlib.Path or similar object supporting
                     .joinpath(), __div__, .parent, and .read_text().
        """
        self._path = path

    def read_text(self, filename):
        with suppress(
            FileNotFoundError,
            IsADirectoryError,
            KeyError,
            NotADirectoryError,
            PermissionError,
        ):
            return self._path.joinpath(filename).read_text(encoding='utf-8')

    read_text.__doc__ = Distribution.read_text.__doc__

    def locate_file(self, path):
        return self._path.parent / path


def distribution(distribution_name):
    """Get the ``Distribution`` instance for the named package.

    :param distribution_name: The name of the distribution package as a string.
    :return: A ``Distribution`` instance (or subclass thereof).
    """
    return Distribution.from_name(distribution_name)


def distributions(**kwargs):
    """Get all ``Distribution`` instances in the current environment.

    :return: An iterable of ``Distribution`` instances.
    """
    return Distribution.discover(**kwargs)


def metadata(distribution_name) -> PackageMetadata:
    """Get the metadata for the named package.

    :param distribution_name: The name of the distribution package to query.
    :return: A PackageMetadata containing the parsed metadata.
    """
    return Distribution.from_name(distribution_name).metadata


def version(distribution_name):
    """Get the version string for the named package.

    :param distribution_name: The name of the distribution package to query.
    :return: The version string for the package as defined in the package's
        "Version" metadata key.
    """
    return distribution(distribution_name).version


def entry_points(**params) -> Union[EntryPoints, SelectableGroups]:
    """Return EntryPoint objects for all installed packages.

    Pass selection parameters (group or name) to filter the
    result to entry points matching those properties (see
    EntryPoints.select()).

    For compatibility, returns ``SelectableGroups`` object unless
    selection parameters are supplied. In the future, this function
    will return ``EntryPoints`` instead of ``SelectableGroups``
    even when no selection parameters are supplied.

    For maximum future compatibility, pass selection parameters
    or invoke ``.select`` with parameters on the result.

    :return: EntryPoints or SelectableGroups for all installed packages.
    """
    unique = functools.partial(unique_everseen, key=operator.attrgetter('name'))
    eps = itertools.chain.from_iterable(
        dist.entry_points for dist in unique(distributions())
    )
    return SelectableGroups.load(eps).select(**params)


def files(distribution_name):
    """Return a list of files for the named package.

    :param distribution_name: The name of the distribution package to query.
    :return: List of files composing the distribution.
    """
    return distribution(distribution_name).files


def requires(distribution_name):
    """
    Return a list of requirements for the named package.

    :return: An iterator of requirements, suitable for
    packaging.requirement.Requirement.
    """
    return distribution(distribution_name).requires


def packages_distributions() -> Mapping[str, List[str]]:
    """
    Return a mapping of top-level packages to their
    distributions.

    >>> pkgs = packages_distributions()
    >>> all(isinstance(dist, collections.abc.Sequence) for dist in pkgs.values())
    True
    """
    pkg_to_dist = collections.defaultdict(list)
    for dist in distributions():
        for pkg in (dist.read_text('top_level.txt') or '').split():
            pkg_to_dist[pkg].append(dist.metadata['Name'])
    return dict(pkg_to_dist)
