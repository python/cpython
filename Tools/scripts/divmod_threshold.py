#!/usr/bin/env python3
#
# Determine threshold for switching from longobject.c divmod to
# _pylong.int_divmod().

from random import randrange
from time import perf_counter as now
from _pylong import int_divmod as divmod_fast

BITS_PER_DIGIT = 30


def rand_digits(n):
    top = 1 << (n * BITS_PER_DIGIT)
    return randrange(top >> 1, top)


def probe_den(nd):
    den = rand_digits(nd)
    count = 0
    for nn in range(nd, nd + 3000):
        num = rand_digits(nn)
        t0 = now()
        e1, e2 = divmod(num, den)
        t1 = now()
        f1, f2 = divmod_fast(num, den)
        t2 = now()
        s1 = t1 - t0
        s2 = t2 - t1
        assert e1 == f1
        assert e2 == f2
        if s2 < s1:
            count += 1
            if count >= 3:
                print(
                    "for",
                    nd,
                    "denom digits,",
                    nn - nd,
                    "extra num digits is enough",
                )
                break
        else:
            count = 0
    else:
        print("for", nd, "denom digits, no num seems big enough")


def main():
    for nd in range(30):
        nd = (nd + 1) * 100
        probe_den(nd)


if __name__ == '__main__':
    main()
