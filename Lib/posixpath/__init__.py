"""Common operations on Posix pathnames.

Instead of importing this module directly, import os and refer to
this module as os.path.  The "os.path" name is an alias for this
module on Posix systems; on other systems (e.g. Windows),
os.path provides the same operations in a manner specific to that
platform, and is an alias to another module (e.g. ntpath).
"""

# Strings representing various path-related bits and pieces.
# These are primarily for export; internally, they are hardcoded.
# Should be set before imports for resolving cyclic dependency.
curdir = '.'
pardir = '..'
extsep = '.'
sep = '/'
pathsep = ':'
defpath = '/bin:/usr/bin'
altsep = None
devnull = '/dev/null'

import errno
import os
import sys
import stat
from genericpath import *
import posixpath.pure
from posixpath.pure import *

__all__ = ["abspath", "altsep", "basename", "commonpath", "commonprefix",
           "curdir", "defpath", "devnull", "dirname", "exists", "expanduser",
           "expandvars", "extsep", "getatime", "getctime", "getmtime",
           "getsize", "isabs", "isdevdrive", "isdir", "isfile", "isjunction",
           "islink", "ismount", "join", "lexists", "normcase", "normpath",
           "pardir", "pathsep", "realpath", "relpath", "samefile",
           "sameopenfile", "samestat", "sep", "split", "splitdrive",
           "splitext", "splitroot", "supports_unicode_filenames"]


# Is a path a mount point?
# (Does this work for all UNIXes?  Is it even guaranteed to work by Posix?)

def ismount(path):
    """Test whether a path is a mount point"""
    try:
        s1 = os.lstat(path)
    except (OSError, ValueError):
        # It doesn't exist -- so not a mount point. :-)
        return False
    else:
        # A symlink can never be a mount point
        if stat.S_ISLNK(s1.st_mode):
            return False

    path = os.fspath(path)
    if isinstance(path, bytes):
        parent = join(path, b'..')
    else:
        parent = join(path, '..')
    try:
        s2 = os.lstat(parent)
    except OSError:
        parent = realpath(parent)
        try:
            s2 = os.lstat(parent)
        except OSError:
            return False

    # path/.. on a different device as path or the same i-node as path
    return s1.st_dev != s2.st_dev or s1.st_ino == s2.st_ino


# Expand paths beginning with '~' or '~user'.
# '~' means $HOME; '~user' means that user's home directory.
# If the path doesn't begin with '~', or if the user or $HOME is unknown,
# the path is returned unchanged (leaving error reporting to whatever
# function is called with the expanded path as argument).
# See also module 'glob' for expansion of *, ? and [...] in pathnames.
# (A function should also be defined to do full *sh-style environment
# variable expansion.)

def expanduser(path):
    """Expand ~ and ~user constructions.  If user or $HOME is unknown,
    do nothing."""
    path = os.fspath(path)
    if isinstance(path, bytes):
        sep = b'/'
        tilde = b'~'
    else:
        sep = '/'
        tilde = '~'
    if not path.startswith(tilde):
        return path
    i = path.find(sep, 1)
    if i < 0:
        i = len(path)
    if i == 1:
        if 'HOME' not in os.environ:
            try:
                import pwd
            except ImportError:
                # pwd module unavailable, return path unchanged
                return path
            try:
                userhome = pwd.getpwuid(os.getuid()).pw_dir
            except KeyError:
                # bpo-10496: if the current user identifier doesn't exist in the
                # password database, return the path unchanged
                return path
        else:
            userhome = os.environ['HOME']
    else:
        try:
            import pwd
        except ImportError:
            # pwd module unavailable, return path unchanged
            return path
        name = path[1:i]
        if isinstance(name, bytes):
            name = os.fsdecode(name)
        try:
            pwent = pwd.getpwnam(name)
        except KeyError:
            # bpo-10496: if the user name from the path doesn't exist in the
            # password database, return the path unchanged
            return path
        userhome = pwent.pw_dir
    # if no user home, return the path unchanged on VxWorks
    if userhome is None and sys.platform == "vxworks":
        return path
    if isinstance(path, bytes):
        userhome = os.fsencode(userhome)
    userhome = userhome.rstrip(sep)
    return (userhome + path[i:]) or sep


# Expand paths containing shell variable substitutions.
# This expands the forms $variable and ${variable} only.
# Non-existent variables are left unchanged.

_varprog = None
_varprogb = None

