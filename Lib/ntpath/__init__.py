# Module 'ntpath' -- common operations on WinNT/Win95 pathnames
"""Common pathname manipulations, WindowsNT/95 version.

Instead of importing this module directly, import os and refer to this
module as os.path.
"""

# strings representing various path-related bits and pieces
# These are primarily for export; internally, they are hardcoded.
# Should be set before imports for resolving cyclic dependency.
curdir = '.'
pardir = '..'
extsep = '.'
sep = '\\'
pathsep = ';'
altsep = '/'
defpath = '.;C:\\bin'
devnull = 'nul'

import os
from genericpath import *
import ntpath.pure
from ntpath.pure import *
from ntpath.pure import _get_bothseps

__all__ = ["abspath", "altsep", "basename", "commonpath", "commonprefix",
           "curdir", "defpath", "devnull", "dirname", "exists", "expanduser",
           "expandvars", "extsep", "getatime", "getctime", "getmtime",
           "getsize", "isabs", "isdevdrive", "isdir", "isfile", "isjunction",
           "islink", "ismount", "isreserved", "join", "lexists", "normcase",
           "normpath", "pardir", "pathsep", "realpath", "relpath", "samefile",
           "sameopenfile", "samestat", "sep", "split", "splitdrive",
           "splitext", "splitroot", "supports_unicode_filenames"]


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
    path = os.fspath(path)
    seps = _get_bothseps(path)
    path = abspath(path)
    drive, root, rest = splitroot(path)
    if drive and drive[0] in seps:
        return not rest
    if root and not rest:
        return True

    if _getvolumepathname:
        x = path.rstrip(seps)
        y =_getvolumepathname(path).rstrip(seps)
        return x.casefold() == y.casefold()
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
    path = os.fspath(path)
    if isinstance(path, bytes):
        seps = b'\\/'
        tilde = b'~'
    else:
        seps = '\\/'
        tilde = '~'
    if not path.startswith(tilde):
        return path
    i, n = 1, len(path)
    while i < n and path[i] not in seps:
        i += 1

    if 'USERPROFILE' in os.environ:
        userhome = os.environ['USERPROFILE']
    elif 'HOMEPATH' not in os.environ:
        return path
    else:
        drive = os.environ.get('HOMEDRIVE', '')
        userhome = join(drive, os.environ['HOMEPATH'])

    if i != 1: #~user
        target_user = path[1:i]
        if isinstance(target_user, bytes):
            target_user = os.fsdecode(target_user)
        current_user = os.environ.get('USERNAME')

        if target_user != current_user:
            # Try to guess user home directory.  By default all user
            # profile directories are located in the same place and are
            # named by corresponding usernames.  If userhome isn't a
            # normal profile directory, this guess is likely wrong,
            # so we bail out.
            if current_user != basename(userhome):
                return path
            userhome = join(dirname(userhome), target_user)

    if isinstance(path, bytes):
        userhome = os.fsencode(userhome)

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
    path = os.fspath(path)
    if isinstance(path, bytes):
        if b'$' not in path and b'%' not in path:
            return path
        import string
        varchars = bytes(string.ascii_letters + string.digits + '_-', 'ascii')
        quote = b'\''
        percent = b'%'
        brace = b'{'
        rbrace = b'}'
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
        rbrace = '}'
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
                    index = path.index(rbrace)
                except ValueError:
                    res += dollar + brace + path
                    index = pathlen - 1
                else:
                    var = path[:index]
                    try:
                        if environ is None:
                            value = os.fsencode(os.environ[os.fsdecode(var)])
                        else:
                            value = environ[var]
                    except KeyError:
                        value = dollar + brace + var + rbrace
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


def _abspath_fallback(path):
    """Return the absolute version of a path as a fallback function in case
    `nt._getfullpathname` is not available or raises OSError. See bpo-31047 for
    more.

    """

    path = os.fspath(path)
    if not isabs(path):
        if isinstance(path, bytes):
            cwd = os.getcwdb()
        else:
            cwd = os.getcwd()
        path = join(cwd, path)
    return normpath(path)

