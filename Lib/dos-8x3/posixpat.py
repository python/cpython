"""Common operations on Posix pathnames.

Instead of importing this module directly, import os and refer to
this module as os.path.  The "os.path" name is an alias for this
module on Posix systems; on other systems (e.g. Mac, Windows),
os.path provides the same operations in a manner specific to that
platform, and is an alias to another module (e.g. macpath, ntpath).

Some of this can actually be useful on non-Posix systems too, e.g.
for manipulation of the pathname component of URLs.
"""

import os
import stat


# Normalize the case of a pathname.  Trivial in Posix, string.lower on Mac.
# On MS-DOS this may also turn slashes into backslashes; however, other
# normalizations (such as optimizing '../' away) are not allowed
# (another function should be defined to do that).

def normcase(s):
    """Normalize case of pathname.  Has no effect under Posix"""
    return s


# Return whether a path is absolute.
# Trivial in Posix, harder on the Mac or MS-DOS.

def isabs(s):
    """Test whether a path is absolute"""
    return s[:1] == '/'


# Join pathnames.
# Ignore the previous parts if a part is absolute.
# Insert a '/' unless the first part is empty or already ends in '/'.

def join(a, *p):
    """Join two or more pathname components, inserting '/' as needed"""
    path = a
    for b in p:
        if b[:1] == '/':
            path = b
        elif path == '' or path[-1:] == '/':
            path = path + b
        else:
            path = path + '/' + b
    return path


# Split a path in head (everything up to the last '/') and tail (the
# rest).  If the path ends in '/', tail will be empty.  If there is no
# '/' in the path, head  will be empty.
# Trailing '/'es are stripped from head unless it is the root.

def split(p):
    """Split a pathname.  Returns tuple "(head, tail)" where "tail" is 
    everything after the final slash.  Either part may be empty."""
    i = p.rfind('/') + 1
    head, tail = p[:i], p[i:]
    if head and head <> '/'*len(head):
        while head[-1] == '/':
            head = head[:-1]
    return head, tail


# Split a path in root and extension.
# The extension is everything starting at the last dot in the last
# pathname component; the root is everything before that.
# It is always true that root + ext == p.

def splitext(p):
    """Split the extension from a pathname.  Extension is everything from the
    last dot to the end.  Returns "(root, ext)", either part may be empty."""
    root, ext = '', ''
    for c in p:
        if c == '/':
            root, ext = root + ext + c, ''
        elif c == '.':
            if ext:
                root, ext = root + ext, c
            else:
                ext = c
        elif ext:
            ext = ext + c
        else:
            root = root + c
    return root, ext


# Split a pathname into a drive specification and the rest of the
# path.  Useful on DOS/Windows/NT; on Unix, the drive is always empty.

def splitdrive(p):
    """Split a pathname into drive and path. On Posix, drive is always 
    empty."""
    return '', p


# Return the tail (basename) part of a path.

def basename(p):
    """Returns the final component of a pathname"""
    return split(p)[1]


# Return the head (dirname) part of a path.

def dirname(p):
    """Returns the directory component of a pathname"""
    return split(p)[0]


# Return the longest prefix of all list elements.

def commonprefix(m):
    "Given a list of pathnames, returns the longest common leading component"
    if not m: return ''
    prefix = m[0]
    for item in m:
        for i in range(len(prefix)):
            if prefix[:i+1] <> item[:i+1]:
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


# Is a path a symbolic link?
# This will always return false on systems where os.lstat doesn't exist.

def islink(path):
    """Test whether a path is a symbolic link"""
    try:
        st = os.lstat(path)
    except (os.error, AttributeError):
        return 0
    return stat.S_ISLNK(st[stat.ST_MODE])


# Does a path exist?
# This is false for dangling symbolic links.

def exists(path):
    """Test whether a path exists.  Returns false for broken symbolic links"""
    try:
        st = os.stat(path)
    except os.error:
        return 0
    return 1


# Is a path a directory?
# This follows symbolic links, so both islink() and isdir() can be true
# for the same path.

def isdir(path):
    """Test whether a path is a directory"""
    try:
        st = os.stat(path)
    except os.error:
        return 0
    return stat.S_ISDIR(st[stat.ST_MODE])


# Is a path a regular file?
# This follows symbolic links, so both islink() and isfile() can be true
# for the same path.

def isfile(path):
    """Test whether a path is a regular file"""
    try:
        st = os.stat(path)
    except os.error:
        return 0
    return stat.S_ISREG(st[stat.ST_MODE])


