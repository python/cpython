"""Test suite for statistics module, including helper NumericTestCase and
approx_equal function.

"""

import bisect
import collections
import collections.abc
import copy
import decimal
import doctest
import math
import pickle
import random
import sys
import unittest
from test import support
from test.support import import_helper

from decimal import Decimal
from fractions import Fraction


# Module to be tested.
import statistics


# === Helper functions and class ===

def sign(x):
    """Return -1.0 for negatives, including -0.0, otherwise +1.0."""
    return math.copysign(1, x)

def _nan_equal(a, b):
    """Return True if a and b are both the same kind of NAN.

    >>> _nan_equal(Decimal('NAN'), Decimal('NAN'))
    True
    >>> _nan_equal(Decimal('sNAN'), Decimal('sNAN'))
    True
    >>> _nan_equal(Decimal('NAN'), Decimal('sNAN'))
    False
    >>> _nan_equal(Decimal(42), Decimal('NAN'))
    False

    >>> _nan_equal(float('NAN'), float('NAN'))
    True
    >>> _nan_equal(float('NAN'), 0.5)
    False

    >>> _nan_equal(float('NAN'), Decimal('NAN'))
    False

    NAN payloads are not compared.
    """
    if type(a) is not type(b):
        return False
    if isinstance(a, float):
        return math.isnan(a) and math.isnan(b)
    aexp = a.as_tuple()[2]
    bexp = b.as_tuple()[2]
    return (aexp == bexp) and (aexp in ('n', 'N'))  # Both NAN or both sNAN.


def _calc_errors(actual, expected):
    """Return the absolute and relative errors between two numbers.

    >>> _calc_errors(100, 75)
    (25, 0.25)
    >>> _calc_errors(100, 100)
    (0, 0.0)

    Returns the (absolute error, relative error) between the two arguments.
    """
    base = max(abs(actual), abs(expected))
    abs_err = abs(actual - expected)
    rel_err = abs_err/base if base else float('inf')
    return (abs_err, rel_err)


def approx_equal(x, y, tol=1e-12, rel=1e-7):
    """approx_equal(x, y [, tol [, rel]]) => True|False

    Return True if numbers x and y are approximately equal, to within some
    margin of error, otherwise return False. Numbers which compare equal
    will also compare approximately equal.

    x is approximately equal to y if the difference between them is less than
    an absolute error tol or a relative error rel, whichever is bigger.

    If given, both tol and rel must be finite, non-negative numbers. If not
    given, default values are tol=1e-12 and rel=1e-7.

    >>> approx_equal(1.2589, 1.2587, tol=0.0003, rel=0)
    True
    >>> approx_equal(1.2589, 1.2587, tol=0.0001, rel=0)
    False

    Absolute error is defined as abs(x-y); if that is less than or equal to
    tol, x and y are considered approximately equal.

    Relative error is defined as abs((x-y)/x) or abs((x-y)/y), whichever is
    smaller, provided x or y are not zero. If that figure is less than or
    equal to rel, x and y are considered approximately equal.

    Complex numbers are not directly supported. If you wish to compare to
    complex numbers, extract their real and imaginary parts and compare them
    individually.

    NANs always compare unequal, even with themselves. Infinities compare
    approximately equal if they have the same sign (both positive or both
    negative). Infinities with different signs compare unequal; so do
    comparisons of infinities with finite numbers.
    """
    if tol < 0 or rel < 0:
        raise ValueError('error tolerances must be non-negative')
    # NANs are never equal to anything, approximately or otherwise.
    if math.isnan(x) or math.isnan(y):
        return False
    # Numbers which compare equal also compare approximately equal.
    if x == y:
        # This includes the case of two infinities with the same sign.
        return True
    if math.isinf(x) or math.isinf(y):
        # This includes the case of two infinities of opposite sign, or
        # one infinity and one finite number.
        return False
    # Two finite numbers.
    actual_error = abs(x - y)
    allowed_error = max(tol, rel*max(abs(x), abs(y)))
    return actual_error <= allowed_error


# This class exists only as somewhere to stick a docstring containing
# doctests. The following docstring and tests were originally in a separate
# module. Now that it has been merged in here, I need somewhere to hang the.
# docstring. Ultimately, this class will die, and the information below will
# either become redundant, or be moved into more appropriate places.
class _DoNothing:
    """
    When doing numeric work, especially with floats, exact equality is often
    not what you want. Due to round-off error, it is often a bad idea to try
    to compare floats with equality. Instead the usual procedure is to test
    them with some (hopefully small!) allowance for error.

    The ``approx_equal`` function allows you to specify either an absolute
    error tolerance, or a relative error, or both.

    Absolute error tolerances are simple, but you need to know the magnitude
    of the quantities being compared:

    >>> approx_equal(12.345, 12.346, tol=1e-3)
    True
    >>> approx_equal(12.345e6, 12.346e6, tol=1e-3)  # tol is too small.
    False

    Relative errors are more suitable when the values you are comparing can
    vary in magnitude:

    >>> approx_equal(12.345, 12.346, rel=1e-4)
    True
    >>> approx_equal(12.345e6, 12.346e6, rel=1e-4)
    True

    but a naive implementation of relative error testing can run into trouble
    around zero.

    If you supply both an absolute tolerance and a relative error, the
    comparison succeeds if either individual test succeeds:

    >>> approx_equal(12.345e6, 12.346e6, tol=1e-3, rel=1e-4)
    True

    """
    pass



# We prefer this for testing numeric values that may not be exactly equal,
# and avoid using TestCase.assertAlmostEqual, because it sucks :-)

py_statistics = import_helper.import_fresh_module('statistics',
                                                  blocked=['_statistics'])
c_statistics = import_helper.import_fresh_module('statistics',
                                                 fresh=['_statistics'])


class TestModules(unittest.TestCase):
    func_names = ['_normal_dist_inv_cdf']

    def test_py_functions(self):
        for fname in self.func_names:
            self.assertEqual(getattr(py_statistics, fname).__module__, 'statistics')

    @unittest.skipUnless(c_statistics, 'requires _statistics')
    def test_c_functions(self):
        for fname in self.func_names:
            self.assertEqual(getattr(c_statistics, fname).__module__, '_statistics')


class NumericTestCase(unittest.TestCase):
    """Unit test class for numeric work.

    This subclasses TestCase. In addition to the standard method
    ``TestCase.assertAlmostEqual``,  ``assertApproxEqual`` is provided.
    """
    # By default, we expect exact equality, unless overridden.
    tol = rel = 0

    def assertApproxEqual(
            self, first, second, tol=None, rel=None, msg=None
            ):
        """Test passes if ``first`` and ``second`` are approximately equal.

        This test passes if ``first`` and ``second`` are equal to
        within ``tol``, an absolute error, or ``rel``, a relative error.

        If either ``tol`` or ``rel`` are None or not given, they default to
        test attributes of the same name (by default, 0).

        The objects may be either numbers, or sequences of numbers. Sequences
        are tested element-by-element.

        >>> class MyTest(NumericTestCase):
        ...     def test_number(self):
        ...         x = 1.0/6
        ...         y = sum([x]*6)
        ...         self.assertApproxEqual(y, 1.0, tol=1e-15)
        ...     def test_sequence(self):
        ...         a = [1.001, 1.001e-10, 1.001e10]
        ...         b = [1.0, 1e-10, 1e10]
        ...         self.assertApproxEqual(a, b, rel=1e-3)
        ...
        >>> import unittest
        >>> from io import StringIO  # Suppress test runner output.
        >>> suite = unittest.TestLoader().loadTestsFromTestCase(MyTest)
        >>> unittest.TextTestRunner(stream=StringIO()).run(suite)
        <unittest.runner.TextTestResult run=2 errors=0 failures=0>

        """
        if tol is None:
            tol = self.tol
        if rel is None:
            rel = self.rel
        if (
                isinstance(first, collections.abc.Sequence) and
                isinstance(second, collections.abc.Sequence)
            ):
            check = self._check_approx_seq
        else:
            check = self._check_approx_num
        check(first, second, tol, rel, msg)

    def _check_approx_seq(self, first, second, tol, rel, msg):
        if len(first) != len(second):
            standardMsg = (
                "sequences differ in length: %d items != %d items"
                % (len(first), len(second))
                )
            msg = self._formatMessage(msg, standardMsg)
            raise self.failureException(msg)
        for i, (a,e) in enumerate(zip(first, second)):
            self._check_approx_num(a, e, tol, rel, msg, i)

    def _check_approx_num(self, first, second, tol, rel, msg, idx=None):
        if approx_equal(first, second, tol, rel):
            # Test passes. Return early, we are done.
            return None
        # Otherwise we failed.
        standardMsg = self._make_std_err_msg(first, second, tol, rel, idx)
        msg = self._formatMessage(msg, standardMsg)
        raise self.failureException(msg)

    @staticmethod
    def _make_std_err_msg(first, second, tol, rel, idx):
        # Create the standard error message for approx_equal failures.
        assert first != second
        template = (
            '  %r != %r\n'
            '  values differ by more than tol=%r and rel=%r\n'
            '  -> absolute error = %r\n'
            '  -> relative error = %r'
            )
        if idx is not None:
            header = 'numeric sequences first differ at index %d.\n' % idx
            template = header + template
        # Calculate actual errors:
        abs_err, rel_err = _calc_errors(first, second)
        return template % (first, second, tol, rel, abs_err, rel_err)


# ========================
# === Test the helpers ===
# ========================

class TestSign(unittest.TestCase):
    """Test that the helper function sign() works correctly."""
    def testZeroes(self):
        # Test that signed zeroes report their sign correctly.
        self.assertEqual(sign(0.0), +1)
        self.assertEqual(sign(-0.0), -1)


# --- Tests for approx_equal ---

class ApproxEqualSymmetryTest(unittest.TestCase):
    # Test symmetry of approx_equal.

    def test_relative_symmetry(self):
        # Check that approx_equal treats relative error symmetrically.
        # (a-b)/a is usually not equal to (a-b)/b. Ensure that this
        # doesn't matter.
        #
        #   Note: the reason for this test is that an early version
        #   of approx_equal was not symmetric. A relative error test
        #   would pass, or fail, depending on which value was passed
        #   as the first argument.
        #
        args1 = [2456, 37.8, -12.45, Decimal('2.54'), Fraction(17, 54)]
        args2 = [2459, 37.2, -12.41, Decimal('2.59'), Fraction(15, 54)]
        assert len(args1) == len(args2)
        for a, b in zip(args1, args2):
            self.do_relative_symmetry(a, b)

    def do_relative_symmetry(self, a, b):
        a, b = min(a, b), max(a, b)
        assert a < b
        delta = b - a  # The absolute difference between the values.
        rel_err1, rel_err2 = abs(delta/a), abs(delta/b)
        # Choose an error margin halfway between the two.
        rel = (rel_err1 + rel_err2)/2
        # Now see that values a and b compare approx equal regardless of
        # which is given first.
        self.assertTrue(approx_equal(a, b, tol=0, rel=rel))
        self.assertTrue(approx_equal(b, a, tol=0, rel=rel))

    def test_symmetry(self):
        # Test that approx_equal(a, b) == approx_equal(b, a)
        args = [-23, -2, 5, 107, 93568]
        delta = 2
        for a in args:
            for type_ in (int, float, Decimal, Fraction):
                x = type_(a)*100
                y = x + delta
                r = abs(delta/max(x, y))
                # There are five cases to check:
                # 1) actual error <= tol, <= rel
                self.do_symmetry_test(x, y, tol=delta, rel=r)
                self.do_symmetry_test(x, y, tol=delta+1, rel=2*r)
                # 2) actual error > tol, > rel
                self.do_symmetry_test(x, y, tol=delta-1, rel=r/2)
                # 3) actual error <= tol, > rel
                self.do_symmetry_test(x, y, tol=delta, rel=r/2)
                # 4) actual error > tol, <= rel
                self.do_symmetry_test(x, y, tol=delta-1, rel=r)
                self.do_symmetry_test(x, y, tol=delta-1, rel=2*r)
                # 5) exact equality test
                self.do_symmetry_test(x, x, tol=0, rel=0)
                self.do_symmetry_test(x, y, tol=0, rel=0)

    def do_symmetry_test(self, a, b, tol, rel):
        template = "approx_equal comparisons don't match for %r"
        flag1 = approx_equal(a, b, tol, rel)
        flag2 = approx_equal(b, a, tol, rel)
        self.assertEqual(flag1, flag2, template.format((a, b, tol, rel)))


class ApproxEqualExactTest(unittest.TestCase):
    # Test the approx_equal function with exactly equal values.
    # Equal values should compare as approximately equal.
    # Test cases for exactly equal values, which should compare approx
    # equal regardless of the error tolerances given.

    def do_exactly_equal_test(self, x, tol, rel):
        result = approx_equal(x, x, tol=tol, rel=rel)
        self.assertTrue(result, 'equality failure for x=%r' % x)
        result = approx_equal(-x, -x, tol=tol, rel=rel)
        self.assertTrue(result, 'equality failure for x=%r' % -x)

    def test_exactly_equal_ints(self):
        # Test that equal int values are exactly equal.
        for n in [42, 19740, 14974, 230, 1795, 700245, 36587]:
            self.do_exactly_equal_test(n, 0, 0)

    def test_exactly_equal_floats(self):
        # Test that equal float values are exactly equal.
        for x in [0.42, 1.9740, 1497.4, 23.0, 179.5, 70.0245, 36.587]:
            self.do_exactly_equal_test(x, 0, 0)

    def test_exactly_equal_fractions(self):
        # Test that equal Fraction values are exactly equal.
        F = Fraction
        for f in [F(1, 2), F(0), F(5, 3), F(9, 7), F(35, 36), F(3, 7)]:
            self.do_exactly_equal_test(f, 0, 0)

    def test_exactly_equal_decimals(self):
        # Test that equal Decimal values are exactly equal.
        D = Decimal
        for d in map(D, "8.2 31.274 912.04 16.745 1.2047".split()):
            self.do_exactly_equal_test(d, 0, 0)

    def test_exactly_equal_absolute(self):
        # Test that equal values are exactly equal with an absolute error.
        for n in [16, 1013, 1372, 1198, 971, 4]:
            # Test as ints.
            self.do_exactly_equal_test(n, 0.01, 0)
            # Test as floats.
            self.do_exactly_equal_test(n/10, 0.01, 0)
            # Test as Fractions.
            f = Fraction(n, 1234)
            self.do_exactly_equal_test(f, 0.01, 0)

    def test_exactly_equal_absolute_decimals(self):
        # Test equal Decimal values are exactly equal with an absolute error.
        self.do_exactly_equal_test(Decimal("3.571"), Decimal("0.01"), 0)
        self.do_exactly_equal_test(-Decimal("81.3971"), Decimal("0.01"), 0)

    def test_exactly_equal_relative(self):
        # Test that equal values are exactly equal with a relative error.
        for x in [8347, 101.3, -7910.28, Fraction(5, 21)]:
            self.do_exactly_equal_test(x, 0, 0.01)
        self.do_exactly_equal_test(Decimal("11.68"), 0, Decimal("0.01"))

    def test_exactly_equal_both(self):
        # Test that equal values are equal when both tol and rel are given.
        for x in [41017, 16.742, -813.02, Fraction(3, 8)]:
            self.do_exactly_equal_test(x, 0.1, 0.01)
        D = Decimal
        self.do_exactly_equal_test(D("7.2"), D("0.1"), D("0.01"))


