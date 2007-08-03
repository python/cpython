#! /usr/bin/env python

"""Print a list of files that are mentioned in CVS directories.

Usage: cvsfiles.py [-n file] [directory] ...

If the '-n file' option is given, only files under CVS that are newer
than the given file are printed; by default, all files under CVS are
printed.  As a special case, if a file does not exist, it is always
printed.
"""

import os
import sys
import stat
import getopt

cutofftime = 0

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "n:")
    except getopt.error as msg:
        print(msg)
        print(__doc__, end=' ')
        return 1
    global cutofftime
    newerfile = None
    for o, a in opts:
        if o == '-n':
            cutofftime = getmtime(a)
    if args:
        for arg in args:
            process(arg)
    else:
        process(".")

def process(dir):
    cvsdir = 0
    subdirs = []
    names = os.listdir(dir)
    for name in names:
        fullname = os.path.join(dir, name)
        if name == "CVS":
            cvsdir = fullname
        else:
            if os.path.isdir(fullname):
                if not os.path.islink(fullname):
                    subdirs.append(fullname)
    if cvsdir:
        entries = os.path.join(cvsdir, "Entries")
        for e in open(entries).readlines():
            words = e.split('/')
            if words[0] == '' and words[1:]:
                name = words[1]
                fullname = os.path.join(dir, name)
                if cutofftime and getmtime(fullname) <= cutofftime:
                    pass
                else:
                    print(fullname)
    for sub in subdirs:
        process(sub)

def getmtime(filename):
    try:
        st = os.stat(filename)
    except os.error:
        return 0
    return st[stat.ST_MTIME]

if __name__ == '__main__':
    main()
