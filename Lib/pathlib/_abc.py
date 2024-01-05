import functools
import ntpath
import posixpath
import sys
from _collections_abc import Sequence
from errno import ENOENT, ENOTDIR, EBADF, ELOOP, EINVAL
from itertools import chain
from stat import S_ISDIR, S_ISLNK, S_ISREG, S_ISSOCK, S_ISBLK, S_ISCHR, S_ISFIFO

#
# Internals
#

# Maximum number of symlinks to follow in PathBase.resolve()
_MAX_SYMLINKS = 40

# Reference for Windows paths can be found at
# https://learn.microsoft.com/en-gb/windows/win32/fileio/naming-a-file .
_WIN_RESERVED_NAMES = frozenset(
    {'CON', 'PRN', 'AUX', 'NUL', 'CONIN$', 'CONOUT$'} |
    {f'COM{c}' for c in '123456789\xb9\xb2\xb3'} |
    {f'LPT{c}' for c in '123456789\xb9\xb2\xb3'}
)

_WINERROR_NOT_READY = 21  # drive exists but is not accessible
_WINERROR_INVALID_NAME = 123  # fix for bpo-35306
_WINERROR_CANT_RESOLVE_FILENAME = 1921  # broken symlink pointing to itself

# EBADF - guard against macOS `stat` throwing EBADF
_IGNORED_ERRNOS = (ENOENT, ENOTDIR, EBADF, ELOOP)

_IGNORED_WINERRORS = (
    _WINERROR_NOT_READY,
    _WINERROR_INVALID_NAME,
    _WINERROR_CANT_RESOLVE_FILENAME)

def _ignore_error(exception):
    return (getattr(exception, 'errno', None) in _IGNORED_ERRNOS or
            getattr(exception, 'winerror', None) in _IGNORED_WINERRORS)


@functools.cache
def _is_case_sensitive(pathmod):
    return pathmod.normcase('Aa') == 'Aa'

#
# Globbing helpers
#

re = glob = None


@functools.lru_cache(maxsize=256)
def _compile_pattern(pat, sep, case_sensitive):
    """Compile given glob pattern to a re.Pattern object (observing case
    sensitivity)."""
    global re, glob
    if re is None:
        import re, glob

    flags = re.NOFLAG if case_sensitive else re.IGNORECASE
    regex = glob.translate(pat, recursive=True, include_hidden=True, seps=sep)
    # The string representation of an empty path is a single dot ('.'). Empty
    # paths shouldn't match wildcards, so we consume it with an atomic group.
    regex = r'(\.\Z)?+' + regex
    return re.compile(regex, flags=flags).match


def _select_children(parent_paths, dir_only, follow_symlinks, match):
    """Yield direct children of given paths, filtering by name and type."""
    if follow_symlinks is None:
        follow_symlinks = True
    for parent_path in parent_paths:
        try:
            # We must close the scandir() object before proceeding to
            # avoid exhausting file descriptors when globbing deep trees.
            with parent_path._scandir() as scandir_it:
                entries = list(scandir_it)
        except OSError:
            pass
        else:
            for entry in entries:
                if dir_only:
                    try:
                        if not entry.is_dir(follow_symlinks=follow_symlinks):
                            continue
                    except OSError:
                        continue
                if match(entry.name):
                    yield parent_path._make_child_entry(entry)


def _select_recursive(parent_paths, dir_only, follow_symlinks):
    """Yield given paths and all their subdirectories, recursively."""
    if follow_symlinks is None:
        follow_symlinks = False
    for parent_path in parent_paths:
        paths = [parent_path]
        while paths:
            path = paths.pop()
            yield path
            try:
                # We must close the scandir() object before proceeding to
                # avoid exhausting file descriptors when globbing deep trees.
                with path._scandir() as scandir_it:
                    entries = list(scandir_it)
            except OSError:
                pass
            else:
                for entry in entries:
                    try:
                        if entry.is_dir(follow_symlinks=follow_symlinks):
                            paths.append(path._make_child_entry(entry))
                            continue
                    except OSError:
                        pass
                    if not dir_only:
                        yield path._make_child_entry(entry)


