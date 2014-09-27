"""Pathname and path-related operations for the Macintosh."""

import os
from stat import *
import genericpath
from genericpath import *

__all__ = ["normcase","isabs","join","splitdrive","split","splitext",
           "basename","dirname","commonprefix","getsize","getmtime",
           "getatime","getctime", "islink","exists","lexists","isdir","isfile",
           "expanduser","expandvars","normpath","abspath",
           "curdir","pardir","sep","pathsep","defpath","altsep","extsep",
           "devnull","realpath","supports_unicode_filenames"]

# strings representing various path-related bits and pieces
# These are primarily for export; internally, they are hardcoded.
curdir = ':'
pardir = '::'
extsep = '.'
sep = ':'
pathsep = '\n'
defpath = ':'
altsep = None
devnull = 'Dev:Null'

def _get_colon(path):
    if isinstance(path, bytes):
        return b':'
    else:
        return ':'

# Normalize the case of a pathname.  Dummy in Posix, but <s>.lower() here.

def normcase(path):
    if not isinstance(path, (bytes, str)):
        raise TypeError("normcase() argument must be str or bytes, "
                        "not '{}'".format(path.__class__.__name__))
    return path.lower()


def isabs(s):
    """Return true if a path is absolute.
    On the Mac, relative paths begin with a colon,
    but as a special case, paths with no colons at all are also relative.
    Anything else is absolute (the string up to the first colon is the
    volume name)."""

    colon = _get_colon(s)
    return colon in s and s[:1] != colon


def join(s, *p):
    colon = _get_colon(s)
    path = s
    for t in p:
        if (not path) or isabs(t):
            path = t
            continue
        if t[:1] == colon:
            t = t[1:]
        if colon not in path:
            path = colon + path
        if path[-1:] != colon:
            path = path + colon
        path = path + t
    return path


def split(s):
    """Split a pathname into two parts: the directory leading up to the final
    bit, and the basename (the filename, without colons, in that directory).
    The result (s, t) is such that join(s, t) yields the original argument."""

    colon = _get_colon(s)
    if colon not in s: return s[:0], s
    col = 0
    for i in range(len(s)):
        if s[i:i+1] == colon: col = i + 1
    path, file = s[:col-1], s[col:]
    if path and not colon in path:
        path = path + colon
    return path, file


def splitext(p):
    if isinstance(p, bytes):
        return genericpath._splitext(p, b':', altsep, b'.')
    else:
        return genericpath._splitext(p, sep, altsep, extsep)
splitext.__doc__ = genericpath._splitext.__doc__

def splitdrive(p):
    """Split a pathname into a drive specification and the rest of the
    path.  Useful on DOS/Windows/NT; on the Mac, the drive is always
    empty (don't use the volume name -- it doesn't have the same
    syntactic and semantic oddities as DOS drive letters, such as there
    being a separate current directory per drive)."""

    return p[:0], p


# Short interfaces to split()

def dirname(s): return split(s)[0]
def basename(s): return split(s)[1]

def ismount(s):
    if not isabs(s):
        return False
    components = split(s)
    return len(components) == 2 and not components[1]

def islink(s):
    """Return true if the pathname refers to a symbolic link."""

    try:
        import Carbon.File
        return Carbon.File.ResolveAliasFile(s, 0)[2]
    except:
        return False

# Is `stat`/`lstat` a meaningful difference on the Mac?  This is safe in any
# case.

def lexists(path):
    """Test whether a path exists.  Returns True for broken symbolic links"""

    try:
        st = os.lstat(path)
    except OSError:
        return False
    return True

def expandvars(path):
    """Dummy to retain interface-compatibility with other operating systems."""
    return path


def expanduser(path):
    """Dummy to retain interface-compatibility with other operating systems."""
    return path

class norm_error(Exception):
    """Path cannot be normalized"""

def normpath(s):
    """Normalize a pathname.  Will return the same result for
    equivalent paths."""

    colon = _get_colon(s)

    if colon not in s:
        return colon + s

    comps = s.split(colon)
    i = 1
    while i < len(comps)-1:
        if not comps[i] and comps[i-1]:
            if i > 1:
                del comps[i-1:i+1]
                i = i - 1
            else:
                # best way to handle this is to raise an exception
                raise norm_error('Cannot use :: immediately after volume name')
        else:
            i = i + 1

    s = colon.join(comps)

    # remove trailing ":" except for ":" and "Volume:"
    if s[-1:] == colon and len(comps) > 2 and s != colon*len(s):
        s = s[:-1]
    return s

def abspath(path):
    """Return an absolute path."""
    if not isabs(path):
        if isinstance(path, bytes):
            cwd = os.getcwdb()
        else:
            cwd = os.getcwd()
        path = join(cwd, path)
    return normpath(path)

# realpath is a no-op on systems without islink support
def realpath(path):
    path = abspath(path)
    try:
        import Carbon.File
    except ImportError:
        return path
    if not path:
        return path
    colon = _get_colon(path)
    components = path.split(colon)
    path = components[0] + colon
    for c in components[1:]:
        path = join(path, c)
        try:
            path = Carbon.File.FSResolveAliasFile(path, 1)[0].as_pathname()
        except Carbon.File.Error:
            pass
    return path

supports_unicode_filenames = True
