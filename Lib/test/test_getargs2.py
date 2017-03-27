import unittest
import math
import sys
from test import support
# Skip this test if the _testcapi module isn't available.
_testcapi = support.import_module('_testcapi')
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


class UnsignedLong_TestCase:
    getargs = None
    MIN = 0
    MAX = 1
    SUPPORT_INT = False

    def test_basic(self):
        self.assertEqual(42, self.getargs(42))
        self.assertEqual(self.MIN, self.getargs(self.MIN))
        self.assertEqual(self.MAX, self.getargs(self.MAX))

    def test_types(self):
        self.assertRaises(TypeError, self.getargs, 3.14)
        self.assertRaises(TypeError, self.getargs, "Hello")
        self.assertEqual(0, self.getargs(IntSubclass()))

    def test_int(self):
        if self.SUPPORT_INT:
            self.assertEqual(99, self.getargs(Int()))
            self.assertRaises(TypeError, self.getargs, BadInt())
            with self.assertWarns(DeprecationWarning):
                self.assertEqual(1, self.getargs(BadInt2()))
        else:
            self.assertRaises(TypeError, self.getargs, Int())
            self.assertRaises(TypeError, self.getargs, BadInt())
            self.assertRaises(TypeError, self.getargs, BadInt2())

        self.assertEqual(0, self.getargs(BadInt3()))

    def test_overflow(self):
        for value in (self.MIN - 1, self.MAX + 1):
            self.assertEqual(value & self.MAX, self.getargs(value))
        self.assertEqual(VERY_LARGE & self.MAX, self.getargs(VERY_LARGE))


class SignedLong_TestCase(UnsignedLong_TestCase):
    MIN = -1
    SUPPORT_INT = True

    def test_overflow(self):
        self.assertRaises(OverflowError, self.getargs, self.MIN - 1)
        self.assertRaises(OverflowError, self.getargs, self.MAX + 1)
        self.assertRaises(OverflowError, self.getargs, VERY_LARGE)


class Long_b(SignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_b
    MIN = 0
    MAX = UCHAR_MAX


class Long_h(SignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_h
    MIN = SHRT_MIN
    MAX = SHRT_MAX


class Long_i(SignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_i
    MIN = INT_MIN
    MAX = INT_MAX


class Long_l(SignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_l
    MIN = LONG_MIN
    MAX = LONG_MAX


class LongLong_L(SignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_L
    MIN = LLONG_MIN
    MAX = LLONG_MAX


class Long_n(SignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_n
    MIN = PY_SSIZE_T_MIN
    MAX = PY_SSIZE_T_MAX
    SUPPORT_INT = False


class Long_B(UnsignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_B
    MAX = UCHAR_MAX
    SUPPORT_INT = True


class Long_H(UnsignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_H
    MAX = USHRT_MAX
    SUPPORT_INT = True


class Long_I(UnsignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_I
    MAX = UINT_MAX
    SUPPORT_INT = True


class Long_k(UnsignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_k
    MAX = ULONG_MAX


class LongLong_K(UnsignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_K
    MAX = ULLONG_MAX


class Long_IntMax_m(SignedLong_TestCase, unittest.TestCase):
    getargs = _testcapi.getargs_m
    MIN = _testcapi.INTMAX_MIN
    MAX = _testcapi.INTMAX_MAX


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
            self.assertEqual(str(err), "Required argument 'arg2' (pos 2) not found")
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
            r"Required argument 'required' \(pos 1\) not found"):
            getargs_keyword_only(optional=2)

        with self.assertRaisesRegex(TypeError,
            r"Required argument 'required' \(pos 1\) not found"):
            getargs_keyword_only(keyword_only=3)

    def test_too_many_args(self):
        with self.assertRaisesRegex(TypeError,
            r"Function takes at most 2 positional arguments \(3 given\)"):
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
            r"Function takes at least 1 positional arguments \(0 given\)"):
            self.getargs()

        with self.assertRaisesRegex(TypeError,
            r"Function takes at least 1 positional arguments \(0 given\)"):
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

    def test_u(self):
        from _testcapi import getargs_u
        self.assertEqual(getargs_u('abc\xe9'), 'abc\xe9')
        self.assertRaises(ValueError, getargs_u, 'nul:\0')
        self.assertRaises(TypeError, getargs_u, b'bytes')
        self.assertRaises(TypeError, getargs_u, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_u, memoryview(b'memoryview'))
        self.assertRaises(TypeError, getargs_u, None)

    def test_u_hash(self):
        from _testcapi import getargs_u_hash
        self.assertEqual(getargs_u_hash('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_u_hash('nul:\0'), 'nul:\0')
        self.assertRaises(TypeError, getargs_u_hash, b'bytes')
        self.assertRaises(TypeError, getargs_u_hash, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_u_hash, memoryview(b'memoryview'))
        self.assertRaises(TypeError, getargs_u_hash, None)

    def test_Z(self):
        from _testcapi import getargs_Z
        self.assertEqual(getargs_Z('abc\xe9'), 'abc\xe9')
        self.assertRaises(ValueError, getargs_Z, 'nul:\0')
        self.assertRaises(TypeError, getargs_Z, b'bytes')
        self.assertRaises(TypeError, getargs_Z, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_Z, memoryview(b'memoryview'))
        self.assertIsNone(getargs_Z(None))

    def test_Z_hash(self):
        from _testcapi import getargs_Z_hash
        self.assertEqual(getargs_Z_hash('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_Z_hash('nul:\0'), 'nul:\0')
        self.assertRaises(TypeError, getargs_Z_hash, b'bytes')
        self.assertRaises(TypeError, getargs_Z_hash, bytearray(b'bytearray'))
        self.assertRaises(TypeError, getargs_Z_hash, memoryview(b'memoryview'))
        self.assertIsNone(getargs_Z_hash(None))


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


if __name__ == "__main__":
    unittest.main()
