"""Common operations on DOS pathnames."""

import os
import stat

__all__ = ["normcase","isabs","join","splitdrive","split","splitext",
           "basename","dirname","commonprefix","getsize","getmtime",
           "getatime","islink","exists","isdir","isfile","ismount",
           "walk","expanduser","expandvars","normpath","abspath","realpath"]

def normcase(s):
    """Normalize the case of a pathname.
    On MS-DOS it maps the pathname to lowercase, turns slashes into
    backslashes.
    Other normalizations (such as optimizing '../' away) are not allowed
    (this is done by normpath).
    Previously, this version mapped invalid consecutive characters to a
    single '_', but this has been removed.  This functionality should
    possibly be added as a new function."""

    return s.replace("/", "\\").lower()


def isabs(s):
    """Return whether a path is absolute.
    Trivial in Posix, harder on the Mac or MS-DOS.
    For DOS it is absolute if it starts with a slash or backslash (current
    volume), or if a pathname after the volume letter and colon starts with
    a slash or backslash."""

    s = splitdrive(s)[1]
    return s != '' and s[:1] in '/\\'


def join(a, *p):
    """Join two (or more) paths."""

    path = a
    for b in p:
        if isabs(b):
            path = b
        elif path == '' or path[-1:] in '/\\:':
            path = path + b
        else:
            path = path + "\\" + b
    return path


def splitdrive(p):
    """Split a path into a drive specification (a drive letter followed
    by a colon) and path specification.
    It is always true that drivespec + pathspec == p."""

    if p[1:2] == ':':
        return p[0:2], p[2:]
    return '', p


def split(p):
    """Split a path into head (everything up to the last '/') and tail
    (the rest).  After the trailing '/' is stripped, the invariant
    join(head, tail) == p holds.
    The resulting head won't end in '/' unless it is the root."""

    d, p = splitdrive(p)
    # set i to index beyond p's last slash
    i = len(p)
    while i and p[i-1] not in '/\\':
        i = i - 1
    head, tail = p[:i], p[i:]  # now tail has no slashes
    # remove trailing slashes from head, unless it's all slashes
    head2 = head
    while head2 and head2[-1] in '/\\':
        head2 = head2[:-1]
    head = head2 or head
    return d + head, tail


def splitext(p):
    """Split a path into root and extension.
    The extension is everything starting at the first dot in the last
    pathname component; the root is everything before that.
    It is always true that root + ext == p."""

    root, ext = '', ''
    for c in p:
        if c in '/\\':
            root, ext = root + ext + c, ''
        elif c == '.' or ext:
            ext = ext + c
        else:
            root = root + c
    return root, ext


def basename(p):
    """Return the tail (basename) part of a path."""

    return split(p)[1]


def dirname(p):
    """Return the head (dirname) part of a path."""

    return split(p)[0]


def commonprefix(m):
    """Return the longest prefix of all list elements."""

    if not m: return ''
    prefix = m[0]
    for item in m:
        for i in range(len(prefix)):
            if prefix[:i+1] != item[:i+1]:
                prefix = prefix[:i]
                if i == 0: return ''
                break
    return prefix


# Get size, mtime, atime of files.

def getsize(filename):
    """Return the size of a file, reported by os.stat()."""
    st = os.stat(filename)
    return st[stat.ST_SIZE]

def getmtime(filename):
    """Return the last modification time of a file, reported by os.stat()."""
    st = os.stat(filename)
    return st[stat.ST_MTIME]

def getatime(filename):
    """Return the last access time of a file, reported by os.stat()."""
    st = os.stat(filename)
    return st[stat.ST_ATIME]


def islink(path):
    """Is a path a symbolic link?
    This will always return false on systems where posix.lstat doesn't exist."""

    return 0


def exists(path):
    """Does a path exist?
    This is false for dangling symbolic links."""

    try:
        st = os.stat(path)
    except os.error:
        return 0
    return 1


def isdir(path):
    """Is a path a dos directory?"""

    try:
        st = os.stat(path)
    except os.error:
        return 0
    return stat.S_ISDIR(st[stat.ST_MODE])


def isfile(path):
    """Is a path a regular file?"""

    try:
        st = os.stat(path)
    except os.error:
        return 0
    return stat.S_ISREG(st[stat.ST_MODE])


def ismount(path):
    """Is a path a mount point?"""
    # XXX This degenerates in: 'is this the root?' on DOS

    return isabs(splitdrive(path)[1])


