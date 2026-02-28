"""Async file I/O for asyncio."""

__all__ = ('open_file', 'wrap_file', 'AsyncFile', 'Path')

import os
import pathlib

from . import mixins
from .threads import to_thread


async def open_file(file, mode='r', buffering=-1, encoding=None,
                    errors=None, newline=None, closefd=True, opener=None):
    """Asynchronously open a file, returning an AsyncFile.

    This is the async equivalent of the builtin open(). The file is opened
    in a separate thread via asyncio.to_thread().
    """
    fp = await to_thread(open, file, mode, buffering, encoding,
                         errors, newline, closefd, opener)
    return AsyncFile(fp)


def wrap_file(file):
    """Wrap an already-open file-like object as an AsyncFile.

    The file object must have a close() method and at least one of
    read() or write().
    """
    if not hasattr(file, 'close'):
        raise TypeError(
            f"Expected a file-like object with a close() method, "
            f"got {type(file).__name__}")
    if not (hasattr(file, 'read') or hasattr(file, 'write')):
        raise TypeError(
            f"Expected a file-like object with read() or write(), "
            f"got {type(file).__name__}")
    return AsyncFile(file)


class AsyncFile(mixins._LoopBoundMixin):
    """Async wrapper around a synchronous file object.

    All I/O methods are delegated to a thread via asyncio.to_thread().
    Sync attributes (name, mode, closed, etc.) are passed through directly.
    """

    def __init__(self, fp):
        self._fp = fp

    @property
    def wrapped(self):
        """The underlying synchronous file object."""
        return self._fp

    # --- Async I/O methods ---

    async def read(self, size=-1):
        return await to_thread(self._fp.read, size)

    async def read1(self, size=-1):
        if not hasattr(self._fp, 'read1'):
            raise AttributeError(
                f"'{type(self._fp).__name__}' object has no attribute 'read1'")
        return await to_thread(self._fp.read1, size)

    async def readinto(self, b):
        if not hasattr(self._fp, 'readinto'):
            raise AttributeError(
                f"'{type(self._fp).__name__}' object has no attribute 'readinto'")
        return await to_thread(self._fp.readinto, b)

    async def readinto1(self, b):
        if not hasattr(self._fp, 'readinto1'):
            raise AttributeError(
                f"'{type(self._fp).__name__}' object has no attribute 'readinto1'")
        return await to_thread(self._fp.readinto1, b)

    async def readline(self):
        return await to_thread(self._fp.readline)

    async def readlines(self):
        return await to_thread(self._fp.readlines)

    async def write(self, data):
        return await to_thread(self._fp.write, data)

    async def writelines(self, lines):
        return await to_thread(self._fp.writelines, lines)

    async def truncate(self, size=None):
        return await to_thread(self._fp.truncate, size)

    async def seek(self, offset, whence=os.SEEK_SET):
        return await to_thread(self._fp.seek, offset, whence)

    async def tell(self):
        return await to_thread(self._fp.tell)

    async def flush(self):
        return await to_thread(self._fp.flush)

    async def peek(self, size=0):
        if not hasattr(self._fp, 'peek'):
            raise AttributeError(
                f"'{type(self._fp).__name__}' object has no attribute 'peek'")
        return await to_thread(self._fp.peek, size)

    async def aclose(self):
        return await to_thread(self._fp.close)

    # --- Sync attribute passthrough ---

    def __getattr__(self, name):
        return getattr(self._fp, name)

    # --- Context manager ---

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.aclose()

    # --- Async iteration (line by line) ---

    def __aiter__(self):
        return self

    async def __anext__(self):
        line = await to_thread(self._fp.readline)
        if line:
            return line
        raise StopAsyncIteration

    def __repr__(self):
        return f"<AsyncFile wrapped={self._fp!r}>"


# --- Path helpers ---

def _async_method(fn):
    """Wrap a pathlib.Path method as an async method on asyncio.Path."""
    name = fn.__name__

    async def wrapper(self, *args, **kwargs):
        return await to_thread(getattr(self._path, name), *args, **kwargs)

    wrapper.__name__ = wrapper.__qualname__ = name
    wrapper.__doc__ = fn.__doc__
    return wrapper


def _async_method_path(fn):
    """Like _async_method but re-wraps pathlib.Path result as asyncio.Path."""
    name = fn.__name__

    async def wrapper(self, *args, **kwargs):
        result = await to_thread(getattr(self._path, name), *args, **kwargs)
        return Path(result)

    wrapper.__name__ = wrapper.__qualname__ = name
    wrapper.__doc__ = fn.__doc__
    return wrapper


