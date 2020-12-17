# Copyright (c) Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Versions for Python packages.

See L{Version}.
"""

from __future__ import division, absolute_import

import sys
import warnings

#
# Compat functions
#

if sys.version_info < (3, 0):
    _PY3 = False
else:
    _PY3 = True

try:
    _cmp = cmp
except NameError:
    def _cmp(a, b):
        """
        Compare two objects.

        Returns a negative number if C{a < b}, zero if they are equal, and a
        positive number if C{a > b}.
        """
        if a < b:
            return -1
        elif a == b:
            return 0
        else:
            return 1


def _comparable(klass):
    """
    Class decorator that ensures support for the special C{__cmp__} method.

    On Python 2 this does nothing.

    On Python 3, C{__eq__}, C{__lt__}, etc. methods are added to the class,
    relying on C{__cmp__} to implement their comparisons.
    """
    # On Python 2, __cmp__ will just work, so no need to add extra methods:
    if not _PY3:
        return klass

    def __eq__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c == 0

    def __ne__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c != 0

    def __lt__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c < 0

    def __le__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c <= 0

    def __gt__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c > 0

    def __ge__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c >= 0

    klass.__lt__ = __lt__
    klass.__gt__ = __gt__
    klass.__le__ = __le__
    klass.__ge__ = __ge__
    klass.__eq__ = __eq__
    klass.__ne__ = __ne__
    return klass

#
# Versioning
#


@_comparable
class _inf(object):
    """
    An object that is bigger than all other objects.
    """
    def __cmp__(self, other):
        """
        @param other: Another object.
        @type other: any

        @return: 0 if other is inf, 1 otherwise.
        @rtype: C{int}
        """
        if other is _inf:
            return 0
        return 1


_inf = _inf()


class IncomparableVersions(TypeError):
    """
    Two versions could not be compared.
    """


@_comparable
class Version(object):
    """
    An encapsulation of a version for a project, with support for outputting
    PEP-440 compatible version strings.

    This class supports the standard major.minor.micro[rcN] scheme of
    versioning.
    """
    def __init__(self, package, major, minor, micro, release_candidate=None,
                 prerelease=None, dev=None):
        """
        @param package: Name of the package that this is a version of.
        @type package: C{str}
        @param major: The major version number.
        @type major: C{int} or C{str} (for the "NEXT" symbol)
        @param minor: The minor version number.
        @type minor: C{int}
        @param micro: The micro version number.
        @type micro: C{int}
        @param release_candidate: The release candidate number.
        @type release_candidate: C{int}
        @param prerelease: The prerelease number. (Deprecated)
        @type prerelease: C{int}
        @param dev: The development release number.
        @type dev: C{int}
        """
        if release_candidate and prerelease:
            raise ValueError("Please only return one of these.")
        elif prerelease and not release_candidate:
            release_candidate = prerelease
            warnings.warn(("Passing prerelease to incremental.Version was "
                           "deprecated in Incremental 16.9.0. Please pass "
                           "release_candidate instead."),
                          DeprecationWarning, stacklevel=2)

        if major == "NEXT":
            if minor or micro or release_candidate or dev:
                raise ValueError(("When using NEXT, all other values except "
                                  "Package must be 0."))

        self.package = package
        self.major = major
        self.minor = minor
        self.micro = micro
        self.release_candidate = release_candidate
        self.dev = dev

    @property
    def prerelease(self):
        warnings.warn(("Accessing incremental.Version.prerelease was "
                       "deprecated in Incremental 16.9.0. Use "
                       "Version.release_candidate instead."),
                      DeprecationWarning, stacklevel=2),
        return self.release_candidate

    def public(self):
        """
        Return a PEP440-compatible "public" representation of this L{Version}.

        Examples:

          - 14.4.0
          - 1.2.3rc1
          - 14.2.1rc1dev9
          - 16.04.0dev0
        """
        if self.major == "NEXT":
            return self.major

        if self.release_candidate is None:
            rc = ""
        else:
            rc = "rc%s" % (self.release_candidate,)

        if self.dev is None:
            dev = ""
        else:
            dev = "dev%s" % (self.dev,)

        return '%r.%d.%d%s%s' % (self.major,
                                 self.minor,
                                 self.micro,
                                 rc, dev)

    base = public
    short = public
    local = public

    def __repr__(self):

        if self.release_candidate is None:
            release_candidate = ""
        else:
            release_candidate = ", release_candidate=%r" % (
                self.release_candidate,)

        if self.dev is None:
            dev = ""
        else:
            dev = ", dev=%r" % (self.dev,)

        return '%s(%r, %r, %d, %d%s%s)' % (
            self.__class__.__name__,
            self.package,
            self.major,
            self.minor,
            self.micro,
            release_candidate,
            dev)

    def __str__(self):
        return '[%s, version %s]' % (
            self.package,
            self.short())

    def __cmp__(self, other):
        """
        Compare two versions, considering major versions, minor versions, micro
        versions, then release candidates. Package names are case insensitive.

        A version with a release candidate is always less than a version
        without a release candidate. If both versions have release candidates,
        they will be included in the comparison.

        @param other: Another version.
        @type other: L{Version}

        @return: NotImplemented when the other object is not a Version, or one
            of -1, 0, or 1.

        @raise IncomparableVersions: when the package names of the versions
            differ.
        """
        if not isinstance(other, self.__class__):
            return NotImplemented
        if self.package.lower() != other.package.lower():
            raise IncomparableVersions("%r != %r"
                                       % (self.package, other.package))

        if self.major == "NEXT":
            major = _inf
        else:
            major = self.major

        if self.release_candidate is None:
            release_candidate = _inf
        else:
            release_candidate = self.release_candidate

        if self.dev is None:
            dev = _inf
        else:
            dev = self.dev

        if other.major == "NEXT":
            othermajor = _inf
        else:
            othermajor = other.major

        if other.release_candidate is None:
            otherrc = _inf
        else:
            otherrc = other.release_candidate

        if other.dev is None:
            otherdev = _inf
        else:
            otherdev = other.dev

        x = _cmp((major,
                  self.minor,
                  self.micro,
                  release_candidate,
                  dev),
                 (othermajor,
                  other.minor,
                  other.micro,
                  otherrc,
                  otherdev))
        return x


def getVersionString(version):
    """
    Get a friendly string for the given version object.

    @param version: A L{Version} object.
    @return: A string containing the package and short version number.
    """
    result = '%s %s' % (version.package, version.short())
    return result


def _get_version(dist, keyword, value):
    """
    Get the version from the package listed in the Distribution.
    """
    if not value:
        return

    from distutils.command import build_py

    sp_command = build_py.build_py(dist)
    sp_command.finalize_options()

    for item in sp_command.find_all_modules():
        if item[1] == "_version":
            version_file = {}

            with open(item[2]) as f:
                exec(f.read(), version_file)

            dist.metadata.version = version_file["__version__"].public()
            return None

    raise Exception("No _version.py found.")


from ._version import __version__ # noqa


__all__ = ["__version__", "Version", "getVersionString"]
