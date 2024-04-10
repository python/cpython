"""Filename globbing utility."""

import os
import re
import fnmatch
import functools
import itertools
import operator
import sys

__all__ = ["glob", "iglob", "escape"]

def glob(pathname, *, root_dir=None, dir_fd=None, recursive=False,
        include_hidden=False):
    """Return a list of paths matching a pathname pattern.

    The pattern may contain simple shell-style wildcards a la
    fnmatch. Unlike fnmatch, filenames starting with a
    dot are special cases that are not matched by '*' and '?'
    patterns by default.

    If `include_hidden` is true, the patterns '*', '?', '**'  will match hidden
    directories.

    If `recursive` is true, the pattern '**' will match any files and
    zero or more directories and subdirectories.
    """
    return list(iglob(pathname, root_dir=root_dir, dir_fd=dir_fd, recursive=recursive,
                      include_hidden=include_hidden))

def iglob(pathname, *, root_dir=None, dir_fd=None, recursive=False,
          include_hidden=False):
    """Return an iterator which yields the paths matching a pathname pattern.

    The pattern may contain simple shell-style wildcards a la
    fnmatch. However, unlike fnmatch, filenames starting with a
    dot are special cases that are not matched by '*' and '?'
    patterns.

    If recursive is true, the pattern '**' will match any files and
    zero or more directories and subdirectories.
    """
    sys.audit("glob.glob", pathname, recursive)
    sys.audit("glob.glob/2", pathname, recursive, root_dir, dir_fd)
    pathname = os.fspath(pathname)
    is_bytes = isinstance(pathname, bytes)
    if is_bytes:
        pathname = os.fsdecode(pathname)
        if root_dir is not None:
            root_dir = os.fsdecode(root_dir)
    anchor, parts = _split_pathname(pathname)

    globber = _Globber(recursive=recursive, include_hidden=include_hidden)
    select = globber.selector(parts)
    if anchor:
        # Non-relative pattern. The anchor is guaranteed to exist unless it
        # has a Windows drive component.
        exists = not os.path.splitdrive(anchor)[0]
        paths = select(anchor, dir_fd, anchor, exists)
    else:
        # Relative pattern.
        if root_dir is None:
            root_dir = os.path.curdir
        paths = _relative_glob(select, root_dir, dir_fd)

        # Ensure that the empty string is not yielded when given a pattern
        # like '' or '**'.
        paths = itertools.dropwhile(operator.not_, paths)
    if is_bytes:
        paths = map(os.fsencode, paths)
    return paths

_deprecated_function_message = (
    "{name} is deprecated and will be removed in Python {remove}. Use "
    "glob.glob and pass a directory to its root_dir argument instead."
)

def glob0(dirname, pattern):
    import warnings
    warnings._deprecated("glob.glob0", _deprecated_function_message, remove=(3, 15))
    return list(_relative_glob(_Globber().literal_selector(pattern, []), dirname))

def glob1(dirname, pattern):
    import warnings
    warnings._deprecated("glob.glob1", _deprecated_function_message, remove=(3, 15))
    return list(_relative_glob(_Globber().wildcard_selector(pattern, []), dirname))

def _split_pathname(pathname):
    """Split the given path into a pair (anchor, parts), where *anchor* is the
    path drive and root (if any), and *parts* is a reversed list of path parts.
    """
    parts = []
    split = os.path.split
    dirname, part = split(pathname)
    while dirname != pathname:
        parts.append(part)
        pathname = dirname
        dirname, part = split(pathname)
    return dirname, parts

def _relative_glob(select, dirname, dir_fd=None):
    """Globs using a select function from the given dirname. The dirname
    prefix is removed from results.
    """
    dirname = _Globber.add_slash(dirname)
    slicer = operator.itemgetter(slice(len(dirname), None))
    return map(slicer, select(dirname, dir_fd, dirname))

magic_check = re.compile('([*?[])')
magic_check_bytes = re.compile(b'([*?[])')

def has_magic(s):
    if isinstance(s, bytes):
        match = magic_check_bytes.search(s)
    else:
        match = magic_check.search(s)
    return match is not None


