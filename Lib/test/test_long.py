from test.test_support import verify, verbose, TestFailed, fcmp
from string import join
from random import random, randint

# SHIFT should match the value in longintrepr.h for best testing.
SHIFT = 15
BASE = 2 ** SHIFT
MASK = BASE - 1
KARATSUBA_CUTOFF = 70   # from longobject.c

# Max number of base BASE digits to use in test cases.  Doubling
# this will more than double the runtime.
MAXDIGITS = 15

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
    verify(ndigits > 0)
    nbits_hi = ndigits * SHIFT
    nbits_lo = nbits_hi - SHIFT + 1
    answer = 0L
    nbits = 0
    r = int(random() * (SHIFT * 2)) | 1  # force 1 bits to start
    while nbits < nbits_lo:
        bits = (r >> 1) + 1
        bits = min(bits, nbits_hi - nbits)
        verify(1 <= bits <= SHIFT)
        nbits = nbits + bits
        answer = answer << bits
        if r & 1:
            answer = answer | ((1 << bits) - 1)
        r = int(random() * (SHIFT * 2))
    verify(nbits_lo <= nbits <= nbits_hi)
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
    q2, r2 = x//y, x%y
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
    if verbose:
        print "long / * % divmod"
    digits = range(1, maxdigits+1) + range(KARATSUBA_CUTOFF,
                                           KARATSUBA_CUTOFF + 14)
    digits.append(KARATSUBA_CUTOFF * 3)
    for lenx in digits:
        x = getran(lenx)
        for leny in digits:
            y = getran(leny) or 1L
            test_division_2(x, y)
# ------------------------------------------------------------ karatsuba

