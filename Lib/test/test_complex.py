import unittest
from test import test_support

from random import random
from math import atan2, isnan, copysign

INF = float("inf")
NAN = float("nan")
# These tests ensure that complex math does the right thing

class ComplexTest(unittest.TestCase):

    def assertAlmostEqual(self, a, b):
        if isinstance(a, complex):
            if isinstance(b, complex):
                unittest.TestCase.assertAlmostEqual(self, a.real, b.real)
                unittest.TestCase.assertAlmostEqual(self, a.imag, b.imag)
            else:
                unittest.TestCase.assertAlmostEqual(self, a.real, b)
                unittest.TestCase.assertAlmostEqual(self, a.imag, 0.)
        else:
            if isinstance(b, complex):
                unittest.TestCase.assertAlmostEqual(self, a, b.real)
                unittest.TestCase.assertAlmostEqual(self, 0., b.imag)
            else:
                unittest.TestCase.assertAlmostEqual(self, a, b)

    def assertCloseAbs(self, x, y, eps=1e-9):
        """Return true iff floats x and y "are close\""""
        # put the one with larger magnitude second
        if abs(x) > abs(y):
            x, y = y, x
        if y == 0:
            return abs(x) < eps
        if x == 0:
            return abs(y) < eps
        # check that relative difference < eps
        self.assertTrue(abs((x-y)/y) < eps)

    def assertFloatsAreIdentical(self, x, y):
        """assert that floats x and y are identical, in the sense that:
        (1) both x and y are nans, or
        (2) both x and y are infinities, with the same sign, or
        (3) both x and y are zeros, with the same sign, or
        (4) x and y are both finite and nonzero, and x == y

        """
        msg = 'floats {!r} and {!r} are not identical'

        if isnan(x) or isnan(y):
            if isnan(x) and isnan(y):
                return
        elif x == y:
            if x != 0.0:
                return
            # both zero; check that signs match
            elif copysign(1.0, x) == copysign(1.0, y):
                return
            else:
                msg += ': zeros have different signs'
        self.fail(msg.format(x, y))

    def assertClose(self, x, y, eps=1e-9):
        """Return true iff complexes x and y "are close\""""
        self.assertCloseAbs(x.real, y.real, eps)
        self.assertCloseAbs(x.imag, y.imag, eps)

    def check_div(self, x, y):
        """Compute complex z=x*y, and check that z/x==y and z/y==x."""
        z = x * y
        if x != 0:
            q = z / x
            self.assertClose(q, y)
            q = z.__div__(x)
            self.assertClose(q, y)
            q = z.__truediv__(x)
            self.assertClose(q, y)
        if y != 0:
            q = z / y
            self.assertClose(q, x)
            q = z.__div__(y)
            self.assertClose(q, x)
            q = z.__truediv__(y)
            self.assertClose(q, x)

    def test_div(self):
        simple_real = [float(i) for i in xrange(-5, 6)]
        simple_complex = [complex(x, y) for x in simple_real for y in simple_real]
        for x in simple_complex:
            for y in simple_complex:
                self.check_div(x, y)

        # A naive complex division algorithm (such as in 2.0) is very prone to
        # nonsense errors for these (overflows and underflows).
        self.check_div(complex(1e200, 1e200), 1+0j)
        self.check_div(complex(1e-200, 1e-200), 1+0j)

        # Just for fun.
        for i in xrange(100):
            self.check_div(complex(random(), random()),
                           complex(random(), random()))

        self.assertRaises(ZeroDivisionError, complex.__div__, 1+1j, 0+0j)
        # FIXME: The following currently crashes on Alpha
        # self.assertRaises(OverflowError, pow, 1e200+1j, 1e200+1j)

    def test_truediv(self):
        self.assertAlmostEqual(complex.__truediv__(2+0j, 1+1j), 1-1j)
        self.assertRaises(ZeroDivisionError, complex.__truediv__, 1+1j, 0+0j)

    def test_floordiv(self):
        self.assertAlmostEqual(complex.__floordiv__(3+0j, 1.5+0j), 2)
        self.assertRaises(ZeroDivisionError, complex.__floordiv__, 3+0j, 0+0j)

    def test_coerce(self):
        self.assertRaises(OverflowError, complex.__coerce__, 1+1j, 1L<<10000)

    def test_no_implicit_coerce(self):
        # Python 2.7 removed implicit coercion from the complex type
        class A(object):
            def __coerce__(self, other):
                raise RuntimeError
            __hash__ = None
            def __cmp__(self, other):
                return -1

        a = A()
        self.assertRaises(TypeError, lambda: a + 2.0j)
        self.assertTrue(a < 2.0j)

    def test_richcompare(self):
        self.assertEqual(complex.__eq__(1+1j, 1L<<10000), False)
        self.assertEqual(complex.__lt__(1+1j, None), NotImplemented)
        self.assertIs(complex.__eq__(1+1j, 1+1j), True)
        self.assertIs(complex.__eq__(1+1j, 2+2j), False)
        self.assertIs(complex.__ne__(1+1j, 1+1j), False)
        self.assertIs(complex.__ne__(1+1j, 2+2j), True)
        self.assertRaises(TypeError, complex.__lt__, 1+1j, 2+2j)
        self.assertRaises(TypeError, complex.__le__, 1+1j, 2+2j)
        self.assertRaises(TypeError, complex.__gt__, 1+1j, 2+2j)
        self.assertRaises(TypeError, complex.__ge__, 1+1j, 2+2j)

    def test_richcompare_boundaries(self):
        def check(n, deltas, is_equal, imag = 0.0):
            for delta in deltas:
                i = n + delta
                z = complex(i, imag)
                self.assertIs(complex.__eq__(z, i), is_equal(delta))
                self.assertIs(complex.__ne__(z, i), not is_equal(delta))
        # For IEEE-754 doubles the following should hold:
        #    x in [2 ** (52 + i), 2 ** (53 + i + 1)] -> x mod 2 ** i == 0
        # where the interval is representable, of course.
        for i in range(1, 10):
            pow = 52 + i
            mult = 2 ** i
            check(2 ** pow, range(1, 101), lambda delta: delta % mult == 0)
            check(2 ** pow, range(1, 101), lambda delta: False, float(i))
        check(2 ** 53, range(-100, 0), lambda delta: True)

    def test_mod(self):
        self.assertRaises(ZeroDivisionError, (1+1j).__mod__, 0+0j)

        a = 3.33+4.43j
        try:
            a % 0
        except ZeroDivisionError:
            pass
        else:
            self.fail("modulo parama can't be 0")

    def test_divmod(self):
        self.assertRaises(ZeroDivisionError, divmod, 1+1j, 0+0j)

    def test_pow(self):
        self.assertAlmostEqual(pow(1+1j, 0+0j), 1.0)
        self.assertAlmostEqual(pow(0+0j, 2+0j), 0.0)
        self.assertRaises(ZeroDivisionError, pow, 0+0j, 1j)
        self.assertAlmostEqual(pow(1j, -1), 1/1j)
        self.assertAlmostEqual(pow(1j, 200), 1)
        self.assertRaises(ValueError, pow, 1+1j, 1+1j, 1+1j)

        a = 3.33+4.43j
        self.assertEqual(a ** 0j, 1)
        self.assertEqual(a ** 0.+0.j, 1)

        self.assertEqual(3j ** 0j, 1)
        self.assertEqual(3j ** 0, 1)

        try:
            0j ** a
        except ZeroDivisionError:
            pass
        else:
            self.fail("should fail 0.0 to negative or complex power")

        try:
            0j ** (3-2j)
        except ZeroDivisionError:
            pass
        else:
            self.fail("should fail 0.0 to negative or complex power")

        # The following is used to exercise certain code paths
        self.assertEqual(a ** 105, a ** 105)
        self.assertEqual(a ** -105, a ** -105)
        self.assertEqual(a ** -30, a ** -30)

        self.assertEqual(0.0j ** 0, 1)

        b = 5.1+2.3j
        self.assertRaises(ValueError, pow, a, b, 0)

    def test_boolcontext(self):
        for i in xrange(100):
            self.assertTrue(complex(random() + 1e-6, random() + 1e-6))
        self.assertTrue(not complex(0.0, 0.0))

    def test_conjugate(self):
        self.assertClose(complex(5.3, 9.8).conjugate(), 5.3-9.8j)

    def test_constructor(self):
        class OS:
            def __init__(self, value): self.value = value
            def __complex__(self): return self.value
        class NS(object):
            def __init__(self, value): self.value = value
            def __complex__(self): return self.value
        self.assertEqual(complex(OS(1+10j)), 1+10j)
        self.assertEqual(complex(NS(1+10j)), 1+10j)
        self.assertRaises(TypeError, complex, OS(None))
        self.assertRaises(TypeError, complex, NS(None))

        self.assertAlmostEqual(complex("1+10j"), 1+10j)
        self.assertAlmostEqual(complex(10), 10+0j)
        self.assertAlmostEqual(complex(10.0), 10+0j)
        self.assertAlmostEqual(complex(10L), 10+0j)
        self.assertAlmostEqual(complex(10+0j), 10+0j)
        self.assertAlmostEqual(complex(1,10), 1+10j)
        self.assertAlmostEqual(complex(1,10L), 1+10j)
        self.assertAlmostEqual(complex(1,10.0), 1+10j)
        self.assertAlmostEqual(complex(1L,10), 1+10j)
        self.assertAlmostEqual(complex(1L,10L), 1+10j)
        self.assertAlmostEqual(complex(1L,10.0), 1+10j)
        self.assertAlmostEqual(complex(1.0,10), 1+10j)
        self.assertAlmostEqual(complex(1.0,10L), 1+10j)
        self.assertAlmostEqual(complex(1.0,10.0), 1+10j)
        self.assertAlmostEqual(complex(3.14+0j), 3.14+0j)
        self.assertAlmostEqual(complex(3.14), 3.14+0j)
        self.assertAlmostEqual(complex(314), 314.0+0j)
        self.assertAlmostEqual(complex(314L), 314.0+0j)
        self.assertAlmostEqual(complex(3.14+0j, 0j), 3.14+0j)
        self.assertAlmostEqual(complex(3.14, 0.0), 3.14+0j)
        self.assertAlmostEqual(complex(314, 0), 314.0+0j)
        self.assertAlmostEqual(complex(314L, 0L), 314.0+0j)
        self.assertAlmostEqual(complex(0j, 3.14j), -3.14+0j)
        self.assertAlmostEqual(complex(0.0, 3.14j), -3.14+0j)
        self.assertAlmostEqual(complex(0j, 3.14), 3.14j)
        self.assertAlmostEqual(complex(0.0, 3.14), 3.14j)
        self.assertAlmostEqual(complex("1"), 1+0j)
        self.assertAlmostEqual(complex("1j"), 1j)
        self.assertAlmostEqual(complex(),  0)
        self.assertAlmostEqual(complex("-1"), -1)
        self.assertAlmostEqual(complex("+1"), +1)
        self.assertAlmostEqual(complex("(1+2j)"), 1+2j)
        self.assertAlmostEqual(complex("(1.3+2.2j)"), 1.3+2.2j)
        self.assertAlmostEqual(complex("3.14+1J"), 3.14+1j)
        self.assertAlmostEqual(complex(" ( +3.14-6J )"), 3.14-6j)
        self.assertAlmostEqual(complex(" ( +3.14-J )"), 3.14-1j)
        self.assertAlmostEqual(complex(" ( +3.14+j )"), 3.14+1j)
        self.assertAlmostEqual(complex("J"), 1j)
        self.assertAlmostEqual(complex("( j )"), 1j)
        self.assertAlmostEqual(complex("+J"), 1j)
        self.assertAlmostEqual(complex("( -j)"), -1j)
        self.assertAlmostEqual(complex('1e-500'), 0.0 + 0.0j)
        self.assertAlmostEqual(complex('-1e-500j'), 0.0 - 0.0j)
        self.assertAlmostEqual(complex('-1e-500+1e-500j'), -0.0 + 0.0j)

        class complex2(complex): pass
        self.assertAlmostEqual(complex(complex2(1+1j)), 1+1j)
        self.assertAlmostEqual(complex(real=17, imag=23), 17+23j)
        self.assertAlmostEqual(complex(real=17+23j), 17+23j)
        self.assertAlmostEqual(complex(real=17+23j, imag=23), 17+46j)
        self.assertAlmostEqual(complex(real=1+2j, imag=3+4j), -3+5j)

        # check that the sign of a zero in the real or imaginary part
        # is preserved when constructing from two floats.  (These checks
        # are harmless on systems without support for signed zeros.)
        def split_zeros(x):
            """Function that produces different results for 0. and -0."""
            return atan2(x, -1.)

        self.assertEqual(split_zeros(complex(1., 0.).imag), split_zeros(0.))
        self.assertEqual(split_zeros(complex(1., -0.).imag), split_zeros(-0.))
        self.assertEqual(split_zeros(complex(0., 1.).real), split_zeros(0.))
        self.assertEqual(split_zeros(complex(-0., 1.).real), split_zeros(-0.))

        c = 3.14 + 1j
        self.assertTrue(complex(c) is c)
        del c

        self.assertRaises(TypeError, complex, "1", "1")
        self.assertRaises(TypeError, complex, 1, "1")

        if test_support.have_unicode:
            self.assertEqual(complex(unicode("  3.14+J  ")), 3.14+1j)

        # SF bug 543840:  complex(string) accepts strings with \0
        # Fixed in 2.3.
        self.assertRaises(ValueError, complex, '1+1j\0j')

        self.assertRaises(TypeError, int, 5+3j)
        self.assertRaises(TypeError, long, 5+3j)
        self.assertRaises(TypeError, float, 5+3j)
        self.assertRaises(ValueError, complex, "")
        self.assertRaises(TypeError, complex, None)
        self.assertRaises(ValueError, complex, "\0")
        self.assertRaises(ValueError, complex, "3\09")
        self.assertRaises(TypeError, complex, "1", "2")
        self.assertRaises(TypeError, complex, "1", 42)
        self.assertRaises(TypeError, complex, 1, "2")
        self.assertRaises(ValueError, complex, "1+")
        self.assertRaises(ValueError, complex, "1+1j+1j")
        self.assertRaises(ValueError, complex, "--")
        self.assertRaises(ValueError, complex, "(1+2j")
        self.assertRaises(ValueError, complex, "1+2j)")
        self.assertRaises(ValueError, complex, "1+(2j)")
        self.assertRaises(ValueError, complex, "(1+2j)123")
        if test_support.have_unicode:
            self.assertRaises(ValueError, complex, unicode("x"))
        self.assertRaises(ValueError, complex, "1j+2")
        self.assertRaises(ValueError, complex, "1e1ej")
        self.assertRaises(ValueError, complex, "1e++1ej")
        self.assertRaises(ValueError, complex, ")1+2j(")
        # the following three are accepted by Python 2.6
        self.assertRaises(ValueError, complex, "1..1j")
        self.assertRaises(ValueError, complex, "1.11.1j")
        self.assertRaises(ValueError, complex, "1e1.1j")

        if test_support.have_unicode:
            # check that complex accepts long unicode strings
            self.assertEqual(type(complex(unicode("1"*500))), complex)

        class EvilExc(Exception):
            pass

        class evilcomplex:
            def __complex__(self):
                raise EvilExc

        self.assertRaises(EvilExc, complex, evilcomplex())

        class float2:
            def __init__(self, value):
                self.value = value
            def __float__(self):
                return self.value

        self.assertAlmostEqual(complex(float2(42.)), 42)
        self.assertAlmostEqual(complex(real=float2(17.), imag=float2(23.)), 17+23j)
        self.assertRaises(TypeError, complex, float2(None))

        class complex0(complex):
            """Test usage of __complex__() when inheriting from 'complex'"""
            def __complex__(self):
                return 42j

        class complex1(complex):
            """Test usage of __complex__() with a __new__() method"""
            def __new__(self, value=0j):
                return complex.__new__(self, 2*value)
            def __complex__(self):
                return self

        class complex2(complex):
            """Make sure that __complex__() calls fail if anything other than a
            complex is returned"""
            def __complex__(self):
                return None

        self.assertAlmostEqual(complex(complex0(1j)), 42j)
        self.assertAlmostEqual(complex(complex1(1j)), 2j)
        self.assertRaises(TypeError, complex, complex2(1j))

    def test_subclass(self):
        class xcomplex(complex):
            def __add__(self,other):
                return xcomplex(complex(self) + other)
            __radd__ = __add__

            def __sub__(self,other):
                return xcomplex(complex(self) + other)
            __rsub__ = __sub__

            def __mul__(self,other):
                return xcomplex(complex(self) * other)
            __rmul__ = __mul__

            def __div__(self,other):
                return xcomplex(complex(self) / other)

            def __rdiv__(self,other):
                return xcomplex(other / complex(self))

            __truediv__ = __div__
            __rtruediv__ = __rdiv__

            def __floordiv__(self,other):
                return xcomplex(complex(self) // other)

            def __rfloordiv__(self,other):
                return xcomplex(other // complex(self))

            def __pow__(self,other):
                return xcomplex(complex(self) ** other)

            def __rpow__(self,other):
                return xcomplex(other ** complex(self) )

            def __mod__(self,other):
                return xcomplex(complex(self) % other)

            def __rmod__(self,other):
                return xcomplex(other % complex(self))

        infix_binops = ('+', '-', '*', '**', '%', '//', '/')
        xcomplex_values = (xcomplex(1), xcomplex(123.0),
                           xcomplex(-10+2j), xcomplex(3+187j),
                           xcomplex(3-78j))
        test_values = (1, 123.0, 10-19j, xcomplex(1+2j),
                       xcomplex(1+87j), xcomplex(10+90j))

        for op in infix_binops:
            for x in xcomplex_values:
                for y in test_values:
                    a = 'x %s y' % op
                    b = 'y %s x' % op
                    self.assertTrue(type(eval(a)) is type(eval(b)) is xcomplex)

    def test_hash(self):
        for x in xrange(-30, 30):
            self.assertEqual(hash(x), hash(complex(x, 0)))
            x /= 3.0    # now check against floating point
            self.assertEqual(hash(x), hash(complex(x, 0.)))

    def test_abs(self):
        nums = [complex(x/3., y/7.) for x in xrange(-9,9) for y in xrange(-9,9)]
        for num in nums:
            self.assertAlmostEqual((num.real**2 + num.imag**2)  ** 0.5, abs(num))

    def test_repr(self):
        self.assertEqual(repr(1+6j), '(1+6j)')
        self.assertEqual(repr(1-6j), '(1-6j)')

        self.assertNotEqual(repr(-(1+0j)), '(-1+-0j)')

        self.assertEqual(1-6j,complex(repr(1-6j)))
        self.assertEqual(1+6j,complex(repr(1+6j)))
        self.assertEqual(-6j,complex(repr(-6j)))
        self.assertEqual(6j,complex(repr(6j)))

        self.assertEqual(repr(complex(1., INF)), "(1+infj)")
        self.assertEqual(repr(complex(1., -INF)), "(1-infj)")
        self.assertEqual(repr(complex(INF, 1)), "(inf+1j)")
        self.assertEqual(repr(complex(-INF, INF)), "(-inf+infj)")
        self.assertEqual(repr(complex(NAN, 1)), "(nan+1j)")
        self.assertEqual(repr(complex(1, NAN)), "(1+nanj)")
        self.assertEqual(repr(complex(NAN, NAN)), "(nan+nanj)")

        self.assertEqual(repr(complex(0, INF)), "infj")
        self.assertEqual(repr(complex(0, -INF)), "-infj")
        self.assertEqual(repr(complex(0, NAN)), "nanj")

    def test_neg(self):
        self.assertEqual(-(1+6j), -1-6j)

    def test_file(self):
        a = 3.33+4.43j
        b = 5.1+2.3j

        fo = None
        try:
            fo = open(test_support.TESTFN, "wb")
            print >>fo, a, b
            fo.close()
            fo = open(test_support.TESTFN, "rb")
            self.assertEqual(fo.read(), "%s %s\n" % (a, b))
        finally:
            if (fo is not None) and (not fo.closed):
                fo.close()
            test_support.unlink(test_support.TESTFN)

    def test_getnewargs(self):
        self.assertEqual((1+2j).__getnewargs__(), (1.0, 2.0))
        self.assertEqual((1-2j).__getnewargs__(), (1.0, -2.0))
        self.assertEqual((2j).__getnewargs__(), (0.0, 2.0))
        self.assertEqual((-0j).__getnewargs__(), (0.0, -0.0))
        self.assertEqual(complex(0, INF).__getnewargs__(), (0.0, INF))
        self.assertEqual(complex(INF, 0).__getnewargs__(), (INF, 0.0))

    if float.__getformat__("double").startswith("IEEE"):
        def test_plus_minus_0j(self):
            # test that -0j and 0j literals are not identified
            z1, z2 = 0j, -0j
            self.assertEqual(atan2(z1.imag, -1.), atan2(0., -1.))
            self.assertEqual(atan2(z2.imag, -1.), atan2(-0., -1.))

    @unittest.skipUnless(float.__getformat__("double").startswith("IEEE"),
                         "test requires IEEE 754 doubles")
    def test_overflow(self):
        self.assertEqual(complex("1e500"), complex(INF, 0.0))
        self.assertEqual(complex("-1e500j"), complex(0.0, -INF))
        self.assertEqual(complex("-1e500+1.8e308j"), complex(-INF, INF))

    @unittest.skipUnless(float.__getformat__("double").startswith("IEEE"),
                         "test requires IEEE 754 doubles")
    def test_repr_roundtrip(self):
        vals = [0.0, 1e-500, 1e-315, 1e-200, 0.0123, 3.1415, 1e50, INF, NAN]
        vals += [-v for v in vals]

        # complex(repr(z)) should recover z exactly, even for complex
        # numbers involving an infinity, nan, or negative zero
        for x in vals:
            for y in vals:
                z = complex(x, y)
                roundtrip = complex(repr(z))
                self.assertFloatsAreIdentical(z.real, roundtrip.real)
                self.assertFloatsAreIdentical(z.imag, roundtrip.imag)

        # if we predefine some constants, then eval(repr(z)) should
        # also work, except that it might change the sign of zeros
        inf, nan = float('inf'), float('nan')
        infj, nanj = complex(0.0, inf), complex(0.0, nan)
        for x in vals:
            for y in vals:
                z = complex(x, y)
                roundtrip = eval(repr(z))
                # adding 0.0 has no effect beside changing -0.0 to 0.0
                self.assertFloatsAreIdentical(0.0 + z.real,
                                              0.0 + roundtrip.real)
                self.assertFloatsAreIdentical(0.0 + z.imag,
                                              0.0 + roundtrip.imag)

    def test_format(self):
        # empty format string is same as str()
        self.assertEqual(format(1+3j, ''), str(1+3j))
        self.assertEqual(format(1.5+3.5j, ''), str(1.5+3.5j))
        self.assertEqual(format(3j, ''), str(3j))
        self.assertEqual(format(3.2j, ''), str(3.2j))
        self.assertEqual(format(3+0j, ''), str(3+0j))
        self.assertEqual(format(3.2+0j, ''), str(3.2+0j))

        # empty presentation type should still be analogous to str,
        # even when format string is nonempty (issue #5920).
        self.assertEqual(format(3.2+0j, '-'), str(3.2+0j))
        self.assertEqual(format(3.2+0j, '<'), str(3.2+0j))
        z = 4/7. - 100j/7.
        self.assertEqual(format(z, ''), str(z))
        self.assertEqual(format(z, '-'), str(z))
        self.assertEqual(format(z, '<'), str(z))
        self.assertEqual(format(z, '10'), str(z))
        z = complex(0.0, 3.0)
        self.assertEqual(format(z, ''), str(z))
        self.assertEqual(format(z, '-'), str(z))
        self.assertEqual(format(z, '<'), str(z))
        self.assertEqual(format(z, '2'), str(z))
        z = complex(-0.0, 2.0)
        self.assertEqual(format(z, ''), str(z))
        self.assertEqual(format(z, '-'), str(z))
        self.assertEqual(format(z, '<'), str(z))
        self.assertEqual(format(z, '3'), str(z))

        self.assertEqual(format(1+3j, 'g'), '1+3j')
        self.assertEqual(format(3j, 'g'), '0+3j')
        self.assertEqual(format(1.5+3.5j, 'g'), '1.5+3.5j')

        self.assertEqual(format(1.5+3.5j, '+g'), '+1.5+3.5j')
        self.assertEqual(format(1.5-3.5j, '+g'), '+1.5-3.5j')
        self.assertEqual(format(1.5-3.5j, '-g'), '1.5-3.5j')
        self.assertEqual(format(1.5+3.5j, ' g'), ' 1.5+3.5j')
        self.assertEqual(format(1.5-3.5j, ' g'), ' 1.5-3.5j')
        self.assertEqual(format(-1.5+3.5j, ' g'), '-1.5+3.5j')
        self.assertEqual(format(-1.5-3.5j, ' g'), '-1.5-3.5j')

        self.assertEqual(format(-1.5-3.5e-20j, 'g'), '-1.5-3.5e-20j')
        self.assertEqual(format(-1.5-3.5j, 'f'), '-1.500000-3.500000j')
        self.assertEqual(format(-1.5-3.5j, 'F'), '-1.500000-3.500000j')
        self.assertEqual(format(-1.5-3.5j, 'e'), '-1.500000e+00-3.500000e+00j')
        self.assertEqual(format(-1.5-3.5j, '.2e'), '-1.50e+00-3.50e+00j')
        self.assertEqual(format(-1.5-3.5j, '.2E'), '-1.50E+00-3.50E+00j')
        self.assertEqual(format(-1.5e10-3.5e5j, '.2G'), '-1.5E+10-3.5E+05j')

        self.assertEqual(format(1.5+3j, '<20g'),  '1.5+3j              ')
        self.assertEqual(format(1.5+3j, '*<20g'), '1.5+3j**************')
        self.assertEqual(format(1.5+3j, '>20g'),  '              1.5+3j')
        self.assertEqual(format(1.5+3j, '^20g'),  '       1.5+3j       ')
        self.assertEqual(format(1.5+3j, '<20'),   '(1.5+3j)            ')
        self.assertEqual(format(1.5+3j, '>20'),   '            (1.5+3j)')
        self.assertEqual(format(1.5+3j, '^20'),   '      (1.5+3j)      ')
        self.assertEqual(format(1.123-3.123j, '^20.2'), '     (1.1-3.1j)     ')

        self.assertEqual(format(1.5+3j, '20.2f'), '          1.50+3.00j')
        self.assertEqual(format(1.5+3j, '>20.2f'), '          1.50+3.00j')
        self.assertEqual(format(1.5+3j, '<20.2f'), '1.50+3.00j          ')
        self.assertEqual(format(1.5e20+3j, '<20.2f'), '150000000000000000000.00+3.00j')
        self.assertEqual(format(1.5e20+3j, '>40.2f'), '          150000000000000000000.00+3.00j')
        self.assertEqual(format(1.5e20+3j, '^40,.2f'), '  150,000,000,000,000,000,000.00+3.00j  ')
        self.assertEqual(format(1.5e21+3j, '^40,.2f'), ' 1,500,000,000,000,000,000,000.00+3.00j ')
        self.assertEqual(format(1.5e21+3000j, ',.2f'), '1,500,000,000,000,000,000,000.00+3,000.00j')

        # alternate is invalid
        self.assertRaises(ValueError, (1.5+0.5j).__format__, '#f')

        # zero padding is invalid
        self.assertRaises(ValueError, (1.5+0.5j).__format__, '010f')

        # '=' alignment is invalid
        self.assertRaises(ValueError, (1.5+3j).__format__, '=20')

        # integer presentation types are an error
        for t in 'bcdoxX':
            self.assertRaises(ValueError, (1.5+0.5j).__format__, t)

        # make sure everything works in ''.format()
        self.assertEqual('*{0:.3f}*'.format(3.14159+2.71828j), '*3.142+2.718j*')

        # issue 3382: 'f' and 'F' with inf's and nan's
        self.assertEqual('{0:f}'.format(INF+0j), 'inf+0.000000j')
        self.assertEqual('{0:F}'.format(INF+0j), 'INF+0.000000j')
        self.assertEqual('{0:f}'.format(-INF+0j), '-inf+0.000000j')
        self.assertEqual('{0:F}'.format(-INF+0j), '-INF+0.000000j')
        self.assertEqual('{0:f}'.format(complex(INF, INF)), 'inf+infj')
        self.assertEqual('{0:F}'.format(complex(INF, INF)), 'INF+INFj')
        self.assertEqual('{0:f}'.format(complex(INF, -INF)), 'inf-infj')
        self.assertEqual('{0:F}'.format(complex(INF, -INF)), 'INF-INFj')
        self.assertEqual('{0:f}'.format(complex(-INF, INF)), '-inf+infj')
        self.assertEqual('{0:F}'.format(complex(-INF, INF)), '-INF+INFj')
        self.assertEqual('{0:f}'.format(complex(-INF, -INF)), '-inf-infj')
        self.assertEqual('{0:F}'.format(complex(-INF, -INF)), '-INF-INFj')

        self.assertEqual('{0:f}'.format(complex(NAN, 0)), 'nan+0.000000j')
        self.assertEqual('{0:F}'.format(complex(NAN, 0)), 'NAN+0.000000j')
        self.assertEqual('{0:f}'.format(complex(NAN, NAN)), 'nan+nanj')
        self.assertEqual('{0:F}'.format(complex(NAN, NAN)), 'NAN+NANj')

def test_main():
    with test_support.check_warnings(("complex divmod.., // and % are "
                                      "deprecated", DeprecationWarning)):
        test_support.run_unittest(ComplexTest)

if __name__ == "__main__":
    test_main()
