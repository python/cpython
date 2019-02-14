import unittest
import math
import string
import sys
from test import test_support
# Skip this test if the _testcapi module isn't available.
_testcapi = test_support.import_module('_testcapi')
from _testcapi import getargs_keywords
import warnings

"""
> How about the following counterproposal. This also changes some of
> the other format codes to be a little more regular.
>
> Code C type Range check
>
> b unsigned char 0..UCHAR_MAX
> h signed short SHRT_MIN..SHRT_MAX
> B unsigned char none **
> H unsigned short none **
> k * unsigned long none
> I * unsigned int 0..UINT_MAX


> i int INT_MIN..INT_MAX
> l long LONG_MIN..LONG_MAX

> K * unsigned long long none
> L long long LLONG_MIN..LLONG_MAX

> Notes:
>
> * New format codes.
>
> ** Changed from previous "range-and-a-half" to "none"; the
> range-and-a-half checking wasn't particularly useful.

Plus a C API or two, e.g. PyInt_AsLongMask() ->
unsigned long and PyInt_AsLongLongMask() -> unsigned
long long (if that exists).
"""

LARGE = 0x7FFFFFFF
VERY_LARGE = 0xFF0000121212121212121242L

from _testcapi import UCHAR_MAX, USHRT_MAX, UINT_MAX, ULONG_MAX, INT_MAX, \
     INT_MIN, LONG_MIN, LONG_MAX, PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, \
     SHRT_MIN, SHRT_MAX, FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX

DBL_MAX_EXP = sys.float_info.max_exp
INF = float('inf')
NAN = float('nan')

try:
    from _testcapi import getargs_L, getargs_K
except ImportError:
    _PY_LONG_LONG_available = False
else:
    _PY_LONG_LONG_available = True

# fake, they are not defined in Python's header files
LLONG_MAX = 2**63-1
LLONG_MIN = -2**63
ULLONG_MAX = 2**64-1

class Long:
    def __int__(self):
        return 99L

class Int:
    def __int__(self):
        return 99


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
        self.assertEqual(99, getargs_b(Long()))
        self.assertEqual(99, getargs_b(Int()))

        self.assertRaises(OverflowError, getargs_b, -1)
        self.assertEqual(0, getargs_b(0))
        self.assertEqual(UCHAR_MAX, getargs_b(UCHAR_MAX))
        self.assertRaises(OverflowError, getargs_b, UCHAR_MAX + 1)

        self.assertEqual(42, getargs_b(42))
        self.assertEqual(42, getargs_b(42L))
        self.assertRaises(OverflowError, getargs_b, VERY_LARGE)

    def test_B(self):
        from _testcapi import getargs_B
        # B returns 'unsigned char', no range checking
        self.assertRaises(TypeError, getargs_B, 3.14)
        self.assertEqual(99, getargs_B(Long()))
        self.assertEqual(99, getargs_B(Int()))

        self.assertEqual(UCHAR_MAX, getargs_B(-1))
        self.assertEqual(UCHAR_MAX, getargs_B(-1L))
        self.assertEqual(0, getargs_B(0))
        self.assertEqual(UCHAR_MAX, getargs_B(UCHAR_MAX))
        self.assertEqual(0, getargs_B(UCHAR_MAX+1))

        self.assertEqual(42, getargs_B(42))
        self.assertEqual(42, getargs_B(42L))
        self.assertEqual(UCHAR_MAX & VERY_LARGE, getargs_B(VERY_LARGE))

    def test_H(self):
        from _testcapi import getargs_H
        # H returns 'unsigned short', no range checking
        self.assertRaises(TypeError, getargs_H, 3.14)
        self.assertEqual(99, getargs_H(Long()))
        self.assertEqual(99, getargs_H(Int()))

        self.assertEqual(USHRT_MAX, getargs_H(-1))
        self.assertEqual(0, getargs_H(0))
        self.assertEqual(USHRT_MAX, getargs_H(USHRT_MAX))
        self.assertEqual(0, getargs_H(USHRT_MAX+1))

        self.assertEqual(42, getargs_H(42))
        self.assertEqual(42, getargs_H(42L))

        self.assertEqual(VERY_LARGE & USHRT_MAX, getargs_H(VERY_LARGE))

    def test_I(self):
        from _testcapi import getargs_I
        # I returns 'unsigned int', no range checking
        self.assertRaises(TypeError, getargs_I, 3.14)
        self.assertEqual(99, getargs_I(Long()))
        self.assertEqual(99, getargs_I(Int()))

        self.assertEqual(UINT_MAX, getargs_I(-1))
        self.assertEqual(0, getargs_I(0))
        self.assertEqual(UINT_MAX, getargs_I(UINT_MAX))
        self.assertEqual(0, getargs_I(UINT_MAX+1))

        self.assertEqual(42, getargs_I(42))
        self.assertEqual(42, getargs_I(42L))

        self.assertEqual(VERY_LARGE & UINT_MAX, getargs_I(VERY_LARGE))

    def test_k(self):
        from _testcapi import getargs_k
        # k returns 'unsigned long', no range checking
        # it does not accept float, or instances with __int__
        self.assertRaises(TypeError, getargs_k, 3.14)
        self.assertRaises(TypeError, getargs_k, Long())
        self.assertRaises(TypeError, getargs_k, Int())

        self.assertEqual(ULONG_MAX, getargs_k(-1))
        self.assertEqual(0, getargs_k(0))
        self.assertEqual(ULONG_MAX, getargs_k(ULONG_MAX))
        self.assertEqual(0, getargs_k(ULONG_MAX+1))

        self.assertEqual(42, getargs_k(42))
        self.assertEqual(42, getargs_k(42L))

        self.assertEqual(VERY_LARGE & ULONG_MAX, getargs_k(VERY_LARGE))

