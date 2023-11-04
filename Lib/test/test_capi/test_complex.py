import unittest
import warnings

from test.test_capi.test_getargs import (BadComplex, BadComplex2, Complex,
                                         FloatSubclass, Float, BadFloat,
                                         BadFloat2, ComplexSubclass)
from test.support import import_helper


_testcapi = import_helper.import_module('_testcapi')

NULL = None

class BadComplex3:
    def __complex__(self):
        raise RuntimeError


class CAPIComplexTest(unittest.TestCase):
    def test_check(self):
        # Test PyComplex_Check()
        check = _testcapi.complex_check

        self.assertTrue(check(1+2j))
        self.assertTrue(check(ComplexSubclass(1+2j)))
        self.assertFalse(check(Complex()))
        self.assertFalse(check(3))
        self.assertFalse(check(3.0))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_checkexact(self):
        # PyComplex_CheckExact()
        checkexact = _testcapi.complex_checkexact

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
        fromdoubles = _testcapi.complex_fromdoubles

        self.assertEqual(fromdoubles(1.0, 2.0), 1.0+2.0j)

    def test_realasdouble(self):
        # Test PyComplex_RealAsDouble()
        realasdouble = _testcapi.complex_realasdouble

        # Test subclasses of complex/float
        self.assertEqual(realasdouble(1+2j), 1.0)
        self.assertEqual(realasdouble(-1+0j), -1.0)
        self.assertEqual(realasdouble(-1.0), -1.0)

        self.assertEqual(realasdouble(ComplexSubclass(1+2j)), 1.0)
        self.assertEqual(realasdouble(3.14), 3.14)
        self.assertEqual(realasdouble(FloatSubclass(3.14)), 3.14)

        # Test types with __complex__ dunder method
        # Function doesn't support classes with __complex__ dunder, see #109598
        self.assertRaises(TypeError, realasdouble, Complex())
        #self.assertEqual(realasdouble(Complex()), 4.25)
        #self.assertRaises(TypeError, realasdouble, BadComplex())
        #with self.assertWarns(DeprecationWarning):
        #    self.assertEqual(realasdouble(BadComplex2()), 4.25)
        #with warnings.catch_warnings():
        #    warnings.simplefilter("error", DeprecationWarning)
        #    self.assertRaises(DeprecationWarning, realasdouble, BadComplex2())
        #self.assertRaises(RuntimeError, realasdouble, BadComplex3())

        # Test types with __float__ dunder method
        self.assertEqual(realasdouble(Float()), 4.25)
        self.assertRaises(TypeError, realasdouble, BadFloat())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(realasdouble(BadFloat2()), 4.25)

        self.assertEqual(realasdouble(42), 42.)
        self.assertRaises(TypeError, realasdouble, object())

        # CRASHES realasdouble(NULL)

    def test_imagasdouble(self):
        # Test PyComplex_ImagAsDouble()
        imagasdouble = _testcapi.complex_imagasdouble

        # Test subclasses of complex/float
        self.assertEqual(imagasdouble(1+2j), 2.0)
        self.assertEqual(imagasdouble(1-1j), -1.0)
        self.assertEqual(imagasdouble(ComplexSubclass(1+2j)), 2.0)
        self.assertEqual(imagasdouble(3.14), 0.0)
        self.assertEqual(imagasdouble(FloatSubclass(3.14)), 0.0)

        # Test types with __complex__ dunder method
        # Function doesn't support classes with __complex__ dunder, see #109598
        #self.assertEqual(imagasdouble(Complex()), 0.5)
        #self.assertRaises(TypeError, imagasdouble, BadComplex())
        #with self.assertWarns(DeprecationWarning):
        #    self.assertEqual(imagasdouble(BadComplex2()), 0.5)
        #with warnings.catch_warnings():
        #    warnings.simplefilter("error", DeprecationWarning)
        #    self.assertRaises(DeprecationWarning, imagasdouble, BadComplex2())
        #self.assertRaises(RuntimeError, imagasdouble, BadComplex3())

        # Test types with __float__ dunder method
        # Function doesn't support classes with __float__ dunder, see #109598
        #self.assertEqual(imagasdouble(Float()), 0.0)
        #self.assertRaises(TypeError, imagasdouble, BadFloat())
        #with self.assertWarns(DeprecationWarning):
        #    self.assertEqual(imagasdouble(BadFloat2()), 0.0)

        # Function returns 0.0 anyway, see #109598
        self.assertEqual(imagasdouble(object()), 0.0)
        #self.assertRaises(TypeError, imagasdouble, object())

        # CRASHES imagasdouble(NULL)

    def test_asccomplex(self):
        # Test PyComplex_AsCComplex()
        asccomplex = _testcapi.complex_asccomplex

        # Test subclasses of complex/float
        self.assertEqual(asccomplex(1+2j), 1.0+2.0j)
        self.assertEqual(asccomplex(ComplexSubclass(1+2j)), 1.0+2.0j)
        self.assertEqual(asccomplex(3.14), 3.14+0.0j)
        self.assertEqual(asccomplex(FloatSubclass(3.14)), 3.14+0.0j)

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
        self.assertEqual(asccomplex(42), 42+0j)

        # CRASHES asccomplex(NULL)


if __name__ == "__main__":
    unittest.main()
