import imath
from decimal import Decimal
from fractions import Fraction
import unittest
from test import support

class IntSubclass(int):
    pass

# Class providing an __index__ method.
class MyIndexable(object):
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value

# Here's a pure Python version of the math.factorial algorithm, for
# documentation and comparison purposes.
#
# Formula:
#
#   factorial(n) = factorial_odd_part(n) << (n - count_set_bits(n))
#
# where
#
#   factorial_odd_part(n) = product_{i >= 0} product_{0 < j <= n >> i; j odd} j
#
# The outer product above is an infinite product, but once i >= n.bit_length,
# (n >> i) < 1 and the corresponding term of the product is empty.  So only the
# finitely many terms for 0 <= i < n.bit_length() contribute anything.
#
# We iterate downwards from i == n.bit_length() - 1 to i == 0.  The inner
# product in the formula above starts at 1 for i == n.bit_length(); for each i
# < n.bit_length() we get the inner product for i from that for i + 1 by
# multiplying by all j in {n >> i+1 < j <= n >> i; j odd}.  In Python terms,
# this set is range((n >> i+1) + 1 | 1, (n >> i) + 1 | 1, 2).

def count_set_bits(n):
    """Number of '1' bits in binary expansion of a nonnnegative integer."""
    return 1 + count_set_bits(n & n - 1) if n else 0

def partial_product(start, stop):
    """Product of integers in range(start, stop, 2), computed recursively.
    start and stop should both be odd, with start <= stop.

    """
    numfactors = (stop - start) >> 1
    if not numfactors:
        return 1
    elif numfactors == 1:
        return start
    else:
        mid = (start + numfactors) | 1
        return partial_product(start, mid) * partial_product(mid, stop)

def py_factorial(n):
    """Factorial of nonnegative integer n, via "Binary Split Factorial Formula"
    described at http://www.luschny.de/math/factorial/binarysplitfact.html

    """
    inner = outer = 1
    for i in reversed(range(n.bit_length())):
        inner *= partial_product((n >> i + 1) + 1 | 1, (n >> i) + 1 | 1)
        outer *= inner
    return outer << (n - count_set_bits(n))


