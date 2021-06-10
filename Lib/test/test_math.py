# Python test set -- math module
# XXXX Should not do tests around zero only

from test.support import run_unittest, verbose, requires_IEEE_754
from test import support
import unittest
import itertools
import decimal
import math
import os
import platform
import random
import struct
import sys


eps = 1E-05
NAN = float('nan')
INF = float('inf')
NINF = float('-inf')
FLOAT_MAX = sys.float_info.max
FLOAT_MIN = sys.float_info.min

# detect evidence of double-rounding: fsum is not always correctly
# rounded on machines that suffer from double rounding.
x, y = 1e16, 2.9999 # use temporary values to defeat peephole optimizer
HAVE_DOUBLE_ROUNDING = (x + y == 1e16 + 4)

# locate file with test values
if __name__ == '__main__':
    file = sys.argv[0]
else:
    file = __file__
test_dir = os.path.dirname(file) or os.curdir
math_testcases = os.path.join(test_dir, 'math_testcases.txt')
test_file = os.path.join(test_dir, 'cmath_testcases.txt')


def to_ulps(x):
    """Convert a non-NaN float x to an integer, in such a way that
    adjacent floats are converted to adjacent integers.  Then
    abs(ulps(x) - ulps(y)) gives the difference in ulps between two
    floats.

    The results from this function will only make sense on platforms
    where native doubles are represented in IEEE 754 binary64 format.

    Note: 0.0 and -0.0 are converted to 0 and -1, respectively.
    """
    n = struct.unpack('<q', struct.pack('<d', x))[0]
    if n < 0:
        n = ~(n+2**63)
    return n


# Here's a pure Python version of the math.factorial algorithm, for
# documentation and comparison purposes.
#
# Formula:
#
#   factorial(n) = factorial_odd_part(n) << (n - count_set_bits(n))
#
# where
#
#   factorial_odd_part(n) = product_{i >= 0} product_{0 < j <= n >> i; j odd} j
#
# The outer product above is an infinite product, but once i >= n.bit_length,
# (n >> i) < 1 and the corresponding term of the product is empty.  So only the
# finitely many terms for 0 <= i < n.bit_length() contribute anything.
#
# We iterate downwards from i == n.bit_length() - 1 to i == 0.  The inner
# product in the formula above starts at 1 for i == n.bit_length(); for each i
# < n.bit_length() we get the inner product for i from that for i + 1 by
# multiplying by all j in {n >> i+1 < j <= n >> i; j odd}.  In Python terms,
# this set is range((n >> i+1) + 1 | 1, (n >> i) + 1 | 1, 2).

def count_set_bits(n):
    """Number of '1' bits in binary expansion of a nonnnegative integer."""
    return 1 + count_set_bits(n & n - 1) if n else 0

def partial_product(start, stop):
    """Product of integers in range(start, stop, 2), computed recursively.
    start and stop should both be odd, with start <= stop.

    """
    numfactors = (stop - start) >> 1
    if not numfactors:
        return 1
    elif numfactors == 1:
        return start
    else:
        mid = (start + numfactors) | 1
        return partial_product(start, mid) * partial_product(mid, stop)

def py_factorial(n):
    """Factorial of nonnegative integer n, via "Binary Split Factorial Formula"
    described at http://www.luschny.de/math/factorial/binarysplitfact.html

    """
    inner = outer = 1
    for i in reversed(range(n.bit_length())):
        inner *= partial_product((n >> i + 1) + 1 | 1, (n >> i) + 1 | 1)
        outer *= inner
    return outer << (n - count_set_bits(n))

def ulp_abs_check(expected, got, ulp_tol, abs_tol):
    """Given finite floats `expected` and `got`, check that they're
    approximately equal to within the given number of ulps or the
    given absolute tolerance, whichever is bigger.

    Returns None on success and an error message on failure.
    """
    ulp_error = abs(to_ulps(expected) - to_ulps(got))
    abs_error = abs(expected - got)

    # Succeed if either abs_error <= abs_tol or ulp_error <= ulp_tol.
    if abs_error <= abs_tol or ulp_error <= ulp_tol:
        return None
    else:
        fmt = ("error = {:.3g} ({:d} ulps); "
               "permitted error = {:.3g} or {:d} ulps")
        return fmt.format(abs_error, ulp_error, abs_tol, ulp_tol)

def parse_mtestfile(fname):
    """Parse a file with test values

    -- starts a comment
    blank lines, or lines containing only a comment, are ignored
    other lines are expected to have the form
      id fn arg -> expected [flag]*

    """
    with open(fname, encoding="utf-8") as fp:
        for line in fp:
            # strip comments, and skip blank lines
            if '--' in line:
                line = line[:line.index('--')]
            if not line.strip():
                continue

            lhs, rhs = line.split('->')
            id, fn, arg = lhs.split()
            rhs_pieces = rhs.split()
            exp = rhs_pieces[0]
            flags = rhs_pieces[1:]

            yield (id, fn, float(arg), float(exp), flags)


def parse_testfile(fname):
    """Parse a file with test values

    Empty lines or lines starting with -- are ignored
    yields id, fn, arg_real, arg_imag, exp_real, exp_imag
    """
    with open(fname, encoding="utf-8") as fp:
        for line in fp:
            # skip comment lines and blank lines
            if line.startswith('--') or not line.strip():
                continue

            lhs, rhs = line.split('->')
            id, fn, arg_real, arg_imag = lhs.split()
            rhs_pieces = rhs.split()
            exp_real, exp_imag = rhs_pieces[0], rhs_pieces[1]
            flags = rhs_pieces[2:]

            yield (id, fn,
                   float(arg_real), float(arg_imag),
                   float(exp_real), float(exp_imag),
                   flags)


def result_check(expected, got, ulp_tol=5, abs_tol=0.0):
    # Common logic of MathTests.(ftest, test_testcases, test_mtestcases)
    """Compare arguments expected and got, as floats, if either
    is a float, using a tolerance expressed in multiples of
    ulp(expected) or absolutely (if given and greater).

    As a convenience, when neither argument is a float, and for
    non-finite floats, exact equality is demanded. Also, nan==nan
    as far as this function is concerned.

    Returns None on success and an error message on failure.
    """

    # Check exactly equal (applies also to strings representing exceptions)
    if got == expected:
        return None

    failure = "not equal"

    # Turn mixed float and int comparison (e.g. floor()) to all-float
    if isinstance(expected, float) and isinstance(got, int):
        got = float(got)
    elif isinstance(got, float) and isinstance(expected, int):
        expected = float(expected)

    if isinstance(expected, float) and isinstance(got, float):
        if math.isnan(expected) and math.isnan(got):
            # Pass, since both nan
            failure = None
        elif math.isinf(expected) or math.isinf(got):
            # We already know they're not equal, drop through to failure
            pass
        else:
            # Both are finite floats (now). Are they close enough?
            failure = ulp_abs_check(expected, got, ulp_tol, abs_tol)

    # arguments are not equal, and if numeric, are too far apart
    if failure is not None:
        fail_fmt = "expected {!r}, got {!r}"
        fail_msg = fail_fmt.format(expected, got)
        fail_msg += ' ({})'.format(failure)
        return fail_msg
    else:
        return None

class FloatLike:
    def __init__(self, value):
        self.value = value

    def __float__(self):
        return self.value

class IntSubclass(int):
    pass

# Class providing an __index__ method.
class MyIndexable(object):
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value

