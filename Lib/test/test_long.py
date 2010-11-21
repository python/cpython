import unittest
from test import support
import sys

import random
import math

# Used for lazy formatting of failure messages
class Frm(object):
    def __init__(self, format, *args):
        self.format = format
        self.args = args

    def __str__(self):
        return self.format % self.args

# SHIFT should match the value in longintrepr.h for best testing.
SHIFT = sys.int_info.bits_per_digit
BASE = 2 ** SHIFT
MASK = BASE - 1
KARATSUBA_CUTOFF = 70   # from longobject.c

# Max number of base BASE digits to use in test cases.  Doubling
# this will more than double the runtime.
MAXDIGITS = 15

# build some special values
special = [0, 1, 2, BASE, BASE >> 1, 0x5555555555555555, 0xaaaaaaaaaaaaaaaa]
#  some solid strings of one bits
p2 = 4  # 0 and 1 already added
for i in range(2*SHIFT):
    special.append(p2 - 1)
    p2 = p2 << 1
del p2
# add complements & negations
special += [~x for x in special] + [-x for x in special]

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
        (repr(sys.maxsize), sys.maxsize),
        ('  1x', ValueError),
        ('  1  ', 1),
        ('  1\02  ', ValueError),
        ('', ValueError),
        (' ', ValueError),
        ('  \t\t  ', ValueError)
]