class IMathTests(unittest.TestCase):

    def assertIntEqual(self, actual, expected):
        self.assertEqual(actual, expected)
        self.assertIs(type(actual), int)

    def testAsIntegerRatio(self):
        as_integer_ratio = imath.as_integer_ratio
        self.assertEqual(as_integer_ratio(0), (0, 1))
        self.assertEqual(as_integer_ratio(3), (3, 1))
        self.assertEqual(as_integer_ratio(-3), (-3, 1))
        self.assertEqual(as_integer_ratio(False), (0, 1))
        self.assertEqual(as_integer_ratio(True), (1, 1))
        self.assertEqual(as_integer_ratio(0.0), (0, 1))
        self.assertEqual(as_integer_ratio(-0.0), (0, 1))
        self.assertEqual(as_integer_ratio(0.875), (7, 8))
        self.assertEqual(as_integer_ratio(-0.875), (-7, 8))
        self.assertEqual(as_integer_ratio(Decimal('0')), (0, 1))
        self.assertEqual(as_integer_ratio(Decimal('0.875')), (7, 8))
        self.assertEqual(as_integer_ratio(Decimal('-0.875')), (-7, 8))
        self.assertEqual(as_integer_ratio(Fraction(0)), (0, 1))
        self.assertEqual(as_integer_ratio(Fraction(7, 8)), (7, 8))
        self.assertEqual(as_integer_ratio(Fraction(-7, 8)), (-7, 8))

        self.assertRaises(OverflowError, as_integer_ratio, float('inf'))
        self.assertRaises(OverflowError, as_integer_ratio, float('-inf'))
        self.assertRaises(ValueError, as_integer_ratio, float('nan'))
        self.assertRaises(OverflowError, as_integer_ratio, Decimal('inf'))
        self.assertRaises(OverflowError, as_integer_ratio, Decimal('-inf'))
        self.assertRaises(ValueError, as_integer_ratio, Decimal('nan'))

        self.assertRaises(TypeError, as_integer_ratio, '0')

    def testFactorial(self):
        factorial = imath.factorial
        self.assertEqual(factorial(0), 1)
        total = 1
        for i in range(1, 1000):
            total *= i
            self.assertEqual(factorial(i), total)
            self.assertEqual(factorial(i), py_factorial(i))

        self.assertIntEqual(factorial(False), 1)
        self.assertIntEqual(factorial(True), 1)
        for i in range(3):
            expected = factorial(i)
            self.assertIntEqual(factorial(IntSubclass(i)), expected)
            self.assertIntEqual(factorial(MyIndexable(i)), expected)

        self.assertRaises(ValueError, factorial, -1)
        self.assertRaises(ValueError, factorial, -10**1000)

        self.assertRaises(TypeError, factorial, 5.0)
        self.assertRaises(TypeError, factorial, -1.0)
        self.assertRaises(TypeError, factorial, -1.0e100)
        self.assertRaises(TypeError, factorial, Decimal(5.0))
        self.assertRaises(TypeError, factorial, Fraction(5, 1))
        self.assertRaises(TypeError, factorial, "5")

        self.assertRaises((OverflowError, MemoryError), factorial, 10**100)

    def testGcd(self):
        gcd = imath.gcd
        self.assertEqual(gcd(0, 0), 0)
        self.assertEqual(gcd(1, 0), 1)
        self.assertEqual(gcd(-1, 0), 1)
        self.assertEqual(gcd(0, 1), 1)
        self.assertEqual(gcd(0, -1), 1)
        self.assertEqual(gcd(7, 1), 1)
        self.assertEqual(gcd(7, -1), 1)
        self.assertEqual(gcd(-23, 15), 1)
        self.assertEqual(gcd(120, 84), 12)
        self.assertEqual(gcd(84, -120), 12)
        self.assertEqual(gcd(1216342683557601535506311712,
                             436522681849110124616458784), 32)
        c = 652560
        x = 434610456570399902378880679233098819019853229470286994367836600566
        y = 1064502245825115327754847244914921553977
        a = x * c
        b = y * c
        self.assertEqual(gcd(a, b), c)
        self.assertEqual(gcd(b, a), c)
        self.assertEqual(gcd(-a, b), c)
        self.assertEqual(gcd(b, -a), c)
        self.assertEqual(gcd(a, -b), c)
        self.assertEqual(gcd(-b, a), c)
        self.assertEqual(gcd(-a, -b), c)
        self.assertEqual(gcd(-b, -a), c)
        c = 576559230871654959816130551884856912003141446781646602790216406874
        a = x * c
        b = y * c
        self.assertEqual(gcd(a, b), c)
        self.assertEqual(gcd(b, a), c)
        self.assertEqual(gcd(-a, b), c)
        self.assertEqual(gcd(b, -a), c)
        self.assertEqual(gcd(a, -b), c)
        self.assertEqual(gcd(-b, a), c)
        self.assertEqual(gcd(-a, -b), c)
        self.assertEqual(gcd(-b, -a), c)

        self.assertRaises(TypeError, gcd, 120.0, 84)
        self.assertRaises(TypeError, gcd, 120, 84.0)
        self.assertIntEqual(gcd(IntSubclass(120), IntSubclass(84)), 12)
        self.assertIntEqual(gcd(MyIndexable(120), MyIndexable(84)), 12)

    def testIlog(self):
        ilog2 = imath.ilog2
        for value in range(1, 1000):
            k = ilog2(value)
            self.assertLessEqual(2**k, value)
            self.assertLess(value, 2**(k+1))
        self.assertRaises(ValueError, ilog2, 0)
        self.assertRaises(ValueError, ilog2, -1)
        self.assertRaises(ValueError, ilog2, -2**1000)

        self.assertIntEqual(ilog2(True), 0)
        self.assertIntEqual(ilog2(IntSubclass(5)), 2)
        self.assertIntEqual(ilog2(MyIndexable(5)), 2)

        self.assertRaises(TypeError, ilog2, 5.0)
        self.assertRaises(TypeError, ilog2, Decimal('5'))
        self.assertRaises(TypeError, ilog2, Fraction(5, 1))
        self.assertRaises(TypeError, ilog2, '5')

    def testIsqrt(self):
        isqrt = imath.isqrt
        # Test a variety of inputs, large and small.
        test_values = (
            list(range(1000))
            + list(range(10**6 - 1000, 10**6 + 1000))
            + [2**e + i for e in range(60, 200) for i in range(-40, 40)]
            + [3**9999, 10**5001]
        )

        for value in test_values:
            with self.subTest(value=value):
                s = isqrt(value)
                self.assertIs(type(s), int)
                self.assertLessEqual(s*s, value)
                self.assertLess(value, (s+1)*(s+1))

        # Negative values
        with self.assertRaises(ValueError):
            isqrt(-1)

        # Integer-like things
        self.assertIntEqual(isqrt(False), 0)
        self.assertIntEqual(isqrt(True), 1)
        self.assertIntEqual(isqrt(MyIndexable(1729)), 41)

        with self.assertRaises(ValueError):
            isqrt(MyIndexable(-3))

        # Non-integer-like things
        for value in 4.0, "4", Decimal("4.0"), Fraction(4, 1), 4j, -4.0:
            with self.subTest(value=value):
                with self.assertRaises(TypeError):
                    isqrt(value)

    def testPerm(self):
        perm = imath.perm
        factorial = imath.factorial
        # Test if factorial defintion is satisfied
        for n in range(100):
            for k in range(n + 1):
                self.assertEqual(perm(n, k),
                                 factorial(n) // factorial(n - k))

        # Test for Pascal's identity
        for n in range(1, 100):
            for k in range(1, n):
                self.assertEqual(perm(n, k), perm(n - 1, k - 1) * k + perm(n - 1, k))

        # Test corner cases
        for n in range(1, 100):
            self.assertEqual(perm(n, 0), 1)
            self.assertEqual(perm(n, 1), n)
            self.assertEqual(perm(n, n), factorial(n))

        # Raises TypeError if any argument is non-integer or argument count is
        # not 2
        self.assertRaises(TypeError, perm, 10, 1.0)
        self.assertRaises(TypeError, perm, 10, Decimal(1.0))
        self.assertRaises(TypeError, perm, 10, Fraction(1, 1))
        self.assertRaises(TypeError, perm, 10, "1")
        self.assertRaises(TypeError, perm, 10.0, 1)
        self.assertRaises(TypeError, perm, Decimal(10.0), 1)
        self.assertRaises(TypeError, perm, Fraction(10, 1), 1)
        self.assertRaises(TypeError, perm, "10", 1)

        self.assertRaises(TypeError, perm, 10)
        self.assertRaises(TypeError, perm, 10, 1, 3)
        self.assertRaises(TypeError, perm)

        # Raises Value error if not k or n are negative numbers
        self.assertRaises(ValueError, perm, -1, 1)
        self.assertRaises(ValueError, perm, -2**1000, 1)
        self.assertRaises(ValueError, perm, 1, -1)
        self.assertRaises(ValueError, perm, 1, -2**1000)

        # Raises value error if k is greater than n
        self.assertRaises(ValueError, perm, 1, 2)
        self.assertRaises(ValueError, perm, 1, 2**1000)

        n = 2**1000
        self.assertEqual(perm(n, 0), 1)
        self.assertEqual(perm(n, 1), n)
        self.assertEqual(perm(n, 2), n * (n-1))
        self.assertRaises((OverflowError, MemoryError), perm, n, n)

        for n, k in (True, True), (True, False), (False, False):
            self.assertIntEqual(perm(n, k), 1)
        self.assertEqual(perm(IntSubclass(5), IntSubclass(2)), 20)
        self.assertEqual(perm(MyIndexable(5), MyIndexable(2)), 20)
        for k in range(3):
            self.assertIs(type(perm(IntSubclass(5), IntSubclass(k))), int)
            self.assertIs(type(perm(MyIndexable(5), MyIndexable(k))), int)

    def testComb(self):
        comb = imath.comb
        factorial = imath.factorial
        # Test if factorial defintion is satisfied
        for n in range(100):
            for k in range(n + 1):
                self.assertEqual(comb(n, k), factorial(n)
                    // (factorial(k) * factorial(n - k)))

        # Test for Pascal's identity
        for n in range(1, 100):
            for k in range(1, n):
                self.assertEqual(comb(n, k), comb(n - 1, k - 1) + comb(n - 1, k))

        # Test corner cases
        for n in range(100):
            self.assertEqual(comb(n, 0), 1)
            self.assertEqual(comb(n, n), 1)

        for n in range(1, 100):
            self.assertEqual(comb(n, 1), n)
            self.assertEqual(comb(n, n - 1), n)

        # Test Symmetry
        for n in range(100):
            for k in range(n // 2):
                self.assertEqual(comb(n, k), comb(n, n - k))

        # Raises TypeError if any argument is non-integer or argument count is
        # not 2
        self.assertRaises(TypeError, comb, 10, 1.0)
        self.assertRaises(TypeError, comb, 10, Decimal(1.0))
        self.assertRaises(TypeError, comb, 10, Fraction(1, 1))
        self.assertRaises(TypeError, comb, 10, "1")
        self.assertRaises(TypeError, comb, 10.0, 1)
        self.assertRaises(TypeError, comb, Fraction(10, 1), 1)
        self.assertRaises(TypeError, comb, "10", 1)

        self.assertRaises(TypeError, comb, 10)
        self.assertRaises(TypeError, comb, 10, 1, 3)
        self.assertRaises(TypeError, comb)

        # Raises Value error if not k or n are negative numbers
        self.assertRaises(ValueError, comb, -1, 1)
        self.assertRaises(ValueError, comb, -2**1000, 1)
        self.assertRaises(ValueError, comb, 1, -1)
        self.assertRaises(ValueError, comb, 1, -2**1000)

        # Raises value error if k is greater than n
        self.assertRaises(ValueError, comb, 1, 2)
        self.assertRaises(ValueError, comb, 1, 2**1000)

        n = 2**1000
        self.assertEqual(comb(n, 0), 1)
        self.assertEqual(comb(n, 1), n)
        self.assertEqual(comb(n, 2), n * (n-1) // 2)
        self.assertEqual(comb(n, n), 1)
        self.assertEqual(comb(n, n-1), n)
        self.assertEqual(comb(n, n-2), n * (n-1) // 2)
        self.assertRaises((OverflowError, MemoryError), comb, n, n//2)

        for n, k in (True, True), (True, False), (False, False):
            self.assertIntEqual(comb(n, k), 1)
        self.assertIntEqual(comb(IntSubclass(5), IntSubclass(2)), 10)
        self.assertIntEqual(comb(MyIndexable(5), MyIndexable(2)), 10)
        for k in range(3):
            self.assertIs(type(comb(IntSubclass(5), IntSubclass(k))), int)
            self.assertIs(type(comb(MyIndexable(5), MyIndexable(k))), int)


if __name__ == '__main__':
    unittest.main()
