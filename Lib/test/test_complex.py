import unittest, os
from test import test_support

import warnings
warnings.filterwarnings(
    "ignore",
    category=DeprecationWarning,
    message=".*complex divmod.*are deprecated"
)

from random import random

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
        self.assert_(abs((x-y)/y) < eps)

    def assertClose(self, x, y, eps=1e-9):
        """Return true iff complexes x and y "are close\""""
        self.assertCloseAbs(x.real, y.real, eps)
        self.assertCloseAbs(x.imag, y.imag, eps)

    def assertIs(self, a, b):
        self.assert_(a is b)

    def check_div(self, x, y):
        """Compute complex z=x*y, and check that z/x==y and z/y==x."""
        z = x * y
        if x != 0:
            q = z / x
            self.assertClose(q, y)
            q = z.__truediv__(x)
            self.assertClose(q, y)
        if y != 0:
            q = z / y
            self.assertClose(q, x)
            q = z.__truediv__(y)
            self.assertClose(q, x)

    def test_truediv(self):
        simple_real = [float(i) for i in range(-5, 6)]
        simple_complex = [complex(x, y) for x in simple_real for y in simple_real]
        for x in simple_complex:
            for y in simple_complex:
                self.check_div(x, y)

        # A naive complex division algorithm (such as in 2.0) is very prone to
        # nonsense errors for these (overflows and underflows).
        self.check_div(complex(1e200, 1e200), 1+0j)
        self.check_div(complex(1e-200, 1e-200), 1+0j)

        # Just for fun.
        for i in range(100):
            self.check_div(complex(random(), random()),
                           complex(random(), random()))

        self.assertRaises(ZeroDivisionError, complex.__truediv__, 1+1j, 0+0j)
        # FIXME: The following currently crashes on Alpha
        # self.assertRaises(OverflowError, pow, 1e200+1j, 1e200+1j)

    def test_truediv(self):
        self.assertAlmostEqual(complex.__truediv__(2+0j, 1+1j), 1-1j)
        self.assertRaises(ZeroDivisionError, complex.__truediv__, 1+1j, 0+0j)

    def test_floordiv(self):
        self.assertRaises(TypeError, complex.__floordiv__, 3+0j, 1.5+0j)
        self.assertRaises(TypeError, complex.__floordiv__, 3+0j, 0+0j)

    def test_richcompare(self):
        self.assertRaises(OverflowError, complex.__eq__, 1+1j, 1<<10000)
        self.assertEqual(complex.__lt__(1+1j, None), NotImplemented)
        self.assertIs(complex.__eq__(1+1j, 1+1j), True)
        self.assertIs(complex.__eq__(1+1j, 2+2j), False)
        self.assertIs(complex.__ne__(1+1j, 1+1j), False)
        self.assertIs(complex.__ne__(1+1j, 2+2j), True)
        self.assertRaises(TypeError, complex.__lt__, 1+1j, 2+2j)
        self.assertRaises(TypeError, complex.__le__, 1+1j, 2+2j)
        self.assertRaises(TypeError, complex.__gt__, 1+1j, 2+2j)
        self.assertRaises(TypeError, complex.__ge__, 1+1j, 2+2j)

    def test_mod(self):
        # % is no longer supported on complex numbers
        self.assertRaises(TypeError, (1+1j).__mod__, 0+0j)
        self.assertRaises(TypeError, lambda: (3.33+4.43j) % 0)

    def test_divmod(self):
        self.assertRaises(TypeError, divmod, 1+1j, 1+0j)
        self.assertRaises(TypeError, divmod, 1+1j, 0+0j)

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
        for i in range(100):
            self.assert_(complex(random() + 1e-6, random() + 1e-6))
        self.assert_(not complex(0.0, 0.0))

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
        self.assertAlmostEqual(complex(10), 10+0j)
        self.assertAlmostEqual(complex(10+0j), 10+0j)
        self.assertAlmostEqual(complex(1,10), 1+10j)
        self.assertAlmostEqual(complex(1,10), 1+10j)
        self.assertAlmostEqual(complex(1,10.0), 1+10j)
        self.assertAlmostEqual(complex(1,10), 1+10j)
        self.assertAlmostEqual(complex(1,10), 1+10j)
        self.assertAlmostEqual(complex(1,10.0), 1+10j)
        self.assertAlmostEqual(complex(1.0,10), 1+10j)
        self.assertAlmostEqual(complex(1.0,10), 1+10j)
        self.assertAlmostEqual(complex(1.0,10.0), 1+10j)
        self.assertAlmostEqual(complex(3.14+0j), 3.14+0j)
        self.assertAlmostEqual(complex(3.14), 3.14+0j)
        self.assertAlmostEqual(complex(314), 314.0+0j)
        self.assertAlmostEqual(complex(314), 314.0+0j)
        self.assertAlmostEqual(complex(3.14+0j, 0j), 3.14+0j)
        self.assertAlmostEqual(complex(3.14, 0.0), 3.14+0j)
        self.assertAlmostEqual(complex(314, 0), 314.0+0j)
        self.assertAlmostEqual(complex(314, 0), 314.0+0j)
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

        class complex2(complex): pass
        self.assertAlmostEqual(complex(complex2(1+1j)), 1+1j)
        self.assertAlmostEqual(complex(real=17, imag=23), 17+23j)
        self.assertAlmostEqual(complex(real=17+23j), 17+23j)
        self.assertAlmostEqual(complex(real=17+23j, imag=23), 17+46j)
        self.assertAlmostEqual(complex(real=1+2j, imag=3+4j), -3+5j)

        c = 3.14 + 1j
        self.assert_(complex(c) is c)
        del c

        self.assertRaises(TypeError, complex, "1", "1")
        self.assertRaises(TypeError, complex, 1, "1")

        self.assertEqual(complex("  3.14+J  "), 3.14+1j)

        # SF bug 543840:  complex(string) accepts strings with \0
        # Fixed in 2.3.
        self.assertRaises(ValueError, complex, '1+1j\0j')

        self.assertRaises(TypeError, int, 5+3j)
        self.assertRaises(TypeError, int, 5+3j)
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
        self.assertRaises(ValueError, complex, "1"*500)
        self.assertRaises(ValueError, complex, "x")

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

    def test_hash(self):
        for x in range(-30, 30):
            self.assertEqual(hash(x), hash(complex(x, 0)))
            x /= 3.0    # now check against floating point
            self.assertEqual(hash(x), hash(complex(x, 0.)))

    def test_abs(self):
        nums = [complex(x/3., y/7.) for x in range(-9,9) for y in range(-9,9)]
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

    def test_neg(self):
        self.assertEqual(-(1+6j), -1-6j)

    def test_file(self):
        a = 3.33+4.43j
        b = 5.1+2.3j

        fo = None
        try:
            fo = open(test_support.TESTFN, "wb")
            print(a, b, file=fo)
            fo.close()
            fo = open(test_support.TESTFN, "rb")
            self.assertEqual(fo.read(), ("%s %s\n" % (a, b)).encode("ascii"))
        finally:
            if (fo is not None) and (not fo.closed):
                fo.close()
            try:
                os.remove(test_support.TESTFN)
            except (OSError, IOError):
                pass

def test_main():
    test_support.run_unittest(ComplexTest)

if __name__ == "__main__":
    test_main()
