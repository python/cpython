#! /usr/bin/env python3
# Check that all ".pyc" files exist and are up-to-date
# Uses module 'os'

import sys
import os
from stat import ST_MTIME
import imp

# PEP 3147 compatibility (PYC Repository Directories)
cache_from_source = (imp.cache_from_source if hasattr(imp, 'get_tag') else
                     lambda path: path + 'c')


def main():
    if len(sys.argv) > 1:
        verbose = (sys.argv[1] == '-v')
        silent = (sys.argv[1] == '-s')
    else:
        verbose = silent = False
    MAGIC = imp.get_magic()
    if not silent:
        print('Using MAGIC word', repr(MAGIC))
    for dirname in sys.path:
        try:
            names = os.listdir(dirname)
        except os.error:
            print('Cannot list directory', repr(dirname))
            continue
        if not silent:
            print('Checking ', repr(dirname), '...')
        for name in sorted(names):
            if name.endswith('.py'):
                name = os.path.join(dirname, name)
                try:
                    st = os.stat(name)
                except os.error:
                    print('Cannot stat', repr(name))
                    continue
                if verbose:
                    print('Check', repr(name), '...')
                name_c = cache_from_source(name)
                try:
                    with open(name_c, 'rb') as f:
                        magic_str = f.read(4)
                        mtime_str = f.read(4)
                except IOError:
                    print('Cannot open', repr(name_c))
                    continue
                if magic_str != MAGIC:
                    print('Bad MAGIC word in ".pyc" file', end=' ')
                    print(repr(name_c))
                    continue
                mtime = get_long(mtime_str)
                if mtime in {0, -1}:
                    print('Bad ".pyc" file', repr(name_c))
                elif mtime != st[ST_MTIME]:
                    print('Out-of-date ".pyc" file', end=' ')
                    print(repr(name_c))


def get_long(s):
    if len(s) != 4:
        return -1
    return s[0] + (s[1] << 8) + (s[2] << 16) + (s[3] << 24)


if __name__ == '__main__':
    main()
