import os
import pathlib
import types

from typing import Union, Iterable, ContextManager, BinaryIO, TextIO

from . import _common

Package = Union[types.ModuleType, str]
Resource = Union[str, os.PathLike]


def open_binary(package: Package, resource: Resource) -> BinaryIO:
    """Return a file-like object opened for binary reading of the resource."""
    return (_common.files(package) / _common.normalize_path(resource)).open('rb')


def read_binary(package: Package, resource: Resource) -> bytes:
    """Return the binary contents of the resource."""
    return (_common.files(package) / _common.normalize_path(resource)).read_bytes()


def open_text(
    package: Package,
    resource: Resource,
    encoding: str = 'utf-8',
    errors: str = 'strict',
) -> TextIO:
    """Return a file-like object opened for text reading of the resource."""
    return (_common.files(package) / _common.normalize_path(resource)).open(
        'r', encoding=encoding, errors=errors
    )


def read_text(
    package: Package,
    resource: Resource,
    encoding: str = 'utf-8',
    errors: str = 'strict',
) -> str:
    """Return the decoded string of the resource.

    The decoding-related arguments have the same semantics as those of
    bytes.decode().
    """
    with open_text(package, resource, encoding, errors) as fp:
        return fp.read()


def contents(package: Package) -> Iterable[str]:
    """Return an iterable of entries in `package`.

    Note that not all entries are resources.  Specifically, directories are
    not considered resources.  Use `is_resource()` on each entry returned here
    to check if it is a resource or not.
    """
    return [path.name for path in _common.files(package).iterdir()]


def is_resource(package: Package, name: str) -> bool:
    """True if `name` is a resource inside `package`.

    Directories are *not* resources.
    """
    resource = _common.normalize_path(name)
    return any(
        traversable.name == resource and traversable.is_file()
        for traversable in _common.files(package).iterdir()
    )


def path(
    package: Package,
    resource: Resource,
) -> ContextManager[pathlib.Path]:
    """A context manager providing a file path object to the resource.

    If the resource does not already exist on its own on the file system,
    a temporary file will be created. If the file was created, the file
    will be deleted upon exiting the context manager (no exception is
    raised if the file was deleted prior to the context manager
    exiting).
    """
    return _common.as_file(_common.files(package) / _common.normalize_path(resource))
