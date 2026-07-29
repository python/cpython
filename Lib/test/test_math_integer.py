from decimal import Decimal
from fractions import Fraction
import itertools
import random
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

# int subclass with broken arithmetic operators; implementations must
# convert their arguments to exact ints instead of using these.
class BadIntSubclass(int):
    def _binop(self, other='ignored', mod=None):
        return 42
    __add__ = __radd__ = __sub__ = __rsub__ = _binop
    __mul__ = __rmul__ = __mod__ = __rmod__ = _binop
    __divmod__ = __rdivmod__ = __pow__ = __rpow__ = _binop
    __floordiv__ = __rfloordiv__ = _binop
    __lshift__ = __rlshift__ = __rshift__ = __rrshift__ = _binop
    __and__ = __rand__ = __or__ = __ror__ = __xor__ = __rxor__ = _binop
    __lt__ = __le__ = __gt__ = __ge__ = _binop

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

# Reference implementations for primality testing.

def primes_below(n):
    """List of the primes below n, by the sieve of Eratosthenes."""
    sieve = bytearray([1]) * n
    sieve[:2] = bytes(2)
    i = 2
    while i * i < n:
        if sieve[i]:
            sieve[i*i::i] = bytes(len(range(i*i, n, i)))
        i += 1
    return [i for i in range(n) if sieve[i]]

# The Miller-Rabin test with the first 13 primes as bases is known to be
# exact for n < 3.3 * 10**24.  More bases are used for larger inputs, for
# which the test is probabilistic (but independent of the Baillie-PSW test
# used in the implementation).
MILLER_RABIN_BASES = primes_below(100)

def py_isprime(n):
    """Miller-Rabin primality test, for cross-checking."""
    if n < 2:
        return False
    for p in MILLER_RABIN_BASES:
        if n % p == 0:
            return n == p
    d = n - 1
    s = (d & -d).bit_length() - 1
    d >>= s
    for a in MILLER_RABIN_BASES:
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


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

        # Overridden operators of an int subclass must not affect the
        # result.
        self.assertIntEqual(isqrt(BadIntSubclass(10**20)), 10**10)
        self.assertIntEqual(isqrt(BadIntSubclass(10**20 - 1)), 10**10 - 1)

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


# isprime() and primes() exist only in math.integer, not in math, so their
# tests are not in IntMathTests (which is re-run against math above).