class ApproxEqualUnequalTest(unittest.TestCase):
    # Unequal values should compare unequal with zero error tolerances.
    # Test cases for unequal values, with exact equality test.

    def do_exactly_unequal_test(self, x):
        for a in (x, -x):
            result = approx_equal(a, a+1, tol=0, rel=0)
            self.assertFalse(result, 'inequality failure for x=%r' % a)

    def test_exactly_unequal_ints(self):
        # Test unequal int values are unequal with zero error tolerance.
        for n in [951, 572305, 478, 917, 17240]:
            self.do_exactly_unequal_test(n)

    def test_exactly_unequal_floats(self):
        # Test unequal float values are unequal with zero error tolerance.
        for x in [9.51, 5723.05, 47.8, 9.17, 17.24]:
            self.do_exactly_unequal_test(x)

    def test_exactly_unequal_fractions(self):
        # Test that unequal Fractions are unequal with zero error tolerance.
        F = Fraction
        for f in [F(1, 5), F(7, 9), F(12, 11), F(101, 99023)]:
            self.do_exactly_unequal_test(f)

    def test_exactly_unequal_decimals(self):
        # Test that unequal Decimals are unequal with zero error tolerance.
        for d in map(Decimal, "3.1415 298.12 3.47 18.996 0.00245".split()):
            self.do_exactly_unequal_test(d)


class ApproxEqualInexactTest(unittest.TestCase):
    # Inexact test cases for approx_error.
    # Test cases when comparing two values that are not exactly equal.

    # === Absolute error tests ===

    def do_approx_equal_abs_test(self, x, delta):
        template = "Test failure for x={!r}, y={!r}"
        for y in (x + delta, x - delta):
            msg = template.format(x, y)
            self.assertTrue(approx_equal(x, y, tol=2*delta, rel=0), msg)
            self.assertFalse(approx_equal(x, y, tol=delta/2, rel=0), msg)

    def test_approx_equal_absolute_ints(self):
        # Test approximate equality of ints with an absolute error.
        for n in [-10737, -1975, -7, -2, 0, 1, 9, 37, 423, 9874, 23789110]:
            self.do_approx_equal_abs_test(n, 10)
            self.do_approx_equal_abs_test(n, 2)

    def test_approx_equal_absolute_floats(self):
        # Test approximate equality of floats with an absolute error.
        for x in [-284.126, -97.1, -3.4, -2.15, 0.5, 1.0, 7.8, 4.23, 3817.4]:
            self.do_approx_equal_abs_test(x, 1.5)
            self.do_approx_equal_abs_test(x, 0.01)
            self.do_approx_equal_abs_test(x, 0.0001)

    def test_approx_equal_absolute_fractions(self):
        # Test approximate equality of Fractions with an absolute error.
        delta = Fraction(1, 29)
        numerators = [-84, -15, -2, -1, 0, 1, 5, 17, 23, 34, 71]
        for f in (Fraction(n, 29) for n in numerators):
            self.do_approx_equal_abs_test(f, delta)
            self.do_approx_equal_abs_test(f, float(delta))

    def test_approx_equal_absolute_decimals(self):
        # Test approximate equality of Decimals with an absolute error.
        delta = Decimal("0.01")
        for d in map(Decimal, "1.0 3.5 36.08 61.79 7912.3648".split()):
            self.do_approx_equal_abs_test(d, delta)
            self.do_approx_equal_abs_test(-d, delta)

    def test_cross_zero(self):
        # Test for the case of the two values having opposite signs.
        self.assertTrue(approx_equal(1e-5, -1e-5, tol=1e-4, rel=0))

    # === Relative error tests ===

    def do_approx_equal_rel_test(self, x, delta):
        template = "Test failure for x={!r}, y={!r}"
        for y in (x*(1+delta), x*(1-delta)):
            msg = template.format(x, y)
            self.assertTrue(approx_equal(x, y, tol=0, rel=2*delta), msg)
            self.assertFalse(approx_equal(x, y, tol=0, rel=delta/2), msg)

    def test_approx_equal_relative_ints(self):
        # Test approximate equality of ints with a relative error.
        self.assertTrue(approx_equal(64, 47, tol=0, rel=0.36))
        self.assertTrue(approx_equal(64, 47, tol=0, rel=0.37))
        # ---
        self.assertTrue(approx_equal(449, 512, tol=0, rel=0.125))
        self.assertTrue(approx_equal(448, 512, tol=0, rel=0.125))
        self.assertFalse(approx_equal(447, 512, tol=0, rel=0.125))

    def test_approx_equal_relative_floats(self):
        # Test approximate equality of floats with a relative error.
        for x in [-178.34, -0.1, 0.1, 1.0, 36.97, 2847.136, 9145.074]:
            self.do_approx_equal_rel_test(x, 0.02)
            self.do_approx_equal_rel_test(x, 0.0001)

    def test_approx_equal_relative_fractions(self):
        # Test approximate equality of Fractions with a relative error.
        F = Fraction
        delta = Fraction(3, 8)
        for f in [F(3, 84), F(17, 30), F(49, 50), F(92, 85)]:
            for d in (delta, float(delta)):
                self.do_approx_equal_rel_test(f, d)
                self.do_approx_equal_rel_test(-f, d)

    def test_approx_equal_relative_decimals(self):
        # Test approximate equality of Decimals with a relative error.
        for d in map(Decimal, "0.02 1.0 5.7 13.67 94.138 91027.9321".split()):
            self.do_approx_equal_rel_test(d, Decimal("0.001"))
            self.do_approx_equal_rel_test(-d, Decimal("0.05"))

    # === Both absolute and relative error tests ===

    # There are four cases to consider:
    #   1) actual error <= both absolute and relative error
    #   2) actual error <= absolute error but > relative error
    #   3) actual error <= relative error but > absolute error
    #   4) actual error > both absolute and relative error

    def do_check_both(self, a, b, tol, rel, tol_flag, rel_flag):
        check = self.assertTrue if tol_flag else self.assertFalse
        check(approx_equal(a, b, tol=tol, rel=0))
        check = self.assertTrue if rel_flag else self.assertFalse
        check(approx_equal(a, b, tol=0, rel=rel))
        check = self.assertTrue if (tol_flag or rel_flag) else self.assertFalse
        check(approx_equal(a, b, tol=tol, rel=rel))

    def test_approx_equal_both1(self):
        # Test actual error <= both absolute and relative error.
        self.do_check_both(7.955, 7.952, 0.004, 3.8e-4, True, True)
        self.do_check_both(-7.387, -7.386, 0.002, 0.0002, True, True)

    def test_approx_equal_both2(self):
        # Test actual error <= absolute error but > relative error.
        self.do_check_both(7.955, 7.952, 0.004, 3.7e-4, True, False)

    def test_approx_equal_both3(self):
        # Test actual error <= relative error but > absolute error.
        self.do_check_both(7.955, 7.952, 0.001, 3.8e-4, False, True)

    def test_approx_equal_both4(self):
        # Test actual error > both absolute and relative error.
        self.do_check_both(2.78, 2.75, 0.01, 0.001, False, False)
        self.do_check_both(971.44, 971.47, 0.02, 3e-5, False, False)


class ApproxEqualSpecialsTest(unittest.TestCase):
    # Test approx_equal with NANs and INFs and zeroes.

    def test_inf(self):
        for type_ in (float, Decimal):
            inf = type_('inf')
            self.assertTrue(approx_equal(inf, inf))
            self.assertTrue(approx_equal(inf, inf, 0, 0))
            self.assertTrue(approx_equal(inf, inf, 1, 0.01))
            self.assertTrue(approx_equal(-inf, -inf))
            self.assertFalse(approx_equal(inf, -inf))
            self.assertFalse(approx_equal(inf, 1000))

    def test_nan(self):
        for type_ in (float, Decimal):
            nan = type_('nan')
            for other in (nan, type_('inf'), 1000):
                self.assertFalse(approx_equal(nan, other))

    def test_float_zeroes(self):
        nzero = math.copysign(0.0, -1)
        self.assertTrue(approx_equal(nzero, 0.0, tol=0.1, rel=0.1))

    def test_decimal_zeroes(self):
        nzero = Decimal("-0.0")
        self.assertTrue(approx_equal(nzero, Decimal(0), tol=0.1, rel=0.1))


class TestApproxEqualErrors(unittest.TestCase):
    # Test error conditions of approx_equal.

    def test_bad_tol(self):
        # Test negative tol raises.
        self.assertRaises(ValueError, approx_equal, 100, 100, -1, 0.1)

    def test_bad_rel(self):
        # Test negative rel raises.
        self.assertRaises(ValueError, approx_equal, 100, 100, 1, -0.1)


# --- Tests for NumericTestCase ---

# The formatting routine that generates the error messages is complex enough
# that it too needs testing.

class TestNumericTestCase(unittest.TestCase):
    # The exact wording of NumericTestCase error messages is *not* guaranteed,
    # but we need to give them some sort of test to ensure that they are
    # generated correctly. As a compromise, we look for specific substrings
    # that are expected to be found even if the overall error message changes.

    def do_test(self, args):
        actual_msg = NumericTestCase._make_std_err_msg(*args)
        expected = self.generate_substrings(*args)
        for substring in expected:
            self.assertIn(substring, actual_msg)

    def test_numerictestcase_is_testcase(self):
        # Ensure that NumericTestCase actually is a TestCase.
        self.assertTrue(issubclass(NumericTestCase, unittest.TestCase))

    def test_error_msg_numeric(self):
        # Test the error message generated for numeric comparisons.
        args = (2.5, 4.0, 0.5, 0.25, None)
        self.do_test(args)

    def test_error_msg_sequence(self):
        # Test the error message generated for sequence comparisons.
        args = (3.75, 8.25, 1.25, 0.5, 7)
        self.do_test(args)

    def generate_substrings(self, first, second, tol, rel, idx):
        """Return substrings we expect to see in error messages."""
        abs_err, rel_err = _calc_errors(first, second)
        substrings = [
                'tol=%r' % tol,
                'rel=%r' % rel,
                'absolute error = %r' % abs_err,
                'relative error = %r' % rel_err,
                ]
        if idx is not None:
            substrings.append('differ at index %d' % idx)
        return substrings


# =======================================
# === Tests for the statistics module ===
# =======================================


class GlobalsTest(unittest.TestCase):
    module = statistics
    expected_metadata = ["__doc__", "__all__"]

    def test_meta(self):
        # Test for the existence of metadata.
        for meta in self.expected_metadata:
            self.assertTrue(hasattr(self.module, meta),
                            "%s not present" % meta)

    def test_check_all(self):
        # Check everything in __all__ exists and is public.
        module = self.module
        for name in module.__all__:
            # No private names in __all__:
            self.assertFalse(name.startswith("_"),
                             'private name "%s" in __all__' % name)
            # And anything in __all__ must exist:
            self.assertTrue(hasattr(module, name),
                            'missing name "%s" in __all__' % name)


class DocTests(unittest.TestCase):
    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -OO and above")
    def test_doc_tests(self):
        failed, tried = doctest.testmod(statistics, optionflags=doctest.ELLIPSIS)
        self.assertGreater(tried, 0)
        self.assertEqual(failed, 0)

class StatisticsErrorTest(unittest.TestCase):
    def test_has_exception(self):
        errmsg = (
                "Expected StatisticsError to be a ValueError, but got a"
                " subclass of %r instead."
                )
        self.assertTrue(hasattr(statistics, 'StatisticsError'))
        self.assertTrue(
                issubclass(statistics.StatisticsError, ValueError),
                errmsg % statistics.StatisticsError.__base__
                )


# === Tests for private utility functions ===

class ExactRatioTest(unittest.TestCase):
    # Test _exact_ratio utility.

    def test_int(self):
        for i in (-20, -3, 0, 5, 99, 10**20):
            self.assertEqual(statistics._exact_ratio(i), (i, 1))

    def test_fraction(self):
        numerators = (-5, 1, 12, 38)
        for n in numerators:
            f = Fraction(n, 37)
            self.assertEqual(statistics._exact_ratio(f), (n, 37))

    def test_float(self):
        self.assertEqual(statistics._exact_ratio(0.125), (1, 8))
        self.assertEqual(statistics._exact_ratio(1.125), (9, 8))
        data = [random.uniform(-100, 100) for _ in range(100)]
        for x in data:
            num, den = statistics._exact_ratio(x)
            self.assertEqual(x, num/den)

    def test_decimal(self):
        D = Decimal
        _exact_ratio = statistics._exact_ratio
        self.assertEqual(_exact_ratio(D("0.125")), (1, 8))
        self.assertEqual(_exact_ratio(D("12.345")), (2469, 200))
        self.assertEqual(_exact_ratio(D("-1.98")), (-99, 50))

    def test_inf(self):
        INF = float("INF")
        class MyFloat(float):
            pass
        class MyDecimal(Decimal):
            pass
        for inf in (INF, -INF):
            for type_ in (float, MyFloat, Decimal, MyDecimal):
                x = type_(inf)
                ratio = statistics._exact_ratio(x)
                self.assertEqual(ratio, (x, None))
                self.assertEqual(type(ratio[0]), type_)
                self.assertTrue(math.isinf(ratio[0]))

    def test_float_nan(self):
        NAN = float("NAN")
        class MyFloat(float):
            pass
        for nan in (NAN, MyFloat(NAN)):
            ratio = statistics._exact_ratio(nan)
            self.assertTrue(math.isnan(ratio[0]))
            self.assertIs(ratio[1], None)
            self.assertEqual(type(ratio[0]), type(nan))

    def test_decimal_nan(self):
        NAN = Decimal("NAN")
        sNAN = Decimal("sNAN")
        class MyDecimal(Decimal):
            pass
        for nan in (NAN, MyDecimal(NAN), sNAN, MyDecimal(sNAN)):
            ratio = statistics._exact_ratio(nan)
            self.assertTrue(_nan_equal(ratio[0], nan))
            self.assertIs(ratio[1], None)
            self.assertEqual(type(ratio[0]), type(nan))


