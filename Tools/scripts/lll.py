#! /usr/bin/env python

# Find symbolic links and show where they point to.
# Arguments are directories to search; default is current directory.
# No recursion.
# (This is a totally different program from "findsymlinks.py"!)

import sys, os

def lll(dirname):
    for name in os.listdir(dirname):
        if name not in (os.curdir, os.pardir):
            full = os.path.join(dirname, name)
            if os.path.islink(full):
                print(name, '->', os.readlink(full))
def main():
    args = sys.argv[1:]
    if not args: args = [os.curdir]
    first = 1
    for arg in args:
        if len(args) > 1:
            if not first: print()
            first = 0
            print(arg + ':')
    lll(arg)

if __name__ == '__main__':
    main()