def _select_unique(paths):
    """Yields the given paths, filtering out duplicates."""
    yielded = set()
    try:
        for path in paths:
            path_str = str(path)
            if path_str not in yielded:
                yield path
                yielded.add(path_str)
    finally:
        yielded.clear()


class UnsupportedOperation(NotImplementedError):
    """An exception that is raised when an unsupported operation is called on
    a path object.
    """
    pass


class _PathParents(Sequence):
    """This object provides sequence-like access to the logical ancestors
    of a path.  Don't try to construct it yourself."""
    __slots__ = ('_path', '_drv', '_root', '_tail')

    def __init__(self, path):
        self._path = path
        self._drv = path.drive
        self._root = path.root
        self._tail = path._tail

    def __len__(self):
        return len(self._tail)

    def __getitem__(self, idx):
        if isinstance(idx, slice):
            return tuple(self[i] for i in range(*idx.indices(len(self))))

        if idx >= len(self) or idx < -len(self):
            raise IndexError(idx)
        if idx < 0:
            idx += len(self)
        return self._path._from_parsed_parts(self._drv, self._root,
                                             self._tail[:-idx - 1])

    def __repr__(self):
        return "<{}.parents>".format(type(self._path).__name__)


class PurePathBase:
    """Base class for pure path objects.

    This class *does not* provide several magic methods that are defined in
    its subclass PurePath. They are: __fspath__, __bytes__, __reduce__,
    __hash__, __eq__, __lt__, __le__, __gt__, __ge__. Its initializer and path
    joining methods accept only strings, not os.PathLike objects more broadly.
    """

    __slots__ = (
        # The `_raw_paths` slot stores unnormalized string paths. This is set
        # in the `__init__()` method.
        '_raw_paths',

        # The `_drv`, `_root` and `_tail_cached` slots store parsed and
        # normalized parts of the path. They are set when any of the `drive`,
        # `root` or `_tail` properties are accessed for the first time. The
        # three-part division corresponds to the result of
        # `os.path.splitroot()`, except that the tail is further split on path
        # separators (i.e. it is a list of strings), and that the root and
        # tail are normalized.
        '_drv', '_root', '_tail_cached',

        # The `_str` slot stores the string representation of the path,
        # computed from the drive, root and tail when `__str__()` is called
        # for the first time. It's used to implement `_str_normcase`
        '_str',

        # The '_resolving' slot stores a boolean indicating whether the path
        # is being processed by `PathBase.resolve()`. This prevents duplicate
        # work from occurring when `resolve()` calls `stat()` or `readlink()`.
        '_resolving',
    )
    pathmod = posixpath

    def __init__(self, *paths):
        self._raw_paths = paths
        self._resolving = False

    def with_segments(self, *pathsegments):
        """Construct a new path object from any number of path-like objects.
        Subclasses may override this method to customize how new path objects
        are created from methods like `iterdir()`.
        """
        return type(self)(*pathsegments)

    @classmethod
    def _parse_path(cls, path):
        if not path:
            return '', '', []
        sep = cls.pathmod.sep
        altsep = cls.pathmod.altsep
        if altsep:
            path = path.replace(altsep, sep)
        drv, root, rel = cls.pathmod.splitroot(path)
        if not root and drv.startswith(sep) and not drv.endswith(sep):
            drv_parts = drv.split(sep)
            if len(drv_parts) == 4 and drv_parts[2] not in '?.':
                # e.g. //server/share
                root = sep
            elif len(drv_parts) == 6:
                # e.g. //?/unc/server/share
                root = sep
        parsed = [sys.intern(str(x)) for x in rel.split(sep) if x and x != '.']
        return drv, root, parsed

    def _load_parts(self):
        paths = self._raw_paths
        if len(paths) == 0:
            path = ''
        elif len(paths) == 1:
            path = paths[0]
        else:
            path = self.pathmod.join(*paths)
        drv, root, tail = self._parse_path(path)
        self._drv = drv
        self._root = root
        self._tail_cached = tail

    def _from_parsed_parts(self, drv, root, tail):
        path_str = self._format_parsed_parts(drv, root, tail)
        path = self.with_segments(path_str)
        path._str = path_str or '.'
        path._drv = drv
        path._root = root
        path._tail_cached = tail
        return path

    @classmethod
    def _format_parsed_parts(cls, drv, root, tail):
        if drv or root:
            return drv + root + cls.pathmod.sep.join(tail)
        elif tail and cls.pathmod.splitdrive(tail[0])[0]:
            tail = ['.'] + tail
        return cls.pathmod.sep.join(tail)

    def __str__(self):
        """Return the string representation of the path, suitable for
        passing to system calls."""
        try:
            return self._str
        except AttributeError:
            self._str = self._format_parsed_parts(self.drive, self.root,
                                                  self._tail) or '.'
            return self._str

    def as_posix(self):
        """Return the string representation of the path with forward (/)
        slashes."""
        return str(self).replace(self.pathmod.sep, '/')

    @property
    def drive(self):
        """The drive prefix (letter or UNC path), if any."""
        try:
            return self._drv
        except AttributeError:
            self._load_parts()
            return self._drv

    @property
    def root(self):
        """The root of the path, if any."""
        try:
            return self._root
        except AttributeError:
            self._load_parts()
            return self._root

    @property
    def _tail(self):
        try:
            return self._tail_cached
        except AttributeError:
            self._load_parts()
            return self._tail_cached

    @property
    def anchor(self):
        """The concatenation of the drive and root, or ''."""
        anchor = self.drive + self.root
        return anchor

    @property
    def name(self):
        """The final path component, if any."""
        tail = self._tail
        if not tail:
            return ''
        return tail[-1]

    @property
    def suffix(self):
        """
        The final component's last suffix, if any.

        This includes the leading period. For example: '.txt'
        """
        name = self.name
        i = name.rfind('.')
        if 0 < i < len(name) - 1:
            return name[i:]
        else:
            return ''

    @property
    def suffixes(self):
        """
        A list of the final component's suffixes, if any.

        These include the leading periods. For example: ['.tar', '.gz']
        """
        name = self.name
        if name.endswith('.'):
            return []
        name = name.lstrip('.')
        return ['.' + suffix for suffix in name.split('.')[1:]]

    @property
    def stem(self):
        """The final path component, minus its last suffix."""
        name = self.name
        i = name.rfind('.')
        if 0 < i < len(name) - 1:
            return name[:i]
        else:
            return name

    def with_name(self, name):
        """Return a new path with the file name changed."""
        m = self.pathmod
        if not name or m.sep in name or (m.altsep and m.altsep in name) or name == '.':
            raise ValueError(f"Invalid name {name!r}")
        tail = self._tail.copy()
        if not tail:
            raise ValueError(f"{self!r} has an empty name")
        tail[-1] = name
        return self._from_parsed_parts(self.drive, self.root, tail)

    def with_stem(self, stem):
        """Return a new path with the stem changed."""
        return self.with_name(stem + self.suffix)

    def with_suffix(self, suffix):
        """Return a new path with the file suffix changed.  If the path
        has no suffix, add given suffix.  If the given suffix is an empty
        string, remove the suffix from the path.
        """
        if not suffix:
            return self.with_name(self.stem)
        elif suffix.startswith('.') and len(suffix) > 1:
            return self.with_name(self.stem + suffix)
        else:
            raise ValueError(f"Invalid suffix {suffix!r}")

    def relative_to(self, other, *, walk_up=False):
        """Return the relative path to another path identified by the passed
        arguments.  If the operation is not possible (because this is not
        related to the other path), raise ValueError.

        The *walk_up* parameter controls whether `..` may be used to resolve
        the path.
        """
        if not isinstance(other, PurePathBase):
            other = self.with_segments(other)
        for step, path in enumerate(chain([other], other.parents)):
            if path == self or path in self.parents:
                break
            elif not walk_up:
                raise ValueError(f"{str(self)!r} is not in the subpath of {str(other)!r}")
            elif path.name == '..':
                raise ValueError(f"'..' segment in {str(other)!r} cannot be walked")
        else:
            raise ValueError(f"{str(self)!r} and {str(other)!r} have different anchors")
        parts = ['..'] * step + self._tail[len(path._tail):]
        return self._from_parsed_parts('', '', parts)

    def is_relative_to(self, other):
        """Return True if the path is relative to another path or False.
        """
        if not isinstance(other, PurePathBase):
            other = self.with_segments(other)
        return other == self or other in self.parents

    @property
    def parts(self):
        """An object providing sequence-like access to the
        components in the filesystem path."""
        if self.drive or self.root:
            return (self.drive + self.root,) + tuple(self._tail)
        else:
            return tuple(self._tail)

    def joinpath(self, *pathsegments):
        """Combine this path with one or several arguments, and return a
        new path representing either a subpath (if all arguments are relative
        paths) or a totally different path (if one of the arguments is
        anchored).
        """
        return self.with_segments(*self._raw_paths, *pathsegments)

    def __truediv__(self, key):
        try:
            return self.joinpath(key)
        except TypeError:
            return NotImplemented

    def __rtruediv__(self, key):
        try:
            return self.with_segments(key, *self._raw_paths)
        except TypeError:
            return NotImplemented

    @property
    def parent(self):
        """The logical parent of the path."""
        drv = self.drive
        root = self.root
        tail = self._tail
        if not tail:
            return self
        path = self._from_parsed_parts(drv, root, tail[:-1])
        path._resolving = self._resolving
        return path

    @property
    def parents(self):
        """A sequence of this path's logical parents."""
        # The value of this property should not be cached on the path object,
        # as doing so would introduce a reference cycle.
        return _PathParents(self)

    def is_absolute(self):
        """True if the path is absolute (has both a root and, if applicable,
        a drive)."""
        if self.pathmod is ntpath:
            # ntpath.isabs() is defective - see GH-44626.
            return bool(self.drive and self.root)
        elif self.pathmod is posixpath:
            # Optimization: work with raw paths on POSIX.
            for path in self._raw_paths:
                if path.startswith('/'):
                    return True
            return False
        else:
            return self.pathmod.isabs(str(self))

    def is_reserved(self):
        """Return True if the path contains one of the special names reserved
        by the system, if any."""
        if self.pathmod is posixpath or not self._tail:
            return False

        # NOTE: the rules for reserved names seem somewhat complicated
        # (e.g. r"..\NUL" is reserved but not r"foo\NUL" if "foo" does not
        # exist). We err on the side of caution and return True for paths
        # which are not considered reserved by Windows.
        if self.drive.startswith('\\\\'):
            # UNC paths are never reserved.
            return False
        name = self._tail[-1].partition('.')[0].partition(':')[0].rstrip(' ')
        return name.upper() in _WIN_RESERVED_NAMES

    def match(self, path_pattern, *, case_sensitive=None):
        """
        Return True if this path matches the given pattern.
        """
        if not isinstance(path_pattern, PurePathBase):
            path_pattern = self.with_segments(path_pattern)
        if case_sensitive is None:
            case_sensitive = _is_case_sensitive(self.pathmod)
        sep = path_pattern.pathmod.sep
        pattern_str = str(path_pattern)
        if path_pattern.drive or path_pattern.root:
            pass
        elif path_pattern._tail:
            pattern_str = f'**{sep}{pattern_str}'
        else:
            raise ValueError("empty pattern")
        match = _compile_pattern(pattern_str, sep, case_sensitive)
        return match(str(self)) is not None



