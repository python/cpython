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

# build some special values
special = map(long, [0, 1, 2, BASE, BASE >> 1])
special.append(0x5555555555555555L)
special.append(0xaaaaaaaaaaaaaaaaL)
#  some solid strings of one bits
p2 = 4L  # 0 and 1 already added
for i in range(2*SHIFT):
    special.append(p2 - 1)
    p2 = p2 << 1
del p2
# add complements & negations
special = special + map(lambda x: ~x, special) + \
                    map(lambda x: -x, special)

# ------------------------------------------------------------ utilities

# Use check instead of assert so the test still does something
# under -O.

def check(ok, *args):
    if not ok:
        raise TestFailed, join(map(str, args), " ")

# Get quasi-random long consisting of ndigits digits (in base BASE).
# quasi == the most-significant digit will not be 0, and the number
# is constructed to contain long strings of 0 and 1 bits.  These are
# more likely than random bits to provoke digit-boundary errors.
# The sign of the number is also random.

def getran(ndigits):
    assert ndigits > 0
    nbits_hi = ndigits * SHIFT
    nbits_lo = nbits_hi - SHIFT + 1
    answer = 0L
    nbits = 0
    r = int(random() * (SHIFT * 2)) | 1  # force 1 bits to start
    while nbits < nbits_lo:
        bits = (r >> 1) + 1
        bits = min(bits, nbits_hi - nbits)
        assert 1 <= bits <= SHIFT
        nbits = nbits + bits
        answer = answer << bits
        if r & 1:
            answer = answer | ((1 << bits) - 1)
        r = int(random() * (SHIFT * 2))
    assert nbits_lo <= nbits <= nbits_hi
    if random() < 0.5:
        answer = -answer
    return answer

# Get random long consisting of ndigits random digits (relative to base
# BASE).  The sign bit is also random.

def getran2(ndigits):
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
    pab, pba = x*y, y*x
    check(pab == pba, "multiplication does not commute for", x, y)
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
    check(x & 0 == 0, "x & 0 != 0 for", x)
    check(x | 0 == x, "x | 0 != x for", x)
    check(x ^ 0 == x, "x ^ 0 != x for", x)
    check(x & -1 == x, "x & -1 != x for", x)
    check(x | -1 == -1, "x | -1 != -1 for", x)
    check(x ^ -1 == ~x, "x ^ -1 != ~x for", x)
    check(x == ~~x, "x != ~~x for", x)
    check(x & x == x, "x & x != x for", x)
    check(x | x == x, "x | x != x for", x)
    check(x ^ x == 0, "x ^ x != 0 for", x)
    check(x & ~x == 0, "x & ~x != 0 for", x)
    check(x | ~x == -1, "x | ~x != -1 for", x)
    check(x ^ ~x == -1, "x ^ ~x != -1 for", x)
    check(-x == 1 + ~x == ~(x-1), "not -x == 1 + ~x == ~(x-1) for", x)
    for n in range(2*SHIFT):
        p2 = 2L ** n
        check(x << n >> n == x, "x << n >> n != x for", x, n)
        check(x / p2 == x >> n, "x / p2 != x >> n for x n p2", x, n, p2)
        check(x * p2 == x << n, "x * p2 != x << n for x n p2", x, n, p2)
        check(x & -p2 == x >> n << n == x & ~(p2 - 1),
            "not x & -p2 == x >> n << n == x & ~(p2 - 1) for x n p2",
            x, n, p2)

def test_bitop_identities_2(x, y):
    check(x & y == y & x, "x & y != y & x for", x, y)
    check(x | y == y | x, "x | y != y | x for", x, y)
    check(x ^ y == y ^ x, "x ^ y != y ^ x for", x, y)
    check(x ^ y ^ x == y, "x ^ y ^ x != y for", x, y)
    check(x & y == ~(~x | ~y), "x & y != ~(~x | ~y) for", x, y)
    check(x | y == ~(~x & ~y), "x | y != ~(~x & ~y) for", x, y)
    check(x ^ y == (x | y) & ~(x & y),
         "x ^ y != (x | y) & ~(x & y) for", x, y)
    check(x ^ y == (x & ~y) | (~x & y),
         "x ^ y == (x & ~y) | (~x & y) for", x, y)
    check(x ^ y == (x | y) & (~x | ~y),
         "x ^ y == (x | y) & (~x | ~y) for", x, y)