class DecimalToRatioTest(unittest.TestCase):
    # Test _exact_ratio private function.

    def test_infinity(self):
        # Test that INFs are handled correctly.
        inf = Decimal('INF')
        self.assertEqual(statistics._exact_ratio(inf), (inf, None))
        self.assertEqual(statistics._exact_ratio(-inf), (-inf, None))

    def test_nan(self):
        # Test that NANs are handled correctly.
        for nan in (Decimal('NAN'), Decimal('sNAN')):
            num, den = statistics._exact_ratio(nan)
            # Because NANs always compare non-equal, we cannot use assertEqual.
            # Nor can we use an identity test, as we don't guarantee anything
            # about the object identity.
            self.assertTrue(_nan_equal(num, nan))
            self.assertIs(den, None)

    def test_sign(self):
        # Test sign is calculated correctly.
        numbers = [Decimal("9.8765e12"), Decimal("9.8765e-12")]
        for d in numbers:
            # First test positive decimals.
            assert d > 0
            num, den = statistics._exact_ratio(d)
            self.assertGreaterEqual(num, 0)
            self.assertGreater(den, 0)
            # Then test negative decimals.
            num, den = statistics._exact_ratio(-d)
            self.assertLessEqual(num, 0)
            self.assertGreater(den, 0)

    def test_negative_exponent(self):
        # Test result when the exponent is negative.
        t = statistics._exact_ratio(Decimal("0.1234"))
        self.assertEqual(t, (617, 5000))

    def test_positive_exponent(self):
        # Test results when the exponent is positive.
        t = statistics._exact_ratio(Decimal("1.234e7"))
        self.assertEqual(t, (12340000, 1))

    def test_regression_20536(self):
        # Regression test for issue 20536.
        # See http://bugs.python.org/issue20536
        t = statistics._exact_ratio(Decimal("1e2"))
        self.assertEqual(t, (100, 1))
        t = statistics._exact_ratio(Decimal("1.47e5"))
        self.assertEqual(t, (147000, 1))


class IsFiniteTest(unittest.TestCase):
    # Test _isfinite private function.

    def test_finite(self):
        # Test that finite numbers are recognised as finite.
        for x in (5, Fraction(1, 3), 2.5, Decimal("5.5")):
            self.assertTrue(statistics._isfinite(x))

    def test_infinity(self):
        # Test that INFs are not recognised as finite.
        for x in (float("inf"), Decimal("inf")):
            self.assertFalse(statistics._isfinite(x))

    def test_nan(self):
        # Test that NANs are not recognised as finite.
        for x in (float("nan"), Decimal("NAN"), Decimal("sNAN")):
            self.assertFalse(statistics._isfinite(x))


class CoerceTest(unittest.TestCase):
    # Test that private function _coerce correctly deals with types.

    # The coercion rules are currently an implementation detail, although at
    # some point that should change. The tests and comments here define the
    # correct implementation.

    # Pre-conditions of _coerce:
    #
    #   - The first time _sum calls _coerce, the
    #   - coerce(T, S) will never be called with bool as the first argument;
    #     this is a pre-condition, guarded with an assertion.

    #
    #   - coerce(T, T) will always return T; we assume T is a valid numeric
    #     type. Violate this assumption at your own risk.
    #
    #   - Apart from as above, bool is treated as if it were actually int.
    #
    #   - coerce(int, X) and coerce(X, int) return X.
    #   -
    def test_bool(self):
        # bool is somewhat special, due to the pre-condition that it is
        # never given as the first argument to _coerce, and that it cannot
        # be subclassed. So we test it specially.
        for T in (int, float, Fraction, Decimal):
            self.assertIs(statistics._coerce(T, bool), T)
            class MyClass(T): pass
            self.assertIs(statistics._coerce(MyClass, bool), MyClass)

    def assertCoerceTo(self, A, B):
        """Assert that type A coerces to B."""
        self.assertIs(statistics._coerce(A, B), B)
        self.assertIs(statistics._coerce(B, A), B)

    def check_coerce_to(self, A, B):
        """Checks that type A coerces to B, including subclasses."""
        # Assert that type A is coerced to B.
        self.assertCoerceTo(A, B)
        # Subclasses of A are also coerced to B.
        class SubclassOfA(A): pass
        self.assertCoerceTo(SubclassOfA, B)
        # A, and subclasses of A, are coerced to subclasses of B.
        class SubclassOfB(B): pass
        self.assertCoerceTo(A, SubclassOfB)
        self.assertCoerceTo(SubclassOfA, SubclassOfB)

    def assertCoerceRaises(self, A, B):
        """Assert that coercing A to B, or vice versa, raises TypeError."""
        self.assertRaises(TypeError, statistics._coerce, (A, B))
        self.assertRaises(TypeError, statistics._coerce, (B, A))

    def check_type_coercions(self, T):
        """Check that type T coerces correctly with subclasses of itself."""
        assert T is not bool
        # Coercing a type with itself returns the same type.
        self.assertIs(statistics._coerce(T, T), T)
        # Coercing a type with a subclass of itself returns the subclass.
        class U(T): pass
        class V(T): pass
        class W(U): pass
        for typ in (U, V, W):
            self.assertCoerceTo(T, typ)
        self.assertCoerceTo(U, W)
        # Coercing two subclasses that aren't parent/child is an error.
        self.assertCoerceRaises(U, V)
        self.assertCoerceRaises(V, W)

    def test_int(self):
        # Check that int coerces correctly.
        self.check_type_coercions(int)
        for typ in (float, Fraction, Decimal):
            self.check_coerce_to(int, typ)

    def test_fraction(self):
        # Check that Fraction coerces correctly.
        self.check_type_coercions(Fraction)
        self.check_coerce_to(Fraction, float)

    def test_decimal(self):
        # Check that Decimal coerces correctly.
        self.check_type_coercions(Decimal)

    def test_float(self):
        # Check that float coerces correctly.
        self.check_type_coercions(float)

    def test_non_numeric_types(self):
        for bad_type in (str, list, type(None), tuple, dict):
            for good_type in (int, float, Fraction, Decimal):
                self.assertCoerceRaises(good_type, bad_type)

    def test_incompatible_types(self):
        # Test that incompatible types raise.
        for T in (float, Fraction):
            class MySubclass(T): pass
            self.assertCoerceRaises(T, Decimal)
            self.assertCoerceRaises(MySubclass, Decimal)


class ConvertTest(unittest.TestCase):
    # Test private _convert function.

    def check_exact_equal(self, x, y):
        """Check that x equals y, and has the same type as well."""
        self.assertEqual(x, y)
        self.assertIs(type(x), type(y))

    def test_int(self):
        # Test conversions to int.
        x = statistics._convert(Fraction(71), int)
        self.check_exact_equal(x, 71)
        class MyInt(int): pass
        x = statistics._convert(Fraction(17), MyInt)
        self.check_exact_equal(x, MyInt(17))

    def test_fraction(self):
        # Test conversions to Fraction.
        x = statistics._convert(Fraction(95, 99), Fraction)
        self.check_exact_equal(x, Fraction(95, 99))
        class MyFraction(Fraction):
            def __truediv__(self, other):
                return self.__class__(super().__truediv__(other))
        x = statistics._convert(Fraction(71, 13), MyFraction)
        self.check_exact_equal(x, MyFraction(71, 13))

    def test_float(self):
        # Test conversions to float.
        x = statistics._convert(Fraction(-1, 2), float)
        self.check_exact_equal(x, -0.5)
        class MyFloat(float):
            def __truediv__(self, other):
                return self.__class__(super().__truediv__(other))
        x = statistics._convert(Fraction(9, 8), MyFloat)
        self.check_exact_equal(x, MyFloat(1.125))

    def test_decimal(self):
        # Test conversions to Decimal.
        x = statistics._convert(Fraction(1, 40), Decimal)
        self.check_exact_equal(x, Decimal("0.025"))
        class MyDecimal(Decimal):
            def __truediv__(self, other):
                return self.__class__(super().__truediv__(other))
        x = statistics._convert(Fraction(-15, 16), MyDecimal)
        self.check_exact_equal(x, MyDecimal("-0.9375"))

    def test_inf(self):
        for INF in (float('inf'), Decimal('inf')):
            for inf in (INF, -INF):
                x = statistics._convert(inf, type(inf))
                self.check_exact_equal(x, inf)

    def test_nan(self):
        for nan in (float('nan'), Decimal('NAN'), Decimal('sNAN')):
            x = statistics._convert(nan, type(nan))
            self.assertTrue(_nan_equal(x, nan))

    def test_invalid_input_type(self):
        with self.assertRaises(TypeError):
            statistics._convert(None, float)


class FailNegTest(unittest.TestCase):
    """Test _fail_neg private function."""

    def test_pass_through(self):
        # Test that values are passed through unchanged.
        values = [1, 2.0, Fraction(3), Decimal(4)]
        new = list(statistics._fail_neg(values))
        self.assertEqual(values, new)

    def test_negatives_raise(self):
        # Test that negatives raise an exception.
        for x in [1, 2.0, Fraction(3), Decimal(4)]:
            seq = [-x]
            it = statistics._fail_neg(seq)
            self.assertRaises(statistics.StatisticsError, next, it)

    def test_error_msg(self):
        # Test that a given error message is used.
        msg = "badness #%d" % random.randint(10000, 99999)
        try:
            next(statistics._fail_neg([-1], msg))
        except statistics.StatisticsError as e:
            errmsg = e.args[0]
        else:
            self.fail("expected exception, but it didn't happen")
        self.assertEqual(errmsg, msg)


class FindLteqTest(unittest.TestCase):
    # Test _find_lteq private function.

    def test_invalid_input_values(self):
        for a, x in [
            ([], 1),
            ([1, 2], 3),
            ([1, 3], 2)
        ]:
            with self.subTest(a=a, x=x):
                with self.assertRaises(ValueError):
                    statistics._find_lteq(a, x)

    def test_locate_successfully(self):
        for a, x, expected_i in [
            ([1, 1, 1, 2, 3], 1, 0),
            ([0, 1, 1, 1, 2, 3], 1, 1),
            ([1, 2, 3, 3, 3], 3, 2)
        ]:
            with self.subTest(a=a, x=x):
                self.assertEqual(expected_i, statistics._find_lteq(a, x))


class FindRteqTest(unittest.TestCase):
    # Test _find_rteq private function.

    def test_invalid_input_values(self):
        for a, l, x in [
            ([1], 2, 1),
            ([1, 3], 0, 2)
        ]:
            with self.assertRaises(ValueError):
                statistics._find_rteq(a, l, x)

    def test_locate_successfully(self):
        for a, l, x, expected_i in [
            ([1, 1, 1, 2, 3], 0, 1, 2),
            ([0, 1, 1, 1, 2, 3], 0, 1, 3),
            ([1, 2, 3, 3, 3], 0, 3, 4)
        ]:
            with self.subTest(a=a, l=l, x=x):
                self.assertEqual(expected_i, statistics._find_rteq(a, l, x))


# === Tests for public functions ===

class UnivariateCommonMixin:
    # Common tests for most univariate functions that take a data argument.

    def test_no_args(self):
        # Fail if given no arguments.
        self.assertRaises(TypeError, self.func)

    def test_empty_data(self):
        # Fail when the data argument (first argument) is empty.
        for empty in ([], (), iter([])):
            self.assertRaises(statistics.StatisticsError, self.func, empty)

    def prepare_data(self):
        """Return int data for various tests."""
        data = list(range(10))
        while data == sorted(data):
            random.shuffle(data)
        return data

    def test_no_inplace_modifications(self):
        # Test that the function does not modify its input data.
        data = self.prepare_data()
        assert len(data) != 1  # Necessary to avoid infinite loop.
        assert data != sorted(data)
        saved = data[:]
        assert data is not saved
        _ = self.func(data)
        self.assertListEqual(data, saved, "data has been modified")

    def test_order_doesnt_matter(self):
        # Test that the order of data points doesn't change the result.

        # CAUTION: due to floating point rounding errors, the result actually
        # may depend on the order. Consider this test representing an ideal.
        # To avoid this test failing, only test with exact values such as ints
        # or Fractions.
        data = [1, 2, 3, 3, 3, 4, 5, 6]*100
        expected = self.func(data)
        random.shuffle(data)
        actual = self.func(data)
        self.assertEqual(expected, actual)

    def test_type_of_data_collection(self):
        # Test that the type of iterable data doesn't effect the result.
        class MyList(list):
            pass
        class MyTuple(tuple):
            pass
        def generator(data):
            return (obj for obj in data)
        data = self.prepare_data()
        expected = self.func(data)
        for kind in (list, tuple, iter, MyList, MyTuple, generator):
            result = self.func(kind(data))
            self.assertEqual(result, expected)

    def test_range_data(self):
        # Test that functions work with range objects.
        data = range(20, 50, 3)
        expected = self.func(list(data))
        self.assertEqual(self.func(data), expected)

    def test_bad_arg_types(self):
        # Test that function raises when given data of the wrong type.

        # Don't roll the following into a loop like this:
        #   for bad in list_of_bad:
        #       self.check_for_type_error(bad)
        #
        # Since assertRaises doesn't show the arguments that caused the test
        # failure, it is very difficult to debug these test failures when the
        # following are in a loop.
        self.check_for_type_error(None)
        self.check_for_type_error(23)
        self.check_for_type_error(42.0)
        self.check_for_type_error(object())

    def check_for_type_error(self, *args):
        self.assertRaises(TypeError, self.func, *args)

    def test_type_of_data_element(self):
        # Check the type of data elements doesn't affect the numeric result.
        # This is a weaker test than UnivariateTypeMixin.testTypesConserved,
        # because it checks the numeric result by equality, but not by type.
        class MyFloat(float):
            def __truediv__(self, other):
                return type(self)(super().__truediv__(other))
            def __add__(self, other):
                return type(self)(super().__add__(other))
            __radd__ = __add__

        raw = self.prepare_data()
        expected = self.func(raw)
        for kind in (float, MyFloat, Decimal, Fraction):
            data = [kind(x) for x in raw]
            result = type(expected)(self.func(data))
            self.assertEqual(result, expected)


