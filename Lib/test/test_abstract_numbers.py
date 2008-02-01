"""Unit tests for numbers.py."""

import math
import operator
import unittest
from numbers import Complex, Real, Rational, Integral
from numbers import Exact, Inexact
from numbers import Number
from test import test_support

class TestNumbers(unittest.TestCase):
    def test_int(self):
        self.failUnless(issubclass(int, Integral))
        self.failUnless(issubclass(int, Complex))
        self.failUnless(issubclass(int, Exact))
        self.failIf(issubclass(int, Inexact))

        self.assertEqual(7, int(7).real)
        self.assertEqual(0, int(7).imag)
        self.assertEqual(7, int(7).conjugate())
        self.assertEqual(7, int(7).numerator)
        self.assertEqual(1, int(7).denominator)

    def test_long(self):
        self.failUnless(issubclass(long, Integral))
        self.failUnless(issubclass(long, Complex))
        self.failUnless(issubclass(long, Exact))
        self.failIf(issubclass(long, Inexact))

        self.assertEqual(7, long(7).real)
        self.assertEqual(0, long(7).imag)
        self.assertEqual(7, long(7).conjugate())
        self.assertEqual(7, long(7).numerator)
        self.assertEqual(1, long(7).denominator)

    def test_float(self):
        self.failIf(issubclass(float, Rational))
        self.failUnless(issubclass(float, Real))
        self.failIf(issubclass(float, Exact))
        self.failUnless(issubclass(float, Inexact))

        self.assertEqual(7.3, float(7.3).real)
        self.assertEqual(0, float(7.3).imag)
        self.assertEqual(7.3, float(7.3).conjugate())

    def test_complex(self):
        self.failIf(issubclass(complex, Real))
        self.failUnless(issubclass(complex, Complex))
        self.failIf(issubclass(complex, Exact))
        self.failUnless(issubclass(complex, Inexact))

        c1, c2 = complex(3, 2), complex(4,1)
        # XXX: This is not ideal, but see the comment in math_trunc().
        self.assertRaises(AttributeError, math.trunc, c1)
        self.assertRaises(TypeError, float, c1)
        self.assertRaises(TypeError, int, c1)

def test_main():
    test_support.run_unittest(TestNumbers)


if __name__ == "__main__":
    unittest.main()
