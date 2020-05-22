import unittest
import imath
import math


class TestPrimes(unittest.TestCase):
    small_primes = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41,
                    43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97)

    def test_is_prime(self):
        primes = list(filter(imath.is_prime, range(100)))
        self.assertSequenceEqual(primes, self.small_primes)
        self.assertFalse(imath.is_prime(2**800+1))
        self.assertFalse(imath.is_prime(2**900+1))
        self.assertFalse(imath.is_prime(2**1000+1))
        self.assertFalse(imath.is_prime(2**(2**8)+1))

    def test_next_prime(self):
        self.assertEqual(imath.next_prime(-1), 2)
        for start, end in zip(self.small_primes, self.small_primes[1:]):
            for n in range(start, end):
                self.assertEqual(imath.next_prime(n), end)


    def test_previous_prime(self):
        for start, end in zip(self.small_primes, self.small_primes[1:]):
            for n in range(start+1, end+1):
                self.assertEqual(imath.previous_prime(n), start)

        for n in (-1, 2):
            with self.assertRaisesRegex(ValueError, 'no prime number'):
                imath.previous_prime(n)

    def test_factorize(self):
        with self.assertRaisesRegex(ValueError, 'Cannot factorise 0.'):
            next(imath.factorise(0))

        # Getting a first factor is fast
        n = 2**800+1
        d = next(imath.factorise(n))
        self.assertEqual(n%d, 0)

        for n in range(1, 100):
            with self.subTest(n=n):
                factors = list(imath.factorise(n))
                self.assertEqual(math.prod(factors), n)
                self.assertTrue(all(map(imath.is_prime, factors)))

                factors = list(imath.factorise(-n))
                self.assertEqual(math.prod(factors), -n)
                self.assertEqual(factors[0], -1)
                self.assertTrue(all(map(imath.is_prime, factors[1:])))

        # Numbers with many factors can be factorised
        self.assertEqual(set(imath.factorise(2**10000)), {2})


def load_tests(loader, tests, pattern):
    from doctest import DocTestSuite
    tests.addTest(DocTestSuite(imath))
    return tests
