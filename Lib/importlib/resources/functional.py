"""Simplified function-based API for importlib.resources


"""

import os
import warnings

from ._common import files, as_file


def open_binary(anchor, *path_names):
    """Open for binary reading the *resource* within *package*."""
    return _get_resource(anchor, path_names).open('rb')


def open_text(anchor, *path_names, encoding='utf-8', errors='strict'):
    """Open for text reading the *resource* within *package*."""
    resource = _get_resource(anchor, path_names)
    return resource.open('r', encoding=encoding, errors=errors)


def read_binary(anchor, *path_names):
    """Read and return contents of *resource* within *package* as bytes."""
    return _get_resource(anchor, path_names).read_bytes()


def read_text(anchor, *path_names, encoding='utf-8', errors='strict'):
    """Read and return contents of *resource* within *package* as str."""
    resource = _get_resource(anchor, path_names)
    return resource.read_text(encoding=encoding, errors=errors)


def path(anchor, *path_names):
    """Return the path to the *resource* as an actual file system path."""
    return as_file(_get_resource(anchor, path_names))


def is_resource(anchor, *path_names):
    """Return ``True`` if there is a resource named *name* in the package,

    Otherwise returns ``False``.
    """
    return _get_resource(anchor, path_names).is_file()


def contents(anchor, *path_names):
    """Return an iterable over the named resources within the package.

    The iterable returns :class:`str` resources (e.g. files).
    The iterable does not recurse into subdirectories.
    """
    warnings.warn(
        "importlib.resources.contents is deprecated. "
        "Use files(anchor).iterdir() instead.",
        DeprecationWarning,
        stacklevel=1,
    )
    return (
        resource.name
        for resource
        in _get_resource(anchor, path_names).iterdir()
    )


def _get_resource(anchor, path_names):
    if anchor is None:
        raise TypeError("anchor must be module or string, got None")
    traversable = files(anchor)
    for name in path_names:
        str_path = str(name)
        parent, file_name = os.path.split(str_path)
        if parent:
            raise ValueError(
                'path name elements must not contain path separators, '
                f'got {name!r}')
        traversable = traversable.joinpath(file_name)
    return traversable