class UnivariateTypeMixin:
    """Mixin class for type-conserving functions.

    This mixin class holds test(s) for functions which conserve the type of
    individual data points. E.g. the mean of a list of Fractions should itself
    be a Fraction.

    Not all tests to do with types need go in this class. Only those that
    rely on the function returning the same type as its input data.
    """
    def prepare_types_for_conservation_test(self):
        """Return the types which are expected to be conserved."""
        class MyFloat(float):
            def __truediv__(self, other):
                return type(self)(super().__truediv__(other))
            def __rtruediv__(self, other):
                return type(self)(super().__rtruediv__(other))
            def __sub__(self, other):
                return type(self)(super().__sub__(other))
            def __rsub__(self, other):
                return type(self)(super().__rsub__(other))
            def __pow__(self, other):
                return type(self)(super().__pow__(other))
            def __add__(self, other):
                return type(self)(super().__add__(other))
            __radd__ = __add__
        return (float, Decimal, Fraction, MyFloat)

    def test_types_conserved(self):
        # Test that functions keeps the same type as their data points.
        # (Excludes mixed data types.) This only tests the type of the return
        # result, not the value.
        data = self.prepare_data()
        for kind in self.prepare_types_for_conservation_test():
            d = [kind(x) for x in data]
            result = self.func(d)
            self.assertIs(type(result), kind)


class TestSumCommon(UnivariateCommonMixin, UnivariateTypeMixin):
    # Common test cases for statistics._sum() function.

    # This test suite looks only at the numeric value returned by _sum,
    # after conversion to the appropriate type.
    def setUp(self):
        def simplified_sum(*args):
            T, value, n = statistics._sum(*args)
            return statistics._coerce(value, T)
        self.func = simplified_sum


class TestSum(NumericTestCase):
    # Test cases for statistics._sum() function.

    # These tests look at the entire three value tuple returned by _sum.

    def setUp(self):
        self.func = statistics._sum

    def test_empty_data(self):
        # Override test for empty data.
        for data in ([], (), iter([])):
            self.assertEqual(self.func(data), (int, Fraction(0), 0))

    def test_ints(self):
        self.assertEqual(self.func([1, 5, 3, -4, -8, 20, 42, 1]),
                         (int, Fraction(60), 8))

    def test_floats(self):
        self.assertEqual(self.func([0.25]*20),
                         (float, Fraction(5.0), 20))

    def test_fractions(self):
        self.assertEqual(self.func([Fraction(1, 1000)]*500),
                         (Fraction, Fraction(1, 2), 500))

    def test_decimals(self):
        D = Decimal
        data = [D("0.001"), D("5.246"), D("1.702"), D("-0.025"),
                D("3.974"), D("2.328"), D("4.617"), D("2.843"),
                ]
        self.assertEqual(self.func(data),
                         (Decimal, Decimal("20.686"), 8))

    def test_compare_with_math_fsum(self):
        # Compare with the math.fsum function.
        # Ideally we ought to get the exact same result, but sometimes
        # we differ by a very slight amount :-(
        data = [random.uniform(-100, 1000) for _ in range(1000)]
        self.assertApproxEqual(float(self.func(data)[1]), math.fsum(data), rel=2e-16)

    def test_strings_fail(self):
        # Sum of strings should fail.
        self.assertRaises(TypeError, self.func, [1, 2, 3], '999')
        self.assertRaises(TypeError, self.func, [1, 2, 3, '999'])

    def test_bytes_fail(self):
        # Sum of bytes should fail.
        self.assertRaises(TypeError, self.func, [1, 2, 3], b'999')
        self.assertRaises(TypeError, self.func, [1, 2, 3, b'999'])

    def test_mixed_sum(self):
        # Mixed input types are not (currently) allowed.
        # Check that mixed data types fail.
        self.assertRaises(TypeError, self.func, [1, 2.0, Decimal(1)])
        # And so does mixed start argument.
        self.assertRaises(TypeError, self.func, [1, 2.0], Decimal(1))


class SumTortureTest(NumericTestCase):
    def test_torture(self):
        # Tim Peters' torture test for sum, and variants of same.
        self.assertEqual(statistics._sum([1, 1e100, 1, -1e100]*10000),
                         (float, Fraction(20000.0), 40000))
        self.assertEqual(statistics._sum([1e100, 1, 1, -1e100]*10000),
                         (float, Fraction(20000.0), 40000))
        T, num, count = statistics._sum([1e-100, 1, 1e-100, -1]*10000)
        self.assertIs(T, float)
        self.assertEqual(count, 40000)
        self.assertApproxEqual(float(num), 2.0e-96, rel=5e-16)


class SumSpecialValues(NumericTestCase):
    # Test that sum works correctly with IEEE-754 special values.

    def test_nan(self):
        for type_ in (float, Decimal):
            nan = type_('nan')
            result = statistics._sum([1, nan, 2])[1]
            self.assertIs(type(result), type_)
            self.assertTrue(math.isnan(result))

    def check_infinity(self, x, inf):
        """Check x is an infinity of the same type and sign as inf."""
        self.assertTrue(math.isinf(x))
        self.assertIs(type(x), type(inf))
        self.assertEqual(x > 0, inf > 0)
        assert x == inf

    def do_test_inf(self, inf):
        # Adding a single infinity gives infinity.
        result = statistics._sum([1, 2, inf, 3])[1]
        self.check_infinity(result, inf)
        # Adding two infinities of the same sign also gives infinity.
        result = statistics._sum([1, 2, inf, 3, inf, 4])[1]
        self.check_infinity(result, inf)

    def test_float_inf(self):
        inf = float('inf')
        for sign in (+1, -1):
            self.do_test_inf(sign*inf)

    def test_decimal_inf(self):
        inf = Decimal('inf')
        for sign in (+1, -1):
            self.do_test_inf(sign*inf)

    def test_float_mismatched_infs(self):
        # Test that adding two infinities of opposite sign gives a NAN.
        inf = float('inf')
        result = statistics._sum([1, 2, inf, 3, -inf, 4])[1]
        self.assertTrue(math.isnan(result))

    def test_decimal_extendedcontext_mismatched_infs_to_nan(self):
        # Test adding Decimal INFs with opposite sign returns NAN.
        inf = Decimal('inf')
        data = [1, 2, inf, 3, -inf, 4]
        with decimal.localcontext(decimal.ExtendedContext):
            self.assertTrue(math.isnan(statistics._sum(data)[1]))

    def test_decimal_basiccontext_mismatched_infs_to_nan(self):
        # Test adding Decimal INFs with opposite sign raises InvalidOperation.
        inf = Decimal('inf')
        data = [1, 2, inf, 3, -inf, 4]
        with decimal.localcontext(decimal.BasicContext):
            self.assertRaises(decimal.InvalidOperation, statistics._sum, data)

    def test_decimal_snan_raises(self):
        # Adding sNAN should raise InvalidOperation.
        sNAN = Decimal('sNAN')
        data = [1, sNAN, 2]
        self.assertRaises(decimal.InvalidOperation, statistics._sum, data)


# === Tests for averages ===

class AverageMixin(UnivariateCommonMixin):
    # Mixin class holding common tests for averages.

    def test_single_value(self):
        # Average of a single value is the value itself.
        for x in (23, 42.5, 1.3e15, Fraction(15, 19), Decimal('0.28')):
            self.assertEqual(self.func([x]), x)

    def prepare_values_for_repeated_single_test(self):
        return (3.5, 17, 2.5e15, Fraction(61, 67), Decimal('4.9712'))

    def test_repeated_single_value(self):
        # The average of a single repeated value is the value itself.
        for x in self.prepare_values_for_repeated_single_test():
            for count in (2, 5, 10, 20):
                with self.subTest(x=x, count=count):
                    data = [x]*count
                    self.assertEqual(self.func(data), x)


class TestMean(NumericTestCase, AverageMixin, UnivariateTypeMixin):
    def setUp(self):
        self.func = statistics.mean

    def test_torture_pep(self):
        # "Torture Test" from PEP-450.
        self.assertEqual(self.func([1e100, 1, 3, -1e100]), 1)

    def test_ints(self):
        # Test mean with ints.
        data = [0, 1, 2, 3, 3, 3, 4, 5, 5, 6, 7, 7, 7, 7, 8, 9]
        random.shuffle(data)
        self.assertEqual(self.func(data), 4.8125)

    def test_floats(self):
        # Test mean with floats.
        data = [17.25, 19.75, 20.0, 21.5, 21.75, 23.25, 25.125, 27.5]
        random.shuffle(data)
        self.assertEqual(self.func(data), 22.015625)

    def test_decimals(self):
        # Test mean with Decimals.
        D = Decimal
        data = [D("1.634"), D("2.517"), D("3.912"), D("4.072"), D("5.813")]
        random.shuffle(data)
        self.assertEqual(self.func(data), D("3.5896"))

    def test_fractions(self):
        # Test mean with Fractions.
        F = Fraction
        data = [F(1, 2), F(2, 3), F(3, 4), F(4, 5), F(5, 6), F(6, 7), F(7, 8)]
        random.shuffle(data)
        self.assertEqual(self.func(data), F(1479, 1960))

    def test_inf(self):
        # Test mean with infinities.
        raw = [1, 3, 5, 7, 9]  # Use only ints, to avoid TypeError later.
        for kind in (float, Decimal):
            for sign in (1, -1):
                inf = kind("inf")*sign
                data = raw + [inf]
                result = self.func(data)
                self.assertTrue(math.isinf(result))
                self.assertEqual(result, inf)

    def test_mismatched_infs(self):
        # Test mean with infinities of opposite sign.
        data = [2, 4, 6, float('inf'), 1, 3, 5, float('-inf')]
        result = self.func(data)
        self.assertTrue(math.isnan(result))

    def test_nan(self):
        # Test mean with NANs.
        raw = [1, 3, 5, 7, 9]  # Use only ints, to avoid TypeError later.
        for kind in (float, Decimal):
            inf = kind("nan")
            data = raw + [inf]
            result = self.func(data)
            self.assertTrue(math.isnan(result))

    def test_big_data(self):
        # Test adding a large constant to every data point.
        c = 1e9
        data = [3.4, 4.5, 4.9, 6.7, 6.8, 7.2, 8.0, 8.1, 9.4]
        expected = self.func(data) + c
        assert expected != c
        result = self.func([x+c for x in data])
        self.assertEqual(result, expected)

    def test_doubled_data(self):
        # Mean of [a,b,c...z] should be same as for [a,a,b,b,c,c...z,z].
        data = [random.uniform(-3, 5) for _ in range(1000)]
        expected = self.func(data)
        actual = self.func(data*2)
        self.assertApproxEqual(actual, expected)

    def test_regression_20561(self):
        # Regression test for issue 20561.
        # See http://bugs.python.org/issue20561
        d = Decimal('1e4')
        self.assertEqual(statistics.mean([d]), d)

    def test_regression_25177(self):
        # Regression test for issue 25177.
        # Ensure very big and very small floats don't overflow.
        # See http://bugs.python.org/issue25177.
        self.assertEqual(statistics.mean(
            [8.988465674311579e+307, 8.98846567431158e+307]),
            8.98846567431158e+307)
        big = 8.98846567431158e+307
        tiny = 5e-324
        for n in (2, 3, 5, 200):
            self.assertEqual(statistics.mean([big]*n), big)
            self.assertEqual(statistics.mean([tiny]*n), tiny)