# Are two filenames really pointing to the same file?

def samefile(f1, f2):
    """Test whether two pathnames reference the same actual file"""
    s1 = os.stat(f1)
    s2 = os.stat(f2)
    return samestat(s1, s2)


# Are two open files really referencing the same file?
# (Not necessarily the same file descriptor!)

def sameopenfile(fp1, fp2):
    """Test whether two open file objects reference the same file"""
    s1 = os.fstat(fp1)
    s2 = os.fstat(fp2)
    return samestat(s1, s2)


# Are two stat buffers (obtained from stat, fstat or lstat)
# describing the same file?

def samestat(s1, s2):
    """Test whether two stat buffers reference the same file"""
    return s1[stat.ST_INO] == s2[stat.ST_INO] and \
	   s1[stat.ST_DEV] == s2[stat.ST_DEV]


# Is a path a mount point?
# (Does this work for all UNIXes?  Is it even guaranteed to work by Posix?)

def ismount(path):
    """Test whether a path is a mount point"""
    try:
        s1 = os.stat(path)
        s2 = os.stat(join(path, '..'))
    except os.error:
        return 0 # It doesn't exist -- so not a mount point :-)
    dev1 = s1[stat.ST_DEV]
    dev2 = s2[stat.ST_DEV]
    if dev1 != dev2:
        return 1        # path/.. on a different device as path
    ino1 = s1[stat.ST_INO]
    ino2 = s2[stat.ST_INO]
    if ino1 == ino2:
        return 1        # path/.. is the same i-node as path
    return 0


# Directory tree walk.
# For each directory under top (including top itself, but excluding
# '.' and '..'), func(arg, dirname, filenames) is called, where
# dirname is the name of the directory and filenames is the list
# of files (and subdirectories etc.) in the directory.
# The func may modify the filenames list, to implement a filter,
# or to impose a different order of visiting.

def walk(top, func, arg):
    """walk(top,func,arg) calls func(arg, d, files) for each directory "d" 
    in the tree  rooted at "top" (including "top" itself).  "files" is a list
    of all the files and subdirs in directory "d".
    """
    try:
        names = os.listdir(top)
    except os.error:
        return
    func(arg, top, names)
    for name in names:
            name = join(top, name)
            st = os.lstat(name)
            if stat.S_ISDIR(st[stat.ST_MODE]):
                walk(name, func, arg)


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
    if path[:1] <> '~':
        return path
    i, n = 1, len(path)
    while i < n and path[i] <> '/':
        i = i + 1
    if i == 1:
        if not os.environ.has_key('HOME'):
            return path
        userhome = os.environ['HOME']
    else:
        import pwd
        try:
            pwent = pwd.getpwnam(path[1:i])
        except KeyError:
            return path
        userhome = pwent[5]
    if userhome[-1:] == '/': i = i + 1
    return userhome + path[i:]


# Expand paths containing shell variable substitutions.
# This expands the forms $variable and ${variable} only.
# Non-existent variables are left unchanged.

_varprog = None

def expandvars(path):
    """Expand shell variables of form $var and ${var}.  Unknown variables
    are left unchanged."""
    global _varprog
    if '$' not in path:
        return path
    if not _varprog:
        import re
        _varprog = re.compile(r'\$(\w+|\{[^}]*\})')
    i = 0
    while 1:
        m = _varprog.search(path, i)
        if not m:
            break
        i, j = m.span(0)
        name = m.group(1)
        if name[:1] == '{' and name[-1:] == '}':
            name = name[1:-1]
        if os.environ.has_key(name):
            tail = path[j:]
            path = path[:i] + os.environ[name]
            i = len(path)
            path = path + tail
        else:
            i = j
    return path


# Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A/B.
# It should be understood that this may change the meaning of the path
# if it contains symbolic links!

def normpath(path):
    """Normalize path, eliminating double slashes, etc."""
    if path == '':
        return '.'
    initial_slash = (path[0] == '/')
    comps = path.split('/')
    new_comps = []
    for comp in comps:
        if comp in ('', '.'):
            continue
        if (comp != '..' or (not initial_slash and not new_comps) or 
             (new_comps and new_comps[-1] == '..')):
            new_comps.append(comp)
        elif new_comps:
            new_comps.pop()
    comps = new_comps
    path = '/'.join(comps)
    if initial_slash:
        path = '/' + path
    return path or '.'


def abspath(path):
    """Return an absolute path."""
    if not isabs(path):
        path = join(os.getcwd(), path)
    return normpath(path)
