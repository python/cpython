"""Object-oriented filesystem paths.

This module provides classes to represent abstract paths and concrete
paths with operations that have semantics appropriate for different
operating systems.
"""

import fnmatch
import functools
import io
import ntpath
import os
import posixpath
import re
import sys
import warnings
from _collections_abc import Sequence
from errno import ENOENT, ENOTDIR, EBADF, ELOOP
from stat import S_ISDIR, S_ISLNK, S_ISREG, S_ISSOCK, S_ISBLK, S_ISCHR, S_ISFIFO
from urllib.parse import quote_from_bytes as urlquote_from_bytes


__all__ = [
    "PurePath", "PurePosixPath", "PureWindowsPath",
    "Path", "PosixPath", "WindowsPath",
    ]

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
def _is_case_sensitive(flavour):
    return flavour.normcase('Aa') == 'Aa'

#
# Globbing helpers
#


# fnmatch.translate() returns a regular expression that includes a prefix and
# a suffix, which enable matching newlines and ensure the end of the string is
# matched, respectively. These features are undesirable for our implementation
# of PurePatch.match(), which represents path separators as newlines and joins
# pattern segments together. As a workaround, we define a slice object that
# can remove the prefix and suffix from any translate() result. See the
# _compile_pattern_lines() function for more details.
_FNMATCH_PREFIX, _FNMATCH_SUFFIX = fnmatch.translate('_').split('_')
_FNMATCH_SLICE = slice(len(_FNMATCH_PREFIX), -len(_FNMATCH_SUFFIX))
_SWAP_SEP_AND_NEWLINE = {
    '/': str.maketrans({'/': '\n', '\n': '/'}),
    '\\': str.maketrans({'\\': '\n', '\n': '\\'}),
}


@functools.lru_cache()
def _make_selector(pattern_parts, flavour, case_sensitive):
    pat = pattern_parts[0]
    if not pat:
        return _TerminatingSelector()
    if pat == '**':
        child_parts_idx = 1
        while child_parts_idx < len(pattern_parts) and pattern_parts[child_parts_idx] == '**':
            child_parts_idx += 1
        child_parts = pattern_parts[child_parts_idx:]
        if '**' in child_parts:
            cls = _DoubleRecursiveWildcardSelector
        else:
            cls = _RecursiveWildcardSelector
    else:
        child_parts = pattern_parts[1:]
        if pat == '..':
            cls = _ParentSelector
        elif '**' in pat:
            raise ValueError("Invalid pattern: '**' can only be an entire path component")
        else:
            cls = _WildcardSelector
    return cls(pat, child_parts, flavour, case_sensitive)


@functools.lru_cache(maxsize=256)
def _compile_pattern(pat, case_sensitive):
    flags = re.NOFLAG if case_sensitive else re.IGNORECASE
    return re.compile(fnmatch.translate(pat), flags).match


@functools.lru_cache()
def _compile_pattern_lines(pattern_lines, case_sensitive):
    """Compile the given pattern lines to an `re.Pattern` object.

    The *pattern_lines* argument is a glob-style pattern (e.g. '*/*.py') with
    its path separators and newlines swapped (e.g. '*\n*.py`). By using
    newlines to separate path components, and not setting `re.DOTALL`, we
    ensure that the `*` wildcard cannot match path separators.

    The returned `re.Pattern` object may have its `match()` method called to
    match a complete pattern, or `search()` to match from the right. The
    argument supplied to these methods must also have its path separators and
    newlines swapped.
    """

    # Match the start of the path, or just after a path separator
    parts = ['^']
    for part in pattern_lines.splitlines(keepends=True):
        if part == '*\n':
            part = r'.+\n'
        elif part == '*':
            part = r'.+'
        else:
            # Any other component: pass to fnmatch.translate(). We slice off
            # the common prefix and suffix added by translate() to ensure that
            # re.DOTALL is not set, and the end of the string not matched,
            # respectively. With DOTALL not set, '*' wildcards will not match
            # path separators, because the '.' characters in the pattern will
            # not match newlines.
            part = fnmatch.translate(part)[_FNMATCH_SLICE]
        parts.append(part)
    # Match the end of the path, always.
    parts.append(r'\Z')
    flags = re.MULTILINE
    if not case_sensitive:
        flags |= re.IGNORECASE
    return re.compile(''.join(parts), flags=flags)