def walk(top, func, arg):
    """Directory tree walk with callback function.

    For each directory in the directory tree rooted at top (including top
    itself, but excluding '.' and '..'), call func(arg, dirname, fnames).
    dirname is the name of the directory, and fnames a list of the names of
    the files and subdirectories in dirname (excluding '.' and '..').  func
    may modify the fnames list in-place (e.g. via del or slice assignment),
    and walk will only recurse into the subdirectories whose names remain in
    fnames; this can be used to implement a filter, or to impose a specific
    order of visiting.  No semantics are defined for, or required of, arg,
    beyond that arg is always passed to func.  It can be used, e.g., to pass
    a filename pattern, or a mutable object designed to accumulate
    statistics.  Passing None for arg is common."""

    try:
        names = os.listdir(top)
    except os.error:
        return
    func(arg, top, names)
    exceptions = ('.', '..')
    for name in names:
        if name not in exceptions:
            name = join(top, name)
            if isdir(name):
                walk(name, func, arg)


def expanduser(path):
    """Expand paths beginning with '~' or '~user'.
    '~' means $HOME; '~user' means that user's home directory.
    If the path doesn't begin with '~', or if the user or $HOME is unknown,
    the path is returned unchanged (leaving error reporting to whatever
    function is called with the expanded path as argument).
    See also module 'glob' for expansion of *, ? and [...] in pathnames.
    (A function should also be defined to do full *sh-style environment
    variable expansion.)"""

    if path[:1] != '~':
        return path
    i, n = 1, len(path)
    while i < n and path[i] not in '/\\':
        i = i+1
    if i == 1:
        if not os.environ.has_key('HOME'):
            return path
        userhome = os.environ['HOME']
    else:
        return path
    return userhome + path[i:]


def expandvars(path):
    """Expand paths containing shell variable substitutions.
    The following rules apply:
        - no expansion within single quotes
        - no escape character, except for '$$' which is translated into '$'
        - ${varname} is accepted.
        - varnames can be made out of letters, digits and the character '_'"""
    # XXX With COMMAND.COM you can use any characters in a variable name,
    # XXX except '^|<>='.

    if '$' not in path:
        return path
    import string
    varchars = string.ascii_letters + string.digits + "_-"
    res = ''
    index = 0
    pathlen = len(path)
    while index < pathlen:
        c = path[index]
        if c == '\'':   # no expansion within single quotes
            path = path[index + 1:]
            pathlen = len(path)
            try:
                index = path.index('\'')
                res = res + '\'' + path[:index + 1]
            except ValueError:
                res = res + path
                index = pathlen -1
        elif c == '$':  # variable or '$$'
            if path[index + 1:index + 2] == '$':
                res = res + c
                index = index + 1
            elif path[index + 1:index + 2] == '{':
                path = path[index+2:]
                pathlen = len(path)
                try:
                    index = path.index('}')
                    var = path[:index]
                    if os.environ.has_key(var):
                        res = res + os.environ[var]
                except ValueError:
                    res = res + path
                    index = pathlen - 1
            else:
                var = ''
                index = index + 1
                c = path[index:index + 1]
                while c != '' and c in varchars:
                    var = var + c
                    index = index + 1
                    c = path[index:index + 1]
                if os.environ.has_key(var):
                    res = res + os.environ[var]
                if c != '':
                    res = res + c
        else:
            res = res + c
        index = index + 1
    return res


def normpath(path):
    """Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A/B.
    Also, components of the path are silently truncated to 8+3 notation."""

    path = path.replace("/", "\\")
    prefix, path = splitdrive(path)
    while path[:1] == "\\":
        prefix = prefix + "\\"
        path = path[1:]
    comps = path.split("\\")
    i = 0
    while i < len(comps):
        if comps[i] == '.':
            del comps[i]
        elif comps[i] == '..' and i > 0 and \
                      comps[i-1] not in ('', '..'):
            del comps[i-1:i+1]
            i = i - 1
        elif comps[i] == '' and i > 0 and comps[i-1] != '':
            del comps[i]
        elif '.' in comps[i]:
            comp = comps[i].split('.')
            comps[i] = comp[0][:8] + '.' + comp[1][:3]
            i = i + 1
        elif len(comps[i]) > 8:
            comps[i] = comps[i][:8]
            i = i + 1
        else:
            i = i + 1
    # If the path is now empty, substitute '.'
    if not prefix and not comps:
        comps.append('.')
    return prefix + "\\".join(comps)



def abspath(path):
    """Return an absolute path."""
    if not isabs(path):
        path = join(os.getcwd(), path)
    return normpath(path)

# realpath is a no-op on systems without islink support
realpath = abspath
