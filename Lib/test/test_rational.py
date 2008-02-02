"""Tests for Lib/rational.py."""

from decimal import Decimal
from test.test_support import run_unittest, verbose
import math
import operator
import rational
import unittest
from copy import copy, deepcopy
from cPickle import dumps, loads
R = rational.Rational
gcd = rational.gcd


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


class RationalTest(unittest.TestCase):

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
        self.assertEquals((0, 1), _components(R()))
        self.assertEquals((7, 1), _components(R(7)))
        self.assertEquals((7, 3), _components(R(R(7, 3))))

        self.assertEquals((-1, 1), _components(R(-1, 1)))
        self.assertEquals((-1, 1), _components(R(1, -1)))
        self.assertEquals((1, 1), _components(R(-2, -2)))
        self.assertEquals((1, 2), _components(R(5, 10)))
        self.assertEquals((7, 15), _components(R(7, 15)))
        self.assertEquals((10**23, 1), _components(R(10**23)))

        self.assertRaisesMessage(ZeroDivisionError, "Rational(12, 0)",
                                 R, 12, 0)
        self.assertRaises(TypeError, R, 1.5)
        self.assertRaises(TypeError, R, 1.5 + 3j)

        self.assertRaises(TypeError, R, R(1, 2), 3)
        self.assertRaises(TypeError, R, "3/2", 3)

    def testFromString(self):
        self.assertEquals((5, 1), _components(R("5")))
        self.assertEquals((3, 2), _components(R("3/2")))
        self.assertEquals((3, 2), _components(R(" \n  +3/2")))
        self.assertEquals((-3, 2), _components(R("-3/2  ")))
        self.assertEquals((13, 2), _components(R("    013/02 \n  ")))
        self.assertEquals((13, 2), _components(R(u"    013/02 \n  ")))

        self.assertEquals((16, 5), _components(R(" 3.2 ")))
        self.assertEquals((-16, 5), _components(R(u" -3.2 ")))
        self.assertEquals((-3, 1), _components(R(u" -3. ")))
        self.assertEquals((3, 5), _components(R(u" .6 ")))


        self.assertRaisesMessage(
            ZeroDivisionError, "Rational(3, 0)",
            R, "3/0")
        self.assertRaisesMessage(
            ValueError, "Invalid literal for Rational: 3/",
            R, "3/")
        self.assertRaisesMessage(
            ValueError, "Invalid literal for Rational: 3 /2",
            R, "3 /2")
        self.assertRaisesMessage(
            # Denominators don't need a sign.
            ValueError, "Invalid literal for Rational: 3/+2",
            R, "3/+2")
        self.assertRaisesMessage(
            # Imitate float's parsing.
            ValueError, "Invalid literal for Rational: + 3/2",
            R, "+ 3/2")
        self.assertRaisesMessage(
            # Avoid treating '.' as a regex special character.
            ValueError, "Invalid literal for Rational: 3a2",
            R, "3a2")
        self.assertRaisesMessage(
            # Only parse ordinary decimals, not scientific form.
            ValueError, "Invalid literal for Rational: 3.2e4",
            R, "3.2e4")
        self.assertRaisesMessage(
            # Don't accept combinations of decimals and rationals.
            ValueError, "Invalid literal for Rational: 3/7.2",
            R, "3/7.2")
        self.assertRaisesMessage(
            # Don't accept combinations of decimals and rationals.
            ValueError, "Invalid literal for Rational: 3.2/7",
            R, "3.2/7")
        self.assertRaisesMessage(
            # Allow 3. and .3, but not .
            ValueError, "Invalid literal for Rational: .",
            R, ".")

    def testImmutable(self):
        r = R(7, 3)
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
        self.assertNotEquals(R(4, 2), r)

    def testFromFloat(self):
        self.assertRaisesMessage(
            TypeError, "Rational.from_float() only takes floats, not 3 (int)",
            R.from_float, 3)

        self.assertEquals((0, 1), _components(R.from_float(-0.0)))
        self.assertEquals((10, 1), _components(R.from_float(10.0)))
        self.assertEquals((-5, 2), _components(R.from_float(-2.5)))
        self.assertEquals((99999999999999991611392, 1),
                          _components(R.from_float(1e23)))
        self.assertEquals(float(10**23), float(R.from_float(1e23)))
        self.assertEquals((3602879701896397, 1125899906842624),
                          _components(R.from_float(3.2)))
        self.assertEquals(3.2, float(R.from_float(3.2)))

        inf = 1e1000
        nan = inf - inf
        self.assertRaisesMessage(
            TypeError, "Cannot convert inf to Rational.",
            R.from_float, inf)
        self.assertRaisesMessage(
            TypeError, "Cannot convert -inf to Rational.",
            R.from_float, -inf)
        self.assertRaisesMessage(
            TypeError, "Cannot convert nan to Rational.",
            R.from_float, nan)

    def testFromDecimal(self):
        self.assertRaisesMessage(
            TypeError,
            "Rational.from_decimal() only takes Decimals, not 3 (int)",
            R.from_decimal, 3)
        self.assertEquals(R(0), R.from_decimal(Decimal("-0")))
        self.assertEquals(R(5, 10), R.from_decimal(Decimal("0.5")))
        self.assertEquals(R(5, 1000), R.from_decimal(Decimal("5e-3")))
        self.assertEquals(R(5000), R.from_decimal(Decimal("5e3")))
        self.assertEquals(1 - R(1, 10**30),
                          R.from_decimal(Decimal("0." + "9" * 30)))

        self.assertRaisesMessage(
            TypeError, "Cannot convert Infinity to Rational.",
            R.from_decimal, Decimal("inf"))
        self.assertRaisesMessage(
            TypeError, "Cannot convert -Infinity to Rational.",
            R.from_decimal, Decimal("-inf"))
        self.assertRaisesMessage(
            TypeError, "Cannot convert NaN to Rational.",
            R.from_decimal, Decimal("nan"))
        self.assertRaisesMessage(
            TypeError, "Cannot convert sNaN to Rational.",
            R.from_decimal, Decimal("snan"))

    def testFromContinuedFraction(self):
        self.assertRaises(TypeError, R.from_continued_fraction, None)
        phi = R.from_continued_fraction([1]*100)
        self.assertEquals(round(phi - (1 + 5 ** 0.5) / 2, 10), 0.0)

        minusphi = R.from_continued_fraction([-1]*100)
        self.assertEquals(round(minusphi + (1 + 5 ** 0.5) / 2, 10), 0.0)

        self.assertEquals(R.from_continued_fraction([0]), R(0))
        self.assertEquals(R.from_continued_fraction([]), R(0))

    def testAsContinuedFraction(self):
        self.assertEqual(R.from_float(math.pi).as_continued_fraction()[:15],
                         [3, 7, 15, 1, 292, 1, 1, 1, 2, 1, 3, 1, 14, 3, 3])
        self.assertEqual(R.from_float(-math.pi).as_continued_fraction()[:16],
                         [-4, 1, 6, 15, 1, 292, 1, 1, 1, 2, 1, 3, 1, 14, 3, 3])
        self.assertEqual(R(0).as_continued_fraction(), [0])

    def testApproximateFrom(self):
        self.assertEqual(R.from_float(math.pi).approximate(10000), R(355, 113))
        self.assertEqual(R.from_float(-math.pi).approximate(10000), R(-355, 113))
        self.assertEqual(R.from_float(0.0).approximate(10000), R(0))

    def testConversions(self):
        self.assertTypedEquals(-1, math.trunc(R(-11, 10)))
        self.assertTypedEquals(-1, int(R(-11, 10)))

        self.assertEquals(False, bool(R(0, 1)))
        self.assertEquals(True, bool(R(3, 2)))
        self.assertTypedEquals(0.1, float(R(1, 10)))

        # Check that __float__ isn't implemented by converting the
        # numerator and denominator to float before dividing.
        self.assertRaises(OverflowError, float, long('2'*400+'7'))
        self.assertAlmostEquals(2.0/3,
                                float(R(long('2'*400+'7'), long('3'*400+'1'))))

        self.assertTypedEquals(0.1+0j, complex(R(1,10)))


    def testArithmetic(self):
        self.assertEquals(R(1, 2), R(1, 10) + R(2, 5))
        self.assertEquals(R(-3, 10), R(1, 10) - R(2, 5))
        self.assertEquals(R(1, 25), R(1, 10) * R(2, 5))
        self.assertEquals(R(1, 4), R(1, 10) / R(2, 5))
        self.assertTypedEquals(2, R(9, 10) // R(2, 5))
        self.assertTypedEquals(10**23, R(10**23, 1) // R(1))
        self.assertEquals(R(2, 3), R(-7, 3) % R(3, 2))
        self.assertEquals(R(8, 27), R(2, 3) ** R(3))
        self.assertEquals(R(27, 8), R(2, 3) ** R(-3))
        self.assertTypedEquals(2.0, R(4) ** R(1, 2))
        # Will return 1j in 3.0:
        self.assertRaises(ValueError, pow, R(-1), R(1, 2))

    def testMixedArithmetic(self):
        self.assertTypedEquals(R(11, 10), R(1, 10) + 1)
        self.assertTypedEquals(1.1, R(1, 10) + 1.0)
        self.assertTypedEquals(1.1 + 0j, R(1, 10) + (1.0 + 0j))
        self.assertTypedEquals(R(11, 10), 1 + R(1, 10))
        self.assertTypedEquals(1.1, 1.0 + R(1, 10))
        self.assertTypedEquals(1.1 + 0j, (1.0 + 0j) + R(1, 10))

        self.assertTypedEquals(R(-9, 10), R(1, 10) - 1)
        self.assertTypedEquals(-0.9, R(1, 10) - 1.0)
        self.assertTypedEquals(-0.9 + 0j, R(1, 10) - (1.0 + 0j))
        self.assertTypedEquals(R(9, 10), 1 - R(1, 10))
        self.assertTypedEquals(0.9, 1.0 - R(1, 10))
        self.assertTypedEquals(0.9 + 0j, (1.0 + 0j) - R(1, 10))

        self.assertTypedEquals(R(1, 10), R(1, 10) * 1)
        self.assertTypedEquals(0.1, R(1, 10) * 1.0)
        self.assertTypedEquals(0.1 + 0j, R(1, 10) * (1.0 + 0j))
        self.assertTypedEquals(R(1, 10), 1 * R(1, 10))
        self.assertTypedEquals(0.1, 1.0 * R(1, 10))
        self.assertTypedEquals(0.1 + 0j, (1.0 + 0j) * R(1, 10))

        self.assertTypedEquals(R(1, 10), R(1, 10) / 1)
        self.assertTypedEquals(0.1, R(1, 10) / 1.0)
        self.assertTypedEquals(0.1 + 0j, R(1, 10) / (1.0 + 0j))
        self.assertTypedEquals(R(10, 1), 1 / R(1, 10))
        self.assertTypedEquals(10.0, 1.0 / R(1, 10))
        self.assertTypedEquals(10.0 + 0j, (1.0 + 0j) / R(1, 10))

        self.assertTypedEquals(0, R(1, 10) // 1)
        self.assertTypedEquals(0.0, R(1, 10) // 1.0)
        self.assertTypedEquals(10, 1 // R(1, 10))
        self.assertTypedEquals(10**23, 10**22 // R(1, 10))
        self.assertTypedEquals(10.0, 1.0 // R(1, 10))

        self.assertTypedEquals(R(1, 10), R(1, 10) % 1)
        self.assertTypedEquals(0.1, R(1, 10) % 1.0)
        self.assertTypedEquals(R(0, 1), 1 % R(1, 10))
        self.assertTypedEquals(0.0, 1.0 % R(1, 10))

        # No need for divmod since we don't override it.

        # ** has more interesting conversion rules.
        self.assertTypedEquals(R(100, 1), R(1, 10) ** -2)
        self.assertTypedEquals(R(100, 1), R(10, 1) ** 2)
        self.assertTypedEquals(0.1, R(1, 10) ** 1.0)
        self.assertTypedEquals(0.1 + 0j, R(1, 10) ** (1.0 + 0j))
        self.assertTypedEquals(4 , 2 ** R(2, 1))
        # Will return 1j in 3.0:
        self.assertRaises(ValueError, pow, (-1), R(1, 2))
        self.assertTypedEquals(R(1, 4) , 2 ** R(-2, 1))
        self.assertTypedEquals(2.0 , 4 ** R(1, 2))
        self.assertTypedEquals(0.25, 2.0 ** R(-2, 1))
        self.assertTypedEquals(1.0 + 0j, (1.0 + 0j) ** R(1, 10))

    def testMixingWithDecimal(self):
        # Decimal refuses mixed comparisons.
        self.assertRaisesMessage(
            TypeError,
            "unsupported operand type(s) for +: 'Rational' and 'Decimal'",
            operator.add, R(3,11), Decimal('3.1415926'))
        self.assertNotEquals(R(5, 2), Decimal('2.5'))

    def testComparisons(self):
        self.assertTrue(R(1, 2) < R(2, 3))
        self.assertFalse(R(1, 2) < R(1, 2))
        self.assertTrue(R(1, 2) <= R(2, 3))
        self.assertTrue(R(1, 2) <= R(1, 2))
        self.assertFalse(R(2, 3) <= R(1, 2))
        self.assertTrue(R(1, 2) == R(1, 2))
        self.assertFalse(R(1, 2) == R(1, 3))

    def testMixedLess(self):
        self.assertTrue(2 < R(5, 2))
        self.assertFalse(2 < R(4, 2))
        self.assertTrue(R(5, 2) < 3)
        self.assertFalse(R(4, 2) < 2)

        self.assertTrue(R(1, 2) < 0.6)
        self.assertFalse(R(1, 2) < 0.4)
        self.assertTrue(0.4 < R(1, 2))
        self.assertFalse(0.5 < R(1, 2))

    def testMixedLessEqual(self):
        self.assertTrue(0.5 <= R(1, 2))
        self.assertFalse(0.6 <= R(1, 2))
        self.assertTrue(R(1, 2) <= 0.5)
        self.assertFalse(R(1, 2) <= 0.4)
        self.assertTrue(2 <= R(4, 2))
        self.assertFalse(2 <= R(3, 2))
        self.assertTrue(R(4, 2) <= 2)
        self.assertFalse(R(5, 2) <= 2)

    def testBigFloatComparisons(self):
        # Because 10**23 can't be represented exactly as a float:
        self.assertFalse(R(10**23) == float(10**23))
        # The first test demonstrates why these are important.
        self.assertFalse(1e23 < float(R(math.trunc(1e23) + 1)))
        self.assertTrue(1e23 < R(math.trunc(1e23) + 1))
        self.assertFalse(1e23 <= R(math.trunc(1e23) - 1))
        self.assertTrue(1e23 > R(math.trunc(1e23) - 1))
        self.assertFalse(1e23 >= R(math.trunc(1e23) + 1))

    def testBigComplexComparisons(self):
        self.assertFalse(R(10**23) == complex(10**23))
        self.assertTrue(R(10**23) > complex(10**23))
        self.assertFalse(R(10**23) <= complex(10**23))

    def testMixedEqual(self):
        self.assertTrue(0.5 == R(1, 2))
        self.assertFalse(0.6 == R(1, 2))
        self.assertTrue(R(1, 2) == 0.5)
        self.assertFalse(R(1, 2) == 0.4)
        self.assertTrue(2 == R(4, 2))
        self.assertFalse(2 == R(3, 2))
        self.assertTrue(R(4, 2) == 2)
        self.assertFalse(R(5, 2) == 2)

    def testStringification(self):
        self.assertEquals("Rational(7,3)", repr(R(7, 3)))
        self.assertEquals("7/3", str(R(7, 3)))
        self.assertEquals("7", str(R(7, 1)))

    def testHash(self):
        self.assertEquals(hash(2.5), hash(R(5, 2)))
        self.assertEquals(hash(10**50), hash(R(10**50)))
        self.assertNotEquals(hash(float(10**23)), hash(R(10**23)))

    def testApproximatePi(self):
        # Algorithm borrowed from
        # http://docs.python.org/lib/decimal-recipes.html
        three = R(3)
        lasts, t, s, n, na, d, da = 0, three, 3, 1, 0, 0, 24
        while abs(s - lasts) > R(1, 10**9):
            lasts = s
            n, na = n+na, na+8
            d, da = d+da, da+32
            t = (t * n) / d
            s += t
        self.assertAlmostEquals(math.pi, s)

    def testApproximateCos1(self):
        # Algorithm borrowed from
        # http://docs.python.org/lib/decimal-recipes.html
        x = R(1)
        i, lasts, s, fact, num, sign = 0, 0, R(1), 1, 1, 1
        while abs(s - lasts) > R(1, 10**9):
            lasts = s
            i += 2
            fact *= i * (i-1)
            num *= x * x
            sign *= -1
            s += num / fact * sign
        self.assertAlmostEquals(math.cos(1), s)

    def test_copy_deepcopy_pickle(self):
        r = R(13, 7)
        self.assertEqual(r, loads(dumps(r)))
        self.assertEqual(id(r), id(copy(r)))
        self.assertEqual(id(r), id(deepcopy(r)))

def test_main():
    run_unittest(RationalTest, GcdTest)

if __name__ == '__main__':
    test_main()
