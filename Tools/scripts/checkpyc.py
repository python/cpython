#! /usr/bin/env python
# Check that all ".pyc" files exist and are up-to-date
# Uses module 'os'

import sys
import os
from stat import ST_MTIME
import imp

def main():
    silent = 0
    verbose = 0
    if sys.argv[1:]:
        if sys.argv[1] == '-v':
            verbose = 1
        elif sys.argv[1] == '-s':
            silent = 1
    MAGIC = imp.get_magic()
    if not silent:
        print 'Using MAGIC word', repr(MAGIC)
    for dirname in sys.path:
        try:
            names = os.listdir(dirname)
        except os.error:
            print 'Cannot list directory', repr(dirname)
            continue
        if not silent:
            print 'Checking ', repr(dirname), '...'
        names.sort()
        for name in names:
            if name[-3:] == '.py':
                name = os.path.join(dirname, name)
                try:
                    st = os.stat(name)
                except os.error:
                    print 'Cannot stat', repr(name)
                    continue
                if verbose:
                    print 'Check', repr(name), '...'
                name_c = name + 'c'
                try:
                    f = open(name_c, 'r')
                except IOError:
                    print 'Cannot open', repr(name_c)
                    continue
                magic_str = f.read(4)
                mtime_str = f.read(4)
                f.close()
                if magic_str <> MAGIC:
                    print 'Bad MAGIC word in ".pyc" file',
                    print repr(name_c)
                    continue
                mtime = get_long(mtime_str)
                if mtime == 0 or mtime == -1:
                    print 'Bad ".pyc" file', repr(name_c)
                elif mtime <> st[ST_MTIME]:
                    print 'Out-of-date ".pyc" file',
                    print repr(name_c)

def get_long(s):
    if len(s) <> 4:
        return -1
    return ord(s[0]) + (ord(s[1])<<8) + (ord(s[2])<<16) + (ord(s[3])<<24)

if __name__ == '__main__':
    main()
