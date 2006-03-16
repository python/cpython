#! /usr/bin/env python

# This script is obsolete -- it is kept for historical purposes only.
#
# Fix Python source files to use the new class definition syntax, i.e.,
# the syntax used in Python versions before 0.9.8:
#       class C() = base(), base(), ...: ...
# is changed to the current syntax:
#       class C(base, base, ...): ...
#
# The script uses heuristics to find class definitions that usually
# work but occasionally can fail; carefully check the output!
#
# Command line arguments are files or directories to be processed.
# Directories are searched recursively for files whose name looks
# like a python module.
# Symbolic links are always ignored (except as explicit directory
# arguments).  Of course, the original file is kept as a back-up
# (with a "~" attached to its name).
#
# Changes made are reported to stdout in a diff-like format.
#
# Undoubtedly you can do this using find and sed or perl, but this is
# a nice example of Python code that recurses down a directory tree
# and uses regular expressions.  Also note several subtleties like
# preserving the file's mode and avoiding to even write a temp file
# when no changes are needed for a file.
#
# NB: by changing only the function fixline() you can turn this
# into a program for a different change to Python programs...

import sys
import re
import os
from stat import *

err = sys.stderr.write
dbg = err
rep = sys.stdout.write

def main():
    bad = 0
    if not sys.argv[1:]: # No arguments
        err('usage: ' + sys.argv[0] + ' file-or-directory ...\n')
        sys.exit(2)
    for arg in sys.argv[1:]:
        if os.path.isdir(arg):
            if recursedown(arg): bad = 1
        elif os.path.islink(arg):
            err(arg + ': will not process symbolic links\n')
            bad = 1
        else:
            if fix(arg): bad = 1
    sys.exit(bad)

ispythonprog = re.compile('^[a-zA-Z0-9_]+\.py$')
def ispython(name):
    return ispythonprog.match(name) >= 0

def recursedown(dirname):
    dbg('recursedown(%r)\n' % (dirname,))
    bad = 0
    try:
        names = os.listdir(dirname)
    except os.error, msg:
        err('%s: cannot list directory: %r\n' % (dirname, msg))
        return 1
    names.sort()
    subdirs = []
    for name in names:
        if name in (os.curdir, os.pardir): continue
        fullname = os.path.join(dirname, name)
        if os.path.islink(fullname): pass
        elif os.path.isdir(fullname):
            subdirs.append(fullname)
        elif ispython(name):
            if fix(fullname): bad = 1
    for fullname in subdirs:
        if recursedown(fullname): bad = 1
    return bad

def fix(filename):
##  dbg('fix(%r)\n' % (filename,))
    try:
        f = open(filename, 'r')
    except IOError, msg:
        err('%s: cannot open: %r\n' % (filename, msg))
        return 1
    head, tail = os.path.split(filename)
    tempname = os.path.join(head, '@' + tail)
    g = None
    # If we find a match, we rewind the file and start over but
    # now copy everything to a temp file.
    lineno = 0
    while 1:
        line = f.readline()
        if not line: break
        lineno = lineno + 1
        while line[-2:] == '\\\n':
            nextline = f.readline()
            if not nextline: break
            line = line + nextline
            lineno = lineno + 1
        newline = fixline(line)
        if newline != line:
            if g is None:
                try:
                    g = open(tempname, 'w')
                except IOError, msg:
                    f.close()
                    err('%s: cannot create: %r\n' % (tempname, msg))
                    return 1
                f.seek(0)
                lineno = 0
                rep(filename + ':\n')
                continue # restart from the beginning
            rep(repr(lineno) + '\n')
            rep('< ' + line)
            rep('> ' + newline)
        if g is not None:
            g.write(newline)

    # End of file
    f.close()
    if not g: return 0 # No changes

    # Finishing touch -- move files

    # First copy the file's mode to the temp file
    try:
        statbuf = os.stat(filename)
        os.chmod(tempname, statbuf[ST_MODE] & 07777)
    except os.error, msg:
        err('%s: warning: chmod failed (%r)\n' % (tempname, msg))
    # Then make a backup of the original file as filename~
    try:
        os.rename(filename, filename + '~')
    except os.error, msg:
        err('%s: warning: backup failed (%r)\n' % (filename, msg))
    # Now move the temp file to the original file
    try:
        os.rename(tempname, filename)
    except os.error, msg:
        err('%s: rename failed (%r)\n' % (filename, msg))
        return 1
    # Return succes
    return 0

# This expression doesn't catch *all* class definition headers,
# but it's pretty darn close.
classexpr = '^([ \t]*class +[a-zA-Z0-9_]+) *( *) *((=.*)?):'
classprog = re.compile(classexpr)

# Expressions for finding base class expressions.
baseexpr = '^ *(.*) *( *) *$'
baseprog = re.compile(baseexpr)

def fixline(line):
    if classprog.match(line) < 0: # No 'class' keyword -- no change
        return line

    (a0, b0), (a1, b1), (a2, b2) = classprog.regs[:3]
    # a0, b0 = Whole match (up to ':')
    # a1, b1 = First subexpression (up to classname)
    # a2, b2 = Second subexpression (=.*)
    head = line[:b1]
    tail = line[b0:] # Unmatched rest of line

    if a2 == b2: # No base classes -- easy case
        return head + ':' + tail

    # Get rid of leading '='
    basepart = line[a2+1:b2]

    # Extract list of base expressions
    bases = basepart.split(',')

    # Strip trailing '()' from each base expression
    for i in range(len(bases)):
        if baseprog.match(bases[i]) >= 0:
            x1, y1 = baseprog.regs[1]
            bases[i] = bases[i][x1:y1]

    # Join the bases back again and build the new line
    basepart = ', '.join(bases)

    return head + '(' + basepart + '):' + tail

if __name__ == '__main__':
    main()
