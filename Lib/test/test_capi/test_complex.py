from math import isnan
import errno
import unittest
import warnings

from test.test_capi.test_getargs import (BadComplex, BadComplex2, Complex,
                                         FloatSubclass, Float, BadFloat,
                                         BadFloat2, ComplexSubclass)
from test.support import import_helper


_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')

NULL = None
INF = float("inf")
NAN = float("nan")
DBL_MAX = _testcapi.DBL_MAX


class BadComplex3:
    def __complex__(self):
        raise RuntimeError


class CAPIComplexTest(unittest.TestCase):
    def test_check(self):
        # Test PyComplex_Check()
        check = _testlimitedcapi.complex_check

        self.assertTrue(check(1+2j))
        self.assertTrue(check(ComplexSubclass(1+2j)))
        self.assertFalse(check(Complex()))
        self.assertFalse(check(3))
        self.assertFalse(check(3.0))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_checkexact(self):
        # PyComplex_CheckExact()
        checkexact = _testlimitedcapi.complex_checkexact

        self.assertTrue(checkexact(1+2j))
        self.assertFalse(checkexact(ComplexSubclass(1+2j)))
        self.assertFalse(checkexact(Complex()))
        self.assertFalse(checkexact(3))
        self.assertFalse(checkexact(3.0))
        self.assertFalse(checkexact(object()))

        # CRASHES checkexact(NULL)

    def test_fromccomplex(self):
        # Test PyComplex_FromCComplex()
        fromccomplex = _testcapi.complex_fromccomplex

        self.assertEqual(fromccomplex(1+2j), 1.0+2.0j)

    def test_fromdoubles(self):
        # Test PyComplex_FromDoubles()
        fromdoubles = _testlimitedcapi.complex_fromdoubles

        self.assertEqual(fromdoubles(1.0, 2.0), 1.0+2.0j)

    def test_realasdouble(self):
        # Test PyComplex_RealAsDouble()
        realasdouble = _testlimitedcapi.complex_realasdouble

        self.assertEqual(realasdouble(1+2j), 1.0)
        self.assertEqual(realasdouble(-1+0j), -1.0)
        self.assertEqual(realasdouble(4.25), 4.25)
        self.assertEqual(realasdouble(-1.0), -1.0)
        self.assertEqual(realasdouble(42), 42.)
        self.assertEqual(realasdouble(-1), -1.0)

        # Test subclasses of complex/float
        self.assertEqual(realasdouble(ComplexSubclass(1+2j)), 1.0)
        self.assertEqual(realasdouble(FloatSubclass(4.25)), 4.25)

        # Test types with __complex__ dunder method
        self.assertEqual(realasdouble(Complex()), 4.25)
        self.assertRaises(TypeError, realasdouble, BadComplex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(realasdouble(BadComplex2()), 4.25)
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, realasdouble, BadComplex2())
        self.assertRaises(RuntimeError, realasdouble, BadComplex3())

        # Test types with __float__ dunder method
        self.assertEqual(realasdouble(Float()), 4.25)
        self.assertRaises(TypeError, realasdouble, BadFloat())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(realasdouble(BadFloat2()), 4.25)

        self.assertRaises(TypeError, realasdouble, object())

        # CRASHES realasdouble(NULL)

    def test_imagasdouble(self):
        # Test PyComplex_ImagAsDouble()
        imagasdouble = _testlimitedcapi.complex_imagasdouble

        self.assertEqual(imagasdouble(1+2j), 2.0)
        self.assertEqual(imagasdouble(1-1j), -1.0)
        self.assertEqual(imagasdouble(4.25), 0.0)
        self.assertEqual(imagasdouble(42), 0.0)

        # Test subclasses of complex/float
        self.assertEqual(imagasdouble(ComplexSubclass(1+2j)), 2.0)
        self.assertEqual(imagasdouble(FloatSubclass(4.25)), 0.0)

        # Test types with __complex__ dunder method
        self.assertEqual(imagasdouble(Complex()), 0.5)
        self.assertRaises(TypeError, imagasdouble, BadComplex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(imagasdouble(BadComplex2()), 0.5)
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, imagasdouble, BadComplex2())
        self.assertRaises(RuntimeError, imagasdouble, BadComplex3())

        # Test types with __float__ dunder method
        self.assertEqual(imagasdouble(Float()), 0.0)
        self.assertRaises(TypeError, imagasdouble, BadFloat())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(imagasdouble(BadFloat2()), 0.0)

        self.assertRaises(TypeError, imagasdouble, object())

        # CRASHES imagasdouble(NULL)

    def test_asccomplex(self):
        # Test PyComplex_AsCComplex()
        asccomplex = _testcapi.complex_asccomplex

        self.assertEqual(asccomplex(1+2j), 1.0+2.0j)
        self.assertEqual(asccomplex(-1+2j), -1.0+2.0j)
        self.assertEqual(asccomplex(4.25), 4.25+0.0j)
        self.assertEqual(asccomplex(-1.0), -1.0+0.0j)
        self.assertEqual(asccomplex(42), 42+0j)
        self.assertEqual(asccomplex(-1), -1.0+0.0j)

        # Test subclasses of complex/float
        self.assertEqual(asccomplex(ComplexSubclass(1+2j)), 1.0+2.0j)
        self.assertEqual(asccomplex(FloatSubclass(4.25)), 4.25+0.0j)

        # Test types with __complex__ dunder method
        self.assertEqual(asccomplex(Complex()), 4.25+0.5j)
        self.assertRaises(TypeError, asccomplex, BadComplex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(asccomplex(BadComplex2()), 4.25+0.5j)
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, asccomplex, BadComplex2())
        self.assertRaises(RuntimeError, asccomplex, BadComplex3())

        # Test types with __float__ dunder method
        self.assertEqual(asccomplex(Float()), 4.25+0.0j)
        self.assertRaises(TypeError, asccomplex, BadFloat())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(asccomplex(BadFloat2()), 4.25+0.0j)

        self.assertRaises(TypeError, asccomplex, object())

        # CRASHES asccomplex(NULL)

    def test_py_c_sum(self):
        # Test _Py_c_sum()
        _py_c_sum = _testcapi._py_c_sum

        self.assertEqual(_py_c_sum(1, 1j), (1+1j, 0))

    def test_py_c_diff(self):
        # Test _Py_c_diff()
        _py_c_diff = _testcapi._py_c_diff

        self.assertEqual(_py_c_diff(1, 1j), (1-1j, 0))

    def test_py_c_neg(self):
        # Test _Py_c_neg()
        _py_c_neg = _testcapi._py_c_neg

        self.assertEqual(_py_c_neg(1+1j), -1-1j)

    def test_py_c_prod(self):
        # Test _Py_c_prod()
        _py_c_prod = _testcapi._py_c_prod

        self.assertEqual(_py_c_prod(2, 1j), (2j, 0))

    def test_py_c_quot(self):
        # Test _Py_c_quot()
        _py_c_quot = _testcapi._py_c_quot

        self.assertEqual(_py_c_quot(1, 1j), (-1j, 0))
        self.assertEqual(_py_c_quot(1, -1j), (1j, 0))
        self.assertEqual(_py_c_quot(1j, 2), (0.5j, 0))
        self.assertEqual(_py_c_quot(1j, -2), (-0.5j, 0))
        self.assertEqual(_py_c_quot(1, 2j), (-0.5j, 0))

        z, e = _py_c_quot(NAN, 1j)
        self.assertTrue(isnan(z.real))
        self.assertTrue(isnan(z.imag))
        self.assertEqual(e, 0)

        z, e = _py_c_quot(1j, NAN)
        self.assertTrue(isnan(z.real))
        self.assertTrue(isnan(z.imag))
        self.assertEqual(e, 0)

        self.assertEqual(_py_c_quot(1, 0j)[1], errno.EDOM)

    def test_py_c_pow(self):
        # Test _Py_c_pow()
        _py_c_pow = _testcapi._py_c_pow

        self.assertEqual(_py_c_pow(1j, 0j), (1+0j, 0))
        self.assertEqual(_py_c_pow(1, 1j), (1+0j, 0))
        self.assertEqual(_py_c_pow(0j, 1), (0j, 0))
        self.assertAlmostEqual(_py_c_pow(1j, 2)[0], -1.0+0j)

        r, e = _py_c_pow(1+1j, -1)
        self.assertAlmostEqual(r, 0.5-0.5j)
        self.assertEqual(e, 0)

        self.assertEqual(_py_c_pow(0j, -1)[1], errno.EDOM)
        self.assertEqual(_py_c_pow(0j, 1j)[1], errno.EDOM)
        self.assertEqual(_py_c_pow(*[DBL_MAX+1j]*2)[0], complex(*[INF]*2))


    def test_py_c_abs(self):
        # Test _Py_c_abs()
        _py_c_abs = _testcapi._py_c_abs

        self.assertEqual(_py_c_abs(-1), (1.0, 0))
        self.assertEqual(_py_c_abs(1j), (1.0, 0))

        self.assertEqual(_py_c_abs(complex('+inf+1j')), (INF, 0))
        self.assertEqual(_py_c_abs(complex('-inf+1j')), (INF, 0))
        self.assertEqual(_py_c_abs(complex('1.25+infj')), (INF, 0))
        self.assertEqual(_py_c_abs(complex('1.25-infj')), (INF, 0))

        self.assertTrue(isnan(_py_c_abs(complex('1.25+nanj'))[0]))
        self.assertTrue(isnan(_py_c_abs(complex('nan-1j'))[0]))

        self.assertEqual(_py_c_abs(complex(*[DBL_MAX]*2))[1], errno.ERANGE)


if __name__ == "__main__":
    unittest.main()
