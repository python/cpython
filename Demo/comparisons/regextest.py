#! /usr/bin/env python

# 1)  Regular Expressions Test
#
#     Read a file of (extended per egrep) regular expressions (one per line),
#     and apply those to all files whose names are listed on the command line.
#     Basically, an 'egrep -f' simulator.  Test it with 20 "vt100" patterns
#     against a five /etc/termcap files.  Tests using more elaborate patters
#     would also be interesting.  Your code should not break if given hundreds
#     of regular expressions or binary files to scan.

# This implementation:
# - combines all patterns into a single one using ( ... | ... | ... )
# - reads patterns from stdin, scans files given as command line arguments
# - produces output in the format <file>:<lineno>:<line>
# - is only about 2.5 times as slow as egrep (though I couldn't run
#   Tom's test -- this system, a vanilla SGI, only has /etc/terminfo)

import string
import sys
import re

def main():
    pats = map(chomp, sys.stdin.readlines())
    bigpat = '(' + '|'.join(pats) + ')'
    prog = re.compile(bigpat)

    for file in sys.argv[1:]:
        try:
            fp = open(file, 'r')
        except IOError, msg:
            print "%s: %s" % (file, msg)
            continue
        lineno = 0
        while 1:
            line = fp.readline()
            if not line:
                break
            lineno = lineno + 1
            if prog.search(line):
                print "%s:%s:%s" % (file, lineno, line),

def chomp(s):
    return s.rstrip('\n')

if __name__ == '__main__':
    main()
