#! /usr/bin/env python

# Variant of "which".
# On stderr, near and total misses are reported.
# '-l<flags>' argument adds ls -l<flags> of each file found.

import sys
if sys.path[0] in (".", ""): del sys.path[0]

import sys, os
from stat import *

def msg(str):
    sys.stderr.write(str + '\n')

def main():
    pathlist = os.environ['PATH'].split(os.pathsep)

    sts = 0
    longlist = ''

    if sys.argv[1:] and sys.argv[1][:2] == '-l':
        longlist = sys.argv[1]
        del sys.argv[1]

    for prog in sys.argv[1:]:
        ident = ()
        for dir in pathlist:
            filename = os.path.join(dir, prog)
            try:
                st = os.stat(filename)
            except os.error:
                continue
            if not S_ISREG(st[ST_MODE]):
                msg(filename + ': not a disk file')
            else:
                mode = S_IMODE(st[ST_MODE])
                if mode & 0o111:
                    if not ident:
                        print(filename)
                        ident = st[:3]
                    else:
                        if st[:3] == ident:
                            s = 'same as: '
                        else:
                            s = 'also: '
                        msg(s + filename)
                else:
                    msg(filename + ': not executable')
            if longlist:
                sts = os.system('ls ' + longlist + ' ' + filename)
                if sts: msg('"ls -l" exit status: ' + repr(sts))
        if not ident:
            msg(prog + ': not found')
            sts = 1

    sys.exit(sts)

if __name__ == '__main__':
    main()
