"""Maintain a cache of stat() information on files.

There are functions to reset the cache or to selectively remove items.
"""

import warnings
warnings.warn("The statcache module is obsolete.  Use os.stat() instead.",
              DeprecationWarning)
del warnings

import os as _os
from stat import *

__all__ = ["stat","reset","forget","forget_prefix","forget_dir",
           "forget_except_prefix","isdir"]

# The cache.  Keys are pathnames, values are os.stat outcomes.
# Remember that multiple threads may be calling this!  So, e.g., that
# path in cache returns 1 doesn't mean the cache will still contain
# path on the next line.  Code defensively.

cache = {}

def stat(path):
    """Stat a file, possibly out of the cache."""
    ret = cache.get(path, None)
    if ret is None:
        cache[path] = ret = _os.stat(path)
    return ret

def reset():
    """Clear the cache."""
    cache.clear()

# For thread saftey, always use forget() internally too.
def forget(path):
    """Remove a given item from the cache, if it exists."""
    try:
        del cache[path]
    except KeyError:
        pass

def forget_prefix(prefix):
    """Remove all pathnames with a given prefix."""
    for path in cache.keys():
        if path.startswith(prefix):
            forget(path)

def forget_dir(prefix):
    """Forget a directory and all entries except for entries in subdirs."""

    # Remove trailing separator, if any.  This is tricky to do in a
    # x-platform way.  For example, Windows accepts both / and \ as
    # separators, and if there's nothing *but* a separator we want to
    # preserve that this is the root.  Only os.path has the platform
    # knowledge we need.
    from os.path import split, join
    prefix = split(join(prefix, "xxx"))[0]
    forget(prefix)
    for path in cache.keys():
        # First check that the path at least starts with the prefix, so
        # that when it doesn't we can avoid paying for split().
        if path.startswith(prefix) and split(path)[0] == prefix:
            forget(path)

def forget_except_prefix(prefix):
    """Remove all pathnames except with a given prefix.

    Normally used with prefix = '/' after a chdir().
    """

    for path in cache.keys():
        if not path.startswith(prefix):
            forget(path)

def isdir(path):
    """Return True if directory, else False."""
    try:
        st = stat(path)
    except _os.error:
        return False
    return S_ISDIR(st.st_mode)
