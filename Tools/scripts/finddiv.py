#! /usr/bin/env python

"""finddiv - a grep-like tool that looks for division operators.

Usage: finddiv [-l] file_or_directory ...

For directory arguments, all files in the directory whose name ends in
.py are processed, and subdirectories are processed recursively.

This actually tokenizes the files to avoid false hits in comments or
strings literals.

By default, this prints all lines containing a / or /= operator, in
grep -n style.  With the -l option specified, it prints the filename
of files that contain at least one / or /= operator.
"""

import os
import sys
import getopt
import tokenize

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "lh")
    except getopt.error, msg:
        usage(msg)
        return 2
    if not args:
        usage("at least one file argument is required")
        return 2
    listnames = 0
    for o, a in opts:
        if o == "-h":
            print __doc__
            return
        if o == "-l":
            listnames = 1
    exit = None
    for filename in args:
        x = process(filename, listnames)
        exit = exit or x
    return exit

def usage(msg):
    sys.stderr.write("%s: %s\n" % (sys.argv[0], msg))
    sys.stderr.write("Usage: %s [-l] file ...\n" % sys.argv[0])
    sys.stderr.write("Try `%s -h' for more information.\n" % sys.argv[0])

def process(filename, listnames):
    if os.path.isdir(filename):
        return processdir(filename, listnames)
    try:
        fp = open(filename)
    except IOError, msg:
        sys.stderr.write("Can't open: %s\n" % msg)
        return 1
    g = tokenize.generate_tokens(fp.readline)
    lastrow = None
    for type, token, (row, col), end, line in g:
        if token in ("/", "/="):
            if listnames:
                print filename
                break
            if row != lastrow:
                lastrow = row
                print "%s:%d:%s" % (filename, row, line),
    fp.close()

def processdir(dir, listnames):
    try:
        names = os.listdir(dir)
    except os.error, msg:
        sys.stderr.write("Can't list directory: %s\n" % dir)
        return 1
    files = []
    for name in names:
        fn = os.path.join(dir, name)
        if os.path.normcase(fn).endswith(".py") or os.path.isdir(fn):
            files.append(fn)
    files.sort(lambda a, b: cmp(os.path.normcase(a), os.path.normcase(b)))
    exit = None
    for fn in files:
        x = process(fn, listnames)
        exit = exit or x
    return exit

if __name__ == "__main__":
    sys.exit(main())