class _Selector:
    """A selector matches a specific glob pattern part against the children
    of a given path."""

    def __init__(self, child_parts, flavour, case_sensitive):
        self.child_parts = child_parts
        if child_parts:
            self.successor = _make_selector(child_parts, flavour, case_sensitive)
            self.dironly = True
        else:
            self.successor = _TerminatingSelector()
            self.dironly = False

    def select_from(self, parent_path):
        """Iterate over all child paths of `parent_path` matched by this
        selector.  This can contain parent_path itself."""
        path_cls = type(parent_path)
        scandir = path_cls._scandir
        if not parent_path.is_dir():
            return iter([])
        return self._select_from(parent_path, scandir)


class _TerminatingSelector:

    def _select_from(self, parent_path, scandir):
        yield parent_path


class _ParentSelector(_Selector):

    def __init__(self, name, child_parts, flavour, case_sensitive):
        _Selector.__init__(self, child_parts, flavour, case_sensitive)

    def _select_from(self,  parent_path, scandir):
        path = parent_path._make_child_relpath('..')
        for p in self.successor._select_from(path, scandir):
            yield p


class _WildcardSelector(_Selector):

    def __init__(self, pat, child_parts, flavour, case_sensitive):
        _Selector.__init__(self, child_parts, flavour, case_sensitive)
        if case_sensitive is None:
            # TODO: evaluate case-sensitivity of each directory in _select_from()
            case_sensitive = _is_case_sensitive(flavour)
        self.match = _compile_pattern(pat, case_sensitive)

    def _select_from(self, parent_path, scandir):
        try:
            # We must close the scandir() object before proceeding to
            # avoid exhausting file descriptors when globbing deep trees.
            with scandir(parent_path) as scandir_it:
                entries = list(scandir_it)
        except OSError:
            pass
        else:
            for entry in entries:
                if self.dironly:
                    try:
                        if not entry.is_dir():
                            continue
                    except OSError:
                        continue
                name = entry.name
                if self.match(name):
                    path = parent_path._make_child_relpath(name)
                    for p in self.successor._select_from(path, scandir):
                        yield p


class _RecursiveWildcardSelector(_Selector):

    def __init__(self, pat, child_parts, flavour, case_sensitive):
        _Selector.__init__(self, child_parts, flavour, case_sensitive)

    def _iterate_directories(self, parent_path):
        yield parent_path
        for dirpath, dirnames, _ in parent_path.walk():
            for dirname in dirnames:
                yield dirpath._make_child_relpath(dirname)

    def _select_from(self, parent_path, scandir):
        successor_select = self.successor._select_from
        for starting_point in self._iterate_directories(parent_path):
            for p in successor_select(starting_point, scandir):
                yield p


class _DoubleRecursiveWildcardSelector(_RecursiveWildcardSelector):
    """
    Like _RecursiveWildcardSelector, but also de-duplicates results from
    successive selectors. This is necessary if the pattern contains
    multiple non-adjacent '**' segments.
    """

    def _select_from(self, parent_path, scandir):
        yielded = set()
        try:
            for p in super()._select_from(parent_path, scandir):
                if p not in yielded:
                    yield p
                    yielded.add(p)
        finally:
            yielded.clear()


#
# Public API
#

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


