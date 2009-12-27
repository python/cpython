from __future__ import division
# When true division is the default, get rid of this and add it to
# test_long.py instead.  In the meantime, it's too obscure to try to
# trick just part of test_long into using future division.

import sys
import random
import math
import unittest
from test.test_support import run_unittest

# decorator for skipping tests on non-IEEE 754 platforms
requires_IEEE_754 = unittest.skipUnless(
    float.__getformat__("double").startswith("IEEE"),
    "test requires IEEE 754 doubles")

DBL_MAX = sys.float_info.max
DBL_MAX_EXP = sys.float_info.max_exp
DBL_MIN_EXP = sys.float_info.min_exp
DBL_MANT_DIG = sys.float_info.mant_dig
DBL_MIN_OVERFLOW = 2**DBL_MAX_EXP - 2**(DBL_MAX_EXP - DBL_MANT_DIG - 1)

# pure Python version of correctly-rounded true division
def truediv(a, b):
    """Correctly-rounded true division for integers."""
    negative = a^b < 0
    a, b = abs(a), abs(b)

    # exceptions:  division by zero, overflow
    if not b:
        raise ZeroDivisionError("division by zero")
    if a >= DBL_MIN_OVERFLOW * b:
        raise OverflowError("int/int too large to represent as a float")

   # find integer d satisfying 2**(d - 1) <= a/b < 2**d
    d = a.bit_length() - b.bit_length()
    if d >= 0 and a >= 2**d * b or d < 0 and a * 2**-d >= b:
        d += 1

    # compute 2**-exp * a / b for suitable exp
    exp = max(d, DBL_MIN_EXP) - DBL_MANT_DIG
    a, b = a << max(-exp, 0), b << max(exp, 0)
    q, r = divmod(a, b)

    # round-half-to-even: fractional part is r/b, which is > 0.5 iff
    # 2*r > b, and == 0.5 iff 2*r == b.
    if 2*r > b or 2*r == b and q % 2 == 1:
        q += 1

    result = math.ldexp(float(q), exp)
    return -result if negative else result

