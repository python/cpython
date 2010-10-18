# Module 'ntpath' -- common operations on WinNT/Win95 pathnames
"""Common pathname manipulations, WindowsNT/95 version.

Instead of importing this module directly, import os and refer to this
module as os.path.
"""

import os
import sys
import stat
import genericpath
from genericpath import *

__all__ = ["normcase","isabs","join","splitdrive","split","splitext",
           "basename","dirname","commonprefix","getsize","getmtime",
           "getatime","getctime", "islink","exists","lexists","isdir","isfile",
           "ismount", "expanduser","expandvars","normpath","abspath",
           "splitunc","curdir","pardir","sep","pathsep","defpath","altsep",
           "extsep","devnull","realpath","supports_unicode_filenames","relpath"]

# strings representing various path-related bits and pieces
# These are primarily for export; internally, they are hardcoded.
curdir = '.'
pardir = '..'
extsep = '.'
sep = '\\'
pathsep = ';'
altsep = '/'
defpath = '.;C:\\bin'
if 'ce' in sys.builtin_module_names:
    defpath = '\\Windows'
elif 'os2' in sys.builtin_module_names:
    # OS/2 w/ VACPP
    altsep = '/'
devnull = 'nul'

def _get_empty(path):
    if isinstance(path, bytes):
        return b''
    else:
        return ''

def _get_sep(path):
    if isinstance(path, bytes):
        return b'\\'
    else:
        return '\\'

def _get_altsep(path):
    if isinstance(path, bytes):
        return b'/'
    else:
        return '/'

def _get_bothseps(path):
    if isinstance(path, bytes):
        return b'\\/'
    else:
        return '\\/'

def _get_dot(path):
    if isinstance(path, bytes):
        return b'.'
    else:
        return '.'

def _get_colon(path):
    if isinstance(path, bytes):
        return b':'
    else:
        return ':'

def _get_special(path):
    if isinstance(path, bytes):
        return (b'\\\\.\\', b'\\\\?\\')
    else:
        return ('\\\\.\\', '\\\\?\\')

# Normalize the case of a pathname and map slashes to backslashes.
# Other normalizations (such as optimizing '../' away) are not done
# (this is done by normpath).

def normcase(s):
    """Normalize case of pathname.

    Makes all characters lowercase and all slashes into backslashes."""
    return s.replace(_get_altsep(s), _get_sep(s)).lower()


# Return whether a path is absolute.
# Trivial in Posix, harder on Windows.
# For Windows it is absolute if it starts with a slash or backslash (current
# volume), or if a pathname after the volume-letter-and-colon or UNC-resource
# starts with a slash or backslash.

def isabs(s):
    """Test whether a path is absolute"""
    s = splitdrive(s)[1]
    return len(s) > 0 and s[:1] in _get_bothseps(s)


# Join two (or more) paths.

def join(a, *p):
    """Join two or more pathname components, inserting "\\" as needed.
    If any component is an absolute path, all previous path components
    will be discarded."""
    sep = _get_sep(a)
    seps = _get_bothseps(a)
    colon = _get_colon(a)
    path = a
    for b in p:
        b_wins = 0  # set to 1 iff b makes path irrelevant
        if not path:
            b_wins = 1

        elif isabs(b):
            # This probably wipes out path so far.  However, it's more
            # complicated if path begins with a drive letter.  You get a+b
            # (minus redundant slashes) in these four cases:
            #     1. join('c:', '/a') == 'c:/a'
            #     2. join('//computer/share', '/a') == '//computer/share/a'
            #     3. join('c:/', '/a') == 'c:/a'
            #     4. join('//computer/share/', '/a') == '//computer/share/a'
            # But b wins in all of these cases:
            #     5. join('c:/a', '/b') == '/b'
            #     6. join('//computer/share/a', '/b') == '/b'
            #     7. join('c:', 'd:/') == 'd:/'
            #     8. join('c:', '//computer/share/') == '//computer/share/'
            #     9. join('//computer/share', 'd:/') == 'd:/'
            #    10. join('//computer/share', '//computer/share/') == '//computer/share/'
            #    11. join('c:/', 'd:/') == 'd:/'
            #    12. join('c:/', '//computer/share/') == '//computer/share/'
            #    13. join('//computer/share/', 'd:/') == 'd:/'
            #    14. join('//computer/share/', '//computer/share/') == '//computer/share/'
            b_prefix, b_rest = splitdrive(b)

            # if b has a prefix, it always wins.
            if b_prefix:
                b_wins = 1
            else:
                # b doesn't have a prefix.
                # but isabs(b) returned true.
                # and therefore b_rest[0] must be a slash.
                # (but let's check that.)
                assert(b_rest and b_rest[0] in seps)

                # so, b still wins if path has a rest that's more than a sep.
                # you get a+b if path_rest is empty or only has a sep.
                # (see cases 1-4 for times when b loses.)
                path_rest = splitdrive(path)[1]
                b_wins = path_rest and path_rest not in seps

        if b_wins:
            path = b
        else:
            # Join, and ensure there's a separator.
            assert len(path) > 0
            if path[-1:] in seps:
                if b and b[:1] in seps:
                    path += b[1:]
                else:
                    path += b
            elif path[-1:] == colon:
                path += b
            elif b:
                if b[:1] in seps:
                    path += b
                else:
                    path += sep + b
            else:
                # path is not empty and does not end with a backslash,
                # but b is empty; since, e.g., split('a/') produces
                # ('a', ''), it's best if join() adds a backslash in
                # this case.
                path += sep

    return path


