#! /usr/bin/env python

OLD_NOTICE = """/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/
"""

NEW_NOTICE = ""

# " <-- Help Emacs

import os, sys, string

def main():
    args = sys.argv[1:]
    if not args:
        print "No arguments."
    for arg in args:
        process(arg)

def process(arg):
    f = open(arg)
    data = f.read()
    f.close()
    i = string.find(data, OLD_NOTICE)
    if i < 0:
##         print "No old notice in", arg
        return
    data = data[:i] + NEW_NOTICE + data[i+len(OLD_NOTICE):]
    new = arg + ".new"
    backup = arg + ".bak"
    print "Replacing notice in", arg, "...",
    sys.stdout.flush()
    f = open(new, "w")
    f.write(data)
    f.close()
    os.rename(arg, backup)
    os.rename(new, arg)
    print "done"

if __name__ == '__main__':
    main()