class MathTests(unittest.TestCase):

    def ftest(self, name, got, expected, ulp_tol=5, abs_tol=0.0):
        """Compare arguments expected and got, as floats, if either
        is a float, using a tolerance expressed in multiples of
        ulp(expected) or absolutely, whichever is greater.

        As a convenience, when neither argument is a float, and for
        non-finite floats, exact equality is demanded. Also, nan==nan
        in this function.
        """
        failure = result_check(expected, got, ulp_tol, abs_tol)
        if failure is not None:
            self.fail("{}: {}".format(name, failure))

    def testConstants(self):
        # Ref: Abramowitz & Stegun (Dover, 1965)
        self.ftest('pi', math.pi, 3.141592653589793238462643)
        self.ftest('e', math.e, 2.718281828459045235360287)
        self.assertEqual(math.tau, 2*math.pi)

    def testAcos(self):
        self.assertRaises(TypeError, math.acos)
        self.ftest('acos(-1)', math.acos(-1), math.pi)
        self.ftest('acos(0)', math.acos(0), math.pi/2)
        self.ftest('acos(1)', math.acos(1), 0)
        self.assertRaises(ValueError, math.acos, INF)
        self.assertRaises(ValueError, math.acos, NINF)
        self.assertRaises(ValueError, math.acos, 1 + eps)
        self.assertRaises(ValueError, math.acos, -1 - eps)
        self.assertTrue(math.isnan(math.acos(NAN)))

    def testAcosh(self):
        self.assertRaises(TypeError, math.acosh)
        self.ftest('acosh(1)', math.acosh(1), 0)
        self.ftest('acosh(2)', math.acosh(2), 1.3169578969248168)
        self.assertRaises(ValueError, math.acosh, 0)
        self.assertRaises(ValueError, math.acosh, -1)
        self.assertEqual(math.acosh(INF), INF)
        self.assertRaises(ValueError, math.acosh, NINF)
        self.assertTrue(math.isnan(math.acosh(NAN)))

    def testAsin(self):
        self.assertRaises(TypeError, math.asin)
        self.ftest('asin(-1)', math.asin(-1), -math.pi/2)
        self.ftest('asin(0)', math.asin(0), 0)
        self.ftest('asin(1)', math.asin(1), math.pi/2)
        self.assertRaises(ValueError, math.asin, INF)
        self.assertRaises(ValueError, math.asin, NINF)
        self.assertRaises(ValueError, math.asin, 1 + eps)
        self.assertRaises(ValueError, math.asin, -1 - eps)
        self.assertTrue(math.isnan(math.asin(NAN)))

    def testAsinh(self):
        self.assertRaises(TypeError, math.asinh)
        self.ftest('asinh(0)', math.asinh(0), 0)
        self.ftest('asinh(1)', math.asinh(1), 0.88137358701954305)
        self.ftest('asinh(-1)', math.asinh(-1), -0.88137358701954305)
        self.assertEqual(math.asinh(INF), INF)
        self.assertEqual(math.asinh(NINF), NINF)
        self.assertTrue(math.isnan(math.asinh(NAN)))

    def testAtan(self):
        self.assertRaises(TypeError, math.atan)
        self.ftest('atan(-1)', math.atan(-1), -math.pi/4)
        self.ftest('atan(0)', math.atan(0), 0)
        self.ftest('atan(1)', math.atan(1), math.pi/4)
        self.ftest('atan(inf)', math.atan(INF), math.pi/2)
        self.ftest('atan(-inf)', math.atan(NINF), -math.pi/2)
        self.assertTrue(math.isnan(math.atan(NAN)))

    def testAtanh(self):
        self.assertRaises(TypeError, math.atan)
        self.ftest('atanh(0)', math.atanh(0), 0)
        self.ftest('atanh(0.5)', math.atanh(0.5), 0.54930614433405489)
        self.ftest('atanh(-0.5)', math.atanh(-0.5), -0.54930614433405489)
        self.assertRaises(ValueError, math.atanh, 1)
        self.assertRaises(ValueError, math.atanh, -1)
        self.assertRaises(ValueError, math.atanh, INF)
        self.assertRaises(ValueError, math.atanh, NINF)
        self.assertTrue(math.isnan(math.atanh(NAN)))

    def testAtan2(self):
        self.assertRaises(TypeError, math.atan2)
        self.ftest('atan2(-1, 0)', math.atan2(-1, 0), -math.pi/2)
        self.ftest('atan2(-1, 1)', math.atan2(-1, 1), -math.pi/4)
        self.ftest('atan2(0, 1)', math.atan2(0, 1), 0)
        self.ftest('atan2(1, 1)', math.atan2(1, 1), math.pi/4)
        self.ftest('atan2(1, 0)', math.atan2(1, 0), math.pi/2)

        # math.atan2(0, x)
        self.ftest('atan2(0., -inf)', math.atan2(0., NINF), math.pi)
        self.ftest('atan2(0., -2.3)', math.atan2(0., -2.3), math.pi)
        self.ftest('atan2(0., -0.)', math.atan2(0., -0.), math.pi)
        self.assertEqual(math.atan2(0., 0.), 0.)
        self.assertEqual(math.atan2(0., 2.3), 0.)
        self.assertEqual(math.atan2(0., INF), 0.)
        self.assertTrue(math.isnan(math.atan2(0., NAN)))
        # math.atan2(-0, x)
        self.ftest('atan2(-0., -inf)', math.atan2(-0., NINF), -math.pi)
        self.ftest('atan2(-0., -2.3)', math.atan2(-0., -2.3), -math.pi)
        self.ftest('atan2(-0., -0.)', math.atan2(-0., -0.), -math.pi)
        self.assertEqual(math.atan2(-0., 0.), -0.)
        self.assertEqual(math.atan2(-0., 2.3), -0.)
        self.assertEqual(math.atan2(-0., INF), -0.)
        self.assertTrue(math.isnan(math.atan2(-0., NAN)))
        # math.atan2(INF, x)
        self.ftest('atan2(inf, -inf)', math.atan2(INF, NINF), math.pi*3/4)
        self.ftest('atan2(inf, -2.3)', math.atan2(INF, -2.3), math.pi/2)
        self.ftest('atan2(inf, -0.)', math.atan2(INF, -0.0), math.pi/2)
        self.ftest('atan2(inf, 0.)', math.atan2(INF, 0.0), math.pi/2)
        self.ftest('atan2(inf, 2.3)', math.atan2(INF, 2.3), math.pi/2)
        self.ftest('atan2(inf, inf)', math.atan2(INF, INF), math.pi/4)
        self.assertTrue(math.isnan(math.atan2(INF, NAN)))
        # math.atan2(NINF, x)
        self.ftest('atan2(-inf, -inf)', math.atan2(NINF, NINF), -math.pi*3/4)
        self.ftest('atan2(-inf, -2.3)', math.atan2(NINF, -2.3), -math.pi/2)
        self.ftest('atan2(-inf, -0.)', math.atan2(NINF, -0.0), -math.pi/2)
        self.ftest('atan2(-inf, 0.)', math.atan2(NINF, 0.0), -math.pi/2)
        self.ftest('atan2(-inf, 2.3)', math.atan2(NINF, 2.3), -math.pi/2)
        self.ftest('atan2(-inf, inf)', math.atan2(NINF, INF), -math.pi/4)
        self.assertTrue(math.isnan(math.atan2(NINF, NAN)))
        # math.atan2(+finite, x)
        self.ftest('atan2(2.3, -inf)', math.atan2(2.3, NINF), math.pi)
        self.ftest('atan2(2.3, -0.)', math.atan2(2.3, -0.), math.pi/2)
        self.ftest('atan2(2.3, 0.)', math.atan2(2.3, 0.), math.pi/2)
        self.assertEqual(math.atan2(2.3, INF), 0.)
        self.assertTrue(math.isnan(math.atan2(2.3, NAN)))
        # math.atan2(-finite, x)
        self.ftest('atan2(-2.3, -inf)', math.atan2(-2.3, NINF), -math.pi)
        self.ftest('atan2(-2.3, -0.)', math.atan2(-2.3, -0.), -math.pi/2)
        self.ftest('atan2(-2.3, 0.)', math.atan2(-2.3, 0.), -math.pi/2)
        self.assertEqual(math.atan2(-2.3, INF), -0.)
        self.assertTrue(math.isnan(math.atan2(-2.3, NAN)))
        # math.atan2(NAN, x)
        self.assertTrue(math.isnan(math.atan2(NAN, NINF)))
        self.assertTrue(math.isnan(math.atan2(NAN, -2.3)))
        self.assertTrue(math.isnan(math.atan2(NAN, -0.)))
        self.assertTrue(math.isnan(math.atan2(NAN, 0.)))
        self.assertTrue(math.isnan(math.atan2(NAN, 2.3)))
        self.assertTrue(math.isnan(math.atan2(NAN, INF)))
        self.assertTrue(math.isnan(math.atan2(NAN, NAN)))

    def testCeil(self):
        self.assertRaises(TypeError, math.ceil)
        self.assertEqual(int, type(math.ceil(0.5)))
        self.assertEqual(math.ceil(0.5), 1)
        self.assertEqual(math.ceil(1.0), 1)
        self.assertEqual(math.ceil(1.5), 2)
        self.assertEqual(math.ceil(-0.5), 0)
        self.assertEqual(math.ceil(-1.0), -1)
        self.assertEqual(math.ceil(-1.5), -1)
        self.assertEqual(math.ceil(0.0), 0)
        self.assertEqual(math.ceil(-0.0), 0)
        #self.assertEqual(math.ceil(INF), INF)
        #self.assertEqual(math.ceil(NINF), NINF)
        #self.assertTrue(math.isnan(math.ceil(NAN)))

        class TestCeil:
            def __ceil__(self):
                return 42
        class FloatCeil(float):
            def __ceil__(self):
                return 42
        class TestNoCeil:
            pass
        self.assertEqual(math.ceil(TestCeil()), 42)
        self.assertEqual(math.ceil(FloatCeil()), 42)
        self.assertEqual(math.ceil(FloatLike(42.5)), 43)
        self.assertRaises(TypeError, math.ceil, TestNoCeil())

        t = TestNoCeil()
        t.__ceil__ = lambda *args: args
        self.assertRaises(TypeError, math.ceil, t)
        self.assertRaises(TypeError, math.ceil, t, 0)

    @requires_IEEE_754
    def testCopysign(self):
        self.assertEqual(math.copysign(1, 42), 1.0)
        self.assertEqual(math.copysign(0., 42), 0.0)
        self.assertEqual(math.copysign(1., -42), -1.0)
        self.assertEqual(math.copysign(3, 0.), 3.0)
        self.assertEqual(math.copysign(4., -0.), -4.0)

        self.assertRaises(TypeError, math.copysign)
        # copysign should let us distinguish signs of zeros
        self.assertEqual(math.copysign(1., 0.), 1.)
        self.assertEqual(math.copysign(1., -0.), -1.)
        self.assertEqual(math.copysign(INF, 0.), INF)
        self.assertEqual(math.copysign(INF, -0.), NINF)
        self.assertEqual(math.copysign(NINF, 0.), INF)
        self.assertEqual(math.copysign(NINF, -0.), NINF)
        # and of infinities
        self.assertEqual(math.copysign(1., INF), 1.)
        self.assertEqual(math.copysign(1., NINF), -1.)
        self.assertEqual(math.copysign(INF, INF), INF)
        self.assertEqual(math.copysign(INF, NINF), NINF)
        self.assertEqual(math.copysign(NINF, INF), INF)
        self.assertEqual(math.copysign(NINF, NINF), NINF)
        self.assertTrue(math.isnan(math.copysign(NAN, 1.)))
        self.assertTrue(math.isnan(math.copysign(NAN, INF)))
        self.assertTrue(math.isnan(math.copysign(NAN, NINF)))
        self.assertTrue(math.isnan(math.copysign(NAN, NAN)))
        # copysign(INF, NAN) may be INF or it may be NINF, since
        # we don't know whether the sign bit of NAN is set on any
        # given platform.
        self.assertTrue(math.isinf(math.copysign(INF, NAN)))
        # similarly, copysign(2., NAN) could be 2. or -2.
        self.assertEqual(abs(math.copysign(2., NAN)), 2.)

    def testCos(self):
        self.assertRaises(TypeError, math.cos)
        self.ftest('cos(-pi/2)', math.cos(-math.pi/2), 0, abs_tol=math.ulp(1))
        self.ftest('cos(0)', math.cos(0), 1)
        self.ftest('cos(pi/2)', math.cos(math.pi/2), 0, abs_tol=math.ulp(1))
        self.ftest('cos(pi)', math.cos(math.pi), -1)
        try:
            self.assertTrue(math.isnan(math.cos(INF)))
            self.assertTrue(math.isnan(math.cos(NINF)))
        except ValueError:
            self.assertRaises(ValueError, math.cos, INF)
            self.assertRaises(ValueError, math.cos, NINF)
        self.assertTrue(math.isnan(math.cos(NAN)))

    @unittest.skipIf(sys.platform == 'win32' and platform.machine() in ('ARM', 'ARM64'),
                    "Windows UCRT is off by 2 ULP this test requires accuracy within 1 ULP")
    def testCosh(self):
        self.assertRaises(TypeError, math.cosh)
        self.ftest('cosh(0)', math.cosh(0), 1)
        self.ftest('cosh(2)-2*cosh(1)**2', math.cosh(2)-2*math.cosh(1)**2, -1) # Thanks to Lambert
        self.assertEqual(math.cosh(INF), INF)
        self.assertEqual(math.cosh(NINF), INF)
        self.assertTrue(math.isnan(math.cosh(NAN)))

    def testDegrees(self):
        self.assertRaises(TypeError, math.degrees)
        self.ftest('degrees(pi)', math.degrees(math.pi), 180.0)
        self.ftest('degrees(pi/2)', math.degrees(math.pi/2), 90.0)
        self.ftest('degrees(-pi/4)', math.degrees(-math.pi/4), -45.0)
        self.ftest('degrees(0)', math.degrees(0), 0)

    def testExp(self):
        self.assertRaises(TypeError, math.exp)
        self.ftest('exp(-1)', math.exp(-1), 1/math.e)
        self.ftest('exp(0)', math.exp(0), 1)
        self.ftest('exp(1)', math.exp(1), math.e)
        self.assertEqual(math.exp(INF), INF)
        self.assertEqual(math.exp(NINF), 0.)
        self.assertTrue(math.isnan(math.exp(NAN)))
        self.assertRaises(OverflowError, math.exp, 1000000)

    def testFabs(self):
        self.assertRaises(TypeError, math.fabs)
        self.ftest('fabs(-1)', math.fabs(-1), 1)
        self.ftest('fabs(0)', math.fabs(0), 0)
        self.ftest('fabs(1)', math.fabs(1), 1)

    def testFactorial(self):
        self.assertEqual(math.factorial(0), 1)
        total = 1
        for i in range(1, 1000):
            total *= i
            self.assertEqual(math.factorial(i), total)
            self.assertEqual(math.factorial(i), py_factorial(i))
        self.assertRaises(ValueError, math.factorial, -1)
        self.assertRaises(ValueError, math.factorial, -10**100)

    def testFactorialNonIntegers(self):
        self.assertRaises(TypeError, math.factorial, 5.0)
        self.assertRaises(TypeError, math.factorial, 5.2)
        self.assertRaises(TypeError, math.factorial, -1.0)
        self.assertRaises(TypeError, math.factorial, -1e100)
        self.assertRaises(TypeError, math.factorial, decimal.Decimal('5'))
        self.assertRaises(TypeError, math.factorial, decimal.Decimal('5.2'))
        self.assertRaises(TypeError, math.factorial, "5")

    # Other implementations may place different upper bounds.
    @support.cpython_only
    def testFactorialHugeInputs(self):
        # Currently raises OverflowError for inputs that are too large
        # to fit into a C long.
        self.assertRaises(OverflowError, math.factorial, 10**100)
        self.assertRaises(TypeError, math.factorial, 1e100)

    def testFloor(self):
        self.assertRaises(TypeError, math.floor)
        self.assertEqual(int, type(math.floor(0.5)))
        self.assertEqual(math.floor(0.5), 0)
        self.assertEqual(math.floor(1.0), 1)
        self.assertEqual(math.floor(1.5), 1)
        self.assertEqual(math.floor(-0.5), -1)
        self.assertEqual(math.floor(-1.0), -1)
        self.assertEqual(math.floor(-1.5), -2)
        #self.assertEqual(math.ceil(INF), INF)
        #self.assertEqual(math.ceil(NINF), NINF)
        #self.assertTrue(math.isnan(math.floor(NAN)))

        class TestFloor:
            def __floor__(self):
                return 42
        class FloatFloor(float):
            def __floor__(self):
                return 42
        class TestNoFloor:
            pass
        self.assertEqual(math.floor(TestFloor()), 42)
        self.assertEqual(math.floor(FloatFloor()), 42)
        self.assertEqual(math.floor(FloatLike(41.9)), 41)
        self.assertRaises(TypeError, math.floor, TestNoFloor())

        t = TestNoFloor()
        t.__floor__ = lambda *args: args
        self.assertRaises(TypeError, math.floor, t)
        self.assertRaises(TypeError, math.floor, t, 0)

    def testFmod(self):
        self.assertRaises(TypeError, math.fmod)
        self.ftest('fmod(10, 1)', math.fmod(10, 1), 0.0)
        self.ftest('fmod(10, 0.5)', math.fmod(10, 0.5), 0.0)
        self.ftest('fmod(10, 1.5)', math.fmod(10, 1.5), 1.0)
        self.ftest('fmod(-10, 1)', math.fmod(-10, 1), -0.0)
        self.ftest('fmod(-10, 0.5)', math.fmod(-10, 0.5), -0.0)
        self.ftest('fmod(-10, 1.5)', math.fmod(-10, 1.5), -1.0)
        self.assertTrue(math.isnan(math.fmod(NAN, 1.)))
        self.assertTrue(math.isnan(math.fmod(1., NAN)))
        self.assertTrue(math.isnan(math.fmod(NAN, NAN)))
        self.assertRaises(ValueError, math.fmod, 1., 0.)
        self.assertRaises(ValueError, math.fmod, INF, 1.)
        self.assertRaises(ValueError, math.fmod, NINF, 1.)
        self.assertRaises(ValueError, math.fmod, INF, 0.)
        self.assertEqual(math.fmod(3.0, INF), 3.0)
        self.assertEqual(math.fmod(-3.0, INF), -3.0)
        self.assertEqual(math.fmod(3.0, NINF), 3.0)
        self.assertEqual(math.fmod(-3.0, NINF), -3.0)
        self.assertEqual(math.fmod(0.0, 3.0), 0.0)
        self.assertEqual(math.fmod(0.0, NINF), 0.0)

    def testFrexp(self):
        self.assertRaises(TypeError, math.frexp)

        def testfrexp(name, result, expected):
            (mant, exp), (emant, eexp) = result, expected
            if abs(mant-emant) > eps or exp != eexp:
                self.fail('%s returned %r, expected %r'%\
                          (name, result, expected))

        testfrexp('frexp(-1)', math.frexp(-1), (-0.5, 1))
        testfrexp('frexp(0)', math.frexp(0), (0, 0))
        testfrexp('frexp(1)', math.frexp(1), (0.5, 1))
        testfrexp('frexp(2)', math.frexp(2), (0.5, 2))

        self.assertEqual(math.frexp(INF)[0], INF)
        self.assertEqual(math.frexp(NINF)[0], NINF)
        self.assertTrue(math.isnan(math.frexp(NAN)[0]))

    @requires_IEEE_754
    @unittest.skipIf(HAVE_DOUBLE_ROUNDING,
                         "fsum is not exact on machines with double rounding")
    def testFsum(self):
        # math.fsum relies on exact rounding for correct operation.
        # There's a known problem with IA32 floating-point that causes
        # inexact rounding in some situations, and will cause the
        # math.fsum tests below to fail; see issue #2937.  On non IEEE
        # 754 platforms, and on IEEE 754 platforms that exhibit the
        # problem described in issue #2937, we simply skip the whole
        # test.

        # Python version of math.fsum, for comparison.  Uses a
        # different algorithm based on frexp, ldexp and integer
        # arithmetic.
        from sys import float_info
        mant_dig = float_info.mant_dig
        etiny = float_info.min_exp - mant_dig

        def msum(iterable):
            """Full precision summation.  Compute sum(iterable) without any
            intermediate accumulation of error.  Based on the 'lsum' function
            at http://code.activestate.com/recipes/393090/

            """
            tmant, texp = 0, 0
            for x in iterable:
                mant, exp = math.frexp(x)
                mant, exp = int(math.ldexp(mant, mant_dig)), exp - mant_dig
                if texp > exp:
                    tmant <<= texp-exp
                    texp = exp
                else:
                    mant <<= exp-texp
                tmant += mant
            # Round tmant * 2**texp to a float.  The original recipe
            # used float(str(tmant)) * 2.0**texp for this, but that's
            # a little unsafe because str -> float conversion can't be
            # relied upon to do correct rounding on all platforms.
            tail = max(len(bin(abs(tmant)))-2 - mant_dig, etiny - texp)
            if tail > 0:
                h = 1 << (tail-1)
                tmant = tmant // (2*h) + bool(tmant & h and tmant & 3*h-1)
                texp += tail
            return math.ldexp(tmant, texp)

        test_values = [
            ([], 0.0),
            ([0.0], 0.0),
            ([1e100, 1.0, -1e100, 1e-100, 1e50, -1.0, -1e50], 1e-100),
            ([2.0**53, -0.5, -2.0**-54], 2.0**53-1.0),
            ([2.0**53, 1.0, 2.0**-100], 2.0**53+2.0),
            ([2.0**53+10.0, 1.0, 2.0**-100], 2.0**53+12.0),
            ([2.0**53-4.0, 0.5, 2.0**-54], 2.0**53-3.0),
            ([1./n for n in range(1, 1001)],
             float.fromhex('0x1.df11f45f4e61ap+2')),
            ([(-1.)**n/n for n in range(1, 1001)],
             float.fromhex('-0x1.62a2af1bd3624p-1')),
            ([1e16, 1., 1e-16], 10000000000000002.0),
            ([1e16-2., 1.-2.**-53, -(1e16-2.), -(1.-2.**-53)], 0.0),
            # exercise code for resizing partials array
            ([2.**n - 2.**(n+50) + 2.**(n+52) for n in range(-1074, 972, 2)] +
             [-2.**1022],
             float.fromhex('0x1.5555555555555p+970')),
            ]

        # Telescoping sum, with exact differences (due to Sterbenz)
        terms = [1.7**i for i in range(1001)]
        test_values.append((
            [terms[i+1] - terms[i] for i in range(1000)] + [-terms[1000]],
            -terms[0]
        ))

        for i, (vals, expected) in enumerate(test_values):
            try:
                actual = math.fsum(vals)
            except OverflowError:
                self.fail("test %d failed: got OverflowError, expected %r "
                          "for math.fsum(%.100r)" % (i, expected, vals))
            except ValueError:
                self.fail("test %d failed: got ValueError, expected %r "
                          "for math.fsum(%.100r)" % (i, expected, vals))
            self.assertEqual(actual, expected)

        from random import random, gauss, shuffle
        for j in range(1000):
            vals = [7, 1e100, -7, -1e100, -9e-20, 8e-20] * 10
            s = 0
            for i in range(200):
                v = gauss(0, random()) ** 7 - s
                s += v
                vals.append(v)
            shuffle(vals)

            s = msum(vals)
            self.assertEqual(msum(vals), math.fsum(vals))

    def testGcd(self):
        gcd = math.gcd
        self.assertEqual(gcd(0, 0), 0)
        self.assertEqual(gcd(1, 0), 1)
        self.assertEqual(gcd(-1, 0), 1)
        self.assertEqual(gcd(0, 1), 1)
        self.assertEqual(gcd(0, -1), 1)
        self.assertEqual(gcd(7, 1), 1)
        self.assertEqual(gcd(7, -1), 1)
        self.assertEqual(gcd(-23, 15), 1)
        self.assertEqual(gcd(120, 84), 12)
        self.assertEqual(gcd(84, -120), 12)
        self.assertEqual(gcd(1216342683557601535506311712,
                             436522681849110124616458784), 32)

        x = 434610456570399902378880679233098819019853229470286994367836600566
        y = 1064502245825115327754847244914921553977
        for c in (652560,
                  576559230871654959816130551884856912003141446781646602790216406874):
            a = x * c
            b = y * c
            self.assertEqual(gcd(a, b), c)
            self.assertEqual(gcd(b, a), c)
            self.assertEqual(gcd(-a, b), c)
            self.assertEqual(gcd(b, -a), c)
            self.assertEqual(gcd(a, -b), c)
            self.assertEqual(gcd(-b, a), c)
            self.assertEqual(gcd(-a, -b), c)
            self.assertEqual(gcd(-b, -a), c)

        self.assertEqual(gcd(), 0)
        self.assertEqual(gcd(120), 120)
        self.assertEqual(gcd(-120), 120)
        self.assertEqual(gcd(120, 84, 102), 6)
        self.assertEqual(gcd(120, 1, 84), 1)

        self.assertRaises(TypeError, gcd, 120.0)
        self.assertRaises(TypeError, gcd, 120.0, 84)
        self.assertRaises(TypeError, gcd, 120, 84.0)
        self.assertRaises(TypeError, gcd, 120, 1, 84.0)
        self.assertEqual(gcd(MyIndexable(120), MyIndexable(84)), 12)

    def testHypot(self):
        from decimal import Decimal
        from fractions import Fraction

        hypot = math.hypot

        # Test different numbers of arguments (from zero to five)
        # against a straightforward pure python implementation
        args = math.e, math.pi, math.sqrt(2.0), math.gamma(3.5), math.sin(2.1)
        for i in range(len(args)+1):
            self.assertAlmostEqual(
                hypot(*args[:i]),
                math.sqrt(sum(s**2 for s in args[:i]))
            )

        # Test allowable types (those with __float__)
        self.assertEqual(hypot(12.0, 5.0), 13.0)
        self.assertEqual(hypot(12, 5), 13)
        self.assertEqual(hypot(Decimal(12), Decimal(5)), 13)
        self.assertEqual(hypot(Fraction(12, 32), Fraction(5, 32)), Fraction(13, 32))
        self.assertEqual(hypot(bool(1), bool(0), bool(1), bool(1)), math.sqrt(3))

        # Test corner cases
        self.assertEqual(hypot(0.0, 0.0), 0.0)     # Max input is zero
        self.assertEqual(hypot(-10.5), 10.5)       # Negative input
        self.assertEqual(hypot(), 0.0)             # Negative input
        self.assertEqual(1.0,
            math.copysign(1.0, hypot(-0.0))        # Convert negative zero to positive zero
        )
        self.assertEqual(                          # Handling of moving max to the end
            hypot(1.5, 1.5, 0.5),
            hypot(1.5, 0.5, 1.5),
        )

        # Test handling of bad arguments
        with self.assertRaises(TypeError):         # Reject keyword args
            hypot(x=1)
        with self.assertRaises(TypeError):         # Reject values without __float__
            hypot(1.1, 'string', 2.2)
        int_too_big_for_float = 10 ** (sys.float_info.max_10_exp + 5)
        with self.assertRaises((ValueError, OverflowError)):
            hypot(1, int_too_big_for_float)

        # Any infinity gives positive infinity.
        self.assertEqual(hypot(INF), INF)
        self.assertEqual(hypot(0, INF), INF)
        self.assertEqual(hypot(10, INF), INF)
        self.assertEqual(hypot(-10, INF), INF)
        self.assertEqual(hypot(NAN, INF), INF)
        self.assertEqual(hypot(INF, NAN), INF)
        self.assertEqual(hypot(NINF, NAN), INF)
        self.assertEqual(hypot(NAN, NINF), INF)
        self.assertEqual(hypot(-INF, INF), INF)
        self.assertEqual(hypot(-INF, -INF), INF)
        self.assertEqual(hypot(10, -INF), INF)

        # If no infinity, any NaN gives a NaN.
        self.assertTrue(math.isnan(hypot(NAN)))
        self.assertTrue(math.isnan(hypot(0, NAN)))
        self.assertTrue(math.isnan(hypot(NAN, 10)))
        self.assertTrue(math.isnan(hypot(10, NAN)))
        self.assertTrue(math.isnan(hypot(NAN, NAN)))
        self.assertTrue(math.isnan(hypot(NAN)))

        # Verify scaling for extremely large values
        fourthmax = FLOAT_MAX / 4.0
        for n in range(32):
            self.assertTrue(math.isclose(hypot(*([fourthmax]*n)),
                                         fourthmax * math.sqrt(n)))

        # Verify scaling for extremely small values
        for exp in range(32):
            scale = FLOAT_MIN / 2.0 ** exp
            self.assertEqual(math.hypot(4*scale, 3*scale), 5*scale)

    @requires_IEEE_754
    @unittest.skipIf(HAVE_DOUBLE_ROUNDING,
                     "hypot() loses accuracy on machines with double rounding")
    def testHypotAccuracy(self):
        # Verify improved accuracy in cases that were known to be inaccurate.
        #
        # The new algorithm's accuracy depends on IEEE 754 arithmetic
        # guarantees, on having the usual ROUND HALF EVEN rounding mode, on
        # the system not having double rounding due to extended precision,
        # and on the compiler maintaining the specified order of operations.
        #
        # This test is known to succeed on most of our builds.  If it fails
        # some build, we either need to add another skipIf if the cause is
        # identifiable; otherwise, we can remove this test entirely.

        hypot = math.hypot
        Decimal = decimal.Decimal
        high_precision = decimal.Context(prec=500)

        for hx, hy in [
            # Cases with a 1 ulp error in Python 3.7 compiled with Clang
            ('0x1.10e89518dca48p+29', '0x1.1970f7565b7efp+30'),
            ('0x1.10106eb4b44a2p+29', '0x1.ef0596cdc97f8p+29'),
            ('0x1.459c058e20bb7p+30', '0x1.993ca009b9178p+29'),
            ('0x1.378371ae67c0cp+30', '0x1.fbe6619854b4cp+29'),
            ('0x1.f4cd0574fb97ap+29', '0x1.50fe31669340ep+30'),
            ('0x1.494b2cdd3d446p+29', '0x1.212a5367b4c7cp+29'),
            ('0x1.f84e649f1e46dp+29', '0x1.1fa56bef8eec4p+30'),
            ('0x1.2e817edd3d6fap+30', '0x1.eb0814f1e9602p+29'),
            ('0x1.0d3a6e3d04245p+29', '0x1.32a62fea52352p+30'),
            ('0x1.888e19611bfc5p+29', '0x1.52b8e70b24353p+29'),

            # Cases with 2 ulp error in Python 3.8
            ('0x1.538816d48a13fp+29', '0x1.7967c5ca43e16p+29'),
            ('0x1.57b47b7234530p+29', '0x1.74e2c7040e772p+29'),
            ('0x1.821b685e9b168p+30', '0x1.677dc1c1e3dc6p+29'),
            ('0x1.9e8247f67097bp+29', '0x1.24bd2dc4f4baep+29'),
            ('0x1.b73b59e0cb5f9p+29', '0x1.da899ab784a97p+28'),
            ('0x1.94a8d2842a7cfp+30', '0x1.326a51d4d8d8ap+30'),
            ('0x1.e930b9cd99035p+29', '0x1.5a1030e18dff9p+30'),
            ('0x1.1592bbb0e4690p+29', '0x1.a9c337b33fb9ap+29'),
            ('0x1.1243a50751fd4p+29', '0x1.a5a10175622d9p+29'),
            ('0x1.57a8596e74722p+30', '0x1.42d1af9d04da9p+30'),

            # Cases with 1 ulp error in version fff3c28052e6b0
            ('0x1.ee7dbd9565899p+29', '0x1.7ab4d6fc6e4b4p+29'),
            ('0x1.5c6bfbec5c4dcp+30', '0x1.02511184b4970p+30'),
            ('0x1.59dcebba995cap+30', '0x1.50ca7e7c38854p+29'),
            ('0x1.768cdd94cf5aap+29', '0x1.9cfdc5571d38ep+29'),
            ('0x1.dcf137d60262ep+29', '0x1.1101621990b3ep+30'),
            ('0x1.3a2d006e288b0p+30', '0x1.e9a240914326cp+29'),
            ('0x1.62a32f7f53c61p+29', '0x1.47eb6cd72684fp+29'),
            ('0x1.d3bcb60748ef2p+29', '0x1.3f13c4056312cp+30'),
            ('0x1.282bdb82f17f3p+30', '0x1.640ba4c4eed3ap+30'),
            ('0x1.89d8c423ea0c6p+29', '0x1.d35dcfe902bc3p+29'),
        ]:
            x = float.fromhex(hx)
            y = float.fromhex(hy)
            with self.subTest(hx=hx, hy=hy, x=x, y=y):
                with decimal.localcontext(high_precision):
                    z = float((Decimal(x)**2 + Decimal(y)**2).sqrt())
                self.assertEqual(hypot(x, y), z)

    def testDist(self):
        from decimal import Decimal as D
        from fractions import Fraction as F

        dist = math.dist
        sqrt = math.sqrt

        # Simple exact cases
        self.assertEqual(dist((1.0, 2.0, 3.0), (4.0, 2.0, -1.0)), 5.0)
        self.assertEqual(dist((1, 2, 3), (4, 2, -1)), 5.0)

        # Test different numbers of arguments (from zero to nine)
        # against a straightforward pure python implementation
        for i in range(9):
            for j in range(5):
                p = tuple(random.uniform(-5, 5) for k in range(i))
                q = tuple(random.uniform(-5, 5) for k in range(i))
                self.assertAlmostEqual(
                    dist(p, q),
                    sqrt(sum((px - qx) ** 2.0 for px, qx in zip(p, q)))
                )

        # Test non-tuple inputs
        self.assertEqual(dist([1.0, 2.0, 3.0], [4.0, 2.0, -1.0]), 5.0)
        self.assertEqual(dist(iter([1.0, 2.0, 3.0]), iter([4.0, 2.0, -1.0])), 5.0)

        # Test allowable types (those with __float__)
        self.assertEqual(dist((14.0, 1.0), (2.0, -4.0)), 13.0)
        self.assertEqual(dist((14, 1), (2, -4)), 13)
        self.assertEqual(dist((D(14), D(1)), (D(2), D(-4))), D(13))
        self.assertEqual(dist((F(14, 32), F(1, 32)), (F(2, 32), F(-4, 32))),
                         F(13, 32))
        self.assertEqual(dist((True, True, False, True, False),
                              (True, False, True, True, False)),
                         sqrt(2.0))

        # Test corner cases
        self.assertEqual(dist((13.25, 12.5, -3.25),
                              (13.25, 12.5, -3.25)),
                         0.0)                      # Distance with self is zero
        self.assertEqual(dist((), ()), 0.0)        # Zero-dimensional case
        self.assertEqual(1.0,                      # Convert negative zero to positive zero
            math.copysign(1.0, dist((-0.0,), (0.0,)))
        )
        self.assertEqual(1.0,                      # Convert negative zero to positive zero
            math.copysign(1.0, dist((0.0,), (-0.0,)))
        )
        self.assertEqual(                          # Handling of moving max to the end
            dist((1.5, 1.5, 0.5), (0, 0, 0)),
            dist((1.5, 0.5, 1.5), (0, 0, 0))
        )

        # Verify tuple subclasses are allowed
        class T(tuple):
            pass
        self.assertEqual(dist(T((1, 2, 3)), ((4, 2, -1))), 5.0)

        # Test handling of bad arguments
        with self.assertRaises(TypeError):         # Reject keyword args
            dist(p=(1, 2, 3), q=(4, 5, 6))
        with self.assertRaises(TypeError):         # Too few args
            dist((1, 2, 3))
        with self.assertRaises(TypeError):         # Too many args
            dist((1, 2, 3), (4, 5, 6), (7, 8, 9))
        with self.assertRaises(TypeError):         # Scalars not allowed
            dist(1, 2)
        with self.assertRaises(TypeError):         # Reject values without __float__
            dist((1.1, 'string', 2.2), (1, 2, 3))
        with self.assertRaises(ValueError):        # Check dimension agree
            dist((1, 2, 3, 4), (5, 6, 7))
        with self.assertRaises(ValueError):        # Check dimension agree
            dist((1, 2, 3), (4, 5, 6, 7))
        with self.assertRaises(TypeError):         # Rejects invalid types
            dist("abc", "xyz")
        int_too_big_for_float = 10 ** (sys.float_info.max_10_exp + 5)
        with self.assertRaises((ValueError, OverflowError)):
            dist((1, int_too_big_for_float), (2, 3))
        with self.assertRaises((ValueError, OverflowError)):
            dist((2, 3), (1, int_too_big_for_float))

        # Verify that the one dimensional case is equivalent to abs()
        for i in range(20):
            p, q = random.random(), random.random()
            self.assertEqual(dist((p,), (q,)), abs(p - q))

        # Test special values
        values = [NINF, -10.5, -0.0, 0.0, 10.5, INF, NAN]
        for p in itertools.product(values, repeat=3):
            for q in itertools.product(values, repeat=3):
                diffs = [px - qx for px, qx in zip(p, q)]
                if any(map(math.isinf, diffs)):
                    # Any infinite difference gives positive infinity.
                    self.assertEqual(dist(p, q), INF)
                elif any(map(math.isnan, diffs)):
                    # If no infinity, any NaN gives a NaN.
                    self.assertTrue(math.isnan(dist(p, q)))

        # Verify scaling for extremely large values
        fourthmax = FLOAT_MAX / 4.0
        for n in range(32):
            p = (fourthmax,) * n
            q = (0.0,) * n
            self.assertTrue(math.isclose(dist(p, q), fourthmax * math.sqrt(n)))
            self.assertTrue(math.isclose(dist(q, p), fourthmax * math.sqrt(n)))

        # Verify scaling for extremely small values
        for exp in range(32):
            scale = FLOAT_MIN / 2.0 ** exp
            p = (4*scale, 3*scale)
            q = (0.0, 0.0)
            self.assertEqual(math.dist(p, q), 5*scale)
            self.assertEqual(math.dist(q, p), 5*scale)

    def testIsqrt(self):
        # Test a variety of inputs, large and small.
        test_values = (
            list(range(1000))
            + list(range(10**6 - 1000, 10**6 + 1000))
            + [2**e + i for e in range(60, 200) for i in range(-40, 40)]
            + [3**9999, 10**5001]
        )

        for value in test_values:
            with self.subTest(value=value):
                s = math.isqrt(value)
                self.assertIs(type(s), int)
                self.assertLessEqual(s*s, value)
                self.assertLess(value, (s+1)*(s+1))

        # Negative values
        with self.assertRaises(ValueError):
            math.isqrt(-1)

        # Integer-like things
        s = math.isqrt(True)
        self.assertIs(type(s), int)
        self.assertEqual(s, 1)

        s = math.isqrt(False)
        self.assertIs(type(s), int)
        self.assertEqual(s, 0)

        class IntegerLike(object):
            def __init__(self, value):
                self.value = value

            def __index__(self):
                return self.value

        s = math.isqrt(IntegerLike(1729))
        self.assertIs(type(s), int)
        self.assertEqual(s, 41)

        with self.assertRaises(ValueError):
            math.isqrt(IntegerLike(-3))

        # Non-integer-like things
        bad_values = [
            3.5, "a string", decimal.Decimal("3.5"), 3.5j,
            100.0, -4.0,
        ]
        for value in bad_values:
            with self.subTest(value=value):
                with self.assertRaises(TypeError):
                    math.isqrt(value)

    def test_lcm(self):
        lcm = math.lcm
        self.assertEqual(lcm(0, 0), 0)
        self.assertEqual(lcm(1, 0), 0)
        self.assertEqual(lcm(-1, 0), 0)
        self.assertEqual(lcm(0, 1), 0)
        self.assertEqual(lcm(0, -1), 0)
        self.assertEqual(lcm(7, 1), 7)
        self.assertEqual(lcm(7, -1), 7)
        self.assertEqual(lcm(-23, 15), 345)
        self.assertEqual(lcm(120, 84), 840)
        self.assertEqual(lcm(84, -120), 840)
        self.assertEqual(lcm(1216342683557601535506311712,
                             436522681849110124616458784),
                             16592536571065866494401400422922201534178938447014944)

        x = 43461045657039990237
        y = 10645022458251153277
        for c in (652560,
                  57655923087165495981):
            a = x * c
            b = y * c
            d = x * y * c
            self.assertEqual(lcm(a, b), d)
            self.assertEqual(lcm(b, a), d)
            self.assertEqual(lcm(-a, b), d)
            self.assertEqual(lcm(b, -a), d)
            self.assertEqual(lcm(a, -b), d)
            self.assertEqual(lcm(-b, a), d)
            self.assertEqual(lcm(-a, -b), d)
            self.assertEqual(lcm(-b, -a), d)

        self.assertEqual(lcm(), 1)
        self.assertEqual(lcm(120), 120)
        self.assertEqual(lcm(-120), 120)
        self.assertEqual(lcm(120, 84, 102), 14280)
        self.assertEqual(lcm(120, 0, 84), 0)

        self.assertRaises(TypeError, lcm, 120.0)
        self.assertRaises(TypeError, lcm, 120.0, 84)
        self.assertRaises(TypeError, lcm, 120, 84.0)
        self.assertRaises(TypeError, lcm, 120, 0, 84.0)
        self.assertEqual(lcm(MyIndexable(120), MyIndexable(84)), 840)

    def testLdexp(self):
        self.assertRaises(TypeError, math.ldexp)
        self.ftest('ldexp(0,1)', math.ldexp(0,1), 0)
        self.ftest('ldexp(1,1)', math.ldexp(1,1), 2)
        self.ftest('ldexp(1,-1)', math.ldexp(1,-1), 0.5)
        self.ftest('ldexp(-1,1)', math.ldexp(-1,1), -2)
        self.assertRaises(OverflowError, math.ldexp, 1., 1000000)
        self.assertRaises(OverflowError, math.ldexp, -1., 1000000)
        self.assertEqual(math.ldexp(1., -1000000), 0.)
        self.assertEqual(math.ldexp(-1., -1000000), -0.)
        self.assertEqual(math.ldexp(INF, 30), INF)
        self.assertEqual(math.ldexp(NINF, -213), NINF)
        self.assertTrue(math.isnan(math.ldexp(NAN, 0)))

        # large second argument
        for n in [10**5, 10**10, 10**20, 10**40]:
            self.assertEqual(math.ldexp(INF, -n), INF)
            self.assertEqual(math.ldexp(NINF, -n), NINF)
            self.assertEqual(math.ldexp(1., -n), 0.)
            self.assertEqual(math.ldexp(-1., -n), -0.)
            self.assertEqual(math.ldexp(0., -n), 0.)
            self.assertEqual(math.ldexp(-0., -n), -0.)
            self.assertTrue(math.isnan(math.ldexp(NAN, -n)))

            self.assertRaises(OverflowError, math.ldexp, 1., n)
            self.assertRaises(OverflowError, math.ldexp, -1., n)
            self.assertEqual(math.ldexp(0., n), 0.)
            self.assertEqual(math.ldexp(-0., n), -0.)
            self.assertEqual(math.ldexp(INF, n), INF)
            self.assertEqual(math.ldexp(NINF, n), NINF)
            self.assertTrue(math.isnan(math.ldexp(NAN, n)))

    def testLog(self):
        self.assertRaises(TypeError, math.log)
        self.ftest('log(1/e)', math.log(1/math.e), -1)
        self.ftest('log(1)', math.log(1), 0)
        self.ftest('log(e)', math.log(math.e), 1)
        self.ftest('log(32,2)', math.log(32,2), 5)
        self.ftest('log(10**40, 10)', math.log(10**40, 10), 40)
        self.ftest('log(10**40, 10**20)', math.log(10**40, 10**20), 2)
        self.ftest('log(10**1000)', math.log(10**1000),
                   2302.5850929940457)
        self.assertRaises(ValueError, math.log, -1.5)
        self.assertRaises(ValueError, math.log, -10**1000)
        self.assertRaises(ValueError, math.log, NINF)
        self.assertEqual(math.log(INF), INF)
        self.assertTrue(math.isnan(math.log(NAN)))

    def testLog1p(self):
        self.assertRaises(TypeError, math.log1p)
        for n in [2, 2**90, 2**300]:
            self.assertAlmostEqual(math.log1p(n), math.log1p(float(n)))
        self.assertRaises(ValueError, math.log1p, -1)
        self.assertEqual(math.log1p(INF), INF)

    @requires_IEEE_754
    def testLog2(self):
        self.assertRaises(TypeError, math.log2)

        # Check some integer values
        self.assertEqual(math.log2(1), 0.0)
        self.assertEqual(math.log2(2), 1.0)
        self.assertEqual(math.log2(4), 2.0)

        # Large integer values
        self.assertEqual(math.log2(2**1023), 1023.0)
        self.assertEqual(math.log2(2**1024), 1024.0)
        self.assertEqual(math.log2(2**2000), 2000.0)

        self.assertRaises(ValueError, math.log2, -1.5)
        self.assertRaises(ValueError, math.log2, NINF)
        self.assertTrue(math.isnan(math.log2(NAN)))

    @requires_IEEE_754
    # log2() is not accurate enough on Mac OS X Tiger (10.4)
    @support.requires_mac_ver(10, 5)
    def testLog2Exact(self):
        # Check that we get exact equality for log2 of powers of 2.
        actual = [math.log2(math.ldexp(1.0, n)) for n in range(-1074, 1024)]
        expected = [float(n) for n in range(-1074, 1024)]
        self.assertEqual(actual, expected)

    def testLog10(self):
        self.assertRaises(TypeError, math.log10)
        self.ftest('log10(0.1)', math.log10(0.1), -1)
        self.ftest('log10(1)', math.log10(1), 0)
        self.ftest('log10(10)', math.log10(10), 1)
        self.ftest('log10(10**1000)', math.log10(10**1000), 1000.0)
        self.assertRaises(ValueError, math.log10, -1.5)
        self.assertRaises(ValueError, math.log10, -10**1000)
        self.assertRaises(ValueError, math.log10, NINF)
        self.assertEqual(math.log(INF), INF)
        self.assertTrue(math.isnan(math.log10(NAN)))

    def testModf(self):
        self.assertRaises(TypeError, math.modf)

        def testmodf(name, result, expected):
            (v1, v2), (e1, e2) = result, expected
            if abs(v1-e1) > eps or abs(v2-e2):
                self.fail('%s returned %r, expected %r'%\
                          (name, result, expected))

        testmodf('modf(1.5)', math.modf(1.5), (0.5, 1.0))
        testmodf('modf(-1.5)', math.modf(-1.5), (-0.5, -1.0))

        self.assertEqual(math.modf(INF), (0.0, INF))
        self.assertEqual(math.modf(NINF), (-0.0, NINF))

        modf_nan = math.modf(NAN)
        self.assertTrue(math.isnan(modf_nan[0]))
        self.assertTrue(math.isnan(modf_nan[1]))

    def testPow(self):
        self.assertRaises(TypeError, math.pow)
        self.ftest('pow(0,1)', math.pow(0,1), 0)
        self.ftest('pow(1,0)', math.pow(1,0), 1)
        self.ftest('pow(2,1)', math.pow(2,1), 2)
        self.ftest('pow(2,-1)', math.pow(2,-1), 0.5)
        self.assertEqual(math.pow(INF, 1), INF)
        self.assertEqual(math.pow(NINF, 1), NINF)
        self.assertEqual((math.pow(1, INF)), 1.)
        self.assertEqual((math.pow(1, NINF)), 1.)
        self.assertTrue(math.isnan(math.pow(NAN, 1)))
        self.assertTrue(math.isnan(math.pow(2, NAN)))
        self.assertTrue(math.isnan(math.pow(0, NAN)))
        self.assertEqual(math.pow(1, NAN), 1)

        # pow(0., x)
        self.assertEqual(math.pow(0., INF), 0.)
        self.assertEqual(math.pow(0., 3.), 0.)
        self.assertEqual(math.pow(0., 2.3), 0.)
        self.assertEqual(math.pow(0., 2.), 0.)
        self.assertEqual(math.pow(0., 0.), 1.)
        self.assertEqual(math.pow(0., -0.), 1.)
        self.assertRaises(ValueError, math.pow, 0., -2.)
        self.assertRaises(ValueError, math.pow, 0., -2.3)
        self.assertRaises(ValueError, math.pow, 0., -3.)
        self.assertRaises(ValueError, math.pow, 0., NINF)
        self.assertTrue(math.isnan(math.pow(0., NAN)))

        # pow(INF, x)
        self.assertEqual(math.pow(INF, INF), INF)
        self.assertEqual(math.pow(INF, 3.), INF)
        self.assertEqual(math.pow(INF, 2.3), INF)
        self.assertEqual(math.pow(INF, 2.), INF)
        self.assertEqual(math.pow(INF, 0.), 1.)
        self.assertEqual(math.pow(INF, -0.), 1.)
        self.assertEqual(math.pow(INF, -2.), 0.)
        self.assertEqual(math.pow(INF, -2.3), 0.)
        self.assertEqual(math.pow(INF, -3.), 0.)
        self.assertEqual(math.pow(INF, NINF), 0.)
        self.assertTrue(math.isnan(math.pow(INF, NAN)))

        # pow(-0., x)
        self.assertEqual(math.pow(-0., INF), 0.)
        self.assertEqual(math.pow(-0., 3.), -0.)
        self.assertEqual(math.pow(-0., 2.3), 0.)
        self.assertEqual(math.pow(-0., 2.), 0.)
        self.assertEqual(math.pow(-0., 0.), 1.)
        self.assertEqual(math.pow(-0., -0.), 1.)
        self.assertRaises(ValueError, math.pow, -0., -2.)
        self.assertRaises(ValueError, math.pow, -0., -2.3)
        self.assertRaises(ValueError, math.pow, -0., -3.)
        self.assertRaises(ValueError, math.pow, -0., NINF)
        self.assertTrue(math.isnan(math.pow(-0., NAN)))

        # pow(NINF, x)
        self.assertEqual(math.pow(NINF, INF), INF)
        self.assertEqual(math.pow(NINF, 3.), NINF)
        self.assertEqual(math.pow(NINF, 2.3), INF)
        self.assertEqual(math.pow(NINF, 2.), INF)
        self.assertEqual(math.pow(NINF, 0.), 1.)
        self.assertEqual(math.pow(NINF, -0.), 1.)
        self.assertEqual(math.pow(NINF, -2.), 0.)
        self.assertEqual(math.pow(NINF, -2.3), 0.)
        self.assertEqual(math.pow(NINF, -3.), -0.)
        self.assertEqual(math.pow(NINF, NINF), 0.)
        self.assertTrue(math.isnan(math.pow(NINF, NAN)))

        # pow(-1, x)
        self.assertEqual(math.pow(-1., INF), 1.)
        self.assertEqual(math.pow(-1., 3.), -1.)
        self.assertRaises(ValueError, math.pow, -1., 2.3)
        self.assertEqual(math.pow(-1., 2.), 1.)
        self.assertEqual(math.pow(-1., 0.), 1.)
        self.assertEqual(math.pow(-1., -0.), 1.)
        self.assertEqual(math.pow(-1., -2.), 1.)
        self.assertRaises(ValueError, math.pow, -1., -2.3)
        self.assertEqual(math.pow(-1., -3.), -1.)
        self.assertEqual(math.pow(-1., NINF), 1.)
        self.assertTrue(math.isnan(math.pow(-1., NAN)))

        # pow(1, x)
        self.assertEqual(math.pow(1., INF), 1.)
        self.assertEqual(math.pow(1., 3.), 1.)
        self.assertEqual(math.pow(1., 2.3), 1.)
        self.assertEqual(math.pow(1., 2.), 1.)
        self.assertEqual(math.pow(1., 0.), 1.)
        self.assertEqual(math.pow(1., -0.), 1.)
        self.assertEqual(math.pow(1., -2.), 1.)
        self.assertEqual(math.pow(1., -2.3), 1.)
        self.assertEqual(math.pow(1., -3.), 1.)
        self.assertEqual(math.pow(1., NINF), 1.)
        self.assertEqual(math.pow(1., NAN), 1.)

        # pow(x, 0) should be 1 for any x
        self.assertEqual(math.pow(2.3, 0.), 1.)
        self.assertEqual(math.pow(-2.3, 0.), 1.)
        self.assertEqual(math.pow(NAN, 0.), 1.)
        self.assertEqual(math.pow(2.3, -0.), 1.)
        self.assertEqual(math.pow(-2.3, -0.), 1.)
        self.assertEqual(math.pow(NAN, -0.), 1.)

        # pow(x, y) is invalid if x is negative and y is not integral
        self.assertRaises(ValueError, math.pow, -1., 2.3)
        self.assertRaises(ValueError, math.pow, -15., -3.1)

        # pow(x, NINF)
        self.assertEqual(math.pow(1.9, NINF), 0.)
        self.assertEqual(math.pow(1.1, NINF), 0.)
        self.assertEqual(math.pow(0.9, NINF), INF)
        self.assertEqual(math.pow(0.1, NINF), INF)
        self.assertEqual(math.pow(-0.1, NINF), INF)
        self.assertEqual(math.pow(-0.9, NINF), INF)
        self.assertEqual(math.pow(-1.1, NINF), 0.)
        self.assertEqual(math.pow(-1.9, NINF), 0.)

        # pow(x, INF)
        self.assertEqual(math.pow(1.9, INF), INF)
        self.assertEqual(math.pow(1.1, INF), INF)
        self.assertEqual(math.pow(0.9, INF), 0.)
        self.assertEqual(math.pow(0.1, INF), 0.)
        self.assertEqual(math.pow(-0.1, INF), 0.)
        self.assertEqual(math.pow(-0.9, INF), 0.)
        self.assertEqual(math.pow(-1.1, INF), INF)
        self.assertEqual(math.pow(-1.9, INF), INF)

        # pow(x, y) should work for x negative, y an integer
        self.ftest('(-2.)**3.', math.pow(-2.0, 3.0), -8.0)
        self.ftest('(-2.)**2.', math.pow(-2.0, 2.0), 4.0)
        self.ftest('(-2.)**1.', math.pow(-2.0, 1.0), -2.0)
        self.ftest('(-2.)**0.', math.pow(-2.0, 0.0), 1.0)
        self.ftest('(-2.)**-0.', math.pow(-2.0, -0.0), 1.0)
        self.ftest('(-2.)**-1.', math.pow(-2.0, -1.0), -0.5)
        self.ftest('(-2.)**-2.', math.pow(-2.0, -2.0), 0.25)
        self.ftest('(-2.)**-3.', math.pow(-2.0, -3.0), -0.125)
        self.assertRaises(ValueError, math.pow, -2.0, -0.5)
        self.assertRaises(ValueError, math.pow, -2.0, 0.5)

        # the following tests have been commented out since they don't
        # really belong here:  the implementation of ** for floats is
        # independent of the implementation of math.pow
        #self.assertEqual(1**NAN, 1)
        #self.assertEqual(1**INF, 1)
        #self.assertEqual(1**NINF, 1)
        #self.assertEqual(1**0, 1)
        #self.assertEqual(1.**NAN, 1)
        #self.assertEqual(1.**INF, 1)
        #self.assertEqual(1.**NINF, 1)
        #self.assertEqual(1.**0, 1)

    def testRadians(self):
        self.assertRaises(TypeError, math.radians)
        self.ftest('radians(180)', math.radians(180), math.pi)
        self.ftest('radians(90)', math.radians(90), math.pi/2)
        self.ftest('radians(-45)', math.radians(-45), -math.pi/4)
        self.ftest('radians(0)', math.radians(0), 0)

    @requires_IEEE_754
    def testRemainder(self):
        from fractions import Fraction

        def validate_spec(x, y, r):
            """
            Check that r matches remainder(x, y) according to the IEEE 754
            specification. Assumes that x, y and r are finite and y is nonzero.
            """
            fx, fy, fr = Fraction(x), Fraction(y), Fraction(r)
            # r should not exceed y/2 in absolute value
            self.assertLessEqual(abs(fr), abs(fy/2))
            # x - r should be an exact integer multiple of y
            n = (fx - fr) / fy
            self.assertEqual(n, int(n))
            if abs(fr) == abs(fy/2):
                # If |r| == |y/2|, n should be even.
                self.assertEqual(n/2, int(n/2))

        # triples (x, y, remainder(x, y)) in hexadecimal form.
        testcases = [
            # Remainders modulo 1, showing the ties-to-even behaviour.
            '-4.0 1 -0.0',
            '-3.8 1  0.8',
            '-3.0 1 -0.0',
            '-2.8 1 -0.8',
            '-2.0 1 -0.0',
            '-1.8 1  0.8',
            '-1.0 1 -0.0',
            '-0.8 1 -0.8',
            '-0.0 1 -0.0',
            ' 0.0 1  0.0',
            ' 0.8 1  0.8',
            ' 1.0 1  0.0',
            ' 1.8 1 -0.8',
            ' 2.0 1  0.0',
            ' 2.8 1  0.8',
            ' 3.0 1  0.0',
            ' 3.8 1 -0.8',
            ' 4.0 1  0.0',

            # Reductions modulo 2*pi
            '0x0.0p+0 0x1.921fb54442d18p+2 0x0.0p+0',
            '0x1.921fb54442d18p+0 0x1.921fb54442d18p+2  0x1.921fb54442d18p+0',
            '0x1.921fb54442d17p+1 0x1.921fb54442d18p+2  0x1.921fb54442d17p+1',
            '0x1.921fb54442d18p+1 0x1.921fb54442d18p+2  0x1.921fb54442d18p+1',
            '0x1.921fb54442d19p+1 0x1.921fb54442d18p+2 -0x1.921fb54442d17p+1',
            '0x1.921fb54442d17p+2 0x1.921fb54442d18p+2 -0x0.0000000000001p+2',
            '0x1.921fb54442d18p+2 0x1.921fb54442d18p+2  0x0p0',
            '0x1.921fb54442d19p+2 0x1.921fb54442d18p+2  0x0.0000000000001p+2',
            '0x1.2d97c7f3321d1p+3 0x1.921fb54442d18p+2  0x1.921fb54442d14p+1',
            '0x1.2d97c7f3321d2p+3 0x1.921fb54442d18p+2 -0x1.921fb54442d18p+1',
            '0x1.2d97c7f3321d3p+3 0x1.921fb54442d18p+2 -0x1.921fb54442d14p+1',
            '0x1.921fb54442d17p+3 0x1.921fb54442d18p+2 -0x0.0000000000001p+3',
            '0x1.921fb54442d18p+3 0x1.921fb54442d18p+2  0x0p0',
            '0x1.921fb54442d19p+3 0x1.921fb54442d18p+2  0x0.0000000000001p+3',
            '0x1.f6a7a2955385dp+3 0x1.921fb54442d18p+2  0x1.921fb54442d14p+1',
            '0x1.f6a7a2955385ep+3 0x1.921fb54442d18p+2  0x1.921fb54442d18p+1',
            '0x1.f6a7a2955385fp+3 0x1.921fb54442d18p+2 -0x1.921fb54442d14p+1',
            '0x1.1475cc9eedf00p+5 0x1.921fb54442d18p+2  0x1.921fb54442d10p+1',
            '0x1.1475cc9eedf01p+5 0x1.921fb54442d18p+2 -0x1.921fb54442d10p+1',

            # Symmetry with respect to signs.
            ' 1  0.c  0.4',
            '-1  0.c -0.4',
            ' 1 -0.c  0.4',
            '-1 -0.c -0.4',
            ' 1.4  0.c -0.4',
            '-1.4  0.c  0.4',
            ' 1.4 -0.c -0.4',
            '-1.4 -0.c  0.4',

            # Huge modulus, to check that the underlying algorithm doesn't
            # rely on 2.0 * modulus being representable.
            '0x1.dp+1023 0x1.4p+1023  0x0.9p+1023',
            '0x1.ep+1023 0x1.4p+1023 -0x0.ap+1023',
            '0x1.fp+1023 0x1.4p+1023 -0x0.9p+1023',
        ]

        for case in testcases:
            with self.subTest(case=case):
                x_hex, y_hex, expected_hex = case.split()
                x = float.fromhex(x_hex)
                y = float.fromhex(y_hex)
                expected = float.fromhex(expected_hex)
                validate_spec(x, y, expected)
                actual = math.remainder(x, y)
                # Cheap way of checking that the floats are
                # as identical as we need them to be.
                self.assertEqual(actual.hex(), expected.hex())

        # Test tiny subnormal modulus: there's potential for
        # getting the implementation wrong here (for example,
        # by assuming that modulus/2 is exactly representable).
        tiny = float.fromhex('1p-1074')  # min +ve subnormal
        for n in range(-25, 25):
            if n == 0:
                continue
            y = n * tiny
            for m in range(100):
                x = m * tiny
                actual = math.remainder(x, y)
                validate_spec(x, y, actual)
                actual = math.remainder(-x, y)
                validate_spec(-x, y, actual)

        # Special values.
        # NaNs should propagate as usual.
        for value in [NAN, 0.0, -0.0, 2.0, -2.3, NINF, INF]:
            self.assertIsNaN(math.remainder(NAN, value))
            self.assertIsNaN(math.remainder(value, NAN))

        # remainder(x, inf) is x, for non-nan non-infinite x.
        for value in [-2.3, -0.0, 0.0, 2.3]:
            self.assertEqual(math.remainder(value, INF), value)
            self.assertEqual(math.remainder(value, NINF), value)

        # remainder(x, 0) and remainder(infinity, x) for non-NaN x are invalid
        # operations according to IEEE 754-2008 7.2(f), and should raise.
        for value in [NINF, -2.3, -0.0, 0.0, 2.3, INF]:
            with self.assertRaises(ValueError):
                math.remainder(INF, value)
            with self.assertRaises(ValueError):
                math.remainder(NINF, value)
            with self.assertRaises(ValueError):
                math.remainder(value, 0.0)
            with self.assertRaises(ValueError):
                math.remainder(value, -0.0)

    def testSin(self):
        self.assertRaises(TypeError, math.sin)
        self.ftest('sin(0)', math.sin(0), 0)
        self.ftest('sin(pi/2)', math.sin(math.pi/2), 1)
        self.ftest('sin(-pi/2)', math.sin(-math.pi/2), -1)
        try:
            self.assertTrue(math.isnan(math.sin(INF)))
            self.assertTrue(math.isnan(math.sin(NINF)))
        except ValueError:
            self.assertRaises(ValueError, math.sin, INF)
            self.assertRaises(ValueError, math.sin, NINF)
        self.assertTrue(math.isnan(math.sin(NAN)))

    def testSinh(self):
        self.assertRaises(TypeError, math.sinh)
        self.ftest('sinh(0)', math.sinh(0), 0)
        self.ftest('sinh(1)**2-cosh(1)**2', math.sinh(1)**2-math.cosh(1)**2, -1)
        self.ftest('sinh(1)+sinh(-1)', math.sinh(1)+math.sinh(-1), 0)
        self.assertEqual(math.sinh(INF), INF)
        self.assertEqual(math.sinh(NINF), NINF)
        self.assertTrue(math.isnan(math.sinh(NAN)))

    def testSqrt(self):
        self.assertRaises(TypeError, math.sqrt)
        self.ftest('sqrt(0)', math.sqrt(0), 0)
        self.ftest('sqrt(0)', math.sqrt(0.0), 0.0)
        self.ftest('sqrt(2.5)', math.sqrt(2.5), 1.5811388300841898)
        self.ftest('sqrt(0.25)', math.sqrt(0.25), 0.5)
        self.ftest('sqrt(25.25)', math.sqrt(25.25), 5.024937810560445)
        self.ftest('sqrt(1)', math.sqrt(1), 1)
        self.ftest('sqrt(4)', math.sqrt(4), 2)
        self.assertEqual(math.sqrt(INF), INF)
        self.assertRaises(ValueError, math.sqrt, -1)
        self.assertRaises(ValueError, math.sqrt, NINF)
        self.assertTrue(math.isnan(math.sqrt(NAN)))

    def testTan(self):
        self.assertRaises(TypeError, math.tan)
        self.ftest('tan(0)', math.tan(0), 0)
        self.ftest('tan(pi/4)', math.tan(math.pi/4), 1)
        self.ftest('tan(-pi/4)', math.tan(-math.pi/4), -1)
        try:
            self.assertTrue(math.isnan(math.tan(INF)))
            self.assertTrue(math.isnan(math.tan(NINF)))
        except:
            self.assertRaises(ValueError, math.tan, INF)
            self.assertRaises(ValueError, math.tan, NINF)
        self.assertTrue(math.isnan(math.tan(NAN)))

    def testTanh(self):
        self.assertRaises(TypeError, math.tanh)
        self.ftest('tanh(0)', math.tanh(0), 0)
        self.ftest('tanh(1)+tanh(-1)', math.tanh(1)+math.tanh(-1), 0,
                   abs_tol=math.ulp(1))
        self.ftest('tanh(inf)', math.tanh(INF), 1)
        self.ftest('tanh(-inf)', math.tanh(NINF), -1)
        self.assertTrue(math.isnan(math.tanh(NAN)))

    @requires_IEEE_754
    def testTanhSign(self):
        # check that tanh(-0.) == -0. on IEEE 754 systems
        self.assertEqual(math.tanh(-0.), -0.)
        self.assertEqual(math.copysign(1., math.tanh(-0.)),
                         math.copysign(1., -0.))

    def test_trunc(self):
        self.assertEqual(math.trunc(1), 1)
        self.assertEqual(math.trunc(-1), -1)
        self.assertEqual(type(math.trunc(1)), int)
        self.assertEqual(type(math.trunc(1.5)), int)
        self.assertEqual(math.trunc(1.5), 1)
        self.assertEqual(math.trunc(-1.5), -1)
        self.assertEqual(math.trunc(1.999999), 1)
        self.assertEqual(math.trunc(-1.999999), -1)
        self.assertEqual(math.trunc(-0.999999), -0)
        self.assertEqual(math.trunc(-100.999), -100)

        class TestTrunc:
            def __trunc__(self):
                return 23
        class FloatTrunc(float):
            def __trunc__(self):
                return 23
        class TestNoTrunc:
            pass

        self.assertEqual(math.trunc(TestTrunc()), 23)
        self.assertEqual(math.trunc(FloatTrunc()), 23)

        self.assertRaises(TypeError, math.trunc)
        self.assertRaises(TypeError, math.trunc, 1, 2)
        self.assertRaises(TypeError, math.trunc, FloatLike(23.5))
        self.assertRaises(TypeError, math.trunc, TestNoTrunc())

    def testIsfinite(self):
        self.assertTrue(math.isfinite(0.0))
        self.assertTrue(math.isfinite(-0.0))
        self.assertTrue(math.isfinite(1.0))
        self.assertTrue(math.isfinite(-1.0))
        self.assertFalse(math.isfinite(float("nan")))
        self.assertFalse(math.isfinite(float("inf")))
        self.assertFalse(math.isfinite(float("-inf")))

    def testIsnan(self):
        self.assertTrue(math.isnan(float("nan")))
        self.assertTrue(math.isnan(float("-nan")))
        self.assertTrue(math.isnan(float("inf") * 0.))
        self.assertFalse(math.isnan(float("inf")))
        self.assertFalse(math.isnan(0.))
        self.assertFalse(math.isnan(1.))

    def testIsinf(self):
        self.assertTrue(math.isinf(float("inf")))
        self.assertTrue(math.isinf(float("-inf")))
        self.assertTrue(math.isinf(1E400))
        self.assertTrue(math.isinf(-1E400))
        self.assertFalse(math.isinf(float("nan")))
        self.assertFalse(math.isinf(0.))
        self.assertFalse(math.isinf(1.))

    @requires_IEEE_754
    def test_nan_constant(self):
        self.assertTrue(math.isnan(math.nan))

    @requires_IEEE_754
    def test_inf_constant(self):
        self.assertTrue(math.isinf(math.inf))
        self.assertGreater(math.inf, 0.0)
        self.assertEqual(math.inf, float("inf"))
        self.assertEqual(-math.inf, float("-inf"))

    # RED_FLAG 16-Oct-2000 Tim
    # While 2.0 is more consistent about exceptions than previous releases, it
    # still fails this part of the test on some platforms.  For now, we only
    # *run* test_exceptions() in verbose mode, so that this isn't normally
    # tested.
    @unittest.skipUnless(verbose, 'requires verbose mode')
    def test_exceptions(self):
        try:
            x = math.exp(-1000000000)
        except:
            # mathmodule.c is failing to weed out underflows from libm, or
            # we've got an fp format with huge dynamic range
            self.fail("underflowing exp() should not have raised "
                        "an exception")
        if x != 0:
            self.fail("underflowing exp() should have returned 0")

        # If this fails, probably using a strict IEEE-754 conforming libm, and x
        # is +Inf afterwards.  But Python wants overflows detected by default.
        try:
            x = math.exp(1000000000)
        except OverflowError:
            pass
        else:
            self.fail("overflowing exp() didn't trigger OverflowError")

        # If this fails, it could be a puzzle.  One odd possibility is that
        # mathmodule.c's macros are getting confused while comparing
        # Inf (HUGE_VAL) to a NaN, and artificially setting errno to ERANGE
        # as a result (and so raising OverflowError instead).
        try:
            x = math.sqrt(-1.0)
        except ValueError:
            pass
        else:
            self.fail("sqrt(-1) didn't raise ValueError")

    @requires_IEEE_754
    def test_testfile(self):
        # Some tests need to be skipped on ancient OS X versions.
        # See issue #27953.
        SKIP_ON_TIGER = {'tan0064'}

        osx_version = None
        if sys.platform == 'darwin':
            version_txt = platform.mac_ver()[0]
            try:
                osx_version = tuple(map(int, version_txt.split('.')))
            except ValueError:
                pass

        fail_fmt = "{}: {}({!r}): {}"

        failures = []
        for id, fn, ar, ai, er, ei, flags in parse_testfile(test_file):
            # Skip if either the input or result is complex
            if ai != 0.0 or ei != 0.0:
                continue
            if fn in ['rect', 'polar']:
                # no real versions of rect, polar
                continue
            # Skip certain tests on OS X 10.4.
            if osx_version is not None and osx_version < (10, 5):
                if id in SKIP_ON_TIGER:
                    continue

            func = getattr(math, fn)

            if 'invalid' in flags or 'divide-by-zero' in flags:
                er = 'ValueError'
            elif 'overflow' in flags:
                er = 'OverflowError'

            try:
                result = func(ar)
            except ValueError:
                result = 'ValueError'
            except OverflowError:
                result = 'OverflowError'

            # Default tolerances
            ulp_tol, abs_tol = 5, 0.0

            failure = result_check(er, result, ulp_tol, abs_tol)
            if failure is None:
                continue

            msg = fail_fmt.format(id, fn, ar, failure)
            failures.append(msg)

        if failures:
            self.fail('Failures in test_testfile:\n  ' +
                      '\n  '.join(failures))

    @requires_IEEE_754
    def test_mtestfile(self):
        fail_fmt = "{}: {}({!r}): {}"

        failures = []
        for id, fn, arg, expected, flags in parse_mtestfile(math_testcases):
            func = getattr(math, fn)

            if 'invalid' in flags or 'divide-by-zero' in flags:
                expected = 'ValueError'
            elif 'overflow' in flags:
                expected = 'OverflowError'

            try:
                got = func(arg)
            except ValueError:
                got = 'ValueError'
            except OverflowError:
                got = 'OverflowError'

            # Default tolerances
            ulp_tol, abs_tol = 5, 0.0

            # Exceptions to the defaults
            if fn == 'gamma':
                # Experimental results on one platform gave
                # an accuracy of <= 10 ulps across the entire float
                # domain. We weaken that to require 20 ulp accuracy.
                ulp_tol = 20

            elif fn == 'lgamma':
                # we use a weaker accuracy test for lgamma;
                # lgamma only achieves an absolute error of
                # a few multiples of the machine accuracy, in
                # general.
                abs_tol = 1e-15

            elif fn == 'erfc' and arg >= 0.0:
                # erfc has less-than-ideal accuracy for large
                # arguments (x ~ 25 or so), mainly due to the
                # error involved in computing exp(-x*x).
                #
                # Observed between CPython and mpmath at 25 dp:
                #       x <  0 : err <= 2 ulp
                #  0 <= x <  1 : err <= 10 ulp
                #  1 <= x < 10 : err <= 100 ulp
                # 10 <= x < 20 : err <= 300 ulp
                # 20 <= x      : < 600 ulp
                #
                if arg < 1.0:
                    ulp_tol = 10
                elif arg < 10.0:
                    ulp_tol = 100
                else:
                    ulp_tol = 1000

            failure = result_check(expected, got, ulp_tol, abs_tol)
            if failure is None:
                continue

            msg = fail_fmt.format(id, fn, arg, failure)
            failures.append(msg)

        if failures:
            self.fail('Failures in test_mtestfile:\n  ' +
                      '\n  '.join(failures))

    def test_prod(self):
        prod = math.prod
        self.assertEqual(prod([]), 1)
        self.assertEqual(prod([], start=5), 5)
        self.assertEqual(prod(list(range(2,8))), 5040)
        self.assertEqual(prod(iter(list(range(2,8)))), 5040)
        self.assertEqual(prod(range(1, 10), start=10), 3628800)

        self.assertEqual(prod([1, 2, 3, 4, 5]), 120)
        self.assertEqual(prod([1.0, 2.0, 3.0, 4.0, 5.0]), 120.0)
        self.assertEqual(prod([1, 2, 3, 4.0, 5.0]), 120.0)
        self.assertEqual(prod([1.0, 2.0, 3.0, 4, 5]), 120.0)

        # Test overflow in fast-path for integers
        self.assertEqual(prod([1, 1, 2**32, 1, 1]), 2**32)
        # Test overflow in fast-path for floats
        self.assertEqual(prod([1.0, 1.0, 2**32, 1, 1]), float(2**32))

        self.assertRaises(TypeError, prod)
        self.assertRaises(TypeError, prod, 42)
        self.assertRaises(TypeError, prod, ['a', 'b', 'c'])
        self.assertRaises(TypeError, prod, ['a', 'b', 'c'], '')
        self.assertRaises(TypeError, prod, [b'a', b'c'], b'')
        values = [bytearray(b'a'), bytearray(b'b')]
        self.assertRaises(TypeError, prod, values, bytearray(b''))
        self.assertRaises(TypeError, prod, [[1], [2], [3]])
        self.assertRaises(TypeError, prod, [{2:3}])
        self.assertRaises(TypeError, prod, [{2:3}]*2, {2:3})
        self.assertRaises(TypeError, prod, [[1], [2], [3]], [])
        with self.assertRaises(TypeError):
            prod([10, 20], [30, 40])     # start is a keyword-only argument

        self.assertEqual(prod([0, 1, 2, 3]), 0)
        self.assertEqual(prod([1, 0, 2, 3]), 0)
        self.assertEqual(prod([1, 2, 3, 0]), 0)

        def _naive_prod(iterable, start=1):
            for elem in iterable:
                start *= elem
            return start

        # Big integers

        iterable = range(1, 10000)
        self.assertEqual(prod(iterable), _naive_prod(iterable))
        iterable = range(-10000, -1)
        self.assertEqual(prod(iterable), _naive_prod(iterable))
        iterable = range(-1000, 1000)
        self.assertEqual(prod(iterable), 0)

        # Big floats

        iterable = [float(x) for x in range(1, 1000)]
        self.assertEqual(prod(iterable), _naive_prod(iterable))
        iterable = [float(x) for x in range(-1000, -1)]
        self.assertEqual(prod(iterable), _naive_prod(iterable))
        iterable = [float(x) for x in range(-1000, 1000)]
        self.assertIsNaN(prod(iterable))

        # Float tests

        self.assertIsNaN(prod([1, 2, 3, float("nan"), 2, 3]))
        self.assertIsNaN(prod([1, 0, float("nan"), 2, 3]))
        self.assertIsNaN(prod([1, float("nan"), 0, 3]))
        self.assertIsNaN(prod([1, float("inf"), float("nan"),3]))
        self.assertIsNaN(prod([1, float("-inf"), float("nan"),3]))
        self.assertIsNaN(prod([1, float("nan"), float("inf"),3]))
        self.assertIsNaN(prod([1, float("nan"), float("-inf"),3]))

        self.assertEqual(prod([1, 2, 3, float('inf'),-3,4]), float('-inf'))
        self.assertEqual(prod([1, 2, 3, float('-inf'),-3,4]), float('inf'))

        self.assertIsNaN(prod([1,2,0,float('inf'), -3, 4]))
        self.assertIsNaN(prod([1,2,0,float('-inf'), -3, 4]))
        self.assertIsNaN(prod([1, 2, 3, float('inf'), -3, 0, 3]))
        self.assertIsNaN(prod([1, 2, 3, float('-inf'), -3, 0, 2]))

        # Type preservation

        self.assertEqual(type(prod([1, 2, 3, 4, 5, 6])), int)
        self.assertEqual(type(prod([1, 2.0, 3, 4, 5, 6])), float)
        self.assertEqual(type(prod(range(1, 10000))), int)
        self.assertEqual(type(prod(range(1, 10000), start=1.0)), float)
        self.assertEqual(type(prod([1, decimal.Decimal(2.0), 3, 4, 5, 6])),
                         decimal.Decimal)

    def testPerm(self):
        perm = math.perm
        factorial = math.factorial
        # Test if factorial definition is satisfied
        for n in range(100):
            for k in range(n + 1):
                self.assertEqual(perm(n, k),
                                 factorial(n) // factorial(n - k))

        # Test for Pascal's identity
        for n in range(1, 100):
            for k in range(1, n):
                self.assertEqual(perm(n, k), perm(n - 1, k - 1) * k + perm(n - 1, k))

        # Test corner cases
        for n in range(1, 100):
            self.assertEqual(perm(n, 0), 1)
            self.assertEqual(perm(n, 1), n)
            self.assertEqual(perm(n, n), factorial(n))

        # Test one argument form
        for n in range(20):
            self.assertEqual(perm(n), factorial(n))
            self.assertEqual(perm(n, None), factorial(n))

        # Raises TypeError if any argument is non-integer or argument count is
        # not 1 or 2
        self.assertRaises(TypeError, perm, 10, 1.0)
        self.assertRaises(TypeError, perm, 10, decimal.Decimal(1.0))
        self.assertRaises(TypeError, perm, 10, "1")
        self.assertRaises(TypeError, perm, 10.0, 1)
        self.assertRaises(TypeError, perm, decimal.Decimal(10.0), 1)
        self.assertRaises(TypeError, perm, "10", 1)

        self.assertRaises(TypeError, perm)
        self.assertRaises(TypeError, perm, 10, 1, 3)
        self.assertRaises(TypeError, perm)

        # Raises Value error if not k or n are negative numbers
        self.assertRaises(ValueError, perm, -1, 1)
        self.assertRaises(ValueError, perm, -2**1000, 1)
        self.assertRaises(ValueError, perm, 1, -1)
        self.assertRaises(ValueError, perm, 1, -2**1000)

        # Returns zero if k is greater than n
        self.assertEqual(perm(1, 2), 0)
        self.assertEqual(perm(1, 2**1000), 0)

        n = 2**1000
        self.assertEqual(perm(n, 0), 1)
        self.assertEqual(perm(n, 1), n)
        self.assertEqual(perm(n, 2), n * (n-1))
        if support.check_impl_detail(cpython=True):
            self.assertRaises(OverflowError, perm, n, n)

        for n, k in (True, True), (True, False), (False, False):
            self.assertEqual(perm(n, k), 1)
            self.assertIs(type(perm(n, k)), int)
        self.assertEqual(perm(IntSubclass(5), IntSubclass(2)), 20)
        self.assertEqual(perm(MyIndexable(5), MyIndexable(2)), 20)
        for k in range(3):
            self.assertIs(type(perm(IntSubclass(5), IntSubclass(k))), int)
            self.assertIs(type(perm(MyIndexable(5), MyIndexable(k))), int)

    def testComb(self):
        comb = math.comb
        factorial = math.factorial
        # Test if factorial definition is satisfied
        for n in range(100):
            for k in range(n + 1):
                self.assertEqual(comb(n, k), factorial(n)
                    // (factorial(k) * factorial(n - k)))

        # Test for Pascal's identity
        for n in range(1, 100):
            for k in range(1, n):
                self.assertEqual(comb(n, k), comb(n - 1, k - 1) + comb(n - 1, k))

        # Test corner cases
        for n in range(100):
            self.assertEqual(comb(n, 0), 1)
            self.assertEqual(comb(n, n), 1)

        for n in range(1, 100):
            self.assertEqual(comb(n, 1), n)
            self.assertEqual(comb(n, n - 1), n)

        # Test Symmetry
        for n in range(100):
            for k in range(n // 2):
                self.assertEqual(comb(n, k), comb(n, n - k))

        # Raises TypeError if any argument is non-integer or argument count is
        # not 2
        self.assertRaises(TypeError, comb, 10, 1.0)
        self.assertRaises(TypeError, comb, 10, decimal.Decimal(1.0))
        self.assertRaises(TypeError, comb, 10, "1")
        self.assertRaises(TypeError, comb, 10.0, 1)
        self.assertRaises(TypeError, comb, decimal.Decimal(10.0), 1)
        self.assertRaises(TypeError, comb, "10", 1)

        self.assertRaises(TypeError, comb, 10)
        self.assertRaises(TypeError, comb, 10, 1, 3)
        self.assertRaises(TypeError, comb)

        # Raises Value error if not k or n are negative numbers
        self.assertRaises(ValueError, comb, -1, 1)
        self.assertRaises(ValueError, comb, -2**1000, 1)
        self.assertRaises(ValueError, comb, 1, -1)
        self.assertRaises(ValueError, comb, 1, -2**1000)

        # Returns zero if k is greater than n
        self.assertEqual(comb(1, 2), 0)
        self.assertEqual(comb(1, 2**1000), 0)

        n = 2**1000
        self.assertEqual(comb(n, 0), 1)
        self.assertEqual(comb(n, 1), n)
        self.assertEqual(comb(n, 2), n * (n-1) // 2)
        self.assertEqual(comb(n, n), 1)
        self.assertEqual(comb(n, n-1), n)
        self.assertEqual(comb(n, n-2), n * (n-1) // 2)
        if support.check_impl_detail(cpython=True):
            self.assertRaises(OverflowError, comb, n, n//2)

        for n, k in (True, True), (True, False), (False, False):
            self.assertEqual(comb(n, k), 1)
            self.assertIs(type(comb(n, k)), int)
        self.assertEqual(comb(IntSubclass(5), IntSubclass(2)), 10)
        self.assertEqual(comb(MyIndexable(5), MyIndexable(2)), 10)
        for k in range(3):
            self.assertIs(type(comb(IntSubclass(5), IntSubclass(k))), int)
            self.assertIs(type(comb(MyIndexable(5), MyIndexable(k))), int)

    @requires_IEEE_754
    def test_nextafter(self):
        # around 2^52 and 2^63
        self.assertEqual(math.nextafter(4503599627370496.0, -INF),
                         4503599627370495.5)
        self.assertEqual(math.nextafter(4503599627370496.0, INF),
                         4503599627370497.0)
        self.assertEqual(math.nextafter(9223372036854775808.0, 0.0),
                         9223372036854774784.0)
        self.assertEqual(math.nextafter(-9223372036854775808.0, 0.0),
                         -9223372036854774784.0)

        # around 1.0
        self.assertEqual(math.nextafter(1.0, -INF),
                         float.fromhex('0x1.fffffffffffffp-1'))
        self.assertEqual(math.nextafter(1.0, INF),
                         float.fromhex('0x1.0000000000001p+0'))

        # x == y: y is returned
        self.assertEqual(math.nextafter(2.0, 2.0), 2.0)
        self.assertEqualSign(math.nextafter(-0.0, +0.0), +0.0)
        self.assertEqualSign(math.nextafter(+0.0, -0.0), -0.0)

        # around 0.0
        smallest_subnormal = sys.float_info.min * sys.float_info.epsilon
        self.assertEqual(math.nextafter(+0.0, INF), smallest_subnormal)
        self.assertEqual(math.nextafter(-0.0, INF), smallest_subnormal)
        self.assertEqual(math.nextafter(+0.0, -INF), -smallest_subnormal)
        self.assertEqual(math.nextafter(-0.0, -INF), -smallest_subnormal)
        self.assertEqualSign(math.nextafter(smallest_subnormal, +0.0), +0.0)
        self.assertEqualSign(math.nextafter(-smallest_subnormal, +0.0), -0.0)
        self.assertEqualSign(math.nextafter(smallest_subnormal, -0.0), +0.0)
        self.assertEqualSign(math.nextafter(-smallest_subnormal, -0.0), -0.0)

        # around infinity
        largest_normal = sys.float_info.max
        self.assertEqual(math.nextafter(INF, 0.0), largest_normal)
        self.assertEqual(math.nextafter(-INF, 0.0), -largest_normal)
        self.assertEqual(math.nextafter(largest_normal, INF), INF)
        self.assertEqual(math.nextafter(-largest_normal, -INF), -INF)

        # NaN
        self.assertIsNaN(math.nextafter(NAN, 1.0))
        self.assertIsNaN(math.nextafter(1.0, NAN))
        self.assertIsNaN(math.nextafter(NAN, NAN))

    @requires_IEEE_754
    def test_ulp(self):
        self.assertEqual(math.ulp(1.0), sys.float_info.epsilon)
        # use int ** int rather than float ** int to not rely on pow() accuracy
        self.assertEqual(math.ulp(2 ** 52), 1.0)
        self.assertEqual(math.ulp(2 ** 53), 2.0)
        self.assertEqual(math.ulp(2 ** 64), 4096.0)

        # min and max
        self.assertEqual(math.ulp(0.0),
                         sys.float_info.min * sys.float_info.epsilon)
        self.assertEqual(math.ulp(FLOAT_MAX),
                         FLOAT_MAX - math.nextafter(FLOAT_MAX, -INF))

        # special cases
        self.assertEqual(math.ulp(INF), INF)
        self.assertIsNaN(math.ulp(math.nan))

        # negative number: ulp(-x) == ulp(x)
        for x in (0.0, 1.0, 2 ** 52, 2 ** 64, INF):
            with self.subTest(x=x):
                self.assertEqual(math.ulp(-x), math.ulp(x))

    def test_issue39871(self):
        # A SystemError should not be raised if the first arg to atan2(),
        # copysign(), or remainder() cannot be converted to a float.
        class F:
            def __float__(self):
                self.converted = True
                1/0
        for func in math.atan2, math.copysign, math.remainder:
            y = F()
            with self.assertRaises(TypeError):
                func("not a number", y)

            # There should not have been any attempt to convert the second
            # argument to a float.
            self.assertFalse(getattr(y, "converted", False))

    # Custom assertions.

    def assertIsNaN(self, value):
        if not math.isnan(value):
            self.fail("Expected a NaN, got {!r}.".format(value))

    def assertEqualSign(self, x, y):
        """Similar to assertEqual(), but compare also the sign with copysign().

        Function useful to compare signed zeros.
        """
        self.assertEqual(x, y)
        self.assertEqual(math.copysign(1.0, x), math.copysign(1.0, y))


class IsCloseTests(unittest.TestCase):
    isclose = math.isclose  # subclasses should override this

    def assertIsClose(self, a, b, *args, **kwargs):
        self.assertTrue(self.isclose(a, b, *args, **kwargs),
                        msg="%s and %s should be close!" % (a, b))

    def assertIsNotClose(self, a, b, *args, **kwargs):
        self.assertFalse(self.isclose(a, b, *args, **kwargs),
                         msg="%s and %s should not be close!" % (a, b))

    def assertAllClose(self, examples, *args, **kwargs):
        for a, b in examples:
            self.assertIsClose(a, b, *args, **kwargs)

    def assertAllNotClose(self, examples, *args, **kwargs):
        for a, b in examples:
            self.assertIsNotClose(a, b, *args, **kwargs)

    def test_negative_tolerances(self):
        # ValueError should be raised if either tolerance is less than zero
        with self.assertRaises(ValueError):
            self.assertIsClose(1, 1, rel_tol=-1e-100)
        with self.assertRaises(ValueError):
            self.assertIsClose(1, 1, rel_tol=1e-100, abs_tol=-1e10)

    def test_identical(self):
        # identical values must test as close
        identical_examples = [(2.0, 2.0),
                              (0.1e200, 0.1e200),
                              (1.123e-300, 1.123e-300),
                              (12345, 12345.0),
                              (0.0, -0.0),
                              (345678, 345678)]
        self.assertAllClose(identical_examples, rel_tol=0.0, abs_tol=0.0)

    def test_eight_decimal_places(self):
        # examples that are close to 1e-8, but not 1e-9
        eight_decimal_places_examples = [(1e8, 1e8 + 1),
                                         (-1e-8, -1.000000009e-8),
                                         (1.12345678, 1.12345679)]
        self.assertAllClose(eight_decimal_places_examples, rel_tol=1e-8)
        self.assertAllNotClose(eight_decimal_places_examples, rel_tol=1e-9)

    def test_near_zero(self):
        # values close to zero
        near_zero_examples = [(1e-9, 0.0),
                              (-1e-9, 0.0),
                              (-1e-150, 0.0)]
        # these should not be close to any rel_tol
        self.assertAllNotClose(near_zero_examples, rel_tol=0.9)
        # these should be close to abs_tol=1e-8
        self.assertAllClose(near_zero_examples, abs_tol=1e-8)

    def test_identical_infinite(self):
        # these are close regardless of tolerance -- i.e. they are equal
        self.assertIsClose(INF, INF)
        self.assertIsClose(INF, INF, abs_tol=0.0)
        self.assertIsClose(NINF, NINF)
        self.assertIsClose(NINF, NINF, abs_tol=0.0)

    def test_inf_ninf_nan(self):
        # these should never be close (following IEEE 754 rules for equality)
        not_close_examples = [(NAN, NAN),
                              (NAN, 1e-100),
                              (1e-100, NAN),
                              (INF, NAN),
                              (NAN, INF),
                              (INF, NINF),
                              (INF, 1.0),
                              (1.0, INF),
                              (INF, 1e308),
                              (1e308, INF)]
        # use largest reasonable tolerance
        self.assertAllNotClose(not_close_examples, abs_tol=0.999999999999999)

    def test_zero_tolerance(self):
        # test with zero tolerance
        zero_tolerance_close_examples = [(1.0, 1.0),
                                         (-3.4, -3.4),
                                         (-1e-300, -1e-300)]
        self.assertAllClose(zero_tolerance_close_examples, rel_tol=0.0)

        zero_tolerance_not_close_examples = [(1.0, 1.000000000000001),
                                             (0.99999999999999, 1.0),
                                             (1.0e200, .999999999999999e200)]
        self.assertAllNotClose(zero_tolerance_not_close_examples, rel_tol=0.0)

    def test_asymmetry(self):
        # test the asymmetry example from PEP 485
        self.assertAllClose([(9, 10), (10, 9)], rel_tol=0.1)

    def test_integers(self):
        # test with integer values
        integer_examples = [(100000001, 100000000),
                            (123456789, 123456788)]

        self.assertAllClose(integer_examples, rel_tol=1e-8)
        self.assertAllNotClose(integer_examples, rel_tol=1e-9)

    def test_decimals(self):
        # test with Decimal values
        from decimal import Decimal

        decimal_examples = [(Decimal('1.00000001'), Decimal('1.0')),
                            (Decimal('1.00000001e-20'), Decimal('1.0e-20')),
                            (Decimal('1.00000001e-100'), Decimal('1.0e-100')),
                            (Decimal('1.00000001e20'), Decimal('1.0e20'))]
        self.assertAllClose(decimal_examples, rel_tol=1e-8)
        self.assertAllNotClose(decimal_examples, rel_tol=1e-9)

    def test_fractions(self):
        # test with Fraction values
        from fractions import Fraction

        fraction_examples = [
            (Fraction(1, 100000000) + 1, Fraction(1)),
            (Fraction(100000001), Fraction(100000000)),
            (Fraction(10**8 + 1, 10**28), Fraction(1, 10**20))]
        self.assertAllClose(fraction_examples, rel_tol=1e-8)
        self.assertAllNotClose(fraction_examples, rel_tol=1e-9)


def test_main():
    from doctest import DocFileSuite
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(MathTests))
    suite.addTest(unittest.makeSuite(IsCloseTests))
    suite.addTest(DocFileSuite("ieee754.txt"))
    run_unittest(suite)

if __name__ == '__main__':
    test_main()