class PurePath(object):
    """Base class for manipulating paths without I/O.

    PurePath represents a filesystem path and offers operations which
    don't imply any actual filesystem I/O.  Depending on your system,
    instantiating a PurePath will return either a PurePosixPath or a
    PureWindowsPath object.  You can also instantiate either of these classes
    directly, regardless of your system.
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

        # The `_str_normcase_cached` slot stores the string path with
        # normalized case. It is set when the `_str_normcase` property is
        # accessed for the first time. It's used to implement `__eq__()`
        # `__hash__()`, and `_parts_normcase`
        '_str_normcase_cached',

        # The `_parts_normcase_cached` slot stores the case-normalized
        # string path after splitting on path separators. It's set when the
        # `_parts_normcase` property is accessed for the first time. It's used
        # to implement comparison methods like `__lt__()`.
        '_parts_normcase_cached',

        # The `_lines_cached` slot stores the string path with path separators
        # and newlines swapped. This is used to implement `match()`.
        '_lines_cached',

        # The `_hash` slot stores the hash of the case-normalized string
        # path. It's set when `__hash__()` is called for the first time.
        '_hash',
    )
    _flavour = os.path

    def __new__(cls, *args, **kwargs):
        """Construct a PurePath from one or several strings and or existing
        PurePath objects.  The strings and path objects are combined so as
        to yield a canonicalized path, which is incorporated into the
        new PurePath object.
        """
        if cls is PurePath:
            cls = PureWindowsPath if os.name == 'nt' else PurePosixPath
        return object.__new__(cls)

    def __reduce__(self):
        # Using the parts tuple helps share interned path parts
        # when pickling related paths.
        return (self.__class__, self.parts)

    def __init__(self, *args):
        paths = []
        for arg in args:
            if isinstance(arg, PurePath):
                if arg._flavour is ntpath and self._flavour is posixpath:
                    # GH-103631: Convert separators for backwards compatibility.
                    paths.extend(path.replace('\\', '/') for path in arg._raw_paths)
                else:
                    paths.extend(arg._raw_paths)
            else:
                try:
                    path = os.fspath(arg)
                except TypeError:
                    path = arg
                if not isinstance(path, str):
                    raise TypeError(
                        "argument should be a str or an os.PathLike "
                        "object where __fspath__ returns a str, "
                        f"not {type(path).__name__!r}")
                paths.append(path)
        self._raw_paths = paths

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
        sep = cls._flavour.sep
        altsep = cls._flavour.altsep
        if altsep:
            path = path.replace(altsep, sep)
        drv, root, rel = cls._flavour.splitroot(path)
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
            path = self._flavour.join(*paths)
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
            return drv + root + cls._flavour.sep.join(tail)
        elif tail and cls._flavour.splitdrive(tail[0])[0]:
            tail = ['.'] + tail
        return cls._flavour.sep.join(tail)

    def __str__(self):
        """Return the string representation of the path, suitable for
        passing to system calls."""
        try:
            return self._str
        except AttributeError:
            self._str = self._format_parsed_parts(self.drive, self.root,
                                                  self._tail) or '.'
            return self._str

    def __fspath__(self):
        return str(self)

    def as_posix(self):
        """Return the string representation of the path with forward (/)
        slashes."""
        f = self._flavour
        return str(self).replace(f.sep, '/')

    def __bytes__(self):
        """Return the bytes representation of the path.  This is only
        recommended to use under Unix."""
        return os.fsencode(self)

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, self.as_posix())

    def as_uri(self):
        """Return the path as a 'file' URI."""
        if not self.is_absolute():
            raise ValueError("relative path can't be expressed as a file URI")

        drive = self.drive
        if len(drive) == 2 and drive[1] == ':':
            # It's a path on a local drive => 'file:///c:/a/b'
            prefix = 'file:///' + drive
            path = self.as_posix()[2:]
        elif drive:
            # It's a path on a network drive => 'file://host/share/a/b'
            prefix = 'file:'
            path = self.as_posix()
        else:
            # It's a posix path => 'file:///etc/hosts'
            prefix = 'file://'
            path = str(self)
        return prefix + urlquote_from_bytes(os.fsencode(path))

    @property
    def _str_normcase(self):
        # String with normalized case, for hashing and equality checks
        try:
            return self._str_normcase_cached
        except AttributeError:
            if _is_case_sensitive(self._flavour):
                self._str_normcase_cached = str(self)
            else:
                self._str_normcase_cached = str(self).lower()
            return self._str_normcase_cached

    @property
    def _parts_normcase(self):
        # Cached parts with normalized case, for comparisons.
        try:
            return self._parts_normcase_cached
        except AttributeError:
            self._parts_normcase_cached = self._str_normcase.split(self._flavour.sep)
            return self._parts_normcase_cached

    @property
    def _lines(self):
        # Path with separators and newlines swapped, for pattern matching.
        try:
            return self._lines_cached
        except AttributeError:
            path_str = str(self)
            if path_str == '.':
                self._lines_cached = ''
            else:
                trans = _SWAP_SEP_AND_NEWLINE[self._flavour.sep]
                self._lines_cached = path_str.translate(trans)
            return self._lines_cached

    def __eq__(self, other):
        if not isinstance(other, PurePath):
            return NotImplemented
        return self._str_normcase == other._str_normcase and self._flavour is other._flavour

    def __hash__(self):
        try:
            return self._hash
        except AttributeError:
            self._hash = hash(self._str_normcase)
            return self._hash

    def __lt__(self, other):
        if not isinstance(other, PurePath) or self._flavour is not other._flavour:
            return NotImplemented
        return self._parts_normcase < other._parts_normcase

    def __le__(self, other):
        if not isinstance(other, PurePath) or self._flavour is not other._flavour:
            return NotImplemented
        return self._parts_normcase <= other._parts_normcase

    def __gt__(self, other):
        if not isinstance(other, PurePath) or self._flavour is not other._flavour:
            return NotImplemented
        return self._parts_normcase > other._parts_normcase

    def __ge__(self, other):
        if not isinstance(other, PurePath) or self._flavour is not other._flavour:
            return NotImplemented
        return self._parts_normcase >= other._parts_normcase

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
        if not self.name:
            raise ValueError("%r has an empty name" % (self,))
        f = self._flavour
        if not name or f.sep in name or (f.altsep and f.altsep in name) or name == '.':
            raise ValueError("Invalid name %r" % (name))
        return self._from_parsed_parts(self.drive, self.root,
                                       self._tail[:-1] + [name])

    def with_stem(self, stem):
        """Return a new path with the stem changed."""
        return self.with_name(stem + self.suffix)

    def with_suffix(self, suffix):
        """Return a new path with the file suffix changed.  If the path
        has no suffix, add given suffix.  If the given suffix is an empty
        string, remove the suffix from the path.
        """
        f = self._flavour
        if f.sep in suffix or f.altsep and f.altsep in suffix:
            raise ValueError("Invalid suffix %r" % (suffix,))
        if suffix and not suffix.startswith('.') or suffix == '.':
            raise ValueError("Invalid suffix %r" % (suffix))
        name = self.name
        if not name:
            raise ValueError("%r has an empty name" % (self,))
        old_suffix = self.suffix
        if not old_suffix:
            name = name + suffix
        else:
            name = name[:-len(old_suffix)] + suffix
        return self._from_parsed_parts(self.drive, self.root,
                                       self._tail[:-1] + [name])

    def relative_to(self, other, /, *_deprecated, walk_up=False):
        """Return the relative path to another path identified by the passed
        arguments.  If the operation is not possible (because this is not
        related to the other path), raise ValueError.

        The *walk_up* parameter controls whether `..` may be used to resolve
        the path.
        """
        if _deprecated:
            msg = ("support for supplying more than one positional argument "
                   "to pathlib.PurePath.relative_to() is deprecated and "
                   "scheduled for removal in Python {remove}")
            warnings._deprecated("pathlib.PurePath.relative_to(*args)", msg,
                                 remove=(3, 14))
        other = self.with_segments(other, *_deprecated)
        for step, path in enumerate([other] + list(other.parents)):
            if self.is_relative_to(path):
                break
            elif not walk_up:
                raise ValueError(f"{str(self)!r} is not in the subpath of {str(other)!r}")
            elif path.name == '..':
                raise ValueError(f"'..' segment in {str(other)!r} cannot be walked")
        else:
            raise ValueError(f"{str(self)!r} and {str(other)!r} have different anchors")
        parts = ['..'] * step + self._tail[len(path._tail):]
        return self.with_segments(*parts)

    def is_relative_to(self, other, /, *_deprecated):
        """Return True if the path is relative to another path or False.
        """
        if _deprecated:
            msg = ("support for supplying more than one argument to "
                   "pathlib.PurePath.is_relative_to() is deprecated and "
                   "scheduled for removal in Python {remove}")
            warnings._deprecated("pathlib.PurePath.is_relative_to(*args)",
                                 msg, remove=(3, 14))
        other = self.with_segments(other, *_deprecated)
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
        return self.with_segments(self, *pathsegments)

    def __truediv__(self, key):
        try:
            return self.joinpath(key)
        except TypeError:
            return NotImplemented

    def __rtruediv__(self, key):
        try:
            return self.with_segments(key, self)
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
        return self._from_parsed_parts(drv, root, tail[:-1])

    @property
    def parents(self):
        """A sequence of this path's logical parents."""
        # The value of this property should not be cached on the path object,
        # as doing so would introduce a reference cycle.
        return _PathParents(self)

    def is_absolute(self):
        """True if the path is absolute (has both a root and, if applicable,
        a drive)."""
        if self._flavour is ntpath:
            # ntpath.isabs() is defective - see GH-44626.
            return bool(self.drive and self.root)
        elif self._flavour is posixpath:
            # Optimization: work with raw paths on POSIX.
            for path in self._raw_paths:
                if path.startswith('/'):
                    return True
            return False
        else:
            return self._flavour.isabs(str(self))

    def is_reserved(self):
        """Return True if the path contains one of the special names reserved
        by the system, if any."""
        if self._flavour is posixpath or not self._tail:
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
        if not isinstance(path_pattern, PurePath):
            path_pattern = self.with_segments(path_pattern)
        if case_sensitive is None:
            case_sensitive = _is_case_sensitive(self._flavour)
        pattern = _compile_pattern_lines(path_pattern._lines, case_sensitive)
        if path_pattern.drive or path_pattern.root:
            return pattern.match(self._lines) is not None
        elif path_pattern._tail:
            return pattern.search(self._lines) is not None
        else:
            raise ValueError("empty pattern")


