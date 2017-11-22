"""Lib/ctypes support for LoadLibrary interface to dlopen() for AIX
Similar kind of support (i.e., as a separate file)
as has been done for Darwin support ctypes.macholib.*
rather than as separate, detailed if: sections in utils.py

dlopen() is an interface to AIX initAndLoad() - primary documentation at:
https://www.ibm.com/support/knowledgecenter/en/ssw_aix_61/com.ibm.aix.basetrf1/dlopen.htm
https://www.ibm.com/support/knowledgecenter/en/ssw_aix_61/com.ibm.aix.basetrf1/load.htm
"""
__author__ = "Michael Felt <aixtools@felt.demon.nl>"
__version__ = "1.0.0"

# Latest Update (comments): 13 October 2016
# Thanks to Martin Panter for his patience and comments

from re import search, match, escape
from os import environ, path
from sys import executable
from ctypes import c_void_p, sizeof
from ctypes._util import _last_version
from subprocess import Popen, PIPE

def aixABI():
    """Internal support function:
    return executable size - 32-bit, or 64-bit
    This is vital to the search for suitable member in an archive
    """
    return sizeof(c_void_p) * 8

def get_dumpH(file):
    """Internal support function:
    dump -H output provides info on archive/executable contents
    and related paths. This function call dump -H as a subprocess
    and returns a list of (object, objectinfo) tuples.
    """
    #dumpH parsing:
    # 1. When line starts with /, ./, or ../ - set as object
    # 2. If "INDEX" in line object is confirmed as object
    # 3. get info (lines starting with [0-9])

    def get_object(p):
        object = None
        for line in p.stdout:
            if line.startswith(('/', './', '../')):
                object = line
            elif "INDEX" in line:
                return object.rstrip('\n')
        return None

    def get_info(p):
        # as an object was found, return known paths, archives and members
        # these lines start with a digit
        info = []
        for line in p.stdout:
            if match("[0-9]", line):
                info.append(line)
            else:
                # Should be a blank separator line, safe to consume
                break
        return info

    p = Popen(["/usr/bin/dump", "-X%s" % aixABI(), "-H", file],
        universal_newlines=True, stdout=PIPE)

    objects = []
    while True:
        object = get_object(p)
        if object is None:
            break
        objects.append((object, get_info(p)))

    p.stdout.close()
    p.wait()
    return objects

def get_shared(input):
    """Internal support function: examine the get_dumpH() output and
    return a list of all shareable objects indicated in the output the
    character "[" is used to strip off the path information.
    Note: the "[" and "]" characters that are part of dump -H output
    are not removed here.
    """
    list = []
    for (line, _) in input:
        # potential member lines contain "["
        # otherwise, no processing needed
        if "[" in line:
            # Strip off trailing colon (:)
            list.append(line[line.index("["):-1])
    return list

def get_exactMatch(expr, lines):
    """Internal support function: Must be only one match, otherwise
    result is None. When there is a match, strip the leading "["
    and trailing "]"
    """
    # member names in the dumpH output are between square brackets
    expr = r'\[(%s)\]' % expr
    matches = list(filter(None, (search(expr, line) for line in lines)))
    if len(matches) == 1:
        return matches[0].group(1)
    else:
        return None

# additional processing to deal with AIX legacy names for 64-bit members
def get_legacy(members):
    """Internal support function: This routine resolves historical aka
    legacy naming schemes started in AIX4 shared library support for
    library members names for, e.g., shr.o in /usr/lib/libc.a for
    32-bit binary and shr_64.o in /usr/lib/libc.a for 64-bit binary.
    """
    if aixABI() == 64:
        # AIX 64-bit member is one of shr64.o, shr_64.o, or shr4_64.o
        expr = r'shr4?_?64\.o'
        member = get_exactMatch(expr, members)
        if member:
            return member
    # 32-bit legacy names - both shr.o and shr4.o exist.
    # shr.o is the preffered name so we look for shr.o first
    #  i.e., shr4.o is returned only when shr.o does not exist
    else:
        for name in ['shr.o', 'shr4.o']:
            member = get_exactMatch(escape(name), members)
            if member:
                return member
    return None

