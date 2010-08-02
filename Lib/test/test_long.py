import unittest
from test import test_support
import sys

import random

# Used for lazy formatting of failure messages
class Frm(object):
    def __init__(self, format, *args):
        self.format = format
        self.args = args

    def __str__(self):
        return self.format % self.args

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

L = [
        ('0', 0),
        ('1', 1),
        ('9', 9),
        ('10', 10),
        ('99', 99),
        ('100', 100),
        ('314', 314),
        (' 314', 314),
        ('314 ', 314),
        ('  \t\t  314  \t\t  ', 314),
        (repr(sys.maxint), sys.maxint),
        ('  1x', ValueError),
        ('  1  ', 1),
        ('  1\02  ', ValueError),
        ('', ValueError),
        (' ', ValueError),
        ('  \t\t  ', ValueError)
]
if test_support.have_unicode:
    L += [
        (unicode('0'), 0),
        (unicode('1'), 1),
        (unicode('9'), 9),
        (unicode('10'), 10),
        (unicode('99'), 99),
        (unicode('100'), 100),
        (unicode('314'), 314),
        (unicode(' 314'), 314),
        (unicode('\u0663\u0661\u0664 ','raw-unicode-escape'), 314),
        (unicode('  \t\t  314  \t\t  '), 314),
        (unicode('  1x'), ValueError),
        (unicode('  1  '), 1),
        (unicode('  1\02  '), ValueError),
        (unicode(''), ValueError),
        (unicode(' '), ValueError),
        (unicode('  \t\t  '), ValueError),
        (unichr(0x200), ValueError),
]


