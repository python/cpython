# Module 'ntpath.pure' -- pure operations on WinNT/Win95 pathnames
"""Pure pathname manipulations, WindowsNT/95 version."""
import os
import ntpath
from ntpath import (altsep, basename, commonpath, commonprefix, curdir,
                    dirname, extsep, isabs, isreserved, join, normcase,
                    normpath, pardir, pathsep, sep, split, splitdrive,
                    splitext, splitroot)

__all__ = ["altsep", "basename", "commonpath", "commonprefix", "curdir",
           "dirname", "extsep", "isabs", "isreserved", "join", "normcase",
           "normpath", "pardir", "pathsep", "relpath", "sep", "split",
           "splitdrive", "splitext", "splitroot"]

def relpath(path, start):
    """Return a relative version of a path"""
    path = os.fspath(path)
    start = os.fspath(start)
    if not isabs(path) or not isabs(start):
        raise ValueError("paths are not absolute")

    return ntpath.relpath(path, start)