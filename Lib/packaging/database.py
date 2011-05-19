"""PEP 376 implementation."""

import io
import os
import re
import csv
import sys
import zipimport
from hashlib import md5
from packaging import logger
from packaging.errors import PackagingError
from packaging.version import suggest_normalized_version, VersionPredicate
from packaging.metadata import Metadata


__all__ = [
    'Distribution', 'EggInfoDistribution', 'distinfo_dirname',
    'get_distributions', 'get_distribution', 'get_file_users',
    'provides_distribution', 'obsoletes_distribution',
    'enable_cache', 'disable_cache', 'clear_cache',
]


# TODO update docs

DIST_FILES = ('INSTALLER', 'METADATA', 'RECORD', 'REQUESTED', 'RESOURCES')

# Cache
_cache_name = {}  # maps names to Distribution instances
_cache_name_egg = {}  # maps names to EggInfoDistribution instances
_cache_path = {}  # maps paths to Distribution instances
_cache_path_egg = {}  # maps paths to EggInfoDistribution instances
_cache_generated = False  # indicates if .dist-info distributions are cached
_cache_generated_egg = False  # indicates if .dist-info and .egg are cached
_cache_enabled = True


def enable_cache():
    """
    Enables the internal cache.

    Note that this function will not clear the cache in any case, for that
    functionality see :func:`clear_cache`.
    """
    global _cache_enabled

    _cache_enabled = True


def disable_cache():
    """
    Disables the internal cache.

    Note that this function will not clear the cache in any case, for that
    functionality see :func:`clear_cache`.
    """
    global _cache_enabled

    _cache_enabled = False


def clear_cache():
    """ Clears the internal cache. """
    global _cache_name, _cache_name_egg, _cache_path, _cache_path_egg, \
        _cache_generated, _cache_generated_egg

    _cache_name = {}
    _cache_name_egg = {}
    _cache_path = {}
    _cache_path_egg = {}
    _cache_generated = False
    _cache_generated_egg = False


def _yield_distributions(include_dist, include_egg, paths=sys.path):
    """
    Yield .dist-info and .egg(-info) distributions, based on the arguments

    :parameter include_dist: yield .dist-info distributions
    :parameter include_egg: yield .egg(-info) distributions
    """
    for path in paths:
        realpath = os.path.realpath(path)
        if not os.path.isdir(realpath):
            continue
        for dir in os.listdir(realpath):
            dist_path = os.path.join(realpath, dir)
            if include_dist and dir.endswith('.dist-info'):
                yield Distribution(dist_path)
            elif include_egg and (dir.endswith('.egg-info') or
                                  dir.endswith('.egg')):
                yield EggInfoDistribution(dist_path)


def _generate_cache(use_egg_info=False, paths=sys.path):
    global _cache_generated, _cache_generated_egg

    if _cache_generated_egg or (_cache_generated and not use_egg_info):
        return
    else:
        gen_dist = not _cache_generated
        gen_egg = use_egg_info

        for dist in _yield_distributions(gen_dist, gen_egg, paths):
            if isinstance(dist, Distribution):
                _cache_path[dist.path] = dist
                if not dist.name in _cache_name:
                    _cache_name[dist.name] = []
                _cache_name[dist.name].append(dist)
            else:
                _cache_path_egg[dist.path] = dist
                if not dist.name in _cache_name_egg:
                    _cache_name_egg[dist.name] = []
                _cache_name_egg[dist.name].append(dist)

        if gen_dist:
            _cache_generated = True
        if gen_egg:
            _cache_generated_egg = True


