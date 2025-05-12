import unittest
import math
import cmath

class TestMathErrors(unittest.TestCase):
    def test_math_value_error_with_value(self):
        # Test math.sqrt with negative number
        try:
            math.sqrt(-1)
        except ValueError as e:
            self.assertTrue(hasattr(e, 'value'))
            self.assertTrue(math.isnan(e.value))

        # Test math.log with negative number
        try:
            math.log(-1)
        except ValueError as e:
            self.assertTrue(hasattr(e, 'value'))
            self.assertTrue(math.isnan(e.value))

        # Test math.acos with value > 1
        try:
            math.acos(2)
        except ValueError as e:
            self.assertTrue(hasattr(e, 'value'))
            self.assertTrue(math.isnan(e.value))

    def test_cmath_value_error_with_value(self):
        # Test cmath.sqrt with negative number
        try:
            cmath.sqrt(-1)
        except ValueError as e:
            self.assertTrue(hasattr(e, 'value'))
            self.assertTrue(isinstance(e.value, complex))
            self.assertTrue(cmath.isnan(e.value))

        # Test cmath.log with negative number
        try:
            cmath.log(-1)
        except ValueError as e:
            self.assertTrue(hasattr(e, 'value'))
            self.assertTrue(isinstance(e.value, complex))
            self.assertTrue(cmath.isnan(e.value))

        # Test cmath.acos with value > 1
        try:
            cmath.acos(2)
        except ValueError as e:
            self.assertTrue(hasattr(e, 'value'))
            self.assertTrue(isinstance(e.value, complex))
            self.assertTrue(cmath.isnan(e.value))

    def test_math_overflow_error_with_value(self):
        # Test math.exp with very large number
        try:
            math.exp(1000)
        except OverflowError as e:
            self.assertTrue(hasattr(e, 'value'))
            self.assertTrue(math.isinf(e.value))

    def test_cmath_overflow_error_with_value(self):
        # Test cmath.exp with very large number
        try:
            cmath.exp(1000)
        except OverflowError as e:
            self.assertTrue(hasattr(e, 'value'))
            self.assertTrue(isinstance(e.value, complex))
            self.assertTrue(cmath.isinf(e.value.real) or cmath.isinf(e.value.imag))

if __name__ == '__main__':
    unittest.main() 