import os
import pathlib
import tempfile
import functools
import contextlib
import types
import importlib

from typing import Union, Any, Optional
from .abc import ResourceReader

Package = Union[types.ModuleType, str]


def files(package):
    """
    Get a Traversable resource from a package
    """
    return from_package(get_package(package))


def normalize_path(path):
    # type: (Any) -> str
    """Normalize a path by ensuring it is a string.

    If the resulting string contains path separators, an exception is raised.
    """
    str_path = str(path)
    parent, file_name = os.path.split(str_path)
    if parent:
        raise ValueError('{!r} must be only a file name'.format(path))
    return file_name


def get_resource_reader(package):
    # type: (types.ModuleType) -> Optional[ResourceReader]
    """
    Return the package's loader if it's a ResourceReader.
    """
    # We can't use
    # a issubclass() check here because apparently abc.'s __subclasscheck__()
    # hook wants to create a weak reference to the object, but
    # zipimport.zipimporter does not support weak references, resulting in a
    # TypeError.  That seems terrible.
    spec = package.__spec__
    reader = getattr(spec.loader, 'get_resource_reader', None)
    if reader is None:
        return None
    return reader(spec.name)


def resolve(cand):
    # type: (Package) -> types.ModuleType
    return (
        cand if isinstance(cand, types.ModuleType)
        else importlib.import_module(cand)
        )


def get_package(package):
    # type: (Package) -> types.ModuleType
    """Take a package name or module object and return the module.

    Raise an exception if the resolved module is not a package.
    """
    resolved = resolve(package)
    if resolved.__spec__.submodule_search_locations is None:
        raise TypeError('{!r} is not a package'.format(package))
    return resolved


def from_package(package):
    """
    Return a Traversable object for the given package.

    """
    spec = package.__spec__
    reader = spec.loader.get_resource_reader(spec.name)
    return reader.files()


@contextlib.contextmanager
def _tempfile(reader, suffix=''):
    # Not using tempfile.NamedTemporaryFile as it leads to deeper 'try'
    # blocks due to the need to close the temporary file to work on Windows
    # properly.
    fd, raw_path = tempfile.mkstemp(suffix=suffix)
    try:
        os.write(fd, reader())
        os.close(fd)
        del reader
        yield pathlib.Path(raw_path)
    finally:
        try:
            os.remove(raw_path)
        except FileNotFoundError:
            pass


@functools.singledispatch
def as_file(path):
    """
    Given a Traversable object, return that object as a
    path on the local file system in a context manager.
    """
    return _tempfile(path.read_bytes, suffix=path.name)


@as_file.register(pathlib.Path)
@contextlib.contextmanager
def _(path):
    """
    Degenerate behavior for pathlib.Path objects.
    """
    yield path