_EXHAUSTED = object()


def _next_sync(it):
    """Call next(it), returning _EXHAUSTED instead of raising StopIteration.

    StopIteration cannot be raised into a Future, so we use a sentinel.
    """
    return next(it, _EXHAUSTED)


class _AsyncPathIterator:
    """Async iterator that lazily pumps a sync path iterator in a thread."""

    __slots__ = ('_it',)

    def __init__(self, sync_iterator):
        self._it = sync_iterator

    def __aiter__(self):
        return self

    async def __anext__(self):
        result = await to_thread(_next_sync, self._it)
        if result is _EXHAUSTED:
            raise StopAsyncIteration
        return Path(result)


class _AsyncWalkIterator:
    """Async iterator over walk() results, wrapping dirpath as asyncio.Path."""

    __slots__ = ('_it',)

    def __init__(self, sync_iterator):
        self._it = sync_iterator

    def __aiter__(self):
        return self

    async def __anext__(self):
        result = await to_thread(_next_sync, self._it)
        if result is _EXHAUSTED:
            raise StopAsyncIteration
        dirpath, dirnames, filenames = result
        return Path(dirpath), dirnames, filenames


class Path:
    """Async wrapper around pathlib.Path.

    All I/O methods are delegated to a thread via asyncio.to_thread().
    Non-I/O properties and methods are passed through directly.
    """

    __slots__ = ('_path',)

    def __init__(self, *args):
        if args and isinstance(args[0], pathlib.PurePath):
            self._path = pathlib.Path(args[0])
        else:
            self._path = pathlib.Path(*args)

    # --- Sync properties (no I/O, direct passthrough) ---

    @property
    def parts(self):
        return self._path.parts

    @property
    def drive(self):
        return self._path.drive

    @property
    def root(self):
        return self._path.root

    @property
    def anchor(self):
        return self._path.anchor

    @property
    def parent(self):
        return Path(self._path.parent)

    @property
    def parents(self):
        return tuple(Path(p) for p in self._path.parents)

    @property
    def name(self):
        return self._path.name

    @property
    def stem(self):
        return self._path.stem

    @property
    def suffix(self):
        return self._path.suffix

    @property
    def suffixes(self):
        return self._path.suffixes

    @property
    def parser(self):
        return self._path.parser

    # --- Sync methods (no I/O) ---

    def as_posix(self):
        return self._path.as_posix()

    def as_uri(self):
        return self._path.as_uri()

    def is_absolute(self):
        return self._path.is_absolute()

    def is_relative_to(self, other):
        return self._path.is_relative_to(other)

    def joinpath(self, *pathsegments):
        return Path(self._path.joinpath(*pathsegments))

    def match(self, pattern, **kwargs):
        return self._path.match(pattern, **kwargs)

    def full_match(self, pattern, **kwargs):
        return self._path.full_match(pattern, **kwargs)

    def relative_to(self, other, **kwargs):
        return Path(self._path.relative_to(other, **kwargs))

    def with_name(self, name):
        return Path(self._path.with_name(name))

    def with_stem(self, stem):
        return Path(self._path.with_stem(stem))

    def with_suffix(self, suffix):
        return Path(self._path.with_suffix(suffix))

    def with_segments(self, *pathsegments):
        return Path(self._path.with_segments(*pathsegments))

    def __truediv__(self, key):
        return Path(self._path / key)

    def __rtruediv__(self, key):
        return Path(key / self._path)

    def __str__(self):
        return str(self._path)

    def __repr__(self):
        return f"asyncio.Path({str(self._path)!r})"

    def __fspath__(self):
        return os.fspath(self._path)

    def __hash__(self):
        return hash(self._path)

    def __eq__(self, other):
        if isinstance(other, Path):
            return self._path == other._path
        return NotImplemented

    def __lt__(self, other):
        if isinstance(other, Path):
            return self._path < other._path
        return NotImplemented

    def __le__(self, other):
        if isinstance(other, Path):
            return self._path <= other._path
        return NotImplemented

    def __gt__(self, other):
        if isinstance(other, Path):
            return self._path > other._path
        return NotImplemented

    def __ge__(self, other):
        if isinstance(other, Path):
            return self._path >= other._path
        return NotImplemented

    # --- Async methods (I/O, delegated to thread) ---

    # File tests
    exists = _async_method(pathlib.Path.exists)
    is_file = _async_method(pathlib.Path.is_file)
    is_dir = _async_method(pathlib.Path.is_dir)
    is_symlink = _async_method(pathlib.Path.is_symlink)
    is_socket = _async_method(pathlib.Path.is_socket)
    is_fifo = _async_method(pathlib.Path.is_fifo)
    is_block_device = _async_method(pathlib.Path.is_block_device)
    is_char_device = _async_method(pathlib.Path.is_char_device)
    is_junction = _async_method(pathlib.Path.is_junction)
    is_mount = _async_method(pathlib.Path.is_mount)

    # Stat
    stat = _async_method(pathlib.Path.stat)
    lstat = _async_method(pathlib.Path.lstat)
    samefile = _async_method(pathlib.Path.samefile)

    # Permissions
    chmod = _async_method(pathlib.Path.chmod)
    lchmod = _async_method(pathlib.Path.lchmod)
    if hasattr(pathlib.Path, 'owner'):
        owner = _async_method(pathlib.Path.owner)
    if hasattr(pathlib.Path, 'group'):
        group = _async_method(pathlib.Path.group)

    # CRUD
    touch = _async_method(pathlib.Path.touch)
    mkdir = _async_method(pathlib.Path.mkdir)
    rmdir = _async_method(pathlib.Path.rmdir)
    unlink = _async_method(pathlib.Path.unlink)

    # Read/write shortcuts
    read_bytes = _async_method(pathlib.Path.read_bytes)
    read_text = _async_method(pathlib.Path.read_text)
    write_bytes = _async_method(pathlib.Path.write_bytes)
    write_text = _async_method(pathlib.Path.write_text)

    # Links
    if hasattr(pathlib.Path, 'symlink_to'):
        symlink_to = _async_method(pathlib.Path.symlink_to)
    if hasattr(pathlib.Path, 'hardlink_to'):
        hardlink_to = _async_method(pathlib.Path.hardlink_to)

    # Copy/move (3.14+)
    if hasattr(pathlib.Path, 'copy'):
        async def copy(self, target, **kwargs):
            result = await to_thread(self._path.copy, target, **kwargs)
            return Path(result)

        copy.__doc__ = pathlib.Path.copy.__doc__

    if hasattr(pathlib.Path, 'copy_into'):
        async def copy_into(self, target_dir, **kwargs):
            result = await to_thread(
                self._path.copy_into, target_dir, **kwargs)
            return Path(result)

        copy_into.__doc__ = pathlib.Path.copy_into.__doc__

    if hasattr(pathlib.Path, 'move'):
        async def move(self, target):
            result = await to_thread(self._path.move, target)
            return Path(result)

        move.__doc__ = pathlib.Path.move.__doc__

    if hasattr(pathlib.Path, 'move_into'):
        async def move_into(self, target_dir):
            result = await to_thread(self._path.move_into, target_dir)
            return Path(result)

        move_into.__doc__ = pathlib.Path.move_into.__doc__

    # Path-returning async methods
    rename = _async_method_path(pathlib.Path.rename)
    replace = _async_method_path(pathlib.Path.replace)
    resolve = _async_method_path(pathlib.Path.resolve)
    absolute = _async_method_path(pathlib.Path.absolute)
    expanduser = _async_method_path(pathlib.Path.expanduser)
    if hasattr(pathlib.Path, 'readlink'):
        readlink = _async_method_path(pathlib.Path.readlink)

    # Open
    async def open(self, mode='r', buffering=-1, encoding=None,
                   errors=None, newline=None):
        """Open the file pointed to by this path, returning an AsyncFile."""
        fp = await to_thread(
            self._path.open, mode, buffering, encoding, errors, newline)
        return AsyncFile(fp)

    # Class methods
    @classmethod
    async def cwd(cls):
        """Return a new async path pointing to the current working directory."""
        result = await to_thread(pathlib.Path.cwd)
        return cls(result)

    @classmethod
    async def home(cls):
        """Return a new async path pointing to the user's home directory."""
        result = await to_thread(pathlib.Path.home)
        return cls(result)

    # Async generators (return async iterators)
    def iterdir(self):
        """Return an async iterator of path objects of the directory contents."""
        return _AsyncPathIterator(self._path.iterdir())

    def glob(self, pattern, **kwargs):
        """Return an async iterator of paths matching the pattern."""
        return _AsyncPathIterator(self._path.glob(pattern, **kwargs))

    def rglob(self, pattern, **kwargs):
        """Return an async iterator of paths matching the pattern recursively."""
        return _AsyncPathIterator(self._path.rglob(pattern, **kwargs))

    def walk(self, top_down=True, on_error=None, follow_symlinks=False):
        """Walk the directory tree, returning an async iterator."""
        return _AsyncWalkIterator(
            self._path.walk(top_down, on_error, follow_symlinks))