class Signed_TestCase(unittest.TestCase):
    def test_h(self):
        from _testcapi import getargs_h
        # h returns 'short', and does range checking (SHRT_MIN ... SHRT_MAX)
        self.assertRaises(TypeError, getargs_h, 3.14)
        self.assertEqual(99, getargs_h(Long()))
        self.assertEqual(99, getargs_h(Int()))

        self.assertRaises(OverflowError, getargs_h, SHRT_MIN-1)
        self.assertEqual(SHRT_MIN, getargs_h(SHRT_MIN))
        self.assertEqual(SHRT_MAX, getargs_h(SHRT_MAX))
        self.assertRaises(OverflowError, getargs_h, SHRT_MAX+1)

        self.assertEqual(42, getargs_h(42))
        self.assertEqual(42, getargs_h(42L))
        self.assertRaises(OverflowError, getargs_h, VERY_LARGE)

    def test_i(self):
        from _testcapi import getargs_i
        # i returns 'int', and does range checking (INT_MIN ... INT_MAX)
        self.assertRaises(TypeError, getargs_i, 3.14)
        self.assertEqual(99, getargs_i(Long()))
        self.assertEqual(99, getargs_i(Int()))

        self.assertRaises(OverflowError, getargs_i, INT_MIN-1)
        self.assertEqual(INT_MIN, getargs_i(INT_MIN))
        self.assertEqual(INT_MAX, getargs_i(INT_MAX))
        self.assertRaises(OverflowError, getargs_i, INT_MAX+1)

        self.assertEqual(42, getargs_i(42))
        self.assertEqual(42, getargs_i(42L))
        self.assertRaises(OverflowError, getargs_i, VERY_LARGE)

    def test_l(self):
        from _testcapi import getargs_l
        # l returns 'long', and does range checking (LONG_MIN ... LONG_MAX)
        self.assertRaises(TypeError, getargs_l, 3.14)
        self.assertEqual(99, getargs_l(Long()))
        self.assertEqual(99, getargs_l(Int()))

        self.assertRaises(OverflowError, getargs_l, LONG_MIN-1)
        self.assertEqual(LONG_MIN, getargs_l(LONG_MIN))
        self.assertEqual(LONG_MAX, getargs_l(LONG_MAX))
        self.assertRaises(OverflowError, getargs_l, LONG_MAX+1)

        self.assertEqual(42, getargs_l(42))
        self.assertEqual(42, getargs_l(42L))
        self.assertRaises(OverflowError, getargs_l, VERY_LARGE)

    def test_n(self):
        from _testcapi import getargs_n
        # n returns 'Py_ssize_t', and does range checking
        # (PY_SSIZE_T_MIN ... PY_SSIZE_T_MAX)
        self.assertRaises(TypeError, getargs_n, 3.14)
        self.assertEqual(99, getargs_n(Long()))
        self.assertEqual(99, getargs_n(Int()))

        self.assertRaises(OverflowError, getargs_n, PY_SSIZE_T_MIN-1)
        self.assertEqual(PY_SSIZE_T_MIN, getargs_n(PY_SSIZE_T_MIN))
        self.assertEqual(PY_SSIZE_T_MAX, getargs_n(PY_SSIZE_T_MAX))
        self.assertRaises(OverflowError, getargs_n, PY_SSIZE_T_MAX+1)

        self.assertEqual(42, getargs_n(42))
        self.assertEqual(42, getargs_n(42L))
        self.assertRaises(OverflowError, getargs_n, VERY_LARGE)