def escape(pathname):
    """Escape all special characters.
    """
    # Escaping is done by wrapping any of "*?[" between square brackets.
    # Metacharacters do not work in the drive part and shouldn't be escaped.
    drive, pathname = os.path.splitdrive(pathname)
    if isinstance(pathname, bytes):
        pathname = magic_check_bytes.sub(br'[\1]', pathname)
    else:
        pathname = magic_check.sub(r'[\1]', pathname)
    return drive + pathname


_special_parts = ('', '.', '..')
_case_sensitive = os.path.normcase('Aa') == 'Aa'
_dir_open_flags = os.O_RDONLY | getattr(os, 'O_DIRECTORY', 0)
_no_recurse_symlinks = object()


def translate(pat, *, recursive=False, include_hidden=False, seps=None):
    """Translate a pathname with shell wildcards to a regular expression.

    If `recursive` is true, the pattern segment '**' will match any number of
    path segments.

    If `include_hidden` is true, wildcards can match path segments beginning
    with a dot ('.').

    If a sequence of separator characters is given to `seps`, they will be
    used to split the pattern into segments and match path separators. If not
    given, os.path.sep and os.path.altsep (where available) are used.
    """
    if not seps:
        if os.path.altsep:
            seps = (os.path.sep, os.path.altsep)
        else:
            seps = os.path.sep
    escaped_seps = ''.join(map(re.escape, seps))
    any_sep = f'[{escaped_seps}]' if len(seps) > 1 else escaped_seps
    not_sep = f'[^{escaped_seps}]'
    if include_hidden:
        one_last_segment = f'{not_sep}+'
        one_segment = f'{one_last_segment}{any_sep}'
        any_segments = f'(?:.+{any_sep})?'
        any_last_segments = '.*'
    else:
        one_last_segment = f'[^{escaped_seps}.]{not_sep}*'
        one_segment = f'{one_last_segment}{any_sep}'
        any_segments = f'(?:{one_segment})*'
        any_last_segments = f'{any_segments}(?:{one_last_segment})?'

    results = []
    parts = re.split(any_sep, pat)
    last_part_idx = len(parts) - 1
    for idx, part in enumerate(parts):
        if part == '*':
            results.append(one_segment if idx < last_part_idx else one_last_segment)
        elif recursive and part == '**':
            if idx < last_part_idx:
                if parts[idx + 1] != '**':
                    results.append(any_segments)
            else:
                results.append(any_last_segments)
        else:
            if part:
                if not include_hidden and part[0] in '*?':
                    results.append(r'(?!\.)')
                results.extend(fnmatch._translate(part, f'{not_sep}*', not_sep))
            if idx < last_part_idx:
                results.append(any_sep)
    res = ''.join(results)
    return fr'(?s:{res})\Z'


@functools.lru_cache(maxsize=512)
def _compile_pattern(pat, sep, case_sensitive, recursive=True):
    """Compile given glob pattern to a re.Pattern object (observing case
    sensitivity)."""
    flags = re.NOFLAG if case_sensitive else re.IGNORECASE
    regex = translate(pat, recursive=recursive, include_hidden=True, seps=sep)
    return re.compile(regex, flags=flags).match


