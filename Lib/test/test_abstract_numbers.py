"""Unit tests for numbers.py."""

import math
import operator
import unittest
from random import random
from numbers import Complex, Real, Rational, Integral

class TestNumbers(unittest.TestCase):
    def test_int(self):
        self.assertTrue(issubclass(int, Integral))
        self.assertTrue(issubclass(int, Complex))

        self.assertEqual(7, int(7).real)
        self.assertEqual(0, int(7).imag)
        self.assertEqual(7, int(7).conjugate())
        self.assertEqual(-7, int(-7).conjugate())
        self.assertEqual(7, int(7).numerator)
        self.assertEqual(1, int(7).denominator)

    def test_float(self):
        self.assertFalse(issubclass(float, Rational))
        self.assertTrue(issubclass(float, Real))

        self.assertEqual(7.3, float(7.3).real)
        self.assertEqual(0, float(7.3).imag)
        self.assertEqual(7.3, float(7.3).conjugate())
        self.assertEqual(-7.3, float(-7.3).conjugate())

    def test_complex(self):
        self.assertFalse(issubclass(complex, Real))
        self.assertTrue(issubclass(complex, Complex))

        c1, c2 = complex(3, 2), complex(4,1)
        # XXX: This is not ideal, but see the comment in math_trunc().
        self.assertRaises(TypeError, math.trunc, c1)
        self.assertRaises(TypeError, operator.mod, c1, c2)
        self.assertRaises(TypeError, divmod, c1, c2)
        self.assertRaises(TypeError, operator.floordiv, c1, c2)
        self.assertRaises(TypeError, float, c1)
        self.assertRaises(TypeError, int, c1)


class TestAbstractNumbers(unittest.TestCase):
    def testComplex(self):

        class SubComplex(Complex):
            def __complex__(self):
                pass

            @property
            def real(self):
                pass

            @property
            def imag(self):
                pass

            def __add__(self, other):
                pass

            def __radd__(self, other):
                pass

            def __neg__(self):
                pass

            def __pos__(self):
                pass

            def __mul__(self, other):
                pass

            def __rmul__(self, other):
                pass

            def __truediv__(self, other):
                pass

            def __rtruediv__(self, other):
                pass

            def __pow__(self, exponent):
                pass

            def __rpow__(self, base):
                pass

            def __abs__(self):
                pass

            def conjugate(self):
                pass

            def __init__(self):
                pass

            def __eq__(self, other):
                return isinstance(other, SubComplex) and self.imag == other.imag and self.real == other.real

        sc = SubComplex()
        self.assertIsInstance(sc, SubComplex)

        nonInstances = [None, "hat", lambda x: x + 1]

        for nonInstance in nonInstances:
            self.assertNotIsInstance(nonInstance, Complex)

        self.assertFalse(SubComplex.__bool__(0))
        for i in range(100):
            self.assertTrue(SubComplex.__bool__(random() + 1e-6))
            x = random()
            y = random()
            self.assertEqual(SubComplex.__sub__(x, y), x - y)
            self.assertEqual(SubComplex.__rsub__(x, y), - x + y)


if __name__ == "__main__":
    unittest.main()
