#! /usr/bin/env python3

# suff
#
# show different suffixes amongst arguments

import sys


def main():
    files = sys.argv[1:]
    suffixes = {}
    for filename in files:
        suff = getsuffix(filename)
        suffixes.setdefault(suff, []).append(filename)
    for suff, filenames in sorted(suffixes.items()):
        print(repr(suff), len(filenames))


def getsuffix(filename):
    name, sep, suff = filename.rpartition('.')
    return sep + suff if sep else ''


if __name__ == '__main__':
    main()