# Split a path in a drive specification (a drive letter followed by a
# colon) and the path specification.
# It is always true that drivespec + pathspec == p
def splitdrive(p):
    """Split a pathname into drive/UNC sharepoint and relative path specifiers.
    Returns a 2-tuple (drive_or_unc, path); either part may be empty.

    If you assign
        result = splitdrive(p)
    It is always true that:
        result[0] + result[1] == p

    If the path contained a drive letter, drive_or_unc will contain everything
    up to and including the colon.  e.g. splitdrive("c:/dir") returns ("c:", "/dir")

    If the path contained a UNC path, the drive_or_unc will contain the host name
    and share up to but not including the fourth directory separator character.
    e.g. splitdrive("//host/computer/dir") returns ("//host/computer", "/dir")

    Paths cannot contain both a drive letter and a UNC path.

    """
    empty = _get_empty(p)
    if len(p) > 1:
        sep = _get_sep(p)
        normp = normcase(p)
        if (normp[0:2] == sep*2) and (normp[2:3] != sep):
            # is a UNC path:
            # vvvvvvvvvvvvvvvvvvvv drive letter or UNC path
            # \\machine\mountpoint\directory\etc\...
            #           directory ^^^^^^^^^^^^^^^
            index = normp.find(sep, 2)
            if index == -1:
                return empty, p
            index2 = normp.find(sep, index + 1)
            # a UNC path can't have two slashes in a row
            # (after the initial two)
            if index2 == index + 1:
                return empty, p
            if index2 == -1:
                index2 = len(p)
            return p[:index2], p[index2:]
        if normp[1:2] == _get_colon(p):
            return p[:2], p[2:]
    return empty, p


# Parse UNC paths
def splitunc(p):
    """Deprecated since Python 3.1.  Please use splitdrive() instead;
    it now handles UNC paths.

    Split a pathname into UNC mount point and relative path specifiers.

    Return a 2-tuple (unc, rest); either part may be empty.
    If unc is not empty, it has the form '//host/mount' (or similar
    using backslashes).  unc+rest is always the input path.
    Paths containing drive letters never have an UNC part.
    """
    import warnings
    warnings.warn("ntpath.splitunc is deprecated, use ntpath.splitdrive instead",
                  PendingDeprecationWarning)
    sep = _get_sep(p)
    if not p[1:2]:
        return p[:0], p # Drive letter present
    firstTwo = p[0:2]
    if normcase(firstTwo) == sep + sep:
        # is a UNC path:
        # vvvvvvvvvvvvvvvvvvvv equivalent to drive letter
        # \\machine\mountpoint\directories...
        #           directory ^^^^^^^^^^^^^^^
        normp = normcase(p)
        index = normp.find(sep, 2)
        if index == -1:
            ##raise RuntimeError, 'illegal UNC path: "' + p + '"'
            return (p[:0], p)
        index = normp.find(sep, index + 1)
        if index == -1:
            index = len(p)
        return p[:index], p[index:]
    return p[:0], p


# Split a path in head (everything up to the last '/') and tail (the
# rest).  After the trailing '/' is stripped, the invariant
# join(head, tail) == p holds.
# The resulting head won't end in '/' unless it is the root.

def split(p):
    """Split a pathname.

    Return tuple (head, tail) where tail is everything after the final slash.
    Either part may be empty."""

    seps = _get_bothseps(p)
    d, p = splitdrive(p)
    # set i to index beyond p's last slash
    i = len(p)
    while i and p[i-1] not in seps:
        i -= 1
    head, tail = p[:i], p[i:]  # now tail has no slashes
    # remove trailing slashes from head, unless it's all slashes
    head2 = head
    while head2 and head2[-1:] in seps:
        head2 = head2[:-1]
    head = head2 or head
    return d + head, tail


# Split a path in root and extension.
# The extension is everything starting at the last dot in the last
# pathname component; the root is everything before that.
# It is always true that root + ext == p.

