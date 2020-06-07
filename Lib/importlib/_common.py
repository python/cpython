import os
import pathlib
import zipfile
import tempfile
import functools
import contextlib


def from_package(package):
    """
    Return a Traversable object for the given package.

    """
    spec = package.__spec__
    return from_traversable_resources(spec) or fallback_resources(spec)


def from_traversable_resources(spec):
    """
    If the spec.loader implements TraversableResources,
    directly or implicitly, it will have a ``files()`` method.
    """
    with contextlib.suppress(AttributeError):
        return spec.loader.files()


def fallback_resources(spec):
    package_directory = pathlib.Path(spec.origin).parent
    try:
        archive_path = spec.loader.archive
        rel_path = package_directory.relative_to(archive_path)
        return zipfile.Path(archive_path, str(rel_path) + '/')
    except Exception:
        pass
    return package_directory


@contextlib.contextmanager
def _tempfile(reader, suffix=''):
    # Not using tempfile.NamedTemporaryFile as it leads to deeper 'try'
    # blocks due to the need to close the temporary file to work on Windows
    # properly.
    fd, raw_path = tempfile.mkstemp(suffix=suffix)
    try:
        os.write(fd, reader())
        os.close(fd)
        yield pathlib.Path(raw_path)
    finally:
        try:
            os.remove(raw_path)
        except FileNotFoundError:
            pass


@functools.singledispatch
@contextlib.contextmanager
def as_file(path):
    """
    Given a Traversable object, return that object as a
    path on the local file system in a context manager.
    """
    with _tempfile(path.read_bytes, suffix=path.name) as local:
        yield local


@as_file.register(pathlib.Path)
@contextlib.contextmanager
def _(path):
    """
    Degenerate behavior for pathlib.Path objects.
    """
    yield path
