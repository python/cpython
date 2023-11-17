import unittest
import sys
import warnings

from test.support import import_helper
from _testbuffer import ndarray

_testcapi = import_helper.import_module('_testcapi')
from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX

NULL = None


class WithDunder:
    @classmethod
    def with_val(cls, val):
        obj = super().__new__(cls)
        def meth(*args):
            return val
        setattr(cls, cls.methname, meth)
        return obj

    @classmethod
    def with_exc(cls, exc):
        obj = super().__new__(cls)
        def meth(*args):
            raise exc
        setattr(cls, cls.methname, meth)
        return obj

class IndexLike(WithDunder):
    methname = '__index__'

class IntLike(WithDunder):
    methname = '__int__'

class FloatLike(WithDunder):
    methname = '__float__'

class HasTrunc(WithDunder):
    methname = '__trunc__'

class BadDescr:
    def __get__(self, obj, objtype=None):
        raise ValueError

class HasBadTrunc:
    __trunc__ = BadDescr()


def subclassof(base):
    return type(base.__name__ + 'Subclass', (base,), {})


class HasRAddIntSubclass(WithDunder, int):
    methname = '__radd__'

class HasIAdd(WithDunder):
    methname = '__iadd__'

class HasRPowIntSubclass(WithDunder, int):
    methname = '__rpow__'

class HasIPow(WithDunder):
    methname = '__ipow__'


class WithMatrixMul(list):
    def __matmul__(self, other):
        return self*other

    def __imatmul__(self, other):
        x = self*other
        self.clear()
        self.extend(x)
        return self


