"""Classes representing releases and distributions retrieved from indexes.

A project (= unique name) can have several releases (= versions) and
each release can have several distributions (= sdist and bdists).

Release objects contain metadata-related information (see PEP 376);
distribution objects contain download-related information.
"""

import sys
import mimetypes
import re
import tempfile
import urllib.request
import urllib.parse
import urllib.error
import urllib.parse
import hashlib
from shutil import unpack_archive

from packaging.errors import IrrationalVersionError
from packaging.version import (suggest_normalized_version, NormalizedVersion,
                               get_version_predicate)
from packaging.metadata import Metadata
from packaging.pypi.errors import (HashDoesNotMatch, UnsupportedHashName,
                                   CantParseArchiveName)


__all__ = ['ReleaseInfo', 'DistInfo', 'ReleasesList', 'get_infos_from_url']

EXTENSIONS = ".tar.gz .tar.bz2 .tar .zip .tgz .egg".split()
MD5_HASH = re.compile(r'^.*#md5=([a-f0-9]+)$')
DIST_TYPES = ['bdist', 'sdist']


class IndexReference:
    """Mixin used to store the index reference"""
    def set_index(self, index=None):
        self._index = index


class ReleaseInfo(IndexReference):
    """Represent a release of a project (a project with a specific version).
    The release contain the _metadata informations related to this specific
    version, and is also a container for distribution related informations.

    See the DistInfo class for more information about distributions.
    """

    def __init__(self, name, version, metadata=None, hidden=False,
                 index=None, **kwargs):
        """
        :param name: the name of the distribution
        :param version: the version of the distribution
        :param metadata: the metadata fields of the release.
        :type metadata: dict
        :param kwargs: optional arguments for a new distribution.
        """
        self.set_index(index)
        self.name = name
        self._version = None
        self.version = version
        if metadata:
            self.metadata = Metadata(mapping=metadata)
        else:
            self.metadata = None
        self.dists = {}
        self.hidden = hidden

        if 'dist_type' in kwargs:
            dist_type = kwargs.pop('dist_type')
            self.add_distribution(dist_type, **kwargs)

    def set_version(self, version):
        try:
            self._version = NormalizedVersion(version)
        except IrrationalVersionError:
            suggestion = suggest_normalized_version(version)
            if suggestion:
                self.version = suggestion
            else:
                raise IrrationalVersionError(version)

    def get_version(self):
        return self._version

    version = property(get_version, set_version)

    def fetch_metadata(self):
        """If the metadata is not set, use the indexes to get it"""
        if not self.metadata:
            self._index.get_metadata(self.name, str(self.version))
        return self.metadata

    @property
    def is_final(self):
        """proxy to version.is_final"""
        return self.version.is_final

    def fetch_distributions(self):
        if self.dists is None:
            self._index.get_distributions(self.name, str(self.version))
            if self.dists is None:
                self.dists = {}
        return self.dists

    def add_distribution(self, dist_type='sdist', python_version=None,
                         **params):
        """Add distribution informations to this release.
        If distribution information is already set for this distribution type,
        add the given url paths to the distribution. This can be useful while
        some of them fails to download.

        :param dist_type: the distribution type (eg. "sdist", "bdist", etc.)
        :param params: the fields to be passed to the distribution object
                       (see the :class:DistInfo constructor).
        """
        if dist_type not in DIST_TYPES:
            raise ValueError(dist_type)
        if dist_type in self.dists:
            self.dists[dist_type].add_url(**params)
        else:
            self.dists[dist_type] = DistInfo(self, dist_type,
                                             index=self._index, **params)
        if python_version:
            self.dists[dist_type].python_version = python_version

    def get_distribution(self, dist_type=None, prefer_source=True):
        """Return a distribution.

        If dist_type is set, find first for this distribution type, and just
        act as an alias of __get_item__.

        If prefer_source is True, search first for source distribution, and if
        not return one existing distribution.
        """
        if len(self.dists) == 0:
            raise LookupError()
        if dist_type:
            return self[dist_type]
        if prefer_source:
            if "sdist" in self.dists:
                dist = self["sdist"]
            else:
                dist = next(self.dists.values())
            return dist

    def unpack(self, path=None, prefer_source=True):
        """Unpack the distribution to the given path.

        If not destination is given, creates a temporary location.

        Returns the location of the extracted files (root).
        """
        return self.get_distribution(prefer_source=prefer_source)\
                   .unpack(path=path)

    def download(self, temp_path=None, prefer_source=True):
        """Download the distribution, using the requirements.

        If more than one distribution match the requirements, use the last
        version.
        Download the distribution, and put it in the temp_path. If no temp_path
        is given, creates and return one.

        Returns the complete absolute path to the downloaded archive.
        """
        return self.get_distribution(prefer_source=prefer_source)\
                   .download(path=temp_path)

    def set_metadata(self, metadata):
        if not self.metadata:
            self.metadata = Metadata()
        self.metadata.update(metadata)

    def __getitem__(self, item):
        """distributions are available using release["sdist"]"""
        return self.dists[item]

    def _check_is_comparable(self, other):
        if not isinstance(other, ReleaseInfo):
            raise TypeError("cannot compare %s and %s"
                % (type(self).__name__, type(other).__name__))
        elif self.name != other.name:
            raise TypeError("cannot compare %s and %s"
                % (self.name, other.name))

    def __repr__(self):
        return "<%s %s>" % (self.name, self.version)

    def __eq__(self, other):
        self._check_is_comparable(other)
        return self.version == other.version

    def __lt__(self, other):
        self._check_is_comparable(other)
        return self.version < other.version

    def __ne__(self, other):
        return not self.__eq__(other)

    def __gt__(self, other):
        return not (self.__lt__(other) or self.__eq__(other))

    def __le__(self, other):
        return self.__eq__(other) or self.__lt__(other)

    def __ge__(self, other):
        return self.__eq__(other) or self.__gt__(other)

    # See http://docs.python.org/reference/datamodel#object.__hash__
    __hash__ = object.__hash__


