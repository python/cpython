#! /usr/bin/env python

# 3)  System Test
#
#     Given a list of directories, report any bogus symbolic links contained
#     anywhere in those subtrees.  A bogus symbolic link is one that cannot
#     be resolved because it points to a nonexistent or otherwise
#     unresolvable file.  Do *not* use an external find executable.
#     Directories may be very very deep.  Print a warning immediately if the
#     system you're running on doesn't support symbolic links.

# This implementation:
# - takes one optional argument, using the current directory as default
# - uses chdir to increase performance
# - sorts the names per directory
# - prints output lines of the form "path1 -> path2" as it goes
# - prints error messages about directories it can't list or chdir into

import os
import sys
from stat import *

def main():
    try:
        # Note: can't test for presence of lstat -- it's always there
        dummy = os.readlink
    except AttributeError:
        print("This system doesn't have symbolic links")
        sys.exit(0)
    if sys.argv[1:]:
        prefix = sys.argv[1]
    else:
        prefix = ''
    if prefix:
        os.chdir(prefix)
        if prefix[-1:] != '/': prefix = prefix + '/'
        reportboguslinks(prefix)
    else:
        reportboguslinks('')

def reportboguslinks(prefix):
    try:
        names = os.listdir('.')
    except os.error as msg:
        print("%s%s: can't list: %s" % (prefix, '.', msg))
        return
    names.sort()
    for name in names:
        if name == os.curdir or name == os.pardir:
            continue
        try:
            mode = os.lstat(name)[ST_MODE]
        except os.error:
            print("%s%s: can't stat: %s" % (prefix, name, msg))
            continue
        if S_ISLNK(mode):
            try:
                os.stat(name)
            except os.error:
                print("%s%s -> %s" % \
                      (prefix, name, os.readlink(name)))
        elif S_ISDIR(mode):
            try:
                os.chdir(name)
            except os.error as msg:
                print("%s%s: can't chdir: %s" % \
                      (prefix, name, msg))
                continue
            try:
                reportboguslinks(prefix + name + '/')
            finally:
                os.chdir('..')

main()
