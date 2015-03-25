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
           "extsep","devnull","realpath","supports_unicode_filenames","relpath",
           "samefile", "sameopenfile", "samestat",]

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
    if not isinstance(s, (bytes, str)):
        raise TypeError("normcase() argument must be str or bytes, "
                        "not '{}'".format(s.__class__.__name__))
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
def join(path, *paths):
    sep = _get_sep(path)
    seps = _get_bothseps(path)
    colon = _get_colon(path)
    result_drive, result_path = splitdrive(path)
    for p in paths:
        p_drive, p_path = splitdrive(p)
        if p_path and p_path[0] in seps:
            # Second path is absolute
            if p_drive or not result_drive:
                result_drive = p_drive
            result_path = p_path
            continue
        elif p_drive and p_drive != result_drive:
            if p_drive.lower() != result_drive.lower():
                # Different drives => ignore the first path entirely
                result_drive = p_drive
                result_path = p_path
                continue
            # Same drive in different case
            result_drive = p_drive
        # Second path is relative to the first
        if result_path and result_path[-1] not in seps:
            result_path = result_path + sep
        result_path = result_path + p_path
    ## add separator between UNC and non-absolute path
    if (result_path and result_path[0] not in seps and
        result_drive and result_drive[-1:] != colon):
        return result_drive + sep + result_path
    return result_drive + result_path


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
        normp = p.replace(_get_altsep(p), sep)
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
                  DeprecationWarning, 2)
    drive, path = splitdrive(p)
    if len(drive) == 2:
         # Drive letter present
        return p[:0], p
    return drive, path


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
# This will always return false on systems where os.lstat doesn't exist.

def islink(path):
    """Test whether a path is a symbolic link.
    This will always return false for Windows prior to 6.0.
    """
    try:
        st = os.lstat(path)
    except (OSError, AttributeError):
        return False
    return stat.S_ISLNK(st.st_mode)

# Being true for dangling symbolic links is also useful.

def lexists(path):
    """Test whether a path exists.  Returns True for broken symbolic links"""
    try:
        st = os.lstat(path)
    except OSError:
        return False
    return True

# Is a path a mount point?
# Any drive letter root (eg c:\)
# Any share UNC (eg \\server\share)
# Any volume mounted on a filesystem folder
#
# No one method detects all three situations. Historically we've lexically
# detected drive letter roots and share UNCs. The canonical approach to
# detecting mounted volumes (querying the reparse tag) fails for the most
# common case: drive letter roots. The alternative which uses GetVolumePathName
# fails if the drive letter is the result of a SUBST.
try:
    from nt import _getvolumepathname
except ImportError:
    _getvolumepathname = None
def ismount(path):
    """Test whether a path is a mount point (a drive root, the root of a
    share, or a mounted volume)"""
    seps = _get_bothseps(path)
    path = abspath(path)
    root, rest = splitdrive(path)
    if root and root[0] in seps:
        return (not rest) or (rest in seps)
    if rest in seps:
        return True

    if _getvolumepathname:
        return path.rstrip(seps) == _getvolumepathname(path).rstrip(seps)
    else:
        return False


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
#         (though is not verified in the ${varname} and %varname% cases)
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
        environ = getattr(os, 'environb', None)
    else:
        if '$' not in path and '%' not in path:
            return path
        import string
        varchars = string.ascii_letters + string.digits + '_-'
        quote = '\''
        percent = '%'
        brace = '{'
        dollar = '$'
        environ = os.environ
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
                res += c + path
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
                    try:
                        if environ is None:
                            value = os.fsencode(os.environ[os.fsdecode(var)])
                        else:
                            value = environ[var]
                    except KeyError:
                        value = percent + var + percent
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
                except ValueError:
                    if isinstance(path, bytes):
                        res += b'${' + path
                    else:
                        res += '${' + path
                    index = pathlen - 1
                else:
                    var = path[:index]
                    try:
                        if environ is None:
                            value = os.fsencode(os.environ[os.fsdecode(var)])
                        else:
                            value = environ[var]
                    except KeyError:
                        if isinstance(path, bytes):
                            value = b'${' + var + b'}'
                        else:
                            value = '${' + var + '}'
                    res += value
            else:
                var = path[:0]
                index += 1
                c = path[index:index + 1]
                while c and c in varchars:
                    var += c
                    index += 1
                    c = path[index:index + 1]
                try:
                    if environ is None:
                        value = os.fsencode(os.environ[os.fsdecode(var)])
                    else:
                        value = environ[var]
                except KeyError:
                    value = dollar + var
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
            except OSError:
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


# determine if two files are in fact the same file
try:
    # GetFinalPathNameByHandle is available starting with Windows 6.0.
    # Windows XP and non-Windows OS'es will mock _getfinalpathname.
    if sys.getwindowsversion()[:2] >= (6, 0):
        from nt import _getfinalpathname
    else:
        raise ImportError
except (AttributeError, ImportError):
    # On Windows XP and earlier, two files are the same if their absolute
    # pathnames are the same.
    # Non-Windows operating systems fake this method with an XP
    # approximation.
    def _getfinalpathname(f):
        return normcase(abspath(f))


try:
    # The genericpath.isdir implementation uses os.stat and checks the mode
    # attribute to tell whether or not the path is a directory.
    # This is overkill on Windows - just pass the path to GetFileAttributes
    # and check the attribute from there.
    from nt import _isdir as isdir
except ImportError:
    # Use genericpath.isdir as imported above.
    pass
