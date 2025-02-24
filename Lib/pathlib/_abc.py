"""
Abstract base classes for rich path objects.

This module is published as a PyPI package called "pathlib-abc".

This module is also a *PRIVATE* part of the Python standard library, where
it's developed alongside pathlib. If it finds success and maturity as a PyPI
package, it could become a public part of the standard library.

Three base classes are defined here -- JoinablePath, ReadablePath and
WritablePath.
"""

from abc import ABC, abstractmethod
from glob import _PathGlobber, _no_recurse_symlinks
from pathlib import PurePath, Path
from pathlib._os import magic_open, ensure_distinct_paths, copy_file


def _explode_path(path):
    """
    Split the path into a 2-tuple (anchor, parts), where *anchor* is the
    uppermost parent of the path (equivalent to path.parents[-1]), and
    *parts* is a reversed list of parts following the anchor.
    """
    split = path.parser.split
    path = str(path)
    parent, name = split(path)
    names = []
    while path != parent:
        names.append(name)
        path = parent
        parent, name = split(path)
    return path, names


class JoinablePath(ABC):
    """Abstract base class for pure path objects.

    This class *does not* provide several magic methods that are defined in
    its implementation PurePath. They are: __init__, __fspath__, __bytes__,
    __reduce__, __hash__, __eq__, __lt__, __le__, __gt__, __ge__.
    """
    __slots__ = ()

    @property
    @abstractmethod
    def parser(self):
        """Implementation of pathlib._types.Parser used for low-level path
        parsing and manipulation.
        """
        raise NotImplementedError

    @abstractmethod
    def with_segments(self, *pathsegments):
        """Construct a new path object from any number of path-like objects.
        Subclasses may override this method to customize how new path objects
        are created from methods like `iterdir()`.
        """
        raise NotImplementedError

    @abstractmethod
    def __str__(self):
        """Return the string representation of the path, suitable for
        passing to system calls."""
        raise NotImplementedError

    @property
    def anchor(self):
        """The concatenation of the drive and root, or ''."""
        return _explode_path(self)[0]

    @property
    def name(self):
        """The final path component, if any."""
        return self.parser.split(str(self))[1]

    @property
    def suffix(self):
        """
        The final component's last suffix, if any.

        This includes the leading period. For example: '.txt'
        """
        return self.parser.splitext(self.name)[1]

    @property
    def suffixes(self):
        """
        A list of the final component's suffixes, if any.

        These include the leading periods. For example: ['.tar', '.gz']
        """
        split = self.parser.splitext
        stem, suffix = split(self.name)
        suffixes = []
        while suffix:
            suffixes.append(suffix)
            stem, suffix = split(stem)
        return suffixes[::-1]

    @property
    def stem(self):
        """The final path component, minus its last suffix."""
        return self.parser.splitext(self.name)[0]

    def with_name(self, name):
        """Return a new path with the file name changed."""
        split = self.parser.split
        if split(name)[0]:
            raise ValueError(f"Invalid name {name!r}")
        return self.with_segments(split(str(self))[0], name)

    def with_stem(self, stem):
        """Return a new path with the stem changed."""
        suffix = self.suffix
        if not suffix:
            return self.with_name(stem)
        elif not stem:
            # If the suffix is non-empty, we can't make the stem empty.
            raise ValueError(f"{self!r} has a non-empty suffix")
        else:
            return self.with_name(stem + suffix)

    def with_suffix(self, suffix):
        """Return a new path with the file suffix changed.  If the path
        has no suffix, add given suffix.  If the given suffix is an empty
        string, remove the suffix from the path.
        """
        stem = self.stem
        if not stem:
            # If the stem is empty, we can't make the suffix non-empty.
            raise ValueError(f"{self!r} has an empty name")
        elif suffix and not suffix.startswith('.'):
            raise ValueError(f"Invalid suffix {suffix!r}")
        else:
            return self.with_name(stem + suffix)

    @property
    def parts(self):
        """An object providing sequence-like access to the
        components in the filesystem path."""
        anchor, parts = _explode_path(self)
        if anchor:
            parts.append(anchor)
        return tuple(reversed(parts))

    def joinpath(self, *pathsegments):
        """Combine this path with one or several arguments, and return a
        new path representing either a subpath (if all arguments are relative
        paths) or a totally different path (if one of the arguments is
        anchored).
        """
        return self.with_segments(str(self), *pathsegments)

    def __truediv__(self, key):
        try:
            return self.with_segments(str(self), key)
        except TypeError:
            return NotImplemented

    def __rtruediv__(self, key):
        try:
            return self.with_segments(key, str(self))
        except TypeError:
            return NotImplemented

    @property
    def parent(self):
        """The logical parent of the path."""
        path = str(self)
        parent = self.parser.split(path)[0]
        if path != parent:
            return self.with_segments(parent)
        return self

    @property
    def parents(self):
        """A sequence of this path's logical parents."""
        split = self.parser.split
        path = str(self)
        parent = split(path)[0]
        parents = []
        while path != parent:
            parents.append(self.with_segments(parent))
            path = parent
            parent = split(path)[0]
        return tuple(parents)

    def full_match(self, pattern, *, case_sensitive=None):
        """
        Return True if this path matches the given glob-style pattern. The
        pattern is matched against the entire path.
        """
        if not hasattr(pattern, 'with_segments'):
            pattern = self.with_segments(pattern)
        if case_sensitive is None:
            case_sensitive = self.parser.normcase('Aa') == 'Aa'
        globber = _PathGlobber(pattern.parser.sep, case_sensitive, recursive=True)
        match = globber.compile(str(pattern))
        return match(str(self)) is not None


