from test_support import TestFailed, verbose
from string import join
from random import random, randint

# SHIFT should match the value in longintrepr.h for best testing.
SHIFT = 15
BASE = 2 ** SHIFT
MASK = BASE - 1

# Max number of base BASE digits to use in test cases.  Doubling
# this will at least quadruple the runtime.
MAXDIGITS = 10

# ------------------------------------------------------------ utilities

# Use check instead of assert so the test still does something
# under -O.

def check(ok, *args):
    if not ok:
        raise TestFailed, join(map(str, args), " ")

# Get random long consisting of ndigits random digits (relative to base
# BASE).  The sign bit is also random.

def getran(ndigits):
    answer = 0L
    for i in range(ndigits):
        answer = (answer << SHIFT) | randint(0, MASK)
    if random() < 0.5:
        answer = -answer
    return answer

# --------------------------------------------------------------- divmod

def test_division_2(x, y):
    q, r = divmod(x, y)
    q2, r2 = x/y, x%y
    check(q == q2, "divmod returns different quotient than / for", x, y)
    check(r == r2, "divmod returns different mod than % for", x, y)
    check(x == q*y + r, "x != q*y + r after divmod on", x, y)
    if y > 0:
        check(0 <= r < y, "bad mod from divmod on", x, y)
    else:
        check(y < r <= 0, "bad mod from divmod on", x, y)

def test_division(maxdigits=MAXDIGITS):
    print "long / * % divmod"
    digits = range(1, maxdigits+1)
    for lenx in digits:
        x = getran(lenx)
        for leny in digits:
            y = getran(leny) or 1L
            test_division_2(x, y)

# -------------------------------------------------------------- ~ & | ^

def test_bitop_identities_1(x):
    check(x == ~~x, "x != ~~x for", x)
    check(x & x == x, "x & x != x for", x)
    check(x | x == x, "x | x != x for", x)
    check(x ^ x == 0, "x ^ x != 0 for", x)
    check(x & ~x == 0, "x & ~x != 0 for", x)
    check(x | ~x == -1, "x | ~x != -1 for", x)
    check(x ^ ~x == -1, "x ^ ~x != -1 for", x)
    check(-x == 1 + ~x, "-x != 1 + ~x for", x)
    for s in range(2*SHIFT):
        check( ((x >> s) << s) | (x & ((1L << s) - 1)) == x,
              "((x >> s) << s) | (x & ((1L << s) - 1)) != x for",
                x, s)

def test_bitop_identities_2(x, y):
    check(x & y == ~(~x | ~y), "x & y != ~(~x | ~y) for", x, y)
    check(x | y == ~(~x & ~y), "x | y != ~(~x & ~y) for", x, y)
    check(x ^ y == (x | y) & ~(x & y),
         "x ^ y != (x | y) & ~(x & y) for", x, y)
    check(x ^ y == (x & ~y) | (~x & y),
         "x ^ y == (x & ~y) | (~x & y) for", x, y)
    check(x ^ y == (x | y) & (~x | ~y),
         "x ^ y == (x | y) & (~x | ~y) for", x, y)

def test_bitop_identities_3(x, y, z):
    check(x & (y | z) == (x & y) | (x & z),
         "x & (y | z) != (x & y) | (x & z) for", x, y, z)
    check(x | (y & z) == (x | y) & (x | z),
         "x | (y & z) != (x | y) & (x | z) for", x, y, z)

def test_bitop_identities(maxdigits=MAXDIGITS):
    print "long bit-operation identities"
    digits = range(1, maxdigits+1)
    for lenx in digits:
        x = getran(lenx)
        test_bitop_identities_1(x)
        for leny in digits:
            y = getran(leny)
            test_bitop_identities_2(x, y)
            test_bitop_identities_3(x, y, getran((lenx + leny)/2))

# ---------------------------------------------------------- hex oct str

def slow_format(x, base):
    if (x, base) == (0, 8):
        # this is an oddball!
        return "0L"
    digits = []
    sign = 0
    if x < 0:
        sign, x = 1, -x
    marks = "0123456789ABCDEF"
    while x:
        x, r = divmod(x, base)
        digits.append(int(r))
    digits.reverse()
    digits = digits or [0]
    return ['', '-'][sign] + \
           {8: '0', 10: '', 16: '0x'}[base] + \
           join(map(lambda i, marks=marks: marks[i], digits), '') + \
           "L"

def test_format_1(x):
    for base, mapper in (8, oct), (10, str), (16, hex):
        got = mapper(x)
        expected = slow_format(x, base)
        check(got == expected, mapper.__name__, "returned",
              got, "but expected", expected, "for", x)

def test_format(maxdigits=MAXDIGITS):
    print "long str/hex/oct"
    for i in range(10):
        for lenx in range(1, maxdigits+1):
            x = getran(lenx)
            test_format_1(x)

# ---------------------------------------------------------------- do it

test_division()
test_bitop_identities()
test_format()