class Distribution:
    """Created with the *path* of the ``.dist-info`` directory provided to the
    constructor. It reads the metadata contained in ``METADATA`` when it is
    instantiated."""

    name = ''
    """The name of the distribution."""

    version = ''
    """The version of the distribution."""

    metadata = None
    """A :class:`packaging.metadata.Metadata` instance loaded with
    the distribution's ``METADATA`` file."""

    requested = False
    """A boolean that indicates whether the ``REQUESTED`` metadata file is
    present (in other words, whether the package was installed by user
    request or it was installed as a dependency)."""

    def __init__(self, path):
        if _cache_enabled and path in _cache_path:
            self.metadata = _cache_path[path].metadata
        else:
            metadata_path = os.path.join(path, 'METADATA')
            self.metadata = Metadata(path=metadata_path)

        self.name = self.metadata['Name']
        self.version = self.metadata['Version']
        self.path = path

        if _cache_enabled and not path in _cache_path:
            _cache_path[path] = self

    def __repr__(self):
        return '<Distribution %r %s at %r>' % (
            self.name, self.version, self.path)

    def _get_records(self, local=False):
        with self.get_distinfo_file('RECORD') as record:
            record_reader = csv.reader(record, delimiter=',')
            # XXX needs an explaining comment
            for row in record_reader:
                path, checksum, size = (row[:] +
                                        [None for i in range(len(row), 3)])
                if local:
                    path = path.replace('/', os.sep)
                    path = os.path.join(sys.prefix, path)
                yield path, checksum, size

    def get_resource_path(self, relative_path):
        with self.get_distinfo_file('RESOURCES') as resources_file:
            resources_reader = csv.reader(resources_file, delimiter=',')
            for relative, destination in resources_reader:
                if relative == relative_path:
                    return destination
        raise KeyError(
            'no resource file with relative path %r is installed' %
            relative_path)

    def list_installed_files(self, local=False):
        """
        Iterates over the ``RECORD`` entries and returns a tuple
        ``(path, md5, size)`` for each line. If *local* is ``True``,
        the returned path is transformed into a local absolute path.
        Otherwise the raw value from RECORD is returned.

        A local absolute path is an absolute path in which occurrences of
        ``'/'`` have been replaced by the system separator given by ``os.sep``.

        :parameter local: flag to say if the path should be returned a local
                          absolute path

        :type local: boolean
        :returns: iterator of (path, md5, size)
        """
        return self._get_records(local)

    def uses(self, path):
        """
        Returns ``True`` if path is listed in ``RECORD``. *path* can be a local
        absolute path or a relative ``'/'``-separated path.

        :rtype: boolean
        """
        for p, checksum, size in self._get_records():
            local_absolute = os.path.join(sys.prefix, p)
            if path == p or path == local_absolute:
                return True
        return False

    def get_distinfo_file(self, path, binary=False):
        """
        Returns a file located under the ``.dist-info`` directory. Returns a
        ``file`` instance for the file pointed by *path*.

        :parameter path: a ``'/'``-separated path relative to the
                         ``.dist-info`` directory or an absolute path;
                         If *path* is an absolute path and doesn't start
                         with the ``.dist-info`` directory path,
                         a :class:`PackagingError` is raised
        :type path: string
        :parameter binary: If *binary* is ``True``, opens the file in read-only
                           binary mode (``rb``), otherwise opens it in
                           read-only mode (``r``).
        :rtype: file object
        """
        open_flags = 'r'
        if binary:
            open_flags += 'b'

        # Check if it is an absolute path  # XXX use relpath, add tests
        if path.find(os.sep) >= 0:
            # it's an absolute path?
            distinfo_dirname, path = path.split(os.sep)[-2:]
            if distinfo_dirname != self.path.split(os.sep)[-1]:
                raise PackagingError(
                    'dist-info file %r does not belong to the %r %s '
                    'distribution' % (path, self.name, self.version))

        # The file must be relative
        if path not in DIST_FILES:
            raise PackagingError('invalid path for a dist-info file: %r' %
                                 path)

        path = os.path.join(self.path, path)
        return open(path, open_flags)

    def list_distinfo_files(self, local=False):
        """
        Iterates over the ``RECORD`` entries and returns paths for each line if
        the path is pointing to a file located in the ``.dist-info`` directory
        or one of its subdirectories.

        :parameter local: If *local* is ``True``, each returned path is
                          transformed into a local absolute path. Otherwise the
                          raw value from ``RECORD`` is returned.
        :type local: boolean
        :returns: iterator of paths
        """
        for path, checksum, size in self._get_records(local):
            yield path

    def __eq__(self, other):
        return isinstance(other, Distribution) and self.path == other.path

    # See http://docs.python.org/reference/datamodel#object.__hash__
    __hash__ = object.__hash__