class TestHarmonicMean(NumericTestCase, AverageMixin, UnivariateTypeMixin):
    def setUp(self):
        self.func = statistics.harmonic_mean

    def prepare_data(self):
        # Override mixin method.
        values = super().prepare_data()
        values.remove(0)
        return values

    def prepare_values_for_repeated_single_test(self):
        # Override mixin method.
        return (3.5, 17, 2.5e15, Fraction(61, 67), Decimal('4.125'))

    def test_zero(self):
        # Test that harmonic mean returns zero when given zero.
        values = [1, 0, 2]
        self.assertEqual(self.func(values), 0)

    def test_negative_error(self):
        # Test that harmonic mean raises when given a negative value.
        exc = statistics.StatisticsError
        for values in ([-1], [1, -2, 3]):
            with self.subTest(values=values):
                self.assertRaises(exc, self.func, values)

    def test_invalid_type_error(self):
        # Test error is raised when input contains invalid type(s)
        for data in [
            ['3.14'],               # single string
            ['1', '2', '3'],        # multiple strings
            [1, '2', 3, '4', 5],    # mixed strings and valid integers
            [2.3, 3.4, 4.5, '5.6']  # only one string and valid floats
        ]:
            with self.subTest(data=data):
                with self.assertRaises(TypeError):
                    self.func(data)

    def test_ints(self):
        # Test harmonic mean with ints.
        data = [2, 4, 4, 8, 16, 16]
        random.shuffle(data)
        self.assertEqual(self.func(data), 6*4/5)

    def test_floats_exact(self):
        # Test harmonic mean with some carefully chosen floats.
        data = [1/8, 1/4, 1/4, 1/2, 1/2]
        random.shuffle(data)
        self.assertEqual(self.func(data), 1/4)
        self.assertEqual(self.func([0.25, 0.5, 1.0, 1.0]), 0.5)

    def test_singleton_lists(self):
        # Test that harmonic mean([x]) returns (approximately) x.
        for x in range(1, 101):
            self.assertEqual(self.func([x]), x)

    def test_decimals_exact(self):
        # Test harmonic mean with some carefully chosen Decimals.
        D = Decimal
        self.assertEqual(self.func([D(15), D(30), D(60), D(60)]), D(30))
        data = [D("0.05"), D("0.10"), D("0.20"), D("0.20")]
        random.shuffle(data)
        self.assertEqual(self.func(data), D("0.10"))
        data = [D("1.68"), D("0.32"), D("5.94"), D("2.75")]
        random.shuffle(data)
        self.assertEqual(self.func(data), D(66528)/70723)

    def test_fractions(self):
        # Test harmonic mean with Fractions.
        F = Fraction
        data = [F(1, 2), F(2, 3), F(3, 4), F(4, 5), F(5, 6), F(6, 7), F(7, 8)]
        random.shuffle(data)
        self.assertEqual(self.func(data), F(7*420, 4029))

    def test_inf(self):
        # Test harmonic mean with infinity.
        values = [2.0, float('inf'), 1.0]
        self.assertEqual(self.func(values), 2.0)

    def test_nan(self):
        # Test harmonic mean with NANs.
        values = [2.0, float('nan'), 1.0]
        self.assertTrue(math.isnan(self.func(values)))

    def test_multiply_data_points(self):
        # Test multiplying every data point by a constant.
        c = 111
        data = [3.4, 4.5, 4.9, 6.7, 6.8, 7.2, 8.0, 8.1, 9.4]
        expected = self.func(data)*c
        result = self.func([x*c for x in data])
        self.assertEqual(result, expected)

    def test_doubled_data(self):
        # Harmonic mean of [a,b...z] should be same as for [a,a,b,b...z,z].
        data = [random.uniform(1, 5) for _ in range(1000)]
        expected = self.func(data)
        actual = self.func(data*2)
        self.assertApproxEqual(actual, expected)

    def test_with_weights(self):
        self.assertEqual(self.func([40, 60], [5, 30]), 56.0)  # common case
        self.assertEqual(self.func([40, 60],
                                   weights=[5, 30]), 56.0)    # keyword argument
        self.assertEqual(self.func(iter([40, 60]),
                                   iter([5, 30])), 56.0)      # iterator inputs
        self.assertEqual(
            self.func([Fraction(10, 3), Fraction(23, 5), Fraction(7, 2)], [5, 2, 10]),
            self.func([Fraction(10, 3)] * 5 +
                      [Fraction(23, 5)] * 2 +
                      [Fraction(7, 2)] * 10))
        self.assertEqual(self.func([10], [7]), 10)            # n=1 fast path
        with self.assertRaises(TypeError):
            self.func([1, 2, 3], [1, (), 3])                  # non-numeric weight
        with self.assertRaises(statistics.StatisticsError):
            self.func([1, 2, 3], [1, 2])                      # wrong number of weights
        with self.assertRaises(statistics.StatisticsError):
            self.func([10], [0])                              # no non-zero weights
        with self.assertRaises(statistics.StatisticsError):
            self.func([10, 20], [0, 0])                       # no non-zero weights


class TestMedian(NumericTestCase, AverageMixin):
    # Common tests for median and all median.* functions.
    def setUp(self):
        self.func = statistics.median

    def prepare_data(self):
        """Overload method from UnivariateCommonMixin."""
        data = super().prepare_data()
        if len(data)%2 != 1:
            data.append(2)
        return data

    def test_even_ints(self):
        # Test median with an even number of int data points.
        data = [1, 2, 3, 4, 5, 6]
        assert len(data)%2 == 0
        self.assertEqual(self.func(data), 3.5)

    def test_odd_ints(self):
        # Test median with an odd number of int data points.
        data = [1, 2, 3, 4, 5, 6, 9]
        assert len(data)%2 == 1
        self.assertEqual(self.func(data), 4)

    def test_odd_fractions(self):
        # Test median works with an odd number of Fractions.
        F = Fraction
        data = [F(1, 7), F(2, 7), F(3, 7), F(4, 7), F(5, 7)]
        assert len(data)%2 == 1
        random.shuffle(data)
        self.assertEqual(self.func(data), F(3, 7))

    def test_even_fractions(self):
        # Test median works with an even number of Fractions.
        F = Fraction
        data = [F(1, 7), F(2, 7), F(3, 7), F(4, 7), F(5, 7), F(6, 7)]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), F(1, 2))

    def test_odd_decimals(self):
        # Test median works with an odd number of Decimals.
        D = Decimal
        data = [D('2.5'), D('3.1'), D('4.2'), D('5.7'), D('5.8')]
        assert len(data)%2 == 1
        random.shuffle(data)
        self.assertEqual(self.func(data), D('4.2'))

    def test_even_decimals(self):
        # Test median works with an even number of Decimals.
        D = Decimal
        data = [D('1.2'), D('2.5'), D('3.1'), D('4.2'), D('5.7'), D('5.8')]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), D('3.65'))


class TestMedianDataType(NumericTestCase, UnivariateTypeMixin):
    # Test conservation of data element type for median.
    def setUp(self):
        self.func = statistics.median

    def prepare_data(self):
        data = list(range(15))
        assert len(data)%2 == 1
        while data == sorted(data):
            random.shuffle(data)
        return data


class TestMedianLow(TestMedian, UnivariateTypeMixin):
    def setUp(self):
        self.func = statistics.median_low

    def test_even_ints(self):
        # Test median_low with an even number of ints.
        data = [1, 2, 3, 4, 5, 6]
        assert len(data)%2 == 0
        self.assertEqual(self.func(data), 3)

    def test_even_fractions(self):
        # Test median_low works with an even number of Fractions.
        F = Fraction
        data = [F(1, 7), F(2, 7), F(3, 7), F(4, 7), F(5, 7), F(6, 7)]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), F(3, 7))

    def test_even_decimals(self):
        # Test median_low works with an even number of Decimals.
        D = Decimal
        data = [D('1.1'), D('2.2'), D('3.3'), D('4.4'), D('5.5'), D('6.6')]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), D('3.3'))


class TestMedianHigh(TestMedian, UnivariateTypeMixin):
    def setUp(self):
        self.func = statistics.median_high

    def test_even_ints(self):
        # Test median_high with an even number of ints.
        data = [1, 2, 3, 4, 5, 6]
        assert len(data)%2 == 0
        self.assertEqual(self.func(data), 4)

    def test_even_fractions(self):
        # Test median_high works with an even number of Fractions.
        F = Fraction
        data = [F(1, 7), F(2, 7), F(3, 7), F(4, 7), F(5, 7), F(6, 7)]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), F(4, 7))

    def test_even_decimals(self):
        # Test median_high works with an even number of Decimals.
        D = Decimal
        data = [D('1.1'), D('2.2'), D('3.3'), D('4.4'), D('5.5'), D('6.6')]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), D('4.4'))


class TestMedianGrouped(TestMedian):
    # Test median_grouped.
    # Doesn't conserve data element types, so don't use TestMedianType.
    def setUp(self):
        self.func = statistics.median_grouped

    def test_odd_number_repeated(self):
        # Test median.grouped with repeated median values.
        data = [12, 13, 14, 14, 14, 15, 15]
        assert len(data)%2 == 1
        self.assertEqual(self.func(data), 14)
        #---
        data = [12, 13, 14, 14, 14, 14, 15]
        assert len(data)%2 == 1
        self.assertEqual(self.func(data), 13.875)
        #---
        data = [5, 10, 10, 15, 20, 20, 20, 20, 25, 25, 30]
        assert len(data)%2 == 1
        self.assertEqual(self.func(data, 5), 19.375)
        #---
        data = [16, 18, 18, 18, 18, 20, 20, 20, 22, 22, 22, 24, 24, 26, 28]
        assert len(data)%2 == 1
        self.assertApproxEqual(self.func(data, 2), 20.66666667, tol=1e-8)

    def test_even_number_repeated(self):
        # Test median.grouped with repeated median values.
        data = [5, 10, 10, 15, 20, 20, 20, 25, 25, 30]
        assert len(data)%2 == 0
        self.assertApproxEqual(self.func(data, 5), 19.16666667, tol=1e-8)
        #---
        data = [2, 3, 4, 4, 4, 5]
        assert len(data)%2 == 0
        self.assertApproxEqual(self.func(data), 3.83333333, tol=1e-8)
        #---
        data = [2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6]
        assert len(data)%2 == 0
        self.assertEqual(self.func(data), 4.5)
        #---
        data = [3, 4, 4, 4, 5, 5, 5, 5, 6, 6]
        assert len(data)%2 == 0
        self.assertEqual(self.func(data), 4.75)

    def test_repeated_single_value(self):
        # Override method from AverageMixin.
        # Yet again, failure of median_grouped to conserve the data type
        # causes me headaches :-(
        for x in (5.3, 68, 4.3e17, Fraction(29, 101), Decimal('32.9714')):
            for count in (2, 5, 10, 20):
                data = [x]*count
                self.assertEqual(self.func(data), float(x))

    def test_odd_fractions(self):
        # Test median_grouped works with an odd number of Fractions.
        F = Fraction
        data = [F(5, 4), F(9, 4), F(13, 4), F(13, 4), F(17, 4)]
        assert len(data)%2 == 1
        random.shuffle(data)
        self.assertEqual(self.func(data), 3.0)

    def test_even_fractions(self):
        # Test median_grouped works with an even number of Fractions.
        F = Fraction
        data = [F(5, 4), F(9, 4), F(13, 4), F(13, 4), F(17, 4), F(17, 4)]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), 3.25)

    def test_odd_decimals(self):
        # Test median_grouped works with an odd number of Decimals.
        D = Decimal
        data = [D('5.5'), D('6.5'), D('6.5'), D('7.5'), D('8.5')]
        assert len(data)%2 == 1
        random.shuffle(data)
        self.assertEqual(self.func(data), 6.75)

    def test_even_decimals(self):
        # Test median_grouped works with an even number of Decimals.
        D = Decimal
        data = [D('5.5'), D('5.5'), D('6.5'), D('6.5'), D('7.5'), D('8.5')]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), 6.5)
        #---
        data = [D('5.5'), D('5.5'), D('6.5'), D('7.5'), D('7.5'), D('8.5')]
        assert len(data)%2 == 0
        random.shuffle(data)
        self.assertEqual(self.func(data), 7.0)

    def test_interval(self):
        # Test median_grouped with interval argument.
        data = [2.25, 2.5, 2.5, 2.75, 2.75, 3.0, 3.0, 3.25, 3.5, 3.75]
        self.assertEqual(self.func(data, 0.25), 2.875)
        data = [2.25, 2.5, 2.5, 2.75, 2.75, 2.75, 3.0, 3.0, 3.25, 3.5, 3.75]
        self.assertApproxEqual(self.func(data, 0.25), 2.83333333, tol=1e-8)
        data = [220, 220, 240, 260, 260, 260, 260, 280, 280, 300, 320, 340]
        self.assertEqual(self.func(data, 20), 265.0)

    def test_data_type_error(self):
        # Test median_grouped with str, bytes data types for data and interval
        data = ["", "", ""]
        self.assertRaises(TypeError, self.func, data)
        #---
        data = [b"", b"", b""]
        self.assertRaises(TypeError, self.func, data)
        #---
        data = [1, 2, 3]
        interval = ""
        self.assertRaises(TypeError, self.func, data, interval)
        #---
        data = [1, 2, 3]
        interval = b""
        self.assertRaises(TypeError, self.func, data, interval)


class TestMode(NumericTestCase, AverageMixin, UnivariateTypeMixin):
    # Test cases for the discrete version of mode.
    def setUp(self):
        self.func = statistics.mode

    def prepare_data(self):
        """Overload method from UnivariateCommonMixin."""
        # Make sure test data has exactly one mode.
        return [1, 1, 1, 1, 3, 4, 7, 9, 0, 8, 2]

    def test_range_data(self):
        # Override test from UnivariateCommonMixin.
        data = range(20, 50, 3)
        self.assertEqual(self.func(data), 20)

    def test_nominal_data(self):
        # Test mode with nominal data.
        data = 'abcbdb'
        self.assertEqual(self.func(data), 'b')
        data = 'fe fi fo fum fi fi'.split()
        self.assertEqual(self.func(data), 'fi')

    def test_discrete_data(self):
        # Test mode with discrete numeric data.
        data = list(range(10))
        for i in range(10):
            d = data + [i]
            random.shuffle(d)
            self.assertEqual(self.func(d), i)

    def test_bimodal_data(self):
        # Test mode with bimodal data.
        data = [1, 1, 2, 2, 2, 2, 3, 4, 5, 6, 6, 6, 6, 7, 8, 9, 9]
        assert data.count(2) == data.count(6) == 4
        # mode() should return 2, the first encountered mode
        self.assertEqual(self.func(data), 2)

    def test_unique_data(self):
        # Test mode when data points are all unique.
        data = list(range(10))
        # mode() should return 0, the first encountered mode
        self.assertEqual(self.func(data), 0)

    def test_none_data(self):
        # Test that mode raises TypeError if given None as data.

        # This test is necessary because the implementation of mode uses
        # collections.Counter, which accepts None and returns an empty dict.
        self.assertRaises(TypeError, self.func, None)

    def test_counter_data(self):
        # Test that a Counter is treated like any other iterable.
        # We're making sure mode() first calls iter() on its input.
        # The concern is that a Counter of a Counter returns the original
        # unchanged rather than counting its keys.
        c = collections.Counter(a=1, b=2)
        # If iter() is called, mode(c) loops over the keys, ['a', 'b'],
        # all the counts will be 1, and the first encountered mode is 'a'.
        self.assertEqual(self.func(c), 'a')


class TestMultiMode(unittest.TestCase):

    def test_basics(self):
        multimode = statistics.multimode
        self.assertEqual(multimode('aabbbbbbbbcc'), ['b'])
        self.assertEqual(multimode('aabbbbccddddeeffffgg'), ['b', 'd', 'f'])
        self.assertEqual(multimode(''), [])


