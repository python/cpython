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


class TestNumbersDefaultMethods(unittest.TestCase):
    def test_complex(self):
        class MyComplex(Complex):
            def __init__(self, real, imag):
                self.r = real
                self.i = imag

            def __complex__(self):
                raise NotImplementedError

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
                raise NotImplementedError

            def __neg__(self):
                return MyComplex(-self.real, -self.imag)

            def __pos__(self):
                raise NotImplementedError

            def __mul__(self, other):
                raise NotImplementedError

            def __rmul__(self, other):
                raise NotImplementedError

            def __truediv__(self, other):
                raise NotImplementedError

            def __rtruediv__(self, other):
                raise NotImplementedError

            def __pow__(self, exponent):
                raise NotImplementedError

            def __rpow__(self, base):
                raise NotImplementedError

            def __abs__(self):
                raise NotImplementedError

            def conjugate(self):
                raise NotImplementedError

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

    def test_real(self):
        class MyReal(Real):
            def __init__(self, n):
                self.n = n

            def __add__(self, other):
                raise NotImplementedError

            def __radd__(self, other):
                raise NotImplementedError

            def __neg__(self):
                raise NotImplementedError

            def __pos__(self):
                return self.n

            def __mul__(self, other):
                raise NotImplementedError

            def __rmul__(self, other):
                raise NotImplementedError

            def __truediv__(self, other):
                raise NotImplementedError

            def __rtruediv__(self, other):
                raise NotImplementedError

            def __pow__(self, exponent):
                raise NotImplementedError

            def __rpow__(self, base):
                raise NotImplementedError

            def __abs__(self):
                raise NotImplementedError

            def __eq__(self, other):
                raise NotImplementedError

            def __float__(self):
                return float(self.n)

            def __trunc__(self):
                raise NotImplementedError

            def __floor__(self):
                raise NotImplementedError

            def __ceil__(self):
                raise NotImplementedError

            def __round__(self, ndigits=None):
                raise NotImplementedError

            def __floordiv__(self, other):
                return self.n // other

            def __rfloordiv__(self, other):
                return other // self.n

            def __mod__(self, other):
                return self.n % other

            def __rmod__(self, other):
                return other % self.n

            def __lt__(self, other):
                raise NotImplementedError

            def __le__(self, other):
                raise NotImplementedError

            # test conjugate

        # test __divmod__
        self.assertEqual(divmod(MyReal(3), 2), (1, 1))

        # test __rdivmod__
        self.assertEqual(divmod(3, MyReal(2)), (1, 1))

        # test __complex__
        self.assertEqual(complex(MyReal(1)).imag, 0j)

        # test real
        self.assertEqual(MyReal(3).real, 3)

        # test imag
        self.assertEqual(MyReal(3).imag, 0)

        # test conjugate
        self.assertEqual(MyReal(123).conjugate(), 123)


    def test_rational(self):
        class MyRational(Rational):
            def __init__(self, numerator, denominator):
                self.n = numerator
                self.d = denominator

            def __add__(self, other):
                raise NotImplementedError

            def __radd__(self, other):
                raise NotImplementedError

            def __neg__(self):
                raise NotImplementedError

            def __pos__(self):
                raise NotImplementedError

            def __mul__(self, other):
                raise NotImplementedError

            def __rmul__(self, other):
                raise NotImplementedError

            def __truediv__(self, other):
                raise NotImplementedError

            def __rtruediv__(self, other):
                raise NotImplementedError

            def __pow__(self, exponent):
                raise NotImplementedError

            def __rpow__(self, base):
                raise NotImplementedError

            def __abs__(self):
                raise NotImplementedError

            def __eq__(self, other):
                raise NotImplementedError

            def __trunc__(self):
                raise NotImplementedError

            def __floor__(self):
                raise NotImplementedError

            def __ceil__(self):
                raise NotImplementedError

            def __round__(self, ndigits=None):
                raise NotImplementedError

            def __floordiv__(self, other):
                raise NotImplementedError

            def __rfloordiv__(self, other):
                raise NotImplementedError

            def __mod__(self, other):
                raise NotImplementedError

            def __rmod__(self, other):
                raise NotImplementedError

            def __lt__(self, other):
                raise NotImplementedError

            def __le__(self, other):
                raise NotImplementedError

            @property
            def numerator(self):
                return self.n

            @property
            def denominator(self):
                return self.d

        # test__float__
        self.assertEqual(float(MyRational(9, 3)), 3)


    def test_integral(self):
        class MyIntegral(Integral):
            def __init__(self, n):
                self.n = n

            def __add__(self, other):
                raise NotImplementedError

            def __radd__(self, other):
                raise NotImplementedError

            def __neg__(self):
                raise NotImplementedError

            def __pos__(self):
                return self.n

            def __mul__(self, other):
                raise NotImplementedError

            def __rmul__(self, other):
                raise NotImplementedError

            def __truediv__(self, other):
                raise NotImplementedError

            def __rtruediv__(self, other):
                raise NotImplementedError

            def __rpow__(self, base):
                raise NotImplementedError

            def __abs__(self):
                raise NotImplementedError

            def __eq__(self, other):
                raise NotImplementedError

            def __trunc__(self):
                raise NotImplementedError

            def __floor__(self):
                raise NotImplementedError

            def __ceil__(self):
                raise NotImplementedError

            def __round__(self, ndigits=None):
                raise NotImplementedError

            def __floordiv__(self, other):
                raise NotImplementedError

            def __rfloordiv__(self, other):
                raise NotImplementedError

            def __mod__(self, other):
                raise NotImplementedError

            def __rmod__(self, other):
                raise NotImplementedError

            def __lt__(self, other):
                raise NotImplementedError

            def __le__(self, other):
                raise NotImplementedError

            def __int__(self):
                return self.n

            def __pow__(self, exponent, modulus=None):
                raise NotImplementedError

            def __lshift__(self, other):
                raise NotImplementedError

            def __rlshift__(self, other):
                raise NotImplementedError

            def __rshift__(self, other):
                raise NotImplementedError

            def __rrshift__(self, other):
                raise NotImplementedError

            def __and__(self, other):
                raise NotImplementedError

            def __rand__(self, other):
                raise NotImplementedError

            def __xor__(self, other):
                raise NotImplementedError

            def __rxor__(self, other):
                raise NotImplementedError

            def __or__(self, other):
                raise NotImplementedError

            def __ror__(self, other):
                raise NotImplementedError

            def __invert__(self):
                raise NotImplementedError

        # test __index__
        self.assertEqual(operator.index(MyIntegral(123)), 123)

        # test __float__
        self.assertEqual(float(MyIntegral(123)), 123.0)

        # test numerator
        self.assertEqual(MyIntegral(123).numerator, 123)

        # test denominator
        self.assertEqual(MyIntegral(123).denominator, 1)


if __name__ == "__main__":
    unittest.main()
