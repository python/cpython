#!/usr/bin/env python

"""
For each argument on the command line, look for it in the set of all Unicode
names.  Arguments are treated as case-insensitive regular expressions, e.g.:

    % find-uname 'small letter a$' 'horizontal line'
    *** small letter a$ matches ***
    LATIN SMALL LETTER A (97)
    COMBINING LATIN SMALL LETTER A (867)
    CYRILLIC SMALL LETTER A (1072)
    PARENTHESIZED LATIN SMALL LETTER A (9372)
    CIRCLED LATIN SMALL LETTER A (9424)
    FULLWIDTH LATIN SMALL LETTER A (65345)
    *** horizontal line matches ***
    HORIZONTAL LINE EXTENSION (9135)
"""

import unicodedata
import sys
import re

def main(args):
    unicode_names= []
    for ix in range(sys.maxunicode+1):
        try:
            unicode_names.append( (ix, unicodedata.name(unichr(ix))) )
        except ValueError: # no name for the character
            pass
    for arg in args:
        pat = re.compile(arg, re.I)
        matches = [(x,y) for (x,y) in unicode_names
                       if pat.search(y) is not None]
        if matches:
            print "***", arg, "matches", "***"
            for (x,y) in matches:
                print "%s (%d)" % (y,x)

if __name__ == "__main__":
    main(sys.argv[1:])