class DistInfo(IndexReference):
    """Represents a distribution retrieved from an index (sdist, bdist, ...)
    """

    def __init__(self, release, dist_type=None, url=None, hashname=None,
                 hashval=None, is_external=True, python_version=None,
                 index=None):
        """Create a new instance of DistInfo.

        :param release: a DistInfo class is relative to a release.
        :param dist_type: the type of the dist (eg. source, bin-*, etc.)
        :param url: URL where we found this distribution
        :param hashname: the name of the hash we want to use. Refer to the
                         hashlib.new documentation for more information.
        :param hashval: the hash value.
        :param is_external: we need to know if the provided url comes from
                            an index browsing, or from an external resource.

        """
        self.set_index(index)
        self.release = release
        self.dist_type = dist_type
        self.python_version = python_version
        self._unpacked_dir = None
        # set the downloaded path to None by default. The goal here
        # is to not download distributions multiple times
        self.downloaded_location = None
        # We store urls in dict, because we need to have a bit more infos
        # than the simple URL. It will be used later to find the good url to
        # use.
        # We have two _url* attributes: _url and urls. urls contains a list
        # of dict for the different urls, and _url contains the choosen url, in
        # order to dont make the selection process multiple times.
        self.urls = []
        self._url = None
        self.add_url(url, hashname, hashval, is_external)

    def add_url(self, url=None, hashname=None, hashval=None, is_external=True):
        """Add a new url to the list of urls"""
        if hashname is not None:
            try:
                hashlib.new(hashname)
            except ValueError:
                raise UnsupportedHashName(hashname)
        if not url in [u['url'] for u in self.urls]:
            self.urls.append({
                'url': url,
                'hashname': hashname,
                'hashval': hashval,
                'is_external': is_external,
            })
            # reset the url selection process
            self._url = None

    @property
    def url(self):
        """Pick up the right url for the list of urls in self.urls"""
        # We return internal urls over externals.
        # If there is more than one internal or external, return the first
        # one.
        if self._url is None:
            if len(self.urls) > 1:
                internals_urls = [u for u in self.urls \
                                  if u['is_external'] == False]
                if len(internals_urls) >= 1:
                    self._url = internals_urls[0]
            if self._url is None:
                self._url = self.urls[0]
        return self._url

    @property
    def is_source(self):
        """return if the distribution is a source one or not"""
        return self.dist_type == 'sdist'

    def download(self, path=None):
        """Download the distribution to a path, and return it.

        If the path is given in path, use this, otherwise, generates a new one
        Return the download location.
        """
        if path is None:
            path = tempfile.mkdtemp()

        # if we do not have downloaded it yet, do it.
        if self.downloaded_location is None:
            url = self.url['url']
            archive_name = urllib.parse.urlparse(url)[2].split('/')[-1]
            filename, headers = urllib.request.urlretrieve(url,
                                                   path + "/" + archive_name)
            self.downloaded_location = filename
            self._check_md5(filename)
        return self.downloaded_location

    def unpack(self, path=None):
        """Unpack the distribution to the given path.

        If not destination is given, creates a temporary location.

        Returns the location of the extracted files (root).
        """
        if not self._unpacked_dir:
            if path is None:
                path = tempfile.mkdtemp()

            filename = self.download(path)
            content_type = mimetypes.guess_type(filename)[0]
            unpack_archive(filename, path)
            self._unpacked_dir = path

        return path

    def _check_md5(self, filename):
        """Check that the md5 checksum of the given file matches the one in
        url param"""
        hashname = self.url['hashname']
        expected_hashval = self.url['hashval']
        if not None in (expected_hashval, hashname):
            with open(filename, 'rb') as f:
                hashval = hashlib.new(hashname)
                hashval.update(f.read())

            if hashval.hexdigest() != expected_hashval:
                raise HashDoesNotMatch("got %s instead of %s"
                    % (hashval.hexdigest(), expected_hashval))

    def __repr__(self):
        if self.release is None:
            return "<? ? %s>" % self.dist_type

        return "<%s %s %s>" % (
            self.release.name, self.release.version, self.dist_type or "")


