"""distutils.dep_util

Utility functions for simple, timestamp-based dependency of files
and groups of files; also, function based entirely on such
timestamp dependency analysis."""

import os
from distutils.errors import DistutilsFileError


def newer (source, target):
    """Return true if 'source' exists and is more recently modified than
    'target', or if 'source' exists and 'target' doesn't.  Return false if
    both exist and 'target' is the same age or younger than 'source'.
    Raise DistutilsFileError if 'source' does not exist.
    """
    if not os.path.exists(source):
        raise DistutilsFileError("file '%s' does not exist" %
                                 os.path.abspath(source))
    if not os.path.exists(target):
        return 1

    from stat import ST_MTIME
    mtime1 = os.stat(source)[ST_MTIME]
    mtime2 = os.stat(target)[ST_MTIME]

    return mtime1 > mtime2

# newer ()
