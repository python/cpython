#! /usr/bin/env python

# suff
#
# show different suffixes amongst arguments

import sys

def main():
    files = sys.argv[1:]
    suffixes = {}
    for filename in files:
        suff = getsuffix(filename)
        if suff not in suffixes:
            suffixes[suff] = []
        suffixes[suff].append(filename)
    keys = sorted(suffixes.keys())
    for suff in keys:
        print(repr(suff), len(suffixes[suff]))

def getsuffix(filename):
    suff = ''
    for i in range(len(filename)):
        if filename[i] == '.':
            suff = filename[i:]
    return suff

if __name__ == '__main__':
    main()
