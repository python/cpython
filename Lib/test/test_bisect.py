from test_support import TestFailed

import bisect
import sys

nerrors = 0

def check_bisect(func, list, elt, expected):
    global nerrors
    got = func(list, elt)
    if got != expected:
        print >> sys.stderr, \
            "expected %s(%s, %s) -> %s, but got %s" % (func.__name__,
                                                       list,
                                                       elt,
                                                       expected,
                                                       got)
        nerrors += 1

# XXX optional slice arguments need tests.

check_bisect(bisect.bisect_right, [], 1, 0)
check_bisect(bisect.bisect_right, [1], 0, 0)
check_bisect(bisect.bisect_right, [1], 1, 1)
check_bisect(bisect.bisect_right, [1], 2, 1)
check_bisect(bisect.bisect_right, [1, 1], 0, 0)
check_bisect(bisect.bisect_right, [1, 1], 1, 2)
check_bisect(bisect.bisect_right, [1, 1], 2, 2)
check_bisect(bisect.bisect_right, [1, 1, 1], 0, 0)
check_bisect(bisect.bisect_right, [1, 1, 1], 1, 3)
check_bisect(bisect.bisect_right, [1, 1, 1], 2, 3)
check_bisect(bisect.bisect_right, [1, 1, 1, 1], 0, 0)
check_bisect(bisect.bisect_right, [1, 1, 1, 1], 1, 4)
check_bisect(bisect.bisect_right, [1, 1, 1, 1], 2, 4)
check_bisect(bisect.bisect_right, [1, 2], 0, 0)
check_bisect(bisect.bisect_right, [1, 2], 1, 1)
check_bisect(bisect.bisect_right, [1, 2], 1.5, 1)
check_bisect(bisect.bisect_right, [1, 2], 2, 2)
check_bisect(bisect.bisect_right, [1, 2], 3, 2)
check_bisect(bisect.bisect_right, [1, 1, 2, 2], 0, 0)
check_bisect(bisect.bisect_right, [1, 1, 2, 2], 1, 2)
check_bisect(bisect.bisect_right, [1, 1, 2, 2], 1.5, 2)
check_bisect(bisect.bisect_right, [1, 1, 2, 2], 2, 4)
check_bisect(bisect.bisect_right, [1, 1, 2, 2], 3, 4)
check_bisect(bisect.bisect_right, [1, 2, 3], 0, 0)
check_bisect(bisect.bisect_right, [1, 2, 3], 1, 1)
check_bisect(bisect.bisect_right, [1, 2, 3], 1.5, 1)
check_bisect(bisect.bisect_right, [1, 2, 3], 2, 2)
check_bisect(bisect.bisect_right, [1, 2, 3], 2.5, 2)
check_bisect(bisect.bisect_right, [1, 2, 3], 3, 3)
check_bisect(bisect.bisect_right, [1, 2, 3], 4, 3)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 1)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 1)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 3)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 3)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 6)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 6)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 10)
check_bisect(bisect.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 10)

check_bisect(bisect.bisect_left, [], 1, 0)
check_bisect(bisect.bisect_left, [1], 0, 0)
check_bisect(bisect.bisect_left, [1], 1, 0)
check_bisect(bisect.bisect_left, [1], 2, 1)
check_bisect(bisect.bisect_left, [1, 1], 0, 0)
check_bisect(bisect.bisect_left, [1, 1], 1, 0)
check_bisect(bisect.bisect_left, [1, 1], 2, 2)
check_bisect(bisect.bisect_left, [1, 1, 1], 0, 0)
check_bisect(bisect.bisect_left, [1, 1, 1], 1, 0)
check_bisect(bisect.bisect_left, [1, 1, 1], 2, 3)
check_bisect(bisect.bisect_left, [1, 1, 1, 1], 0, 0)
check_bisect(bisect.bisect_left, [1, 1, 1, 1], 1, 0)
check_bisect(bisect.bisect_left, [1, 1, 1, 1], 2, 4)
check_bisect(bisect.bisect_left, [1, 2], 0, 0)
check_bisect(bisect.bisect_left, [1, 2], 1, 0)
check_bisect(bisect.bisect_left, [1, 2], 1.5, 1)
check_bisect(bisect.bisect_left, [1, 2], 2, 1)
check_bisect(bisect.bisect_left, [1, 2], 3, 2)
check_bisect(bisect.bisect_left, [1, 1, 2, 2], 0, 0)
check_bisect(bisect.bisect_left, [1, 1, 2, 2], 1, 0)
check_bisect(bisect.bisect_left, [1, 1, 2, 2], 1.5, 2)
check_bisect(bisect.bisect_left, [1, 1, 2, 2], 2, 2)
check_bisect(bisect.bisect_left, [1, 1, 2, 2], 3, 4)
check_bisect(bisect.bisect_left, [1, 2, 3], 0, 0)
check_bisect(bisect.bisect_left, [1, 2, 3], 1, 0)
check_bisect(bisect.bisect_left, [1, 2, 3], 1.5, 1)
check_bisect(bisect.bisect_left, [1, 2, 3], 2, 1)
check_bisect(bisect.bisect_left, [1, 2, 3], 2.5, 2)
check_bisect(bisect.bisect_left, [1, 2, 3], 3, 2)
check_bisect(bisect.bisect_left, [1, 2, 3], 4, 3)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 0)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 1)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 1)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 3)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 3)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 6)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 6)
check_bisect(bisect.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 10)

def check_insort(n):
    global nerrors
    from random import choice
    import sys
    digits = "0123456789"
    raw = []
    insorted = []
    for i in range(n):
        digit = choice(digits)
        raw.append(digit)
        if digit in "02468":
            f = bisect.insort_left
        else:
            f = bisect.insort_right
        f(insorted, digit)
    sorted = raw[:]
    sorted.sort()
    if sorted == insorted:
        return
    print >> sys.stderr, "insort test failed: raw %s got %s" % (raw, insorted)
    nerrors += 1

check_insort(500)

if nerrors:
    raise TestFailed("%d errors in test_bisect" % nerrors)
