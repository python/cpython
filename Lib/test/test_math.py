# Python test set -- math module
# XXXX Should not do tests around zero only

from test.test_support import run_unittest, verbose
import unittest
import math
import os
import sys

eps = 1E-05
NAN = float('nan')
INF = float('inf')
NINF = float('-inf')

# locate file with test values
if __name__ == '__main__':
    file = sys.argv[0]
else:
    file = __file__
test_dir = os.path.dirname(file) or os.curdir
test_file = os.path.join(test_dir, 'cmath_testcases.txt')

def parse_testfile(fname):
    """Parse a file with test values

    Empty lines or lines starting with -- are ignored
    yields id, fn, arg_real, arg_imag, exp_real, exp_imag
    """
    with open(fname) as fp:
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
                   flags
                  )

class MathTests(unittest.TestCase):

    def ftest(self, name, value, expected):
        if abs(value-expected) > eps:
            # Use %r instead of %f so the error message
            # displays full precision. Otherwise discrepancies
            # in the last few bits will lead to very confusing
            # error messages
            self.fail('%s returned %r, expected %r' %
                      (name, value, expected))

    def testConstants(self):
        self.ftest('pi', math.pi, 3.1415926)
        self.ftest('e', math.e, 2.7182818)

    def testAcos(self):
        self.assertRaises(TypeError, math.acos)
        self.ftest('acos(-1)', math.acos(-1), math.pi)
        self.ftest('acos(0)', math.acos(0), math.pi/2)
        self.ftest('acos(1)', math.acos(1), 0)
        self.assertRaises(ValueError, math.acos, INF)
        self.assertRaises(ValueError, math.acos, NINF)
        self.assert_(math.isnan(math.acos(NAN)))

    def testAcosh(self):
        self.assertRaises(TypeError, math.acosh)
        self.ftest('acosh(1)', math.acosh(1), 0)
        self.ftest('acosh(2)', math.acosh(2), 1.3169578969248168)
        self.assertRaises(ValueError, math.acosh, 0)
        self.assertRaises(ValueError, math.acosh, -1)
        self.assertEquals(math.acosh(INF), INF)
        self.assertRaises(ValueError, math.acosh, NINF)
        self.assert_(math.isnan(math.acosh(NAN)))

    def testAsin(self):
        self.assertRaises(TypeError, math.asin)
        self.ftest('asin(-1)', math.asin(-1), -math.pi/2)
        self.ftest('asin(0)', math.asin(0), 0)
        self.ftest('asin(1)', math.asin(1), math.pi/2)
        self.assertRaises(ValueError, math.asin, INF)
        self.assertRaises(ValueError, math.asin, NINF)
        self.assert_(math.isnan(math.asin(NAN)))

    def testAsinh(self):
        self.assertRaises(TypeError, math.asinh)
        self.ftest('asinh(0)', math.asinh(0), 0)
        self.ftest('asinh(1)', math.asinh(1), 0.88137358701954305)
        self.ftest('asinh(-1)', math.asinh(-1), -0.88137358701954305)
        self.assertEquals(math.asinh(INF), INF)
        self.assertEquals(math.asinh(NINF), NINF)
        self.assert_(math.isnan(math.asinh(NAN)))

    def testAtan(self):
        self.assertRaises(TypeError, math.atan)
        self.ftest('atan(-1)', math.atan(-1), -math.pi/4)
        self.ftest('atan(0)', math.atan(0), 0)
        self.ftest('atan(1)', math.atan(1), math.pi/4)
        self.ftest('atan(inf)', math.atan(INF), math.pi/2)
        self.ftest('atan(-inf)', math.atan(NINF), -math.pi/2)
        self.assert_(math.isnan(math.atan(NAN)))

    def testAtanh(self):
        self.assertRaises(TypeError, math.atan)
        self.ftest('atanh(0)', math.atanh(0), 0)
        self.ftest('atanh(0.5)', math.atanh(0.5), 0.54930614433405489)
        self.ftest('atanh(-0.5)', math.atanh(-0.5), -0.54930614433405489)
        self.assertRaises(ValueError, math.atanh, 1)
        self.assertRaises(ValueError, math.atanh, -1)
        self.assertRaises(ValueError, math.atanh, INF)
        self.assertRaises(ValueError, math.atanh, NINF)
        self.assert_(math.isnan(math.atanh(NAN)))

    def testAtan2(self):
        self.assertRaises(TypeError, math.atan2)
        self.ftest('atan2(-1, 0)', math.atan2(-1, 0), -math.pi/2)
        self.ftest('atan2(-1, 1)', math.atan2(-1, 1), -math.pi/4)
        self.ftest('atan2(0, 1)', math.atan2(0, 1), 0)
        self.ftest('atan2(1, 1)', math.atan2(1, 1), math.pi/4)
        self.ftest('atan2(1, 0)', math.atan2(1, 0), math.pi/2)

    def testCeil(self):
        self.assertRaises(TypeError, math.ceil)
        # These types will be int in py3k.
        self.assertEquals(float, type(math.ceil(1)))
        self.assertEquals(float, type(math.ceil(1L)))
        self.assertEquals(float, type(math.ceil(1.0)))
        self.ftest('ceil(0.5)', math.ceil(0.5), 1)
        self.ftest('ceil(1.0)', math.ceil(1.0), 1)
        self.ftest('ceil(1.5)', math.ceil(1.5), 2)
        self.ftest('ceil(-0.5)', math.ceil(-0.5), 0)
        self.ftest('ceil(-1.0)', math.ceil(-1.0), -1)
        self.ftest('ceil(-1.5)', math.ceil(-1.5), -1)
        self.assertEquals(math.ceil(INF), INF)
        self.assertEquals(math.ceil(NINF), NINF)
        self.assert_(math.isnan(math.ceil(NAN)))

        class TestCeil(object):
            def __float__(self):
                return 41.3
        class TestNoCeil(object):
            pass
        self.ftest('ceil(TestCeil())', math.ceil(TestCeil()), 42)
        self.assertRaises(TypeError, math.ceil, TestNoCeil())

        t = TestNoCeil()
        t.__ceil__ = lambda *args: args
        self.assertRaises(TypeError, math.ceil, t)
        self.assertRaises(TypeError, math.ceil, t, 0)

    if float.__getformat__("double").startswith("IEEE"):
        def testCopysign(self):
            self.assertRaises(TypeError, math.copysign)
            # copysign should let us distinguish signs of zeros
            self.assertEquals(copysign(1., 0.), 1.)
            self.assertEquals(copysign(1., -0.), -1.)
            self.assertEquals(copysign(INF, 0.), INF)
            self.assertEquals(copysign(INF, -0.), NINF)
            self.assertEquals(copysign(NINF, 0.), INF)
            self.assertEquals(copysign(NINF, -0.), NINF)
            # and of infinities
            self.assertEquals(copysign(1., INF), 1.)
            self.assertEquals(copysign(1., NINF), -1.)
            self.assertEquals(copysign(INF, INF), INF)
            self.assertEquals(copysign(INF, NINF), NINF)
            self.assertEquals(copysign(NINF, INF), INF)
            self.assertEquals(copysign(NINF, NINF), NINF)
            self.assert_(math.isnan(copysign(NAN, 1.)))
            self.assert_(math.isnan(copysign(NAN, INF)))
            self.assert_(math.isnan(copysign(NAN, NINF)))
            self.assert_(math.isnan(copysign(NAN, NAN)))
            # copysign(INF, NAN) may be INF or it may be NINF, since
            # we don't know whether the sign bit of NAN is set on any
            # given platform.
            self.assert_(math.isinf(copysign(INF, NAN)))
            # similarly, copysign(2., NAN) could be 2. or -2.
            self.assertEquals(abs(copysign(2., NAN)), 2.)

    def testCos(self):
        self.assertRaises(TypeError, math.cos)
        self.ftest('cos(-pi/2)', math.cos(-math.pi/2), 0)
        self.ftest('cos(0)', math.cos(0), 1)
        self.ftest('cos(pi/2)', math.cos(math.pi/2), 0)
        self.ftest('cos(pi)', math.cos(math.pi), -1)
        try:
            self.assert_(math.isnan(math.cos(INF)))
            self.assert_(math.isnan(math.cos(NINF)))
        except ValueError:
            self.assertRaises(ValueError, math.cos, INF)
            self.assertRaises(ValueError, math.cos, NINF)
        self.assert_(math.isnan(math.cos(NAN)))

    def testCosh(self):
        self.assertRaises(TypeError, math.cosh)
        self.ftest('cosh(0)', math.cosh(0), 1)
        self.ftest('cosh(2)-2*cosh(1)**2', math.cosh(2)-2*math.cosh(1)**2, -1) # Thanks to Lambert
        self.assertEquals(math.cosh(INF), INF)
        self.assertEquals(math.cosh(NINF), INF)
        self.assert_(math.isnan(math.cosh(NAN)))

    def testDegrees(self):
        self.assertRaises(TypeError, math.degrees)
        self.ftest('degrees(pi)', math.degrees(math.pi), 180.0)
        self.ftest('degrees(pi/2)', math.degrees(math.pi/2), 90.0)
        self.ftest('degrees(-pi/4)', math.degrees(-math.pi/4), -45.0)

    def testExp(self):
        self.assertRaises(TypeError, math.exp)
        self.ftest('exp(-1)', math.exp(-1), 1/math.e)
        self.ftest('exp(0)', math.exp(0), 1)
        self.ftest('exp(1)', math.exp(1), math.e)
        self.assertEquals(math.exp(INF), INF)
        self.assertEquals(math.exp(NINF), 0.)
        self.assert_(math.isnan(math.exp(NAN)))

    def testFabs(self):
        self.assertRaises(TypeError, math.fabs)
        self.ftest('fabs(-1)', math.fabs(-1), 1)
        self.ftest('fabs(0)', math.fabs(0), 0)
        self.ftest('fabs(1)', math.fabs(1), 1)

    def testFloor(self):
        self.assertRaises(TypeError, math.floor)
        # These types will be int in py3k.
        self.assertEquals(float, type(math.floor(1)))
        self.assertEquals(float, type(math.floor(1L)))
        self.assertEquals(float, type(math.floor(1.0)))
        self.ftest('floor(0.5)', math.floor(0.5), 0)
        self.ftest('floor(1.0)', math.floor(1.0), 1)
        self.ftest('floor(1.5)', math.floor(1.5), 1)
        self.ftest('floor(-0.5)', math.floor(-0.5), -1)
        self.ftest('floor(-1.0)', math.floor(-1.0), -1)
        self.ftest('floor(-1.5)', math.floor(-1.5), -2)
        # pow() relies on floor() to check for integers
        # This fails on some platforms - so check it here
        self.ftest('floor(1.23e167)', math.floor(1.23e167), 1.23e167)
        self.ftest('floor(-1.23e167)', math.floor(-1.23e167), -1.23e167)
        self.assertEquals(math.ceil(INF), INF)
        self.assertEquals(math.ceil(NINF), NINF)
        self.assert_(math.isnan(math.floor(NAN)))

        class TestFloor(object):
            def __float__(self):
                return 42.3
        class TestNoFloor(object):
            pass
        self.ftest('floor(TestFloor())', math.floor(TestFloor()), 42)
        self.assertRaises(TypeError, math.floor, TestNoFloor())

        t = TestNoFloor()
        t.__floor__ = lambda *args: args
        self.assertRaises(TypeError, math.floor, t)
        self.assertRaises(TypeError, math.floor, t, 0)

    def testFmod(self):
        self.assertRaises(TypeError, math.fmod)
        self.ftest('fmod(10,1)', math.fmod(10,1), 0)
        self.ftest('fmod(10,0.5)', math.fmod(10,0.5), 0)
        self.ftest('fmod(10,1.5)', math.fmod(10,1.5), 1)
        self.ftest('fmod(-10,1)', math.fmod(-10,1), 0)
        self.ftest('fmod(-10,0.5)', math.fmod(-10,0.5), 0)
        self.ftest('fmod(-10,1.5)', math.fmod(-10,1.5), -1)
        self.assert_(math.isnan(math.fmod(NAN, 1.)))
        self.assert_(math.isnan(math.fmod(1., NAN)))
        self.assert_(math.isnan(math.fmod(NAN, NAN)))
        self.assertRaises(ValueError, math.fmod, 1., 0.)
        self.assertRaises(ValueError, math.fmod, INF, 1.)
        self.assertRaises(ValueError, math.fmod, NINF, 1.)
        self.assertRaises(ValueError, math.fmod, INF, 0.)
        self.assertEquals(math.fmod(3.0, INF), 3.0)
        self.assertEquals(math.fmod(-3.0, INF), -3.0)
        self.assertEquals(math.fmod(3.0, NINF), 3.0)
        self.assertEquals(math.fmod(-3.0, NINF), -3.0)
        self.assertEquals(math.fmod(0.0, 3.0), 0.0)
        self.assertEquals(math.fmod(0.0, NINF), 0.0)

    def testFrexp(self):
        self.assertRaises(TypeError, math.frexp)

        def testfrexp(name, (mant, exp), (emant, eexp)):
            if abs(mant-emant) > eps or exp != eexp:
                self.fail('%s returned %r, expected %r'%\
                          (name, (mant, exp), (emant,eexp)))

        testfrexp('frexp(-1)', math.frexp(-1), (-0.5, 1))
        testfrexp('frexp(0)', math.frexp(0), (0, 0))
        testfrexp('frexp(1)', math.frexp(1), (0.5, 1))
        testfrexp('frexp(2)', math.frexp(2), (0.5, 2))

        self.assertEquals(math.frexp(INF)[0], INF)
        self.assertEquals(math.frexp(NINF)[0], NINF)
        self.assert_(math.isnan(math.frexp(NAN)[0]))

    def testHypot(self):
        self.assertRaises(TypeError, math.hypot)
        self.ftest('hypot(0,0)', math.hypot(0,0), 0)
        self.ftest('hypot(3,4)', math.hypot(3,4), 5)
        self.assertEqual(math.hypot(NAN, INF), INF)
        self.assertEqual(math.hypot(INF, NAN), INF)
        self.assertEqual(math.hypot(NAN, NINF), INF)
        self.assertEqual(math.hypot(NINF, NAN), INF)
        self.assert_(math.isnan(math.hypot(1.0, NAN)))
        self.assert_(math.isnan(math.hypot(NAN, -2.0)))

    def testLdexp(self):
        self.assertRaises(TypeError, math.ldexp)
        self.ftest('ldexp(0,1)', math.ldexp(0,1), 0)
        self.ftest('ldexp(1,1)', math.ldexp(1,1), 2)
        self.ftest('ldexp(1,-1)', math.ldexp(1,-1), 0.5)
        self.ftest('ldexp(-1,1)', math.ldexp(-1,1), -2)
        self.assertRaises(OverflowError, math.ldexp, 1., 1000000)
        self.assertRaises(OverflowError, math.ldexp, -1., 1000000)
        self.assertEquals(math.ldexp(1., -1000000), 0.)
        self.assertEquals(math.ldexp(-1., -1000000), -0.)
        self.assertEquals(math.ldexp(INF, 30), INF)
        self.assertEquals(math.ldexp(NINF, -213), NINF)
        self.assert_(math.isnan(math.ldexp(NAN, 0)))

    def testLog(self):
        self.assertRaises(TypeError, math.log)
        self.ftest('log(1/e)', math.log(1/math.e), -1)
        self.ftest('log(1)', math.log(1), 0)
        self.ftest('log(e)', math.log(math.e), 1)
        self.ftest('log(32,2)', math.log(32,2), 5)
        self.ftest('log(10**40, 10)', math.log(10**40, 10), 40)
        self.ftest('log(10**40, 10**20)', math.log(10**40, 10**20), 2)
        self.assertEquals(math.log(INF), INF)
        self.assertRaises(ValueError, math.log, NINF)
        self.assert_(math.isnan(math.log(NAN)))

    def testLog1p(self):
        self.assertRaises(TypeError, math.log1p)
        self.ftest('log1p(1/e -1)', math.log1p(1/math.e-1), -1)
        self.ftest('log1p(0)', math.log1p(0), 0)
        self.ftest('log1p(e-1)', math.log1p(math.e-1), 1)
        self.ftest('log1p(1)', math.log1p(1), math.log(2))
        self.assertEquals(math.log1p(INF), INF)
        self.assertRaises(ValueError, math.log1p, NINF)
        self.assert_(math.isnan(math.log1p(NAN)))
        n= 2**90
        self.assertAlmostEquals(math.log1p(n), 62.383246250395075)
        self.assertAlmostEquals(math.log1p(n), math.log1p(float(n)))

    def testLog10(self):
        self.assertRaises(TypeError, math.log10)
        self.ftest('log10(0.1)', math.log10(0.1), -1)
        self.ftest('log10(1)', math.log10(1), 0)
        self.ftest('log10(10)', math.log10(10), 1)
        self.assertEquals(math.log(INF), INF)
        self.assertRaises(ValueError, math.log10, NINF)
        self.assert_(math.isnan(math.log10(NAN)))

    def testModf(self):
        self.assertRaises(TypeError, math.modf)

        def testmodf(name, (v1, v2), (e1, e2)):
            if abs(v1-e1) > eps or abs(v2-e2):
                self.fail('%s returned %r, expected %r'%\
                          (name, (v1,v2), (e1,e2)))

        testmodf('modf(1.5)', math.modf(1.5), (0.5, 1.0))
        testmodf('modf(-1.5)', math.modf(-1.5), (-0.5, -1.0))

        self.assertEquals(math.modf(INF), (0.0, INF))
        self.assertEquals(math.modf(NINF), (-0.0, NINF))

        modf_nan = math.modf(NAN)
        self.assert_(math.isnan(modf_nan[0]))
        self.assert_(math.isnan(modf_nan[1]))

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
        self.assert_(math.isnan(math.pow(NAN, 1)))
        self.assert_(math.isnan(math.pow(2, NAN)))
        self.assert_(math.isnan(math.pow(0, NAN)))
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
        self.assert_(math.isnan(math.pow(0., NAN)))

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
        self.assert_(math.isnan(math.pow(INF, NAN)))

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
        self.assert_(math.isnan(math.pow(-0., NAN)))

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
        self.assert_(math.isnan(math.pow(NINF, NAN)))

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
        self.assert_(math.isnan(math.pow(-1., NAN)))

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

        # the following tests have been commented out since they don't
        # really belong here:  the implementation of ** for floats is
        # independent of the implemention of math.pow
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

    def testSin(self):
        self.assertRaises(TypeError, math.sin)
        self.ftest('sin(0)', math.sin(0), 0)
        self.ftest('sin(pi/2)', math.sin(math.pi/2), 1)
        self.ftest('sin(-pi/2)', math.sin(-math.pi/2), -1)
        try:
            self.assert_(math.isnan(math.sin(INF)))
            self.assert_(math.isnan(math.sin(NINF)))
        except ValueError:
            self.assertRaises(ValueError, math.sin, INF)
            self.assertRaises(ValueError, math.sin, NINF)
        self.assert_(math.isnan(math.sin(NAN)))

    def testSinh(self):
        self.assertRaises(TypeError, math.sinh)
        self.ftest('sinh(0)', math.sinh(0), 0)
        self.ftest('sinh(1)**2-cosh(1)**2', math.sinh(1)**2-math.cosh(1)**2, -1)
        self.ftest('sinh(1)+sinh(-1)', math.sinh(1)+math.sinh(-1), 0)
        self.assertEquals(math.sinh(INF), INF)
        self.assertEquals(math.sinh(NINF), NINF)
        self.assert_(math.isnan(math.sinh(NAN)))

    def testSqrt(self):
        self.assertRaises(TypeError, math.sqrt)
        self.ftest('sqrt(0)', math.sqrt(0), 0)
        self.ftest('sqrt(1)', math.sqrt(1), 1)
        self.ftest('sqrt(4)', math.sqrt(4), 2)
        self.assertEquals(math.sqrt(INF), INF)
        self.assertRaises(ValueError, math.sqrt, NINF)
        self.assert_(math.isnan(math.sqrt(NAN)))

    def testTan(self):
        self.assertRaises(TypeError, math.tan)
        self.ftest('tan(0)', math.tan(0), 0)
        self.ftest('tan(pi/4)', math.tan(math.pi/4), 1)
        self.ftest('tan(-pi/4)', math.tan(-math.pi/4), -1)
        try:
            self.assert_(math.isnan(math.tan(INF)))
            self.assert_(math.isnan(math.tan(NINF)))
        except:
            self.assertRaises(ValueError, math.tan, INF)
            self.assertRaises(ValueError, math.tan, NINF)
        self.assert_(math.isnan(math.tan(NAN)))

    def testTanh(self):
        self.assertRaises(TypeError, math.tanh)
        self.ftest('tanh(0)', math.tanh(0), 0)
        self.ftest('tanh(1)+tanh(-1)', math.tanh(1)+math.tanh(-1), 0)
        self.ftest('tanh(inf)', math.tanh(INF), 1)
        self.ftest('tanh(-inf)', math.tanh(NINF), -1)
        self.assert_(math.isnan(math.tanh(NAN)))

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

        class TestTrunc(object):
            def __trunc__(self):
                return 23

        class TestNoTrunc(object):
            pass

        self.assertEqual(math.trunc(TestTrunc()), 23)

        self.assertRaises(TypeError, math.trunc)
        self.assertRaises(TypeError, math.trunc, 1, 2)
        # XXX: This is not ideal, but see the comment in math_trunc().
        self.assertRaises(AttributeError, math.trunc, TestNoTrunc())

        t = TestNoTrunc()
        t.__trunc__ = lambda *args: args
        self.assertEquals((), math.trunc(t))
        self.assertRaises(TypeError, math.trunc, t, 0)

    def testCopysign(self):
        self.assertEqual(math.copysign(1, 42), 1.0)
        self.assertEqual(math.copysign(0., 42), 0.0)
        self.assertEqual(math.copysign(1., -42), -1.0)
        self.assertEqual(math.copysign(3, 0.), 3.0)
        self.assertEqual(math.copysign(4., -0.), -4.0)

    def testIsnan(self):
        self.assert_(math.isnan(float("nan")))
        self.assert_(math.isnan(float("inf")* 0.))
        self.failIf(math.isnan(float("inf")))
        self.failIf(math.isnan(0.))
        self.failIf(math.isnan(1.))

    def testIsinf(self):
        self.assert_(math.isinf(float("inf")))
        self.assert_(math.isinf(float("-inf")))
        self.assert_(math.isinf(1E400))
        self.assert_(math.isinf(-1E400))
        self.failIf(math.isinf(float("nan")))
        self.failIf(math.isinf(0.))
        self.failIf(math.isinf(1.))

    # RED_FLAG 16-Oct-2000 Tim
    # While 2.0 is more consistent about exceptions than previous releases, it
    # still fails this part of the test on some platforms.  For now, we only
    # *run* test_exceptions() in verbose mode, so that this isn't normally
    # tested.

    if verbose:
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

    def test_testfile(self):
        if not float.__getformat__("double").startswith("IEEE"):
            return
        for id, fn, ar, ai, er, ei, flags in parse_testfile(test_file):
            # Skip if either the input or result is complex, or if
            # flags is nonempty
            if ai != 0. or ei != 0. or flags:
                continue
            if fn in ['rect', 'polar']:
                # no real versions of rect, polar
                continue
            func = getattr(math, fn)
            result = func(ar)
            self.ftest("%s:%s(%r)" % (id, fn, ar), result, er)

def test_main():
    from doctest import DocFileSuite
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(MathTests))
    suite.addTest(DocFileSuite("ieee754.txt"))
    run_unittest(suite)

if __name__ == '__main__':
    test_main()
