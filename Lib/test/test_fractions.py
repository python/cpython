"""Tests for Lib/fractions.py."""

from decimal import Decimal
from test.test_support import run_unittest
import math
import operator
import fractions
import unittest
from copy import copy, deepcopy
from cPickle import dumps, loads
F = fractions.Fraction
gcd = fractions.gcd


class GcdTest(unittest.TestCase):

    def testMisc(self):
        self.assertEquals(0, gcd(0, 0))
        self.assertEquals(1, gcd(1, 0))
        self.assertEquals(-1, gcd(-1, 0))
        self.assertEquals(1, gcd(0, 1))
        self.assertEquals(-1, gcd(0, -1))
        self.assertEquals(1, gcd(7, 1))
        self.assertEquals(-1, gcd(7, -1))
        self.assertEquals(1, gcd(-23, 15))
        self.assertEquals(12, gcd(120, 84))
        self.assertEquals(-12, gcd(84, -120))


def _components(r):
    return (r.numerator, r.denominator)


class FractionTest(unittest.TestCase):

    def assertTypedEquals(self, expected, actual):
        """Asserts that both the types and values are the same."""
        self.assertEquals(type(expected), type(actual))
        self.assertEquals(expected, actual)

    def assertRaisesMessage(self, exc_type, message,
                            callable, *args, **kwargs):
        """Asserts that callable(*args, **kwargs) raises exc_type(message)."""
        try:
            callable(*args, **kwargs)
        except exc_type, e:
            self.assertEquals(message, str(e))
        else:
            self.fail("%s not raised" % exc_type.__name__)

    def testInit(self):
        self.assertEquals((0, 1), _components(F()))
        self.assertEquals((7, 1), _components(F(7)))
        self.assertEquals((7, 3), _components(F(F(7, 3))))

        self.assertEquals((-1, 1), _components(F(-1, 1)))
        self.assertEquals((-1, 1), _components(F(1, -1)))
        self.assertEquals((1, 1), _components(F(-2, -2)))
        self.assertEquals((1, 2), _components(F(5, 10)))
        self.assertEquals((7, 15), _components(F(7, 15)))
        self.assertEquals((10**23, 1), _components(F(10**23)))

        self.assertRaisesMessage(ZeroDivisionError, "Fraction(12, 0)",
                                 F, 12, 0)
        self.assertRaises(TypeError, F, 1.5)
        self.assertRaises(TypeError, F, 1.5 + 3j)

        self.assertRaises(TypeError, F, F(1, 2), 3)
        self.assertRaises(TypeError, F, "3/2", 3)

    def testFromString(self):
        self.assertEquals((5, 1), _components(F("5")))
        self.assertEquals((3, 2), _components(F("3/2")))
        self.assertEquals((3, 2), _components(F(" \n  +3/2")))
        self.assertEquals((-3, 2), _components(F("-3/2  ")))
        self.assertEquals((13, 2), _components(F("    013/02 \n  ")))
        self.assertEquals((13, 2), _components(F(u"    013/02 \n  ")))

        self.assertEquals((16, 5), _components(F(" 3.2 ")))
        self.assertEquals((-16, 5), _components(F(u" -3.2 ")))
        self.assertEquals((-3, 1), _components(F(u" -3. ")))
        self.assertEquals((3, 5), _components(F(u" .6 ")))


        self.assertRaisesMessage(
            ZeroDivisionError, "Fraction(3, 0)",
            F, "3/0")
        self.assertRaisesMessage(
            ValueError, "Invalid literal for Fraction: '3/'",
            F, "3/")
        self.assertRaisesMessage(
            ValueError, "Invalid literal for Fraction: '3 /2'",
            F, "3 /2")
        self.assertRaisesMessage(
            # Denominators don't need a sign.
            ValueError, "Invalid literal for Fraction: '3/+2'",
            F, "3/+2")
        self.assertRaisesMessage(
            # Imitate float's parsing.
            ValueError, "Invalid literal for Fraction: '+ 3/2'",
            F, "+ 3/2")
        self.assertRaisesMessage(
            # Avoid treating '.' as a regex special character.
            ValueError, "Invalid literal for Fraction: '3a2'",
            F, "3a2")
        self.assertRaisesMessage(
            # Only parse ordinary decimals, not scientific form.
            ValueError, "Invalid literal for Fraction: '3.2e4'",
            F, "3.2e4")
        self.assertRaisesMessage(
            # Don't accept combinations of decimals and fractions.
            ValueError, "Invalid literal for Fraction: '3/7.2'",
            F, "3/7.2")
        self.assertRaisesMessage(
            # Don't accept combinations of decimals and fractions.
            ValueError, "Invalid literal for Fraction: '3.2/7'",
            F, "3.2/7")
        self.assertRaisesMessage(
            # Allow 3. and .3, but not .
            ValueError, "Invalid literal for Fraction: '.'",
            F, ".")

    def testImmutable(self):
        r = F(7, 3)
        r.__init__(2, 15)
        self.assertEquals((7, 3), _components(r))

        self.assertRaises(AttributeError, setattr, r, 'numerator', 12)
        self.assertRaises(AttributeError, setattr, r, 'denominator', 6)
        self.assertEquals((7, 3), _components(r))

        # But if you _really_ need to:
        r._numerator = 4
        r._denominator = 2
        self.assertEquals((4, 2), _components(r))
        # Which breaks some important operations:
        self.assertNotEquals(F(4, 2), r)

    def testFromFloat(self):
        self.assertRaises(TypeError, F.from_float, 3+4j)
        self.assertEquals((10, 1), _components(F.from_float(10)))
        bigint = 1234567890123456789
        self.assertEquals((bigint, 1), _components(F.from_float(bigint)))
        self.assertEquals((0, 1), _components(F.from_float(-0.0)))
        self.assertEquals((10, 1), _components(F.from_float(10.0)))
        self.assertEquals((-5, 2), _components(F.from_float(-2.5)))
        self.assertEquals((99999999999999991611392, 1),
                          _components(F.from_float(1e23)))
        self.assertEquals(float(10**23), float(F.from_float(1e23)))
        self.assertEquals((3602879701896397, 1125899906842624),
                          _components(F.from_float(3.2)))
        self.assertEquals(3.2, float(F.from_float(3.2)))

        inf = 1e1000
        nan = inf - inf
        self.assertRaisesMessage(
            TypeError, "Cannot convert inf to Fraction.",
            F.from_float, inf)
        self.assertRaisesMessage(
            TypeError, "Cannot convert -inf to Fraction.",
            F.from_float, -inf)
        self.assertRaisesMessage(
            TypeError, "Cannot convert nan to Fraction.",
            F.from_float, nan)

    def testFromDecimal(self):
        self.assertRaises(TypeError, F.from_decimal, 3+4j)
        self.assertEquals(F(10, 1), F.from_decimal(10))
        self.assertEquals(F(0), F.from_decimal(Decimal("-0")))
        self.assertEquals(F(5, 10), F.from_decimal(Decimal("0.5")))
        self.assertEquals(F(5, 1000), F.from_decimal(Decimal("5e-3")))
        self.assertEquals(F(5000), F.from_decimal(Decimal("5e3")))
        self.assertEquals(1 - F(1, 10**30),
                          F.from_decimal(Decimal("0." + "9" * 30)))

        self.assertRaisesMessage(
            TypeError, "Cannot convert Infinity to Fraction.",
            F.from_decimal, Decimal("inf"))
        self.assertRaisesMessage(
            TypeError, "Cannot convert -Infinity to Fraction.",
            F.from_decimal, Decimal("-inf"))
        self.assertRaisesMessage(
            TypeError, "Cannot convert NaN to Fraction.",
            F.from_decimal, Decimal("nan"))
        self.assertRaisesMessage(
            TypeError, "Cannot convert sNaN to Fraction.",
            F.from_decimal, Decimal("snan"))

    def testLimitDenominator(self):
        rpi = F('3.1415926535897932')
        self.assertEqual(rpi.limit_denominator(10000), F(355, 113))
        self.assertEqual(-rpi.limit_denominator(10000), F(-355, 113))
        self.assertEqual(rpi.limit_denominator(113), F(355, 113))
        self.assertEqual(rpi.limit_denominator(112), F(333, 106))
        self.assertEqual(F(201, 200).limit_denominator(100), F(1))
        self.assertEqual(F(201, 200).limit_denominator(101), F(102, 101))
        self.assertEqual(F(0).limit_denominator(10000), F(0))

    def testConversions(self):
        self.assertTypedEquals(-1, math.trunc(F(-11, 10)))
        self.assertTypedEquals(-1, int(F(-11, 10)))

        self.assertEquals(False, bool(F(0, 1)))
        self.assertEquals(True, bool(F(3, 2)))
        self.assertTypedEquals(0.1, float(F(1, 10)))

        # Check that __float__ isn't implemented by converting the
        # numerator and denominator to float before dividing.
        self.assertRaises(OverflowError, float, long('2'*400+'7'))
        self.assertAlmostEquals(2.0/3,
                                float(F(long('2'*400+'7'), long('3'*400+'1'))))

        self.assertTypedEquals(0.1+0j, complex(F(1,10)))


    def testArithmetic(self):
        self.assertEquals(F(1, 2), F(1, 10) + F(2, 5))
        self.assertEquals(F(-3, 10), F(1, 10) - F(2, 5))
        self.assertEquals(F(1, 25), F(1, 10) * F(2, 5))
        self.assertEquals(F(1, 4), F(1, 10) / F(2, 5))
        self.assertTypedEquals(2, F(9, 10) // F(2, 5))
        self.assertTypedEquals(10**23, F(10**23, 1) // F(1))
        self.assertEquals(F(2, 3), F(-7, 3) % F(3, 2))
        self.assertEquals(F(8, 27), F(2, 3) ** F(3))
        self.assertEquals(F(27, 8), F(2, 3) ** F(-3))
        self.assertTypedEquals(2.0, F(4) ** F(1, 2))
        # Will return 1j in 3.0:
        self.assertRaises(ValueError, pow, F(-1), F(1, 2))

    def testMixedArithmetic(self):
        self.assertTypedEquals(F(11, 10), F(1, 10) + 1)
        self.assertTypedEquals(1.1, F(1, 10) + 1.0)
        self.assertTypedEquals(1.1 + 0j, F(1, 10) + (1.0 + 0j))
        self.assertTypedEquals(F(11, 10), 1 + F(1, 10))
        self.assertTypedEquals(1.1, 1.0 + F(1, 10))
        self.assertTypedEquals(1.1 + 0j, (1.0 + 0j) + F(1, 10))

        self.assertTypedEquals(F(-9, 10), F(1, 10) - 1)
        self.assertTypedEquals(-0.9, F(1, 10) - 1.0)
        self.assertTypedEquals(-0.9 + 0j, F(1, 10) - (1.0 + 0j))
        self.assertTypedEquals(F(9, 10), 1 - F(1, 10))
        self.assertTypedEquals(0.9, 1.0 - F(1, 10))
        self.assertTypedEquals(0.9 + 0j, (1.0 + 0j) - F(1, 10))

        self.assertTypedEquals(F(1, 10), F(1, 10) * 1)
        self.assertTypedEquals(0.1, F(1, 10) * 1.0)
        self.assertTypedEquals(0.1 + 0j, F(1, 10) * (1.0 + 0j))
        self.assertTypedEquals(F(1, 10), 1 * F(1, 10))
        self.assertTypedEquals(0.1, 1.0 * F(1, 10))
        self.assertTypedEquals(0.1 + 0j, (1.0 + 0j) * F(1, 10))

        self.assertTypedEquals(F(1, 10), F(1, 10) / 1)
        self.assertTypedEquals(0.1, F(1, 10) / 1.0)
        self.assertTypedEquals(0.1 + 0j, F(1, 10) / (1.0 + 0j))
        self.assertTypedEquals(F(10, 1), 1 / F(1, 10))
        self.assertTypedEquals(10.0, 1.0 / F(1, 10))
        self.assertTypedEquals(10.0 + 0j, (1.0 + 0j) / F(1, 10))

        self.assertTypedEquals(0, F(1, 10) // 1)
        self.assertTypedEquals(0.0, F(1, 10) // 1.0)
        self.assertTypedEquals(10, 1 // F(1, 10))
        self.assertTypedEquals(10**23, 10**22 // F(1, 10))
        self.assertTypedEquals(10.0, 1.0 // F(1, 10))

        self.assertTypedEquals(F(1, 10), F(1, 10) % 1)
        self.assertTypedEquals(0.1, F(1, 10) % 1.0)
        self.assertTypedEquals(F(0, 1), 1 % F(1, 10))
        self.assertTypedEquals(0.0, 1.0 % F(1, 10))

        # No need for divmod since we don't override it.

        # ** has more interesting conversion rules.
        self.assertTypedEquals(F(100, 1), F(1, 10) ** -2)
        self.assertTypedEquals(F(100, 1), F(10, 1) ** 2)
        self.assertTypedEquals(0.1, F(1, 10) ** 1.0)
        self.assertTypedEquals(0.1 + 0j, F(1, 10) ** (1.0 + 0j))
        self.assertTypedEquals(4 , 2 ** F(2, 1))
        # Will return 1j in 3.0:
        self.assertRaises(ValueError, pow, (-1), F(1, 2))
        self.assertTypedEquals(F(1, 4) , 2 ** F(-2, 1))
        self.assertTypedEquals(2.0 , 4 ** F(1, 2))
        self.assertTypedEquals(0.25, 2.0 ** F(-2, 1))
        self.assertTypedEquals(1.0 + 0j, (1.0 + 0j) ** F(1, 10))

    def testMixingWithDecimal(self):
        # Decimal refuses mixed comparisons.
        self.assertRaisesMessage(
            TypeError,
            "unsupported operand type(s) for +: 'Fraction' and 'Decimal'",
            operator.add, F(3,11), Decimal('3.1415926'))
        self.assertNotEquals(F(5, 2), Decimal('2.5'))

    def testComparisons(self):
        self.assertTrue(F(1, 2) < F(2, 3))
        self.assertFalse(F(1, 2) < F(1, 2))
        self.assertTrue(F(1, 2) <= F(2, 3))
        self.assertTrue(F(1, 2) <= F(1, 2))
        self.assertFalse(F(2, 3) <= F(1, 2))
        self.assertTrue(F(1, 2) == F(1, 2))
        self.assertFalse(F(1, 2) == F(1, 3))
        self.assertFalse(F(1, 2) != F(1, 2))
        self.assertTrue(F(1, 2) != F(1, 3))

    def testMixedLess(self):
        self.assertTrue(2 < F(5, 2))
        self.assertFalse(2 < F(4, 2))
        self.assertTrue(F(5, 2) < 3)
        self.assertFalse(F(4, 2) < 2)

        self.assertTrue(F(1, 2) < 0.6)
        self.assertFalse(F(1, 2) < 0.4)
        self.assertTrue(0.4 < F(1, 2))
        self.assertFalse(0.5 < F(1, 2))

    def testMixedLessEqual(self):
        self.assertTrue(0.5 <= F(1, 2))
        self.assertFalse(0.6 <= F(1, 2))
        self.assertTrue(F(1, 2) <= 0.5)
        self.assertFalse(F(1, 2) <= 0.4)
        self.assertTrue(2 <= F(4, 2))
        self.assertFalse(2 <= F(3, 2))
        self.assertTrue(F(4, 2) <= 2)
        self.assertFalse(F(5, 2) <= 2)

    def testBigFloatComparisons(self):
        # Because 10**23 can't be represented exactly as a float:
        self.assertFalse(F(10**23) == float(10**23))
        # The first test demonstrates why these are important.
        self.assertFalse(1e23 < float(F(math.trunc(1e23) + 1)))
        self.assertTrue(1e23 < F(math.trunc(1e23) + 1))
        self.assertFalse(1e23 <= F(math.trunc(1e23) - 1))
        self.assertTrue(1e23 > F(math.trunc(1e23) - 1))
        self.assertFalse(1e23 >= F(math.trunc(1e23) + 1))

    def testBigComplexComparisons(self):
        self.assertFalse(F(10**23) == complex(10**23))
        self.assertTrue(F(10**23) > complex(10**23))
        self.assertFalse(F(10**23) <= complex(10**23))

    def testMixedEqual(self):
        self.assertTrue(0.5 == F(1, 2))
        self.assertFalse(0.6 == F(1, 2))
        self.assertTrue(F(1, 2) == 0.5)
        self.assertFalse(F(1, 2) == 0.4)
        self.assertTrue(2 == F(4, 2))
        self.assertFalse(2 == F(3, 2))
        self.assertTrue(F(4, 2) == 2)
        self.assertFalse(F(5, 2) == 2)

    def testStringification(self):
        self.assertEquals("Fraction(7, 3)", repr(F(7, 3)))
        self.assertEquals("Fraction(6283185307, 2000000000)",
                          repr(F('3.1415926535')))
        self.assertEquals("Fraction(-1, 100000000000000000000)",
                          repr(F(1, -10**20)))
        self.assertEquals("7/3", str(F(7, 3)))
        self.assertEquals("7", str(F(7, 1)))

    def testHash(self):
        self.assertEquals(hash(2.5), hash(F(5, 2)))
        self.assertEquals(hash(10**50), hash(F(10**50)))
        self.assertNotEquals(hash(float(10**23)), hash(F(10**23)))

    def testApproximatePi(self):
        # Algorithm borrowed from
        # http://docs.python.org/lib/decimal-recipes.html
        three = F(3)
        lasts, t, s, n, na, d, da = 0, three, 3, 1, 0, 0, 24
        while abs(s - lasts) > F(1, 10**9):
            lasts = s
            n, na = n+na, na+8
            d, da = d+da, da+32
            t = (t * n) / d
            s += t
        self.assertAlmostEquals(math.pi, s)

    def testApproximateCos1(self):
        # Algorithm borrowed from
        # http://docs.python.org/lib/decimal-recipes.html
        x = F(1)
        i, lasts, s, fact, num, sign = 0, 0, F(1), 1, 1, 1
        while abs(s - lasts) > F(1, 10**9):
            lasts = s
            i += 2
            fact *= i * (i-1)
            num *= x * x
            sign *= -1
            s += num / fact * sign
        self.assertAlmostEquals(math.cos(1), s)

    def test_copy_deepcopy_pickle(self):
        r = F(13, 7)
        self.assertEqual(r, loads(dumps(r)))
        self.assertEqual(id(r), id(copy(r)))
        self.assertEqual(id(r), id(deepcopy(r)))

    def test_slots(self):
        # Issue 4998
        r = F(13, 7)
        self.assertRaises(AttributeError, setattr, r, 'a', 10)

def test_main():
    run_unittest(FractionTest, GcdTest)

if __name__ == '__main__':
    test_main()
