import os

from . import abc as resources_abc
from . import _common
from ._common import as_file
from contextlib import contextmanager, suppress
from importlib import import_module
from importlib.abc import ResourceLoader
from io import BytesIO, TextIOWrapper
from pathlib import Path
from types import ModuleType
from typing import ContextManager, Iterable, Optional, Union
from typing import cast
from typing.io import BinaryIO, TextIO


__all__ = [
    'Package',
    'Resource',
    'as_file',
    'contents',
    'files',
    'is_resource',
    'open_binary',
    'open_text',
    'path',
    'read_binary',
    'read_text',
    ]


Package = Union[str, ModuleType]
Resource = Union[str, os.PathLike]


def _resolve(name) -> ModuleType:
    """If name is a string, resolve to a module."""
    if hasattr(name, '__spec__'):
        return name
    return import_module(name)


def _get_package(package) -> ModuleType:
    """Take a package name or module object and return the module.

    If a name, the module is imported.  If the resolved module
    object is not a package, raise an exception.
    """
    module = _resolve(package)
    if module.__spec__.submodule_search_locations is None:
        raise TypeError('{!r} is not a package'.format(package))
    return module


def _normalize_path(path) -> str:
    """Normalize a path by ensuring it is a string.

    If the resulting string contains path separators, an exception is raised.
    """
    parent, file_name = os.path.split(path)
    if parent:
        raise ValueError('{!r} must be only a file name'.format(path))
    return file_name


def _get_resource_reader(
        package: ModuleType) -> Optional[resources_abc.ResourceReader]:
    # Return the package's loader if it's a ResourceReader.  We can't use
    # a issubclass() check here because apparently abc.'s __subclasscheck__()
    # hook wants to create a weak reference to the object, but
    # zipimport.zipimporter does not support weak references, resulting in a
    # TypeError.  That seems terrible.
    spec = package.__spec__
    if hasattr(spec.loader, 'get_resource_reader'):
        return cast(resources_abc.ResourceReader,
                    spec.loader.get_resource_reader(spec.name))
    return None


def _check_location(package):
    if package.__spec__.origin is None or not package.__spec__.has_location:
        raise FileNotFoundError(f'Package has no location {package!r}')


def open_binary(package: Package, resource: Resource) -> BinaryIO:
    """Return a file-like object opened for binary reading of the resource."""
    resource = _normalize_path(resource)
    package = _get_package(package)
    reader = _get_resource_reader(package)
    if reader is not None:
        return reader.open_resource(resource)
    absolute_package_path = os.path.abspath(
        package.__spec__.origin or 'non-existent file')
    package_path = os.path.dirname(absolute_package_path)
    full_path = os.path.join(package_path, resource)
    try:
        return open(full_path, mode='rb')
    except OSError:
        # Just assume the loader is a resource loader; all the relevant
        # importlib.machinery loaders are and an AttributeError for
        # get_data() will make it clear what is needed from the loader.
        loader = cast(ResourceLoader, package.__spec__.loader)
        data = None
        if hasattr(package.__spec__.loader, 'get_data'):
            with suppress(OSError):
                data = loader.get_data(full_path)
        if data is None:
            package_name = package.__spec__.name
            message = '{!r} resource not found in {!r}'.format(
                resource, package_name)
            raise FileNotFoundError(message)
        return BytesIO(data)


def open_text(package: Package,
              resource: Resource,
              encoding: str = 'utf-8',
              errors: str = 'strict') -> TextIO:
    """Return a file-like object opened for text reading of the resource."""
    return TextIOWrapper(
        open_binary(package, resource), encoding=encoding, errors=errors)


def read_binary(package: Package, resource: Resource) -> bytes:
    """Return the binary contents of the resource."""
    with open_binary(package, resource) as fp:
        return fp.read()


def read_text(package: Package,
              resource: Resource,
              encoding: str = 'utf-8',
              errors: str = 'strict') -> str:
    """Return the decoded string of the resource.

    The decoding-related arguments have the same semantics as those of
    bytes.decode().
    """
    with open_text(package, resource, encoding, errors) as fp:
        return fp.read()


def files(package: Package) -> resources_abc.Traversable:
    """
    Get a Traversable resource from a package
    """
    return _common.from_package(_get_package(package))


def path(
        package: Package, resource: Resource,
        ) -> 'ContextManager[Path]':
    """A context manager providing a file path object to the resource.

    If the resource does not already exist on its own on the file system,
    a temporary file will be created. If the file was created, the file
    will be deleted upon exiting the context manager (no exception is
    raised if the file was deleted prior to the context manager
    exiting).
    """
    reader = _get_resource_reader(_get_package(package))
    return (
        _path_from_reader(reader, resource)
        if reader else
        _common.as_file(files(package).joinpath(_normalize_path(resource)))
        )


@contextmanager
def _path_from_reader(reader, resource):
    norm_resource = _normalize_path(resource)
    with suppress(FileNotFoundError):
        yield Path(reader.resource_path(norm_resource))
        return
    opener_reader = reader.open_resource(norm_resource)
    with _common._tempfile(opener_reader.read, suffix=norm_resource) as res:
        yield res


def is_resource(package: Package, name: str) -> bool:
    """True if 'name' is a resource inside 'package'.

    Directories are *not* resources.
    """
    package = _get_package(package)
    _normalize_path(name)
    reader = _get_resource_reader(package)
    if reader is not None:
        return reader.is_resource(name)
    package_contents = set(contents(package))
    if name not in package_contents:
        return False
    return (_common.from_package(package) / name).is_file()


def contents(package: Package) -> Iterable[str]:
    """Return an iterable of entries in 'package'.

    Note that not all entries are resources.  Specifically, directories are
    not considered resources.  Use `is_resource()` on each entry returned here
    to check if it is a resource or not.
    """
    package = _get_package(package)
    reader = _get_resource_reader(package)
    if reader is not None:
        return reader.contents()
    # Is the package a namespace package?  By definition, namespace packages
    # cannot have resources.
    namespace = (
        package.__spec__.origin is None or
        package.__spec__.origin == 'namespace'
        )
    if namespace or not package.__spec__.has_location:
        return ()
    return list(item.name for item in _common.from_package(package).iterdir())