class ReleasesList(IndexReference):
    """A container of Release.

    Provides useful methods and facilities to sort and filter releases.
    """
    def __init__(self, name, releases=None, contains_hidden=False, index=None):
        self.set_index(index)
        self.releases = []
        self.name = name
        self.contains_hidden = contains_hidden
        if releases:
            self.add_releases(releases)

    def fetch_releases(self):
        self._index.get_releases(self.name)
        return self.releases

    def filter(self, predicate):
        """Filter and return a subset of releases matching the given predicate.
        """
        return ReleasesList(self.name, [release for release in self.releases
                                        if predicate.match(release.version)],
                                        index=self._index)

    def get_last(self, requirements, prefer_final=None):
        """Return the "last" release, that satisfy the given predicates.

        "last" is defined by the version number of the releases, you also could
        set prefer_final parameter to True or False to change the order results
        """
        predicate = get_version_predicate(requirements)
        releases = self.filter(predicate)
        if len(releases) == 0:
            return None
        releases.sort_releases(prefer_final, reverse=True)
        return releases[0]

    def add_releases(self, releases):
        """Add releases in the release list.

        :param: releases is a list of ReleaseInfo objects.
        """
        for r in releases:
            self.add_release(release=r)

    def add_release(self, version=None, dist_type='sdist', release=None,
                    **dist_args):
        """Add a release to the list.

        The release can be passed in the `release` parameter, and in this case,
        it will be crawled to extract the useful informations if necessary, or
        the release informations can be directly passed in the `version` and
        `dist_type` arguments.

        Other keywords arguments can be provided, and will be forwarded to the
        distribution creation (eg. the arguments of the DistInfo constructor).
        """
        if release:
            if release.name.lower() != self.name.lower():
                raise ValueError("%s is not the same project as %s" %
                                 (release.name, self.name))
            version = str(release.version)

            if not version in self.get_versions():
                # append only if not already exists
                self.releases.append(release)
            for dist in release.dists.values():
                for url in dist.urls:
                    self.add_release(version, dist.dist_type, **url)
        else:
            matches = [r for r in self.releases
                       if str(r.version) == version and r.name == self.name]
            if not matches:
                release = ReleaseInfo(self.name, version, index=self._index)
                self.releases.append(release)
            else:
                release = matches[0]

            release.add_distribution(dist_type=dist_type, **dist_args)

    def sort_releases(self, prefer_final=False, reverse=True, *args, **kwargs):
        """Sort the results with the given properties.

        The `prefer_final` argument can be used to specify if final
        distributions (eg. not dev, bet or alpha) would be prefered or not.

        Results can be inverted by using `reverse`.

        Any other parameter provided will be forwarded to the sorted call. You
        cannot redefine the key argument of "sorted" here, as it is used
        internally to sort the releases.
        """

        sort_by = []
        if prefer_final:
            sort_by.append("is_final")
        sort_by.append("version")

        self.releases.sort(
            key=lambda i: tuple(getattr(i, arg) for arg in sort_by),
            reverse=reverse, *args, **kwargs)

    def get_release(self, version):
        """Return a release from its version."""
        matches = [r for r in self.releases if str(r.version) == version]
        if len(matches) != 1:
            raise KeyError(version)
        return matches[0]

    def get_versions(self):
        """Return a list of releases versions contained"""
        return [str(r.version) for r in self.releases]

    def __getitem__(self, key):
        return self.releases[key]

    def __len__(self):
        return len(self.releases)

    def __repr__(self):
        string = 'Project "%s"' % self.name
        if self.get_versions():
            string += ' versions: %s' % ', '.join(self.get_versions())
        return '<%s>' % string


