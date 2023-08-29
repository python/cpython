"""Unit tests for numbers.py."""

import math
import operator
import unittest
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

        match 1:
            case Real(f):
                self.assertIsInstance(f, int)
                self.assertEqual(f, 1)
            case _:
                self.fail("int is not matched as Real")
        match 1:
            case Rational(f):
                self.assertIsInstance(f, int)
                self.assertEqual(f, 1)
            case _:
                self.fail("int is not matched as Rational")

    def test_float(self):
        self.assertFalse(issubclass(float, Rational))
        self.assertTrue(issubclass(float, Real))

        self.assertEqual(7.3, float(7.3).real)
        self.assertEqual(0, float(7.3).imag)
        self.assertEqual(7.3, float(7.3).conjugate())
        self.assertEqual(-7.3, float(-7.3).conjugate())

        match 1.0:
            case Real(f):
                self.assertEqual(f, 1.0)
            case _:
                self.fail("float is not matched as Real")

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

        match 1j:
            case Complex(real, imag):
                self.assertEqual(real, 0.0)
                self.assertEqual(imag, 1.0)
            case _:
                self.fail(f"complex is not matched as Complex")


if __name__ == "__main__":
    unittest.main()
