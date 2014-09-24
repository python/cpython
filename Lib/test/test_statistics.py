"""Test suite for statistics module, including helper NumericTestCase and
approx_equal function.

"""

import collections
import decimal
import doctest
import math
import random
import sys
import types
import unittest

from decimal import Decimal
from fractions import Fraction


# Module to be tested.
import statistics


# === Helper functions and class ===

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
                isinstance(first, collections.Sequence) and
                isinstance(second, collections.Sequence)
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
        failed, tried = doctest.testmod(statistics)
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
        self.assertEqual(_exact_ratio(D("0.125")), (125, 1000))
        self.assertEqual(_exact_ratio(D("12.345")), (12345, 1000))
        self.assertEqual(_exact_ratio(D("-1.98")), (-198, 100))


class DecimalToRatioTest(unittest.TestCase):
    # Test _decimal_to_ratio private function.

    def testSpecialsRaise(self):
        # Test that NANs and INFs raise ValueError.
        # Non-special values are covered by _exact_ratio above.
        for d in (Decimal('NAN'), Decimal('sNAN'), Decimal('INF')):
            self.assertRaises(ValueError, statistics._decimal_to_ratio, d)

    def test_sign(self):
        # Test sign is calculated correctly.
        numbers = [Decimal("9.8765e12"), Decimal("9.8765e-12")]
        for d in numbers:
            # First test positive decimals.
            assert d > 0
            num, den = statistics._decimal_to_ratio(d)
            self.assertGreaterEqual(num, 0)
            self.assertGreater(den, 0)
            # Then test negative decimals.
            num, den = statistics._decimal_to_ratio(-d)
            self.assertLessEqual(num, 0)
            self.assertGreater(den, 0)

    def test_negative_exponent(self):
        # Test result when the exponent is negative.
        t = statistics._decimal_to_ratio(Decimal("0.1234"))
        self.assertEqual(t, (1234, 10000))

    def test_positive_exponent(self):
        # Test results when the exponent is positive.
        t = statistics._decimal_to_ratio(Decimal("1.234e7"))
        self.assertEqual(t, (12340000, 1))

    def test_regression_20536(self):
        # Regression test for issue 20536.
        # See http://bugs.python.org/issue20536
        t = statistics._decimal_to_ratio(Decimal("1e2"))
        self.assertEqual(t, (100, 1))
        t = statistics._decimal_to_ratio(Decimal("1.47e5"))
        self.assertEqual(t, (147000, 1))


class CheckTypeTest(unittest.TestCase):
    # Test _check_type private function.

    def test_allowed(self):
        # Test that a type which should be allowed is allowed.
        allowed = set([int, float])
        statistics._check_type(int, allowed)
        statistics._check_type(float, allowed)

    def test_not_allowed(self):
        # Test that a type which should not be allowed raises.
        allowed = set([int, float])
        self.assertRaises(TypeError, statistics._check_type, Decimal, allowed)

    def test_add_to_allowed(self):
        # Test that a second type will be added to the allowed set.
        allowed = set([int])
        statistics._check_type(float, allowed)
        self.assertEqual(allowed, set([int, float]))


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
    def test_types_conserved(self):
        # Test that functions keeps the same type as their data points.
        # (Excludes mixed data types.) This only tests the type of the return
        # result, not the value.
        class MyFloat(float):
            def __truediv__(self, other):
                return type(self)(super().__truediv__(other))
            def __sub__(self, other):
                return type(self)(super().__sub__(other))
            def __rsub__(self, other):
                return type(self)(super().__rsub__(other))
            def __pow__(self, other):
                return type(self)(super().__pow__(other))
            def __add__(self, other):
                return type(self)(super().__add__(other))
            __radd__ = __add__

        data = self.prepare_data()
        for kind in (float, Decimal, Fraction, MyFloat):
            d = [kind(x) for x in data]
            result = self.func(d)
            self.assertIs(type(result), kind)