def test_bitop_identities_3(x, y, z):
    check((x & y) & z == x & (y & z),
         "(x & y) & z != x & (y & z) for", x, y, z)
    check((x | y) | z == x | (y | z),
         "(x | y) | z != x | (y | z) for", x, y, z)
    check((x ^ y) ^ z == x ^ (y ^ z),
         "(x ^ y) ^ z != x ^ (y ^ z) for", x, y, z)
    check(x & (y | z) == (x & y) | (x & z),
         "x & (y | z) != (x & y) | (x & z) for", x, y, z)
    check(x | (y & z) == (x | y) & (x | z),
         "x | (y & z) != (x | y) & (x | z) for", x, y, z)

def test_bitop_identities(maxdigits=MAXDIGITS):
    print "long bit-operation identities"
    for x in special:
        test_bitop_identities_1(x)
    digits = range(1, maxdigits+1)
    for lenx in digits:
        x = getran(lenx)
        test_bitop_identities_1(x)
        for leny in digits:
            y = getran(leny)
            test_bitop_identities_2(x, y)
            test_bitop_identities_3(x, y, getran((lenx + leny)/2))

# ------------------------------------------------- hex oct repr str atol

def slow_format(x, base):
    if (x, base) == (0, 8):
        # this is an oddball!
        return "0L"
    digits = []
    sign = 0
    if x < 0:
        sign, x = 1, -x
    while x:
        x, r = divmod(x, base)
        digits.append(int(r))
    digits.reverse()
    digits = digits or [0]
    return '-'[:sign] + \
           {8: '0', 10: '', 16: '0x'}[base] + \
           join(map(lambda i: "0123456789ABCDEF"[i], digits), '') + \
           "L"

def test_format_1(x):
    from string import atol
    for base, mapper in (8, oct), (10, repr), (16, hex):
        got = mapper(x)
        expected = slow_format(x, base)
        check(got == expected, mapper.__name__, "returned",
              got, "but expected", expected, "for", x)
        check(atol(got, 0) == x, 'atol("%s", 0) !=' % got, x)
    # str() has to be checked a little differently since there's no
    # trailing "L"
    got = str(x)
    expected = slow_format(x, 10)[:-1]
    check(got == expected, mapper.__name__, "returned",
          got, "but expected", expected, "for", x)

def test_format(maxdigits=MAXDIGITS):
    print "long str/hex/oct/atol"
    for x in special:
        test_format_1(x)
    for i in range(10):
        for lenx in range(1, maxdigits+1):
            x = getran(lenx)
            test_format_1(x)

# ----------------------------------------------------------------- misc

def test_misc(maxdigits=MAXDIGITS):
    print "long miscellaneous operations"
    import sys

    # check the extremes in int<->long conversion
    hugepos = sys.maxint
    hugeneg = -hugepos - 1
    hugepos_aslong = long(hugepos)
    hugeneg_aslong = long(hugeneg)
    check(hugepos == hugepos_aslong, "long(sys.maxint) != sys.maxint")
    check(hugeneg == hugeneg_aslong,
        "long(-sys.maxint-1) != -sys.maxint-1")

    # long -> int should not fail for hugepos_aslong or hugeneg_aslong
    try:
        check(int(hugepos_aslong) == hugepos,
              "converting sys.maxint to long and back to int fails")
    except OverflowError:
        raise TestFailed, "int(long(sys.maxint)) overflowed!"
    try:
        check(int(hugeneg_aslong) == hugeneg,
              "converting -sys.maxint-1 to long and back to int fails")
    except OverflowError:
        raise TestFailed, "int(long(-sys.maxint-1)) overflowed!"

    # but long -> int should overflow for hugepos+1 and hugeneg-1
    x = hugepos_aslong + 1
    try:
        int(x)
        raise ValueError
    except OverflowError:
        pass
    except:
        raise TestFailed, "int(long(sys.maxint) + 1) didn't overflow"

    x = hugeneg_aslong - 1
    try:
        int(x)
        raise ValueError
    except OverflowError:
        pass
    except:
        raise TestFailed, "int(long(-sys.maxint-1) - 1) didn't overflow"

# ---------------------------------------------------------------- do it

test_division()
test_bitop_identities()
test_format()
test_misc()