class ReadablePath(JoinablePath):
    """Abstract base class for readable path objects.

    The Path class implements this ABC for local filesystem paths. Users may
    create subclasses to implement readable virtual filesystem paths, such as
    paths in archive files or on remote storage systems.
    """
    __slots__ = ()

    @property
    @abstractmethod
    def info(self):
        """
        A PathInfo object that exposes the file type and other file attributes
        of this path.
        """
        raise NotImplementedError

    def exists(self, *, follow_symlinks=True):
        """
        Whether this path exists.

        This method normally follows symlinks; to check whether a symlink exists,
        add the argument follow_symlinks=False.
        """
        info = self.joinpath().info
        return info.exists(follow_symlinks=follow_symlinks)

    def is_dir(self, *, follow_symlinks=True):
        """
        Whether this path is a directory.
        """
        info = self.joinpath().info
        return info.is_dir(follow_symlinks=follow_symlinks)

    def is_file(self, *, follow_symlinks=True):
        """
        Whether this path is a regular file (also True for symlinks pointing
        to regular files).
        """
        info = self.joinpath().info
        return info.is_file(follow_symlinks=follow_symlinks)

    def is_symlink(self):
        """
        Whether this path is a symbolic link.
        """
        info = self.joinpath().info
        return info.is_symlink()

    @abstractmethod
    def __open_rb__(self, buffering=-1):
        """
        Open the file pointed to by this path for reading in binary mode and
        return a file object, like open(mode='rb').
        """
        raise NotImplementedError

    def read_bytes(self):
        """
        Open the file in bytes mode, read it, and close the file.
        """
        with magic_open(self, mode='rb', buffering=0) as f:
            return f.read()

    def read_text(self, encoding=None, errors=None, newline=None):
        """
        Open the file in text mode, read it, and close the file.
        """
        with magic_open(self, mode='r', encoding=encoding, errors=errors, newline=newline) as f:
            return f.read()

    @abstractmethod
    def iterdir(self):
        """Yield path objects of the directory contents.

        The children are yielded in arbitrary order, and the
        special entries '.' and '..' are not included.
        """
        raise NotImplementedError

    def glob(self, pattern, *, case_sensitive=None, recurse_symlinks=True):
        """Iterate over this subtree and yield all existing files (of any
        kind, including directories) matching the given relative pattern.
        """
        if not hasattr(pattern, 'with_segments'):
            pattern = self.with_segments(pattern)
        anchor, parts = _explode_path(pattern)
        if anchor:
            raise NotImplementedError("Non-relative patterns are unsupported")
        case_sensitive_default = self.parser.normcase('Aa') == 'Aa'
        if case_sensitive is None:
            case_sensitive = case_sensitive_default
            case_pedantic = False
        else:
            case_pedantic = case_sensitive_default != case_sensitive
        recursive = True if recurse_symlinks else _no_recurse_symlinks
        globber = _PathGlobber(self.parser.sep, case_sensitive, case_pedantic, recursive)
        select = globber.selector(parts)
        return select(self.joinpath(''))

    def walk(self, top_down=True, on_error=None, follow_symlinks=False):
        """Walk the directory tree from this directory, similar to os.walk()."""
        paths = [self]
        while paths:
            path = paths.pop()
            if isinstance(path, tuple):
                yield path
                continue
            dirnames = []
            filenames = []
            if not top_down:
                paths.append((path, dirnames, filenames))
            try:
                for child in path.iterdir():
                    if child.info.is_dir(follow_symlinks=follow_symlinks):
                        if not top_down:
                            paths.append(child)
                        dirnames.append(child.name)
                    else:
                        filenames.append(child.name)
            except OSError as error:
                if on_error is not None:
                    on_error(error)
                if not top_down:
                    while not isinstance(paths.pop(), tuple):
                        pass
                continue
            if top_down:
                yield path, dirnames, filenames
                paths += [path.joinpath(d) for d in reversed(dirnames)]

    @abstractmethod
    def readlink(self):
        """
        Return the path to which the symbolic link points.
        """
        raise NotImplementedError

    def copy(self, target, follow_symlinks=True, dirs_exist_ok=False,
             preserve_metadata=False):
        """
        Recursively copy this file or directory tree to the given destination.
        """
        if not hasattr(target, 'with_segments'):
            target = self.with_segments(target)
        ensure_distinct_paths(self, target)
        copy_file(self, target, follow_symlinks, dirs_exist_ok, preserve_metadata)
        return target.joinpath()  # Empty join to ensure fresh metadata.

    def copy_into(self, target_dir, *, follow_symlinks=True,
                  dirs_exist_ok=False, preserve_metadata=False):
        """
        Copy this file or directory tree into the given existing directory.
        """
        name = self.name
        if not name:
            raise ValueError(f"{self!r} has an empty name")
        elif hasattr(target_dir, 'with_segments'):
            target = target_dir / name
        else:
            target = self.with_segments(target_dir, name)
        return self.copy(target, follow_symlinks=follow_symlinks,
                         dirs_exist_ok=dirs_exist_ok,
                         preserve_metadata=preserve_metadata)


