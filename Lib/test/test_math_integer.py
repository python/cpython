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

# Here's a pure Python version of the math.integer.factorial algorithm, for
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


class IntMathTests(unittest.TestCase):
    import math.integer as module

    def assertIntEqual(self, actual, expected):
        self.assertEqual(actual, expected)
        self.assertIs(type(actual), int)

    def test_factorial(self):
        factorial = self.module.factorial
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

    def test_factorial_non_integers(self):
        factorial = self.module.factorial
        self.assertRaises(TypeError, factorial, 5.0)
        self.assertRaises(TypeError, factorial, 5.2)
        self.assertRaises(TypeError, factorial, -1.0)
        self.assertRaises(TypeError, factorial, -1e100)
        self.assertRaises(TypeError, factorial, Decimal('5'))
        self.assertRaises(TypeError, factorial, Decimal('5.2'))
        self.assertRaises(TypeError, factorial, Fraction(5, 1))
        self.assertRaises(TypeError, factorial, "5")

    # Other implementations may place different upper bounds.
    @support.cpython_only
    def test_factorial_huge_inputs(self):
        factorial = self.module.factorial
        # Currently raises OverflowError for inputs that are too large
        # to fit into a C long.
        self.assertRaises(OverflowError, factorial, 10**100)
        self.assertRaises(TypeError, factorial, 1e100)

    def test_gcd(self):
        gcd = self.module.gcd
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

    def test_lcm(self):
        lcm = self.module.lcm
        self.assertEqual(lcm(0, 0), 0)
        self.assertEqual(lcm(1, 0), 0)
        self.assertEqual(lcm(-1, 0), 0)
        self.assertEqual(lcm(0, 1), 0)
        self.assertEqual(lcm(0, -1), 0)
        self.assertEqual(lcm(7, 1), 7)
        self.assertEqual(lcm(7, -1), 7)
        self.assertEqual(lcm(-23, 15), 345)
        self.assertEqual(lcm(120, 84), 840)
        self.assertEqual(lcm(84, -120), 840)
        self.assertEqual(lcm(1216342683557601535506311712,
                             436522681849110124616458784),
                             16592536571065866494401400422922201534178938447014944)

        x = 43461045657039990237
        y = 10645022458251153277
        for c in (652560,
                  57655923087165495981):
            a = x * c
            b = y * c
            d = x * y * c
            self.assertEqual(lcm(a, b), d)
            self.assertEqual(lcm(b, a), d)
            self.assertEqual(lcm(-a, b), d)
            self.assertEqual(lcm(b, -a), d)
            self.assertEqual(lcm(a, -b), d)
            self.assertEqual(lcm(-b, a), d)
            self.assertEqual(lcm(-a, -b), d)
            self.assertEqual(lcm(-b, -a), d)

        self.assertEqual(lcm(), 1)
        self.assertEqual(lcm(120), 120)
        self.assertEqual(lcm(-120), 120)
        self.assertEqual(lcm(120, 84, 102), 14280)
        self.assertEqual(lcm(120, 0, 84), 0)

        self.assertRaises(TypeError, lcm, 120.0)
        self.assertRaises(TypeError, lcm, 120.0, 84)
        self.assertRaises(TypeError, lcm, 120, 84.0)
        self.assertRaises(TypeError, lcm, 120, 0, 84.0)
        self.assertEqual(lcm(MyIndexable(120), MyIndexable(84)), 840)

    def test_isqrt(self):
        isqrt = self.module.isqrt
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
        self.assertIntEqual(isqrt(True), 1)
        self.assertIntEqual(isqrt(False), 0)
        self.assertIntEqual(isqrt(MyIndexable(1729)), 41)

        with self.assertRaises(ValueError):
            isqrt(MyIndexable(-3))

        # Non-integer-like things
        bad_values = [
            3.5, "a string", Decimal("3.5"), 3.5j,
            100.0, -4.0,
        ]
        for value in bad_values:
            with self.subTest(value=value):
                with self.assertRaises(TypeError):
                    isqrt(value)

    @support.bigmemtest(2**32, memuse=0.85)
    def test_isqrt_huge(self, size):
        isqrt = self.module.isqrt
        if size & 1:
            size += 1
        v = 1 << size
        w = isqrt(v)
        self.assertEqual(w.bit_length(), size // 2 + 1)
        self.assertEqual(w.bit_count(), 1)

    def test_perm(self):
        perm = self.module.perm
        factorial = self.module.factorial
        # Test if factorial definition is satisfied
        for n in range(500):
            for k in (range(n + 1) if n < 100 else range(30) if n < 200 else range(10)):
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

        # Test one argument form
        for n in range(20):
            self.assertEqual(perm(n), factorial(n))
            self.assertEqual(perm(n, None), factorial(n))

        # Raises TypeError if any argument is non-integer or argument count is
        # not 1 or 2
        self.assertRaises(TypeError, perm, 10, 1.0)
        self.assertRaises(TypeError, perm, 10, Decimal(1.0))
        self.assertRaises(TypeError, perm, 10, Fraction(1, 1))
        self.assertRaises(TypeError, perm, 10, "1")
        self.assertRaises(TypeError, perm, 10.0, 1)
        self.assertRaises(TypeError, perm, Decimal(10.0), 1)
        self.assertRaises(TypeError, perm, Fraction(10, 1), 1)
        self.assertRaises(TypeError, perm, "10", 1)

        self.assertRaises(TypeError, perm)
        self.assertRaises(TypeError, perm, 10, 1, 3)
        self.assertRaises(TypeError, perm)

        # Raises Value error if not k or n are negative numbers
        self.assertRaises(ValueError, perm, -1, 1)
        self.assertRaises(ValueError, perm, -2**1000, 1)
        self.assertRaises(ValueError, perm, 1, -1)
        self.assertRaises(ValueError, perm, 1, -2**1000)

        # Returns zero if k is greater than n
        self.assertEqual(perm(1, 2), 0)
        self.assertEqual(perm(1, 2**1000), 0)

        n = 2**1000
        self.assertEqual(perm(n, 0), 1)
        self.assertEqual(perm(n, 1), n)
        self.assertEqual(perm(n, 2), n * (n-1))
        if support.check_impl_detail(cpython=True):
            self.assertRaises(OverflowError, perm, n, n)

        for n, k in (True, True), (True, False), (False, False):
            self.assertIntEqual(perm(n, k), 1)
        self.assertEqual(perm(IntSubclass(5), IntSubclass(2)), 20)
        self.assertEqual(perm(MyIndexable(5), MyIndexable(2)), 20)
        for k in range(3):
            self.assertIs(type(perm(IntSubclass(5), IntSubclass(k))), int)
            self.assertIs(type(perm(MyIndexable(5), MyIndexable(k))), int)

    def test_comb(self):
        comb = self.module.comb
        factorial = self.module.factorial
        # Test if factorial definition is satisfied
        for n in range(500):
            for k in (range(n + 1) if n < 100 else range(30) if n < 200 else range(10)):
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
        self.assertRaises(TypeError, comb, 10, "1")
        self.assertRaises(TypeError, comb, 10.0, 1)
        self.assertRaises(TypeError, comb, Decimal(10.0), 1)
        self.assertRaises(TypeError, comb, "10", 1)

        self.assertRaises(TypeError, comb, 10)
        self.assertRaises(TypeError, comb, 10, 1, 3)
        self.assertRaises(TypeError, comb)

        # Raises Value error if not k or n are negative numbers
        self.assertRaises(ValueError, comb, -1, 1)
        self.assertRaises(ValueError, comb, -2**1000, 1)
        self.assertRaises(ValueError, comb, 1, -1)
        self.assertRaises(ValueError, comb, 1, -2**1000)

        # Returns zero if k is greater than n
        self.assertEqual(comb(1, 2), 0)
        self.assertEqual(comb(1, 2**1000), 0)

        n = 2**1000
        self.assertEqual(comb(n, 0), 1)
        self.assertEqual(comb(n, 1), n)
        self.assertEqual(comb(n, 2), n * (n-1) // 2)
        self.assertEqual(comb(n, n), 1)
        self.assertEqual(comb(n, n-1), n)
        self.assertEqual(comb(n, n-2), n * (n-1) // 2)
        if support.check_impl_detail(cpython=True):
            self.assertRaises(OverflowError, comb, n, n//2)

        for n, k in (True, True), (True, False), (False, False):
            self.assertIntEqual(comb(n, k), 1)
        self.assertEqual(comb(IntSubclass(5), IntSubclass(2)), 10)
        self.assertEqual(comb(MyIndexable(5), MyIndexable(2)), 10)
        for k in range(3):
            self.assertIs(type(comb(IntSubclass(5), IntSubclass(k))), int)
            self.assertIs(type(comb(MyIndexable(5), MyIndexable(k))), int)


class MathTests(IntMathTests):
    import math as module


class MiscTests(unittest.TestCase):

    def test_module_name(self):
        import math.integer
        self.assertEqual(math.integer.__name__, 'math.integer')
        for name in dir(math.integer):
            if not name.startswith('_'):
                obj = getattr(math.integer, name)
                self.assertEqual(obj.__module__, 'math.integer')


if __name__ == '__main__':
    unittest.main()
