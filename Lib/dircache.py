"""Read and cache directory listings.

The listdir() routine returns a sorted list of the files in a directory,
using a cache to avoid reading the directory more often than necessary.
The annotate() routine appends slashes to directories."""

import os

cache = {}

def listdir(path):
    """List directory contents, using cache."""
    try:
        cached_mtime, list = cache[path]
        del cache[path]
    except KeyError:
        cached_mtime, list = -1, []
    try:
        mtime = os.stat(path)[8]
    except os.error:
        return []
    if mtime <> cached_mtime:
        try:
            list = os.listdir(path)
        except os.error:
            return []
        list.sort()
    cache[path] = mtime, list
    return list

opendir = listdir # XXX backward compatibility

def annotate(head, list):
    """Add '/' suffixes to directories."""
    for i in range(len(list)):
        if os.path.isdir(os.path.join(head, list[i])):
            list[i] = list[i] + '/'