def splitext(p):
    return genericpath._splitext(p, _get_sep(p), _get_altsep(p),
                                 _get_dot(p))
splitext.__doc__ = genericpath._splitext.__doc__


# Return the tail (basename) part of a path.

def basename(p):
    """Returns the final component of a pathname"""
    return split(p)[1]


# Return the head (dirname) part of a path.

def dirname(p):
    """Returns the directory component of a pathname"""
    return split(p)[0]

# Is a path a symbolic link?
# This will always return false on systems where posix.lstat doesn't exist.

def islink(path):
    """Test for symbolic link.
    On WindowsNT/95 and OS/2 always returns false
    """
    return False

# alias exists to lexists
lexists = exists

# Is a path a mount point?  Either a root (with or without drive letter)
# or an UNC path with at most a / or \ after the mount point.

def ismount(path):
    """Test whether a path is a mount point (defined as root of drive)"""
    seps = _get_bothseps(path)
    root, rest = splitdrive(path)
    if root and root[0] in seps:
        return (not rest) or (rest in seps)
    return rest in seps


# Expand paths beginning with '~' or '~user'.
# '~' means $HOME; '~user' means that user's home directory.
# If the path doesn't begin with '~', or if the user or $HOME is unknown,
# the path is returned unchanged (leaving error reporting to whatever
# function is called with the expanded path as argument).
# See also module 'glob' for expansion of *, ? and [...] in pathnames.
# (A function should also be defined to do full *sh-style environment
# variable expansion.)

def expanduser(path):
    """Expand ~ and ~user constructs.

    If user or $HOME is unknown, do nothing."""
    if isinstance(path, bytes):
        tilde = b'~'
    else:
        tilde = '~'
    if not path.startswith(tilde):
        return path
    i, n = 1, len(path)
    while i < n and path[i] not in _get_bothseps(path):
        i += 1

    if 'HOME' in os.environ:
        userhome = os.environ['HOME']
    elif 'USERPROFILE' in os.environ:
        userhome = os.environ['USERPROFILE']
    elif not 'HOMEPATH' in os.environ:
        return path
    else:
        try:
            drive = os.environ['HOMEDRIVE']
        except KeyError:
            drive = ''
        userhome = join(drive, os.environ['HOMEPATH'])

    if isinstance(path, bytes):
        userhome = userhome.encode(sys.getfilesystemencoding())

    if i != 1: #~user
        userhome = join(dirname(userhome), path[1:i])

    return userhome + path[i:]


# Expand paths containing shell variable substitutions.
# The following rules apply:
#       - no expansion within single quotes
#       - '$$' is translated into '$'
#       - '%%' is translated into '%' if '%%' are not seen in %var1%%var2%
#       - ${varname} is accepted.
#       - $varname is accepted.
#       - %varname% is accepted.
#       - varnames can be made out of letters, digits and the characters '_-'
#         (though is not verifed in the ${varname} and %varname% cases)
# XXX With COMMAND.COM you can use any characters in a variable name,
# XXX except '^|<>='.

def expandvars(path):
    """Expand shell variables of the forms $var, ${var} and %var%.

    Unknown variables are left unchanged."""
    if isinstance(path, bytes):
        if ord('$') not in path and ord('%') not in path:
            return path
        import string
        varchars = bytes(string.ascii_letters + string.digits + '_-', 'ascii')
        quote = b'\''
        percent = b'%'
        brace = b'{'
        dollar = b'$'
    else:
        if '$' not in path and '%' not in path:
            return path
        import string
        varchars = string.ascii_letters + string.digits + '_-'
        quote = '\''
        percent = '%'
        brace = '{'
        dollar = '$'
    res = path[:0]
    index = 0
    pathlen = len(path)
    while index < pathlen:
        c = path[index:index+1]
        if c == quote:   # no expansion within single quotes
            path = path[index + 1:]
            pathlen = len(path)
            try:
                index = path.index(c)
                res += c + path[:index + 1]
            except ValueError:
                res += path
                index = pathlen - 1
        elif c == percent:  # variable or '%'
            if path[index + 1:index + 2] == percent:
                res += c
                index += 1
            else:
                path = path[index+1:]
                pathlen = len(path)
                try:
                    index = path.index(percent)
                except ValueError:
                    res += percent + path
                    index = pathlen - 1
                else:
                    var = path[:index]
                    if isinstance(path, bytes):
                        var = var.decode('ascii')
                    if var in os.environ:
                        value = os.environ[var]
                    else:
                        value = '%' + var + '%'
                    if isinstance(path, bytes):
                        value = value.encode('ascii')
                    res += value
        elif c == dollar:  # variable or '$$'
            if path[index + 1:index + 2] == dollar:
                res += c
                index += 1
            elif path[index + 1:index + 2] == brace:
                path = path[index+2:]
                pathlen = len(path)
                try:
                    if isinstance(path, bytes):
                        index = path.index(b'}')
                    else:
                        index = path.index('}')
                    var = path[:index]
                    if isinstance(path, bytes):
                        var = var.decode('ascii')
                    if var in os.environ:
                        value = os.environ[var]
                    else:
                        value = '${' + var + '}'
                    if isinstance(path, bytes):
                        value = value.encode('ascii')
                    res += value
                except ValueError:
                    if isinstance(path, bytes):
                        res += b'${' + path
                    else:
                        res += '${' + path
                    index = pathlen - 1
            else:
                var = ''
                index += 1
                c = path[index:index + 1]
                while c and c in varchars:
                    if isinstance(path, bytes):
                        var += c.decode('ascii')
                    else:
                        var += c
                    index += 1
                    c = path[index:index + 1]
                if var in os.environ:
                    value = os.environ[var]
                else:
                    value = '$' + var
                if isinstance(path, bytes):
                    value = value.encode('ascii')
                res += value
                if c:
                    index -= 1
        else:
            res += c
        index += 1
    return res


# Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A\B.
# Previously, this function also truncated pathnames to 8+3 format,
# but as this module is called "ntpath", that's obviously wrong!

def normpath(path):
    """Normalize path, eliminating double slashes, etc."""
    sep = _get_sep(path)
    dotdot = _get_dot(path) * 2
    special_prefixes = _get_special(path)
    if path.startswith(special_prefixes):
        # in the case of paths with these prefixes:
        # \\.\ -> device names
        # \\?\ -> literal paths
        # do not do any normalization, but return the path unchanged
        return path
    path = path.replace(_get_altsep(path), sep)
    prefix, path = splitdrive(path)

    # collapse initial backslashes
    if path.startswith(sep):
        prefix += sep
        path = path.lstrip(sep)

    comps = path.split(sep)
    i = 0
    while i < len(comps):
        if not comps[i] or comps[i] == _get_dot(path):
            del comps[i]
        elif comps[i] == dotdot:
            if i > 0 and comps[i-1] != dotdot:
                del comps[i-1:i+1]
                i -= 1
            elif i == 0 and prefix.endswith(_get_sep(path)):
                del comps[i]
            else:
                i += 1
        else:
            i += 1
    # If the path is now empty, substitute '.'
    if not prefix and not comps:
        comps.append(_get_dot(path))
    return prefix + sep.join(comps)


# Return an absolute path.
try:
    from nt import _getfullpathname

except ImportError: # not running on Windows - mock up something sensible
    def abspath(path):
        """Return the absolute version of a path."""
        if not isabs(path):
            if isinstance(path, bytes):
                cwd = os.getcwdb()
            else:
                cwd = os.getcwd()
            path = join(cwd, path)
        return normpath(path)

else:  # use native Windows method on Windows
    def abspath(path):
        """Return the absolute version of a path."""

        if path: # Empty path must return current working directory.
            try:
                path = _getfullpathname(path)
            except WindowsError:
                pass # Bad path - return unchanged.
        elif isinstance(path, bytes):
            path = os.getcwdb()
        else:
            path = os.getcwd()
        return normpath(path)

# realpath is a no-op on systems without islink support
realpath = abspath
# Win9x family and earlier have no Unicode filename support.
supports_unicode_filenames = (hasattr(sys, "getwindowsversion") and
                              sys.getwindowsversion()[3] >= 2)

def relpath(path, start=curdir):
    """Return a relative version of a path"""
    sep = _get_sep(path)

    if start is curdir:
        start = _get_dot(path)

    if not path:
        raise ValueError("no path specified")

    start_abs = abspath(normpath(start))
    path_abs = abspath(normpath(path))
    start_drive, start_rest = splitdrive(start_abs)
    path_drive, path_rest = splitdrive(path_abs)
    if normcase(start_drive) != normcase(path_drive):
        error = "path is on mount '{0}', start on mount '{1}'".format(
            path_drive, start_drive)
        raise ValueError(error)

    start_list = [x for x in start_rest.split(sep) if x]
    path_list = [x for x in path_rest.split(sep) if x]
    # Work out how much of the filepath is shared by start and path.
    i = 0
    for e1, e2 in zip(start_list, path_list):
        if normcase(e1) != normcase(e2):
            break
        i += 1

    if isinstance(path, bytes):
        pardir = b'..'
    else:
        pardir = '..'
    rel_list = [pardir] * (len(start_list)-i) + path_list[i:]
    if not rel_list:
        return _get_dot(path)
    return join(*rel_list)
