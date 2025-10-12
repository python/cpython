#!/usr/bin/env python3

# No bug report AFAIK, mail on python-dev on 2006-01-10

# This is a "won't fix" case.  It is known that setting a high enough
# recursion limit crashes by overflowing the stack.  Unless this is
# redesigned somehow, it won't go away.

import sys

sys.setrecursionlimit(1 << 30)
f = lambda f:f(f)

if __name__ == '__main__':
    f(f)
