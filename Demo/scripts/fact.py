#! /usr/bin/env python

# Factorize numbers.
# The algorithm is not efficient, but easy to understand.
# If there are large factors, it will take forever to find them,
# because we try all odd numbers between 3 and sqrt(n)...

import sys
from math import sqrt

def fact(n):
    if n < 1:
        raise ValueError('fact() argument should be >= 1')
    if n == 1:
        return []  # special case
    res = []
    # Treat even factors special, so we can use i += 2 later
    while n % 2 == 0:
        res.append(2)
        n //= 2
    # Try odd numbers up to sqrt(n)
    limit = sqrt(n+1)
    i = 3
    while i <= limit:
        if n % i == 0:
            res.append(i)
            n //= i
            limit = sqrt(n+1)
        else:
            i += 2
    if n != 1:
        res.append(n)
    return res

def main():
    if len(sys.argv) > 1:
        source = sys.argv[1:]
    else:
        source = iter(raw_input, '')
    for arg in source:
        try:
            n = int(arg)
        except ValueError:
            print arg, 'is not an integer'
        else:
            print n, fact(n)

if __name__ == "__main__":
    main()
