import unittest
import math
import string
import sys
import warnings
from test import support
from test.support import import_helper
from test.support import warnings_helper
# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')
from _testcapi import getargs_keywords, getargs_keyword_only

# > How about the following counterproposal. This also changes some of
# > the other format codes to be a little more regular.
# >
# > Code C type Range check
# >
# > b unsigned char 0..UCHAR_MAX
# > h signed short SHRT_MIN..SHRT_MAX
# > B unsigned char none **
# > H unsigned short none **
# > k * unsigned long none
# > I * unsigned int 0..UINT_MAX
#
#
# > i int INT_MIN..INT_MAX
# > l long LONG_MIN..LONG_MAX
#
# > K * unsigned long long none
# > L long long LLONG_MIN..LLONG_MAX
#
# > Notes:
# >
# > * New format codes.
# >
# > ** Changed from previous "range-and-a-half" to "none"; the
# > range-and-a-half checking wasn't particularly useful.
#
# Plus a C API or two, e.g. PyLong_AsUnsignedLongMask() ->
# unsigned long and PyLong_AsUnsignedLongLongMask() -> unsigned
# long long (if that exists).

LARGE = 0x7FFFFFFF
VERY_LARGE = 0xFF0000121212121212121242

from _testcapi import UCHAR_MAX, USHRT_MAX, UINT_MAX, ULONG_MAX, INT_MAX, \
     INT_MIN, LONG_MIN, LONG_MAX, PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, \
     SHRT_MIN, SHRT_MAX, FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX

DBL_MAX_EXP = sys.float_info.max_exp
INF = float('inf')
NAN = float('nan')

# fake, they are not defined in Python's header files
LLONG_MAX = 2**63-1
LLONG_MIN = -2**63
ULLONG_MAX = 2**64-1

class Index:
    def __index__(self):
        return 99

class IndexIntSubclass(int):
    def __index__(self):
        return 99

class BadIndex:
    def __index__(self):
        return 1.0

class BadIndex2:
    def __index__(self):
        return True

class BadIndex3(int):
    def __index__(self):
        return True


class Int:
    def __int__(self):
        return 99

class IntSubclass(int):
    def __int__(self):
        return 99

class BadInt:
    def __int__(self):
        return 1.0

class BadInt2:
    def __int__(self):
        return True

class BadInt3(int):
    def __int__(self):
        return True


class Float:
    def __float__(self):
        return 4.25

class FloatSubclass(float):
    pass

class FloatSubclass2(float):
    def __float__(self):
        return 4.25

class BadFloat:
    def __float__(self):
        return 687

class BadFloat2:
    def __float__(self):
        return FloatSubclass(4.25)

class BadFloat3(float):
    def __float__(self):
        return FloatSubclass(4.25)


class Complex:
    def __complex__(self):
        return 4.25+0.5j

class ComplexSubclass(complex):
    pass

class ComplexSubclass2(complex):
    def __complex__(self):
        return 4.25+0.5j

class BadComplex:
    def __complex__(self):
        return 1.25

class BadComplex2:
    def __complex__(self):
        return ComplexSubclass(4.25+0.5j)

class BadComplex3(complex):
    def __complex__(self):
        return ComplexSubclass(4.25+0.5j)


class TupleSubclass(tuple):
    pass

class DictSubclass(dict):
    pass