@unittest.skipUnless(_PY_LONG_LONG_available, 'PY_LONG_LONG not available')
class LongLong_TestCase(unittest.TestCase):
    def test_L(self):
        from _testcapi import getargs_L
        # L returns 'long long', and does range checking (LLONG_MIN
        # ... LLONG_MAX)
        with warnings.catch_warnings():
            warnings.filterwarnings(
                "ignore",
                category=DeprecationWarning,
                message=".*integer argument expected, got float",
                module=__name__)
            self.assertEqual(3, getargs_L(3.14))
        with warnings.catch_warnings():
            warnings.filterwarnings(
                "error",
                category=DeprecationWarning,
                message=".*integer argument expected, got float",
                module="unittest")
            self.assertRaises(DeprecationWarning, getargs_L, 3.14)

        self.assertRaises(TypeError, getargs_L, "Hello")
        self.assertEqual(99, getargs_L(Long()))
        self.assertEqual(99, getargs_L(Int()))

        self.assertRaises(OverflowError, getargs_L, LLONG_MIN-1)
        self.assertEqual(LLONG_MIN, getargs_L(LLONG_MIN))
        self.assertEqual(LLONG_MAX, getargs_L(LLONG_MAX))
        self.assertRaises(OverflowError, getargs_L, LLONG_MAX+1)

        self.assertEqual(42, getargs_L(42))
        self.assertEqual(42, getargs_L(42L))
        self.assertRaises(OverflowError, getargs_L, VERY_LARGE)

    def test_K(self):
        from _testcapi import getargs_K
        # K return 'unsigned long long', no range checking
        self.assertRaises(TypeError, getargs_K, 3.14)
        self.assertRaises(TypeError, getargs_K, Long())
        self.assertRaises(TypeError, getargs_K, Int())
        self.assertEqual(ULLONG_MAX, getargs_K(ULLONG_MAX))
        self.assertEqual(0, getargs_K(0))
        self.assertEqual(0, getargs_K(ULLONG_MAX+1))

        self.assertEqual(42, getargs_K(42))
        self.assertEqual(42, getargs_K(42L))

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
        self.assertIsInstance(ret, tuple)

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
        self.assertIsInstance(ret, dict)

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
        except TypeError, err:
            self.assertEqual(str(err), "Required argument 'arg2' (pos 2) not found")
        else:
            self.fail('TypeError should have been raised')
    def test_too_many_args(self):
        try:
            getargs_keywords((1,2),3,(4,(5,6)),(7,8,9),10,111)
        except TypeError, err:
            self.assertEqual(str(err), "function takes at most 5 arguments (6 given)")
        else:
            self.fail('TypeError should have been raised')
    def test_invalid_keyword(self):
        # extraneous keyword arg
        try:
            getargs_keywords((1,2),3,arg5=10,arg666=666)
        except TypeError, err:
            self.assertEqual(str(err), "'arg666' is an invalid keyword argument for this function")
        else:
            self.fail('TypeError should have been raised')


