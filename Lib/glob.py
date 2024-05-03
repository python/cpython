"""Filename globbing utility."""

import os
import itertools
import operator
import sys

from pathlib._glob import translate, magic_check, magic_check_bytes, Globber

__all__ = ["glob", "iglob", "escape", "translate"]

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

    globber = Globber(recursive=recursive, include_hidden=include_hidden)
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
    return list(_relative_glob(Globber().literal_selector(pattern, []), dirname))

def glob1(dirname, pattern):
    import warnings
    warnings._deprecated("glob.glob1", _deprecated_function_message, remove=(3, 15))
    return list(_relative_glob(Globber().wildcard_selector(pattern, []), dirname))

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
    dirname = Globber.add_slash(dirname)
    slicer = operator.itemgetter(slice(len(dirname), None))
    return map(slicer, select(dirname, dir_fd, dirname))

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