class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyNumber_Check()
        check = _testcapi.number_check

        self.assertTrue(check(1))
        self.assertTrue(check(IndexLike.with_val(1)))
        self.assertTrue(check(IntLike.with_val(99)))
        self.assertTrue(check(0.5))
        self.assertTrue(check(FloatLike.with_val(4.25)))
        self.assertTrue(check(1+2j))
        self.assertFalse(check([]))
        self.assertFalse(check("1 + 1j"))
        self.assertFalse(check(object()))
        self.assertFalse(check(NULL))

    def test_add(self):
        # Test PyNumber_Add()
        add = _testcapi.number_add

        self.assertEqual(add(1, 2), 1 + 2)
        self.assertEqual(add(1, 0.75), 1 + 0.75)
        self.assertEqual(add(0.75, 1), 0.75 + 1)
        self.assertEqual(add(42, HasRAddIntSubclass.with_val(NotImplemented)), 42)
        self.assertEqual(add([1, 2], [3, 4]), [1, 2, 3, 4])


        self.assertRaises(TypeError, add, 1, object())
        self.assertRaises(TypeError, add, object(), 1)
        self.assertRaises(TypeError, add, ndarray([1], (1,)), 2)

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
        self.assertEqual(multiply([1], -1), [])

        self.assertRaises(TypeError, multiply, 1, object())
        self.assertRaises(TypeError, multiply, object(), 1)
        self.assertRaises(TypeError, multiply, ndarray([1], (1,)), 2)
        self.assertRaises(TypeError, multiply, 2, ndarray([1], (1,)))
        self.assertRaises(TypeError, multiply, [1], 3.14)
        self.assertRaises(OverflowError, multiply, [1], PY_SSIZE_T_MAX + 1)
        self.assertRaises(MemoryError, multiply, [1, 2], PY_SSIZE_T_MAX//2 + 1)

        # CRASHES multiply(NULL, 42)
        # CRASHES multiply(42, NULL)

    def test_matrixmultiply(self):
        # Test PyNumber_MatrixMultiply()
        matrixmultiply = _testcapi.number_matrixmultiply

        a, b, r = WithMatrixMul([1]), 2, [1, 1]
        self.assertEqual(matrixmultiply(a, b), r)
        self.assertEqual(a, [1])
        self.assertEqual(b, 2)

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

        self.assertEqual(power(4, 3), pow(4, 3))
        self.assertEqual(power(0.5, 2), pow(0.5, 2))
        self.assertEqual(power(2, -1.0), pow(2, -1.0))
        self.assertEqual(power(4, 11, 5), pow(4, 11, 5))
        self.assertEqual(power(4, subclassof(int)(11)), pow(4, 11))
        self.assertEqual(power(4, HasRPowIntSubclass.with_val(42)), 42)
        self.assertEqual(power(4, HasRPowIntSubclass.with_val(NotImplemented)), 1)

        self.assertRaises(TypeError, power, "spam", 42)
        self.assertRaises(TypeError, power, object(), 42)
        self.assertRaises(TypeError, power, 42, "spam")
        self.assertRaises(TypeError, power, 42, object())
        self.assertRaises(TypeError, power, 1, 2, "spam")
        self.assertRaises(TypeError, power, 1, 2, object())

        # CRASHES power(NULL, 42)
        # CRASHES power(42, NULL)

    def test_negative(self):
        # Test PyNumber_Negative()
        negative = _testcapi.number_negative

        self.assertEqual(negative(42), -42)
        self.assertEqual(negative(1.25), -1.25)

        self.assertRaises(TypeError, negative, "123")
        self.assertRaises(TypeError, negative, object())
        self.assertRaises(SystemError, negative, NULL)

    def test_positive(self):
        # Test PyNumber_Positive()
        positive = _testcapi.number_positive

        self.assertEqual(positive(-1), +(-1))
        self.assertEqual(positive(1.25), 1.25)

        self.assertRaises(TypeError, positive, "123")
        self.assertRaises(TypeError, positive, object())
        self.assertRaises(SystemError, positive, NULL)

    def test_absolute(self):
        # Test PyNumber_Absolute()
        absolute = _testcapi.number_absolute

        self.assertEqual(absolute(-1), abs(-1))
        self.assertEqual(absolute(-1.25), 1.25)
        self.assertEqual(absolute(1j), 1.0)

        self.assertRaises(TypeError, absolute, "123")
        self.assertRaises(TypeError, absolute, object())
        self.assertRaises(SystemError, absolute, NULL)

    def test_invert(self):
        # Test PyNumber_Invert()
        invert = _testcapi.number_invert

        self.assertEqual(invert(123), ~123)

        self.assertRaises(TypeError, invert, 1.25)
        self.assertRaises(TypeError, invert, "123")
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

        self.assertEqual(inplaceadd(1, 2), 1 + 2)
        self.assertEqual(inplaceadd(1, 0.75), 1 + 0.75)
        self.assertEqual(inplaceadd(0.75, 1), 0.75 + 1)

        a, b, r = [1, 2], [3, 4], [1, 2, 3, 4]
        self.assertEqual(inplaceadd(a, b), r)
        self.assertEqual(a, r)
        self.assertEqual(b, [3, 4])

        self.assertRaises(TypeError, inplaceadd, HasIAdd.with_val(NotImplemented), object())
        self.assertRaises(TypeError, inplaceadd, 1, object())
        self.assertRaises(TypeError, inplaceadd, object(), 1)

        # CRASHES inplaceadd(NULL, 42)
        # CRASHES inplaceadd(42, NULL)

    def test_inplacesubtract(self):
        # Test PyNumber_InPlaceSubtract()
        inplacesubtract = _testcapi.number_inplacesubtract

        self.assertEqual(inplacesubtract(1, 2), 1 - 2)

        self.assertRaises(TypeError, inplacesubtract, 1, object())
        self.assertRaises(TypeError, inplacesubtract, object(), 1)

        # CRASHES inplacesubtract(NULL, 42)
        # CRASHES inplacesubtract(42, NULL)

    def test_inplacemultiply(self):
        # Test PyNumber_InPlaceMultiply()
        inplacemultiply = _testcapi.number_inplacemultiply

        self.assertEqual(inplacemultiply(2, 3), 2 * 3)

        a, b, r = [1], 2, [1, 1]
        self.assertEqual(inplacemultiply(a, b), r)
        self.assertEqual(a, r)
        self.assertEqual(b, 2)

        a, b = 2, [1]
        self.assertEqual(inplacemultiply(a, b), r)
        self.assertEqual(a, 2)
        self.assertEqual(b, [1])

        self.assertRaises(TypeError, inplacemultiply, 1, object())
        self.assertRaises(TypeError, inplacemultiply, object(), 1)
        self.assertRaises(TypeError, inplacemultiply, ndarray([1], (1,)), 2)
        self.assertRaises(TypeError, inplacemultiply, 2, ndarray([1], (1,)))

        # CRASHES inplacemultiply(NULL, 42)
        # CRASHES inplacemultiply(42, NULL)

    def test_inplacematrixmultiply(self):
        # Test PyNumber_InPlaceMatrixMultiply()
        inplacematrixmultiply = _testcapi.number_inplacematrixmultiply

        a, b, r = WithMatrixMul([1]), 2, [1, 1]
        self.assertEqual(inplacematrixmultiply(a, b), r)
        self.assertEqual(a, r)
        self.assertEqual(b, 2)

        self.assertRaises(TypeError, inplacematrixmultiply, 2, a)

        # CRASHES inplacematrixmultiply(NULL, 42)
        # CRASHES inplacematrixmultiply(42, NULL)

    def test_inplacefloordivide(self):
        # Test PyNumber_InPlaceFloorDivide()
        inplacefloordivide = _testcapi.number_inplacefloordivide

        self.assertEqual(inplacefloordivide(4, 3), 4 // 3)

        # CRASHES inplacefloordivide(NULL, 42)
        # CRASHES inplacefloordivide(42, NULL)

    def test_inplacetruedivide(self):
        # Test PyNumber_InPlaceTrueDivide()
        inplacetruedivide = _testcapi.number_inplacetruedivide

        self.assertEqual(inplacetruedivide(3, 4), 3 / 4)

        # CRASHES inplacetruedivide(NULL, 42)
        # CRASHES inplacetruedivide(42, NULL)

    def test_inplaceremainder(self):
        # Test PyNumber_InPlaceRemainder()
        inplaceremainder = _testcapi.number_inplaceremainder

        self.assertEqual(inplaceremainder(4, 3), 4 % 3)

        # CRASHES inplaceremainder(NULL, 42)
        # CRASHES inplaceremainder(42, NULL)

    def test_inplacepower(self):
        # Test PyNumber_InPlacePower()
        inplacepower = _testcapi.number_inplacepower

        self.assertEqual(inplacepower(2, 3), pow(2, 3))
        self.assertEqual(inplacepower(2, 3, 4), pow(2, 3, 4))
        self.assertEqual(inplacepower(HasIPow.with_val(42), 2), 42)

        self.assertRaises(TypeError, inplacepower,
                          HasIPow.with_val(NotImplemented), object())
        self.assertRaises(TypeError, inplacepower, object(), 2)

        # CRASHES inplacepower(NULL, 42)
        # CRASHES inplacepower(42, NULL)

    def test_inplacelshift(self):
        # Test PyNumber_InPlaceLshift()
        inplacelshift = _testcapi.number_inplacelshift

        self.assertEqual(inplacelshift(3, 5), 3 << 5)

        # CRASHES inplacelshift(NULL, 42)
        # CRASHES inplacelshift(42, NULL)

    def test_inplacershift(self):
        # Test PyNumber_InPlaceRshift()
        inplacershift = _testcapi.number_inplacershift

        self.assertEqual(inplacershift(5, 3), 5 >> 3)

        # CRASHES inplacershift(NULL, 42)
        # CRASHES inplacershift(42, NULL)

    def test_inplaceand(self):
        # Test PyNumber_InPlaceAnd()
        inplaceand = _testcapi.number_inplaceand

        self.assertEqual(inplaceand(0b10, 0b01), 0b10 & 0b01)

        a, b, r = {1, 2}, {2, 3}, {1, 2} & {2, 3}
        self.assertEqual(inplaceand(a, b), r)
        self.assertEqual(a, r)
        self.assertEqual(b, {2, 3})

        # CRASHES inplaceand(NULL, 42)
        # CRASHES inplaceand(42, NULL)

    def test_inplacexor(self):
        # Test PyNumber_InPlaceXor()
        inplacexor = _testcapi.number_inplacexor

        self.assertEqual(inplacexor(0b10, 0b01), 0b10 ^ 0b01)

        a, b, r = {1, 2}, {2, 3}, {1, 2} ^ {2, 3}
        self.assertEqual(inplacexor(a, b), r)
        self.assertEqual(a, r)
        self.assertEqual(b, {2, 3})

        # CRASHES inplacexor(NULL, 42)
        # CRASHES inplacexor(42, NULL)

    def test_inplaceor(self):
        # Test PyNumber_InPlaceOr()
        inplaceor = _testcapi.number_inplaceor

        self.assertEqual(inplaceor(0b10, 0b01), 0b10 | 0b01)

        a, b, r = {1, 2}, {2, 3}, {1, 2} | {2, 3}
        self.assertEqual(inplaceor(a, b), r)
        self.assertEqual(a, r)
        self.assertEqual(b, {2, 3})

        # CRASHES inplaceor(NULL, 42)
        # CRASHES inplaceor(42, NULL)

    def test_long(self):
        # Test PyNumber_Long()
        long = _testcapi.number_long

        self.assertEqual(long(42), 42)
        self.assertEqual(long(1.25), 1)
        self.assertEqual(long("42"), 42)
        self.assertEqual(long(b"42"), 42)
        self.assertEqual(long(bytearray(b"42")), 42)
        self.assertEqual(long(memoryview(b"42")), 42)
        self.assertEqual(long(IndexLike.with_val(99)), 99)
        self.assertEqual(long(IntLike.with_val(99)), 99)

        self.assertRaises(TypeError, long, IntLike.with_val(1.0))
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, long, IntLike.with_val(True))
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(long(IntLike.with_val(True)), 1)
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, long, HasTrunc.with_val(42))
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(long(HasTrunc.with_val(42)), 42)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(long(HasTrunc.with_val(subclassof(int)(42))), 42)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(long(HasTrunc.with_val(IndexLike.with_val(42))), 42)

        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, long, HasTrunc.with_val(1.25))

        self.assertRaises(TypeError, long, 1j)
        self.assertRaises(TypeError, long, object())
        self.assertRaises(SystemError, long, NULL)
        self.assertRaises(RuntimeError, long, HasTrunc.with_exc(RuntimeError))
        self.assertRaises(ValueError, long, HasBadTrunc())
        self.assertRaises(RuntimeError, long, IntLike.with_exc(RuntimeError))

    def test_float(self):
        # Test PyNumber_Float()
        float_ = _testcapi.number_float

        self.assertEqual(float_(1.25), 1.25)
        self.assertEqual(float_(123), 123.)
        self.assertEqual(float_("1.25"), 1.25)

        self.assertEqual(float_(FloatLike.with_val(4.25)), 4.25)
        self.assertEqual(float_(IndexLike.with_val(99)), 99.0)
        self.assertEqual(float_(IndexLike.with_val(-1)), -1.0)

        self.assertRaises(TypeError, float_, FloatLike.with_val(687))
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, float_, FloatLike.with_val(subclassof(float)(4.25)))
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(float_(FloatLike.with_val(subclassof(float)(4.25))), 4.25)
        self.assertRaises(TypeError, float_, IndexLike.with_val(1.25))
        self.assertRaises(OverflowError, float_, IndexLike.with_val(2**2000))
        self.assertRaises(TypeError, float_, 1j)
        self.assertRaises(TypeError, float_, object())
        self.assertRaises(SystemError, float_, NULL)
        self.assertRaises(RuntimeError, float_, FloatLike.with_exc(RuntimeError))

    def test_index(self):
        # Test PyNumber_Index()
        index = _testcapi.number_index

        self.assertEqual(index(11), 11)

        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, index, IndexLike.with_val(True))
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(index(IndexLike.with_val(True)), 1)
        self.assertRaises(TypeError, index, 1.25)
        self.assertRaises(TypeError, index, "42")
        self.assertRaises(TypeError, index, IndexLike.with_val(1.0))
        self.assertRaises(TypeError, index, object())
        self.assertRaises(SystemError, index, NULL)
        self.assertRaises(RuntimeError, index, IndexLike.with_exc(RuntimeError))

    def test_tobase(self):
        # Test PyNumber_ToBase()
        tobase = _testcapi.number_tobase

        self.assertEqual(tobase(10, 2), bin(10))
        self.assertEqual(tobase(11, 8), oct(11))
        self.assertEqual(tobase(16, 10), str(16))
        self.assertEqual(tobase(13, 16), hex(13))

        self.assertRaises(SystemError, tobase, NULL, 2)
        self.assertRaises(SystemError, tobase, 2, 3)
        self.assertRaises(TypeError, tobase, 1.25, 2)
        self.assertRaises(TypeError, tobase, "42", 2)

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
