# This module provides standard support for "packages".
#
# The idea is that large groups of related modules can be placed in
# their own subdirectory, which can be added to the Python search path
# in a relatively easy way.
#
# The current version takes a package name and searches the Python
# search path for a directory by that name, and if found adds it to
# the module search path (sys.path).  It maintains a list of packages
# that have already been added so adding the same package many times
# is OK.
#
# It is intended to be used in a fairly stylized manner: each module
# that wants to use a particular package, say 'Foo', is supposed to
# contain the following code:
#
#   from addpack import addpack
#   addpack('Foo')
#   <import modules from package Foo>
#
# Additional arguments, when present, provide additional places where
# to look for the package before trying sys.path (these may be either
# strings or lists/tuples of strings).  Also, if the package name is a
# full pathname, first the last component is tried in the usual way,
# then the full pathname is tried last.  If the package name is a
# *relative* pathname (UNIX: contains a slash but doesn't start with
# one), then nothing special is done.  The packages "/foo/bar/bletch"
# and "bletch" are considered the same, but unrelated to "bar/bletch".
#
# If the algorithm finds more than one suitable subdirectory, all are
# added to the search path -- this makes it possible to override part
# of a package.  The same path will not be added more than once.
#
# If no directory is found, ImportError is raised.

_packs = {}                             # {pack: [pathname, ...], ...}

def addpack(pack, *locations):
    import os
    if os.path.isabs(pack):
        base = os.path.basename(pack)
    else:
        base = pack
    if _packs.has_key(base):
        return
    import sys
    path = []
    for loc in _flatten(locations) + sys.path:
        fn = os.path.join(loc, base)
        if fn not in path and os.path.isdir(fn):
            path.append(fn)
    if pack != base and pack not in path and os.path.isdir(pack):
        path.append(pack)
    if not path: raise ImportError, 'package ' + pack + ' not found'
    _packs[base] = path
    for fn in path:
        if fn not in sys.path:
            sys.path.append(fn)

def _flatten(locations):
    locs = []
    for loc in locations:
        if type(loc) == type(''):
            locs.append(loc)
        else:
            locs = locs + _flatten(loc)
    return locs
