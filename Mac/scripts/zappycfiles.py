#!/usr/local/bin/python
"""Recursively zap all .pyc and .pyo files"""
import os
import sys

# set doit true to actually delete files
# set doit false to just print what would be deleted
doit = 1

def main():
    if not sys.argv[1:]:
        if os.name == 'mac':
            import EasyDialogs
            dir = EasyDialogs.AskFolder(message='Directory to zap pyc files in')
            if not dir:
                sys.exit(0)
            zappyc(dir)
        else:
            print('Usage: zappyc dir ...')
            sys.exit(1)
    for dir in sys.argv[1:]:
        zappyc(dir)

def zappyc(dir):
    os.path.walk(dir, walker, None)

def walker(dummy, top, names):
    for name in names:
        if name[-4:] in ('.pyc', '.pyo'):
            path = os.path.join(top, name)
            print('Zapping', path)
            if doit:
                os.unlink(path)

if __name__ == '__main__':
    main()
