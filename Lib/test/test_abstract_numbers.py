"""Unit tests for numbers.py."""

import math
import operator
import unittest
from numbers import Complex, Real, Rational, Integral


class DummyIntegral(Integral):
    """Dummy Integral class to test default implementations of methods."""

    def __init__(self, val=0):
        self._val = int(val)

    def __int__(self):
        return self._val

    def __pow__(self, exponent, modulus=None):
        return NotImplemented
    __rpow__ = __pow__

    def __lshift__(self, other):
        return NotImplemented
    __rlshift__ = __lshift__
    __rshift__ = __lshift__
    __rrshift__ = __lshift__
    __and__ = __lshift__
    __rand__ = __lshift__
    __xor__ = __lshift__
    __rxor__ = __lshift__
    __or__ = __lshift__
    __ror__ = __lshift__
    __add__ = __lshift__
    __radd__ = __lshift__
    __mul__ = __lshift__
    __rmul__ = __lshift__
    __mod__ = __lshift__
    __rmod__ = __lshift__
    __floordiv__ = __lshift__
    __rfloordiv__ = __lshift__
    __truediv__ = __lshift__
    __rtruediv__ = __lshift__
    __lt__ = __lshift__
    __le__ = __lshift__

    def __eq__(self, other):
        return all(isinstance(_, DummyIntegral)
                   for _ in [self, other]) and self._val == other._val

    def __invert__(self):
        return NotImplemented
    __abs__ = __invert__
    __neg__ = __invert__
    __trunc__ = __invert__
    __ceil__ = __invert__
    __floor__ = __invert__
    __round__ = __invert__

    def __pos__(self):
        return self


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

    def test_DummyIntegral(self):
        seven = DummyIntegral(7)
        one = DummyIntegral(1)
        self.assertEqual(float(seven), 7.0)
        self.assertEqual(float(one), 1.0)
        self.assertEqual(seven.numerator, seven)
        self.assertEqual(seven.denominator, one)

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


if __name__ == "__main__":
    unittest.main()
