#! /usr/bin/env python3

"Replace tabs with spaces in argument files.  Print names of changed files."

import os
import sys
import getopt
import tokenize

def main():
    tabsize = 8
    try:
        opts, args = getopt.getopt(sys.argv[1:], "t:")
        if not args:
            raise getopt.error("At least one file argument required")
    except getopt.error as msg:
        print(msg)
        print("usage:", sys.argv[0], "[-t tabwidth] file ...")
        return
    for optname, optvalue in opts:
        if optname == '-t':
            tabsize = int(optvalue)

    return max(process(filename, tabsize) for filename in args)


def process(filename, tabsize, verbose=True):
    try:
        with tokenize.open(filename) as f:
            text = f.read()
            encoding = f.encoding
    except IOError as msg:
        print("%r: I/O error: %s" % (filename, msg))
        return 2
    newtext = text.expandtabs(tabsize)
    if newtext == text:
        return 0
    backup = filename + "~"
    try:
        os.unlink(backup)
    except OSError:
        pass
    try:
        os.rename(filename, backup)
    except OSError:
        pass
    with open(filename, "w", encoding=encoding) as f:
        f.write(newtext)
    if verbose:
        print(filename)
    return 1


if __name__ == '__main__':
    raise SystemExit(main())
