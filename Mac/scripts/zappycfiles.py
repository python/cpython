#!/usr/bin/env python
"""Recursively zap all .pyc and .pyo files"""
import os
import sys

# set doit true to actually delete files
# set doit false to just print what would be deleted
doit = 1

def main():
    if not sys.argv[1:]:
        print 'Usage: zappyc dir ...'
        sys.exit(1)
    for dir in sys.argv[1:]:
        zappyc(dir)

def zappyc(dir):
    os.path.walk(dir, walker, None)

def walker(dummy, top, names):
    for name in names:
        if name[-4:] in ('.pyc', '.pyo'):
            path = os.path.join(top, name)
            print 'Zapping', path
            if doit:
                os.unlink(path)

if __name__ == '__main__':
    main()