class EggInfoDistribution:
    """Created with the *path* of the ``.egg-info`` directory or file provided
    to the constructor. It reads the metadata contained in the file itself, or
    if the given path happens to be a directory, the metadata is read from the
    file ``PKG-INFO`` under that directory."""

    name = ''
    """The name of the distribution."""

    version = ''
    """The version of the distribution."""

    metadata = None
    """A :class:`packaging.metadata.Metadata` instance loaded with
    the distribution's ``METADATA`` file."""

    _REQUIREMENT = re.compile(
        r'(?P<name>[-A-Za-z0-9_.]+)\s*'
        r'(?P<first>(?:<|<=|!=|==|>=|>)[-A-Za-z0-9_.]+)?\s*'
        r'(?P<rest>(?:\s*,\s*(?:<|<=|!=|==|>=|>)[-A-Za-z0-9_.]+)*)\s*'
        r'(?P<extras>\[.*\])?')

    def __init__(self, path):
        self.path = path
        if _cache_enabled and path in _cache_path_egg:
            self.metadata = _cache_path_egg[path].metadata
            self.name = self.metadata['Name']
            self.version = self.metadata['Version']
            return

        # reused from Distribute's pkg_resources
        def yield_lines(strs):
            """Yield non-empty/non-comment lines of a ``basestring``
            or sequence"""
            if isinstance(strs, str):
                for s in strs.splitlines():
                    s = s.strip()
                    # skip blank lines/comments
                    if s and not s.startswith('#'):
                        yield s
            else:
                for ss in strs:
                    for s in yield_lines(ss):
                        yield s

        requires = None

        if path.endswith('.egg'):
            if os.path.isdir(path):
                meta_path = os.path.join(path, 'EGG-INFO', 'PKG-INFO')
                self.metadata = Metadata(path=meta_path)
                try:
                    req_path = os.path.join(path, 'EGG-INFO', 'requires.txt')
                    with open(req_path, 'r') as fp:
                        requires = fp.read()
                except IOError:
                    requires = None
            else:
                # FIXME handle the case where zipfile is not available
                zipf = zipimport.zipimporter(path)
                fileobj = io.StringIO(
                    zipf.get_data('EGG-INFO/PKG-INFO').decode('utf8'))
                self.metadata = Metadata(fileobj=fileobj)
                try:
                    requires = zipf.get_data('EGG-INFO/requires.txt')
                except IOError:
                    requires = None
            self.name = self.metadata['Name']
            self.version = self.metadata['Version']

        elif path.endswith('.egg-info'):
            if os.path.isdir(path):
                path = os.path.join(path, 'PKG-INFO')
                try:
                    with open(os.path.join(path, 'requires.txt'), 'r') as fp:
                        requires = fp.read()
                except IOError:
                    requires = None
            self.metadata = Metadata(path=path)
            self.name = self.metadata['name']
            self.version = self.metadata['Version']

        else:
            raise ValueError('path must end with .egg-info or .egg, got %r' %
                             path)

        if requires is not None:
            if self.metadata['Metadata-Version'] == '1.1':
                # we can't have 1.1 metadata *and* Setuptools requires
                for field in ('Obsoletes', 'Requires', 'Provides'):
                    del self.metadata[field]

        reqs = []

        if requires is not None:
            for line in yield_lines(requires):
                if line.startswith('['):
                    logger.warning(
                        'extensions in requires.txt are not supported '
                        '(used by %r %s)', self.name, self.version)
                    break
                else:
                    match = self._REQUIREMENT.match(line.strip())
                    if not match:
                        # this happens when we encounter extras; since they
                        # are written at the end of the file we just exit
                        break
                    else:
                        if match.group('extras'):
                            msg = ('extra requirements are not supported '
                                   '(used by %r %s)', self.name, self.version)
                            logger.warning(msg, self.name)
                        name = match.group('name')
                        version = None
                        if match.group('first'):
                            version = match.group('first')
                            if match.group('rest'):
                                version += match.group('rest')
                            version = version.replace(' ', '')  # trim spaces
                        if version is None:
                            reqs.append(name)
                        else:
                            reqs.append('%s (%s)' % (name, version))

            if len(reqs) > 0:
                self.metadata['Requires-Dist'] += reqs

        if _cache_enabled:
            _cache_path_egg[self.path] = self

    def __repr__(self):
        return '<EggInfoDistribution %r %s at %r>' % (
            self.name, self.version, self.path)

    def list_installed_files(self, local=False):

        def _md5(path):
            with open(path, 'rb') as f:
                content = f.read()
            return md5(content).hexdigest()

        def _size(path):
            return os.stat(path).st_size

        path = self.path
        if local:
            path = path.replace('/', os.sep)

        # XXX What about scripts and data files ?
        if os.path.isfile(path):
            return [(path, _md5(path), _size(path))]
        else:
            files = []
            for root, dir, files_ in os.walk(path):
                for item in files_:
                    item = os.path.join(root, item)
                    files.append((item, _md5(item), _size(item)))
            return files

        return []

    def uses(self, path):
        return False

    def __eq__(self, other):
        return (isinstance(other, EggInfoDistribution) and
                self.path == other.path)

    # See http://docs.python.org/reference/datamodel#object.__hash__
    __hash__ = object.__hash__


