import array
import math
import sys
import unittest
import warnings

from test.test_capi.test_getargs import (Float, FloatSubclass, FloatSubclass2,
                                         BadIndex2, BadFloat2, Index, BadIndex,
                                         BadFloat)
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')

NULL = None


class CAPIFloatTest(unittest.TestCase):
    def assertEqualWithSign(self, actual, expected):
        self.assertEqual(actual, expected)
        self.assertEqual(math.copysign(1, actual), math.copysign(1, expected))

    def test_check(self):
        # Test PyFloat_Check()
        check = _testcapi.float_check

        self.assertTrue(check(3.14))
        self.assertTrue(check(FloatSubclass(3.14)))
        self.assertFalse(check(Float()))
        self.assertFalse(check(3))
        self.assertFalse(check([]))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_checkexact(self):
        # Test PyFloat_CheckExact()
        checkexact = _testcapi.float_checkexact

        self.assertTrue(checkexact(3.14))
        self.assertFalse(checkexact(FloatSubclass(3.14)))
        self.assertFalse(checkexact(Float()))
        self.assertFalse(checkexact(3))
        self.assertFalse(checkexact([]))
        self.assertFalse(checkexact(object()))

        # CRASHES checkexact(NULL)

    def test_fromstring(self):
        # Test PyFloat_FromString()
        fromstring = _testcapi.float_fromstring

        self.assertEqual(fromstring("3.14"), 3.14)
        self.assertEqual(fromstring(b"3.14"), 3.14)
        self.assertRaises(ValueError, fromstring, "3.14\0")
        self.assertRaises(ValueError, fromstring, b"3.14\0")
        self.assertEqual(fromstring(bytearray(b"3.14")), 3.14)
        self.assertEqual(fromstring(array.array('b', b'3.14')), 3.14)
        self.assertRaises(TypeError, fromstring, 3.14)

        # CRASHES fromstring(NULL)

    def test_asdouble(self):
        # Test PyFloat_AsDouble()
        asdouble = _testcapi.float_asdouble

        class BadFloat3:
            def __float__(self):
                raise RuntimeError

        self.assertEqual(asdouble(3.14), 3.14)
        self.assertEqual(asdouble(-1), -1.0)
        self.assertEqual(asdouble(42), 42.0)
        self.assertEqual(asdouble(2**1000), float(2**1000))
        self.assertEqual(asdouble(FloatSubclass(3.14)), 3.14)
        self.assertEqual(asdouble(FloatSubclass2(3.14)), 3.14)
        self.assertEqual(asdouble(Index()), 99.)

        self.assertRaises(TypeError, asdouble, BadIndex())
        self.assertRaises(TypeError, asdouble, BadFloat())
        self.assertRaises(RuntimeError, asdouble, BadFloat3())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(asdouble(BadIndex2()), 1.)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(asdouble(BadFloat2()), 4.25)
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, asdouble, BadFloat2())
        self.assertRaises(TypeError, asdouble, object())
        self.assertRaises(TypeError, asdouble, NULL)

    def test_getinfo(self):
        # Test PyFloat_GetInfo()
        getinfo = _testcapi.float_getinfo

        self.assertEqual(getinfo(), sys.float_info)

    def test_getmax(self):
        # Test PyFloat_GetMax()
        getmax = _testcapi.float_getmax

        self.assertEqual(getmax(), sys.float_info.max)

    def test_getmin(self):
        # Test PyFloat_GetMax()
        getmin = _testcapi.float_getmin

        self.assertEqual(getmin(), sys.float_info.min)