class LongTest(unittest.TestCase):

    # Get quasi-random long consisting of ndigits digits (in base BASE).
    # quasi == the most-significant digit will not be 0, and the number
    # is constructed to contain long strings of 0 and 1 bits.  These are
    # more likely than random bits to provoke digit-boundary errors.
    # The sign of the number is also random.

    def getran(self, ndigits):
        self.assertTrue(ndigits > 0)
        nbits_hi = ndigits * SHIFT
        nbits_lo = nbits_hi - SHIFT + 1
        answer = 0
        nbits = 0
        r = int(random.random() * (SHIFT * 2)) | 1  # force 1 bits to start
        while nbits < nbits_lo:
            bits = (r >> 1) + 1
            bits = min(bits, nbits_hi - nbits)
            self.assertTrue(1 <= bits <= SHIFT)
            nbits = nbits + bits
            answer = answer << bits
            if r & 1:
                answer = answer | ((1 << bits) - 1)
            r = int(random.random() * (SHIFT * 2))
        self.assertTrue(nbits_lo <= nbits <= nbits_hi)
        if random.random() < 0.5:
            answer = -answer
        return answer

    # Get random long consisting of ndigits random digits (relative to base
    # BASE).  The sign bit is also random.

    def getran2(ndigits):
        answer = 0
        for i in range(ndigits):
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
            self.assertTrue(0 <= r < y, Frm("bad mod from divmod on %r and %r", x, y))
        else:
            self.assertTrue(y < r <= 0, Frm("bad mod from divmod on %r and %r", x, y))

    def test_division(self):
        digits = list(range(1, MAXDIGITS+1)) + list(range(KARATSUBA_CUTOFF,
                                                      KARATSUBA_CUTOFF + 14))
        digits.append(KARATSUBA_CUTOFF * 3)
        for lenx in digits:
            x = self.getran(lenx)
            for leny in digits:
                y = self.getran(leny) or 1
                self.check_division(x, y)

        # specific numbers chosen to exercise corner cases of the
        # current long division implementation

        # 30-bit cases involving a quotient digit estimate of BASE+1
        self.check_division(1231948412290879395966702881,
                            1147341367131428698)
        self.check_division(815427756481275430342312021515587883,
                       707270836069027745)
        self.check_division(627976073697012820849443363563599041,
                       643588798496057020)
        self.check_division(1115141373653752303710932756325578065,
                       1038556335171453937726882627)
        # 30-bit cases that require the post-subtraction correction step
        self.check_division(922498905405436751940989320930368494,
                       949985870686786135626943396)
        self.check_division(768235853328091167204009652174031844,
                       1091555541180371554426545266)

        # 15-bit cases involving a quotient digit estimate of BASE+1
        self.check_division(20172188947443, 615611397)
        self.check_division(1020908530270155025, 950795710)
        self.check_division(128589565723112408, 736393718)
        self.check_division(609919780285761575, 18613274546784)
        # 15-bit cases that require the post-subtraction correction step
        self.check_division(710031681576388032, 26769404391308)
        self.check_division(1933622614268221, 30212853348836)



    def test_karatsuba(self):
        digits = list(range(1, 5)) + list(range(KARATSUBA_CUTOFF,
                                                KARATSUBA_CUTOFF + 10))
        digits.extend([KARATSUBA_CUTOFF * 10, KARATSUBA_CUTOFF * 100])

        bits = [digit * SHIFT for digit in digits]

        # Test products of long strings of 1 bits -- (2**x-1)*(2**y-1) ==
        # 2**(x+y) - 2**x - 2**y + 1, so the proper result is easy to check.
        for abits in bits:
            a = (1 << abits) - 1
            for bbits in bits:
                if bbits < abits:
                    continue
                b = (1 << bbits) - 1
                x = a * b
                y = ((1 << (abits + bbits)) -
                     (1 << abits) -
                     (1 << bbits) +
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
        for n in range(2*SHIFT):
            p2 = 2 ** n
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
        digits = range(1, MAXDIGITS+1)
        for lenx in digits:
            x = self.getran(lenx)
            self.check_bitop_identities_1(x)
            for leny in digits:
                y = self.getran(leny)
                self.check_bitop_identities_2(x, y)
                self.check_bitop_identities_3(x, y, self.getran((lenx + leny)//2))

    def slow_format(self, x, base):
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
               {2: '0b', 8: '0o', 10: '', 16: '0x'}[base] + \
               "".join(map(lambda i: "0123456789abcdef"[i], digits))

    def check_format_1(self, x):
        for base, mapper in (8, oct), (10, repr), (16, hex):
            got = mapper(x)
            expected = self.slow_format(x, base)
            msg = Frm("%s returned %r but expected %r for %r",
                mapper.__name__, got, expected, x)
            self.assertEqual(got, expected, msg)
            self.assertEqual(int(got, 0), x, Frm('long("%s", 0) != %r', got, x))
        # str() has to be checked a little differently since there's no
        # trailing "L"
        got = str(x)
        expected = self.slow_format(x, 10)
        msg = Frm("%s returned %r but expected %r for %r",
            mapper.__name__, got, expected, x)
        self.assertEqual(got, expected, msg)

    def test_format(self):
        for x in special:
            self.check_format_1(x)
        for i in range(10):
            for lenx in range(1, MAXDIGITS+1):
                x = self.getran(lenx)
                self.check_format_1(x)

    def test_long(self):
        self.assertEqual(int(314), 314)
        self.assertEqual(int(3.14), 3)
        self.assertEqual(int(314), 314)
        # Check that conversion from float truncates towards zero
        self.assertEqual(int(-3.14), -3)
        self.assertEqual(int(3.9), 3)
        self.assertEqual(int(-3.9), -3)
        self.assertEqual(int(3.5), 3)
        self.assertEqual(int(-3.5), -3)
        self.assertEqual(int("-3"), -3)
        # Different base:
        self.assertEqual(int("10",16), 16)
        # Check conversions from string (same test set as for int(), and then some)
        LL = [
                ('1' + '0'*20, 10**20),
                ('1' + '0'*100, 10**100)
        ]
        L2 = L[:]
        for s, v in L2 + LL:
            for sign in "", "+", "-":
                for prefix in "", " ", "\t", "  \t\t  ":
                    ss = prefix + sign + s
                    vv = v
                    if sign == "-" and v is not ValueError:
                        vv = -v
                    try:
                        self.assertEqual(int(ss), int(vv))
                    except ValueError:
                        pass

        self.assertRaises(ValueError, int, '123\0')
        self.assertRaises(ValueError, int, '53', 40)
        # trailing L should no longer be accepted...
        self.assertRaises(ValueError, int, '123L')
        self.assertRaises(ValueError, int, '123l')
        self.assertRaises(ValueError, int, '0L')
        self.assertRaises(ValueError, int, '-37L')
        self.assertRaises(ValueError, int, '0x32L', 16)
        self.assertRaises(ValueError, int, '1L', 21)
        # ... but it's just a normal digit if base >= 22
        self.assertEqual(int('1L', 22), 43)

        self.assertRaises(TypeError, int, 1, 12)

        # SF patch #1638879: embedded NULs were not detected with
        # explicit base
        self.assertRaises(ValueError, int, '123\0', 10)
        self.assertRaises(ValueError, int, '123\x00 245', 20)

        self.assertEqual(int('100000000000000000000000000000000', 2),
                         4294967296)
        self.assertEqual(int('102002022201221111211', 3), 4294967296)
        self.assertEqual(int('10000000000000000', 4), 4294967296)
        self.assertEqual(int('32244002423141', 5), 4294967296)
        self.assertEqual(int('1550104015504', 6), 4294967296)
        self.assertEqual(int('211301422354', 7), 4294967296)
        self.assertEqual(int('40000000000', 8), 4294967296)
        self.assertEqual(int('12068657454', 9), 4294967296)
        self.assertEqual(int('4294967296', 10), 4294967296)
        self.assertEqual(int('1904440554', 11), 4294967296)
        self.assertEqual(int('9ba461594', 12), 4294967296)
        self.assertEqual(int('535a79889', 13), 4294967296)
        self.assertEqual(int('2ca5b7464', 14), 4294967296)
        self.assertEqual(int('1a20dcd81', 15), 4294967296)
        self.assertEqual(int('100000000', 16), 4294967296)
        self.assertEqual(int('a7ffda91', 17), 4294967296)
        self.assertEqual(int('704he7g4', 18), 4294967296)
        self.assertEqual(int('4f5aff66', 19), 4294967296)
        self.assertEqual(int('3723ai4g', 20), 4294967296)
        self.assertEqual(int('281d55i4', 21), 4294967296)
        self.assertEqual(int('1fj8b184', 22), 4294967296)
        self.assertEqual(int('1606k7ic', 23), 4294967296)
        self.assertEqual(int('mb994ag', 24), 4294967296)
        self.assertEqual(int('hek2mgl', 25), 4294967296)
        self.assertEqual(int('dnchbnm', 26), 4294967296)
        self.assertEqual(int('b28jpdm', 27), 4294967296)
        self.assertEqual(int('8pfgih4', 28), 4294967296)
        self.assertEqual(int('76beigg', 29), 4294967296)
        self.assertEqual(int('5qmcpqg', 30), 4294967296)
        self.assertEqual(int('4q0jto4', 31), 4294967296)
        self.assertEqual(int('4000000', 32), 4294967296)
        self.assertEqual(int('3aokq94', 33), 4294967296)
        self.assertEqual(int('2qhxjli', 34), 4294967296)
        self.assertEqual(int('2br45qb', 35), 4294967296)
        self.assertEqual(int('1z141z4', 36), 4294967296)

        self.assertEqual(int('100000000000000000000000000000001', 2),
                         4294967297)
        self.assertEqual(int('102002022201221111212', 3), 4294967297)
        self.assertEqual(int('10000000000000001', 4), 4294967297)
        self.assertEqual(int('32244002423142', 5), 4294967297)
        self.assertEqual(int('1550104015505', 6), 4294967297)
        self.assertEqual(int('211301422355', 7), 4294967297)
        self.assertEqual(int('40000000001', 8), 4294967297)
        self.assertEqual(int('12068657455', 9), 4294967297)
        self.assertEqual(int('4294967297', 10), 4294967297)
        self.assertEqual(int('1904440555', 11), 4294967297)
        self.assertEqual(int('9ba461595', 12), 4294967297)
        self.assertEqual(int('535a7988a', 13), 4294967297)
        self.assertEqual(int('2ca5b7465', 14), 4294967297)
        self.assertEqual(int('1a20dcd82', 15), 4294967297)
        self.assertEqual(int('100000001', 16), 4294967297)
        self.assertEqual(int('a7ffda92', 17), 4294967297)
        self.assertEqual(int('704he7g5', 18), 4294967297)
        self.assertEqual(int('4f5aff67', 19), 4294967297)
        self.assertEqual(int('3723ai4h', 20), 4294967297)
        self.assertEqual(int('281d55i5', 21), 4294967297)
        self.assertEqual(int('1fj8b185', 22), 4294967297)
        self.assertEqual(int('1606k7id', 23), 4294967297)
        self.assertEqual(int('mb994ah', 24), 4294967297)
        self.assertEqual(int('hek2mgm', 25), 4294967297)
        self.assertEqual(int('dnchbnn', 26), 4294967297)
        self.assertEqual(int('b28jpdn', 27), 4294967297)
        self.assertEqual(int('8pfgih5', 28), 4294967297)
        self.assertEqual(int('76beigh', 29), 4294967297)
        self.assertEqual(int('5qmcpqh', 30), 4294967297)
        self.assertEqual(int('4q0jto5', 31), 4294967297)
        self.assertEqual(int('4000001', 32), 4294967297)
        self.assertEqual(int('3aokq95', 33), 4294967297)
        self.assertEqual(int('2qhxjlj', 34), 4294967297)
        self.assertEqual(int('2br45qc', 35), 4294967297)
        self.assertEqual(int('1z141z5', 36), 4294967297)

        # tests with base 0
        self.assertEqual(int('000', 0), 0)
        self.assertEqual(int('0o123', 0), 83)
        self.assertEqual(int('0x123', 0), 291)
        self.assertEqual(int('0b100', 0), 4)
        self.assertEqual(int(' 0O123   ', 0), 83)
        self.assertEqual(int(' 0X123  ', 0), 291)
        self.assertEqual(int(' 0B100 ', 0), 4)
        self.assertEqual(int('0', 0), 0)
        self.assertEqual(int('+0', 0), 0)
        self.assertEqual(int('-0', 0), 0)
        self.assertEqual(int('00', 0), 0)
        self.assertRaises(ValueError, int, '08', 0)
        self.assertRaises(ValueError, int, '-012395', 0)

    def test_conversion(self):
        # Test __int__()
        class ClassicMissingMethods:
            pass
        self.assertRaises(TypeError, int, ClassicMissingMethods())

        class MissingMethods(object):
            pass
        self.assertRaises(TypeError, int, MissingMethods())

        class Foo0:
            def __int__(self):
                return 42

        class Foo1(object):
            def __int__(self):
                return 42

        class Foo2(int):
            def __int__(self):
                return 42

        class Foo3(int):
            def __int__(self):
                return self

        class Foo4(int):
            def __int__(self):
                return 42

        class Foo5(int):
            def __int__(self):
                return 42.

        self.assertEqual(int(Foo0()), 42)
        self.assertEqual(int(Foo1()), 42)
        self.assertEqual(int(Foo2()), 42)
        self.assertEqual(int(Foo3()), 0)
        self.assertEqual(int(Foo4()), 42)
        self.assertRaises(TypeError, int, Foo5())

        class Classic:
            pass
        for base in (object, Classic):
            class IntOverridesTrunc(base):
                def __int__(self):
                    return 42
                def __trunc__(self):
                    return -12
            self.assertEqual(int(IntOverridesTrunc()), 42)

            class JustTrunc(base):
                def __trunc__(self):
                    return 42
            self.assertEqual(int(JustTrunc()), 42)

            class JustLong(base):
                # test that __long__ no longer used in 3.x
                def __long__(self):
                    return 42
            self.assertRaises(TypeError, int, JustLong())

            class LongTrunc(base):
                # __long__ should be ignored in 3.x
                def __long__(self):
                    return 42
                def __trunc__(self):
                    return 1729
            self.assertEqual(int(LongTrunc()), 1729)

            for trunc_result_base in (object, Classic):
                class Integral(trunc_result_base):
                    def __int__(self):
                        return 42

                class TruncReturnsNonLong(base):
                    def __trunc__(self):
                        return Integral()
                self.assertEqual(int(TruncReturnsNonLong()), 42)

                class NonIntegral(trunc_result_base):
                    def __trunc__(self):
                        # Check that we avoid infinite recursion.
                        return NonIntegral()

                class TruncReturnsNonIntegral(base):
                    def __trunc__(self):
                        return NonIntegral()
                try:
                    int(TruncReturnsNonIntegral())
                except TypeError as e:
                    self.assertEqual(str(e),
                                     "__trunc__ returned non-Integral"
                                     " (type NonIntegral)")
                else:
                    self.fail("Failed to raise TypeError with %s" %
                              ((base, trunc_result_base),))

    def test_misc(self):

        # check the extremes in int<->long conversion
        hugepos = sys.maxsize
        hugeneg = -hugepos - 1
        hugepos_aslong = int(hugepos)
        hugeneg_aslong = int(hugeneg)
        self.assertEqual(hugepos, hugepos_aslong, "long(sys.maxsize) != sys.maxsize")
        self.assertEqual(hugeneg, hugeneg_aslong,
            "long(-sys.maxsize-1) != -sys.maxsize-1")

        # long -> int should not fail for hugepos_aslong or hugeneg_aslong
        x = int(hugepos_aslong)
        try:
            self.assertEqual(x, hugepos,
                  "converting sys.maxsize to long and back to int fails")
        except OverflowError:
            self.fail("int(long(sys.maxsize)) overflowed!")
        if not isinstance(x, int):
            raise TestFailed("int(long(sys.maxsize)) should have returned int")
        x = int(hugeneg_aslong)
        try:
            self.assertEqual(x, hugeneg,
                  "converting -sys.maxsize-1 to long and back to int fails")
        except OverflowError:
            self.fail("int(long(-sys.maxsize-1)) overflowed!")
        if not isinstance(x, int):
            raise TestFailed("int(long(-sys.maxsize-1)) should have "
                             "returned int")
        # but long -> int should overflow for hugepos+1 and hugeneg-1
        x = hugepos_aslong + 1
        try:
            y = int(x)
        except OverflowError:
            self.fail("int(long(sys.maxsize) + 1) mustn't overflow")
        self.assertTrue(isinstance(y, int),
            "int(long(sys.maxsize) + 1) should have returned long")

        x = hugeneg_aslong - 1
        try:
            y = int(x)
        except OverflowError:
            self.fail("int(long(-sys.maxsize-1) - 1) mustn't overflow")
        self.assertTrue(isinstance(y, int),
               "int(long(-sys.maxsize-1) - 1) should have returned long")

        class long2(int):
            pass
        x = long2(1<<100)
        y = int(x)
        self.assertTrue(type(y) is int,
            "overflowing int conversion must return long not long subtype")

# ----------------------------------- tests of auto int->long conversion

    def test_auto_overflow(self):
        import math, sys

        special = [0, 1, 2, 3, sys.maxsize-1, sys.maxsize, sys.maxsize+1]
        sqrt = int(math.sqrt(sys.maxsize))
        special.extend([sqrt-1, sqrt, sqrt+1])
        special.extend([-i for i in special])

        def checkit(*args):
            # Heavy use of nested scopes here!
            self.assertEqual(got, expected,
                Frm("for %r expected %r got %r", args, expected, got))

        for x in special:
            longx = int(x)

            expected = -longx
            got = -x
            checkit('-', x)

            for y in special:
                longy = int(y)

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
                                expected = pow(longx, longy, int(z))
                                got = pow(x, y, z)
                                checkit('pow', x, y, '%', z)
                            else:
                                self.assertRaises(TypeError, pow,longx, longy, int(z))

    @unittest.skipUnless(float.__getformat__("double").startswith("IEEE"),
                         "test requires IEEE 754 doubles")
    def test_float_conversion(self):
        import sys
        DBL_MAX = sys.float_info.max
        DBL_MAX_EXP = sys.float_info.max_exp
        DBL_MANT_DIG = sys.float_info.mant_dig

        exact_values = [0, 1, 2,
                         2**53-3,
                         2**53-2,
                         2**53-1,
                         2**53,
                         2**53+2,
                         2**54-4,
                         2**54-2,
                         2**54,
                         2**54+4]
        for x in exact_values:
            self.assertEqual(float(x), x)
            self.assertEqual(float(-x), -x)

        # test round-half-even
        for x, y in [(1, 0), (2, 2), (3, 4), (4, 4), (5, 4), (6, 6), (7, 8)]:
            for p in range(15):
                self.assertEqual(int(float(2**p*(2**53+x))), 2**p*(2**53+y))

        for x, y in [(0, 0), (1, 0), (2, 0), (3, 4), (4, 4), (5, 4), (6, 8),
                     (7, 8), (8, 8), (9, 8), (10, 8), (11, 12), (12, 12),
                     (13, 12), (14, 16), (15, 16)]:
            for p in range(15):
                self.assertEqual(int(float(2**p*(2**54+x))), 2**p*(2**54+y))

        # behaviour near extremes of floating-point range
        int_dbl_max = int(DBL_MAX)
        top_power = 2**DBL_MAX_EXP
        halfway = (int_dbl_max + top_power)//2
        self.assertEqual(float(int_dbl_max), DBL_MAX)
        self.assertEqual(float(int_dbl_max+1), DBL_MAX)
        self.assertEqual(float(halfway-1), DBL_MAX)
        self.assertRaises(OverflowError, float, halfway)
        self.assertEqual(float(1-halfway), -DBL_MAX)
        self.assertRaises(OverflowError, float, -halfway)
        self.assertRaises(OverflowError, float, top_power-1)
        self.assertRaises(OverflowError, float, top_power)
        self.assertRaises(OverflowError, float, top_power+1)
        self.assertRaises(OverflowError, float, 2*top_power-1)
        self.assertRaises(OverflowError, float, 2*top_power)
        self.assertRaises(OverflowError, float, top_power*top_power)

        for p in range(100):
            x = 2**p * (2**53 + 1) + 1
            y = 2**p * (2**53 + 2)
            self.assertEqual(int(float(x)), y)

            x = 2**p * (2**53 + 1)
            y = 2**p * 2**53
            self.assertEqual(int(float(x)), y)

    def test_float_overflow(self):
        import math

        for x in -2.0, -1.0, 0.0, 1.0, 2.0:
            self.assertEqual(float(int(x)), x)

        shuge = '12345' * 120
        huge = 1 << 30000
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
                     # math.floor() of an int returns an int now
                     ##"math.floor(huge)", "math.floor(mhuge)",
                     ]:

            self.assertRaises(OverflowError, eval, test, namespace)

            # XXX Perhaps float(shuge) can raise OverflowError on some box?
            # The comparison should not.
            self.assertNotEqual(float(shuge), int(shuge),
                "float(shuge) should not equal int(shuge)")

    def test_logs(self):
        import math

        LOG10E = math.log10(math.e)

        for exp in list(range(10)) + [100, 1000, 10000]:
            value = 10 ** exp
            log10 = math.log10(value)
            self.assertAlmostEqual(log10, exp)

            # log10(value) == exp, so log(value) == log10(value)/log10(e) ==
            # exp/LOG10E
            expected = exp / LOG10E
            log = math.log(value)
            self.assertAlmostEqual(log, expected)

        for bad in -(1 << 10000), -2, 0:
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
                if isinstance(value, int):
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

            def _cmp__(self, other):
                if not isinstance(other, Rat):
                    other = Rat(other)
                x, y = self.n * other.d, self.d * other.n
                return (x > y) - (x < y)
            def __eq__(self, other):
                return self._cmp__(other) == 0
            def __ne__(self, other):
                return self._cmp__(other) != 0
            def __ge__(self, other):
                return self._cmp__(other) >= 0
            def __gt__(self, other):
                return self._cmp__(other) > 0
            def __le__(self, other):
                return self._cmp__(other) <= 0
            def __lt__(self, other):
                return self._cmp__(other) < 0

        cases = [0, 0.001, 0.99, 1.0, 1.5, 1e20, 1e200]
        # 2**48 is an important boundary in the internals.  2**53 is an
        # important boundary for IEEE double precision.
        for t in 2.0**48, 2.0**50, 2.0**53:
            cases.extend([t - 1.0, t - 0.3, t, t + 0.3, t + 1.0,
                          int(t-1), int(t), int(t+1)])
        cases.extend([0, 1, 2, sys.maxsize, float(sys.maxsize)])
        # 1L<<20000 should exceed all double formats.  long(1e200) is to
        # check that we get equality with 1e200 above.
        t = int(1e200)
        cases.extend([0, 1, 2, 1 << 20000, t-1, t, t+1])
        cases.extend([-x for x in cases])
        for x in cases:
            Rx = Rat(x)
            for y in cases:
                Ry = Rat(y)
                Rcmp = (Rx > Ry) - (Rx < Ry)
                xycmp = (x > y) - (x < y)
                eq(Rcmp, xycmp, Frm("%r %r %d %d", x, y, Rcmp, xycmp))
                eq(x == y, Rcmp == 0, Frm("%r == %r %d", x, y, Rcmp))
                eq(x != y, Rcmp != 0, Frm("%r != %r %d", x, y, Rcmp))
                eq(x < y, Rcmp < 0, Frm("%r < %r %d", x, y, Rcmp))
                eq(x <= y, Rcmp <= 0, Frm("%r <= %r %d", x, y, Rcmp))
                eq(x > y, Rcmp > 0, Frm("%r > %r %d", x, y, Rcmp))
                eq(x >= y, Rcmp >= 0, Frm("%r >= %r %d", x, y, Rcmp))

    def test__format__(self):
        self.assertEqual(format(123456789, 'd'), '123456789')
        self.assertEqual(format(123456789, 'd'), '123456789')

        # sign and aligning are interdependent
        self.assertEqual(format(1, "-"), '1')
        self.assertEqual(format(-1, "-"), '-1')
        self.assertEqual(format(1, "-3"), '  1')
        self.assertEqual(format(-1, "-3"), ' -1')
        self.assertEqual(format(1, "+3"), ' +1')
        self.assertEqual(format(-1, "+3"), ' -1')
        self.assertEqual(format(1, " 3"), '  1')
        self.assertEqual(format(-1, " 3"), ' -1')
        self.assertEqual(format(1, " "), ' 1')
        self.assertEqual(format(-1, " "), '-1')

        # hex
        self.assertEqual(format(3, "x"), "3")
        self.assertEqual(format(3, "X"), "3")
        self.assertEqual(format(1234, "x"), "4d2")
        self.assertEqual(format(-1234, "x"), "-4d2")
        self.assertEqual(format(1234, "8x"), "     4d2")
        self.assertEqual(format(-1234, "8x"), "    -4d2")
        self.assertEqual(format(1234, "x"), "4d2")
        self.assertEqual(format(-1234, "x"), "-4d2")
        self.assertEqual(format(-3, "x"), "-3")
        self.assertEqual(format(-3, "X"), "-3")
        self.assertEqual(format(int('be', 16), "x"), "be")
        self.assertEqual(format(int('be', 16), "X"), "BE")
        self.assertEqual(format(-int('be', 16), "x"), "-be")
        self.assertEqual(format(-int('be', 16), "X"), "-BE")

        # octal
        self.assertEqual(format(3, "b"), "11")
        self.assertEqual(format(-3, "b"), "-11")
        self.assertEqual(format(1234, "b"), "10011010010")
        self.assertEqual(format(-1234, "b"), "-10011010010")
        self.assertEqual(format(1234, "-b"), "10011010010")
        self.assertEqual(format(-1234, "-b"), "-10011010010")
        self.assertEqual(format(1234, " b"), " 10011010010")
        self.assertEqual(format(-1234, " b"), "-10011010010")
        self.assertEqual(format(1234, "+b"), "+10011010010")
        self.assertEqual(format(-1234, "+b"), "-10011010010")

        # make sure these are errors
        self.assertRaises(ValueError, format, 3, "1.3")  # precision disallowed
        self.assertRaises(ValueError, format, 3, "+c")   # sign not allowed
                                                         # with 'c'

        # ensure that only int and float type specifiers work
        for format_spec in ([chr(x) for x in range(ord('a'), ord('z')+1)] +
                            [chr(x) for x in range(ord('A'), ord('Z')+1)]):
            if not format_spec in 'bcdoxXeEfFgGn%':
                self.assertRaises(ValueError, format, 0, format_spec)
                self.assertRaises(ValueError, format, 1, format_spec)
                self.assertRaises(ValueError, format, -1, format_spec)
                self.assertRaises(ValueError, format, 2**100, format_spec)
                self.assertRaises(ValueError, format, -(2**100), format_spec)

        # ensure that float type specifiers work; format converts
        #  the int to a float
        for format_spec in 'eEfFgG%':
            for value in [0, 1, -1, 100, -100, 1234567890, -1234567890]:
                self.assertEqual(format(value, format_spec),
                                 format(float(value), format_spec))

    def test_nan_inf(self):
        self.assertRaises(OverflowError, int, float('inf'))
        self.assertRaises(OverflowError, int, float('-inf'))
        self.assertRaises(ValueError, int, float('nan'))

    def test_true_division(self):
        huge = 1 << 40000
        mhuge = -huge
        self.assertEqual(huge / huge, 1.0)
        self.assertEqual(mhuge / mhuge, 1.0)
        self.assertEqual(huge / mhuge, -1.0)
        self.assertEqual(mhuge / huge, -1.0)
        self.assertEqual(1 / huge, 0.0)
        self.assertEqual(1 / huge, 0.0)
        self.assertEqual(1 / mhuge, 0.0)
        self.assertEqual(1 / mhuge, 0.0)
        self.assertEqual((666 * huge + (huge >> 1)) / huge, 666.5)
        self.assertEqual((666 * mhuge + (mhuge >> 1)) / mhuge, 666.5)
        self.assertEqual((666 * huge + (huge >> 1)) / mhuge, -666.5)
        self.assertEqual((666 * mhuge + (mhuge >> 1)) / huge, -666.5)
        self.assertEqual(huge / (huge << 1), 0.5)
        self.assertEqual((1000000 * huge) / huge, 1000000)

        namespace = {'huge': huge, 'mhuge': mhuge}

        for overflow in ["float(huge)", "float(mhuge)",
                         "huge / 1", "huge / 2", "huge / -1", "huge / -2",
                         "mhuge / 100", "mhuge / 200"]:
            self.assertRaises(OverflowError, eval, overflow, namespace)

        for underflow in ["1 / huge", "2 / huge", "-1 / huge", "-2 / huge",
                         "100 / mhuge", "200 / mhuge"]:
            result = eval(underflow, namespace)
            self.assertEqual(result, 0.0,
                             "expected underflow to 0 from %r" % underflow)

        for zero in ["huge / 0", "mhuge / 0"]:
            self.assertRaises(ZeroDivisionError, eval, zero, namespace)


    def test_small_ints(self):
        for i in range(-5, 257):
            self.assertTrue(i is i + 0)
            self.assertTrue(i is i * 1)
            self.assertTrue(i is i - 0)
            self.assertTrue(i is i // 1)
            self.assertTrue(i is i & -1)
            self.assertTrue(i is i | 0)
            self.assertTrue(i is i ^ 0)
            self.assertTrue(i is ~~i)
            self.assertTrue(i is i**1)
            self.assertTrue(i is int(str(i)))
            self.assertTrue(i is i<<2>>2, str(i))
        # corner cases
        i = 1 << 70
        self.assertTrue(i - i is 0)
        self.assertTrue(0 * i is 0)

    def test_bit_length(self):
        tiny = 1e-10
        for x in range(-65000, 65000):
            k = x.bit_length()
            # Check equivalence with Python version
            self.assertEqual(k, len(bin(x).lstrip('-0b')))
            # Behaviour as specified in the docs
            if x != 0:
                self.assertTrue(2**(k-1) <= abs(x) < 2**k)
            else:
                self.assertEqual(k, 0)
            # Alternative definition: x.bit_length() == 1 + floor(log_2(x))
            if x != 0:
                # When x is an exact power of 2, numeric errors can
                # cause floor(log(x)/log(2)) to be one too small; for
                # small x this can be fixed by adding a small quantity
                # to the quotient before taking the floor.
                self.assertEqual(k, 1 + math.floor(
                        math.log(abs(x))/math.log(2) + tiny))

        self.assertEqual((0).bit_length(), 0)
        self.assertEqual((1).bit_length(), 1)
        self.assertEqual((-1).bit_length(), 1)
        self.assertEqual((2).bit_length(), 2)
        self.assertEqual((-2).bit_length(), 2)
        for i in [2, 3, 15, 16, 17, 31, 32, 33, 63, 64, 234]:
            a = 2**i
            self.assertEqual((a-1).bit_length(), i)
            self.assertEqual((1-a).bit_length(), i)
            self.assertEqual((a).bit_length(), i+1)
            self.assertEqual((-a).bit_length(), i+1)
            self.assertEqual((a+1).bit_length(), i+1)
            self.assertEqual((-a-1).bit_length(), i+1)

    def test_round(self):
        # check round-half-even algorithm. For round to nearest ten;
        # rounding map is invariant under adding multiples of 20
        test_dict = {0:0, 1:0, 2:0, 3:0, 4:0, 5:0,
                     6:10, 7:10, 8:10, 9:10, 10:10, 11:10, 12:10, 13:10, 14:10,
                     15:20, 16:20, 17:20, 18:20, 19:20}
        for offset in range(-520, 520, 20):
            for k, v in test_dict.items():
                got = round(k+offset, -1)
                expected = v+offset
                self.assertEqual(got, expected)
                self.assertTrue(type(got) is int)

        # larger second argument
        self.assertEqual(round(-150, -2), -200)
        self.assertEqual(round(-149, -2), -100)
        self.assertEqual(round(-51, -2), -100)
        self.assertEqual(round(-50, -2), 0)
        self.assertEqual(round(-49, -2), 0)
        self.assertEqual(round(-1, -2), 0)
        self.assertEqual(round(0, -2), 0)
        self.assertEqual(round(1, -2), 0)
        self.assertEqual(round(49, -2), 0)
        self.assertEqual(round(50, -2), 0)
        self.assertEqual(round(51, -2), 100)
        self.assertEqual(round(149, -2), 100)
        self.assertEqual(round(150, -2), 200)
        self.assertEqual(round(250, -2), 200)
        self.assertEqual(round(251, -2), 300)
        self.assertEqual(round(172500, -3), 172000)
        self.assertEqual(round(173500, -3), 174000)
        self.assertEqual(round(31415926535, -1), 31415926540)
        self.assertEqual(round(31415926535, -2), 31415926500)
        self.assertEqual(round(31415926535, -3), 31415927000)
        self.assertEqual(round(31415926535, -4), 31415930000)
        self.assertEqual(round(31415926535, -5), 31415900000)
        self.assertEqual(round(31415926535, -6), 31416000000)
        self.assertEqual(round(31415926535, -7), 31420000000)
        self.assertEqual(round(31415926535, -8), 31400000000)
        self.assertEqual(round(31415926535, -9), 31000000000)
        self.assertEqual(round(31415926535, -10), 30000000000)
        self.assertEqual(round(31415926535, -11), 0)
        self.assertEqual(round(31415926535, -12), 0)
        self.assertEqual(round(31415926535, -999), 0)

        # should get correct results even for huge inputs
        for k in range(10, 100):
            got = round(10**k + 324678, -3)
            expect = 10**k + 325000
            self.assertEqual(got, expect)
            self.assertTrue(type(got) is int)

        # nonnegative second argument: round(x, n) should just return x
        for n in range(5):
            for i in range(100):
                x = random.randrange(-10000, 10000)
                got = round(x, n)
                self.assertEqual(got, x)
                self.assertTrue(type(got) is int)
        for huge_n in 2**31-1, 2**31, 2**63-1, 2**63, 2**100, 10**100:
            self.assertEqual(round(8979323, huge_n), 8979323)

        # omitted second argument
        for i in range(100):
            x = random.randrange(-10000, 10000)
            got = round(x)
            self.assertEqual(got, x)
            self.assertTrue(type(got) is int)

        # bad second argument
        bad_exponents = ('brian', 2.0, 0j, None)
        for e in bad_exponents:
            self.assertRaises(TypeError, round, 3, e)



def test_main():
    support.run_unittest(LongTest)

if __name__ == "__main__":
    test_main()
