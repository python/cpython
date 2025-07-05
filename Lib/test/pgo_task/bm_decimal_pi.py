"""
Calculate `pi` using the decimal module.

The `pidigits` benchmark does a similar thing using regular (long) ints.

- 2024-06-14: Michael Droettboom copied this from
  Modules/_decimal/tests/bench.py in the CPython source and adapted to use
  pyperf.
"""

# Original copyright notice in CPython source:

#
# Copyright (C) 2001-2012 Python Software Foundation. All Rights Reserved.
# Modified and extended by Stefan Krah.
#


import decimal



def pi_decimal():
    """decimal"""
    D = decimal.Decimal
    lasts, t, s, n, na, d, da = D(0), D(3), D(3), D(1), D(0), D(0), D(24)
    while s != lasts:
        lasts = s
        n, na = n + na, na + 8
        d, da = d + da, da + 32
        t = (t * n) / d
        s += t
    return s


def bench_decimal_pi():
    for prec in [9, 19]:
        decimal.getcontext().prec = prec
        for _ in range(10000):
            _ = pi_decimal()


def run_pgo():
    bench_decimal_pi()
