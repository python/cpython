import functools
import ntpath
import posixpath
from errno import ENOENT, ENOTDIR, EBADF, ELOOP, EINVAL
from stat import S_ISDIR, S_ISLNK, S_ISREG, S_ISSOCK, S_ISBLK, S_ISCHR, S_ISFIFO

#
# Internals
#

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
                    yield parent_path._make_child_entry(entry, dir_only)


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
                            paths.append(path._make_child_entry(entry, dir_only))
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

    def __str__(self):
        """Return the string representation of the path, suitable for
        passing to system calls."""
        return self.pathmod.join(*self._raw_paths)

    def as_posix(self):
        """Return the string representation of the path with forward (/)
        slashes."""
        return str(self).replace(self.pathmod.sep, '/')

    @property
    def drive(self):
        """The drive prefix (letter or UNC path), if any."""
        return self.pathmod.splitdrive(str(self))[0]

    @property
    def root(self):
        """The root of the path, if any."""
        return self.pathmod.splitroot(str(self))[1]

    @property
    def anchor(self):
        """The concatenation of the drive and root, or ''."""
        drive, root, _ =  self.pathmod.splitroot(str(self))
        return drive + root

    @property
    def name(self):
        """The final path component, if any."""
        return self.pathmod.basename(str(self))

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
        dirname = self.pathmod.dirname
        if dirname(name):
            raise ValueError(f"Invalid name {name!r}")
        return self.with_segments(dirname(str(self)), name)

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
        anchor0, parts0 = self._stack
        anchor1, parts1 = other._stack
        if anchor0 != anchor1:
            raise ValueError(f"{str(self)!r} and {str(other)!r} have different anchors")
        while parts0 and parts1 and parts0[-1] == parts1[-1]:
            parts0.pop()
            parts1.pop()
        for part in parts1:
            if not part or part == '.':
                pass
            elif not walk_up:
                raise ValueError(f"{str(self)!r} is not in the subpath of {str(other)!r}")
            elif part == '..':
                raise ValueError(f"'..' segment in {str(other)!r} cannot be walked")
            else:
                parts0.append('..')
        return self.with_segments('', *reversed(parts0))

    def is_relative_to(self, other):
        """Return True if the path is relative to another path or False.
        """
        if not isinstance(other, PurePathBase):
            other = self.with_segments(other)
        anchor0, parts0 = self._stack
        anchor1, parts1 = other._stack
        if anchor0 != anchor1:
            return False
        while parts0 and parts1 and parts0[-1] == parts1[-1]:
            parts0.pop()
            parts1.pop()
        for part in parts1:
            if part and part != '.':
                return False
        return True

    @property
    def parts(self):
        """An object providing sequence-like access to the
        components in the filesystem path."""
        anchor, parts = self._stack
        if anchor:
            parts.append(anchor)
        return tuple(reversed(parts))

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
    def _stack(self):
        """
        Split the path into a 2-tuple (anchor, parts), where *anchor* is the
        uppermost parent of the path (equivalent to path.parents[-1]), and
        *parts* is a reversed list of parts following the anchor.
        """
        split = self.pathmod.split
        path = str(self)
        parent, name = split(path)
        names = []
        while path != parent:
            names.append(name)
            path = parent
            parent, name = split(path)
        return path, names

    @property
    def parent(self):
        """The logical parent of the path."""
        path = str(self)
        parent = self.pathmod.dirname(path)
        if path != parent:
            parent = self.with_segments(parent)
            parent._resolving = self._resolving
            return parent
        return self

    @property
    def parents(self):
        """A sequence of this path's logical parents."""
        dirname = self.pathmod.dirname
        path = str(self)
        parent = dirname(path)
        parents = []
        while path != parent:
            parents.append(self.with_segments(parent))
            path = parent
            parent = dirname(path)
        return tuple(parents)

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
        if self.pathmod is posixpath or not self.name:
            return False

        # NOTE: the rules for reserved names seem somewhat complicated
        # (e.g. r"..\NUL" is reserved but not r"foo\NUL" if "foo" does not
        # exist). We err on the side of caution and return True for paths
        # which are not considered reserved by Windows.
        if self.drive.startswith('\\\\'):
            # UNC paths are never reserved.
            return False
        name = self.name.partition('.')[0].partition(':')[0].rstrip(' ')
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
        if path_pattern.anchor:
            pass
        elif path_pattern.parts:
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

    # Maximum number of symlinks to follow in resolve()
    _max_symlinks = 40

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

    def _make_child_entry(self, entry, is_dir=False):
        # Transform an entry yielded from _scandir() into a path object.
        if is_dir:
            return entry.joinpath('')
        return entry

    def _make_child_relpath(self, name):
        return self.joinpath(name)

    def glob(self, pattern, *, case_sensitive=None, follow_symlinks=None):
        """Iterate over this subtree and yield all existing files (of any
        kind, including directories) matching the given relative pattern.
        """
        path_pattern = self.with_segments(pattern)
        if path_pattern.anchor:
            raise NotImplementedError("Non-relative patterns are unsupported")
        elif not path_pattern.parts:
            raise ValueError("Unacceptable pattern: {!r}".format(pattern))

        pattern_parts = list(path_pattern.parts)
        if not self.pathmod.basename(pattern):
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
        paths = iter([self.joinpath('')] if self.is_dir() else [])
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
        return cls('').absolute()

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

    def resolve(self, strict=False):
        """
        Make the path absolute, resolving all symlinks on the way and also
        normalizing it.
        """
        if self._resolving:
            return self
        path_root, parts = self._stack
        path = self.with_segments(path_root)
        try:
            path = path.absolute()
        except UnsupportedOperation:
            path_tail = []
        else:
            path_root, path_tail = path._stack
            path_tail.reverse()

        # If the user has *not* overridden the `readlink()` method, then symlinks are unsupported
        # and (in non-strict mode) we can improve performance by not calling `stat()`.
        querying = strict or getattr(self.readlink, '_supported', True)
        link_count = 0
        while parts:
            part = parts.pop()
            if not part or part == '.':
                continue
            if part == '..':
                if not path_tail:
                    if path_root:
                        # Delete '..' segment immediately following root
                        continue
                elif path_tail[-1] != '..':
                    # Delete '..' segment and its predecessor
                    path_tail.pop()
                    continue
            path_tail.append(part)
            if querying and part != '..':
                path = self.with_segments(path_root + self.pathmod.sep.join(path_tail))
                path._resolving = True
                try:
                    st = path.stat(follow_symlinks=False)
                    if S_ISLNK(st.st_mode):
                        # Like Linux and macOS, raise OSError(errno.ELOOP) if too many symlinks are
                        # encountered during resolution.
                        link_count += 1
                        if link_count >= self._max_symlinks:
                            raise OSError(ELOOP, "Too many symbolic links in path", str(self))
                        target_root, target_parts = path.readlink()._stack
                        # If the symlink target is absolute (like '/etc/hosts'), set the current
                        # path to its uppermost parent (like '/').
                        if target_root:
                            path_root = target_root
                            path_tail.clear()
                        else:
                            path_tail.pop()
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
        return self.with_segments(path_root + self.pathmod.sep.join(path_tail))

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
