#!/usr/bin/env python

# No bug report AFAIK, mail on python-dev on 2006-01-10
import sys

sys.setrecursionlimit(1 << 30)
f = lambda f:f(f)

if __name__ == '__main__':
    f(f)