class Bytes_TestCase(unittest.TestCase):
    def test_c(self):
        from _testcapi import getargs_c
        self.assertRaises(TypeError, getargs_c, 'abc')  # len > 1
        self.assertEqual(getargs_c('a'), 97)
        if test_support.have_unicode:
            self.assertRaises(TypeError, getargs_c, u's')
        self.assertRaises(TypeError, getargs_c, bytearray('a'))
        self.assertRaises(TypeError, getargs_c, memoryview('a'))
        with test_support.check_py3k_warnings():
            self.assertRaises(TypeError, getargs_c, buffer('a'))
        self.assertRaises(TypeError, getargs_c, 97)
        self.assertRaises(TypeError, getargs_c, None)

    def test_w(self):
        from _testcapi import getargs_w
        self.assertRaises(TypeError, getargs_w, 'abc', 3)
        self.assertRaises(TypeError, getargs_w, u'abc', 3)
        self.assertRaises(TypeError, getargs_w, bytearray('bytes'), 3)
        self.assertRaises(TypeError, getargs_w, memoryview('bytes'), 3)
        self.assertRaises(TypeError, getargs_w,
                          memoryview(bytearray('bytes')), 3)
        with test_support.check_py3k_warnings():
            self.assertRaises(TypeError, getargs_w, buffer('bytes'), 3)
            self.assertRaises(TypeError, getargs_w,
                              buffer(bytearray('bytes')), 3)
        self.assertRaises(TypeError, getargs_w, None, 0)

    def test_w_hash(self):
        from _testcapi import getargs_w_hash
        self.assertRaises(TypeError, getargs_w_hash, 'abc')
        self.assertRaises(TypeError, getargs_w_hash, u'abc')
        self.assertRaises(TypeError, getargs_w_hash, bytearray('bytes'))
        self.assertRaises(TypeError, getargs_w_hash, memoryview('bytes'))
        self.assertRaises(TypeError, getargs_w_hash,
                          memoryview(bytearray('bytes')))
        with test_support.check_py3k_warnings():
            self.assertRaises(TypeError, getargs_w_hash, buffer('bytes'))
            self.assertRaises(TypeError, getargs_w_hash,
                              buffer(bytearray('bytes')))
        self.assertRaises(TypeError, getargs_w_hash, None)

    def test_w_star(self):
        # getargs_w_star() modifies first and last byte
        from _testcapi import getargs_w_star
        self.assertRaises(TypeError, getargs_w_star, 'abc')
        self.assertRaises(TypeError, getargs_w_star, u'abc')
        self.assertRaises(TypeError, getargs_w_star, memoryview('bytes'))
        buf = bytearray('bytearray')
        self.assertEqual(getargs_w_star(buf), '[ytearra]')
        self.assertEqual(buf, bytearray('[ytearra]'))
        buf = bytearray(b'memoryview')
        self.assertEqual(getargs_w_star(memoryview(buf)), '[emoryvie]')
        self.assertEqual(buf, bytearray('[emoryvie]'))
        with test_support.check_py3k_warnings():
            self.assertRaises(TypeError, getargs_w_star, buffer('buffer'))
            self.assertRaises(TypeError, getargs_w_star,
                              buffer(bytearray('buffer')))
        self.assertRaises(TypeError, getargs_w_star, None)


