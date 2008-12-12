"""Cache lines from files.

This is intended to read lines from modules imported -- hence if a filename
is not found, it will look down the module search path for a file by
that name.
"""

import sys
import os
import re

__all__ = ["getline", "clearcache", "checkcache"]

def getline(filename, lineno, module_globals=None):
    lines = getlines(filename, module_globals)
    if 1 <= lineno <= len(lines):
        return lines[lineno-1]
    else:
        return ''


# The cache

cache = {} # The cache


def clearcache():
    """Clear the cache entirely."""

    global cache
    cache = {}


def getlines(filename, module_globals=None):
    """Get the lines for a file from the cache.
    Update the cache if it doesn't contain an entry for this file already."""

    if filename in cache:
        return cache[filename][2]
    else:
        return updatecache(filename, module_globals)


def checkcache(filename=None):
    """Discard cache entries that are out of date.
    (This is not checked upon each call!)"""

    if filename is None:
        filenames = list(cache.keys())
    else:
        if filename in cache:
            filenames = [filename]
        else:
            return

    for filename in filenames:
        size, mtime, lines, fullname = cache[filename]
        if mtime is None:
            continue   # no-op for files loaded via a __loader__
        try:
            stat = os.stat(fullname)
        except os.error:
            del cache[filename]
            continue
        if size != stat.st_size or mtime != stat.st_mtime:
            del cache[filename]


def updatecache(filename, module_globals=None):
    """Update a cache entry and return its list of lines.
    If something's wrong, print a message, discard the cache entry,
    and return an empty list."""

    if filename in cache:
        del cache[filename]
    if not filename or filename[0] + filename[-1] == '<>':
        return []

    fullname = filename
    try:
        stat = os.stat(fullname)
    except os.error as msg:
        basename = os.path.split(filename)[1]

        # Try for a __loader__, if available
        if module_globals and '__loader__' in module_globals:
            name = module_globals.get('__name__')
            loader = module_globals['__loader__']
            get_source = getattr(loader, 'get_source', None)

            if name and get_source:
                if basename.startswith(name.split('.')[-1]+'.'):
                    try:
                        data = get_source(name)
                    except (ImportError, IOError):
                        pass
                    else:
                        if data is None:
                            # No luck, the PEP302 loader cannot find the source
                            # for this module.
                            return []
                        cache[filename] = (
                            len(data), None,
                            [line+'\n' for line in data.splitlines()], fullname
                        )
                        return cache[filename][2]

        # Try looking through the module search path.

        for dirname in sys.path:
            try:
                fullname = os.path.join(dirname, basename)
            except (TypeError, AttributeError):
                # Not sufficiently string-like to do anything useful with.
                pass
            else:
                try:
                    stat = os.stat(fullname)
                    break
                except os.error:
                    pass
        else:
            # No luck
##          print '*** Cannot stat', filename, ':', msg
            return []
##  print("Refreshing cache for %s..." % fullname)
    try:
        fp = open(fullname, 'rU')
        lines = fp.readlines()
        fp.close()
    except Exception as msg:
##      print '*** Cannot open', fullname, ':', msg
        return []
    coding = "utf-8"
    for line in lines[:2]:
        m = re.search(r"coding[:=]\s*([-\w.]+)", line)
        if m:
            coding = m.group(1)
            break
    try:
        lines = [line if isinstance(line, str) else str(line, coding)
                 for line in lines]
    except:
        pass  # Hope for the best
    size, mtime = stat.st_size, stat.st_mtime
    cache[filename] = size, mtime, lines, fullname
    return lines