class WritablePath(JoinablePath):
    """Abstract base class for writable path objects.

    The Path class implements this ABC for local filesystem paths. Users may
    create subclasses to implement writable virtual filesystem paths, such as
    paths in archive files or on remote storage systems.
    """
    __slots__ = ()

    @abstractmethod
    def symlink_to(self, target, target_is_directory=False):
        """
        Make this path a symlink pointing to the target path.
        Note the order of arguments (link, target) is the reverse of os.symlink.
        """
        raise NotImplementedError

    @abstractmethod
    def mkdir(self, mode=0o777, parents=False, exist_ok=False):
        """
        Create a new directory at this given path.
        """
        raise NotImplementedError

    @abstractmethod
    def __open_wb__(self, buffering=-1):
        """
        Open the file pointed to by this path for writing in binary mode and
        return a file object, like open(mode='wb').
        """
        raise NotImplementedError

    def write_bytes(self, data):
        """
        Open the file in bytes mode, write to it, and close the file.
        """
        # type-check for the buffer interface before truncating the file
        view = memoryview(data)
        with magic_open(self, mode='wb') as f:
            return f.write(view)

    def write_text(self, data, encoding=None, errors=None, newline=None):
        """
        Open the file in text mode, write to it, and close the file.
        """
        if not isinstance(data, str):
            raise TypeError('data must be str, not %s' %
                            data.__class__.__name__)
        with magic_open(self, mode='w', encoding=encoding, errors=errors, newline=newline) as f:
            return f.write(data)

    def _write_info(self, info, follow_symlinks=True):
        """
        Write the given PathInfo to this path.
        """
        pass


JoinablePath.register(PurePath)
ReadablePath.register(Path)
WritablePath.register(Path)