class TestFMean(unittest.TestCase):

    def test_basics(self):
        fmean = statistics.fmean
        D = Decimal
        F = Fraction
        for data, expected_mean, kind in [
            ([3.5, 4.0, 5.25], 4.25, 'floats'),
            ([D('3.5'), D('4.0'), D('5.25')], 4.25, 'decimals'),
            ([F(7, 2), F(4, 1), F(21, 4)], 4.25, 'fractions'),
            ([True, False, True, True, False], 0.60, 'booleans'),
            ([3.5, 4, F(21, 4)], 4.25, 'mixed types'),
            ((3.5, 4.0, 5.25), 4.25, 'tuple'),
            (iter([3.5, 4.0, 5.25]), 4.25, 'iterator'),
                ]:
            actual_mean = fmean(data)
            self.assertIs(type(actual_mean), float, kind)
            self.assertEqual(actual_mean, expected_mean, kind)

    def test_error_cases(self):
        fmean = statistics.fmean
        StatisticsError = statistics.StatisticsError
        with self.assertRaises(StatisticsError):
            fmean([])                               # empty input
        with self.assertRaises(StatisticsError):
            fmean(iter([]))                         # empty iterator
        with self.assertRaises(TypeError):
            fmean(None)                             # non-iterable input
        with self.assertRaises(TypeError):
            fmean([10, None, 20])                   # non-numeric input
        with self.assertRaises(TypeError):
            fmean()                                 # missing data argument
        with self.assertRaises(TypeError):
            fmean([10, 20, 60], 70)                 # too many arguments

    def test_special_values(self):
        # Rules for special values are inherited from math.fsum()
        fmean = statistics.fmean
        NaN = float('Nan')
        Inf = float('Inf')
        self.assertTrue(math.isnan(fmean([10, NaN])), 'nan')
        self.assertTrue(math.isnan(fmean([NaN, Inf])), 'nan and infinity')
        self.assertTrue(math.isinf(fmean([10, Inf])), 'infinity')
        with self.assertRaises(ValueError):
            fmean([Inf, -Inf])


# === Tests for variances and standard deviations ===

class VarianceStdevMixin(UnivariateCommonMixin):
    # Mixin class holding common tests for variance and std dev.

    # Subclasses should inherit from this before NumericTestClass, in order
    # to see the rel attribute below. See testShiftData for an explanation.

    rel = 1e-12

    def test_single_value(self):
        # Deviation of a single value is zero.
        for x in (11, 19.8, 4.6e14, Fraction(21, 34), Decimal('8.392')):
            self.assertEqual(self.func([x]), 0)

    def test_repeated_single_value(self):
        # The deviation of a single repeated value is zero.
        for x in (7.2, 49, 8.1e15, Fraction(3, 7), Decimal('62.4802')):
            for count in (2, 3, 5, 15):
                data = [x]*count
                self.assertEqual(self.func(data), 0)

    def test_domain_error_regression(self):
        # Regression test for a domain error exception.
        # (Thanks to Geremy Condra.)
        data = [0.123456789012345]*10000
        # All the items are identical, so variance should be exactly zero.
        # We allow some small round-off error, but not much.
        result = self.func(data)
        self.assertApproxEqual(result, 0.0, tol=5e-17)
        self.assertGreaterEqual(result, 0)  # A negative result must fail.

    def test_shift_data(self):
        # Test that shifting the data by a constant amount does not affect
        # the variance or stdev. Or at least not much.

        # Due to rounding, this test should be considered an ideal. We allow
        # some tolerance away from "no change at all" by setting tol and/or rel
        # attributes. Subclasses may set tighter or looser error tolerances.
        raw = [1.03, 1.27, 1.94, 2.04, 2.58, 3.14, 4.75, 4.98, 5.42, 6.78]
        expected = self.func(raw)
        # Don't set shift too high, the bigger it is, the more rounding error.
        shift = 1e5
        data = [x + shift for x in raw]
        self.assertApproxEqual(self.func(data), expected)

    def test_shift_data_exact(self):
        # Like test_shift_data, but result is always exact.
        raw = [1, 3, 3, 4, 5, 7, 9, 10, 11, 16]
        assert all(x==int(x) for x in raw)
        expected = self.func(raw)
        shift = 10**9
        data = [x + shift for x in raw]
        self.assertEqual(self.func(data), expected)

    def test_iter_list_same(self):
        # Test that iter data and list data give the same result.

        # This is an explicit test that iterators and lists are treated the
        # same; justification for this test over and above the similar test
        # in UnivariateCommonMixin is that an earlier design had variance and
        # friends swap between one- and two-pass algorithms, which would
        # sometimes give different results.
        data = [random.uniform(-3, 8) for _ in range(1000)]
        expected = self.func(data)
        self.assertEqual(self.func(iter(data)), expected)


class TestPVariance(VarianceStdevMixin, NumericTestCase, UnivariateTypeMixin):
    # Tests for population variance.
    def setUp(self):
        self.func = statistics.pvariance

    def test_exact_uniform(self):
        # Test the variance against an exact result for uniform data.
        data = list(range(10000))
        random.shuffle(data)
        expected = (10000**2 - 1)/12  # Exact value.
        self.assertEqual(self.func(data), expected)

    def test_ints(self):
        # Test population variance with int data.
        data = [4, 7, 13, 16]
        exact = 22.5
        self.assertEqual(self.func(data), exact)

    def test_fractions(self):
        # Test population variance with Fraction data.
        F = Fraction
        data = [F(1, 4), F(1, 4), F(3, 4), F(7, 4)]
        exact = F(3, 8)
        result = self.func(data)
        self.assertEqual(result, exact)
        self.assertIsInstance(result, Fraction)

    def test_decimals(self):
        # Test population variance with Decimal data.
        D = Decimal
        data = [D("12.1"), D("12.2"), D("12.5"), D("12.9")]
        exact = D('0.096875')
        result = self.func(data)
        self.assertEqual(result, exact)
        self.assertIsInstance(result, Decimal)

    def test_accuracy_bug_20499(self):
        data = [0, 0, 1]
        exact = 2 / 9
        result = self.func(data)
        self.assertEqual(result, exact)
        self.assertIsInstance(result, float)


class TestVariance(VarianceStdevMixin, NumericTestCase, UnivariateTypeMixin):
    # Tests for sample variance.
    def setUp(self):
        self.func = statistics.variance

    def test_single_value(self):
        # Override method from VarianceStdevMixin.
        for x in (35, 24.7, 8.2e15, Fraction(19, 30), Decimal('4.2084')):
            self.assertRaises(statistics.StatisticsError, self.func, [x])

    def test_ints(self):
        # Test sample variance with int data.
        data = [4, 7, 13, 16]
        exact = 30
        self.assertEqual(self.func(data), exact)

    def test_fractions(self):
        # Test sample variance with Fraction data.
        F = Fraction
        data = [F(1, 4), F(1, 4), F(3, 4), F(7, 4)]
        exact = F(1, 2)
        result = self.func(data)
        self.assertEqual(result, exact)
        self.assertIsInstance(result, Fraction)

    def test_decimals(self):
        # Test sample variance with Decimal data.
        D = Decimal
        data = [D(2), D(2), D(7), D(9)]
        exact = 4*D('9.5')/D(3)
        result = self.func(data)
        self.assertEqual(result, exact)
        self.assertIsInstance(result, Decimal)

    def test_center_not_at_mean(self):
        data = (1.0, 2.0)
        self.assertEqual(self.func(data), 0.5)
        self.assertEqual(self.func(data, xbar=2.0), 1.0)

    def test_accuracy_bug_20499(self):
        data = [0, 0, 2]
        exact = 4 / 3
        result = self.func(data)
        self.assertEqual(result, exact)
        self.assertIsInstance(result, float)

class TestPStdev(VarianceStdevMixin, NumericTestCase):
    # Tests for population standard deviation.
    def setUp(self):
        self.func = statistics.pstdev

    def test_compare_to_variance(self):
        # Test that stdev is, in fact, the square root of variance.
        data = [random.uniform(-17, 24) for _ in range(1000)]
        expected = math.sqrt(statistics.pvariance(data))
        self.assertEqual(self.func(data), expected)

    def test_center_not_at_mean(self):
        # See issue: 40855
        data = (3, 6, 7, 10)
        self.assertEqual(self.func(data), 2.5)
        self.assertEqual(self.func(data, mu=0.5), 6.5)

class TestStdev(VarianceStdevMixin, NumericTestCase):
    # Tests for sample standard deviation.
    def setUp(self):
        self.func = statistics.stdev

    def test_single_value(self):
        # Override method from VarianceStdevMixin.
        for x in (81, 203.74, 3.9e14, Fraction(5, 21), Decimal('35.719')):
            self.assertRaises(statistics.StatisticsError, self.func, [x])

    def test_compare_to_variance(self):
        # Test that stdev is, in fact, the square root of variance.
        data = [random.uniform(-2, 9) for _ in range(1000)]
        expected = math.sqrt(statistics.variance(data))
        self.assertEqual(self.func(data), expected)

    def test_center_not_at_mean(self):
        data = (1.0, 2.0)
        self.assertEqual(self.func(data, xbar=2.0), 1.0)

class TestGeometricMean(unittest.TestCase):

    def test_basics(self):
        geometric_mean = statistics.geometric_mean
        self.assertAlmostEqual(geometric_mean([54, 24, 36]), 36.0)
        self.assertAlmostEqual(geometric_mean([4.0, 9.0]), 6.0)
        self.assertAlmostEqual(geometric_mean([17.625]), 17.625)

        random.seed(86753095551212)
        for rng in [
                range(1, 100),
                range(1, 1_000),
                range(1, 10_000),
                range(500, 10_000, 3),
                range(10_000, 500, -3),
                [12, 17, 13, 5, 120, 7],
                [random.expovariate(50.0) for i in range(1_000)],
                [random.lognormvariate(20.0, 3.0) for i in range(2_000)],
                [random.triangular(2000, 3000, 2200) for i in range(3_000)],
            ]:
            gm_decimal = math.prod(map(Decimal, rng)) ** (Decimal(1) / len(rng))
            gm_float = geometric_mean(rng)
            self.assertTrue(math.isclose(gm_float, float(gm_decimal)))

    def test_various_input_types(self):
        geometric_mean = statistics.geometric_mean
        D = Decimal
        F = Fraction
        # https://www.wolframalpha.com/input/?i=geometric+mean+3.5,+4.0,+5.25
        expected_mean = 4.18886
        for data, kind in [
            ([3.5, 4.0, 5.25], 'floats'),
            ([D('3.5'), D('4.0'), D('5.25')], 'decimals'),
            ([F(7, 2), F(4, 1), F(21, 4)], 'fractions'),
            ([3.5, 4, F(21, 4)], 'mixed types'),
            ((3.5, 4.0, 5.25), 'tuple'),
            (iter([3.5, 4.0, 5.25]), 'iterator'),
                ]:
            actual_mean = geometric_mean(data)
            self.assertIs(type(actual_mean), float, kind)
            self.assertAlmostEqual(actual_mean, expected_mean, places=5)

    def test_big_and_small(self):
        geometric_mean = statistics.geometric_mean

        # Avoid overflow to infinity
        large = 2.0 ** 1000
        big_gm = geometric_mean([54.0 * large, 24.0 * large, 36.0 * large])
        self.assertTrue(math.isclose(big_gm, 36.0 * large))
        self.assertFalse(math.isinf(big_gm))

        # Avoid underflow to zero
        small = 2.0 ** -1000
        small_gm = geometric_mean([54.0 * small, 24.0 * small, 36.0 * small])
        self.assertTrue(math.isclose(small_gm, 36.0 * small))
        self.assertNotEqual(small_gm, 0.0)

    def test_error_cases(self):
        geometric_mean = statistics.geometric_mean
        StatisticsError = statistics.StatisticsError
        with self.assertRaises(StatisticsError):
            geometric_mean([])                      # empty input
        with self.assertRaises(StatisticsError):
            geometric_mean([3.5, 0.0, 5.25])        # zero input
        with self.assertRaises(StatisticsError):
            geometric_mean([3.5, -4.0, 5.25])       # negative input
        with self.assertRaises(StatisticsError):
            geometric_mean(iter([]))                # empty iterator
        with self.assertRaises(TypeError):
            geometric_mean(None)                    # non-iterable input
        with self.assertRaises(TypeError):
            geometric_mean([10, None, 20])          # non-numeric input
        with self.assertRaises(TypeError):
            geometric_mean()                        # missing data argument
        with self.assertRaises(TypeError):
            geometric_mean([10, 20, 60], 70)        # too many arguments

    def test_special_values(self):
        # Rules for special values are inherited from math.fsum()
        geometric_mean = statistics.geometric_mean
        NaN = float('Nan')
        Inf = float('Inf')
        self.assertTrue(math.isnan(geometric_mean([10, NaN])), 'nan')
        self.assertTrue(math.isnan(geometric_mean([NaN, Inf])), 'nan and infinity')
        self.assertTrue(math.isinf(geometric_mean([10, Inf])), 'infinity')
        with self.assertRaises(ValueError):
            geometric_mean([Inf, -Inf])