# Can't subclass os.PathLike from PurePath and keep the constructor
# optimizations in PurePath.__slots__.
os.PathLike.register(PurePath)


class PurePosixPath(PurePath):
    """PurePath subclass for non-Windows systems.

    On a POSIX system, instantiating a PurePath should return this object.
    However, you can also instantiate it directly on any system.
    """
    _flavour = posixpath
    __slots__ = ()


class PureWindowsPath(PurePath):
    """PurePath subclass for Windows systems.

    On a Windows system, instantiating a PurePath should return this object.
    However, you can also instantiate it directly on any system.
    """
    _flavour = ntpath
    __slots__ = ()


# Filesystem-accessing classes


class Path(PurePath):
    """PurePath subclass that can make system calls.

    Path represents a filesystem path but unlike PurePath, also offers
    methods to do system calls on path objects. Depending on your system,
    instantiating a Path will return either a PosixPath or a WindowsPath
    object. You can also instantiate a PosixPath or WindowsPath directly,
    but cannot instantiate a WindowsPath on a POSIX system or vice versa.
    """
    __slots__ = ()

    def stat(self, *, follow_symlinks=True):
        """
        Return the result of the stat() system call on this path, like
        os.stat() does.
        """
        return os.stat(self, follow_symlinks=follow_symlinks)

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

    def is_dir(self):
        """
        Whether this path is a directory.
        """
        try:
            return S_ISDIR(self.stat().st_mode)
        except OSError as e:
            if not _ignore_error(e):
                raise
            # Path doesn't exist or is a broken symlink
            # (see http://web.archive.org/web/20200623061726/https://bitbucket.org/pitrou/pathlib/issues/12/ )
            return False
        except ValueError:
            # Non-encodable path
            return False

    def is_file(self):
        """
        Whether this path is a regular file (also True for symlinks pointing
        to regular files).
        """
        try:
            return S_ISREG(self.stat().st_mode)
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
        return self._flavour.ismount(self)

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
        return self._flavour.isjunction(self)

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
        return self._flavour.samestat(st, other_st)

    def open(self, mode='r', buffering=-1, encoding=None,
             errors=None, newline=None):
        """
        Open the file pointed by this path and return a file object, as
        the built-in open() function does.
        """
        if "b" not in mode:
            encoding = io.text_encoding(encoding)
        return io.open(self, mode, buffering, encoding, errors, newline)

    def read_bytes(self):
        """
        Open the file in bytes mode, read it, and close the file.
        """
        with self.open(mode='rb') as f:
            return f.read()

    def read_text(self, encoding=None, errors=None):
        """
        Open the file in text mode, read it, and close the file.
        """
        encoding = io.text_encoding(encoding)
        with self.open(mode='r', encoding=encoding, errors=errors) as f:
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
        encoding = io.text_encoding(encoding)
        with self.open(mode='w', encoding=encoding, errors=errors, newline=newline) as f:
            return f.write(data)

    def iterdir(self):
        """Yield path objects of the directory contents.

        The children are yielded in arbitrary order, and the
        special entries '.' and '..' are not included.
        """
        for name in os.listdir(self):
            yield self._make_child_relpath(name)

    def _scandir(self):
        # bpo-24132: a future version of pathlib will support subclassing of
        # pathlib.Path to customize how the filesystem is accessed. This
        # includes scandir(), which is used to implement glob().
        return os.scandir(self)

    def _make_child_relpath(self, name):
        path_str = str(self)
        tail = self._tail
        if tail:
            path_str = f'{path_str}{self._flavour.sep}{name}'
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

    def glob(self, pattern, *, case_sensitive=None):
        """Iterate over this subtree and yield all existing files (of any
        kind, including directories) matching the given relative pattern.
        """
        sys.audit("pathlib.Path.glob", self, pattern)
        if not pattern:
            raise ValueError("Unacceptable pattern: {!r}".format(pattern))
        drv, root, pattern_parts = self._parse_path(pattern)
        if drv or root:
            raise NotImplementedError("Non-relative patterns are unsupported")
        if pattern[-1] in (self._flavour.sep, self._flavour.altsep):
            pattern_parts.append('')
        selector = _make_selector(tuple(pattern_parts), self._flavour, case_sensitive)
        for p in selector.select_from(self):
            yield p

    def rglob(self, pattern, *, case_sensitive=None):
        """Recursively yield all existing files (of any kind, including
        directories) matching the given relative pattern, anywhere in
        this subtree.
        """
        sys.audit("pathlib.Path.rglob", self, pattern)
        drv, root, pattern_parts = self._parse_path(pattern)
        if drv or root:
            raise NotImplementedError("Non-relative patterns are unsupported")
        if pattern and pattern[-1] in (self._flavour.sep, self._flavour.altsep):
            pattern_parts.append('')
        selector = _make_selector(("**",) + tuple(pattern_parts), self._flavour, case_sensitive)
        for p in selector.select_from(self):
            yield p

    def walk(self, top_down=True, on_error=None, follow_symlinks=False):
        """Walk the directory tree from this directory, similar to os.walk()."""
        sys.audit("pathlib.Path.walk", self, on_error, follow_symlinks)
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
                scandir_it = path._scandir()
            except OSError as error:
                if on_error is not None:
                    on_error(error)
                continue

            with scandir_it:
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

    def __init__(self, *args, **kwargs):
        if kwargs:
            msg = ("support for supplying keyword arguments to pathlib.PurePath "
                   "is deprecated and scheduled for removal in Python {remove}")
            warnings._deprecated("pathlib.PurePath(**kwargs)", msg, remove=(3, 14))
        super().__init__(*args)

    def __new__(cls, *args, **kwargs):
        if cls is Path:
            cls = WindowsPath if os.name == 'nt' else PosixPath
        return object.__new__(cls)

    def __enter__(self):
        # In previous versions of pathlib, __exit__() marked this path as
        # closed; subsequent attempts to perform I/O would raise an IOError.
        # This functionality was never documented, and had the effect of
        # making Path objects mutable, contrary to PEP 428.
        # In Python 3.9 __exit__() was made a no-op.
        # In Python 3.11 __enter__() began emitting DeprecationWarning.
        # In Python 3.13 __enter__() and __exit__() should be removed.
        warnings.warn("pathlib.Path.__enter__() is deprecated and scheduled "
                      "for removal in Python 3.13; Path objects as a context "
                      "manager is a no-op",
                      DeprecationWarning, stacklevel=2)
        return self

    def __exit__(self, t, v, tb):
        pass

    # Public API

    @classmethod
    def cwd(cls):
        """Return a new path pointing to the current working directory."""
        # We call 'absolute()' rather than using 'os.getcwd()' directly to
        # enable users to replace the implementation of 'absolute()' in a
        # subclass and benefit from the new behaviour here. This works because
        # os.path.abspath('.') == os.getcwd().
        return cls().absolute()

    @classmethod
    def home(cls):
        """Return a new path pointing to the user's home directory (as
        returned by os.path.expanduser('~')).
        """
        return cls("~").expanduser()

    def absolute(self):
        """Return an absolute version of this path by prepending the current
        working directory. No normalization or symlink resolution is performed.

        Use resolve() to get the canonical path to a file.
        """
        if self.is_absolute():
            return self
        elif self.drive:
            # There is a CWD on each drive-letter drive.
            cwd = self._flavour.abspath(self.drive)
        else:
            cwd = os.getcwd()
            # Fast path for "empty" paths, e.g. Path("."), Path("") or Path().
            # We pass only one argument to with_segments() to avoid the cost
            # of joining, and we exploit the fact that getcwd() returns a
            # fully-normalized string by storing it in _str. This is used to
            # implement Path.cwd().
            if not self.root and not self._tail:
                result = self.with_segments(cwd)
                result._str = cwd
                return result
        return self.with_segments(cwd, self)

    def resolve(self, strict=False):
        """
        Make the path absolute, resolving all symlinks on the way and also
        normalizing it.
        """

        def check_eloop(e):
            winerror = getattr(e, 'winerror', 0)
            if e.errno == ELOOP or winerror == _WINERROR_CANT_RESOLVE_FILENAME:
                raise RuntimeError("Symlink loop from %r" % e.filename)

        try:
            s = self._flavour.realpath(self, strict=strict)
        except OSError as e:
            check_eloop(e)
            raise
        p = self.with_segments(s)

        # In non-strict mode, realpath() doesn't raise on symlink loops.
        # Ensure we get an exception by calling stat()
        if not strict:
            try:
                p.stat()
            except OSError as e:
                check_eloop(e)
        return p

    def owner(self):
        """
        Return the login name of the file owner.
        """
        try:
            import pwd
            return pwd.getpwuid(self.stat().st_uid).pw_name
        except ImportError:
            raise NotImplementedError("Path.owner() is unsupported on this system")

    def group(self):
        """
        Return the group name of the file gid.
        """

        try:
            import grp
            return grp.getgrgid(self.stat().st_gid).gr_name
        except ImportError:
            raise NotImplementedError("Path.group() is unsupported on this system")

    def readlink(self):
        """
        Return the path to which the symbolic link points.
        """
        if not hasattr(os, "readlink"):
            raise NotImplementedError("os.readlink() not available on this system")
        return self.with_segments(os.readlink(self))

    def touch(self, mode=0o666, exist_ok=True):
        """
        Create this file with the given access mode, if it doesn't exist.
        """

        if exist_ok:
            # First try to bump modification time
            # Implementation note: GNU touch uses the UTIME_NOW option of
            # the utimensat() / futimens() functions.
            try:
                os.utime(self, None)
            except OSError:
                # Avoid exception chaining
                pass
            else:
                return
        flags = os.O_CREAT | os.O_WRONLY
        if not exist_ok:
            flags |= os.O_EXCL
        fd = os.open(self, flags, mode)
        os.close(fd)

    def mkdir(self, mode=0o777, parents=False, exist_ok=False):
        """
        Create a new directory at this given path.
        """
        try:
            os.mkdir(self, mode)
        except FileNotFoundError:
            if not parents or self.parent == self:
                raise
            self.parent.mkdir(parents=True, exist_ok=True)
            self.mkdir(mode, parents=False, exist_ok=exist_ok)
        except OSError:
            # Cannot rely on checking for EEXIST, since the operating system
            # could give priority to other errors like EACCES or EROFS
            if not exist_ok or not self.is_dir():
                raise

    def chmod(self, mode, *, follow_symlinks=True):
        """
        Change the permissions of the path, like os.chmod().
        """
        os.chmod(self, mode, follow_symlinks=follow_symlinks)

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
        try:
            os.unlink(self)
        except FileNotFoundError:
            if not missing_ok:
                raise

    def rmdir(self):
        """
        Remove this directory.  The directory must be empty.
        """
        os.rmdir(self)

    def rename(self, target):
        """
        Rename this path to the target path.

        The target path may be absolute or relative. Relative paths are
        interpreted relative to the current working directory, *not* the
        directory of the Path object.

        Returns the new Path instance pointing to the target path.
        """
        os.rename(self, target)
        return self.with_segments(target)

    def replace(self, target):
        """
        Rename this path to the target path, overwriting if that path exists.

        The target path may be absolute or relative. Relative paths are
        interpreted relative to the current working directory, *not* the
        directory of the Path object.

        Returns the new Path instance pointing to the target path.
        """
        os.replace(self, target)
        return self.with_segments(target)

    def symlink_to(self, target, target_is_directory=False):
        """
        Make this path a symlink pointing to the target path.
        Note the order of arguments (link, target) is the reverse of os.symlink.
        """
        if not hasattr(os, "symlink"):
            raise NotImplementedError("os.symlink() not available on this system")
        os.symlink(target, self, target_is_directory)

    def hardlink_to(self, target):
        """
        Make this path a hard link pointing to the same file as *target*.

        Note the order of arguments (self, target) is the reverse of os.link's.
        """
        if not hasattr(os, "link"):
            raise NotImplementedError("os.link() not available on this system")
        os.link(target, self)

    def expanduser(self):
        """ Return a new path with expanded ~ and ~user constructs
        (as returned by os.path.expanduser)
        """
        if (not (self.drive or self.root) and
            self._tail and self._tail[0][:1] == '~'):
            homedir = self._flavour.expanduser(self._tail[0])
            if homedir[:1] == "~":
                raise RuntimeError("Could not determine home directory.")
            drv, root, tail = self._parse_path(homedir)
            return self._from_parsed_parts(drv, root, tail + self._tail[1:])

        return self


class PosixPath(Path, PurePosixPath):
    """Path subclass for non-Windows systems.

    On a POSIX system, instantiating a Path should return this object.
    """
    __slots__ = ()

    if os.name == 'nt':
        def __new__(cls, *args, **kwargs):
            raise NotImplementedError(
                f"cannot instantiate {cls.__name__!r} on your system")

class WindowsPath(Path, PureWindowsPath):
    """Path subclass for Windows systems.

    On a Windows system, instantiating a Path should return this object.
    """
    __slots__ = ()

    if os.name != 'nt':
        def __new__(cls, *args, **kwargs):
            raise NotImplementedError(
                f"cannot instantiate {cls.__name__!r} on your system")
