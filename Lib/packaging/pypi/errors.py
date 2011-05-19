"""Exceptions raised by packaging.pypi code."""

from packaging.errors import PackagingPyPIError


class ProjectNotFound(PackagingPyPIError):
    """Project has not been found"""


class DistributionNotFound(PackagingPyPIError):
    """The release has not been found"""


class ReleaseNotFound(PackagingPyPIError):
    """The release has not been found"""


class CantParseArchiveName(PackagingPyPIError):
    """An archive name can't be parsed to find distribution name and version"""


class DownloadError(PackagingPyPIError):
    """An error has occurs while downloading"""


class HashDoesNotMatch(DownloadError):
    """Compared hashes does not match"""


class UnsupportedHashName(PackagingPyPIError):
    """A unsupported hashname has been used"""


class UnableToDownload(PackagingPyPIError):
    """All mirrors have been tried, without success"""


class InvalidSearchField(PackagingPyPIError):
    """An invalid search field has been used"""