def get_version(name, members):
    """Internal support function: examine member list and return highest
    numbered version - if it exists. This function is called when an
    unversioned libFOO.a(libFOO.so) has not been found.

    Versioning for the member name is expected to follow
    GNU LIBTOOL conventions: the highest version (x, then X.y, then X.Y.z)
     * find [libFoo.so.X]
     * find [libFoo.so.X.Y]
     * find [libFoo.so.X.Y.Z]

    Before the GNU convention became the standard scheme regardless of
    binary size AIX packagers used GNU convention "as-is" for 32-bit
    archive members but used an "distinguishing" name for 64-bit members.
    This scheme inserted either 64 or _64 between libFOO and .so
    - generally libFOO_64.so, but occasionally libFOO64.so
    """
    # the expression ending for versions must start as
    # '.so.[0-9]', i.e., *.so.[at least one digit]
    # while multiple, more specific expressions could be specified
    # to search for .so.X, .so.X.Y and .so.X.Y.Z
    # after the first required 'dot' digit
    # any combination of additional 'dot' digits pairs are accepted
    # anything more than libFOO.so.digits.digits.digits
    # should be seen as a member name outside normal expectations
    exprs = [r'lib%s\.so\.[0-9]+[0-9.]*' % name,
        r'lib%s_?64\.so\.[0-9]+[0-9.]*' % name]
    for expr in exprs:
        versions = []
        for line in members:
            m = search(expr, line)
            if m:
                versions.append(m.group(0))
        if versions:
            return _last_version(versions, '.')
    return None

def get_member(name, members):
    """Internal support function: Return an archive member matching name.
    Given a list of members find and return the most appropriate result
    Priority is given to generic libXXX.so, then a versioned libXXX.so.a.b.c
    and finally, legacy AIX naming scheme.
    """
    # look first for a generic match - prepend lib and append .so
    expr = r'lib%s\.so' % name
    member = get_exactMatch(expr, members)
    if member:
        return member
    # since an exact match with .so as extension of member name
    # was not found, look for a versioned name
    # If a versioned name is not found, look for AIX legacy member name
    member = get_version(name, members)
    if member:
        return member
    else:
        return get_legacy(members)

def getExecLibPath_aix():
    """Internal support function:
    On AIX, the buildtime searchpath is stored in the executable.
    The command dump -H can extract this info.
    Prefix searched libraries with LIBPATH, or LD_LIBRARY_PATH if defined
    Additional paths are appended based on paths to libraries the python
    executable is linked with.  This mimics AIX dlopen() behavior.
    """
    libpaths = environ.get("LIBPATH")
    if libpaths is None:
        libpaths = environ.get("LD_LIBRARY_PATH")
    if libpaths is None:
        libpaths = []
    else:
        libpaths = libpaths.split(":")
    objects = get_dumpH(executable)
    for (_, lines) in objects:
        for line in lines:
            # the second (optional) argument is PATH if it includes a /
            path = line.split()[1]
            if "/" in path:
                libpaths.extend(path.split(":"))
    return libpaths

def find_library(name):
    """AIX specific routine - to find an archive member that will dlopen()

    find_library() looks first for an archive (.a) with a suitable member.
    If no archive is found, look for a .so file.
    """

    def find_shared(paths, name):
        """
        paths is a list of directories to search for an archive.
        name is the abbreviated name given to find_library().
        Process: search "paths" for archive, and if an archive is found
        return the result of get_member().
        If an archive is not found then return None
        """
        for dir in paths:
            # /lib is a symbolic link to /usr/lib, skip it
            if dir == "/lib":
                continue
            # "lib" is prefixed to emulate compiler name resolution,
            # e.g., -lc to libc
            base = 'lib%s.a' % name
            archive = path.join(dir, base)
            if path.exists(archive):
                members = get_shared(get_dumpH(archive))
                member = get_member(escape(name), members)
                if member != None:
                    return (base, member)
                else:
                    return (None, None)
        return (None, None)

    libpaths = getExecLibPath_aix()
    (base, member) = find_shared(libpaths, name)
    if base != None:
        return "%s(%s)" % (base, member)

    # To get here, a member in an archive has not been found
    # In other words, either:
    # a) a .a file was not found
    # b) a .a file did not have a suitable member
    # So, look for a .so file
    # Check libpaths for .so file
    # Note, the installation must prepare a link from a .so
    # to a versioned file
    # This is common practice by GNU libtool on other platforms
    soname = "lib%s.so" % name
    for dir in libpaths:
        # /lib is a symbolic link to /usr/lib, skip it
        if dir == "/lib":
            continue
        shlib = path.join(dir, soname)
        if path.exists(shlib):
            return soname
    # if we are here, we have not found anything plausible
    return None
