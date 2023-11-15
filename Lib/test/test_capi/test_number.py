import unittest
import sys
import warnings

from test.support import import_helper
from test.test_capi.test_getargs import (Float, BadFloat, BadFloat2, Index,
                                         BadIndex, BadIndex2, Int, BadInt, BadInt2)

_testcapi = import_helper.import_module('_testcapi')
from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX

NULL = None


class BadAdd:
    def __add__(self, other):
        raise RuntimeError


class BadAdd2(int):
    def __radd__(self, other):
        return NotImplemented


class IndexLike:
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value


class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyNumber_Check()
        check = _testcapi.number_check

        self.assertTrue(check(1))
        self.assertTrue(check(1.0))
        self.assertTrue(check(1+2j))
        self.assertFalse(check([]))
        self.assertFalse(check(object()))
        self.assertFalse(check(NULL))

    def test_add(self):
        # Test PyNumber_Add()
        add = _testcapi.number_add

        self.assertEqual(add(1, 2), 1 + 2)
        self.assertEqual(add(1, 0.75), 1 + 0.75)
        self.assertEqual(add(0.75, 1), 0.75 + 1)
        self.assertEqual(add(1, BadAdd2(2)), 3)
        self.assertEqual(add([1, 2], [3, 4]), [1, 2, 3, 4])

        self.assertRaises(TypeError, add, 1, object())
        self.assertRaises(TypeError, add, object(), 1)

        # CRASHES add(NULL, 42)
        # CRASHES add(42, NULL)

    def test_subtract(self):
        # Test PyNumber_Subtract()
        subtract = _testcapi.number_subtract

        self.assertEqual(subtract(1, 2), 1 - 2)

        self.assertRaises(TypeError, subtract, 1, object())
        self.assertRaises(TypeError, subtract, object(), 1)

        # CRASHES subtract(NULL, 42)
        # CRASHES subtract(42, NULL)

    def test_multiply(self):
        # Test PyNumber_Multiply()
        multiply = _testcapi.number_multiply

        self.assertEqual(multiply(2, 3), 2 * 3)
        self.assertEqual(multiply([1], 2), [1, 1])
        self.assertEqual(multiply(2, [1]), [1, 1])

        self.assertRaises(TypeError, multiply, 1, object())
        self.assertRaises(TypeError, multiply, object(), 1)

        # CRASHES multiply(NULL, 42)
        # CRASHES multiply(42, NULL)

    def test_matrixmultiply(self):
        # Test PyNumber_MatrixMultiply()
        matrixmultiply = _testcapi.number_matrixmultiply

        # CRASHES matrixmultiply(NULL, NULL)

    def test_floordivide(self):
        # Test PyNumber_FloorDivide()
        floordivide = _testcapi.number_floordivide

        self.assertEqual(floordivide(4, 3), 4 // 3)

        # CRASHES floordivide(NULL, 42)
        # CRASHES floordivide(42, NULL)

    def test_truedivide(self):
        # Test PyNumber_TrueDivide()
        truedivide = _testcapi.number_truedivide

        self.assertEqual(truedivide(3, 4), 3 / 4)

        # CRASHES truedivide(NULL, 42)
        # CRASHES truedivide(42, NULL)

    def test_remainder(self):
        # Test PyNumber_Remainder()
        remainder = _testcapi.number_remainder

        self.assertEqual(remainder(4, 3), 4 % 3)

        # CRASHES remainder(NULL, 42)
        # CRASHES remainder(42, NULL)

    def test_divmod(self):
        # Test PyNumber_divmod()
        divmod_ = _testcapi.number_divmod

        self.assertEqual(divmod_(4, 3), divmod(4, 3))

        # CRASHES divmod_(NULL, 42)
        # CRASHES divmod_(42, NULL)

    def test_power(self):
        # Test PyNumber_Power()
        power = _testcapi.number_power

        class IntSubclass(int):
            pass

        class IntSubclass2(int):
            def __new__(cls, value):
                obj = super().__new__(cls)
                obj.value = value
                return obj

            def __rpow__(self, other, mod=None):
                return self.value

        self.assertEqual(power(4, 3, NULL), pow(4, 3))
        self.assertEqual(power(0.5, 2, NULL), pow(0.5, 2))
        self.assertEqual(power(2, -1.0, NULL), pow(2, -1.0))
        self.assertEqual(power(4, 11, 5), pow(4, 11, 5))
        self.assertEqual(power(4, IntSubclass(11), NULL), pow(4, 11))
        self.assertEqual(power(4, IntSubclass2(11), NULL), 11)
        self.assertEqual(power(4, IntSubclass2(NotImplemented), NULL), 1)

        self.assertRaises(TypeError, power, "spam", 42, NULL)
        self.assertRaises(TypeError, power, object(), 42, NULL)
        self.assertRaises(TypeError, power, 42, "spam", NULL)
        self.assertRaises(TypeError, power, 42, object(), NULL)
        self.assertRaises(TypeError, power, 1, 2, "spam")
        self.assertRaises(TypeError, power, 1, 2, object())

        # CRASHES power(NULL, 42)
        # CRASHES power(42, NULL)

    def test_negative(self):
        # Test PyNumber_Negative()
        negative = _testcapi.number_negative

        self.assertEqual(negative(42), -42)

        self.assertRaises(TypeError, negative, BadAdd())
        self.assertRaises(TypeError, negative, object())
        self.assertRaises(SystemError, negative, NULL)

    def test_positive(self):
        # Test PyNumber_Positive()
        positive = _testcapi.number_positive

        self.assertEqual(positive(-1), +(-1))

        self.assertRaises(TypeError, positive, BadAdd())
        self.assertRaises(TypeError, positive, object())
        self.assertRaises(SystemError, positive, NULL)

    def test_absolute(self):
        # Test PyNumber_Absolute()
        absolute = _testcapi.number_absolute

        self.assertEqual(absolute(-1), abs(-1))

        self.assertRaises(TypeError, absolute, BadAdd())
        self.assertRaises(TypeError, absolute, object())
        self.assertRaises(SystemError, absolute, NULL)

    def test_invert(self):
        # Test PyNumber_Invert()
        invert = _testcapi.number_invert

        self.assertEqual(invert(123), ~123)

        self.assertRaises(TypeError, invert, BadAdd())
        self.assertRaises(TypeError, invert, object())
        self.assertRaises(SystemError, invert, NULL)

    def test_lshift(self):
        # Test PyNumber_Lshift()
        lshift = _testcapi.number_lshift

        self.assertEqual(lshift(3, 5), 3 << 5)

        # CRASHES lshift(NULL, 1)
        # CRASHES lshift(1, NULL)

    def test_rshift(self):
        # Test PyNumber_Rshift()
        rshift = _testcapi.number_rshift

        self.assertEqual(rshift(5, 3), 5 >> 3)

        # RHS implements __rrshift__ but returns NotImplemented
        with self.assertRaises(TypeError) as context:
            print >> 42
        self.assertIn('Did you mean "print(<message>, '
                      'file=<output_stream>)"?', str(context.exception))

        # Test stream redirection hint is specific to print
        with self.assertRaises(TypeError) as context:
            max >> sys.stderr
        self.assertNotIn('Did you mean ', str(context.exception))

        with self.assertRaises(TypeError) as context:
            1 >> "spam"

        # CRASHES rshift(NULL, 1)
        # CRASHES rshift(1, NULL)

    def test_and(self):
        # Test PyNumber_And()
        and_ = _testcapi.number_and

        self.assertEqual(and_(0b10, 0b01), 0b10 & 0b01)
        self.assertEqual(and_({1, 2}, {2, 3}), {1, 2} & {2, 3})

        # CRASHES and_(NULL, 1)
        # CRASHES and_(1, NULL)

    def test_xor(self):
        # Test PyNumber_Xor()
        xor = _testcapi.number_xor

        self.assertEqual(xor(0b10, 0b01), 0b10 ^ 0b01)
        self.assertEqual(xor({1, 2}, {2, 3}), {1, 2} ^ {2, 3})

        # CRASHES xor(NULL, 1)
        # CRASHES xor(1, NULL)

    def test_or(self):
        # Test PyNumber_Or()
        or_ = _testcapi.number_or

        self.assertEqual(or_(0b10, 0b01), 0b10 | 0b01)
        self.assertEqual(or_({1, 2}, {2, 3}), {1, 2} | {2, 3})

        # CRASHES or_(NULL, 1)
        # CRASHES or_(1, NULL)

    def test_inplaceadd(self):
        # Test PyNumber_InPlaceAdd()
        inplaceadd = _testcapi.number_inplaceadd

        # CRASHES inplaceadd(NULL)

    def test_inplacesubtract(self):
        # Test PyNumber_InPlaceSubtract()
        inplacesubtract = _testcapi.number_inplacesubtract

        # CRASHES inplacesubtract(NULL)

    def test_inplacemultiply(self):
        # Test PyNumber_InPlaceMultiply()
        inplacemultiply = _testcapi.number_inplacemultiply

        # CRASHES inplacemultiply(NULL)

    def test_inplacematrixmultiply(self):
        # Test PyNumber_InPlaceMatrixMultiply()
        inplacematrixmultiply = _testcapi.number_inplacematrixmultiply

        # CRASHES inplacematrixmultiply(NULL)

    def test_inplacefloordivide(self):
        # Test PyNumber_InPlaceFloorDivide()
        inplacefloordivide = _testcapi.number_inplacefloordivide

        # CRASHES inplacefloordivide(NULL)

    def test_inplacetruedivide(self):
        # Test PyNumber_InPlaceTrueDivide()
        inplacetruedivide = _testcapi.number_inplacetruedivide

        # CRASHES inplacetruedivide(NULL)

    def test_inplaceremainder(self):
        # Test PyNumber_InPlaceRemainder()
        inplaceremainder = _testcapi.number_inplaceremainder

        # CRASHES inplaceremainder(NULL)

    def test_inplacepower(self):
        # Test PyNumber_InPlacePower()
        inplacepower = _testcapi.number_inplacepower

        # CRASHES inplacepower(NULL)

    def test_inplacelshift(self):
        # Test PyNumber_InPlaceLshift()
        inplacelshift = _testcapi.number_inplacelshift

        # CRASHES inplacelshift(NULL)

    def test_inplacershift(self):
        # Test PyNumber_InPlaceRshift()
        inplacershift = _testcapi.number_inplacershift

        # CRASHES inplacershift(NULL)

    def test_inplaceand(self):
        # Test PyNumber_InPlaceAnd()
        inplaceand = _testcapi.number_inplaceand

        # CRASHES inplaceand(NULL)

    def test_inplacexor(self):
        # Test PyNumber_InPlaceXor()
        inplacexor = _testcapi.number_inplacexor

        # CRASHES inplacexor(NULL)

    def test_inplaceor(self):
        # Test PyNumber_InPlaceOr()
        inplaceor = _testcapi.number_inplaceor

        # CRASHES inplaceor(NULL)

    def test_long(self):
        # Test PyNumber_Long()
        long = _testcapi.number_long

        self.assertEqual(long(42), 42)
        self.assertEqual(long(1.25), 1)
        self.assertEqual(long("42"), 42)
        self.assertEqual(long(b"42"), 42)
        self.assertEqual(long(bytearray(b"42")), 42)
        self.assertEqual(long(memoryview(b"42")), 42)
        self.assertEqual(long(IndexLike(99)), 99)
        self.assertEqual(long(Int()), 99)

        self.assertRaises(TypeError, long, BadInt())
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, long, BadInt2())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(long(BadInt2()), 1)

        self.assertRaises(TypeError, long, 1j)
        self.assertRaises(TypeError, long, object())
        self.assertRaises(SystemError, long, NULL)

    def test_float(self):
        # Test PyNumber_Float()
        float_ = _testcapi.number_float

        self.assertEqual(float_(1.25), 1.25)
        self.assertEqual(float_(123), 123.)

        self.assertEqual(float_(Float()), 4.25)
        self.assertEqual(float_(IndexLike(99)), 99.0)
        self.assertEqual(float_(IndexLike(-1)), -1.0)

        self.assertRaises(TypeError, float_, BadFloat())
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, float_, BadFloat2())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(float_(BadFloat2()), 4.25)
        self.assertRaises(TypeError, float_, IndexLike(1.25))
        self.assertRaises(OverflowError, float_, IndexLike(2**2000))
        self.assertRaises(TypeError, float_, 1j)
        self.assertRaises(TypeError, float_, object())
        self.assertRaises(SystemError, float_, NULL)

    def test_index(self):
        # Test PyNumber_Index()
        index = _testcapi.number_index

        self.assertEqual(index(11), 11)

        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, index, BadIndex2())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(index(BadIndex2()), 1)
        self.assertRaises(TypeError, index, BadIndex())
        self.assertRaises(TypeError, index, object())
        self.assertRaises(SystemError, index, NULL)

    def test_tobase(self):
        # Test PyNumber_ToBase()
        tobase = _testcapi.number_tobase

        self.assertEqual(tobase(10, 2), bin(10))
        self.assertEqual(tobase(11, 8), oct(11))
        self.assertEqual(tobase(16, 10), str(16))
        self.assertEqual(tobase(13, 16), hex(13))

        self.assertRaises(SystemError, tobase, NULL, 2)
        self.assertRaises(SystemError, tobase, 2, 42)

    def test_asssizet(self):
        # Test PyNumber_AsSsize_t()
        asssizet = _testcapi.number_asssizet

        for n in [*range(-6, 7), PY_SSIZE_T_MIN, PY_SSIZE_T_MAX]:
            self.assertEqual(asssizet(n, OverflowError), n)
        self.assertEqual(asssizet(PY_SSIZE_T_MAX+10, NULL), PY_SSIZE_T_MAX)
        self.assertEqual(asssizet(PY_SSIZE_T_MIN-10, NULL), PY_SSIZE_T_MIN)

        self.assertRaises(OverflowError, asssizet, PY_SSIZE_T_MAX + 10, OverflowError)
        self.assertRaises(RuntimeError, asssizet, PY_SSIZE_T_MAX + 10, RuntimeError)
        self.assertRaises(SystemError, asssizet, NULL, TypeError)


if __name__ == "__main__":
    unittest.main()
