"""Unit tests for numbers.py."""

import math
import operator
import unittest
from numbers import Complex, Real, Rational, Integral, Number

class TestNumbers(unittest.TestCase):
    def test_int(self):
        self.assertTrue(issubclass(int, Integral))
        self.assertTrue(issubclass(int, Complex))
        self.assertTrue(issubclass(int, Number))

        self.assertEqual(7, int(7).real)
        self.assertEqual(0, int(7).imag)
        self.assertEqual(7, int(7).conjugate())
        self.assertEqual(-7, int(-7).conjugate())
        self.assertEqual(7, int(7).numerator)
        self.assertEqual(1, int(7).denominator)

    def test_float(self):
        self.assertFalse(issubclass(float, Rational))
        self.assertTrue(issubclass(float, Real))
        self.assertTrue(issubclass(float, Number))

        self.assertEqual(7.3, float(7.3).real)
        self.assertEqual(0, float(7.3).imag)
        self.assertEqual(7.3, float(7.3).conjugate())
        self.assertEqual(-7.3, float(-7.3).conjugate())

    def test_complex(self):
        self.assertFalse(issubclass(complex, Real))
        self.assertTrue(issubclass(complex, Complex))
        self.assertTrue(issubclass(complex, Number))

        c1, c2 = complex(3, 2), complex(4,1)
        # XXX: This is not ideal, but see the comment in math_trunc().
        self.assertRaises(TypeError, math.trunc, c1)
        self.assertRaises(TypeError, operator.mod, c1, c2)
        self.assertRaises(TypeError, divmod, c1, c2)
        self.assertRaises(TypeError, operator.floordiv, c1, c2)
        self.assertRaises(TypeError, float, c1)
        self.assertRaises(TypeError, int, c1)


class TestSubclassNumbers(unittest.TestCase):
    def test_subclass_complex(self):
        class MyComplex(Complex):
            def __init__(self, real, imag):
                self.r = real
                self.i = imag
            
            def __complex__(self):
                pass

            @property
            def real(self):
                return self.r
            
            @property
            def imag(self):
                return self.i
            
            def __add__(self, other):
                if isinstance(other, Complex):
                    return MyComplex(self.imag + other.imag,
                                     self.real + other.real)
                if isinstance(other, Number):
                    return MyComplex(self.imag + 0, self.real + other.real)

            def __radd__(self, other):
                pass

            def __neg__(self):
                return MyComplex(-self.real, -self.imag)

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

            def __eq__(self, other):
                if isinstance(other, Complex):
                    return self.imag == other.imag and self.real == other.real
                if isinstance(other, Number):
                    return self.imag == 0 and self.real == other.real

        # test __bool__
        self.assertTrue(bool(MyComplex(1, 1)))
        self.assertTrue(bool(MyComplex(0, 1)))
        self.assertTrue(bool(MyComplex(1, 0)))
        self.assertFalse(bool(MyComplex(0, 0)))

        # test __sub__
        self.assertEqual(MyComplex(2, 3) - complex(1, 2), MyComplex(1, 1))

        # test __rsub__
        self.assertEqual(complex(2, 3) - MyComplex(1, 2), MyComplex(1, 1))


if __name__ == "__main__":
    unittest.main()