class Unsigned_TestCase(unittest.TestCase):
    def test_b(self):
        from _testcapi import getargs_b
        # b returns 'unsigned char', and does range checking (0 ... UCHAR_MAX)
        self.assertRaises(TypeError, getargs_b, 3.14)
        self.assertEqual(99, getargs_b(Index()))
        self.assertEqual(0, getargs_b(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_b, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_b(BadIndex2()))
        self.assertEqual(0, getargs_b(BadIndex3()))
        self.assertRaises(TypeError, getargs_b, Int())
        self.assertEqual(0, getargs_b(IntSubclass()))
        self.assertRaises(TypeError, getargs_b, BadInt())
        self.assertRaises(TypeError, getargs_b, BadInt2())
        self.assertEqual(0, getargs_b(BadInt3()))

        self.assertRaises(OverflowError, getargs_b, -1)
        self.assertEqual(0, getargs_b(0))
        self.assertEqual(UCHAR_MAX, getargs_b(UCHAR_MAX))
        self.assertRaises(OverflowError, getargs_b, UCHAR_MAX + 1)

        self.assertEqual(42, getargs_b(42))
        self.assertRaises(OverflowError, getargs_b, VERY_LARGE)

    def test_B(self):
        from _testcapi import getargs_B
        # B returns 'unsigned char', no range checking
        self.assertRaises(TypeError, getargs_B, 3.14)
        self.assertEqual(99, getargs_B(Index()))
        self.assertEqual(0, getargs_B(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_B, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_B(BadIndex2()))
        self.assertEqual(0, getargs_B(BadIndex3()))
        self.assertRaises(TypeError, getargs_B, Int())
        self.assertEqual(0, getargs_B(IntSubclass()))
        self.assertRaises(TypeError, getargs_B, BadInt())
        self.assertRaises(TypeError, getargs_B, BadInt2())
        self.assertEqual(0, getargs_B(BadInt3()))

        self.assertEqual(UCHAR_MAX, getargs_B(-1))
        self.assertEqual(0, getargs_B(0))
        self.assertEqual(UCHAR_MAX, getargs_B(UCHAR_MAX))
        self.assertEqual(0, getargs_B(UCHAR_MAX+1))

        self.assertEqual(42, getargs_B(42))
        self.assertEqual(UCHAR_MAX & VERY_LARGE, getargs_B(VERY_LARGE))

    def test_H(self):
        from _testcapi import getargs_H
        # H returns 'unsigned short', no range checking
        self.assertRaises(TypeError, getargs_H, 3.14)
        self.assertEqual(99, getargs_H(Index()))
        self.assertEqual(0, getargs_H(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_H, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_H(BadIndex2()))
        self.assertEqual(0, getargs_H(BadIndex3()))
        self.assertRaises(TypeError, getargs_H, Int())
        self.assertEqual(0, getargs_H(IntSubclass()))
        self.assertRaises(TypeError, getargs_H, BadInt())
        self.assertRaises(TypeError, getargs_H, BadInt2())
        self.assertEqual(0, getargs_H(BadInt3()))

        self.assertEqual(USHRT_MAX, getargs_H(-1))
        self.assertEqual(0, getargs_H(0))
        self.assertEqual(USHRT_MAX, getargs_H(USHRT_MAX))
        self.assertEqual(0, getargs_H(USHRT_MAX+1))

        self.assertEqual(42, getargs_H(42))

        self.assertEqual(VERY_LARGE & USHRT_MAX, getargs_H(VERY_LARGE))

    def test_I(self):
        from _testcapi import getargs_I
        # I returns 'unsigned int', no range checking
        self.assertRaises(TypeError, getargs_I, 3.14)
        self.assertEqual(99, getargs_I(Index()))
        self.assertEqual(0, getargs_I(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_I, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_I(BadIndex2()))
        self.assertEqual(0, getargs_I(BadIndex3()))
        self.assertRaises(TypeError, getargs_I, Int())
        self.assertEqual(0, getargs_I(IntSubclass()))
        self.assertRaises(TypeError, getargs_I, BadInt())
        self.assertRaises(TypeError, getargs_I, BadInt2())
        self.assertEqual(0, getargs_I(BadInt3()))

        self.assertEqual(UINT_MAX, getargs_I(-1))
        self.assertEqual(0, getargs_I(0))
        self.assertEqual(UINT_MAX, getargs_I(UINT_MAX))
        self.assertEqual(0, getargs_I(UINT_MAX+1))

        self.assertEqual(42, getargs_I(42))

        self.assertEqual(VERY_LARGE & UINT_MAX, getargs_I(VERY_LARGE))

    def test_k(self):
        from _testcapi import getargs_k
        # k returns 'unsigned long', no range checking
        # it does not accept float, or instances with __int__
        self.assertRaises(TypeError, getargs_k, 3.14)
        self.assertRaises(TypeError, getargs_k, Index())
        self.assertEqual(0, getargs_k(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_k, BadIndex())
        self.assertRaises(TypeError, getargs_k, BadIndex2())
        self.assertEqual(0, getargs_k(BadIndex3()))
        self.assertRaises(TypeError, getargs_k, Int())
        self.assertEqual(0, getargs_k(IntSubclass()))
        self.assertRaises(TypeError, getargs_k, BadInt())
        self.assertRaises(TypeError, getargs_k, BadInt2())
        self.assertEqual(0, getargs_k(BadInt3()))

        self.assertEqual(ULONG_MAX, getargs_k(-1))
        self.assertEqual(0, getargs_k(0))
        self.assertEqual(ULONG_MAX, getargs_k(ULONG_MAX))
        self.assertEqual(0, getargs_k(ULONG_MAX+1))

        self.assertEqual(42, getargs_k(42))

        self.assertEqual(VERY_LARGE & ULONG_MAX, getargs_k(VERY_LARGE))

class Signed_TestCase(unittest.TestCase):
    def test_h(self):
        from _testcapi import getargs_h
        # h returns 'short', and does range checking (SHRT_MIN ... SHRT_MAX)
        self.assertRaises(TypeError, getargs_h, 3.14)
        self.assertEqual(99, getargs_h(Index()))
        self.assertEqual(0, getargs_h(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_h, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_h(BadIndex2()))
        self.assertEqual(0, getargs_h(BadIndex3()))
        self.assertRaises(TypeError, getargs_h, Int())
        self.assertEqual(0, getargs_h(IntSubclass()))
        self.assertRaises(TypeError, getargs_h, BadInt())
        self.assertRaises(TypeError, getargs_h, BadInt2())
        self.assertEqual(0, getargs_h(BadInt3()))

        self.assertRaises(OverflowError, getargs_h, SHRT_MIN-1)
        self.assertEqual(SHRT_MIN, getargs_h(SHRT_MIN))
        self.assertEqual(SHRT_MAX, getargs_h(SHRT_MAX))
        self.assertRaises(OverflowError, getargs_h, SHRT_MAX+1)

        self.assertEqual(42, getargs_h(42))
        self.assertRaises(OverflowError, getargs_h, VERY_LARGE)

    def test_i(self):
        from _testcapi import getargs_i
        # i returns 'int', and does range checking (INT_MIN ... INT_MAX)
        self.assertRaises(TypeError, getargs_i, 3.14)
        self.assertEqual(99, getargs_i(Index()))
        self.assertEqual(0, getargs_i(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_i, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_i(BadIndex2()))
        self.assertEqual(0, getargs_i(BadIndex3()))
        self.assertRaises(TypeError, getargs_i, Int())
        self.assertEqual(0, getargs_i(IntSubclass()))
        self.assertRaises(TypeError, getargs_i, BadInt())
        self.assertRaises(TypeError, getargs_i, BadInt2())
        self.assertEqual(0, getargs_i(BadInt3()))

        self.assertRaises(OverflowError, getargs_i, INT_MIN-1)
        self.assertEqual(INT_MIN, getargs_i(INT_MIN))
        self.assertEqual(INT_MAX, getargs_i(INT_MAX))
        self.assertRaises(OverflowError, getargs_i, INT_MAX+1)

        self.assertEqual(42, getargs_i(42))
        self.assertRaises(OverflowError, getargs_i, VERY_LARGE)

    def test_l(self):
        from _testcapi import getargs_l
        # l returns 'long', and does range checking (LONG_MIN ... LONG_MAX)
        self.assertRaises(TypeError, getargs_l, 3.14)
        self.assertEqual(99, getargs_l(Index()))
        self.assertEqual(0, getargs_l(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_l, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_l(BadIndex2()))
        self.assertEqual(0, getargs_l(BadIndex3()))
        self.assertRaises(TypeError, getargs_l, Int())
        self.assertEqual(0, getargs_l(IntSubclass()))
        self.assertRaises(TypeError, getargs_l, BadInt())
        self.assertRaises(TypeError, getargs_l, BadInt2())
        self.assertEqual(0, getargs_l(BadInt3()))

        self.assertRaises(OverflowError, getargs_l, LONG_MIN-1)
        self.assertEqual(LONG_MIN, getargs_l(LONG_MIN))
        self.assertEqual(LONG_MAX, getargs_l(LONG_MAX))
        self.assertRaises(OverflowError, getargs_l, LONG_MAX+1)

        self.assertEqual(42, getargs_l(42))
        self.assertRaises(OverflowError, getargs_l, VERY_LARGE)

    def test_n(self):
        from _testcapi import getargs_n
        # n returns 'Py_ssize_t', and does range checking
        # (PY_SSIZE_T_MIN ... PY_SSIZE_T_MAX)
        self.assertRaises(TypeError, getargs_n, 3.14)
        self.assertEqual(99, getargs_n(Index()))
        self.assertEqual(0, getargs_n(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_n, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_n(BadIndex2()))
        self.assertEqual(0, getargs_n(BadIndex3()))
        self.assertRaises(TypeError, getargs_n, Int())
        self.assertEqual(0, getargs_n(IntSubclass()))
        self.assertRaises(TypeError, getargs_n, BadInt())
        self.assertRaises(TypeError, getargs_n, BadInt2())
        self.assertEqual(0, getargs_n(BadInt3()))

        self.assertRaises(OverflowError, getargs_n, PY_SSIZE_T_MIN-1)
        self.assertEqual(PY_SSIZE_T_MIN, getargs_n(PY_SSIZE_T_MIN))
        self.assertEqual(PY_SSIZE_T_MAX, getargs_n(PY_SSIZE_T_MAX))
        self.assertRaises(OverflowError, getargs_n, PY_SSIZE_T_MAX+1)

        self.assertEqual(42, getargs_n(42))
        self.assertRaises(OverflowError, getargs_n, VERY_LARGE)


class LongLong_TestCase(unittest.TestCase):
    def test_L(self):
        from _testcapi import getargs_L
        # L returns 'long long', and does range checking (LLONG_MIN
        # ... LLONG_MAX)
        self.assertRaises(TypeError, getargs_L, 3.14)
        self.assertRaises(TypeError, getargs_L, "Hello")
        self.assertEqual(99, getargs_L(Index()))
        self.assertEqual(0, getargs_L(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_L, BadIndex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(1, getargs_L(BadIndex2()))
        self.assertEqual(0, getargs_L(BadIndex3()))
        self.assertRaises(TypeError, getargs_L, Int())
        self.assertEqual(0, getargs_L(IntSubclass()))
        self.assertRaises(TypeError, getargs_L, BadInt())
        self.assertRaises(TypeError, getargs_L, BadInt2())
        self.assertEqual(0, getargs_L(BadInt3()))

        self.assertRaises(OverflowError, getargs_L, LLONG_MIN-1)
        self.assertEqual(LLONG_MIN, getargs_L(LLONG_MIN))
        self.assertEqual(LLONG_MAX, getargs_L(LLONG_MAX))
        self.assertRaises(OverflowError, getargs_L, LLONG_MAX+1)

        self.assertEqual(42, getargs_L(42))
        self.assertRaises(OverflowError, getargs_L, VERY_LARGE)

    def test_K(self):
        from _testcapi import getargs_K
        # K return 'unsigned long long', no range checking
        self.assertRaises(TypeError, getargs_K, 3.14)
        self.assertRaises(TypeError, getargs_K, Index())
        self.assertEqual(0, getargs_K(IndexIntSubclass()))
        self.assertRaises(TypeError, getargs_K, BadIndex())
        self.assertRaises(TypeError, getargs_K, BadIndex2())
        self.assertEqual(0, getargs_K(BadIndex3()))
        self.assertRaises(TypeError, getargs_K, Int())
        self.assertEqual(0, getargs_K(IntSubclass()))
        self.assertRaises(TypeError, getargs_K, BadInt())
        self.assertRaises(TypeError, getargs_K, BadInt2())
        self.assertEqual(0, getargs_K(BadInt3()))

        self.assertEqual(ULLONG_MAX, getargs_K(ULLONG_MAX))
        self.assertEqual(0, getargs_K(0))
        self.assertEqual(0, getargs_K(ULLONG_MAX+1))

        self.assertEqual(42, getargs_K(42))

        self.assertEqual(VERY_LARGE & ULLONG_MAX, getargs_K(VERY_LARGE))


class Float_TestCase(unittest.TestCase):
    def assertEqualWithSign(self, actual, expected):
        self.assertEqual(actual, expected)
        self.assertEqual(math.copysign(1, actual), math.copysign(1, expected))

    def test_f(self):
        from _testcapi import getargs_f
        self.assertEqual(getargs_f(4.25), 4.25)
        self.assertEqual(getargs_f(4), 4.0)
        self.assertRaises(TypeError, getargs_f, 4.25+0j)
        self.assertEqual(getargs_f(Float()), 4.25)
        self.assertEqual(getargs_f(FloatSubclass(7.5)), 7.5)
        self.assertEqual(getargs_f(FloatSubclass2(7.5)), 7.5)
        self.assertRaises(TypeError, getargs_f, BadFloat())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_f(BadFloat2()), 4.25)
        self.assertEqual(getargs_f(BadFloat3(7.5)), 7.5)
        self.assertEqual(getargs_f(Index()), 99.0)
        self.assertRaises(TypeError, getargs_f, Int())

        for x in (FLT_MIN, -FLT_MIN, FLT_MAX, -FLT_MAX, INF, -INF):
            self.assertEqual(getargs_f(x), x)
        if FLT_MAX < DBL_MAX:
            self.assertEqual(getargs_f(DBL_MAX), INF)
            self.assertEqual(getargs_f(-DBL_MAX), -INF)
        if FLT_MIN > DBL_MIN:
            self.assertEqualWithSign(getargs_f(DBL_MIN), 0.0)
            self.assertEqualWithSign(getargs_f(-DBL_MIN), -0.0)
        self.assertEqualWithSign(getargs_f(0.0), 0.0)
        self.assertEqualWithSign(getargs_f(-0.0), -0.0)
        r = getargs_f(NAN)
        self.assertNotEqual(r, r)

    @support.requires_IEEE_754
    def test_f_rounding(self):
        from _testcapi import getargs_f
        self.assertEqual(getargs_f(3.40282356e38), FLT_MAX)
        self.assertEqual(getargs_f(-3.40282356e38), -FLT_MAX)

    def test_d(self):
        from _testcapi import getargs_d
        self.assertEqual(getargs_d(4.25), 4.25)
        self.assertEqual(getargs_d(4), 4.0)
        self.assertRaises(TypeError, getargs_d, 4.25+0j)
        self.assertEqual(getargs_d(Float()), 4.25)
        self.assertEqual(getargs_d(FloatSubclass(7.5)), 7.5)
        self.assertEqual(getargs_d(FloatSubclass2(7.5)), 7.5)
        self.assertRaises(TypeError, getargs_d, BadFloat())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_d(BadFloat2()), 4.25)
        self.assertEqual(getargs_d(BadFloat3(7.5)), 7.5)
        self.assertEqual(getargs_d(Index()), 99.0)
        self.assertRaises(TypeError, getargs_d, Int())

        for x in (DBL_MIN, -DBL_MIN, DBL_MAX, -DBL_MAX, INF, -INF):
            self.assertEqual(getargs_d(x), x)
        self.assertRaises(OverflowError, getargs_d, 1<<DBL_MAX_EXP)
        self.assertRaises(OverflowError, getargs_d, -1<<DBL_MAX_EXP)
        self.assertEqualWithSign(getargs_d(0.0), 0.0)
        self.assertEqualWithSign(getargs_d(-0.0), -0.0)
        r = getargs_d(NAN)
        self.assertNotEqual(r, r)

    def test_D(self):
        from _testcapi import getargs_D
        self.assertEqual(getargs_D(4.25+0.5j), 4.25+0.5j)
        self.assertEqual(getargs_D(4.25), 4.25+0j)
        self.assertEqual(getargs_D(4), 4.0+0j)
        self.assertEqual(getargs_D(Complex()), 4.25+0.5j)
        self.assertEqual(getargs_D(ComplexSubclass(7.5+0.25j)), 7.5+0.25j)
        self.assertEqual(getargs_D(ComplexSubclass2(7.5+0.25j)), 7.5+0.25j)
        self.assertRaises(TypeError, getargs_D, BadComplex())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_D(BadComplex2()), 4.25+0.5j)
        self.assertEqual(getargs_D(BadComplex3(7.5+0.25j)), 7.5+0.25j)
        self.assertEqual(getargs_D(Index()), 99.0+0j)
        self.assertRaises(TypeError, getargs_D, Int())

        for x in (DBL_MIN, -DBL_MIN, DBL_MAX, -DBL_MAX, INF, -INF):
            c = complex(x, 1.0)
            self.assertEqual(getargs_D(c), c)
            c = complex(1.0, x)
            self.assertEqual(getargs_D(c), c)
        self.assertEqualWithSign(getargs_D(complex(0.0, 1.0)).real, 0.0)
        self.assertEqualWithSign(getargs_D(complex(-0.0, 1.0)).real, -0.0)
        self.assertEqualWithSign(getargs_D(complex(1.0, 0.0)).imag, 0.0)
        self.assertEqualWithSign(getargs_D(complex(1.0, -0.0)).imag, -0.0)


class Paradox:
    "This statement is false."
    def __bool__(self):
        raise NotImplementedError

class Boolean_TestCase(unittest.TestCase):
    def test_p(self):
        from _testcapi import getargs_p
        self.assertEqual(0, getargs_p(False))
        self.assertEqual(0, getargs_p(None))
        self.assertEqual(0, getargs_p(0))
        self.assertEqual(0, getargs_p(0.0))
        self.assertEqual(0, getargs_p(0j))
        self.assertEqual(0, getargs_p(''))
        self.assertEqual(0, getargs_p(()))
        self.assertEqual(0, getargs_p([]))
        self.assertEqual(0, getargs_p({}))

        self.assertEqual(1, getargs_p(True))
        self.assertEqual(1, getargs_p(1))
        self.assertEqual(1, getargs_p(1.0))
        self.assertEqual(1, getargs_p(1j))
        self.assertEqual(1, getargs_p('x'))
        self.assertEqual(1, getargs_p((1,)))
        self.assertEqual(1, getargs_p([1]))
        self.assertEqual(1, getargs_p({1:2}))
        self.assertEqual(1, getargs_p(unittest.TestCase))

        self.assertRaises(NotImplementedError, getargs_p, Paradox())


class Tuple_TestCase(unittest.TestCase):
    def test_args(self):
        from _testcapi import get_args

        ret = get_args(1, 2)
        self.assertEqual(ret, (1, 2))
        self.assertIs(type(ret), tuple)

        ret = get_args(1, *(2, 3))
        self.assertEqual(ret, (1, 2, 3))
        self.assertIs(type(ret), tuple)

        ret = get_args(*[1, 2])
        self.assertEqual(ret, (1, 2))
        self.assertIs(type(ret), tuple)

        ret = get_args(*TupleSubclass([1, 2]))
        self.assertEqual(ret, (1, 2))
        self.assertIs(type(ret), tuple)

        ret = get_args()
        self.assertIn(ret, ((), None))
        self.assertIn(type(ret), (tuple, type(None)))

        ret = get_args(*())
        self.assertIn(ret, ((), None))
        self.assertIn(type(ret), (tuple, type(None)))

    def test_tuple(self):
        from _testcapi import getargs_tuple

        ret = getargs_tuple(1, (2, 3))
        self.assertEqual(ret, (1,2,3))

        # make sure invalid tuple arguments are handled correctly
        class seq:
            def __len__(self):
                return 2
            def __getitem__(self, n):
                raise ValueError
        self.assertRaises(TypeError, getargs_tuple, 1, seq())

class Keywords_TestCase(unittest.TestCase):
    def test_kwargs(self):
        from _testcapi import get_kwargs

        ret = get_kwargs(a=1, b=2)
        self.assertEqual(ret, {'a': 1, 'b': 2})
        self.assertIs(type(ret), dict)

        ret = get_kwargs(a=1, **{'b': 2, 'c': 3})
        self.assertEqual(ret, {'a': 1, 'b': 2, 'c': 3})
        self.assertIs(type(ret), dict)

        ret = get_kwargs(**DictSubclass({'a': 1, 'b': 2}))
        self.assertEqual(ret, {'a': 1, 'b': 2})
        self.assertIs(type(ret), dict)

        ret = get_kwargs()
        self.assertIn(ret, ({}, None))
        self.assertIn(type(ret), (dict, type(None)))

        ret = get_kwargs(**{})
        self.assertIn(ret, ({}, None))
        self.assertIn(type(ret), (dict, type(None)))

    def test_positional_args(self):
        # using all positional args
        self.assertEqual(
            getargs_keywords((1,2), 3, (4,(5,6)), (7,8,9), 10),
            (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
            )

    def test_mixed_args(self):
        # positional and keyword args
        self.assertEqual(
            getargs_keywords((1,2), 3, (4,(5,6)), arg4=(7,8,9), arg5=10),
            (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
            )

    def test_keyword_args(self):
        # all keywords
        self.assertEqual(
            getargs_keywords(arg1=(1,2), arg2=3, arg3=(4,(5,6)), arg4=(7,8,9), arg5=10),
            (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
            )

    def test_optional_args(self):
        # missing optional keyword args, skipping tuples
        self.assertEqual(
            getargs_keywords(arg1=(1,2), arg2=3, arg5=10),
            (1, 2, 3, -1, -1, -1, -1, -1, -1, 10)
            )

    def test_required_args(self):
        # required arg missing
        try:
            getargs_keywords(arg1=(1,2))
        except TypeError as err:
            self.assertEqual(
                str(err), "function missing required argument 'arg2' (pos 2)")
        else:
            self.fail('TypeError should have been raised')

    def test_too_many_args(self):
        try:
            getargs_keywords((1,2),3,(4,(5,6)),(7,8,9),10,111)
        except TypeError as err:
            self.assertEqual(str(err), "function takes at most 5 arguments (6 given)")
        else:
            self.fail('TypeError should have been raised')

    def test_invalid_keyword(self):
        # extraneous keyword arg
        try:
            getargs_keywords((1,2),3,arg5=10,arg666=666)
        except TypeError as err:
            self.assertEqual(str(err), "'arg666' is an invalid keyword argument for this function")
        else:
            self.fail('TypeError should have been raised')

    def test_surrogate_keyword(self):
        try:
            getargs_keywords((1,2), 3, (4,(5,6)), (7,8,9), **{'\uDC80': 10})
        except TypeError as err:
            self.assertEqual(str(err), "'\udc80' is an invalid keyword argument for this function")
        else:
            self.fail('TypeError should have been raised')

class KeywordOnly_TestCase(unittest.TestCase):
    def test_positional_args(self):
        # using all possible positional args
        self.assertEqual(
            getargs_keyword_only(1, 2),
            (1, 2, -1)
            )

    def test_mixed_args(self):
        # positional and keyword args
        self.assertEqual(
            getargs_keyword_only(1, 2, keyword_only=3),
            (1, 2, 3)
            )

    def test_keyword_args(self):
        # all keywords
        self.assertEqual(
            getargs_keyword_only(required=1, optional=2, keyword_only=3),
            (1, 2, 3)
            )

    def test_optional_args(self):
        # missing optional keyword args, skipping tuples
        self.assertEqual(
            getargs_keyword_only(required=1, optional=2),
            (1, 2, -1)
            )
        self.assertEqual(
            getargs_keyword_only(required=1, keyword_only=3),
            (1, -1, 3)
            )

    def test_required_args(self):
        self.assertEqual(
            getargs_keyword_only(1),
            (1, -1, -1)
            )
        self.assertEqual(
            getargs_keyword_only(required=1),
            (1, -1, -1)
            )
        # required arg missing
        with self.assertRaisesRegex(TypeError,
            r"function missing required argument 'required' \(pos 1\)"):
            getargs_keyword_only(optional=2)

        with self.assertRaisesRegex(TypeError,
            r"function missing required argument 'required' \(pos 1\)"):
            getargs_keyword_only(keyword_only=3)

    def test_too_many_args(self):
        with self.assertRaisesRegex(TypeError,
            r"function takes at most 2 positional arguments \(3 given\)"):
            getargs_keyword_only(1, 2, 3)

        with self.assertRaisesRegex(TypeError,
            r"function takes at most 3 arguments \(4 given\)"):
            getargs_keyword_only(1, 2, 3, keyword_only=5)

    def test_invalid_keyword(self):
        # extraneous keyword arg
        with self.assertRaisesRegex(TypeError,
            "'monster' is an invalid keyword argument for this function"):
            getargs_keyword_only(1, 2, monster=666)

    def test_surrogate_keyword(self):
        with self.assertRaisesRegex(TypeError,
            "'\udc80' is an invalid keyword argument for this function"):
            getargs_keyword_only(1, 2, **{'\uDC80': 10})

    def test_weird_str_subclass(self):
        class BadStr(str):
            def __eq__(self, other):
                return True
            def __hash__(self):
                # Guaranteed different hash
                return str.__hash__(self) ^ 3
        with self.assertRaisesRegex(TypeError,
            "invalid keyword argument for this function"):
            getargs_keyword_only(1, 2, **{BadStr("keyword_only"): 3})
        with self.assertRaisesRegex(TypeError,
            "invalid keyword argument for this function"):
            getargs_keyword_only(1, 2, **{BadStr("monster"): 666})

    def test_weird_str_subclass2(self):
        class BadStr(str):
            def __eq__(self, other):
                return False
            def __hash__(self):
                return str.__hash__(self)
        with self.assertRaisesRegex(TypeError,
            "invalid keyword argument for this function"):
            getargs_keyword_only(1, 2, **{BadStr("keyword_only"): 3})
        with self.assertRaisesRegex(TypeError,
            "invalid keyword argument for this function"):
            getargs_keyword_only(1, 2, **{BadStr("monster"): 666})


class PositionalOnlyAndKeywords_TestCase(unittest.TestCase):
    from _testcapi import getargs_positional_only_and_keywords as getargs

    def test_positional_args(self):
        # using all possible positional args
        self.assertEqual(self.getargs(1, 2, 3), (1, 2, 3))

    def test_mixed_args(self):
        # positional and keyword args
        self.assertEqual(self.getargs(1, 2, keyword=3), (1, 2, 3))

    def test_optional_args(self):
        # missing optional args
        self.assertEqual(self.getargs(1, 2), (1, 2, -1))
        self.assertEqual(self.getargs(1, keyword=3), (1, -1, 3))

    def test_required_args(self):
        self.assertEqual(self.getargs(1), (1, -1, -1))
        # required positional arg missing
        with self.assertRaisesRegex(TypeError,
            r"function takes at least 1 positional argument \(0 given\)"):
            self.getargs()

        with self.assertRaisesRegex(TypeError,
            r"function takes at least 1 positional argument \(0 given\)"):
            self.getargs(keyword=3)

    def test_empty_keyword(self):
        with self.assertRaisesRegex(TypeError,
            "'' is an invalid keyword argument for this function"):
            self.getargs(1, 2, **{'': 666})


class Bytes_TestCase(unittest.TestCase):
    def test_c(self):
        from _testcapi import getargs_c
        self.assertRaises(TypeError, getargs_c, b'abc')  # len > 1
        self.assertEqual(getargs_c(b'a'), 97)
        self.assertEqual(getargs_c(bytearray(b'a')), 97)
        self.assertRaises(TypeError, getargs_c, memoryview(b'a'))
        self.assertRaises(TypeError, getargs_c, 's')
        self.assertRaises(TypeError, getargs_c, 97)
        self.assertRaises(TypeError, getargs_c, None)

    def test_y(self):
        from _testcapi import getargs_y
        self.assertRaises(TypeError, getargs_y, 'abc\xe9')
        self.assertEqual(getargs_y(b'bytes'), b'bytes')
        self.assertRaises(ValueError, getargs_y, b'nul:\0')
        self.assertRaises(TypeError, getargs_y, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_y, memoryview(b'memoryview'))
        self.assertRaises(TypeError, getargs_y, None)

    def test_y_star(self):
        from _testcapi import getargs_y_star
        self.assertRaises(TypeError, getargs_y_star, 'abc\xe9')
        self.assertEqual(getargs_y_star(b'bytes'), b'bytes')
        self.assertEqual(getargs_y_star(b'nul:\0'), b'nul:\0')
        self.assertEqual(getargs_y_star(bytearray(b'bytearray')), b'bytearray')
        self.assertEqual(getargs_y_star(memoryview(b'memoryview')), b'memoryview')
        self.assertRaises(TypeError, getargs_y_star, None)

    def test_y_hash(self):
        from _testcapi import getargs_y_hash
        self.assertRaises(TypeError, getargs_y_hash, 'abc\xe9')
        self.assertEqual(getargs_y_hash(b'bytes'), b'bytes')
        self.assertEqual(getargs_y_hash(b'nul:\0'), b'nul:\0')
        self.assertRaises(TypeError, getargs_y_hash, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_y_hash, memoryview(b'memoryview'))
        self.assertRaises(TypeError, getargs_y_hash, None)

    def test_w_star(self):
        # getargs_w_star() modifies first and last byte
        from _testcapi import getargs_w_star
        self.assertRaises(TypeError, getargs_w_star, 'abc\xe9')
        self.assertRaises(TypeError, getargs_w_star, b'bytes')
        self.assertRaises(TypeError, getargs_w_star, b'nul:\0')
        self.assertRaises(TypeError, getargs_w_star, memoryview(b'bytes'))
        buf = bytearray(b'bytearray')
        self.assertEqual(getargs_w_star(buf), b'[ytearra]')
        self.assertEqual(buf, bytearray(b'[ytearra]'))
        buf = bytearray(b'memoryview')
        self.assertEqual(getargs_w_star(memoryview(buf)), b'[emoryvie]')
        self.assertEqual(buf, bytearray(b'[emoryvie]'))
        self.assertRaises(TypeError, getargs_w_star, None)


class String_TestCase(unittest.TestCase):
    def test_C(self):
        from _testcapi import getargs_C
        self.assertRaises(TypeError, getargs_C, 'abc')  # len > 1
        self.assertEqual(getargs_C('a'), 97)
        self.assertEqual(getargs_C('\u20ac'), 0x20ac)
        self.assertEqual(getargs_C('\U0001f40d'), 0x1f40d)
        self.assertRaises(TypeError, getargs_C, b'a')
        self.assertRaises(TypeError, getargs_C, bytearray(b'a'))
        self.assertRaises(TypeError, getargs_C, memoryview(b'a'))
        self.assertRaises(TypeError, getargs_C, 97)
        self.assertRaises(TypeError, getargs_C, None)

    def test_s(self):
        from _testcapi import getargs_s
        self.assertEqual(getargs_s('abc\xe9'), b'abc\xc3\xa9')
        self.assertRaises(ValueError, getargs_s, 'nul:\0')
        self.assertRaises(TypeError, getargs_s, b'bytes')
        self.assertRaises(TypeError, getargs_s, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_s, memoryview(b'memoryview'))
        self.assertRaises(TypeError, getargs_s, None)

    def test_s_star(self):
        from _testcapi import getargs_s_star
        self.assertEqual(getargs_s_star('abc\xe9'), b'abc\xc3\xa9')
        self.assertEqual(getargs_s_star('nul:\0'), b'nul:\0')
        self.assertEqual(getargs_s_star(b'bytes'), b'bytes')
        self.assertEqual(getargs_s_star(bytearray(b'bytearray')), b'bytearray')
        self.assertEqual(getargs_s_star(memoryview(b'memoryview')), b'memoryview')
        self.assertRaises(TypeError, getargs_s_star, None)

    def test_s_hash(self):
        from _testcapi import getargs_s_hash
        self.assertEqual(getargs_s_hash('abc\xe9'), b'abc\xc3\xa9')
        self.assertEqual(getargs_s_hash('nul:\0'), b'nul:\0')
        self.assertEqual(getargs_s_hash(b'bytes'), b'bytes')
        self.assertRaises(TypeError, getargs_s_hash, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_s_hash, memoryview(b'memoryview'))
        self.assertRaises(TypeError, getargs_s_hash, None)

    def test_s_hash_int(self):
        # "s#" without PY_SSIZE_T_CLEAN defined.
        from _testcapi import getargs_s_hash_int
        from _testcapi import getargs_s_hash_int2
        buf = bytearray([1, 2])
        self.assertRaises(SystemError, getargs_s_hash_int, buf, "abc")
        self.assertRaises(SystemError, getargs_s_hash_int, buf, x=42)
        self.assertRaises(SystemError, getargs_s_hash_int, buf, x="abc")
        self.assertRaises(SystemError, getargs_s_hash_int2, buf, ("abc",))
        self.assertRaises(SystemError, getargs_s_hash_int2, buf, x=42)
        self.assertRaises(SystemError, getargs_s_hash_int2, buf, x="abc")
        buf.append(3)  # still mutable -- not locked by a buffer export
        # getargs_s_hash_int(buf) may not raise SystemError because skipitem()
        # is not called. But it is an implementation detail.
        # getargs_s_hash_int(buf)
        # getargs_s_hash_int2(buf)

    def test_z(self):
        from _testcapi import getargs_z
        self.assertEqual(getargs_z('abc\xe9'), b'abc\xc3\xa9')
        self.assertRaises(ValueError, getargs_z, 'nul:\0')
        self.assertRaises(TypeError, getargs_z, b'bytes')
        self.assertRaises(TypeError, getargs_z, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_z, memoryview(b'memoryview'))
        self.assertIsNone(getargs_z(None))

    def test_z_star(self):
        from _testcapi import getargs_z_star
        self.assertEqual(getargs_z_star('abc\xe9'), b'abc\xc3\xa9')
        self.assertEqual(getargs_z_star('nul:\0'), b'nul:\0')
        self.assertEqual(getargs_z_star(b'bytes'), b'bytes')
        self.assertEqual(getargs_z_star(bytearray(b'bytearray')), b'bytearray')
        self.assertEqual(getargs_z_star(memoryview(b'memoryview')), b'memoryview')
        self.assertIsNone(getargs_z_star(None))

    def test_z_hash(self):
        from _testcapi import getargs_z_hash
        self.assertEqual(getargs_z_hash('abc\xe9'), b'abc\xc3\xa9')
        self.assertEqual(getargs_z_hash('nul:\0'), b'nul:\0')
        self.assertEqual(getargs_z_hash(b'bytes'), b'bytes')
        self.assertRaises(TypeError, getargs_z_hash, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_z_hash, memoryview(b'memoryview'))
        self.assertIsNone(getargs_z_hash(None))

    def test_es(self):
        from _testcapi import getargs_es
        self.assertEqual(getargs_es('abc\xe9'), b'abc\xc3\xa9')
        self.assertEqual(getargs_es('abc\xe9', 'latin1'), b'abc\xe9')
        self.assertRaises(UnicodeEncodeError, getargs_es, 'abc\xe9', 'ascii')
        self.assertRaises(LookupError, getargs_es, 'abc\xe9', 'spam')
        self.assertRaises(TypeError, getargs_es, b'bytes', 'latin1')
        self.assertRaises(TypeError, getargs_es, bytearray(b'bytearray'), 'latin1')
        self.assertRaises(TypeError, getargs_es, memoryview(b'memoryview'), 'latin1')
        self.assertRaises(TypeError, getargs_es, None, 'latin1')
        self.assertRaises(TypeError, getargs_es, 'nul:\0', 'latin1')

    def test_et(self):
        from _testcapi import getargs_et
        self.assertEqual(getargs_et('abc\xe9'), b'abc\xc3\xa9')
        self.assertEqual(getargs_et('abc\xe9', 'latin1'), b'abc\xe9')
        self.assertRaises(UnicodeEncodeError, getargs_et, 'abc\xe9', 'ascii')
        self.assertRaises(LookupError, getargs_et, 'abc\xe9', 'spam')
        self.assertEqual(getargs_et(b'bytes', 'latin1'), b'bytes')
        self.assertEqual(getargs_et(bytearray(b'bytearray'), 'latin1'), b'bytearray')
        self.assertRaises(TypeError, getargs_et, memoryview(b'memoryview'), 'latin1')
        self.assertRaises(TypeError, getargs_et, None, 'latin1')
        self.assertRaises(TypeError, getargs_et, 'nul:\0', 'latin1')
        self.assertRaises(TypeError, getargs_et, b'nul:\0', 'latin1')
        self.assertRaises(TypeError, getargs_et, bytearray(b'nul:\0'), 'latin1')

    def test_es_hash(self):
        from _testcapi import getargs_es_hash
        self.assertEqual(getargs_es_hash('abc\xe9'), b'abc\xc3\xa9')
        self.assertEqual(getargs_es_hash('abc\xe9', 'latin1'), b'abc\xe9')
        self.assertRaises(UnicodeEncodeError, getargs_es_hash, 'abc\xe9', 'ascii')
        self.assertRaises(LookupError, getargs_es_hash, 'abc\xe9', 'spam')
        self.assertRaises(TypeError, getargs_es_hash, b'bytes', 'latin1')
        self.assertRaises(TypeError, getargs_es_hash, bytearray(b'bytearray'), 'latin1')
        self.assertRaises(TypeError, getargs_es_hash, memoryview(b'memoryview'), 'latin1')
        self.assertRaises(TypeError, getargs_es_hash, None, 'latin1')
        self.assertEqual(getargs_es_hash('nul:\0', 'latin1'), b'nul:\0')

        buf = bytearray(b'x'*8)
        self.assertEqual(getargs_es_hash('abc\xe9', 'latin1', buf), b'abc\xe9')
        self.assertEqual(buf, bytearray(b'abc\xe9\x00xxx'))
        buf = bytearray(b'x'*5)
        self.assertEqual(getargs_es_hash('abc\xe9', 'latin1', buf), b'abc\xe9')
        self.assertEqual(buf, bytearray(b'abc\xe9\x00'))
        buf = bytearray(b'x'*4)
        self.assertRaises(ValueError, getargs_es_hash, 'abc\xe9', 'latin1', buf)
        self.assertEqual(buf, bytearray(b'x'*4))
        buf = bytearray()
        self.assertRaises(ValueError, getargs_es_hash, 'abc\xe9', 'latin1', buf)

    def test_et_hash(self):
        from _testcapi import getargs_et_hash
        self.assertEqual(getargs_et_hash('abc\xe9'), b'abc\xc3\xa9')
        self.assertEqual(getargs_et_hash('abc\xe9', 'latin1'), b'abc\xe9')
        self.assertRaises(UnicodeEncodeError, getargs_et_hash, 'abc\xe9', 'ascii')
        self.assertRaises(LookupError, getargs_et_hash, 'abc\xe9', 'spam')
        self.assertEqual(getargs_et_hash(b'bytes', 'latin1'), b'bytes')
        self.assertEqual(getargs_et_hash(bytearray(b'bytearray'), 'latin1'), b'bytearray')
        self.assertRaises(TypeError, getargs_et_hash, memoryview(b'memoryview'), 'latin1')
        self.assertRaises(TypeError, getargs_et_hash, None, 'latin1')
        self.assertEqual(getargs_et_hash('nul:\0', 'latin1'), b'nul:\0')
        self.assertEqual(getargs_et_hash(b'nul:\0', 'latin1'), b'nul:\0')
        self.assertEqual(getargs_et_hash(bytearray(b'nul:\0'), 'latin1'), b'nul:\0')

        buf = bytearray(b'x'*8)
        self.assertEqual(getargs_et_hash('abc\xe9', 'latin1', buf), b'abc\xe9')
        self.assertEqual(buf, bytearray(b'abc\xe9\x00xxx'))
        buf = bytearray(b'x'*5)
        self.assertEqual(getargs_et_hash('abc\xe9', 'latin1', buf), b'abc\xe9')
        self.assertEqual(buf, bytearray(b'abc\xe9\x00'))
        buf = bytearray(b'x'*4)
        self.assertRaises(ValueError, getargs_et_hash, 'abc\xe9', 'latin1', buf)
        self.assertEqual(buf, bytearray(b'x'*4))
        buf = bytearray()
        self.assertRaises(ValueError, getargs_et_hash, 'abc\xe9', 'latin1', buf)

    @support.requires_legacy_unicode_capi
    def test_u(self):
        from _testcapi import getargs_u
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_u('abc\xe9'), 'abc\xe9')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(ValueError, getargs_u, 'nul:\0')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_u, b'bytes')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_u, bytearray(b'bytearray'))
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_u, memoryview(b'memoryview'))
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_u, None)
        with warnings.catch_warnings():
            warnings.simplefilter('error', DeprecationWarning)
            self.assertRaises(DeprecationWarning, getargs_u, 'abc\xe9')

    @support.requires_legacy_unicode_capi
    def test_u_hash(self):
        from _testcapi import getargs_u_hash
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_u_hash('abc\xe9'), 'abc\xe9')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_u_hash('nul:\0'), 'nul:\0')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_u_hash, b'bytes')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_u_hash, bytearray(b'bytearray'))
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_u_hash, memoryview(b'memoryview'))
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_u_hash, None)
        with warnings.catch_warnings():
            warnings.simplefilter('error', DeprecationWarning)
            self.assertRaises(DeprecationWarning, getargs_u_hash, 'abc\xe9')

    @support.requires_legacy_unicode_capi
    def test_Z(self):
        from _testcapi import getargs_Z
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_Z('abc\xe9'), 'abc\xe9')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(ValueError, getargs_Z, 'nul:\0')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_Z, b'bytes')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_Z, bytearray(b'bytearray'))
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_Z, memoryview(b'memoryview'))
        with self.assertWarns(DeprecationWarning):
            self.assertIsNone(getargs_Z(None))
        with warnings.catch_warnings():
            warnings.simplefilter('error', DeprecationWarning)
            self.assertRaises(DeprecationWarning, getargs_Z, 'abc\xe9')

    @support.requires_legacy_unicode_capi
    def test_Z_hash(self):
        from _testcapi import getargs_Z_hash
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_Z_hash('abc\xe9'), 'abc\xe9')
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(getargs_Z_hash('nul:\0'), 'nul:\0')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_Z_hash, b'bytes')
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_Z_hash, bytearray(b'bytearray'))
        with self.assertWarns(DeprecationWarning):
            self.assertRaises(TypeError, getargs_Z_hash, memoryview(b'memoryview'))
        with self.assertWarns(DeprecationWarning):
            self.assertIsNone(getargs_Z_hash(None))
        with warnings.catch_warnings():
            warnings.simplefilter('error', DeprecationWarning)
            self.assertRaises(DeprecationWarning, getargs_Z_hash, 'abc\xe9')

    def test_gh_99240_clear_args(self):
        from _testcapi import gh_99240_clear_args
        self.assertRaises(TypeError, gh_99240_clear_args, 'a', '\0b')