def distinfo_dirname(name, version):
    """
    The *name* and *version* parameters are converted into their
    filename-escaped form, i.e. any ``'-'`` characters are replaced
    with ``'_'`` other than the one in ``'dist-info'`` and the one
    separating the name from the version number.

    :parameter name: is converted to a standard distribution name by replacing
                     any runs of non- alphanumeric characters with a single
                     ``'-'``.
    :type name: string
    :parameter version: is converted to a standard version string. Spaces
                        become dots, and all other non-alphanumeric characters
                        (except dots) become dashes, with runs of multiple
                        dashes condensed to a single dash.
    :type version: string
    :returns: directory name
    :rtype: string"""
    file_extension = '.dist-info'
    name = name.replace('-', '_')
    normalized_version = suggest_normalized_version(version)
    # Because this is a lookup procedure, something will be returned even if
    #   it is a version that cannot be normalized
    if normalized_version is None:
        # Unable to achieve normality?
        normalized_version = version
    return '-'.join([name, normalized_version]) + file_extension


def get_distributions(use_egg_info=False, paths=sys.path):
    """
    Provides an iterator that looks for ``.dist-info`` directories in
    ``sys.path`` and returns :class:`Distribution` instances for each one of
    them. If the parameters *use_egg_info* is ``True``, then the ``.egg-info``
    files and directores are iterated as well.

    :rtype: iterator of :class:`Distribution` and :class:`EggInfoDistribution`
            instances
    """
    if not _cache_enabled:
        for dist in _yield_distributions(True, use_egg_info, paths):
            yield dist
    else:
        _generate_cache(use_egg_info, paths)

        for dist in _cache_path.values():
            yield dist

        if use_egg_info:
            for dist in _cache_path_egg.values():
                yield dist


