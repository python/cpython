#! /usr/bin/env python3

# findlinksto
#
# find symbolic links to a path matching a regular expression

import os
import sys
import re
import getopt

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], '')
        if len(args) < 2:
            raise getopt.GetoptError('not enough arguments', None)
    except getopt.GetoptError as msg:
        sys.stdout = sys.stderr
        print(msg)
        print('usage: findlinksto pattern directory ...')
        sys.exit(2)
    pat, dirs = args[0], args[1:]
    prog = re.compile(pat)
    for dirname in dirs:
        os.walk(dirname, visit, prog)

def visit(prog, dirname, names):
    if os.path.islink(dirname):
        names[:] = []
        return
    if os.path.ismount(dirname):
        print('descend into', dirname)
    for name in names:
        name = os.path.join(dirname, name)
        try:
            linkto = os.readlink(name)
            if prog.search(linkto) is not None:
                print(name, '->', linkto)
        except OSError:
            pass

if __name__ == '__main__':
    main()