class PathBase(PurePathBase):
    """Base class for concrete path objects.

    This class provides dummy implementations for many methods that derived
    classes can override selectively; the default implementations raise
    UnsupportedOperation. The most basic methods, such as stat() and open(),
    directly raise UnsupportedOperation; these basic methods are called by
    other methods such as is_dir() and read_text().

    The Path class derives this class to implement local filesystem paths.
    Users may derive their own classes to implement virtual filesystem paths,
    such as paths in archive files or on remote storage systems.
    """
    __slots__ = ()

    @classmethod
    def _unsupported(cls, method_name):
        msg = f"{cls.__name__}.{method_name}() is unsupported"
        raise UnsupportedOperation(msg)

    def stat(self, *, follow_symlinks=True):
        """
        Return the result of the stat() system call on this path, like
        os.stat() does.
        """
        self._unsupported("stat")

    def lstat(self):
        """
        Like stat(), except if the path points to a symlink, the symlink's
        status information is returned, rather than its target's.
        """
        return self.stat(follow_symlinks=False)


    # Convenience functions for querying the stat results

    def exists(self, *, follow_symlinks=True):
        """
        Whether this path exists.

        This method normally follows symlinks; to check whether a symlink exists,
        add the argument follow_symlinks=False.
        """
        try:
            self.stat(follow_symlinks=follow_symlinks)
        except OSError as e:
            if not _ignore_error(e):
                raise
            return False
        except ValueError:
            # Non-encodable path
            return False
        return True

    def is_dir(self, *, follow_symlinks=True):
        """
        Whether this path is a directory.
        """
        try:
            return S_ISDIR(self.stat(follow_symlinks=follow_symlinks).st_mode)
        except OSError as e:
            if not _ignore_error(e):
                raise
            # Path doesn't exist or is a broken symlink
            # (see http://web.archive.org/web/20200623061726/https://bitbucket.org/pitrou/pathlib/issues/12/ )
            return False
        except ValueError:
            # Non-encodable path
            return False

    def is_file(self, *, follow_symlinks=True):
        """
        Whether this path is a regular file (also True for symlinks pointing
        to regular files).
        """
        try:
            return S_ISREG(self.stat(follow_symlinks=follow_symlinks).st_mode)
        except OSError as e:
            if not _ignore_error(e):
                raise
            # Path doesn't exist or is a broken symlink
            # (see http://web.archive.org/web/20200623061726/https://bitbucket.org/pitrou/pathlib/issues/12/ )
            return False
        except ValueError:
            # Non-encodable path
            return False

    def is_mount(self):
        """
        Check if this path is a mount point
        """
        # Need to exist and be a dir
        if not self.exists() or not self.is_dir():
            return False

        try:
            parent_dev = self.parent.stat().st_dev
        except OSError:
            return False

        dev = self.stat().st_dev
        if dev != parent_dev:
            return True
        ino = self.stat().st_ino
        parent_ino = self.parent.stat().st_ino
        return ino == parent_ino

    def is_symlink(self):
        """
        Whether this path is a symbolic link.
        """
        try:
            return S_ISLNK(self.lstat().st_mode)
        except OSError as e:
            if not _ignore_error(e):
                raise
            # Path doesn't exist
            return False
        except ValueError:
            # Non-encodable path
            return False

    def is_junction(self):
        """
        Whether this path is a junction.
        """
        # Junctions are a Windows-only feature, not present in POSIX nor the
        # majority of virtual filesystems. There is no cross-platform idiom
        # to check for junctions (using stat().st_mode).
        return False

    def is_block_device(self):
        """
        Whether this path is a block device.
        """
        try:
            return S_ISBLK(self.stat().st_mode)
        except OSError as e:
            if not _ignore_error(e):
                raise
            # Path doesn't exist or is a broken symlink
            # (see http://web.archive.org/web/20200623061726/https://bitbucket.org/pitrou/pathlib/issues/12/ )
            return False
        except ValueError:
            # Non-encodable path
            return False

    def is_char_device(self):
        """
        Whether this path is a character device.
        """
        try:
            return S_ISCHR(self.stat().st_mode)
        except OSError as e:
            if not _ignore_error(e):
                raise
            # Path doesn't exist or is a broken symlink
            # (see http://web.archive.org/web/20200623061726/https://bitbucket.org/pitrou/pathlib/issues/12/ )
            return False
        except ValueError:
            # Non-encodable path
            return False

    def is_fifo(self):
        """
        Whether this path is a FIFO.
        """
        try:
            return S_ISFIFO(self.stat().st_mode)
        except OSError as e:
            if not _ignore_error(e):
                raise
            # Path doesn't exist or is a broken symlink
            # (see http://web.archive.org/web/20200623061726/https://bitbucket.org/pitrou/pathlib/issues/12/ )
            return False
        except ValueError:
            # Non-encodable path
            return False

    def is_socket(self):
        """
        Whether this path is a socket.
        """
        try:
            return S_ISSOCK(self.stat().st_mode)
        except OSError as e:
            if not _ignore_error(e):
                raise
            # Path doesn't exist or is a broken symlink
            # (see http://web.archive.org/web/20200623061726/https://bitbucket.org/pitrou/pathlib/issues/12/ )
            return False
        except ValueError:
            # Non-encodable path
            return False

    def samefile(self, other_path):
        """Return whether other_path is the same or not as this file
        (as returned by os.path.samefile()).
        """
        st = self.stat()
        try:
            other_st = other_path.stat()
        except AttributeError:
            other_st = self.with_segments(other_path).stat()
        return (st.st_ino == other_st.st_ino and
                st.st_dev == other_st.st_dev)

    def open(self, mode='r', buffering=-1, encoding=None,
             errors=None, newline=None):
        """
        Open the file pointed by this path and return a file object, as
        the built-in open() function does.
        """
        self._unsupported("open")

    def read_bytes(self):
        """
        Open the file in bytes mode, read it, and close the file.
        """
        with self.open(mode='rb') as f:
            return f.read()

    def read_text(self, encoding=None, errors=None, newline=None):
        """
        Open the file in text mode, read it, and close the file.
        """
        with self.open(mode='r', encoding=encoding, errors=errors, newline=newline) as f:
            return f.read()

    def write_bytes(self, data):
        """
        Open the file in bytes mode, write to it, and close the file.
        """
        # type-check for the buffer interface before truncating the file
        view = memoryview(data)
        with self.open(mode='wb') as f:
            return f.write(view)

    def write_text(self, data, encoding=None, errors=None, newline=None):
        """
        Open the file in text mode, write to it, and close the file.
        """
        if not isinstance(data, str):
            raise TypeError('data must be str, not %s' %
                            data.__class__.__name__)
        with self.open(mode='w', encoding=encoding, errors=errors, newline=newline) as f:
            return f.write(data)

    def iterdir(self):
        """Yield path objects of the directory contents.

        The children are yielded in arbitrary order, and the
        special entries '.' and '..' are not included.
        """
        self._unsupported("iterdir")

    def _scandir(self):
        # Emulate os.scandir(), which returns an object that can be used as a
        # context manager. This method is called by walk() and glob().
        from contextlib import nullcontext
        return nullcontext(self.iterdir())

    def _make_child_entry(self, entry):
        # Transform an entry yielded from _scandir() into a path object.
        return entry

    def _make_child_relpath(self, name):
        path_str = str(self)
        tail = self._tail
        if tail:
            path_str = f'{path_str}{self.pathmod.sep}{name}'
        elif path_str != '.':
            path_str = f'{path_str}{name}'
        else:
            path_str = name
        path = self.with_segments(path_str)
        path._str = path_str
        path._drv = self.drive
        path._root = self.root
        path._tail_cached = tail + [name]
        return path

    def glob(self, pattern, *, case_sensitive=None, follow_symlinks=None):
        """Iterate over this subtree and yield all existing files (of any
        kind, including directories) matching the given relative pattern.
        """
        path_pattern = self.with_segments(pattern)
        if path_pattern.drive or path_pattern.root:
            raise NotImplementedError("Non-relative patterns are unsupported")
        elif not path_pattern._tail:
            raise ValueError("Unacceptable pattern: {!r}".format(pattern))

        pattern_parts = path_pattern._tail.copy()
        if pattern[-1] in (self.pathmod.sep, self.pathmod.altsep):
            # GH-65238: pathlib doesn't preserve trailing slash. Add it back.
            pattern_parts.append('')

        if case_sensitive is None:
            # TODO: evaluate case-sensitivity of each directory in _select_children().
            case_sensitive = _is_case_sensitive(self.pathmod)

        # If symlinks are handled consistently, and the pattern does not
        # contain '..' components, then we can use a 'walk-and-match' strategy
        # when expanding '**' wildcards. When a '**' wildcard is encountered,
        # all following pattern parts are immediately consumed and used to
        # build a `re.Pattern` object. This pattern is used to filter the
        # recursive walk. As a result, pattern parts following a '**' wildcard
        # do not perform any filesystem access, which can be much faster!
        filter_paths = follow_symlinks is not None and '..' not in pattern_parts
        deduplicate_paths = False
        sep = self.pathmod.sep
        paths = iter([self] if self.is_dir() else [])
        part_idx = 0
        while part_idx < len(pattern_parts):
            part = pattern_parts[part_idx]
            part_idx += 1
            if part == '':
                # Trailing slash.
                pass
            elif part == '..':
                paths = (path._make_child_relpath('..') for path in paths)
            elif part == '**':
                # Consume adjacent '**' components.
                while part_idx < len(pattern_parts) and pattern_parts[part_idx] == '**':
                    part_idx += 1

                if filter_paths and part_idx < len(pattern_parts) and pattern_parts[part_idx] != '':
                    dir_only = pattern_parts[-1] == ''
                    paths = _select_recursive(paths, dir_only, follow_symlinks)

                    # Filter out paths that don't match pattern.
                    prefix_len = len(str(self._make_child_relpath('_'))) - 1
                    match = _compile_pattern(str(path_pattern), sep, case_sensitive)
                    paths = (path for path in paths if match(str(path), prefix_len))
                    return paths

                dir_only = part_idx < len(pattern_parts)
                paths = _select_recursive(paths, dir_only, follow_symlinks)
                if deduplicate_paths:
                    # De-duplicate if we've already seen a '**' component.
                    paths = _select_unique(paths)
                deduplicate_paths = True
            elif '**' in part:
                raise ValueError("Invalid pattern: '**' can only be an entire path component")
            else:
                dir_only = part_idx < len(pattern_parts)
                match = _compile_pattern(part, sep, case_sensitive)
                paths = _select_children(paths, dir_only, follow_symlinks, match)
        return paths

    def rglob(self, pattern, *, case_sensitive=None, follow_symlinks=None):
        """Recursively yield all existing files (of any kind, including
        directories) matching the given relative pattern, anywhere in
        this subtree.
        """
        return self.glob(
            f'**/{pattern}', case_sensitive=case_sensitive, follow_symlinks=follow_symlinks)

    def walk(self, top_down=True, on_error=None, follow_symlinks=False):
        """Walk the directory tree from this directory, similar to os.walk()."""
        paths = [self]

        while paths:
            path = paths.pop()
            if isinstance(path, tuple):
                yield path
                continue

            # We may not have read permission for self, in which case we can't
            # get a list of the files the directory contains. os.walk()
            # always suppressed the exception in that instance, rather than
            # blow up for a minor reason when (say) a thousand readable
            # directories are still left to visit. That logic is copied here.
            try:
                scandir_obj = path._scandir()
            except OSError as error:
                if on_error is not None:
                    on_error(error)
                continue

            with scandir_obj as scandir_it:
                dirnames = []
                filenames = []
                for entry in scandir_it:
                    try:
                        is_dir = entry.is_dir(follow_symlinks=follow_symlinks)
                    except OSError:
                        # Carried over from os.path.isdir().
                        is_dir = False

                    if is_dir:
                        dirnames.append(entry.name)
                    else:
                        filenames.append(entry.name)

            if top_down:
                yield path, dirnames, filenames
            else:
                paths.append((path, dirnames, filenames))

            paths += [path._make_child_relpath(d) for d in reversed(dirnames)]

    def absolute(self):
        """Return an absolute version of this path
        No normalization or symlink resolution is performed.

        Use resolve() to resolve symlinks and remove '..' segments.
        """
        self._unsupported("absolute")

    @classmethod
    def cwd(cls):
        """Return a new path pointing to the current working directory."""
        # We call 'absolute()' rather than using 'os.getcwd()' directly to
        # enable users to replace the implementation of 'absolute()' in a
        # subclass and benefit from the new behaviour here. This works because
        # os.path.abspath('.') == os.getcwd().
        return cls().absolute()

    def expanduser(self):
        """ Return a new path with expanded ~ and ~user constructs
        (as returned by os.path.expanduser)
        """
        self._unsupported("expanduser")

    @classmethod
    def home(cls):
        """Return a new path pointing to expanduser('~').
        """
        return cls("~").expanduser()

    def readlink(self):
        """
        Return the path to which the symbolic link points.
        """
        self._unsupported("readlink")
    readlink._supported = False

    def _split_stack(self):
        """
        Split the path into a 2-tuple (anchor, parts), where *anchor* is the
        uppermost parent of the path (equivalent to path.parents[-1]), and
        *parts* is a reversed list of parts following the anchor.
        """
        if not self._tail:
            return self, []
        return self._from_parsed_parts(self.drive, self.root, []), self._tail[::-1]

    def resolve(self, strict=False):
        """
        Make the path absolute, resolving all symlinks on the way and also
        normalizing it.
        """
        if self._resolving:
            return self
        path, parts = self._split_stack()
        try:
            path = path.absolute()
        except UnsupportedOperation:
            pass

        # If the user has *not* overridden the `readlink()` method, then symlinks are unsupported
        # and (in non-strict mode) we can improve performance by not calling `stat()`.
        querying = strict or getattr(self.readlink, '_supported', True)
        link_count = 0
        while parts:
            part = parts.pop()
            if part == '..':
                if not path._tail:
                    if path.root:
                        # Delete '..' segment immediately following root
                        continue
                elif path._tail[-1] != '..':
                    # Delete '..' segment and its predecessor
                    path = path.parent
                    continue
            next_path = path._make_child_relpath(part)
            if querying and part != '..':
                next_path._resolving = True
                try:
                    st = next_path.stat(follow_symlinks=False)
                    if S_ISLNK(st.st_mode):
                        # Like Linux and macOS, raise OSError(errno.ELOOP) if too many symlinks are
                        # encountered during resolution.
                        link_count += 1
                        if link_count >= _MAX_SYMLINKS:
                            raise OSError(ELOOP, "Too many symbolic links in path", str(self))
                        target, target_parts = next_path.readlink()._split_stack()
                        # If the symlink target is absolute (like '/etc/hosts'), set the current
                        # path to its uppermost parent (like '/').
                        if target.root:
                            path = target
                        # Add the symlink target's reversed tail parts (like ['hosts', 'etc']) to
                        # the stack of unresolved path parts.
                        parts.extend(target_parts)
                        continue
                    elif parts and not S_ISDIR(st.st_mode):
                        raise NotADirectoryError(ENOTDIR, "Not a directory", str(self))
                except OSError:
                    if strict:
                        raise
                    else:
                        querying = False
                next_path._resolving = False
            path = next_path
        return path

    def symlink_to(self, target, target_is_directory=False):
        """
        Make this path a symlink pointing to the target path.
        Note the order of arguments (link, target) is the reverse of os.symlink.
        """
        self._unsupported("symlink_to")

    def hardlink_to(self, target):
        """
        Make this path a hard link pointing to the same file as *target*.

        Note the order of arguments (self, target) is the reverse of os.link's.
        """
        self._unsupported("hardlink_to")

    def touch(self, mode=0o666, exist_ok=True):
        """
        Create this file with the given access mode, if it doesn't exist.
        """
        self._unsupported("touch")

    def mkdir(self, mode=0o777, parents=False, exist_ok=False):
        """
        Create a new directory at this given path.
        """
        self._unsupported("mkdir")

    def rename(self, target):
        """
        Rename this path to the target path.

        The target path may be absolute or relative. Relative paths are
        interpreted relative to the current working directory, *not* the
        directory of the Path object.

        Returns the new Path instance pointing to the target path.
        """
        self._unsupported("rename")

    def replace(self, target):
        """
        Rename this path to the target path, overwriting if that path exists.

        The target path may be absolute or relative. Relative paths are
        interpreted relative to the current working directory, *not* the
        directory of the Path object.

        Returns the new Path instance pointing to the target path.
        """
        self._unsupported("replace")

    def chmod(self, mode, *, follow_symlinks=True):
        """
        Change the permissions of the path, like os.chmod().
        """
        self._unsupported("chmod")

    def lchmod(self, mode):
        """
        Like chmod(), except if the path points to a symlink, the symlink's
        permissions are changed, rather than its target's.
        """
        self.chmod(mode, follow_symlinks=False)

    def unlink(self, missing_ok=False):
        """
        Remove this file or link.
        If the path is a directory, use rmdir() instead.
        """
        self._unsupported("unlink")

    def rmdir(self):
        """
        Remove this directory.  The directory must be empty.
        """
        self._unsupported("rmdir")

    def owner(self, *, follow_symlinks=True):
        """
        Return the login name of the file owner.
        """
        self._unsupported("owner")

    def group(self, *, follow_symlinks=True):
        """
        Return the group name of the file gid.
        """
        self._unsupported("group")

    @classmethod
    def from_uri(cls, uri):
        """Return a new path from the given 'file' URI."""
        cls._unsupported("from_uri")

    def as_uri(self):
        """Return the path as a URI."""
        self._unsupported("as_uri")