def get_distribution(name, use_egg_info=False, paths=None):
    """
    Scans all elements in ``sys.path`` and looks for all directories
    ending with ``.dist-info``. Returns a :class:`Distribution`
    corresponding to the ``.dist-info`` directory that contains the
    ``METADATA`` that matches *name* for the *name* metadata field.
    If no distribution exists with the given *name* and the parameter
    *use_egg_info* is set to ``True``, then all files and directories ending
    with ``.egg-info`` are scanned. A :class:`EggInfoDistribution` instance is
    returned if one is found that has metadata that matches *name* for the
    *name* metadata field.

    This function only returns the first result found, as no more than one
    value is expected. If the directory is not found, ``None`` is returned.

    :rtype: :class:`Distribution` or :class:`EggInfoDistribution` or None
    """
    if paths == None:
        paths = sys.path

    if not _cache_enabled:
        for dist in _yield_distributions(True, use_egg_info, paths):
            if dist.name == name:
                return dist
    else:
        _generate_cache(use_egg_info, paths)

        if name in _cache_name:
            return _cache_name[name][0]
        elif use_egg_info and name in _cache_name_egg:
            return _cache_name_egg[name][0]
        else:
            return None


def obsoletes_distribution(name, version=None, use_egg_info=False):
    """
    Iterates over all distributions to find which distributions obsolete
    *name*.

    If a *version* is provided, it will be used to filter the results.
    If the argument *use_egg_info* is set to ``True``, then ``.egg-info``
    distributions will be considered as well.

    :type name: string
    :type version: string
    :parameter name:
    """
    for dist in get_distributions(use_egg_info):
        obsoleted = (dist.metadata['Obsoletes-Dist'] +
                     dist.metadata['Obsoletes'])
        for obs in obsoleted:
            o_components = obs.split(' ', 1)
            if len(o_components) == 1 or version is None:
                if name == o_components[0]:
                    yield dist
                    break
            else:
                try:
                    predicate = VersionPredicate(obs)
                except ValueError:
                    raise PackagingError(
                        'distribution %r has ill-formed obsoletes field: '
                        '%r' % (dist.name, obs))
                if name == o_components[0] and predicate.match(version):
                    yield dist
                    break


def provides_distribution(name, version=None, use_egg_info=False):
    """
    Iterates over all distributions to find which distributions provide *name*.
    If a *version* is provided, it will be used to filter the results. Scans
    all elements in ``sys.path``  and looks for all directories ending with
    ``.dist-info``. Returns a :class:`Distribution`  corresponding to the
    ``.dist-info`` directory that contains a ``METADATA`` that matches *name*
    for the name metadata. If the argument *use_egg_info* is set to ``True``,
    then all files and directories ending with ``.egg-info`` are considered
    as well and returns an :class:`EggInfoDistribution` instance.

    This function only returns the first result found, since no more than
    one values are expected. If the directory is not found, returns ``None``.

    :parameter version: a version specifier that indicates the version
                        required, conforming to the format in ``PEP-345``

    :type name: string
    :type version: string
    """
    predicate = None
    if not version is None:
        try:
            predicate = VersionPredicate(name + ' (' + version + ')')
        except ValueError:
            raise PackagingError('invalid name or version: %r, %r' %
                                 (name, version))

    for dist in get_distributions(use_egg_info):
        provided = dist.metadata['Provides-Dist'] + dist.metadata['Provides']

        for p in provided:
            p_components = p.rsplit(' ', 1)
            if len(p_components) == 1 or predicate is None:
                if name == p_components[0]:
                    yield dist
                    break
            else:
                p_name, p_ver = p_components
                if len(p_ver) < 2 or p_ver[0] != '(' or p_ver[-1] != ')':
                    raise PackagingError(
                        'distribution %r has invalid Provides field: %r' %
                        (dist.name, p))
                p_ver = p_ver[1:-1]  # trim off the parenthesis
                if p_name == name and predicate.match(p_ver):
                    yield dist
                    break


def get_file_users(path):
    """
    Iterates over all distributions to find out which distributions use
    *path*.

    :parameter path: can be a local absolute path or a relative
                     ``'/'``-separated path.
    :type path: string
    :rtype: iterator of :class:`Distribution` instances
    """
    for dist in get_distributions():
        if dist.uses(path):
            yield dist
