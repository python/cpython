"""Filename globbing utility."""

import os
import re
import fnmatch
import functools
import itertools
import operator
import sys

__all__ = ["glob", "iglob", "escape"]


_special_parts = ('', os.path.curdir, os.path.pardir)
_pattern_flags = re.NOFLAG if os.path.normcase('Aa') == 'Aa' else re.IGNORECASE
_dir_open_flags = os.O_RDONLY | getattr(os, 'O_DIRECTORY', 0)
if os.path.altsep:
    _path_seps = (os.path.sep, os.path.altsep)
else:
    _path_seps = os.path.sep


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
    pathname, parts = _split_pathname(pathname)

    select = _selector(parts, recursive, include_hidden)
    if pathname:
        # Absolute pattern.
        drive = os.path.splitdrive(pathname)[0]
        paths = select(pathname, pathname, dir_fd, not drive)
    else:
        # Relative pattern.
        if root_dir is None:
            root_dir = os.path.curdir
        root_dir = _add_trailing_slash(root_dir)
        root_slicer = operator.itemgetter(slice(len(root_dir), None))
        paths = select(root_dir, root_dir, dir_fd, False)
        paths = map(root_slicer, paths)
        paths = itertools.dropwhile(operator.not_, paths)
    if is_bytes:
        paths = map(os.fsencode, paths)
    return paths


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


def translate(pat, *, recursive=False, include_hidden=False, seps=None):
    """Translate a pathname with shell wildcards to a regular expression.

    If `recursive` is true, the pattern segment '**' will match any number of
    path segments; if '**' appears outside its own segment, ValueError will be
    raised.

    If `include_hidden` is true, wildcards can match path segments beginning
    with a dot ('.').

    If a sequence of separator characters is given to `seps`, they will be
    used to split the pattern into segments and match path separators. If not
    given, os.path.sep and os.path.altsep (where available) are used.
    """
    if not seps:
        seps = _path_seps
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
            continue
        if recursive:
            if part == '**':
                if idx < last_part_idx:
                    if parts[idx + 1] != '**':
                        results.append(any_segments)
                else:
                    results.append(any_last_segments)
                continue
            elif '**' in part:
                raise ValueError("Invalid pattern: '**' can only be an entire path component")
        if part:
            if not include_hidden and part[0] in '*?':
                results.append(r'(?!\.)')
            results.extend(fnmatch._translate(part, f'{not_sep}*', not_sep))
        if idx < last_part_idx:
            results.append(any_sep)
    res = ''.join(results)
    return fr'(?s:{res})\Z'


def _compile_pattern(pattern, recursive, include_hidden):
    regex = translate(pattern, recursive=recursive,
                      include_hidden=include_hidden, seps=os.path.sep)
    return re.compile(regex, flags=_pattern_flags).match


def _split_pathname(pathname):
    """Split the given path into a pair (anchor, parts), where *anchor* is the
    path drive and root (if any), and *parts* is a tuple of path components.
    """
    parts = []
    split = os.path.split
    dirname, part = split(pathname)
    while dirname != pathname:
        parts.append(part)
        pathname = dirname
        dirname, part = split(pathname)
    parts.reverse()
    return dirname, tuple(parts)


def _add_trailing_slash(pathname):
    """Returns the given path with a trailing slash added, where possible.
    """
    tail = os.path.splitdrive(pathname)[1]
    if not tail or tail[-1] in _path_seps:
        return pathname
    return pathname + os.path.sep


def _open_dir(path, rel_path, dir_fd):
    """Prepares the directory for scanning. Returns a 3-tuple with parts:

    1. A path or fd to supply to `os.scandir()`.
    2. The file descriptor for the directory, or None.
    3. Whether the caller should close the fd (bool).
    """
    if dir_fd is None:
        return path, None, False
    elif rel_path == './':
        return dir_fd, dir_fd, False
    else:
        fd = os.open(rel_path, _dir_open_flags, dir_fd=dir_fd)
        return fd, fd, True


@functools.lru_cache(maxsize=1024)
def _selector(parts, recursive, include_hidden):
    """Returns a function that selects from a given path, walking and
    filtering according to the glob-style pattern parts in *parts*.
    """
    if not parts:
        return _select_exists
    part = parts[0]
    if recursive and part == '**':
        selector = _recursive_selector
    elif magic_check.search(part) is not None:
        selector = _wildcard_selector
    else:
        selector = _literal_selector
    return selector(part, parts[1:], recursive, include_hidden)


def _literal_selector(part, parts, recursive, include_hidden):
    """Returns a function that selects a literal descendant of a given path.
    """
    is_special = part in _special_parts
    while parts and magic_check.search(next_part := parts[0]) is None:
        # Consume next non-wildcard component (speeds up joining).
        if next_part not in _special_parts:
            is_special = False
        part += os.path.sep + next_part
        parts = parts[1:]

    select_next = _selector(parts, recursive, include_hidden)

    def select_literal(path, rel_path, dir_fd, exists):
        path = _add_trailing_slash(path) + part
        rel_path = _add_trailing_slash(rel_path) + part
        return select_next(path, rel_path, dir_fd, exists and is_special)
    return select_literal