class Object_TestCase(unittest.TestCase):
    def test_S(self):
        from _testcapi import getargs_S
        obj = b'bytes'
        self.assertIs(getargs_S(obj), obj)
        self.assertRaises(TypeError, getargs_S, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_S, 'str')
        self.assertRaises(TypeError, getargs_S, None)
        self.assertRaises(TypeError, getargs_S, memoryview(obj))

    def test_Y(self):
        from _testcapi import getargs_Y
        obj = bytearray(b'bytearray')
        self.assertIs(getargs_Y(obj), obj)
        self.assertRaises(TypeError, getargs_Y, b'bytes')
        self.assertRaises(TypeError, getargs_Y, 'str')
        self.assertRaises(TypeError, getargs_Y, None)
        self.assertRaises(TypeError, getargs_Y, memoryview(obj))

    def test_U(self):
        from _testcapi import getargs_U
        obj = 'str'
        self.assertIs(getargs_U(obj), obj)
        self.assertRaises(TypeError, getargs_U, b'bytes')
        self.assertRaises(TypeError, getargs_U, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_U, None)


# Bug #6012
class Test6012(unittest.TestCase):
    def test(self):
        self.assertEqual(_testcapi.argparsing("Hello", "World"), 1)


class SkipitemTest(unittest.TestCase):

    # u, and Z raises DeprecationWarning
    @warnings_helper.ignore_warnings(category=DeprecationWarning)
    def test_skipitem(self):
        """
        If this test failed, you probably added a new "format unit"
        in Python/getargs.c, but neglected to update our poor friend
        skipitem() in the same file.  (If so, shame on you!)

        With a few exceptions**, this function brute-force tests all
        printable ASCII*** characters (32 to 126 inclusive) as format units,
        checking to see that PyArg_ParseTupleAndKeywords() return consistent
        errors both when the unit is attempted to be used and when it is
        skipped.  If the format unit doesn't exist, we'll get one of two
        specific error messages (one for used, one for skipped); if it does
        exist we *won't* get that error--we'll get either no error or some
        other error.  If we get the specific "does not exist" error for one
        test and not for the other, there's a mismatch, and the test fails.

           ** Some format units have special funny semantics and it would
              be difficult to accommodate them here.  Since these are all
              well-established and properly skipped in skipitem() we can
              get away with not testing them--this test is really intended
              to catch *new* format units.

          *** Python C source files must be ASCII.  Therefore it's impossible
              to have non-ASCII format units.

        """
        empty_tuple = ()
        tuple_1 = (0,)
        dict_b = {'b':1}
        keywords = ["a", "b"]

        for i in range(32, 127):
            c = chr(i)

            # skip parentheses, the error reporting is inconsistent about them
            # skip 'e', it's always a two-character code
            # skip '|' and '$', they don't represent arguments anyway
            if c in '()e|$':
                continue

            # test the format unit when not skipped
            format = c + "i"
            try:
                _testcapi.parse_tuple_and_keywords(tuple_1, dict_b,
                    format, keywords)
                when_not_skipped = False
            except SystemError as e:
                s = "argument 1 (impossible<bad format char>)"
                when_not_skipped = (str(e) == s)
            except TypeError:
                when_not_skipped = False

            # test the format unit when skipped
            optional_format = "|" + format
            try:
                _testcapi.parse_tuple_and_keywords(empty_tuple, dict_b,
                    optional_format, keywords)
                when_skipped = False
            except SystemError as e:
                s = "impossible<bad format char>: '{}'".format(format)
                when_skipped = (str(e) == s)

            message = ("test_skipitem_parity: "
                "detected mismatch between convertsimple and skipitem "
                "for format unit '{}' ({}), not skipped {}, skipped {}".format(
                    c, i, when_skipped, when_not_skipped))
            self.assertIs(when_skipped, when_not_skipped, message)

    def test_skipitem_with_suffix(self):
        parse = _testcapi.parse_tuple_and_keywords
        empty_tuple = ()
        tuple_1 = (0,)
        dict_b = {'b':1}
        keywords = ["a", "b"]

        supported = ('s#', 's*', 'z#', 'z*', 'u#', 'Z#', 'y#', 'y*', 'w#', 'w*')
        for c in string.ascii_letters:
            for c2 in '#*':
                f = c + c2
                with self.subTest(format=f):
                    optional_format = "|" + f + "i"
                    if f in supported:
                        parse(empty_tuple, dict_b, optional_format, keywords)
                    else:
                        with self.assertRaisesRegex(SystemError,
                                    'impossible<bad format char>'):
                            parse(empty_tuple, dict_b, optional_format, keywords)

        for c in map(chr, range(32, 128)):
            f = 'e' + c
            optional_format = "|" + f + "i"
            with self.subTest(format=f):
                if c in 'st':
                    parse(empty_tuple, dict_b, optional_format, keywords)
                else:
                    with self.assertRaisesRegex(SystemError,
                                'impossible<bad format char>'):
                        parse(empty_tuple, dict_b, optional_format, keywords)