class IsPrimeTests(unittest.TestCase):
    import math.integer as module

    def test_isprime_small(self):
        isprime = self.module.isprime
        sieve = set(primes_below(10**4))
        for n in range(-10, 10**4):
            with self.subTest(n=n):
                self.assertIs(isprime(n), n in sieve)

    def test_isprime_negative(self):
        isprime = self.module.isprime
        self.assertIs(isprime(-1), False)
        self.assertIs(isprime(-2), False)
        self.assertIs(isprime(-3), False)
        self.assertIs(isprime(-10**100), False)

    def test_isprime_carmichael(self):
        # Carmichael numbers are composite (A002997).
        isprime = self.module.isprime
        for n in [561, 1105, 1729, 2465, 2821, 6601, 8911, 10585, 15841,
                  29341, 41041, 46657, 52633, 62745, 63973, 75361]:
            with self.subTest(n=n):
                self.assertIs(isprime(n), False)

    def test_isprime_strong_pseudoprimes_base_2(self):
        # Composites that pass the strong probable prime test to base 2
        # (A001262); they must be caught by the strong Lucas test.
        isprime = self.module.isprime
        for n in [2047, 3277, 4033, 4681, 8321, 15841, 29341, 42799, 49141,
                  52633, 65281, 74665, 80581, 85489, 88357, 90751,
                  3825123056546413051]:
            with self.subTest(n=n):
                self.assertIs(isprime(n), False)

    def test_isprime_strong_lucas_pseudoprimes(self):
        # Composites that pass the strong Lucas test with Selfridge's
        # parameters (A217255).
        isprime = self.module.isprime
        for n in [5459, 5777, 10877, 16109, 18971, 22499, 24569, 25199,
                  40309, 58519, 75077, 97439]:
            with self.subTest(n=n):
                self.assertIs(isprime(n), False)

    def test_isprime_base_divisors(self):
        # Divisors of the Miller-Rabin bases exercise the case where a
        # base is divisible by the tested number.
        isprime = self.module.isprime
        for base in [2, 7, 61, 325, 9375, 28178, 450775, 9780504,
                     1795265022]:
            d = 1
            while d * d <= base:
                if base % d == 0:
                    for n in [d, base // d]:
                        with self.subTest(base=base, n=n):
                            self.assertIs(isprime(n), py_isprime(n))
                d += 1

    def test_isprime_perfect_squares(self):
        isprime = self.module.isprime
        for k in range(1000):
            with self.subTest(k=k):
                self.assertIs(isprime(k*k), False)
        for k in [2**31 - 1, 2**32 - 5]:
            with self.subTest(k=k):
                self.assertIs(isprime(k*k), False)

    def test_isprime_word_boundaries(self):
        # Exercise the boundaries between the base sets.
        isprime = self.module.isprime
        for boundary in [2**32, 4759123141]:
            for n in range(boundary - 200, boundary + 200):
                with self.subTest(n=n):
                    self.assertIs(isprime(n), py_isprime(n))
        for n in range(2**64 - 200, 2**64):
            with self.subTest(n=n):
                self.assertIs(isprime(n), py_isprime(n))
        self.assertIs(isprime(2**64 - 59), True)   # largest prime < 2**64

    def test_isprime_large_values(self):
        isprime = self.module.isprime
        self.assertIs(isprime(2**61 - 1), True)    # Mersenne prime
        self.assertIs(isprime(2**62 - 1), False)
        self.assertIs(isprime(10**19), False)
        # Arguments not less than 2**64 are not supported.
        for n in [2**64, 2**64 + 13, 2**89 - 1, 10**100]:
            with self.subTest(n=n):
                self.assertRaises(OverflowError, isprime, n)

    def test_isprime_random(self):
        isprime = self.module.isprime
        rng = random.Random(1729)
        for bits in [32, 34, 63, 64]:
            for _ in range(300):
                n = rng.getrandbits(bits)
                with self.subTest(n=n):
                    self.assertIs(isprime(n), py_isprime(n))
        for bits in [65, 80, 128]:
            n = rng.getrandbits(bits) | (1 << (bits - 1))
            with self.subTest(n=n):
                self.assertRaises(OverflowError, isprime, n)

    def test_isprime_integer_like(self):
        isprime = self.module.isprime
        self.assertIs(isprime(False), False)
        self.assertIs(isprime(True), False)
        self.assertIs(isprime(IntSubclass(7)), True)
        self.assertIs(isprime(IntSubclass(8)), False)
        self.assertIs(isprime(MyIndexable(97)), True)
        self.assertIs(isprime(MyIndexable(-97)), False)

    def test_isprime_int_subclass_operators(self):
        # Overridden operators of an int subclass must not affect the
        # result.
        isprime = self.module.isprime
        self.assertIs(isprime(BadIntSubclass(97)), True)
        self.assertIs(isprime(BadIntSubclass(2**61 - 1)), True)
        self.assertIs(isprime(BadIntSubclass(2**62 - 1)), False)

    def test_isprime_non_integers(self):
        isprime = self.module.isprime
        for value in [7.0, 7.5, Decimal('7'), Fraction(7, 1), '7', 7.5j]:
            with self.subTest(value=value):
                self.assertRaises(TypeError, isprime, value)
        self.assertRaises(TypeError, isprime)
        self.assertRaises(TypeError, isprime, 7, 11)


class PrimesIterTests(unittest.TestCase):
    import math.integer as module

    def test_primes(self):
        primes = self.module.primes
        expected = primes_below(10**4)
        self.assertEqual(list(primes(stop=10**4)), expected)
        self.assertEqual(len(expected), 1229)
        self.assertEqual(list(itertools.islice(primes(), 25)), expected[:25])

    def test_primes_start(self):
        primes = self.module.primes
        for start in [-10**100, -100, -1, 0, 1, 2]:
            with self.subTest(start=start):
                self.assertEqual(list(primes(start, 10)), [2, 3, 5, 7])
        self.assertEqual(list(primes(3, 10)), [3, 5, 7])
        self.assertEqual(list(primes(4, 10)), [5, 7])
        self.assertEqual(list(primes(8, 12)), [11])
        self.assertEqual(list(primes(9, 12)), [11])
        self.assertEqual(list(primes(7, 8)), [7])

    def test_primes_stop(self):
        primes = self.module.primes
        # The range is half-open.
        self.assertEqual(list(primes(2, 2)), [])
        self.assertEqual(list(primes(2, 3)), [2])
        self.assertEqual(list(primes(3, 3)), [])
        self.assertEqual(list(primes(7, 7)), [])
        self.assertEqual(list(primes(2, -10)), [])
        self.assertEqual(list(primes(10, 5)), [])
        self.assertEqual(list(primes(stop=0)), [])

    def test_primes_unbounded(self):
        primes = self.module.primes
        it = primes()
        self.assertEqual([next(it) for _ in range(5)], [2, 3, 5, 7, 11])
        it = primes(10**6)
        self.assertEqual(next(it), 1000003)

    def test_primes_huge(self):
        primes = self.module.primes
        boundary = 10**18
        expected = [n for n in range(boundary - 200, boundary + 200)
                    if py_isprime(n)]
        self.assertEqual(list(primes(boundary - 200, boundary + 200)),
                         expected)
        # The top of the supported range.
        expected = [n for n in range(2**64 - 200, 2**64) if py_isprime(n)]
        self.assertEqual(list(primes(2**64 - 200, 2**64 - 1)), expected)

    def test_primes_overflow(self):
        primes = self.module.primes
        # The bounds must be less than 2**64.
        self.assertRaises(OverflowError, primes, 2**64)
        self.assertRaises(OverflowError, primes, 2**64 + 100, 2**64 + 200)
        self.assertRaises(OverflowError, primes, 0, 2**64)
        self.assertRaises(OverflowError, primes, 10**100)
        # An unbounded iterator raises when it runs out of the
        # supported range.
        it = primes(2**64 - 60)
        self.assertEqual(next(it), 2**64 - 59)
        self.assertRaises(OverflowError, next, it)
        self.assertRaises(StopIteration, next, it)

    def test_primes_iterator_protocol(self):
        primes = self.module.primes
        it = primes(2, 10)
        self.assertIs(iter(it), it)
        self.assertEqual(list(it), [2, 3, 5, 7])
        # An exhausted iterator stays exhausted.
        self.assertEqual(list(it), [])
        self.assertRaises(StopIteration, next, it)
        # The iterator type cannot be instantiated directly.
        self.assertRaises(TypeError, type(primes()))

    def test_primes_keywords(self):
        primes = self.module.primes
        self.assertEqual(list(primes(start=10, stop=30)), [11, 13, 17, 19, 23, 29])
        self.assertEqual(list(primes(10, stop=30)), [11, 13, 17, 19, 23, 29])

    def test_primes_integer_like(self):
        primes = self.module.primes
        self.assertEqual(list(primes(True, 10)), [2, 3, 5, 7])
        self.assertEqual(list(primes(IntSubclass(3), IntSubclass(10))), [3, 5, 7])
        self.assertEqual(list(primes(MyIndexable(3), MyIndexable(10))), [3, 5, 7])
        # The yielded values are exact ints.
        for p in primes(IntSubclass(3), IntSubclass(10)):
            self.assertIs(type(p), int)

    def test_primes_int_subclass_operators(self):
        # Overridden operators of an int subclass must not affect the
        # iteration.
        primes = self.module.primes
        self.assertEqual(list(primes(BadIntSubclass(3), BadIntSubclass(10))),
                         [3, 5, 7])
        big = 10**18
        self.assertEqual(list(primes(BadIntSubclass(big), big + 100)),
                         [n for n in range(big, big + 100) if py_isprime(n)])

    def test_primes_non_integers(self):
        primes = self.module.primes
        self.assertRaises(TypeError, primes, 2.5)
        self.assertRaises(TypeError, primes, 2.5, 10)
        self.assertRaises(TypeError, primes, 2, 10.5)
        self.assertRaises(TypeError, primes, '2')
        self.assertRaises(TypeError, primes, 2, '10')
        self.assertRaises(TypeError, primes, 2, 10, 3)


class MiscTests(unittest.TestCase):

    def test_module_name(self):
        import math.integer
        self.assertEqual(math.integer.__name__, 'math.integer')
        for name in dir(math.integer):
            if not name.startswith('_'):
                obj = getattr(math.integer, name)
                self.assertEqual(obj.__module__, 'math.integer')

    def test_math_namespace(self):
        # New functions are added only to math.integer, not to math
        # (PEP 791).
        import math
        self.assertFalse(hasattr(math, 'isprime'))
        self.assertFalse(hasattr(math, 'primes'))


if __name__ == '__main__':
    unittest.main()