class _Globber:
    """Class providing shell-style pattern matching and globbing.
    """

    def __init__(self, sep=os.path.sep, case_sensitive=_case_sensitive, recursive=False):
        self.sep = sep
        self.case_sensitive = case_sensitive
        self.recursive = recursive

    # Low-level methods

    lstat = staticmethod(os.lstat)
    scandir = staticmethod(os.scandir)
    parse_entry = operator.attrgetter('path')
    concat_path = operator.add

    if os.name == 'nt':
        @staticmethod
        def add_slash(pathname):
            tail = os.path.splitroot(pathname)[2]
            if not tail or tail[-1] in '\\/':
                return pathname
            return f'{pathname}\\'
    else:
        @staticmethod
        def add_slash(pathname):
            if not pathname or pathname[-1] == '/':
                return pathname
            return f'{pathname}/'

    # High-level methods

    def compile(self, pat):
        return _compile_pattern(pat, self.sep, self.case_sensitive, self.recursive)

    def selector(self, parts):
        """Returns a function that selects from a given path, walking and
        filtering according to the glob-style pattern parts in *parts*.
        """
        if not parts:
            return self.select_exists
        part = parts.pop()
        if self.recursive and part == '**':
            selector = self.recursive_selector
        elif part in _special_parts:
            selector = self.special_selector
        else:
            selector = self.wildcard_selector
        return selector(part, parts)

    def special_selector(self, part, parts):
        """Returns a function that selects special children of the given path.
        """
        select_next = self.selector(parts)

        def select_special(path, exists=False):
            path = self.concat_path(self.add_slash(path), part)
            return select_next(path, exists)
        return select_special

    def wildcard_selector(self, part, parts):
        """Returns a function that selects direct children of a given path,
        filtering by pattern.
        """

        match = None if part == '*' else self.compile(part)
        dir_only = bool(parts)
        if dir_only:
            select_next = self.selector(parts)

        def select_wildcard(path, exists=False):
            try:
                # We must close the scandir() object before proceeding to
                # avoid exhausting file descriptors when globbing deep trees.
                with self.scandir(path) as scandir_it:
                    entries = list(scandir_it)
            except OSError:
                pass
            else:
                for entry in entries:
                    if match is None or match(entry.name):
                        if dir_only:
                            try:
                                if not entry.is_dir():
                                    continue
                            except OSError:
                                continue
                        entry_path = self.parse_entry(entry)
                        if dir_only:
                            yield from select_next(entry_path, exists=True)
                        else:
                            yield entry_path
        return select_wildcard

    def recursive_selector(self, part, parts):
        """Returns a function that selects a given path and all its children,
        recursively, filtering by pattern.
        """
        # Optimization: consume following '**' parts, which have no effect.
        while parts and parts[-1] == '**':
            parts.pop()

        # Optimization: consume and join any following non-special parts here,
        # rather than leaving them for the next selector. They're used to
        # build a regular expression, which we use to filter the results of
        # the recursive walk. As a result, non-special pattern segments
        # following a '**' wildcard don't require additional filesystem access
        # to expand.
        follow_symlinks = self.recursive is not _no_recurse_symlinks
        if follow_symlinks:
            while parts and parts[-1] not in _special_parts:
                part += self.sep + parts.pop()

        match = None if part == '**' else self.compile(part)
        dir_only = bool(parts)
        select_next = self.selector(parts)

        def select_recursive(path, exists=False):
            path = self.add_slash(path)
            match_pos = len(str(path))
            if match is None or match(str(path), match_pos):
                yield from select_next(path, exists)
            stack = [path]
            while stack:
                yield from select_recursive_step(stack, match_pos)

        def select_recursive_step(stack, match_pos):
            path = stack.pop()
            try:
                # We must close the scandir() object before proceeding to
                # avoid exhausting file descriptors when globbing deep trees.
                with self.scandir(path) as scandir_it:
                    entries = list(scandir_it)
            except OSError:
                pass
            else:
                for entry in entries:
                    is_dir = False
                    try:
                        if entry.is_dir(follow_symlinks=follow_symlinks):
                            is_dir = True
                    except OSError:
                        pass

                    if is_dir or not dir_only:
                        entry_path = self.parse_entry(entry)
                        if match is None or match(str(entry_path), match_pos):
                            if dir_only:
                                yield from select_next(entry_path, exists=True)
                            else:
                                # Optimization: directly yield the path if this is
                                # last pattern part.
                                yield entry_path
                        if is_dir:
                            stack.append(entry_path)

        return select_recursive

    def select_exists(self, path, exists=False):
        """Yields the given path, if it exists.
        """
        if exists:
            # Optimization: this path is already known to exist, e.g. because
            # it was returned from os.scandir(), so we skip calling lstat().
            yield path
        else:
            try:
                self.lstat(path)
                yield path
            except OSError:
                pass