# Return an absolute path.
try:
    from nt import _getfullpathname

except ImportError: # not running on Windows - mock up something sensible
    abspath = _abspath_fallback

else:  # use native Windows method on Windows
    def abspath(path):
        """Return the absolute version of a path."""
        try:
            return _getfullpathname(normpath(path))
        except (OSError, ValueError):
            return _abspath_fallback(path)

try:
    from nt import _findfirstfile, _getfinalpathname, readlink as _nt_readlink
except ImportError:
    # realpath is a no-op on systems without _getfinalpathname support.
    realpath = abspath
else:
    def _readlink_deep(path):
        # These error codes indicate that we should stop reading links and
        # return the path we currently have.
        # 1: ERROR_INVALID_FUNCTION
        # 2: ERROR_FILE_NOT_FOUND
        # 3: ERROR_DIRECTORY_NOT_FOUND
        # 5: ERROR_ACCESS_DENIED
        # 21: ERROR_NOT_READY (implies drive with no media)
        # 32: ERROR_SHARING_VIOLATION (probably an NTFS paging file)
        # 50: ERROR_NOT_SUPPORTED (implies no support for reparse points)
        # 67: ERROR_BAD_NET_NAME (implies remote server unavailable)
        # 87: ERROR_INVALID_PARAMETER
        # 4390: ERROR_NOT_A_REPARSE_POINT
        # 4392: ERROR_INVALID_REPARSE_DATA
        # 4393: ERROR_REPARSE_TAG_INVALID
        allowed_winerror = 1, 2, 3, 5, 21, 32, 50, 67, 87, 4390, 4392, 4393

        seen = set()
        while normcase(path) not in seen:
            seen.add(normcase(path))
            try:
                old_path = path
                path = _nt_readlink(path)
                # Links may be relative, so resolve them against their
                # own location
                if not isabs(path):
                    # If it's something other than a symlink, we don't know
                    # what it's actually going to be resolved against, so
                    # just return the old path.
                    if not islink(old_path):
                        path = old_path
                        break
                    path = normpath(join(dirname(old_path), path))
            except OSError as ex:
                if ex.winerror in allowed_winerror:
                    break
                raise
            except ValueError:
                # Stop on reparse points that are not symlinks
                break
        return path

    def _getfinalpathname_nonstrict(path):
        # These error codes indicate that we should stop resolving the path
        # and return the value we currently have.
        # 1: ERROR_INVALID_FUNCTION
        # 2: ERROR_FILE_NOT_FOUND
        # 3: ERROR_DIRECTORY_NOT_FOUND
        # 5: ERROR_ACCESS_DENIED
        # 21: ERROR_NOT_READY (implies drive with no media)
        # 32: ERROR_SHARING_VIOLATION (probably an NTFS paging file)
        # 50: ERROR_NOT_SUPPORTED
        # 53: ERROR_BAD_NETPATH
        # 65: ERROR_NETWORK_ACCESS_DENIED
        # 67: ERROR_BAD_NET_NAME (implies remote server unavailable)
        # 87: ERROR_INVALID_PARAMETER
        # 123: ERROR_INVALID_NAME
        # 161: ERROR_BAD_PATHNAME
        # 1920: ERROR_CANT_ACCESS_FILE
        # 1921: ERROR_CANT_RESOLVE_FILENAME (implies unfollowable symlink)
        allowed_winerror = 1, 2, 3, 5, 21, 32, 50, 53, 65, 67, 87, 123, 161, 1920, 1921

        # Non-strict algorithm is to find as much of the target directory
        # as we can and join the rest.
        tail = path[:0]
        while path:
            try:
                path = _getfinalpathname(path)
                return join(path, tail) if tail else path
            except OSError as ex:
                if ex.winerror not in allowed_winerror:
                    raise
                try:
                    # The OS could not resolve this path fully, so we attempt
                    # to follow the link ourselves. If we succeed, join the tail
                    # and return.
                    new_path = _readlink_deep(path)
                    if new_path != path:
                        return join(new_path, tail) if tail else new_path
                except OSError:
                    # If we fail to readlink(), let's keep traversing
                    pass
                # If we get these errors, try to get the real name of the file without accessing it.
                if ex.winerror in (1, 5, 32, 50, 87, 1920, 1921):
                    try:
                        name = _findfirstfile(path)
                        path, _ = split(path)
                    except OSError:
                        path, name = split(path)
                else:
                    path, name = split(path)
                if path and not name:
                    return path + tail
                tail = join(name, tail) if tail else name
        return tail

    def realpath(path, *, strict=False):
        path = normpath(path)
        if isinstance(path, bytes):
            prefix = b'\\\\?\\'
            unc_prefix = b'\\\\?\\UNC\\'
            new_unc_prefix = b'\\\\'
            cwd = os.getcwdb()
            # bpo-38081: Special case for realpath(b'nul')
            devnull = b'nul'
            if normcase(path) == devnull:
                return b'\\\\.\\NUL'
        else:
            prefix = '\\\\?\\'
            unc_prefix = '\\\\?\\UNC\\'
            new_unc_prefix = '\\\\'
            cwd = os.getcwd()
            # bpo-38081: Special case for realpath('nul')
            devnull = 'nul'
            if normcase(path) == devnull:
                return '\\\\.\\NUL'
        had_prefix = path.startswith(prefix)
        if not had_prefix and not isabs(path):
            path = join(cwd, path)
        try:
            path = _getfinalpathname(path)
            initial_winerror = 0
        except ValueError as ex:
            # gh-106242: Raised for embedded null characters
            # In strict mode, we convert into an OSError.
            # Non-strict mode returns the path as-is, since we've already
            # made it absolute.
            if strict:
                raise OSError(str(ex)) from None
            path = normpath(path)
        except OSError as ex:
            if strict:
                raise
            initial_winerror = ex.winerror
            path = _getfinalpathname_nonstrict(path)
        # The path returned by _getfinalpathname will always start with \\?\ -
        # strip off that prefix unless it was already provided on the original
        # path.
        if not had_prefix and path.startswith(prefix):
            # For UNC paths, the prefix will actually be \\?\UNC\
            # Handle that case as well.
            if path.startswith(unc_prefix):
                spath = new_unc_prefix + path[len(unc_prefix):]
            else:
                spath = path[len(prefix):]
            # Ensure that the non-prefixed path resolves to the same path
            try:
                if _getfinalpathname(spath) == path:
                    path = spath
            except ValueError as ex:
                # Unexpected, as an invalid path should not have gained a prefix
                # at any point, but we ignore this error just in case.
                pass
            except OSError as ex:
                # If the path does not exist and originally did not exist, then
                # strip the prefix anyway.
                if ex.winerror == initial_winerror:
                    path = spath
        return path


# All supported version have Unicode filename support.
supports_unicode_filenames = True

def relpath(path, start=None):
    """Return a relative version of a path"""
    path = os.fspath(path)
    if not path:
        raise ValueError("no path specified")

    if start is None:
        start = b"." if isinstance(path, bytes) else "."

    return ntpath.pure.relpath(abspath(path), abspath(start))


try:
    # The isdir(), isfile(), islink(), exists() and lexists() implementations
    # in genericpath use os.stat(). This is overkill on Windows. Use simpler
    # builtin functions if they are available.
    from nt import _path_isdir as isdir
    from nt import _path_isfile as isfile
    from nt import _path_islink as islink
    from nt import _path_isjunction as isjunction
    from nt import _path_exists as exists
    from nt import _path_lexists as lexists
except ImportError:
    # Use genericpath.* as imported above
    pass


try:
    from nt import _path_isdevdrive
    def isdevdrive(path):
        """Determines whether the specified path is on a Windows Dev Drive."""
        try:
            return _path_isdevdrive(abspath(path))
        except OSError:
            return False
except ImportError:
    # Use genericpath.isdevdrive as imported above
    pass
