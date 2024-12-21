import unittest
import time

class TestIsPnz(unittest.TestCase):

    def _time_execution(self, test_func):
        start_time = time.time()
        result = test_func()
        end_time = time.time()
        print(f"{test_func.__name__} took {end_time - start_time:.6f} seconds.")
        return result

    # Test positive numbers
    def test_positive_float(self):
        self._time_execution(lambda: self.assertEqual(isPnz(43534.43534534), True))
        self._time_execution(lambda: self.assertEqual(isPnz(0.143234234), True))

    def test_positive_int(self):
        self._time_execution(lambda: self.assertEqual(isPnz(1), True))
        self._time_execution(lambda: self.assertEqual(isPnz(999999999999), True))

    # Test negative numbers
    def test_negative_float(self):
        self._time_execution(lambda: self.assertEqual(isPnz(-43534.43534534), False))
        self._time_execution(lambda: self.assertEqual(isPnz(-0.23424), False))
        self._time_execution(lambda: self.assertEqual(isPnz(-23.3342354636), False))

    def test_negative_int(self):
        self._time_execution(lambda: self.assertEqual(isPnz(-1), False))
        self._time_execution(lambda: self.assertEqual(isPnz(-999999999999), False))

    # Test zeros
    def test_positive_zero(self):
        self._time_execution(lambda: self.assertIs(isPnz(0.0), None))  # Positive zero with float

    def test_negative_zero(self):
        self._time_execution(lambda: self.assertEqual(isPnz(-0.0), False))  # Negative zero

    # Test positive and negative non-zero floats
    def test_positive_float_non_zero(self):
        self._time_execution(lambda: self.assertEqual(isPnz(0.03243423), True))  # Positive float

    def test_negative_float_non_zero(self):
        self._time_execution(lambda: self.assertEqual(isPnz(-0.03243423), False))  # Negative float

    # Test edge cases
    def test_very_small_positive(self):
        self._time_execution(lambda: self.assertEqual(isPnz(1e-10), True))

    def test_very_small_negative(self):
        self._time_execution(lambda: self.assertEqual(isPnz(-1e-10), False))

    def test_very_large_positive(self):
        self._time_execution(lambda: self.assertEqual(isPnz(1e+10), True))

    def test_very_large_negative(self):
        self._time_execution(lambda: self.assertEqual(isPnz(-1e+50), False))

if __name__ == '__main__':
    unittest.main()