class TrueDivisionTests(unittest.TestCase):
    def test(self):
        huge = 1L << 40000
        mhuge = -huge
        self.assertEqual(huge / huge, 1.0)
        self.assertEqual(mhuge / mhuge, 1.0)
        self.assertEqual(huge / mhuge, -1.0)
        self.assertEqual(mhuge / huge, -1.0)
        self.assertEqual(1 / huge, 0.0)
        self.assertEqual(1L / huge, 0.0)
        self.assertEqual(1 / mhuge, 0.0)
        self.assertEqual(1L / mhuge, 0.0)
        self.assertEqual((666 * huge + (huge >> 1)) / huge, 666.5)
        self.assertEqual((666 * mhuge + (mhuge >> 1)) / mhuge, 666.5)
        self.assertEqual((666 * huge + (huge >> 1)) / mhuge, -666.5)
        self.assertEqual((666 * mhuge + (mhuge >> 1)) / huge, -666.5)
        self.assertEqual(huge / (huge << 1), 0.5)
        self.assertEqual((1000000 * huge) / huge, 1000000)

        namespace = {'huge': huge, 'mhuge': mhuge}

        for overflow in ["float(huge)", "float(mhuge)",
                         "huge / 1", "huge / 2L", "huge / -1", "huge / -2L",
                         "mhuge / 100", "mhuge / 100L"]:
            # If the "eval" does not happen in this module,
            # true division is not enabled
            with self.assertRaises(OverflowError):
                eval(overflow, namespace)

        for underflow in ["1 / huge", "2L / huge", "-1 / huge", "-2L / huge",
                         "100 / mhuge", "100L / mhuge"]:
            result = eval(underflow, namespace)
            self.assertEqual(result, 0.0, 'expected underflow to 0 '
                             'from {!r}'.format(underflow))

        for zero in ["huge / 0", "huge / 0L", "mhuge / 0", "mhuge / 0L"]:
            with self.assertRaises(ZeroDivisionError):
                eval(zero, namespace)

    def check_truediv(self, a, b, skip_small=True):
        """Verify that the result of a/b is correctly rounded, by
        comparing it with a pure Python implementation of correctly
        rounded division.  b should be nonzero."""

        a, b = long(a), long(b)

        # skip check for small a and b: in this case, the current
        # implementation converts the arguments to float directly and
        # then applies a float division.  This can give doubly-rounded
        # results on x87-using machines (particularly 32-bit Linux).
        if skip_small and max(abs(a), abs(b)) < 2**DBL_MANT_DIG:
            return

        try:
            # use repr so that we can distinguish between -0.0 and 0.0
            expected = repr(truediv(a, b))
        except OverflowError:
            expected = 'overflow'
        except ZeroDivisionError:
            expected = 'zerodivision'

        try:
            got = repr(a / b)
        except OverflowError:
            got = 'overflow'
        except ZeroDivisionError:
            got = 'zerodivision'

        self.assertEqual(expected, got, "Incorrectly rounded division {}/{}: "
                         "expected {}, got {}".format(a, b, expected, got))

    @requires_IEEE_754
    def test_correctly_rounded_true_division(self):
        # more stringent tests than those above, checking that the
        # result of true division of ints is always correctly rounded.
        # This test should probably be considered CPython-specific.

        # Exercise all the code paths not involving Gb-sized ints.
        # ... divisions involving zero
        self.check_truediv(123, 0)
        self.check_truediv(-456, 0)
        self.check_truediv(0, 3)
        self.check_truediv(0, -3)
        self.check_truediv(0, 0)
        # ... overflow or underflow by large margin
        self.check_truediv(671 * 12345 * 2**DBL_MAX_EXP, 12345)
        self.check_truediv(12345, 345678 * 2**(DBL_MANT_DIG - DBL_MIN_EXP))
        # ... a much larger or smaller than b
        self.check_truediv(12345*2**100, 98765)
        self.check_truediv(12345*2**30, 98765*7**81)
        # ... a / b near a boundary: one of 1, 2**DBL_MANT_DIG, 2**DBL_MIN_EXP,
        #                 2**DBL_MAX_EXP, 2**(DBL_MIN_EXP-DBL_MANT_DIG)
        bases = (0, DBL_MANT_DIG, DBL_MIN_EXP,
                 DBL_MAX_EXP, DBL_MIN_EXP - DBL_MANT_DIG)
        for base in bases:
            for exp in range(base - 15, base + 15):
                self.check_truediv(75312*2**max(exp, 0), 69187*2**max(-exp, 0))
                self.check_truediv(69187*2**max(exp, 0), 75312*2**max(-exp, 0))

        # overflow corner case
        for m in [1, 2, 7, 17, 12345, 7**100,
                  -1, -2, -5, -23, -67891, -41**50]:
            for n in range(-10, 10):
                self.check_truediv(m*DBL_MIN_OVERFLOW + n, m)
                self.check_truediv(m*DBL_MIN_OVERFLOW + n, -m)

        # check detection of inexactness in shifting stage
        for n in range(250):
            # (2**DBL_MANT_DIG+1)/(2**DBL_MANT_DIG) lies halfway
            # between two representable floats, and would usually be
            # rounded down under round-half-to-even.  The tiniest of
            # additions to the numerator should cause it to be rounded
            # up instead.
            self.check_truediv((2**DBL_MANT_DIG + 1)*12345*2**200 + 2**n,
                           2**DBL_MANT_DIG*12345)

        # 1/2731 is one of the smallest division cases that's subject
        # to double rounding on IEEE 754 machines working internally with
        # 64-bit precision.  On such machines, the next check would fail,
        # were it not explicitly skipped in check_truediv.
        self.check_truediv(1, 2731)

        # a particularly bad case for the old algorithm:  gives an
        # error of close to 3.5 ulps.
        self.check_truediv(295147931372582273023, 295147932265116303360)
        for i in range(1000):
            self.check_truediv(10**(i+1), 10**i)
            self.check_truediv(10**i, 10**(i+1))

        # test round-half-to-even behaviour, normal result
        for m in [1, 2, 4, 7, 8, 16, 17, 32, 12345, 7**100,
                  -1, -2, -5, -23, -67891, -41**50]:
            for n in range(-10, 10):
                self.check_truediv(2**DBL_MANT_DIG*m + n, m)

        # test round-half-to-even, subnormal result
        for n in range(-20, 20):
            self.check_truediv(n, 2**1076)

        # largeish random divisions: a/b where |a| <= |b| <=
        # 2*|a|; |ans| is between 0.5 and 1.0, so error should
        # always be bounded by 2**-54 with equality possible only
        # if the least significant bit of q=ans*2**53 is zero.
        for M in [10**10, 10**100, 10**1000]:
            for i in range(1000):
                a = random.randrange(1, M)
                b = random.randrange(a, 2*a+1)
                self.check_truediv(a, b)
                self.check_truediv(-a, b)
                self.check_truediv(a, -b)
                self.check_truediv(-a, -b)

        # and some (genuinely) random tests
        for _ in range(10000):
            a_bits = random.randrange(1000)
            b_bits = random.randrange(1, 1000)
            x = random.randrange(2**a_bits)
            y = random.randrange(1, 2**b_bits)
            self.check_truediv(x, y)
            self.check_truediv(x, -y)
            self.check_truediv(-x, y)
            self.check_truediv(-x, -y)


def test_main():
    run_unittest(TrueDivisionTests)

if __name__ == "__main__":
    test_main()