class TestQuantiles(unittest.TestCase):

    def test_specific_cases(self):
        # Match results computed by hand and cross-checked
        # against the PERCENTILE.EXC function in MS Excel.
        quantiles = statistics.quantiles
        data = [120, 200, 250, 320, 350]
        random.shuffle(data)
        for n, expected in [
            (1, []),
            (2, [250.0]),
            (3, [200.0, 320.0]),
            (4, [160.0, 250.0, 335.0]),
            (5, [136.0, 220.0, 292.0, 344.0]),
            (6, [120.0, 200.0, 250.0, 320.0, 350.0]),
            (8, [100.0, 160.0, 212.5, 250.0, 302.5, 335.0, 357.5]),
            (10, [88.0, 136.0, 184.0, 220.0, 250.0, 292.0, 326.0, 344.0, 362.0]),
            (12, [80.0, 120.0, 160.0, 200.0, 225.0, 250.0, 285.0, 320.0, 335.0,
                  350.0, 365.0]),
            (15, [72.0, 104.0, 136.0, 168.0, 200.0, 220.0, 240.0, 264.0, 292.0,
                  320.0, 332.0, 344.0, 356.0, 368.0]),
                ]:
            self.assertEqual(expected, quantiles(data, n=n))
            self.assertEqual(len(quantiles(data, n=n)), n - 1)
            # Preserve datatype when possible
            for datatype in (float, Decimal, Fraction):
                result = quantiles(map(datatype, data), n=n)
                self.assertTrue(all(type(x) == datatype) for x in result)
                self.assertEqual(result, list(map(datatype, expected)))
            # Quantiles should be idempotent
            if len(expected) >= 2:
                self.assertEqual(quantiles(expected, n=n), expected)
            # Cross-check against method='inclusive' which should give
            # the same result after adding in minimum and maximum values
            # extrapolated from the two lowest and two highest points.
            sdata = sorted(data)
            lo = 2 * sdata[0] - sdata[1]
            hi = 2 * sdata[-1] - sdata[-2]
            padded_data = data + [lo, hi]
            self.assertEqual(
                quantiles(data, n=n),
                quantiles(padded_data, n=n, method='inclusive'),
                (n, data),
            )
            # Invariant under translation and scaling
            def f(x):
                return 3.5 * x - 1234.675
            exp = list(map(f, expected))
            act = quantiles(map(f, data), n=n)
            self.assertTrue(all(math.isclose(e, a) for e, a in zip(exp, act)))
        # Q2 agrees with median()
        for k in range(2, 60):
            data = random.choices(range(100), k=k)
            q1, q2, q3 = quantiles(data)
            self.assertEqual(q2, statistics.median(data))

    def test_specific_cases_inclusive(self):
        # Match results computed by hand and cross-checked
        # against the PERCENTILE.INC function in MS Excel
        # and against the quantile() function in SciPy.
        quantiles = statistics.quantiles
        data = [100, 200, 400, 800]
        random.shuffle(data)
        for n, expected in [
            (1, []),
            (2, [300.0]),
            (3, [200.0, 400.0]),
            (4, [175.0, 300.0, 500.0]),
            (5, [160.0, 240.0, 360.0, 560.0]),
            (6, [150.0, 200.0, 300.0, 400.0, 600.0]),
            (8, [137.5, 175, 225.0, 300.0, 375.0, 500.0,650.0]),
            (10, [130.0, 160.0, 190.0, 240.0, 300.0, 360.0, 440.0, 560.0, 680.0]),
            (12, [125.0, 150.0, 175.0, 200.0, 250.0, 300.0, 350.0, 400.0,
                  500.0, 600.0, 700.0]),
            (15, [120.0, 140.0, 160.0, 180.0, 200.0, 240.0, 280.0, 320.0, 360.0,
                  400.0, 480.0, 560.0, 640.0, 720.0]),
                ]:
            self.assertEqual(expected, quantiles(data, n=n, method="inclusive"))
            self.assertEqual(len(quantiles(data, n=n, method="inclusive")), n - 1)
            # Preserve datatype when possible
            for datatype in (float, Decimal, Fraction):
                result = quantiles(map(datatype, data), n=n, method="inclusive")
                self.assertTrue(all(type(x) == datatype) for x in result)
                self.assertEqual(result, list(map(datatype, expected)))
            # Invariant under translation and scaling
            def f(x):
                return 3.5 * x - 1234.675
            exp = list(map(f, expected))
            act = quantiles(map(f, data), n=n, method="inclusive")
            self.assertTrue(all(math.isclose(e, a) for e, a in zip(exp, act)))
        # Natural deciles
        self.assertEqual(quantiles([0, 100], n=10, method='inclusive'),
                         [10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0])
        self.assertEqual(quantiles(range(0, 101), n=10, method='inclusive'),
                         [10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0])
        # Whenever n is smaller than the number of data points, running
        # method='inclusive' should give the same result as method='exclusive'
        # after the two included extreme points are removed.
        data = [random.randrange(10_000) for i in range(501)]
        actual = quantiles(data, n=32, method='inclusive')
        data.remove(min(data))
        data.remove(max(data))
        expected = quantiles(data, n=32)
        self.assertEqual(expected, actual)
        # Q2 agrees with median()
        for k in range(2, 60):
            data = random.choices(range(100), k=k)
            q1, q2, q3 = quantiles(data, method='inclusive')
            self.assertEqual(q2, statistics.median(data))

    def test_equal_inputs(self):
        quantiles = statistics.quantiles
        for n in range(2, 10):
            data = [10.0] * n
            self.assertEqual(quantiles(data), [10.0, 10.0, 10.0])
            self.assertEqual(quantiles(data, method='inclusive'),
                            [10.0, 10.0, 10.0])

    def test_equal_sized_groups(self):
        quantiles = statistics.quantiles
        total = 10_000
        data = [random.expovariate(0.2) for i in range(total)]
        while len(set(data)) != total:
            data.append(random.expovariate(0.2))
        data.sort()

        # Cases where the group size exactly divides the total
        for n in (1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000):
            group_size = total // n
            self.assertEqual(
                [bisect.bisect(data, q) for q in quantiles(data, n=n)],
                list(range(group_size, total, group_size)))

        # When the group sizes can't be exactly equal, they should
        # differ by no more than one
        for n in (13, 19, 59, 109, 211, 571, 1019, 1907, 5261, 9769):
            group_sizes = {total // n, total // n + 1}
            pos = [bisect.bisect(data, q) for q in quantiles(data, n=n)]
            sizes = {q - p for p, q in zip(pos, pos[1:])}
            self.assertTrue(sizes <= group_sizes)

    def test_error_cases(self):
        quantiles = statistics.quantiles
        StatisticsError = statistics.StatisticsError
        with self.assertRaises(TypeError):
            quantiles()                         # Missing arguments
        with self.assertRaises(TypeError):
            quantiles([10, 20, 30], 13, n=4)    # Too many arguments
        with self.assertRaises(TypeError):
            quantiles([10, 20, 30], 4)          # n is a positional argument
        with self.assertRaises(StatisticsError):
            quantiles([10, 20, 30], n=0)        # n is zero
        with self.assertRaises(StatisticsError):
            quantiles([10, 20, 30], n=-1)       # n is negative
        with self.assertRaises(TypeError):
            quantiles([10, 20, 30], n=1.5)      # n is not an integer
        with self.assertRaises(ValueError):
            quantiles([10, 20, 30], method='X') # method is unknown
        with self.assertRaises(StatisticsError):
            quantiles([10], n=4)                # not enough data points
        with self.assertRaises(TypeError):
            quantiles([10, None, 30], n=4)      # data is non-numeric


class TestBivariateStatistics(unittest.TestCase):

    def test_unequal_size_error(self):
        for x, y in [
            ([1, 2, 3], [1, 2]),
            ([1, 2], [1, 2, 3]),
        ]:
            with self.assertRaises(statistics.StatisticsError):
                statistics.covariance(x, y)
            with self.assertRaises(statistics.StatisticsError):
                statistics.correlation(x, y)
            with self.assertRaises(statistics.StatisticsError):
                statistics.linear_regression(x, y)

    def test_small_sample_error(self):
        for x, y in [
            ([], []),
            ([], [1, 2,]),
            ([1, 2,], []),
            ([1,], [1,]),
            ([1,], [1, 2,]),
            ([1, 2,], [1,]),
        ]:
            with self.assertRaises(statistics.StatisticsError):
                statistics.covariance(x, y)
            with self.assertRaises(statistics.StatisticsError):
                statistics.correlation(x, y)
            with self.assertRaises(statistics.StatisticsError):
                statistics.linear_regression(x, y)


class TestCorrelationAndCovariance(unittest.TestCase):

    def test_results(self):
        for x, y, result in [
            ([1, 2, 3], [1, 2, 3], 1),
            ([1, 2, 3], [-1, -2, -3], -1),
            ([1, 2, 3], [3, 2, 1], -1),
            ([1, 2, 3], [1, 2, 1], 0),
            ([1, 2, 3], [1, 3, 2], 0.5),
        ]:
            self.assertAlmostEqual(statistics.correlation(x, y), result)
            self.assertAlmostEqual(statistics.covariance(x, y), result)

    def test_different_scales(self):
        x = [1, 2, 3]
        y = [10, 30, 20]
        self.assertAlmostEqual(statistics.correlation(x, y), 0.5)
        self.assertAlmostEqual(statistics.covariance(x, y), 5)

        y = [.1, .2, .3]
        self.assertAlmostEqual(statistics.correlation(x, y), 1)
        self.assertAlmostEqual(statistics.covariance(x, y), 0.1)


class TestLinearRegression(unittest.TestCase):

    def test_constant_input_error(self):
        x = [1, 1, 1,]
        y = [1, 2, 3,]
        with self.assertRaises(statistics.StatisticsError):
            statistics.linear_regression(x, y)

    def test_results(self):
        for x, y, true_intercept, true_slope in [
            ([1, 2, 3], [0, 0, 0], 0, 0),
            ([1, 2, 3], [1, 2, 3], 0, 1),
            ([1, 2, 3], [100, 100, 100], 100, 0),
            ([1, 2, 3], [12, 14, 16], 10, 2),
            ([1, 2, 3], [-1, -2, -3], 0, -1),
            ([1, 2, 3], [21, 22, 23], 20, 1),
            ([1, 2, 3], [5.1, 5.2, 5.3], 5, 0.1),
        ]:
            slope, intercept = statistics.linear_regression(x, y)
            self.assertAlmostEqual(intercept, true_intercept)
            self.assertAlmostEqual(slope, true_slope)


class TestNormalDist:

    # General note on precision: The pdf(), cdf(), and overlap() methods
    # depend on functions in the math libraries that do not make
    # explicit accuracy guarantees.  Accordingly, some of the accuracy
    # tests below may fail if the underlying math functions are
    # inaccurate.  There isn't much we can do about this short of
    # implementing our own implementations from scratch.

    def test_slots(self):
        nd = self.module.NormalDist(300, 23)
        with self.assertRaises(TypeError):
            vars(nd)
        self.assertEqual(tuple(nd.__slots__), ('_mu', '_sigma'))

    def test_instantiation_and_attributes(self):
        nd = self.module.NormalDist(500, 17)
        self.assertEqual(nd.mean, 500)
        self.assertEqual(nd.stdev, 17)
        self.assertEqual(nd.variance, 17**2)

        # default arguments
        nd = self.module.NormalDist()
        self.assertEqual(nd.mean, 0)
        self.assertEqual(nd.stdev, 1)
        self.assertEqual(nd.variance, 1**2)

        # error case: negative sigma
        with self.assertRaises(self.module.StatisticsError):
            self.module.NormalDist(500, -10)

        # verify that subclass type is honored
        class NewNormalDist(self.module.NormalDist):
            pass
        nnd = NewNormalDist(200, 5)
        self.assertEqual(type(nnd), NewNormalDist)

    def test_alternative_constructor(self):
        NormalDist = self.module.NormalDist
        data = [96, 107, 90, 92, 110]
        # list input
        self.assertEqual(NormalDist.from_samples(data), NormalDist(99, 9))
        # tuple input
        self.assertEqual(NormalDist.from_samples(tuple(data)), NormalDist(99, 9))
        # iterator input
        self.assertEqual(NormalDist.from_samples(iter(data)), NormalDist(99, 9))
        # error cases
        with self.assertRaises(self.module.StatisticsError):
            NormalDist.from_samples([])                      # empty input
        with self.assertRaises(self.module.StatisticsError):
            NormalDist.from_samples([10])                    # only one input

        # verify that subclass type is honored
        class NewNormalDist(NormalDist):
            pass
        nnd = NewNormalDist.from_samples(data)
        self.assertEqual(type(nnd), NewNormalDist)

    def test_sample_generation(self):
        NormalDist = self.module.NormalDist
        mu, sigma = 10_000, 3.0
        X = NormalDist(mu, sigma)
        n = 1_000
        data = X.samples(n)
        self.assertEqual(len(data), n)
        self.assertEqual(set(map(type, data)), {float})
        # mean(data) expected to fall within 8 standard deviations
        xbar = self.module.mean(data)
        self.assertTrue(mu - sigma*8 <= xbar <= mu + sigma*8)

        # verify that seeding makes reproducible sequences
        n = 100
        data1 = X.samples(n, seed='happiness and joy')
        data2 = X.samples(n, seed='trouble and despair')
        data3 = X.samples(n, seed='happiness and joy')
        data4 = X.samples(n, seed='trouble and despair')
        self.assertEqual(data1, data3)
        self.assertEqual(data2, data4)
        self.assertNotEqual(data1, data2)

    def test_pdf(self):
        NormalDist = self.module.NormalDist
        X = NormalDist(100, 15)
        # Verify peak around center
        self.assertLess(X.pdf(99), X.pdf(100))
        self.assertLess(X.pdf(101), X.pdf(100))
        # Test symmetry
        for i in range(50):
            self.assertAlmostEqual(X.pdf(100 - i), X.pdf(100 + i))
        # Test vs CDF
        dx = 2.0 ** -10
        for x in range(90, 111):
            est_pdf = (X.cdf(x + dx) - X.cdf(x)) / dx
            self.assertAlmostEqual(X.pdf(x), est_pdf, places=4)
        # Test vs table of known values -- CRC 26th Edition
        Z = NormalDist()
        for x, px in enumerate([
            0.3989, 0.3989, 0.3989, 0.3988, 0.3986,
            0.3984, 0.3982, 0.3980, 0.3977, 0.3973,
            0.3970, 0.3965, 0.3961, 0.3956, 0.3951,
            0.3945, 0.3939, 0.3932, 0.3925, 0.3918,
            0.3910, 0.3902, 0.3894, 0.3885, 0.3876,
            0.3867, 0.3857, 0.3847, 0.3836, 0.3825,
            0.3814, 0.3802, 0.3790, 0.3778, 0.3765,
            0.3752, 0.3739, 0.3725, 0.3712, 0.3697,
            0.3683, 0.3668, 0.3653, 0.3637, 0.3621,
            0.3605, 0.3589, 0.3572, 0.3555, 0.3538,
        ]):
            self.assertAlmostEqual(Z.pdf(x / 100.0), px, places=4)
            self.assertAlmostEqual(Z.pdf(-x / 100.0), px, places=4)
        # Error case: variance is zero
        Y = NormalDist(100, 0)
        with self.assertRaises(self.module.StatisticsError):
            Y.pdf(90)
        # Special values
        self.assertEqual(X.pdf(float('-Inf')), 0.0)
        self.assertEqual(X.pdf(float('Inf')), 0.0)
        self.assertTrue(math.isnan(X.pdf(float('NaN'))))

    def test_cdf(self):
        NormalDist = self.module.NormalDist
        X = NormalDist(100, 15)
        cdfs = [X.cdf(x) for x in range(1, 200)]
        self.assertEqual(set(map(type, cdfs)), {float})
        # Verify montonic
        self.assertEqual(cdfs, sorted(cdfs))
        # Verify center (should be exact)
        self.assertEqual(X.cdf(100), 0.50)
        # Check against a table of known values
        # https://en.wikipedia.org/wiki/Standard_normal_table#Cumulative
        Z = NormalDist()
        for z, cum_prob in [
            (0.00, 0.50000), (0.01, 0.50399), (0.02, 0.50798),
            (0.14, 0.55567), (0.29, 0.61409), (0.33, 0.62930),
            (0.54, 0.70540), (0.60, 0.72575), (1.17, 0.87900),
            (1.60, 0.94520), (2.05, 0.97982), (2.89, 0.99807),
            (3.52, 0.99978), (3.98, 0.99997), (4.07, 0.99998),
            ]:
            self.assertAlmostEqual(Z.cdf(z), cum_prob, places=5)
            self.assertAlmostEqual(Z.cdf(-z), 1.0 - cum_prob, places=5)
        # Error case: variance is zero
        Y = NormalDist(100, 0)
        with self.assertRaises(self.module.StatisticsError):
            Y.cdf(90)
        # Special values
        self.assertEqual(X.cdf(float('-Inf')), 0.0)
        self.assertEqual(X.cdf(float('Inf')), 1.0)
        self.assertTrue(math.isnan(X.cdf(float('NaN'))))

    @support.skip_if_pgo_task
    def test_inv_cdf(self):
        NormalDist = self.module.NormalDist

        # Center case should be exact.
        iq = NormalDist(100, 15)
        self.assertEqual(iq.inv_cdf(0.50), iq.mean)

        # Test versus a published table of known percentage points.
        # See the second table at the bottom of the page here:
        # http://people.bath.ac.uk/masss/tables/normaltable.pdf
        Z = NormalDist()
        pp = {5.0: (0.000, 1.645, 2.576, 3.291, 3.891,
                    4.417, 4.892, 5.327, 5.731, 6.109),
              2.5: (0.674, 1.960, 2.807, 3.481, 4.056,
                    4.565, 5.026, 5.451, 5.847, 6.219),
              1.0: (1.282, 2.326, 3.090, 3.719, 4.265,
                    4.753, 5.199, 5.612, 5.998, 6.361)}
        for base, row in pp.items():
            for exp, x in enumerate(row, start=1):
                p = base * 10.0 ** (-exp)
                self.assertAlmostEqual(-Z.inv_cdf(p), x, places=3)
                p = 1.0 - p
                self.assertAlmostEqual(Z.inv_cdf(p), x, places=3)

        # Match published example for MS Excel
        # https://support.office.com/en-us/article/norm-inv-function-54b30935-fee7-493c-bedb-2278a9db7e13
        self.assertAlmostEqual(NormalDist(40, 1.5).inv_cdf(0.908789), 42.000002)

        # One million equally spaced probabilities
        n = 2**20
        for p in range(1, n):
            p /= n
            self.assertAlmostEqual(iq.cdf(iq.inv_cdf(p)), p)

        # One hundred ever smaller probabilities to test tails out to
        # extreme probabilities: 1 / 2**50 and (2**50-1) / 2 ** 50
        for e in range(1, 51):
            p = 2.0 ** (-e)
            self.assertAlmostEqual(iq.cdf(iq.inv_cdf(p)), p)
            p = 1.0 - p
            self.assertAlmostEqual(iq.cdf(iq.inv_cdf(p)), p)

        # Now apply cdf() first.  Near the tails, the round-trip loses
        # precision and is ill-conditioned (small changes in the inputs
        # give large changes in the output), so only check to 5 places.
        for x in range(200):
            self.assertAlmostEqual(iq.inv_cdf(iq.cdf(x)), x, places=5)

        # Error cases:
        with self.assertRaises(self.module.StatisticsError):
            iq.inv_cdf(0.0)                         # p is zero
        with self.assertRaises(self.module.StatisticsError):
            iq.inv_cdf(-0.1)                        # p under zero
        with self.assertRaises(self.module.StatisticsError):
            iq.inv_cdf(1.0)                         # p is one
        with self.assertRaises(self.module.StatisticsError):
            iq.inv_cdf(1.1)                         # p over one
        with self.assertRaises(self.module.StatisticsError):
            iq = NormalDist(100, 0)                 # sigma is zero
            iq.inv_cdf(0.5)

        # Special values
        self.assertTrue(math.isnan(Z.inv_cdf(float('NaN'))))

    def test_quantiles(self):
        # Quartiles of a standard normal distribution
        Z = self.module.NormalDist()
        for n, expected in [
            (1, []),
            (2, [0.0]),
            (3, [-0.4307, 0.4307]),
            (4 ,[-0.6745, 0.0, 0.6745]),
                ]:
            actual = Z.quantiles(n=n)
            self.assertTrue(all(math.isclose(e, a, abs_tol=0.0001)
                            for e, a in zip(expected, actual)))

    def test_overlap(self):
        NormalDist = self.module.NormalDist

        # Match examples from Imman and Bradley
        for X1, X2, published_result in [
                (NormalDist(0.0, 2.0), NormalDist(1.0, 2.0), 0.80258),
                (NormalDist(0.0, 1.0), NormalDist(1.0, 2.0), 0.60993),
            ]:
            self.assertAlmostEqual(X1.overlap(X2), published_result, places=4)
            self.assertAlmostEqual(X2.overlap(X1), published_result, places=4)

        # Check against integration of the PDF
        def overlap_numeric(X, Y, *, steps=8_192, z=5):
            'Numerical integration cross-check for overlap() '
            fsum = math.fsum
            center = (X.mean + Y.mean) / 2.0
            width = z * max(X.stdev, Y.stdev)
            start = center - width
            dx = 2.0 * width / steps
            x_arr = [start + i*dx for i in range(steps)]
            xp = list(map(X.pdf, x_arr))
            yp = list(map(Y.pdf, x_arr))
            total = max(fsum(xp), fsum(yp))
            return fsum(map(min, xp, yp)) / total

        for X1, X2 in [
                # Examples from Imman and Bradley
                (NormalDist(0.0, 2.0), NormalDist(1.0, 2.0)),
                (NormalDist(0.0, 1.0), NormalDist(1.0, 2.0)),
                # Example from https://www.rasch.org/rmt/rmt101r.htm
                (NormalDist(0.0, 1.0), NormalDist(1.0, 2.0)),
                # Gender heights from http://www.usablestats.com/lessons/normal
                (NormalDist(70, 4), NormalDist(65, 3.5)),
                # Misc cases with equal standard deviations
                (NormalDist(100, 15), NormalDist(110, 15)),
                (NormalDist(-100, 15), NormalDist(110, 15)),
                (NormalDist(-100, 15), NormalDist(-110, 15)),
                # Misc cases with unequal standard deviations
                (NormalDist(100, 12), NormalDist(100, 15)),
                (NormalDist(100, 12), NormalDist(110, 15)),
                (NormalDist(100, 12), NormalDist(150, 15)),
                (NormalDist(100, 12), NormalDist(150, 35)),
                # Misc cases with small values
                (NormalDist(1.000, 0.002), NormalDist(1.001, 0.003)),
                (NormalDist(1.000, 0.002), NormalDist(1.006, 0.0003)),
                (NormalDist(1.000, 0.002), NormalDist(1.001, 0.099)),
            ]:
            self.assertAlmostEqual(X1.overlap(X2), overlap_numeric(X1, X2), places=5)
            self.assertAlmostEqual(X2.overlap(X1), overlap_numeric(X1, X2), places=5)

        # Error cases
        X = NormalDist()
        with self.assertRaises(TypeError):
            X.overlap()                             # too few arguments
        with self.assertRaises(TypeError):
            X.overlap(X, X)                         # too may arguments
        with self.assertRaises(TypeError):
            X.overlap(None)                         # right operand not a NormalDist
        with self.assertRaises(self.module.StatisticsError):
            X.overlap(NormalDist(1, 0))             # right operand sigma is zero
        with self.assertRaises(self.module.StatisticsError):
            NormalDist(1, 0).overlap(X)             # left operand sigma is zero

    def test_zscore(self):
        NormalDist = self.module.NormalDist
        X = NormalDist(100, 15)
        self.assertEqual(X.zscore(142), 2.8)
        self.assertEqual(X.zscore(58), -2.8)
        self.assertEqual(X.zscore(100), 0.0)
        with self.assertRaises(TypeError):
            X.zscore()                              # too few arguments
        with self.assertRaises(TypeError):
            X.zscore(1, 1)                          # too may arguments
        with self.assertRaises(TypeError):
            X.zscore(None)                          # non-numeric type
        with self.assertRaises(self.module.StatisticsError):
            NormalDist(1, 0).zscore(100)            # sigma is zero

    def test_properties(self):
        X = self.module.NormalDist(100, 15)
        self.assertEqual(X.mean, 100)
        self.assertEqual(X.median, 100)
        self.assertEqual(X.mode, 100)
        self.assertEqual(X.stdev, 15)
        self.assertEqual(X.variance, 225)

    def test_same_type_addition_and_subtraction(self):
        NormalDist = self.module.NormalDist
        X = NormalDist(100, 12)
        Y = NormalDist(40, 5)
        self.assertEqual(X + Y, NormalDist(140, 13))        # __add__
        self.assertEqual(X - Y, NormalDist(60, 13))         # __sub__

    def test_translation_and_scaling(self):
        NormalDist = self.module.NormalDist
        X = NormalDist(100, 15)
        y = 10
        self.assertEqual(+X, NormalDist(100, 15))           # __pos__
        self.assertEqual(-X, NormalDist(-100, 15))          # __neg__
        self.assertEqual(X + y, NormalDist(110, 15))        # __add__
        self.assertEqual(y + X, NormalDist(110, 15))        # __radd__
        self.assertEqual(X - y, NormalDist(90, 15))         # __sub__
        self.assertEqual(y - X, NormalDist(-90, 15))        # __rsub__
        self.assertEqual(X * y, NormalDist(1000, 150))      # __mul__
        self.assertEqual(y * X, NormalDist(1000, 150))      # __rmul__
        self.assertEqual(X / y, NormalDist(10, 1.5))        # __truediv__
        with self.assertRaises(TypeError):                  # __rtruediv__
            y / X

    def test_unary_operations(self):
        NormalDist = self.module.NormalDist
        X = NormalDist(100, 12)
        Y = +X
        self.assertIsNot(X, Y)
        self.assertEqual(X.mean, Y.mean)
        self.assertEqual(X.stdev, Y.stdev)
        Y = -X
        self.assertIsNot(X, Y)
        self.assertEqual(X.mean, -Y.mean)
        self.assertEqual(X.stdev, Y.stdev)

    def test_equality(self):
        NormalDist = self.module.NormalDist
        nd1 = NormalDist()
        nd2 = NormalDist(2, 4)
        nd3 = NormalDist()
        nd4 = NormalDist(2, 4)
        nd5 = NormalDist(2, 8)
        nd6 = NormalDist(8, 4)
        self.assertNotEqual(nd1, nd2)
        self.assertEqual(nd1, nd3)
        self.assertEqual(nd2, nd4)
        self.assertNotEqual(nd2, nd5)
        self.assertNotEqual(nd2, nd6)

        # Test NotImplemented when types are different
        class A:
            def __eq__(self, other):
                return 10
        a = A()
        self.assertEqual(nd1.__eq__(a), NotImplemented)
        self.assertEqual(nd1 == a, 10)
        self.assertEqual(a == nd1, 10)

        # All subclasses to compare equal giving the same behavior
        # as list, tuple, int, float, complex, str, dict, set, etc.
        class SizedNormalDist(NormalDist):
            def __init__(self, mu, sigma, n):
                super().__init__(mu, sigma)
                self.n = n
        s = SizedNormalDist(100, 15, 57)
        nd4 = NormalDist(100, 15)
        self.assertEqual(s, nd4)

        # Don't allow duck type equality because we wouldn't
        # want a lognormal distribution to compare equal
        # to a normal distribution with the same parameters
        class LognormalDist:
            def __init__(self, mu, sigma):
                self.mu = mu
                self.sigma = sigma
        lnd = LognormalDist(100, 15)
        nd = NormalDist(100, 15)
        self.assertNotEqual(nd, lnd)

    def test_pickle_and_copy(self):
        nd = self.module.NormalDist(37.5, 5.625)
        nd1 = copy.copy(nd)
        self.assertEqual(nd, nd1)
        nd2 = copy.deepcopy(nd)
        self.assertEqual(nd, nd2)
        nd3 = pickle.loads(pickle.dumps(nd))
        self.assertEqual(nd, nd3)

    def test_hashability(self):
        ND = self.module.NormalDist
        s = {ND(100, 15), ND(100.0, 15.0), ND(100, 10), ND(95, 15), ND(100, 15)}
        self.assertEqual(len(s), 3)

    def test_repr(self):
        nd = self.module.NormalDist(37.5, 5.625)
        self.assertEqual(repr(nd), 'NormalDist(mu=37.5, sigma=5.625)')

# Swapping the sys.modules['statistics'] is to solving the
# _pickle.PicklingError:
# Can't pickle <class 'statistics.NormalDist'>:
# it's not the same object as statistics.NormalDist
class TestNormalDistPython(unittest.TestCase, TestNormalDist):
    module = py_statistics
    def setUp(self):
        sys.modules['statistics'] = self.module

    def tearDown(self):
        sys.modules['statistics'] = statistics


@unittest.skipUnless(c_statistics, 'requires _statistics')
class TestNormalDistC(unittest.TestCase, TestNormalDist):
    module = c_statistics
    def setUp(self):
        sys.modules['statistics'] = self.module

    def tearDown(self):
        sys.modules['statistics'] = statistics


# === Run tests ===

def load_tests(loader, tests, ignore):
    """Used for doctest/unittest integration."""
    tests.addTests(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