def test_karatsuba():

    if verbose:
        print "Karatsuba"

    digits = range(1, 5) + range(KARATSUBA_CUTOFF, KARATSUBA_CUTOFF + 10)
    digits.extend([KARATSUBA_CUTOFF * 10, KARATSUBA_CUTOFF * 100])

    bits = [digit * SHIFT for digit in digits]

    # Test products of long strings of 1 bits -- (2**x-1)*(2**y-1) ==
    # 2**(x+y) - 2**x - 2**y + 1, so the proper result is easy to check.
    for abits in bits:
        a = (1L << abits) - 1
        for bbits in bits:
            if bbits < abits:
                continue
            b = (1L << bbits) - 1
            x = a * b
            y = ((1L << (abits + bbits)) -
                 (1L << abits) -
                 (1L << bbits) +
                 1)
            check(x == y, "bad result for", a, "*", b, x, y)
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
        check(x // p2 == x >> n, "x // p2 != x >> n for x n p2", x, n, p2)
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
    if verbose:
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
            test_bitop_identities_3(x, y, getran((lenx + leny)//2))

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
    if verbose:
        print "long str/hex/oct/atol"
    for x in special:
        test_format_1(x)
    for i in range(10):
        for lenx in range(1, maxdigits+1):
            x = getran(lenx)
            test_format_1(x)

# ----------------------------------------------------------------- misc

def test_misc(maxdigits=MAXDIGITS):
    if verbose:
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
        y = int(x)
    except OverflowError:
        raise TestFailed, "int(long(sys.maxint) + 1) mustn't overflow"
    if not isinstance(y, long):
        raise TestFailed("int(long(sys.maxint) + 1) should have returned long")

    x = hugeneg_aslong - 1
    try:
        y = int(x)
    except OverflowError:
        raise TestFailed, "int(long(-sys.maxint-1) - 1) mustn't overflow"
    if not isinstance(y, long):
        raise TestFailed("int(long(-sys.maxint-1) - 1) should have returned long")

    class long2(long):
        pass
    x = long2(1L<<100)
    y = int(x)
    if type(y) is not long:
        raise TestFailed("overflowing int conversion must return long not long subtype")
# ----------------------------------- tests of auto int->long conversion

def test_auto_overflow():
    import math, sys

    if verbose:
        print "auto-convert int->long on overflow"

    special = [0, 1, 2, 3, sys.maxint-1, sys.maxint, sys.maxint+1]
    sqrt = int(math.sqrt(sys.maxint))
    special.extend([sqrt-1, sqrt, sqrt+1])
    special.extend([-i for i in special])

    def checkit(*args):
        # Heavy use of nested scopes here!
        verify(got == expected, "for %r expected %r got %r" %
                                (args, expected, got))

    for x in special:
        longx = long(x)

        expected = -longx
        got = -x
        checkit('-', x)

        for y in special:
            longy = long(y)

            expected = longx + longy
            got = x + y
            checkit(x, '+', y)

            expected = longx - longy
            got = x - y
            checkit(x, '-', y)

            expected = longx * longy
            got = x * y
            checkit(x, '*', y)

            if y:
                expected = longx / longy
                got = x / y
                checkit(x, '/', y)

                expected = longx // longy
                got = x // y
                checkit(x, '//', y)

                expected = divmod(longx, longy)
                got = divmod(longx, longy)
                checkit(x, 'divmod', y)

            if abs(y) < 5 and not (x == 0 and y < 0):
                expected = longx ** longy
                got = x ** y
                checkit(x, '**', y)

                for z in special:
                    if z != 0 :
                        if y >= 0:
                            expected = pow(longx, longy, long(z))
                            got = pow(x, y, z)
                            checkit('pow', x, y, '%', z)
                        else:
                            try:
                                pow(longx, longy, long(z))
                            except TypeError:
                                pass
                            else:
                                raise TestFailed("pow%r should have raised "
                                "TypeError" % ((longx, longy, long(z)),))

# ---------------------------------------- tests of long->float overflow

def test_float_overflow():
    import math

    if verbose:
        print "long->float overflow"

    for x in -2.0, -1.0, 0.0, 1.0, 2.0:
        verify(float(long(x)) == x)

    shuge = '12345' * 120
    huge = 1L << 30000
    mhuge = -huge
    namespace = {'huge': huge, 'mhuge': mhuge, 'shuge': shuge, 'math': math}
    for test in ["float(huge)", "float(mhuge)",
                 "complex(huge)", "complex(mhuge)",
                 "complex(huge, 1)", "complex(mhuge, 1)",
                 "complex(1, huge)", "complex(1, mhuge)",
                 "1. + huge", "huge + 1.", "1. + mhuge", "mhuge + 1.",
                 "1. - huge", "huge - 1.", "1. - mhuge", "mhuge - 1.",
                 "1. * huge", "huge * 1.", "1. * mhuge", "mhuge * 1.",
                 "1. // huge", "huge // 1.", "1. // mhuge", "mhuge // 1.",
                 "1. / huge", "huge / 1.", "1. / mhuge", "mhuge / 1.",
                 "1. ** huge", "huge ** 1.", "1. ** mhuge", "mhuge ** 1.",
                 "math.sin(huge)", "math.sin(mhuge)",
                 "math.sqrt(huge)", "math.sqrt(mhuge)", # should do better
                 "math.floor(huge)", "math.floor(mhuge)"]:

        try:
            eval(test, namespace)
        except OverflowError:
            pass
        else:
            raise TestFailed("expected OverflowError from %s" % test)

        # XXX Perhaps float(shuge) can raise OverflowError on some box?
        # The comparison should not.
        if float(shuge) == int(shuge):
            raise TestFailed("float(shuge) should not equal int(shuge)")

# ---------------------------------------------- test huge log and log10

def test_logs():
    import math

    if verbose:
        print "log and log10"

    LOG10E = math.log10(math.e)

    for exp in range(10) + [100, 1000, 10000]:
        value = 10 ** exp
        log10 = math.log10(value)
        verify(fcmp(log10, exp) == 0)

        # log10(value) == exp, so log(value) == log10(value)/log10(e) ==
        # exp/LOG10E
        expected = exp / LOG10E
        log = math.log(value)
        verify(fcmp(log, expected) == 0)

    for bad in -(1L << 10000), -2L, 0L:
        try:
            math.log(bad)
            raise TestFailed("expected ValueError from log(<= 0)")
        except ValueError:
            pass

        try:
            math.log10(bad)
            raise TestFailed("expected ValueError from log10(<= 0)")
        except ValueError:
            pass

# ----------------------------------------------- test mixed comparisons

def test_mixed_compares():
    import math
    import sys

    if verbose:
        print "mixed comparisons"

    # We're mostly concerned with that mixing floats and longs does the
    # right stuff, even when longs are too large to fit in a float.
    # The safest way to check the results is to use an entirely different
    # method, which we do here via a skeletal rational class (which
    # represents all Python ints, longs and floats exactly).
    class Rat:
        def __init__(self, value):
            if isinstance(value, (int, long)):
                self.n = value
                self.d = 1

            elif isinstance(value, float):
                # Convert to exact rational equivalent.
                f, e = math.frexp(abs(value))
                assert f == 0 or 0.5 <= f < 1.0
                # |value| = f * 2**e exactly

                # Suck up CHUNK bits at a time; 28 is enough so that we suck
                # up all bits in 2 iterations for all known binary double-
                # precision formats, and small enough to fit in an int.
                CHUNK = 28
                top = 0
                # invariant: |value| = (top + f) * 2**e exactly
                while f:
                    f = math.ldexp(f, CHUNK)
                    digit = int(f)
                    assert digit >> CHUNK == 0
                    top = (top << CHUNK) | digit
                    f -= digit
                    assert 0.0 <= f < 1.0
                    e -= CHUNK

                # Now |value| = top * 2**e exactly.
                if e >= 0:
                    n = top << e
                    d = 1
                else:
                    n = top
                    d = 1 << -e
                if value < 0:
                    n = -n
                self.n = n
                self.d = d
                assert float(n) / float(d) == value

            else:
                raise TypeError("can't deal with %r" % val)

        def __cmp__(self, other):
            if not isinstance(other, Rat):
                other = Rat(other)
            return cmp(self.n * other.d, self.d * other.n)

    cases = [0, 0.001, 0.99, 1.0, 1.5, 1e20, 1e200]
    # 2**48 is an important boundary in the internals.  2**53 is an
    # important boundary for IEEE double precision.
    for t in 2.0**48, 2.0**50, 2.0**53:
        cases.extend([t - 1.0, t - 0.3, t, t + 0.3, t + 1.0,
                      long(t-1), long(t), long(t+1)])
    cases.extend([0, 1, 2, sys.maxint, float(sys.maxint)])
    # 1L<<20000 should exceed all double formats.  long(1e200) is to
    # check that we get equality with 1e200 above.
    t = long(1e200)
    cases.extend([0L, 1L, 2L, 1L << 20000, t-1, t, t+1])
    cases.extend([-x for x in cases])
    for x in cases:
        Rx = Rat(x)
        for y in cases:
            Ry = Rat(y)
            Rcmp = cmp(Rx, Ry)
            xycmp = cmp(x, y)
            if Rcmp != xycmp:
                raise TestFailed('%r %r %d %d' % (x, y, Rcmp, xycmp))
            if (x == y) != (Rcmp == 0):
                raise TestFailed('%r == %r %d' % (x, y, Rcmp))
            if (x != y) != (Rcmp != 0):
                raise TestFailed('%r != %r %d' % (x, y, Rcmp))
            if (x < y) != (Rcmp < 0):
                raise TestFailed('%r < %r %d' % (x, y, Rcmp))
            if (x <= y) != (Rcmp <= 0):
                raise TestFailed('%r <= %r %d' % (x, y, Rcmp))
            if (x > y) != (Rcmp > 0):
                raise TestFailed('%r > %r %d' % (x, y, Rcmp))
            if (x >= y) != (Rcmp >= 0):
                raise TestFailed('%r >= %r %d' % (x, y, Rcmp))

# ---------------------------------------------------------------- do it

test_division()
test_karatsuba()
test_bitop_identities()
test_format()
test_misc()
test_auto_overflow()
test_float_overflow()
test_logs()
test_mixed_compares()
