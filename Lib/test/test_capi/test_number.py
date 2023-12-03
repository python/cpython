import operator
import unittest
import sys
import warnings

from test.support import cpython_only, import_helper
from _testbuffer import ndarray

_testcapi = import_helper.import_module('_testcapi')
from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX

NULL = None

class BadDescr:
    def __get__(self, obj, objtype=None):
        raise RuntimeError

class WithDunder:
    def _meth(self, *args):
        if self.val:
            return self.val
        if self.exc:
            raise self.exc
    @classmethod
    def with_val(cls, val):
        obj = super().__new__(cls)
        obj.val = val
        obj.exc = None
        setattr(cls, cls.methname, cls._meth)
        return obj

    @classmethod
    def with_exc(cls, exc):
        obj = super().__new__(cls)
        obj.val = None
        obj.exc = exc
        setattr(cls, cls.methname, cls._meth)
        return obj

class HasBadAttr:
    def __new__(cls):
        obj = super().__new__(cls)
        setattr(cls, cls.methname, BadDescr())
        return obj


class IndexLike(WithDunder):
    methname = '__index__'

class IntLike(WithDunder):
    methname = '__int__'

class FloatLike(WithDunder):
    methname = '__float__'

class HasTrunc(WithDunder):
    methname = '__trunc__'


class HasBadTrunc:
    __trunc__ = BadDescr()


def subclassof(base):
    return type(base.__name__ + 'Subclass', (base,), {})


class SomeError(Exception):
    pass

