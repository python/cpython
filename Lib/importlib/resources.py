import os
import tempfile

from . import abc as resources_abc
from contextlib import contextmanager, suppress
from importlib import import_module
from importlib.abc import ResourceLoader
from io import BytesIO, TextIOWrapper
from pathlib import Path
from types import ModuleType
from typing import Iterable, Iterator, Optional, Set, Union   # noqa: F401
from typing import cast
from typing.io import BinaryIO, TextIO
from zipimport import ZipImportError


__all__ = [
    'Package',
    'Resource',
    'contents',
    'is_resource',
    'open_binary',
    'open_text',
    'path',
    'read_binary',
    'read_text',
    ]


Package = Union[str, ModuleType]
Resource = Union[str, os.PathLike]


def _get_package(package) -> ModuleType:
    """Take a package name or module object and return the module.

    If a name, the module is imported.  If the passed or imported module
    object is not a package, raise an exception.
    """
    if hasattr(package, '__spec__'):
        if package.__spec__.submodule_search_locations is None:
            raise TypeError('{!r} is not a package'.format(
                package.__spec__.name))
        else:
            return package
    else:
        module = import_module(package)
        if module.__spec__.submodule_search_locations is None:
            raise TypeError('{!r} is not a package'.format(package))
        else:
            return module


def _normalize_path(path) -> str:
    """Normalize a path by ensuring it is a string.

    If the resulting string contains path separators, an exception is raised.
    """
    parent, file_name = os.path.split(path)
    if parent:
        raise ValueError('{!r} must be only a file name'.format(path))
    else:
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
    _check_location(package)
    absolute_package_path = os.path.abspath(package.__spec__.origin)
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
        else:
            return BytesIO(data)


def open_text(package: Package,
              resource: Resource,
              encoding: str = 'utf-8',
              errors: str = 'strict') -> TextIO:
    """Return a file-like object opened for text reading of the resource."""
    resource = _normalize_path(resource)
    package = _get_package(package)
    reader = _get_resource_reader(package)
    if reader is not None:
        return TextIOWrapper(reader.open_resource(resource), encoding, errors)
    _check_location(package)
    absolute_package_path = os.path.abspath(package.__spec__.origin)
    package_path = os.path.dirname(absolute_package_path)
    full_path = os.path.join(package_path, resource)
    try:
        return open(full_path, mode='r', encoding=encoding, errors=errors)
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
        else:
            return TextIOWrapper(BytesIO(data), encoding, errors)


def read_binary(package: Package, resource: Resource) -> bytes:
    """Return the binary contents of the resource."""
    resource = _normalize_path(resource)
    package = _get_package(package)
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
    resource = _normalize_path(resource)
    package = _get_package(package)
    with open_text(package, resource, encoding, errors) as fp:
        return fp.read()


@contextmanager
def path(package: Package, resource: Resource) -> Iterator[Path]:
    """A context manager providing a file path object to the resource.

    If the resource does not already exist on its own on the file system,
    a temporary file will be created. If the file was created, the file
    will be deleted upon exiting the context manager (no exception is
    raised if the file was deleted prior to the context manager
    exiting).
    """
    resource = _normalize_path(resource)
    package = _get_package(package)
    reader = _get_resource_reader(package)
    if reader is not None:
        try:
            yield Path(reader.resource_path(resource))
            return
        except FileNotFoundError:
            pass
    else:
        _check_location(package)
    # Fall-through for both the lack of resource_path() *and* if
    # resource_path() raises FileNotFoundError.
    file_path = None
    if package.__spec__.origin is not None:
        package_directory = Path(package.__spec__.origin).parent
        file_path = package_directory / resource
    if file_path is not None and file_path.exists():
        yield file_path
    else:
        with open_binary(package, resource) as fp:
            data = fp.read()
        # Not using tempfile.NamedTemporaryFile as it leads to deeper 'try'
        # blocks due to the need to close the temporary file to work on
        # Windows properly.
        fd, raw_path = tempfile.mkstemp()
        try:
            os.write(fd, data)
            os.close(fd)
            yield Path(raw_path)
        finally:
            try:
                os.remove(raw_path)
            except FileNotFoundError:
                pass


def is_resource(package: Package, name: str) -> bool:
    """True if 'name' is a resource inside 'package'.

    Directories are *not* resources.
    """
    package = _get_package(package)
    _normalize_path(name)
    reader = _get_resource_reader(package)
    if reader is not None:
        return reader.is_resource(name)
    try:
        package_contents = set(contents(package))
    except (NotADirectoryError, FileNotFoundError):
        return False
    if name not in package_contents:
        return False
    # Just because the given file_name lives as an entry in the package's
    # contents doesn't necessarily mean it's a resource.  Directories are not
    # resources, so let's try to find out if it's a directory or not.
    path = Path(package.__spec__.origin).parent / name
    return path.is_file()


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
    # cannot have resources.  We could use _check_location() and catch the
    # exception, but that's extra work, so just inline the check.
    elif package.__spec__.origin is None or not package.__spec__.has_location:
        return ()
    else:
        package_directory = Path(package.__spec__.origin).parent
        return os.listdir(package_directory)
