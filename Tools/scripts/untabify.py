#! /usr/bin/env python

"Replace tabs with spaces in argument files.  Print names of changed files."

import os
import sys
import string
import getopt

def main():
    tabsize = 8
    try:
        opts, args = getopt.getopt(sys.argv[1:], "t:")
        if not args:
            raise getopt.error, "At least one file argument required"
    except getopt.error, msg:
        print msg
        print "usage:", sys.argv[0], "[-t tabwidth] file ..."
        return
    for optname, optvalue in opts:
        if optname == '-t':
            tabsize = int(optvalue)

    for file in args:
        process(file, tabsize)

def process(file, tabsize):
    try:
        f = open(file)
        text = f.read()
        f.close()
    except IOError, msg:
        print "%s: I/O error: %s" % (`file`, str(msg))
        return
    newtext = string.expandtabs(text, tabsize)
    if newtext == text:
        return
    backup = file + "~"
    try:
        os.unlink(backup)
    except os.error:
        pass
    try:
        os.rename(file, backup)
    except os.error:
        pass
    f = open(file, "w")
    f.write(newtext)
    f.close()
    print file

if __name__ == '__main__':
    main()