class OtherError(Exception):
    pass


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

    def test_unary_ops(self):
        methmap = {'__neg__': _testcapi.number_negative,   # PyNumber_Negative()
                   '__pos__': _testcapi.number_positive,   # PyNumber_Positive()
                   '__abs__': _testcapi.number_absolute,   # PyNumber_Absolute()
                   '__invert__': _testcapi.number_invert}  # PyNumber_Invert()

        for name, func in methmap.items():
            class HasAdd(WithDunder):
                methname = '__add__'
            class HasMeth(WithDunder):
                methname = name
            class BadMeth(HasBadAttr):
                methname = name

            # Generic object, has no tp_as_number structure
            self.assertRaises(TypeError, func, object())

            # Has tp_as_number, but not the given unary op slot
            self.assertRaises(TypeError, func, HasAdd.with_val("don't care about"))

            # Dunder method triggers an error
            self.assertRaises(SomeError, func, HasMeth.with_exc(SomeError))

            # Finally, it returns something
            self.assertEqual(func(HasMeth.with_val(42)), 42)
            self.assertEqual(func(HasMeth.with_val(NotImplemented)), NotImplemented)

            # C-API function accepts NULL
            self.assertRaises(SystemError, func, NULL)

    def test_binary_ops(self):
        methmap = {'__add__': _testcapi.number_add,   # PyNumber_Add()
                   '__sub__': _testcapi.number_subtract,  # PyNumber_Subtract()
                   '__mul__': _testcapi.number_multiply,  # PyNumber_Multiply()
                   '__matmul__': _testcapi.number_matrixmultiply,  # PyNumber_MatrixMultiply()
                   '__floordiv__': _testcapi.number_floordivide,  # PyNumber_FloorDivide()
                   '__truediv__': _testcapi.number_truedivide,  # PyNumber_TrueDivide()
                   '__mod__': _testcapi.number_remainder,  # PyNumber_Remainder()
                   '__divmod__': _testcapi.number_divmod,  # PyNumber_Divmod()
                   '__lshift__': _testcapi.number_lshift,  # PyNumber_Lshift()
                   '__rshift__': _testcapi.number_rshift,  # PyNumber_Rshift()
                   '__and__': _testcapi.number_and,  # PyNumber_And()
                   '__xor__': _testcapi.number_xor,  # PyNumber_Xor()
                   '__or__': _testcapi.number_or,  # PyNumber_Or()
                   '__pow__': _testcapi.number_power,  # PyNumber_Power()
                   }

        for name, func in methmap.items():
            rname = '__r' + name[2:]
            class HasPos(WithDunder):
                methname = '__pos__'
            class HasMeth(WithDunder):
                methname = name
            class HasRMeth(WithDunder):
                methname = rname

            # First argument has no tp_as_number structure
            x = object()
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertEqual(func(x, HasRMeth.with_val(42)), 42)
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, x, HasRMeth.with_val(NotImplemented))

            # Has tp_as_number, but not the given binary op slot
            x = HasPos.with_val("don't care about")
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertEqual(func(x, HasRMeth.with_val(42)), 42)
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, x, HasRMeth.with_val(NotImplemented))

            # First argument has dunder/rdunder method, but it triggers an error
            x = HasMeth.with_exc(SomeError)
            self.assertRaises(SomeError, func, x, object())
            self.assertRaises(SomeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(SomeError, func, x, HasMeth.with_exc(OtherError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(OtherError))
            self.assertRaises(SomeError, func, x, HasMeth.with_val(42))
            self.assertRaises(SomeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(SomeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(SomeError, func, x, HasRMeth.with_val(NotImplemented))
            x = HasRMeth.with_exc(SomeError)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(OtherError))
            self.assertRaises(TypeError, func, x, HasRMeth.with_exc(OtherError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))

            # First argument has dunder/rdunder method, but it returns NotImplemented
            x = HasMeth.with_val(NotImplemented)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertEqual(func(x, HasRMeth.with_val(42)), 42)
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))
            x = HasRMeth.with_val(NotImplemented)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))

            # First argument has dunder/rdunder method and it returns something
            x = HasMeth.with_val(123)
            self.assertEqual(func(x, object()), 123)
            self.assertEqual(func(x, HasPos.with_val("don't care about")), 123)
            self.assertEqual(func(x, HasMeth.with_exc(SomeError)), 123)
            self.assertEqual(func(x, HasRMeth.with_exc(SomeError)), 123)
            self.assertEqual(func(x, HasMeth.with_val(42)), 123)
            self.assertEqual(func(x, HasRMeth.with_val(42)), 123)
            self.assertEqual(func(x, HasMeth.with_val(NotImplemented)), 123)
            self.assertEqual(func(x, HasRMeth.with_val(NotImplemented)), 123)
            x = HasRMeth.with_val(123)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))

            class IntSubclsWithRMeth2(WithDunder, int):
                methname = rname
            class FloatSubclsWithRMeth2(WithDunder, float):
                methname = rname

            # Test subtype with overloaded rdunder method (slotv != slotw)
            if hasattr(int, name):
                op = getattr(operator, name, divmod)
                try:
                    self.assertEqual(func(123, IntSubclsWithRMeth2.with_val(NotImplemented)), op(123, 0))
                except ZeroDivisionError:
                    self.assertRaises(ZeroDivisionError, func, 123, IntSubclsWithRMeth2.with_val(NotImplemented))
                    self.assertRaises(ZeroDivisionError, op, 123, 0)

                self.assertEqual(func(123, IntSubclsWithRMeth2.with_val(42)), 42)
                self.assertEqual(func(123, FloatSubclsWithRMeth2.with_val(0.5)), 0.5)

            # CRASHES func(NULL, object())
            # CRASHES func(object(), NULL)

    def test_inplace_binary_ops(self):
        methmap = {'__iadd__': _testcapi.number_inplaceadd,   # PyNumber_InPlaceAdd()
                   '__isub__': _testcapi.number_inplacesubtract,  # PyNumber_InPlaceSubtract()
                   '__imul__': _testcapi.number_inplacemultiply,  # PyNumber_InPlaceMultiply()
                   '__imatmul__': _testcapi.number_inplacematrixmultiply,  # PyNumber_InPlaceMatrixMultiply()
                   '__ifloordiv__': _testcapi.number_inplacefloordivide,  # PyNumber_InPlaceFloorDivide()
                   '__itruediv__': _testcapi.number_inplacetruedivide,  # PyNumber_InPlaceTrueDivide()
                   '__imod__': _testcapi.number_inplaceremainder,  # PyNumber_InPlaceRemainder()
                   '__ilshift__': _testcapi.number_inplacelshift,  # PyNumber_InPlaceLshift()
                   '__irshift__': _testcapi.number_inplacershift,  # PyNumber_InPlaceRshift()
                   '__iand__': _testcapi.number_inplaceand,  # PyNumber_InPlaceAnd()
                   '__ixor__': _testcapi.number_inplacexor,  # PyNumber_InPlaceXor()
                   '__ior__': _testcapi.number_inplaceor,  # PyNumber_InPlaceOr()
                   '__ipow__': _testcapi.number_inplacepower,  # PyNumber_InPlacePower()
                   }

        for iname, func in methmap.items():
            name = '__' + iname[3:]
            rname = '__r' + name[2:]
            class HasPos(WithDunder):
                methname = '__pos__'
            class HasMeth(WithDunder):
                methname = name
            class HasRMeth(WithDunder):
                methname = rname
            class HasIMeth(WithDunder):
                methname = iname

            # First argument has no tp_as_number structure
            x = object()
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasIMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertEqual(func(x, HasRMeth.with_val(42)), 42)
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, x, HasIMeth.with_val(NotImplemented))

            # Has tp_as_number, but not the given binary op slot
            x = HasPos.with_val("don't care about")
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasIMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertEqual(func(x, HasRMeth.with_val(42)), 42)
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, x, HasIMeth.with_val(NotImplemented))

            # First argument has dunder/rdunder/idunder method, but it triggers an error
            x = HasMeth.with_exc(SomeError)
            self.assertRaises(SomeError, func, x, object())
            self.assertRaises(SomeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(SomeError, func, x, HasMeth.with_exc(OtherError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(OtherError))
            self.assertRaises(SomeError, func, x, HasIMeth.with_exc(OtherError))
            self.assertRaises(SomeError, func, x, HasMeth.with_val(42))
            self.assertRaises(SomeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(SomeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(SomeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(SomeError, func, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(SomeError, func, x, HasIMeth.with_val(NotImplemented))
            x = HasRMeth.with_exc(SomeError)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(OtherError))
            self.assertRaises(TypeError, func, x, HasRMeth.with_exc(OtherError))
            self.assertRaises(TypeError, func, x, HasIMeth.with_exc(OtherError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(NotImplemented))
            x = HasIMeth.with_exc(SomeError)
            self.assertRaises(SomeError, func, x, object())
            self.assertRaises(SomeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(SomeError, func, x, HasMeth.with_exc(OtherError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(OtherError))
            self.assertRaises(SomeError, func, x, HasIMeth.with_exc(OtherError))
            self.assertRaises(SomeError, func, x, HasMeth.with_val(42))
            self.assertRaises(SomeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(SomeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(SomeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(SomeError, func, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(SomeError, func, x, HasIMeth.with_val(NotImplemented))

            # First argument has dunder/rdunder/idunder method, but it returns NotImplemented
            x = HasMeth.with_val(NotImplemented)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasIMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertEqual(func(x, HasRMeth.with_val(42)), 42)
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(NotImplemented))
            x = HasRMeth.with_val(NotImplemented)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasIMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(NotImplemented))
            x = HasIMeth.with_val(NotImplemented)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(SomeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasIMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertEqual(func(x, HasRMeth.with_val(42)), 42)
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(NotImplemented))

            # First argument has dunder/rdunder/idunder method and it returns something
            x = HasMeth.with_val(123)
            self.assertEqual(func(x, object()), 123)
            self.assertEqual(func(x, HasPos.with_val("don't care about")), 123)
            self.assertEqual(func(x, HasMeth.with_exc(SomeError)), 123)
            self.assertEqual(func(x, HasRMeth.with_exc(SomeError)), 123)
            self.assertEqual(func(x, HasIMeth.with_exc(SomeError)), 123)
            self.assertEqual(func(x, HasMeth.with_val(42)), 123)
            self.assertEqual(func(x, HasRMeth.with_val(42)), 123)
            self.assertEqual(func(x, HasIMeth.with_val(42)), 123)
            self.assertEqual(func(x, HasMeth.with_val(NotImplemented)), 123)
            self.assertEqual(func(x, HasRMeth.with_val(NotImplemented)), 123)
            self.assertEqual(func(x, HasIMeth.with_val(NotImplemented)), 123)
            x = HasRMeth.with_val(123)
            self.assertRaises(TypeError, func, x, object())
            self.assertRaises(TypeError, func, x, HasPos.with_val("don't care about"))
            self.assertRaises(TypeError, func, x, HasMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasRMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasIMeth.with_exc(SomeError))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(42))
            self.assertRaises(TypeError, func, x, HasMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasRMeth.with_val(NotImplemented))
            self.assertRaises(TypeError, func, x, HasIMeth.with_val(NotImplemented))
            x = HasIMeth.with_val(123)
            self.assertEqual(func(x, object()), 123)
            self.assertEqual(func(x, HasPos.with_val("don't care about")), 123)
            self.assertEqual(func(x, HasMeth.with_exc(SomeError)), 123)
            self.assertEqual(func(x, HasRMeth.with_exc(SomeError)), 123)
            self.assertEqual(func(x, HasIMeth.with_exc(SomeError)), 123)
            self.assertEqual(func(x, HasMeth.with_val(42)), 123)
            self.assertEqual(func(x, HasRMeth.with_val(42)), 123)
            self.assertEqual(func(x, HasIMeth.with_val(42)), 123)
            self.assertEqual(func(x, HasMeth.with_val(NotImplemented)), 123)
            self.assertEqual(func(x, HasRMeth.with_val(NotImplemented)), 123)
            self.assertEqual(func(x, HasIMeth.with_val(NotImplemented)), 123)

            # CRASHES func(NULL, object())
            # CRASHES func(object(), NULL)

    def test_misc(self):
        # PyNumber_Add(), PyNumber_InPlaceAdd()
        add = _testcapi.number_add
        inplaceadd = _testcapi.number_inplaceadd

        # test sq_concat/sq_inplace_concat slots
        a, b, r = [1, 2], [3, 4], [1, 2, 3, 4]
        self.assertEqual(add(a, b), r)
        self.assertEqual(a, [1, 2])
        self.assertRaises(TypeError, add, ndarray([1], (1,)), 2)
        a, b, r = [1, 2], [3, 4], [1, 2, 3, 4]
        self.assertEqual(inplaceadd(a, b), r)
        self.assertEqual(a, r)
        self.assertRaises(TypeError, inplaceadd, ndarray([1], (1,)), 2)

        # PyNumber_Multiply(), PyNumber_InPlaceMultiply()
        multiply = _testcapi.number_multiply
        inplacemultiply = _testcapi.number_inplacemultiply

        # test sq_repeat/sq_inplace_repeat slots
        a, b, r = [1], 2, [1, 1]
        self.assertEqual(multiply(a, b), r)
        self.assertEqual((a, b), ([1], 2))
        self.assertEqual(multiply(b, a), r)
        self.assertEqual((a, b), ([1], 2))
        self.assertEqual(multiply([1], -1), [])
        self.assertRaises(TypeError, multiply, ndarray([1], (1,)), 2)
        self.assertRaises(TypeError, multiply, [1], 0.5)
        self.assertRaises(OverflowError, multiply, [1], PY_SSIZE_T_MAX + 1)
        self.assertRaises(MemoryError, multiply, [1, 2], PY_SSIZE_T_MAX//2 + 1)
        a, b, r = [1], 2, [1, 1]
        self.assertEqual(inplacemultiply(a, b), r)
        self.assertEqual((a, b), (r, 2))
        a = [1]
        self.assertEqual(inplacemultiply(b, a), r)
        self.assertEqual((a, b), ([1], 2))
        self.assertRaises(TypeError, inplacemultiply, ndarray([1], (1,)), 2)
        self.assertRaises(OverflowError, inplacemultiply, [1], PY_SSIZE_T_MAX + 1)
        self.assertRaises(MemoryError, inplacemultiply, [1, 2], PY_SSIZE_T_MAX//2 + 1)

        # PyNumber_Power()
        power = _testcapi.number_power

        class HasPow(WithDunder):
            methname = '__pow__'

        # ternary op
        self.assertEqual(power(4, 11, 5), pow(4, 11, 5))
        self.assertRaises(TypeError, power, 4, 11, 1.25)
        self.assertRaises(TypeError, power, 4, 11, HasPow.with_val(NotImplemented))
        self.assertRaises(TypeError, power, 4, 11, object())

    @cpython_only
    def test_rshift_print(self):
        # This tests correct syntax hint for py2 redirection (>>).
        rshift = _testcapi.number_rshift

        with self.assertRaises(TypeError) as context:
            rshift(print, 42)
        self.assertIn('Did you mean "print(<message>, '
                      'file=<output_stream>)"?', str(context.exception))
        with self.assertRaises(TypeError) as context:
            rshift(max, sys.stderr)
        self.assertNotIn('Did you mean ', str(context.exception))
        with self.assertRaises(TypeError) as context:
            rshift(1, "spam")

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
        self.assertRaises(RuntimeError, long, IntLike.with_exc(RuntimeError))

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
        self.assertRaises(RuntimeError, long, HasTrunc.with_exc(RuntimeError))
        self.assertRaises(RuntimeError, long, HasBadTrunc())

        self.assertRaises(TypeError, long, 1j)
        self.assertRaises(TypeError, long, object())
        self.assertRaises(SystemError, long, NULL)

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
        self.assertRaises(RuntimeError, float_, FloatLike.with_exc(RuntimeError))

        self.assertRaises(TypeError, float_, IndexLike.with_val(1.25))
        self.assertRaises(OverflowError, float_, IndexLike.with_val(2**2000))

        self.assertRaises(TypeError, float_, 1j)
        self.assertRaises(TypeError, float_, object())
        self.assertRaises(SystemError, float_, NULL)

    def test_index(self):
        # Test PyNumber_Index()
        index = _testcapi.number_index

        self.assertEqual(index(11), 11)

        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, index, IndexLike.with_val(True))
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(index(IndexLike.with_val(True)), 1)
        self.assertRaises(TypeError, index, IndexLike.with_val(1.0))
        self.assertRaises(RuntimeError, index, IndexLike.with_exc(RuntimeError))

        self.assertRaises(TypeError, index, 1.25)
        self.assertRaises(TypeError, index, "42")
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