class ParseTupleAndKeywords_Test(unittest.TestCase):

    def test_parse_tuple_and_keywords(self):
        # Test handling errors in the parse_tuple_and_keywords helper itself
        self.assertRaises(TypeError, _testcapi.parse_tuple_and_keywords,
                          (), {}, 42, [])
        self.assertRaises(ValueError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '', 42)
        self.assertRaises(ValueError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '', [''] * 42)
        self.assertRaises(ValueError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '', [42])

    def test_bad_use(self):
        # Test handling invalid format and keywords in
        # PyArg_ParseTupleAndKeywords()
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '||O', ['a'])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (1, 2), {}, '|O|O', ['a', 'b'])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (), {'a': 1}, '$$O', ['a'])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (), {'a': 1, 'b': 2}, '$O$O', ['a', 'b'])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (), {'a': 1}, '$|O', ['a'])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (), {'a': 1, 'b': 2}, '$O|O', ['a', 'b'])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '|O', ['a', 'b'])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '|OO', ['a'])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '|$O', [''])
        self.assertRaises(SystemError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '|OO', ['a', ''])

    def test_positional_only(self):
        parse = _testcapi.parse_tuple_and_keywords

        parse((1, 2, 3), {}, 'OOO', ['', '', 'a'])
        parse((1, 2), {'a': 3}, 'OOO', ['', '', 'a'])
        with self.assertRaisesRegex(TypeError,
               r'function takes at least 2 positional arguments \(1 given\)'):
            parse((1,), {'a': 3}, 'OOO', ['', '', 'a'])
        parse((1,), {}, 'O|OO', ['', '', 'a'])
        with self.assertRaisesRegex(TypeError,
               r'function takes at least 1 positional argument \(0 given\)'):
            parse((), {}, 'O|OO', ['', '', 'a'])
        parse((1, 2), {'a': 3}, 'OO$O', ['', '', 'a'])
        with self.assertRaisesRegex(TypeError,
               r'function takes exactly 2 positional arguments \(1 given\)'):
            parse((1,), {'a': 3}, 'OO$O', ['', '', 'a'])
        parse((1,), {}, 'O|O$O', ['', '', 'a'])
        with self.assertRaisesRegex(TypeError,
               r'function takes at least 1 positional argument \(0 given\)'):
            parse((), {}, 'O|O$O', ['', '', 'a'])
        with self.assertRaisesRegex(SystemError, r'Empty parameter name after \$'):
            parse((1,), {}, 'O|$OO', ['', '', 'a'])
        with self.assertRaisesRegex(SystemError, 'Empty keyword'):
            parse((1,), {}, 'O|OO', ['', 'a', ''])


class Test_testcapi(unittest.TestCase):
    locals().update((name, getattr(_testcapi, name))
                    for name in dir(_testcapi)
                    if name.startswith('test_') and name.endswith('_code'))

    @warnings_helper.ignore_warnings(category=DeprecationWarning)
    def test_u_code(self):
        _testcapi.test_u_code()

    @warnings_helper.ignore_warnings(category=DeprecationWarning)
    def test_Z_code(self):
        _testcapi.test_Z_code()


if __name__ == "__main__":
    unittest.main()