def get_infos_from_url(url, probable_dist_name=None, is_external=True):
    """Get useful informations from an URL.

    Return a dict of (name, version, url, hashtype, hash, is_external)

    :param url: complete url of the distribution
    :param probable_dist_name: A probable name of the project.
    :param is_external: Tell if the url commes from an index or from
                        an external URL.
    """
    # if the url contains a md5 hash, get it.
    md5_hash = None
    match = MD5_HASH.match(url)
    if match is not None:
        md5_hash = match.group(1)
        # remove the hash
        url = url.replace("#md5=%s" % md5_hash, "")

    # parse the archive name to find dist name and version
    archive_name = urllib.parse.urlparse(url)[2].split('/')[-1]
    extension_matched = False
    # remove the extension from the name
    for ext in EXTENSIONS:
        if archive_name.endswith(ext):
            archive_name = archive_name[:-len(ext)]
            extension_matched = True

    name, version = split_archive_name(archive_name)
    if extension_matched is True:
        return {'name': name,
                'version': version,
                'url': url,
                'hashname': "md5",
                'hashval': md5_hash,
                'is_external': is_external,
                'dist_type': 'sdist'}


def split_archive_name(archive_name, probable_name=None):
    """Split an archive name into two parts: name and version.

    Return the tuple (name, version)
    """
    # Try to determine wich part is the name and wich is the version using the
    # "-" separator. Take the larger part to be the version number then reduce
    # if this not works.
    def eager_split(str, maxsplit=2):
        # split using the "-" separator
        splits = str.rsplit("-", maxsplit)
        name = splits[0]
        version = "-".join(splits[1:])
        if version.startswith("-"):
            version = version[1:]
        if suggest_normalized_version(version) is None and maxsplit >= 0:
            # we dont get a good version number: recurse !
            return eager_split(str, maxsplit - 1)
        else:
            return name, version
    if probable_name is not None:
        probable_name = probable_name.lower()
    name = None
    if probable_name is not None and probable_name in archive_name:
        # we get the name from probable_name, if given.
        name = probable_name
        version = archive_name.lstrip(name)
    else:
        name, version = eager_split(archive_name)

    version = suggest_normalized_version(version)
    if version is not None and name != "":
        return name.lower(), version
    else:
        raise CantParseArchiveName(archive_name)