class String_TestCase(unittest.TestCase):
    def test_s(self):
        from _testcapi import getargs_s
        self.assertEqual(getargs_s('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_s(u'abc'), 'abc')
        self.assertRaises(TypeError, getargs_s, 'nul:\0')
        self.assertRaises(TypeError, getargs_s, u'nul:\0')
        self.assertRaises(TypeError, getargs_s, bytearray('bytearray'))
        self.assertRaises(TypeError, getargs_s, memoryview('memoryview'))
        with test_support.check_py3k_warnings():
            self.assertRaises(TypeError, getargs_s, buffer('buffer'))
        self.assertRaises(TypeError, getargs_s, None)

    def test_s_star(self):
        from _testcapi import getargs_s_star
        self.assertEqual(getargs_s_star('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_s_star(u'abc'), 'abc')
        self.assertEqual(getargs_s_star('nul:\0'), 'nul:\0')
        self.assertEqual(getargs_s_star(u'nul:\0'), 'nul:\0')
        self.assertEqual(getargs_s_star(bytearray('abc\xe9')), 'abc\xe9')
        self.assertEqual(getargs_s_star(memoryview('abc\xe9')), 'abc\xe9')
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_s_star(buffer('abc\xe9')), 'abc\xe9')
            self.assertEqual(getargs_s_star(buffer(u'abc\xe9')),
                             str(buffer(u'abc\xe9')))
        self.assertRaises(TypeError, getargs_s_star, None)

    def test_s_hash(self):
        from _testcapi import getargs_s_hash
        self.assertEqual(getargs_s_hash('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_s_hash(u'abc'), 'abc')
        self.assertEqual(getargs_s_hash('nul:\0'), 'nul:\0')
        self.assertEqual(getargs_s_hash(u'nul:\0'), 'nul:\0')
        self.assertRaises(TypeError, getargs_s_hash, bytearray('bytearray'))
        self.assertRaises(TypeError, getargs_s_hash, memoryview('memoryview'))
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_s_hash(buffer('abc\xe9')), 'abc\xe9')
            self.assertEqual(getargs_s_hash(buffer(u'abc\xe9')),
                             str(buffer(u'abc\xe9')))
        self.assertRaises(TypeError, getargs_s_hash, None)

    def test_t_hash(self):
        from _testcapi import getargs_t_hash
        self.assertEqual(getargs_t_hash('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_t_hash(u'abc'), 'abc')
        self.assertEqual(getargs_t_hash('nul:\0'), 'nul:\0')
        self.assertEqual(getargs_t_hash(u'nul:\0'), 'nul:\0')
        self.assertRaises(TypeError, getargs_t_hash, bytearray('bytearray'))
        self.assertRaises(TypeError, getargs_t_hash, memoryview('memoryview'))
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_t_hash(buffer('abc\xe9')), 'abc\xe9')
            self.assertEqual(getargs_t_hash(buffer(u'abc')), 'abc')
        self.assertRaises(TypeError, getargs_t_hash, None)

    def test_z(self):
        from _testcapi import getargs_z
        self.assertEqual(getargs_z('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_z(u'abc'), 'abc')
        self.assertRaises(TypeError, getargs_z, 'nul:\0')
        self.assertRaises(TypeError, getargs_z, u'nul:\0')
        self.assertRaises(TypeError, getargs_z, bytearray('bytearray'))
        self.assertRaises(TypeError, getargs_z, memoryview('memoryview'))
        with test_support.check_py3k_warnings():
            self.assertRaises(TypeError, getargs_z, buffer('buffer'))
        self.assertIsNone(getargs_z(None))

    def test_z_star(self):
        from _testcapi import getargs_z_star
        self.assertEqual(getargs_z_star('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_z_star(u'abc'), 'abc')
        self.assertEqual(getargs_z_star('nul:\0'), 'nul:\0')
        self.assertEqual(getargs_z_star(u'nul:\0'), 'nul:\0')
        self.assertEqual(getargs_z_star(bytearray('abc\xe9')), 'abc\xe9')
        self.assertEqual(getargs_z_star(memoryview('abc\xe9')), 'abc\xe9')
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_z_star(buffer('abc\xe9')), 'abc\xe9')
            self.assertEqual(getargs_z_star(buffer(u'abc\xe9')),
                             str(buffer(u'abc\xe9')))
        self.assertIsNone(getargs_z_star(None))

    def test_z_hash(self):
        from _testcapi import getargs_z_hash
        self.assertEqual(getargs_z_hash('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_z_hash(u'abc'), 'abc')
        self.assertEqual(getargs_z_hash('nul:\0'), 'nul:\0')
        self.assertEqual(getargs_z_hash(u'nul:\0'), 'nul:\0')
        self.assertRaises(TypeError, getargs_z_hash, bytearray('bytearray'))
        self.assertRaises(TypeError, getargs_z_hash, memoryview('memoryview'))
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_z_hash(buffer('abc\xe9')), 'abc\xe9')
            self.assertEqual(getargs_z_hash(buffer(u'abc\xe9')),
                             str(buffer(u'abc\xe9')))
        self.assertIsNone(getargs_z_hash(None))


@test_support.requires_unicode
class Unicode_TestCase(unittest.TestCase):
    def test_es(self):
        from _testcapi import getargs_es
        self.assertEqual(getargs_es('abc'), 'abc')
        self.assertEqual(getargs_es(u'abc'), 'abc')
        self.assertEqual(getargs_es('abc', 'ascii'), 'abc')
        self.assertEqual(getargs_es(u'abc\xe9', 'latin1'), 'abc\xe9')
        self.assertRaises(UnicodeEncodeError, getargs_es, u'abc\xe9', 'ascii')
        self.assertRaises(LookupError, getargs_es, u'abc', 'spam')
        self.assertRaises(TypeError, getargs_es,
                          bytearray('bytearray'), 'latin1')
        self.assertRaises(TypeError, getargs_es,
                          memoryview('memoryview'), 'latin1')
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_es(buffer('abc'), 'ascii'), 'abc')
            self.assertEqual(getargs_es(buffer(u'abc'), 'ascii'), 'abc')
        self.assertRaises(TypeError, getargs_es, None, 'latin1')
        self.assertRaises(TypeError, getargs_es, 'nul:\0', 'latin1')
        self.assertRaises(TypeError, getargs_es, u'nul:\0', 'latin1')

    def test_et(self):
        from _testcapi import getargs_et
        self.assertEqual(getargs_et('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_et(u'abc'), 'abc')
        self.assertEqual(getargs_et('abc', 'ascii'), 'abc')
        self.assertEqual(getargs_et('abc\xe9', 'ascii'), 'abc\xe9')
        self.assertEqual(getargs_et(u'abc\xe9', 'latin1'), 'abc\xe9')
        self.assertRaises(UnicodeEncodeError, getargs_et, u'abc\xe9', 'ascii')
        self.assertRaises(LookupError, getargs_et, u'abc', 'spam')
        self.assertRaises(TypeError, getargs_et,
                          bytearray('bytearray'), 'latin1')
        self.assertRaises(TypeError, getargs_et,
                          memoryview('memoryview'), 'latin1')
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_et(buffer('abc'), 'ascii'), 'abc')
            self.assertEqual(getargs_et(buffer(u'abc'), 'ascii'), 'abc')
        self.assertRaises(TypeError, getargs_et, None, 'latin1')
        self.assertRaises(TypeError, getargs_et, 'nul:\0', 'latin1')
        self.assertRaises(TypeError, getargs_et, u'nul:\0', 'latin1')

    def test_es_hash(self):
        from _testcapi import getargs_es_hash
        self.assertEqual(getargs_es_hash('abc'), 'abc')
        self.assertEqual(getargs_es_hash(u'abc'), 'abc')
        self.assertEqual(getargs_es_hash(u'abc\xe9', 'latin1'), 'abc\xe9')
        self.assertRaises(UnicodeEncodeError, getargs_es_hash, u'abc\xe9', 'ascii')
        self.assertRaises(LookupError, getargs_es_hash, u'abc', 'spam')
        self.assertRaises(TypeError, getargs_es_hash,
                          bytearray('bytearray'), 'latin1')
        self.assertRaises(TypeError, getargs_es_hash,
                          memoryview('memoryview'), 'latin1')
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_es_hash(buffer('abc'), 'ascii'), 'abc')
            self.assertEqual(getargs_es_hash(buffer(u'abc'), 'ascii'), 'abc')
        self.assertRaises(TypeError, getargs_es_hash, None, 'latin1')
        self.assertEqual(getargs_es_hash('nul:\0', 'latin1'), 'nul:\0')
        self.assertEqual(getargs_es_hash(u'nul:\0', 'latin1'), 'nul:\0')

        buf = bytearray('x'*8)
        self.assertEqual(getargs_es_hash(u'abc\xe9', 'latin1', buf), 'abc\xe9')
        self.assertEqual(buf, bytearray('abc\xe9\x00xxx'))
        buf = bytearray('x'*5)
        self.assertEqual(getargs_es_hash(u'abc\xe9', 'latin1', buf), 'abc\xe9')
        self.assertEqual(buf, bytearray('abc\xe9\x00'))
        buf = bytearray('x'*4)
        self.assertRaises(TypeError, getargs_es_hash, u'abc\xe9', 'latin1', buf)
        self.assertEqual(buf, bytearray('x'*4))
        buf = bytearray()
        self.assertRaises(TypeError, getargs_es_hash, u'abc\xe9', 'latin1', buf)

    def test_et_hash(self):
        from _testcapi import getargs_et_hash
        self.assertEqual(getargs_et_hash('abc\xe9'), 'abc\xe9')
        self.assertEqual(getargs_et_hash(u'abc'), 'abc')
        self.assertEqual(getargs_et_hash('abc\xe9', 'ascii'), 'abc\xe9')
        self.assertEqual(getargs_et_hash(u'abc\xe9', 'latin1'), 'abc\xe9')
        self.assertRaises(UnicodeEncodeError, getargs_et_hash,
                          u'abc\xe9', 'ascii')
        self.assertRaises(LookupError, getargs_et_hash, u'abc', 'spam')
        self.assertRaises(TypeError, getargs_et_hash,
                          bytearray('bytearray'), 'latin1')
        self.assertRaises(TypeError, getargs_et_hash,
                          memoryview('memoryview'), 'latin1')
        with test_support.check_py3k_warnings():
            self.assertEqual(getargs_et_hash(buffer('abc'), 'ascii'), 'abc')
            self.assertEqual(getargs_et_hash(buffer(u'abc'), 'ascii'), 'abc')
        self.assertRaises(TypeError, getargs_et_hash, None, 'latin1')
        self.assertEqual(getargs_et_hash('nul:\0', 'latin1'), 'nul:\0')
        self.assertEqual(getargs_et_hash(u'nul:\0', 'latin1'), 'nul:\0')

        buf = bytearray('x'*8)
        self.assertEqual(getargs_et_hash(u'abc\xe9', 'latin1', buf), 'abc\xe9')
        self.assertEqual(buf, bytearray('abc\xe9\x00xxx'))
        buf = bytearray('x'*5)
        self.assertEqual(getargs_et_hash(u'abc\xe9', 'latin1', buf), 'abc\xe9')
        self.assertEqual(buf, bytearray('abc\xe9\x00'))
        buf = bytearray('x'*4)
        self.assertRaises(TypeError, getargs_et_hash, u'abc\xe9', 'latin1', buf)
        self.assertEqual(buf, bytearray('x'*4))
        buf = bytearray()
        self.assertRaises(TypeError, getargs_et_hash, u'abc\xe9', 'latin1', buf)

    def test_u(self):
        from _testcapi import getargs_u
        self.assertEqual(getargs_u(u'abc\xe9'), u'abc\xe9')
        self.assertRaises(TypeError, getargs_u, u'nul:\0')
        self.assertRaises(TypeError, getargs_u, 'bytes')
        self.assertRaises(TypeError, getargs_u, bytearray('bytearray'))
        self.assertRaises(TypeError, getargs_u, memoryview('memoryview'))
        with test_support.check_py3k_warnings():
            self.assertRaises(TypeError, getargs_u, buffer('buffer'))
        self.assertRaises(TypeError, getargs_u, None)

    def test_u_hash(self):
        from _testcapi import getargs_u_hash
        self.assertEqual(getargs_u_hash(u'abc\xe9'), u'abc\xe9')
        self.assertEqual(getargs_u_hash(u'nul:\0'), u'nul:\0')
        self.assertRaises(TypeError, getargs_u_hash, 'bytes')
        self.assertRaises(TypeError, getargs_u_hash, bytearray('bytearray'))
        self.assertRaises(TypeError, getargs_u_hash, memoryview('memoryview'))
        with test_support.check_py3k_warnings():
            self.assertRaises(TypeError, getargs_u_hash, buffer('buffer'))
        self.assertRaises(TypeError, getargs_u_hash, None)


class Object_TestCase(unittest.TestCase):
    def test_S(self):
        from _testcapi import getargs_S
        obj = 'str'
        self.assertIs(getargs_S(obj), obj)
        self.assertRaises(TypeError, getargs_S, bytearray('bytearray'))
        if test_support.have_unicode:
            self.assertRaises(TypeError, getargs_S, u'unicode')
        self.assertRaises(TypeError, getargs_S, None)
        self.assertRaises(TypeError, getargs_S, memoryview(obj))
        self.assertRaises(TypeError, getargs_S, buffer(obj))

    def test_Y(self):
        from _testcapi import getargs_Y
        obj = bytearray('bytearray')
        self.assertIs(getargs_Y(obj), obj)
        self.assertRaises(TypeError, getargs_Y, 'str')
        if test_support.have_unicode:
            self.assertRaises(TypeError, getargs_Y, u'unicode')
        self.assertRaises(TypeError, getargs_Y, None)
        self.assertRaises(TypeError, getargs_Y, memoryview(obj))
        self.assertRaises(TypeError, getargs_Y, buffer(obj))

    @test_support.requires_unicode
    def test_U(self):
        from _testcapi import getargs_U
        obj = u'unicode'
        self.assertIs(getargs_U(obj), obj)
        self.assertRaises(TypeError, getargs_U, 'str')
        self.assertRaises(TypeError, getargs_U, bytearray('bytearray'))
        self.assertRaises(TypeError, getargs_U, None)
        self.assertRaises(TypeError, getargs_U, memoryview(obj))
        self.assertRaises(TypeError, getargs_U, buffer(obj))


class SkipitemTest(unittest.TestCase):

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
            # skip '|', it doesn't represent arguments anyway
            if c in '()e|':
                continue

            # test the format unit when not skipped
            format = c + "i"
            try:
                _testcapi.parse_tuple_and_keywords(tuple_1, dict_b,
                    format, keywords)
                when_not_skipped = False
            except TypeError as e:
                s = "argument 1 (impossible<bad format char>)"
                when_not_skipped = (str(e) == s)
            except RuntimeError:
                when_not_skipped = False

            # test the format unit when skipped
            optional_format = "|" + format
            try:
                _testcapi.parse_tuple_and_keywords(empty_tuple, dict_b,
                    optional_format, keywords)
                when_skipped = False
            except RuntimeError as e:
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

        supported = ('s#', 's*', 'z#', 'z*', 'u#', 't#', 'w#', 'w*')
        for c in string.ascii_letters:
            for c2 in '#*':
                f = c + c2
                optional_format = "|" + f + "i"
                if f in supported:
                    parse(empty_tuple, dict_b, optional_format, keywords)
                else:
                    with self.assertRaisesRegexp((RuntimeError, TypeError),
                                'impossible<bad format char>'):
                        parse(empty_tuple, dict_b, optional_format, keywords)

        for c in map(chr, range(32, 128)):
            f = 'e' + c
            optional_format = "|" + f + "i"
            if c in 'st':
                parse(empty_tuple, dict_b, optional_format, keywords)
            else:
                with self.assertRaisesRegexp(RuntimeError,
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
        self.assertRaises(TypeError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '', [42])

    def test_bad_use(self):
        # Test handling invalid format and keywords in
        # PyArg_ParseTupleAndKeywords()
        self.assertRaises(TypeError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '||O', ['a'])
        self.assertRaises(RuntimeError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '|O', ['a', 'b'])
        self.assertRaises(RuntimeError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '|OO', ['a'])


class Test_testcapi(unittest.TestCase):
    locals().update((name, getattr(_testcapi, name))
                    for name in dir(_testcapi)
                    if name.startswith('test_') and name.endswith('_code'))


def test_main():
    tests = [Signed_TestCase, Unsigned_TestCase, LongLong_TestCase,
             Tuple_TestCase, Keywords_TestCase,
             Bytes_TestCase, String_TestCase, Unicode_TestCase,
             SkipitemTest, ParseTupleAndKeywords_Test, Test_testcapi]
    test_support.run_unittest(*tests)

if __name__ == "__main__":
    test_main()
