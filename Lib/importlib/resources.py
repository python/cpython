import os
import tempfile

from . import abc as resources_abc
from builtins import open as builtins_open
from contextlib import contextmanager, suppress
from importlib import import_module
from importlib.abc import ResourceLoader
from io import BytesIO, TextIOWrapper
from pathlib import Path
from types import ModuleType
from typing import Iterator, Optional, Set, Union   # noqa: F401
from typing import cast
from typing.io import BinaryIO, TextIO
from zipfile import ZipFile


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
    str_path = str(path)
    parent, file_name = os.path.split(str_path)
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
    if hasattr(package.__spec__.loader, 'open_resource'):
        return cast(resources_abc.ResourceReader, package.__spec__.loader)
    return None


def open_binary(package: Package, resource: Resource) -> BinaryIO:
    """Return a file-like object opened for binary reading of the resource."""
    resource = _normalize_path(resource)
    package = _get_package(package)
    reader = _get_resource_reader(package)
    if reader is not None:
        return reader.open_resource(resource)
    absolute_package_path = os.path.abspath(package.__spec__.origin)
    package_path = os.path.dirname(absolute_package_path)
    full_path = os.path.join(package_path, resource)
    try:
        return builtins_open(full_path, mode='rb')
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
    absolute_package_path = os.path.abspath(package.__spec__.origin)
    package_path = os.path.dirname(absolute_package_path)
    full_path = os.path.join(package_path, resource)
    try:
        return builtins_open(
            full_path, mode='r', encoding=encoding, errors=errors)
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
    # Fall-through for both the lack of resource_path() *and* if
    # resource_path() raises FileNotFoundError.
    package_directory = Path(package.__spec__.origin).parent
    file_path = package_directory / resource
    if file_path.exists():
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
    if path.is_file():
        return True
    if path.is_dir():
        return False
    # If it's not a file and it's not a directory, what is it?  Well, this
    # means the file doesn't exist on the file system, so it probably lives
    # inside a zip file.  We have to crack open the zip, look at its table of
    # contents, and make sure that this entry doesn't have sub-entries.
    archive_path = package.__spec__.loader.archive   # type: ignore
    package_directory = Path(package.__spec__.origin).parent
    with ZipFile(archive_path) as zf:
        toc = zf.namelist()
    relpath = package_directory.relative_to(archive_path)
    candidate_path = relpath / name
    for entry in toc:
        try:
            relative_to_candidate = Path(entry).relative_to(candidate_path)
        except ValueError:
            # The two paths aren't relative to each other so we can ignore it.
            continue
        # Since directories aren't explicitly listed in the zip file, we must
        # infer their 'directory-ness' by looking at the number of path
        # components in the path relative to the package resource we're
        # looking up.  If there are zero additional parts, it's a file, i.e. a
        # resource.  If there are more than zero it's a directory, i.e. not a
        # resource.  It has to be one of these two cases.
        return len(relative_to_candidate.parts) == 0
    # I think it's impossible to get here.  It would mean that we are looking
    # for a resource in a zip file, there's an entry matching it in the return
    # value of contents(), but we never actually found it in the zip's table of
    # contents.
    raise AssertionError('Impossible situation')


def contents(package: Package) -> Iterator[str]:
    """Return the list of entries in 'package'.

    Note that not all entries are resources.  Specifically, directories are
    not considered resources.  Use `is_resource()` on each entry returned here
    to check if it is a resource or not.
    """
    package = _get_package(package)
    reader = _get_resource_reader(package)
    if reader is not None:
        yield from reader.contents()
        return
    # Is the package a namespace package?  By definition, namespace packages
    # cannot have resources.
    if (package.__spec__.origin == 'namespace' and
            not package.__spec__.has_location):
        return []
    package_directory = Path(package.__spec__.origin).parent
    try:
        yield from os.listdir(str(package_directory))
    except (NotADirectoryError, FileNotFoundError):
        # The package is probably in a zip file.
        archive_path = getattr(package.__spec__.loader, 'archive', None)
        if archive_path is None:
            raise
        relpath = package_directory.relative_to(archive_path)
        with ZipFile(archive_path) as zf:
            toc = zf.namelist()
        subdirs_seen = set()                        # type: Set
        for filename in toc:
            path = Path(filename)
            # Strip off any path component parts that are in common with the
            # package directory, relative to the zip archive's file system
            # path.  This gives us all the parts that live under the named
            # package inside the zip file.  If the length of these subparts is
            # exactly 1, then it is situated inside the package.  The resulting
            # length will be 0 if it's above the package, and it will be
            # greater than 1 if it lives in a subdirectory of the package
            # directory.
            #
            # However, since directories themselves don't appear in the zip
            # archive as a separate entry, we need to return the first path
            # component for any case that has > 1 subparts -- but only once!
            if path.parts[:len(relpath.parts)] != relpath.parts:
                continue
            subparts = path.parts[len(relpath.parts):]
            if len(subparts) == 1:
                yield subparts[0]
            elif len(subparts) > 1:
                subdir = subparts[0]
                if subdir not in subdirs_seen:
                    subdirs_seen.add(subdir)
                    yield subdir