class LongTest(unittest.TestCase):

    # Get quasi-random long consisting of ndigits digits (in base BASE).
    # quasi == the most-significant digit will not be 0, and the number
    # is constructed to contain long strings of 0 and 1 bits.  These are
    # more likely than random bits to provoke digit-boundary errors.
    # The sign of the number is also random.

    def getran(self, ndigits):
        self.assert_(ndigits > 0)
        nbits_hi = ndigits * SHIFT
        nbits_lo = nbits_hi - SHIFT + 1
        answer = 0L
        nbits = 0
        r = int(random.random() * (SHIFT * 2)) | 1  # force 1 bits to start
        while nbits < nbits_lo:
            bits = (r >> 1) + 1
            bits = min(bits, nbits_hi - nbits)
            self.assert_(1 <= bits <= SHIFT)
            nbits = nbits + bits
            answer = answer << bits
            if r & 1:
                answer = answer | ((1 << bits) - 1)
            r = int(random.random() * (SHIFT * 2))
        self.assert_(nbits_lo <= nbits <= nbits_hi)
        if random.random() < 0.5:
            answer = -answer
        return answer

    # Get random long consisting of ndigits random digits (relative to base
    # BASE).  The sign bit is also random.

    def getran2(ndigits):
        answer = 0L
        for i in xrange(ndigits):
            answer = (answer << SHIFT) | random.randint(0, MASK)
        if random.random() < 0.5:
            answer = -answer
        return answer

    def check_division(self, x, y):
        eq = self.assertEqual
        q, r = divmod(x, y)
        q2, r2 = x//y, x%y
        pab, pba = x*y, y*x
        eq(pab, pba, Frm("multiplication does not commute for %r and %r", x, y))
        eq(q, q2, Frm("divmod returns different quotient than / for %r and %r", x, y))
        eq(r, r2, Frm("divmod returns different mod than %% for %r and %r", x, y))
        eq(x, q*y + r, Frm("x != q*y + r after divmod on x=%r, y=%r", x, y))
        if y > 0:
            self.assert_(0 <= r < y, Frm("bad mod from divmod on %r and %r", x, y))
        else:
            self.assert_(y < r <= 0, Frm("bad mod from divmod on %r and %r", x, y))

    def test_division(self):
        digits = range(1, MAXDIGITS+1) + range(KARATSUBA_CUTOFF,
                                               KARATSUBA_CUTOFF + 14)
        digits.append(KARATSUBA_CUTOFF * 3)
        for lenx in digits:
            x = self.getran(lenx)
            for leny in digits:
                y = self.getran(leny) or 1L
                self.check_division(x, y)

    def test_karatsuba(self):
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
                self.assertEqual(x, y,
                    Frm("bad result for a*b: a=%r, b=%r, x=%r, y=%r", a, b, x, y))

    def check_bitop_identities_1(self, x):
        eq = self.assertEqual
        eq(x & 0, 0, Frm("x & 0 != 0 for x=%r", x))
        eq(x | 0, x, Frm("x | 0 != x for x=%r", x))
        eq(x ^ 0, x, Frm("x ^ 0 != x for x=%r", x))
        eq(x & -1, x, Frm("x & -1 != x for x=%r", x))
        eq(x | -1, -1, Frm("x | -1 != -1 for x=%r", x))
        eq(x ^ -1, ~x, Frm("x ^ -1 != ~x for x=%r", x))
        eq(x, ~~x, Frm("x != ~~x for x=%r", x))
        eq(x & x, x, Frm("x & x != x for x=%r", x))
        eq(x | x, x, Frm("x | x != x for x=%r", x))
        eq(x ^ x, 0, Frm("x ^ x != 0 for x=%r", x))
        eq(x & ~x, 0, Frm("x & ~x != 0 for x=%r", x))
        eq(x | ~x, -1, Frm("x | ~x != -1 for x=%r", x))
        eq(x ^ ~x, -1, Frm("x ^ ~x != -1 for x=%r", x))
        eq(-x, 1 + ~x, Frm("not -x == 1 + ~x for x=%r", x))
        eq(-x, ~(x-1), Frm("not -x == ~(x-1) forx =%r", x))
        for n in xrange(2*SHIFT):
            p2 = 2L ** n
            eq(x << n >> n, x,
                Frm("x << n >> n != x for x=%r, n=%r", (x, n)))
            eq(x // p2, x >> n,
                Frm("x // p2 != x >> n for x=%r n=%r p2=%r", (x, n, p2)))
            eq(x * p2, x << n,
                Frm("x * p2 != x << n for x=%r n=%r p2=%r", (x, n, p2)))
            eq(x & -p2, x >> n << n,
                Frm("not x & -p2 == x >> n << n for x=%r n=%r p2=%r", (x, n, p2)))
            eq(x & -p2, x & ~(p2 - 1),
                Frm("not x & -p2 == x & ~(p2 - 1) for x=%r n=%r p2=%r", (x, n, p2)))

    def check_bitop_identities_2(self, x, y):
        eq = self.assertEqual
        eq(x & y, y & x, Frm("x & y != y & x for x=%r, y=%r", (x, y)))
        eq(x | y, y | x, Frm("x | y != y | x for x=%r, y=%r", (x, y)))
        eq(x ^ y, y ^ x, Frm("x ^ y != y ^ x for x=%r, y=%r", (x, y)))
        eq(x ^ y ^ x, y, Frm("x ^ y ^ x != y for x=%r, y=%r", (x, y)))
        eq(x & y, ~(~x | ~y), Frm("x & y != ~(~x | ~y) for x=%r, y=%r", (x, y)))
        eq(x | y, ~(~x & ~y), Frm("x | y != ~(~x & ~y) for x=%r, y=%r", (x, y)))
        eq(x ^ y, (x | y) & ~(x & y),
             Frm("x ^ y != (x | y) & ~(x & y) for x=%r, y=%r", (x, y)))
        eq(x ^ y, (x & ~y) | (~x & y),
             Frm("x ^ y == (x & ~y) | (~x & y) for x=%r, y=%r", (x, y)))
        eq(x ^ y, (x | y) & (~x | ~y),
             Frm("x ^ y == (x | y) & (~x | ~y) for x=%r, y=%r", (x, y)))

    def check_bitop_identities_3(self, x, y, z):
        eq = self.assertEqual
        eq((x & y) & z, x & (y & z),
             Frm("(x & y) & z != x & (y & z) for x=%r, y=%r, z=%r", (x, y, z)))
        eq((x | y) | z, x | (y | z),
             Frm("(x | y) | z != x | (y | z) for x=%r, y=%r, z=%r", (x, y, z)))
        eq((x ^ y) ^ z, x ^ (y ^ z),
             Frm("(x ^ y) ^ z != x ^ (y ^ z) for x=%r, y=%r, z=%r", (x, y, z)))
        eq(x & (y | z), (x & y) | (x & z),
             Frm("x & (y | z) != (x & y) | (x & z) for x=%r, y=%r, z=%r", (x, y, z)))
        eq(x | (y & z), (x | y) & (x | z),
             Frm("x | (y & z) != (x | y) & (x | z) for x=%r, y=%r, z=%r", (x, y, z)))

    def test_bitop_identities(self):
        for x in special:
            self.check_bitop_identities_1(x)
        digits = xrange(1, MAXDIGITS+1)
        for lenx in digits:
            x = self.getran(lenx)
            self.check_bitop_identities_1(x)
            for leny in digits:
                y = self.getran(leny)
                self.check_bitop_identities_2(x, y)
                self.check_bitop_identities_3(x, y, self.getran((lenx + leny)//2))

    def slow_format(self, x, base):
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
               "".join(map(lambda i: "0123456789abcdef"[i], digits)) + "L"

    def check_format_1(self, x):
        for base, mapper in (8, oct), (10, repr), (16, hex):
            got = mapper(x)
            expected = self.slow_format(x, base)
            msg = Frm("%s returned %r but expected %r for %r",
                mapper.__name__, got, expected, x)
            self.assertEqual(got, expected, msg)
            self.assertEqual(long(got, 0), x, Frm('long("%s", 0) != %r', got, x))
        # str() has to be checked a little differently since there's no
        # trailing "L"
        got = str(x)
        expected = self.slow_format(x, 10)[:-1]
        msg = Frm("%s returned %r but expected %r for %r",
            mapper.__name__, got, expected, x)
        self.assertEqual(got, expected, msg)

    def test_format(self):
        for x in special:
            self.check_format_1(x)
        for i in xrange(10):
            for lenx in xrange(1, MAXDIGITS+1):
                x = self.getran(lenx)
                self.check_format_1(x)

    def test_long(self):
        self.assertEqual(long(314), 314L)
        self.assertEqual(long(3.14), 3L)
        self.assertEqual(long(314L), 314L)
        # Check that long() of basic types actually returns a long
        self.assertEqual(type(long(314)), long)
        self.assertEqual(type(long(3.14)), long)
        self.assertEqual(type(long(314L)), long)
        # Check that conversion from float truncates towards zero
        self.assertEqual(long(-3.14), -3L)
        self.assertEqual(long(3.9), 3L)
        self.assertEqual(long(-3.9), -3L)
        self.assertEqual(long(3.5), 3L)
        self.assertEqual(long(-3.5), -3L)
        self.assertEqual(long("-3"), -3L)
        if test_support.have_unicode:
            self.assertEqual(long(unicode("-3")), -3L)
        # Different base:
        self.assertEqual(long("10",16), 16L)
        if test_support.have_unicode:
            self.assertEqual(long(unicode("10"),16), 16L)
        # Check conversions from string (same test set as for int(), and then some)
        LL = [
                ('1' + '0'*20, 10L**20),
                ('1' + '0'*100, 10L**100)
        ]
        L2 = L[:]
        if test_support.have_unicode:
            L2 += [
                (unicode('1') + unicode('0')*20, 10L**20),
                (unicode('1') + unicode('0')*100, 10L**100),
        ]
        for s, v in L2 + LL:
            for sign in "", "+", "-":
                for prefix in "", " ", "\t", "  \t\t  ":
                    ss = prefix + sign + s
                    vv = v
                    if sign == "-" and v is not ValueError:
                        vv = -v
                    try:
                        self.assertEqual(long(ss), long(vv))
                    except v:
                        pass

        self.assertRaises(ValueError, long, '123\0')
        self.assertRaises(ValueError, long, '53', 40)
        self.assertRaises(TypeError, long, 1, 12)

        # tests with base 0
        self.assertEqual(long(' 0123  ', 0), 83)
        self.assertEqual(long(' 0123  ', 0), 83)
        self.assertEqual(long('000', 0), 0)
        self.assertEqual(long('0o123', 0), 83)
        self.assertEqual(long('0x123', 0), 291)
        self.assertEqual(long('0b100', 0), 4)
        self.assertEqual(long(' 0O123   ', 0), 83)
        self.assertEqual(long(' 0X123  ', 0), 291)
        self.assertEqual(long(' 0B100 ', 0), 4)
        self.assertEqual(long('0', 0), 0)
        self.assertEqual(long('+0', 0), 0)
        self.assertEqual(long('-0', 0), 0)
        self.assertEqual(long('00', 0), 0)
        self.assertRaises(ValueError, long, '08', 0)
        self.assertRaises(ValueError, long, '-012395', 0)

        # SF patch #1638879: embedded NULs were not detected with
        # explicit base
        self.assertRaises(ValueError, long, '123\0', 10)
        self.assertRaises(ValueError, long, '123\x00 245', 20)

        self.assertEqual(long('100000000000000000000000000000000', 2),
                         4294967296)
        self.assertEqual(long('102002022201221111211', 3), 4294967296)
        self.assertEqual(long('10000000000000000', 4), 4294967296)
        self.assertEqual(long('32244002423141', 5), 4294967296)
        self.assertEqual(long('1550104015504', 6), 4294967296)
        self.assertEqual(long('211301422354', 7), 4294967296)
        self.assertEqual(long('40000000000', 8), 4294967296)
        self.assertEqual(long('12068657454', 9), 4294967296)
        self.assertEqual(long('4294967296', 10), 4294967296)
        self.assertEqual(long('1904440554', 11), 4294967296)
        self.assertEqual(long('9ba461594', 12), 4294967296)
        self.assertEqual(long('535a79889', 13), 4294967296)
        self.assertEqual(long('2ca5b7464', 14), 4294967296)
        self.assertEqual(long('1a20dcd81', 15), 4294967296)
        self.assertEqual(long('100000000', 16), 4294967296)
        self.assertEqual(long('a7ffda91', 17), 4294967296)
        self.assertEqual(long('704he7g4', 18), 4294967296)
        self.assertEqual(long('4f5aff66', 19), 4294967296)
        self.assertEqual(long('3723ai4g', 20), 4294967296)
        self.assertEqual(long('281d55i4', 21), 4294967296)
        self.assertEqual(long('1fj8b184', 22), 4294967296)
        self.assertEqual(long('1606k7ic', 23), 4294967296)
        self.assertEqual(long('mb994ag', 24), 4294967296)
        self.assertEqual(long('hek2mgl', 25), 4294967296)
        self.assertEqual(long('dnchbnm', 26), 4294967296)
        self.assertEqual(long('b28jpdm', 27), 4294967296)
        self.assertEqual(long('8pfgih4', 28), 4294967296)
        self.assertEqual(long('76beigg', 29), 4294967296)
        self.assertEqual(long('5qmcpqg', 30), 4294967296)
        self.assertEqual(long('4q0jto4', 31), 4294967296)
        self.assertEqual(long('4000000', 32), 4294967296)
        self.assertEqual(long('3aokq94', 33), 4294967296)
        self.assertEqual(long('2qhxjli', 34), 4294967296)
        self.assertEqual(long('2br45qb', 35), 4294967296)
        self.assertEqual(long('1z141z4', 36), 4294967296)

        self.assertEqual(long('100000000000000000000000000000001', 2),
                         4294967297)
        self.assertEqual(long('102002022201221111212', 3), 4294967297)
        self.assertEqual(long('10000000000000001', 4), 4294967297)
        self.assertEqual(long('32244002423142', 5), 4294967297)
        self.assertEqual(long('1550104015505', 6), 4294967297)
        self.assertEqual(long('211301422355', 7), 4294967297)
        self.assertEqual(long('40000000001', 8), 4294967297)
        self.assertEqual(long('12068657455', 9), 4294967297)
        self.assertEqual(long('4294967297', 10), 4294967297)
        self.assertEqual(long('1904440555', 11), 4294967297)
        self.assertEqual(long('9ba461595', 12), 4294967297)
        self.assertEqual(long('535a7988a', 13), 4294967297)
        self.assertEqual(long('2ca5b7465', 14), 4294967297)
        self.assertEqual(long('1a20dcd82', 15), 4294967297)
        self.assertEqual(long('100000001', 16), 4294967297)
        self.assertEqual(long('a7ffda92', 17), 4294967297)
        self.assertEqual(long('704he7g5', 18), 4294967297)
        self.assertEqual(long('4f5aff67', 19), 4294967297)
        self.assertEqual(long('3723ai4h', 20), 4294967297)
        self.assertEqual(long('281d55i5', 21), 4294967297)
        self.assertEqual(long('1fj8b185', 22), 4294967297)
        self.assertEqual(long('1606k7id', 23), 4294967297)
        self.assertEqual(long('mb994ah', 24), 4294967297)
        self.assertEqual(long('hek2mgm', 25), 4294967297)
        self.assertEqual(long('dnchbnn', 26), 4294967297)
        self.assertEqual(long('b28jpdn', 27), 4294967297)
        self.assertEqual(long('8pfgih5', 28), 4294967297)
        self.assertEqual(long('76beigh', 29), 4294967297)
        self.assertEqual(long('5qmcpqh', 30), 4294967297)
        self.assertEqual(long('4q0jto5', 31), 4294967297)
        self.assertEqual(long('4000001', 32), 4294967297)
        self.assertEqual(long('3aokq95', 33), 4294967297)
        self.assertEqual(long('2qhxjlj', 34), 4294967297)
        self.assertEqual(long('2br45qc', 35), 4294967297)
        self.assertEqual(long('1z141z5', 36), 4294967297)


    def test_conversion(self):
        # Test __long__()
        class ClassicMissingMethods:
            pass
        self.assertRaises(AttributeError, long, ClassicMissingMethods())

        class MissingMethods(object):
            pass
        self.assertRaises(TypeError, long, MissingMethods())

        class Foo0:
            def __long__(self):
                return 42L

        class Foo1(object):
            def __long__(self):
                return 42L

        class Foo2(long):
            def __long__(self):
                return 42L

        class Foo3(long):
            def __long__(self):
                return self

        class Foo4(long):
            def __long__(self):
                return 42

        class Foo5(long):
            def __long__(self):
                return 42.

        self.assertEqual(long(Foo0()), 42L)
        self.assertEqual(long(Foo1()), 42L)
        self.assertEqual(long(Foo2()), 42L)
        self.assertEqual(long(Foo3()), 0)
        self.assertEqual(long(Foo4()), 42)
        self.assertRaises(TypeError, long, Foo5())

        class Classic:
            pass
        for base in (object, Classic):
            class LongOverridesTrunc(base):
                def __long__(self):
                    return 42
                def __trunc__(self):
                    return -12
            self.assertEqual(long(LongOverridesTrunc()), 42)

            class JustTrunc(base):
                def __trunc__(self):
                    return 42
            self.assertEqual(long(JustTrunc()), 42)

            for trunc_result_base in (object, Classic):
                class Integral(trunc_result_base):
                    def __int__(self):
                        return 42

                class TruncReturnsNonLong(base):
                    def __trunc__(self):
                        return Integral()
                self.assertEqual(long(TruncReturnsNonLong()), 42)

                class NonIntegral(trunc_result_base):
                    def __trunc__(self):
                        # Check that we avoid infinite recursion.
                        return NonIntegral()

                class TruncReturnsNonIntegral(base):
                    def __trunc__(self):
                        return NonIntegral()
                try:
                    long(TruncReturnsNonIntegral())
                except TypeError as e:
                    self.assertEquals(str(e),
                                      "__trunc__ returned non-Integral"
                                      " (type NonIntegral)")
                else:
                    self.fail("Failed to raise TypeError with %s" %
                              ((base, trunc_result_base),))

    def test_misc(self):

        # check the extremes in int<->long conversion
        hugepos = sys.maxint
        hugeneg = -hugepos - 1
        hugepos_aslong = long(hugepos)
        hugeneg_aslong = long(hugeneg)
        self.assertEqual(hugepos, hugepos_aslong, "long(sys.maxint) != sys.maxint")
        self.assertEqual(hugeneg, hugeneg_aslong,
            "long(-sys.maxint-1) != -sys.maxint-1")

        # long -> int should not fail for hugepos_aslong or hugeneg_aslong
        x = int(hugepos_aslong)
        try:
            self.assertEqual(x, hugepos,
                  "converting sys.maxint to long and back to int fails")
        except OverflowError:
            self.fail("int(long(sys.maxint)) overflowed!")
        if not isinstance(x, int):
            self.fail("int(long(sys.maxint)) should have returned int")
        x = int(hugeneg_aslong)
        try:
            self.assertEqual(x, hugeneg,
                  "converting -sys.maxint-1 to long and back to int fails")
        except OverflowError:
            self.fail("int(long(-sys.maxint-1)) overflowed!")
        if not isinstance(x, int):
            self.fail("int(long(-sys.maxint-1)) should have returned int")
        # but long -> int should overflow for hugepos+1 and hugeneg-1
        x = hugepos_aslong + 1
        try:
            y = int(x)
        except OverflowError:
            self.fail("int(long(sys.maxint) + 1) mustn't overflow")
        self.assert_(isinstance(y, long),
            "int(long(sys.maxint) + 1) should have returned long")

        x = hugeneg_aslong - 1
        try:
            y = int(x)
        except OverflowError:
            self.fail("int(long(-sys.maxint-1) - 1) mustn't overflow")
        self.assert_(isinstance(y, long),
               "int(long(-sys.maxint-1) - 1) should have returned long")

        class long2(long):
            pass
        x = long2(1L<<100)
        y = int(x)
        self.assert_(type(y) is long,
            "overflowing int conversion must return long not long subtype")

        # long -> Py_ssize_t conversion
        class X(object):
            def __getslice__(self, i, j):
                return i, j

        with test_support._check_py3k_warnings():
            self.assertEqual(X()[-5L:7L], (-5, 7))
            # use the clamping effect to test the smallest and largest longs
            # that fit a Py_ssize_t
            slicemin, slicemax = X()[-2L**100:2L**100]
            self.assertEqual(X()[slicemin:slicemax], (slicemin, slicemax))

# ----------------------------------- tests of auto int->long conversion

    def test_auto_overflow(self):
        import math, sys

        special = [0, 1, 2, 3, sys.maxint-1, sys.maxint, sys.maxint+1]
        sqrt = int(math.sqrt(sys.maxint))
        special.extend([sqrt-1, sqrt, sqrt+1])
        special.extend([-i for i in special])

        def checkit(*args):
            # Heavy use of nested scopes here!
            self.assertEqual(got, expected,
                Frm("for %r expected %r got %r", args, expected, got))

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
                    with test_support._check_py3k_warnings():
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
                                self.assertRaises(TypeError, pow,longx, longy, long(z))

    def test_float_overflow(self):
        import math

        for x in -2.0, -1.0, 0.0, 1.0, 2.0:
            self.assertEqual(float(long(x)), x)

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

            self.assertRaises(OverflowError, eval, test, namespace)

            # XXX Perhaps float(shuge) can raise OverflowError on some box?
            # The comparison should not.
            self.assertNotEqual(float(shuge), int(shuge),
                "float(shuge) should not equal int(shuge)")

    def test_logs(self):
        import math

        LOG10E = math.log10(math.e)

        for exp in range(10) + [100, 1000, 10000]:
            value = 10 ** exp
            log10 = math.log10(value)
            self.assertAlmostEqual(log10, exp)

            # log10(value) == exp, so log(value) == log10(value)/log10(e) ==
            # exp/LOG10E
            expected = exp / LOG10E
            log = math.log(value)
            self.assertAlmostEqual(log, expected)

        for bad in -(1L << 10000), -2L, 0L:
            self.assertRaises(ValueError, math.log, bad)
            self.assertRaises(ValueError, math.log10, bad)

    def test_mixed_compares(self):
        eq = self.assertEqual
        import math

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
                    raise TypeError("can't deal with %r" % value)

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
                eq(Rcmp, xycmp, Frm("%r %r %d %d", x, y, Rcmp, xycmp))
                eq(x == y, Rcmp == 0, Frm("%r == %r %d", x, y, Rcmp))
                eq(x != y, Rcmp != 0, Frm("%r != %r %d", x, y, Rcmp))
                eq(x < y, Rcmp < 0, Frm("%r < %r %d", x, y, Rcmp))
                eq(x <= y, Rcmp <= 0, Frm("%r <= %r %d", x, y, Rcmp))
                eq(x > y, Rcmp > 0, Frm("%r > %r %d", x, y, Rcmp))
                eq(x >= y, Rcmp >= 0, Frm("%r >= %r %d", x, y, Rcmp))

    def test_nan_inf(self):
        self.assertRaises(OverflowError, long, float('inf'))
        self.assertRaises(OverflowError, long, float('-inf'))
        self.assertRaises(ValueError, long, float('nan'))

def test_main():
    test_support.run_unittest(LongTest)

if __name__ == "__main__":
    test_main()
