#!/usr/bin/env python

#
# Copyright (C) 2001-2012 Python Software Foundation. All Rights Reserved.
# Modified and extended by Stefan Krah.
#

# Usage: ../../../python bench.py


import time
from math import log, ceil
from test.support import import_fresh_module

C = import_fresh_module('decimal', fresh=['_decimal'])
P = import_fresh_module('decimal', blocked=['_decimal'])


# Pi function from the decimal.py documentation
def pi_float():
    """native float"""
    lasts, t, s, n, na, d, da = 0, 3.0, 3, 1, 0, 0, 24
    while s != lasts:
        lasts = s
        n, na = n+na, na+8
        d, da = d+da, da+32
        t = (t * n) / d
        s += t
    return s

def pi_cdecimal():
    """cdecimal"""
    D = C.Decimal
    lasts, t, s, n, na, d, da = D(0), D(3), D(3), D(1), D(0), D(0), D(24)
    while s != lasts:
        lasts = s
        n, na = n+na, na+8
        d, da = d+da, da+32
        t = (t * n) / d
        s += t
    return s

def pi_decimal():
    """decimal"""
    D = P.Decimal
    lasts, t, s, n, na, d, da = D(0), D(3), D(3), D(1), D(0), D(0), D(24)
    while s != lasts:
        lasts = s
        n, na = n+na, na+8
        d, da = d+da, da+32
        t = (t * n) / d
        s += t
    return s

def factorial(n, m):
    if (n > m):
        return factorial(m, n)
    elif m == 0:
        return 1
    elif n == m:
        return n
    else:
        return factorial(n, (n+m)//2) * factorial((n+m)//2 + 1, m)


print("\n# ======================================================================")
print("#                   Calculating pi, 10000 iterations")
print("# ======================================================================\n")

for prec in [9, 19]:
    print("\nPrecision: %d decimal digits\n" % prec)
    for func in [pi_float, pi_cdecimal, pi_decimal]:
        start = time.time()
        C.getcontext().prec = prec
        P.getcontext().prec = prec
        for i in range(10000):
            x = func()
        print("%s:" % func.__name__.replace("pi_", ""))
        print("result: %s" % str(x))
        print("time: %fs\n" % (time.time()-start))


print("\n# ======================================================================")
print("#                               Factorial")
print("# ======================================================================\n")

c = C.getcontext()
c.prec = C.MAX_PREC
c.Emax = C.MAX_EMAX
c.Emin = C.MIN_EMIN

for n in [100000, 1000000]:

    print("n = %d\n" % n)

    # C version of decimal
    start_calc = time.time()
    x = factorial(C.Decimal(n), 0)
    end_calc = time.time()
    start_conv = time.time()
    sx = str(x)
    end_conv = time.time()
    print("cdecimal:")
    print("calculation time: %fs" % (end_calc-start_calc))
    print("conversion time: %fs\n" % (end_conv-start_conv))

    # Python integers
    start_calc = time.time()
    y = factorial(n, 0)
    end_calc = time.time()
    start_conv = time.time()
    sy = str(y)
    end_conv =  time.time()

    print("int:")
    print("calculation time: %fs" % (end_calc-start_calc))
    print("conversion time: %fs\n\n" % (end_conv-start_conv))

    assert(sx == sy)