class TestSum(NumericTestCase, UnivariateCommonMixin, UnivariateTypeMixin):
    # Test cases for statistics._sum() function.

    def setUp(self):
        self.func = statistics._sum

    def test_empty_data(self):
        # Override test for empty data.
        for data in ([], (), iter([])):
            self.assertEqual(self.func(data), 0)
            self.assertEqual(self.func(data, 23), 23)
            self.assertEqual(self.func(data, 2.3), 2.3)

    def test_ints(self):
        self.assertEqual(self.func([1, 5, 3, -4, -8, 20, 42, 1]), 60)
        self.assertEqual(self.func([4, 2, 3, -8, 7], 1000), 1008)

    def test_floats(self):
        self.assertEqual(self.func([0.25]*20), 5.0)
        self.assertEqual(self.func([0.125, 0.25, 0.5, 0.75], 1.5), 3.125)

    def test_fractions(self):
        F = Fraction
        self.assertEqual(self.func([Fraction(1, 1000)]*500), Fraction(1, 2))

    def test_decimals(self):
        D = Decimal
        data = [D("0.001"), D("5.246"), D("1.702"), D("-0.025"),
                D("3.974"), D("2.328"), D("4.617"), D("2.843"),
                ]
        self.assertEqual(self.func(data), Decimal("20.686"))

    def test_compare_with_math_fsum(self):
        # Compare with the math.fsum function.
        # Ideally we ought to get the exact same result, but sometimes
        # we differ by a very slight amount :-(
        data = [random.uniform(-100, 1000) for _ in range(1000)]
        self.assertApproxEqual(self.func(data), math.fsum(data), rel=2e-16)

    def test_start_argument(self):
        # Test that the optional start argument works correctly.
        data = [random.uniform(1, 1000) for _ in range(100)]
        t = self.func(data)
        self.assertEqual(t+42, self.func(data, 42))
        self.assertEqual(t-23, self.func(data, -23))
        self.assertEqual(t+1e20, self.func(data, 1e20))

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
        self.assertRaises(TypeError, self.func, [1, 2.0, Fraction(1, 2)])
        # And so does mixed start argument.
        self.assertRaises(TypeError, self.func, [1, 2.0], Decimal(1))


class SumTortureTest(NumericTestCase):
    def test_torture(self):
        # Tim Peters' torture test for sum, and variants of same.
        self.assertEqual(statistics._sum([1, 1e100, 1, -1e100]*10000), 20000.0)
        self.assertEqual(statistics._sum([1e100, 1, 1, -1e100]*10000), 20000.0)
        self.assertApproxEqual(
            statistics._sum([1e-100, 1, 1e-100, -1]*10000), 2.0e-96, rel=5e-16
            )


class SumSpecialValues(NumericTestCase):
    # Test that sum works correctly with IEEE-754 special values.

    def test_nan(self):
        for type_ in (float, Decimal):
            nan = type_('nan')
            result = statistics._sum([1, nan, 2])
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
        result = statistics._sum([1, 2, inf, 3])
        self.check_infinity(result, inf)
        # Adding two infinities of the same sign also gives infinity.
        result = statistics._sum([1, 2, inf, 3, inf, 4])
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
        result = statistics._sum([1, 2, inf, 3, -inf, 4])
        self.assertTrue(math.isnan(result))

    def test_decimal_extendedcontext_mismatched_infs_to_nan(self):
        # Test adding Decimal INFs with opposite sign returns NAN.
        inf = Decimal('inf')
        data = [1, 2, inf, 3, -inf, 4]
        with decimal.localcontext(decimal.ExtendedContext):
            self.assertTrue(math.isnan(statistics._sum(data)))

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

    def test_repeated_single_value(self):
        # The average of a single repeated value is the value itself.
        for x in (3.5, 17, 2.5e15, Fraction(61, 67), Decimal('4.9712')):
            for count in (2, 5, 10, 20):
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
        # Test mean with ints.
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
        self.assertRaises(statistics.StatisticsError, self.func, data)

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
        # Check for an exception.
        self.assertRaises(statistics.StatisticsError, self.func, data)

    def test_unique_data_failure(self):
        # Test mode exception when data points are all unique.
        data = list(range(10))
        self.assertRaises(statistics.StatisticsError, self.func, data)

    def test_none_data(self):
        # Test that mode raises TypeError if given None as data.

        # This test is necessary because the implementation of mode uses
        # collections.Counter, which accepts None and returns an empty dict.
        self.assertRaises(TypeError, self.func, None)

    def test_counter_data(self):
        # Test that a Counter is treated like any other iterable.
        data = collections.Counter([1, 1, 1, 2])
        # Since the keys of the counter are treated as data points, not the
        # counts, this should raise.
        self.assertRaises(statistics.StatisticsError, self.func, data)



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


class TestPStdev(VarianceStdevMixin, NumericTestCase):
    # Tests for population standard deviation.
    def setUp(self):
        self.func = statistics.pstdev

    def test_compare_to_variance(self):
        # Test that stdev is, in fact, the square root of variance.
        data = [random.uniform(-17, 24) for _ in range(1000)]
        expected = math.sqrt(statistics.pvariance(data))
        self.assertEqual(self.func(data), expected)


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


# === Run tests ===

def load_tests(loader, tests, ignore):
    """Used for doctest/unittest integration."""
    tests.addTests(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