def expandvars(path):
    """Expand shell variables of form $var and ${var}.  Unknown variables
    are left unchanged."""
    path = os.fspath(path)
    global _varprog, _varprogb
    if isinstance(path, bytes):
        if b'$' not in path:
            return path
        if not _varprogb:
            import re
            _varprogb = re.compile(br'\$(\w+|\{[^}]*\})', re.ASCII)
        search = _varprogb.search
        start = b'{'
        end = b'}'
        environ = getattr(os, 'environb', None)
    else:
        if '$' not in path:
            return path
        if not _varprog:
            import re
            _varprog = re.compile(r'\$(\w+|\{[^}]*\})', re.ASCII)
        search = _varprog.search
        start = '{'
        end = '}'
        environ = os.environ
    i = 0
    while True:
        m = search(path, i)
        if not m:
            break
        i, j = m.span(0)
        name = m.group(1)
        if name.startswith(start) and name.endswith(end):
            name = name[1:-1]
        try:
            if environ is None:
                value = os.fsencode(os.environ[os.fsdecode(name)])
            else:
                value = environ[name]
        except KeyError:
            i = j
        else:
            tail = path[j:]
            path = path[:i] + value
            i = len(path)
            path += tail
    return path


def abspath(path):
    """Return an absolute path."""
    path = os.fspath(path)
    if isinstance(path, bytes):
        if not path.startswith(b'/'):
            path = join(os.getcwdb(), path)
    else:
        if not path.startswith('/'):
            path = join(os.getcwd(), path)
    return normpath(path)


# Return a canonical path (i.e. the absolute location of a file on the
# filesystem).

def realpath(filename, *, strict=False):
    """Return the canonical path of the specified filename, eliminating any
symbolic links encountered in the path."""
    filename = os.fspath(filename)
    if isinstance(filename, bytes):
        sep = b'/'
        curdir = b'.'
        pardir = b'..'
        getcwd = os.getcwdb
    else:
        sep = '/'
        curdir = '.'
        pardir = '..'
        getcwd = os.getcwd
    return _realpath(filename, strict, sep, curdir, pardir, getcwd)

def _realpath(filename, strict=False, sep=sep, curdir=curdir, pardir=pardir,
              getcwd=os.getcwd, lstat=os.lstat, readlink=os.readlink, maxlinks=None):
    # The stack of unresolved path parts. When popped, a special value of None
    # indicates that a symlink target has been resolved, and that the original
    # symlink path can be retrieved by popping again. The [::-1] slice is a
    # very fast way of spelling list(reversed(...)).
    rest = filename.split(sep)[::-1]

    # The resolved path, which is absolute throughout this function.
    # Note: getcwd() returns a normalized and symlink-free path.
    path = sep if filename.startswith(sep) else getcwd()

    # Mapping from symlink paths to *fully resolved* symlink targets. If a
    # symlink is encountered but not yet resolved, the value is None. This is
    # used both to detect symlink loops and to speed up repeated traversals of
    # the same links.
    seen = {}

    # Number of symlinks traversed. When the number of traversals is limited
    # by *maxlinks*, this is used instead of *seen* to detect symlink loops.
    link_count = 0

    while rest:
        name = rest.pop()
        if name is None:
            # resolved symlink target
            seen[rest.pop()] = path
            continue
        if not name or name == curdir:
            # current dir
            continue
        if name == pardir:
            # parent dir
            path = path[:path.rindex(sep)] or sep
            continue
        if path == sep:
            newpath = path + name
        else:
            newpath = path + sep + name
        try:
            st = lstat(newpath)
            if not stat.S_ISLNK(st.st_mode):
                path = newpath
                continue
            elif maxlinks is not None:
                link_count += 1
                if link_count > maxlinks:
                    if strict:
                        raise OSError(errno.ELOOP, os.strerror(errno.ELOOP),
                                      newpath)
                    path = newpath
                    continue
            elif newpath in seen:
                # Already seen this path
                path = seen[newpath]
                if path is not None:
                    # use cached value
                    continue
                # The symlink is not resolved, so we must have a symlink loop.
                if strict:
                    raise OSError(errno.ELOOP, os.strerror(errno.ELOOP),
                                  newpath)
                path = newpath
                continue
            target = readlink(newpath)
        except OSError:
            if strict:
                raise
            path = newpath
            continue
        # Resolve the symbolic link
        if target.startswith(sep):
            # Symlink target is absolute; reset resolved path.
            path = sep
        if maxlinks is None:
            # Mark this symlink as seen but not fully resolved.
            seen[newpath] = None
            # Push the symlink path onto the stack, and signal its specialness
            # by also pushing None. When these entries are popped, we'll
            # record the fully-resolved symlink target in the 'seen' mapping.
            rest.append(newpath)
            rest.append(None)
        # Push the unresolved symlink target parts onto the stack.
        rest.extend(target.split(sep)[::-1])

    return path


supports_unicode_filenames = (sys.platform == 'darwin')

def relpath(path, start=None):
    """Return a relative version of a path"""

    path = os.fspath(path)
    if not path:
        raise ValueError("no path specified")

    if start is None:
        start = b'.' if isinstance(path, bytes) else '.'

    return posixpath.pure.relpath(abspath(path), abspath(start))
