#! /usr/bin/env python

"""Reverse grep.

Usage: rgrep [-i] pattern file
"""

import sys
import re
import string
import getopt

def main():
    bufsize = 64*1024
    reflags = 0
    opts, args = getopt.getopt(sys.argv[1:], "i")
    for o, a in opts:
        if o == '-i':
            reflags = reflags | re.IGNORECASE
    if len(args) < 2:
        usage("not enough arguments")
    if len(args) > 2:
        usage("exactly one file argument required")
    pattern, filename = args
    try:
        prog = re.compile(pattern, reflags)
    except re.error, msg:
        usage("error in regular expression: %s" % str(msg))
    try:
        f = open(filename)
    except IOError, msg:
        usage("can't open %s: %s" % (repr(filename), str(msg)), 1)
    f.seek(0, 2)
    pos = f.tell()
    leftover = None
    while pos > 0:
        size = min(pos, bufsize)
        pos = pos - size
        f.seek(pos)
        buffer = f.read(size)
        lines = string.split(buffer, "\n")
        del buffer
        if leftover is None:
            if not lines[-1]:
                del lines[-1]
        else:
            lines[-1] = lines[-1] + leftover
        if pos > 0:
            leftover = lines[0]
            del lines[0]
        else:
            leftover = None
        lines.reverse()
        for line in lines:
            if prog.search(line):
                print line

def usage(msg, code=2):
    sys.stdout = sys.stderr
    print msg
    print __doc__
    sys.exit(code)

if __name__ == '__main__':
    main()
