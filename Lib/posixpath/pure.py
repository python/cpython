"""Pure operations on Posix pathnames.

This can actually be useful on non-Posix systems too, e.g.
for manipulation of the pathname component of URLs.
"""
import os
import posixpath
from posixpath import (altsep, basename, commonpath, commonprefix, curdir,
                       dirname, extsep, isabs, join, normcase, normpath,
                       pardir, pathsep, sep, split, splitdrive, splitext,
                       splitroot)

__all__ = ["altsep", "basename", "commonpath", "commonprefix", "curdir",
           "dirname", "extsep", "isabs", "join", "normcase", "normpath",
           "pardir", "pathsep", "relpath", "sep", "split", "splitdrive",
           "splitext", "splitroot"]

def relpath(path, start):
    """Return a relative version of a path"""
    path = os.fspath(path)
    start = os.fspath(start)
    if not isabs(path) or not isabs(start):
        raise ValueError("paths are not absolute")

    return posixpath.relpath(path, start)