def _wildcard_selector(part, parts, recursive, include_hidden):
    """Returns a function that selects direct children of a given path,
    filtering by pattern.
    """
    if include_hidden and part == '*':
        match = None  # Skip generating a pattern that would match all inputs.
    else:
        match = _compile_pattern(part, recursive, include_hidden)

    if parts:
        select_next = _selector(parts, recursive, include_hidden)
        def select_wildcard(path, rel_path, dir_fd, exists):
            close_fd = False
            try:
                arg, fd, close_fd = _open_dir(path, rel_path, dir_fd)
                if fd is not None:
                    prefix = _add_trailing_slash(path)
                # Ensure we don't exhaust file descriptors when globbing deep
                # trees by closing the directory *before* yielding anything.
                with os.scandir(arg) as scandir_it:
                    entries = list(scandir_it)
                for entry in entries:
                    if match is None or match(entry.name):
                        try:
                            if entry.is_dir():
                                entry_path = entry.path
                                if fd is not None:
                                    entry_path = prefix + entry_path
                                yield from select_next(entry_path, entry.name, fd, True)
                        except OSError:
                            pass
            except OSError:
                pass
            finally:
                if close_fd:
                    os.close(fd)

    else:
        def select_wildcard(path, rel_path, dir_fd, exists):
            close_fd = False
            try:
                arg, fd, close_fd = _open_dir(path, rel_path, dir_fd)
                prefix = _add_trailing_slash(path)
                # We use listdir() rather than scandir() because we don't need
                # to check for subdirectories; we only need the child names.
                for name in os.listdir(arg):
                    if match is None or match(name):
                        yield prefix + name
            except OSError:
                pass
            finally:
                if close_fd:
                    os.close(fd)

    return select_wildcard


def _recursive_selector(part, parts, recursive, include_hidden):
    """Returns a function that selects a given path and all its children,
    recursively, filtering by pattern.
    """
    while parts and parts[0] == '**':
        parts = parts[1:]
    while parts and (next_part := parts[0]) not in _special_parts:
        # Consume next non-special component (used to build regex).
        part += os.path.sep + next_part
        parts = parts[1:]

    if include_hidden and part == '**':
        match = None  # Skip generating a pattern that would match all inputs.
    else:
        match = _compile_pattern(part, recursive, include_hidden)

    dir_only = bool(parts)
    select_next = _selector(parts, recursive, include_hidden)

    def select_recursive(path, rel_path, dir_fd, exists):
        path = _add_trailing_slash(path)
        rel_path = _add_trailing_slash(rel_path)
        match_pos = len(path)
        if match is None or match(path, match_pos):
            yield from select_next(path, rel_path, dir_fd, exists)
        yield from select_recursive_step(path, rel_path, dir_fd, match_pos)

    def select_recursive_step(path, rel_path, dir_fd, match_pos):
        close_fd = False
        try:
            arg, fd, close_fd = _open_dir(path, rel_path, dir_fd)
            if fd is not None:
                prefix = _add_trailing_slash(path)
            # Ensure we don't exhaust file descriptors when globbing deep
            # trees by closing the directory *before* yielding anything.
            with os.scandir(arg) as scandir_it:
                entries = list(scandir_it)
            for entry in entries:
                is_dir = False
                try:
                    if entry.is_dir():
                        is_dir = True
                except OSError:
                    pass

                if is_dir or not dir_only:
                    entry_path = entry.path
                    if fd is not None:
                        entry_path = prefix + entry_path
                    if match is None or match(entry_path, match_pos):
                        if dir_only:
                            yield from select_next(entry_path, entry.name, fd, True)
                        else:
                            yield entry_path
                    if is_dir:
                        yield from select_recursive_step(entry_path, entry.name, fd, match_pos)
        except OSError:
            pass
        finally:
            if close_fd:
                os.close(fd)
    return select_recursive


def _select_exists(path, rel_path, dir_fd, exists):
    """Yields the given path, if it exists.
    """
    if exists:
        yield path
    elif dir_fd is None:
        try:
            os.lstat(path)
            yield path
        except OSError:
            pass
    else:
        try:
            os.lstat(rel_path, dir_fd=dir_fd)
            yield path
        except OSError:
            pass


def _legacy_glob(selector, dirname, pattern):
    """Implements the undocumented glob0() and glob1() functions.
    """
    root = _add_trailing_slash(dirname)
    root_slicer = operator.itemgetter(slice(len(root), None))
    select = selector(pattern, (), False, False)
    paths = select(dirname, dirname, None, False)
    paths = map(root_slicer, paths)
    return list(paths)


glob0 = functools.partial(_legacy_glob, _literal_selector)
glob1 = functools.partial(_legacy_glob, _wildcard_selector)
