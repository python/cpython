import unittest
from test import test_support
import sys

import warnings, re
warnings.filterwarnings("ignore",
                        category=DeprecationWarning,
                        message=".*integer argument expected, got float",
                        module=__name__)
warnings.filterwarnings("ignore",
                        category=DeprecationWarning,
                        message=".*integer argument expected, got float",
                        module="unittest")

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
     INT_MIN, LONG_MIN, LONG_MAX, PY_SSIZE_T_MIN, PY_SSIZE_T_MAX

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

class Unsigned_TestCase(unittest.TestCase):
    def test_b(self):
        from _testcapi import getargs_b
        # b returns 'unsigned char', and does range checking (0 ... UCHAR_MAX)
        self.failUnlessEqual(3, getargs_b(3.14))
        self.failUnlessEqual(99, getargs_b(Long()))
        self.failUnlessEqual(99, getargs_b(Int()))

        self.assertRaises(OverflowError, getargs_b, -1)
        self.failUnlessEqual(0, getargs_b(0))
        self.failUnlessEqual(UCHAR_MAX, getargs_b(UCHAR_MAX))
        self.assertRaises(OverflowError, getargs_b, UCHAR_MAX + 1)

        self.failUnlessEqual(42, getargs_b(42))
        self.failUnlessEqual(42, getargs_b(42L))
        self.assertRaises(OverflowError, getargs_b, VERY_LARGE)

    def test_B(self):
        from _testcapi import getargs_B
        # B returns 'unsigned char', no range checking
        self.failUnlessEqual(3, getargs_B(3.14))
        self.failUnlessEqual(99, getargs_B(Long()))
        self.failUnlessEqual(99, getargs_B(Int()))

        self.failUnlessEqual(UCHAR_MAX, getargs_B(-1))
        self.failUnlessEqual(UCHAR_MAX, getargs_B(-1L))
        self.failUnlessEqual(0, getargs_B(0))
        self.failUnlessEqual(UCHAR_MAX, getargs_B(UCHAR_MAX))
        self.failUnlessEqual(0, getargs_B(UCHAR_MAX+1))

        self.failUnlessEqual(42, getargs_B(42))
        self.failUnlessEqual(42, getargs_B(42L))
        self.failUnlessEqual(UCHAR_MAX & VERY_LARGE, getargs_B(VERY_LARGE))

    def test_H(self):
        from _testcapi import getargs_H
        # H returns 'unsigned short', no range checking
        self.failUnlessEqual(3, getargs_H(3.14))
        self.failUnlessEqual(99, getargs_H(Long()))
        self.failUnlessEqual(99, getargs_H(Int()))

        self.failUnlessEqual(USHRT_MAX, getargs_H(-1))
        self.failUnlessEqual(0, getargs_H(0))
        self.failUnlessEqual(USHRT_MAX, getargs_H(USHRT_MAX))
        self.failUnlessEqual(0, getargs_H(USHRT_MAX+1))

        self.failUnlessEqual(42, getargs_H(42))
        self.failUnlessEqual(42, getargs_H(42L))

        self.failUnlessEqual(VERY_LARGE & USHRT_MAX, getargs_H(VERY_LARGE))

    def test_I(self):
        from _testcapi import getargs_I
        # I returns 'unsigned int', no range checking
        self.failUnlessEqual(3, getargs_I(3.14))
        self.failUnlessEqual(99, getargs_I(Long()))
        self.failUnlessEqual(99, getargs_I(Int()))

        self.failUnlessEqual(UINT_MAX, getargs_I(-1))
        self.failUnlessEqual(0, getargs_I(0))
        self.failUnlessEqual(UINT_MAX, getargs_I(UINT_MAX))
        self.failUnlessEqual(0, getargs_I(UINT_MAX+1))

        self.failUnlessEqual(42, getargs_I(42))
        self.failUnlessEqual(42, getargs_I(42L))

        self.failUnlessEqual(VERY_LARGE & UINT_MAX, getargs_I(VERY_LARGE))

    def test_k(self):
        from _testcapi import getargs_k
        # k returns 'unsigned long', no range checking
        # it does not accept float, or instances with __int__
        self.assertRaises(TypeError, getargs_k, 3.14)
        self.assertRaises(TypeError, getargs_k, Long())
        self.assertRaises(TypeError, getargs_k, Int())

        self.failUnlessEqual(ULONG_MAX, getargs_k(-1))
        self.failUnlessEqual(0, getargs_k(0))
        self.failUnlessEqual(ULONG_MAX, getargs_k(ULONG_MAX))
        self.failUnlessEqual(0, getargs_k(ULONG_MAX+1))

        self.failUnlessEqual(42, getargs_k(42))
        self.failUnlessEqual(42, getargs_k(42L))

        self.failUnlessEqual(VERY_LARGE & ULONG_MAX, getargs_k(VERY_LARGE))

class Signed_TestCase(unittest.TestCase):
    def test_i(self):
        from _testcapi import getargs_i
        # i returns 'int', and does range checking (INT_MIN ... INT_MAX)
        self.failUnlessEqual(3, getargs_i(3.14))
        self.failUnlessEqual(99, getargs_i(Long()))
        self.failUnlessEqual(99, getargs_i(Int()))

        self.assertRaises(OverflowError, getargs_i, INT_MIN-1)
        self.failUnlessEqual(INT_MIN, getargs_i(INT_MIN))
        self.failUnlessEqual(INT_MAX, getargs_i(INT_MAX))
        self.assertRaises(OverflowError, getargs_i, INT_MAX+1)

        self.failUnlessEqual(42, getargs_i(42))
        self.failUnlessEqual(42, getargs_i(42L))
        self.assertRaises(OverflowError, getargs_i, VERY_LARGE)

    def test_l(self):
        from _testcapi import getargs_l
        # l returns 'long', and does range checking (LONG_MIN ... LONG_MAX)
        self.failUnlessEqual(3, getargs_l(3.14))
        self.failUnlessEqual(99, getargs_l(Long()))
        self.failUnlessEqual(99, getargs_l(Int()))

        self.assertRaises(OverflowError, getargs_l, LONG_MIN-1)
        self.failUnlessEqual(LONG_MIN, getargs_l(LONG_MIN))
        self.failUnlessEqual(LONG_MAX, getargs_l(LONG_MAX))
        self.assertRaises(OverflowError, getargs_l, LONG_MAX+1)

        self.failUnlessEqual(42, getargs_l(42))
        self.failUnlessEqual(42, getargs_l(42L))
        self.assertRaises(OverflowError, getargs_l, VERY_LARGE)

    def test_n(self):
        from _testcapi import getargs_n
        # n returns 'Py_ssize_t', and does range checking
        # (PY_SSIZE_T_MIN ... PY_SSIZE_T_MAX)
        self.failUnlessEqual(3, getargs_n(3.14))
        self.failUnlessEqual(99, getargs_n(Long()))
        self.failUnlessEqual(99, getargs_n(Int()))

        self.assertRaises(OverflowError, getargs_n, PY_SSIZE_T_MIN-1)
        self.failUnlessEqual(PY_SSIZE_T_MIN, getargs_n(PY_SSIZE_T_MIN))
        self.failUnlessEqual(PY_SSIZE_T_MAX, getargs_n(PY_SSIZE_T_MAX))
        self.assertRaises(OverflowError, getargs_n, PY_SSIZE_T_MAX+1)

        self.failUnlessEqual(42, getargs_n(42))
        self.failUnlessEqual(42, getargs_n(42L))
        self.assertRaises(OverflowError, getargs_n, VERY_LARGE)


class LongLong_TestCase(unittest.TestCase):
    def test_L(self):
        from _testcapi import getargs_L
        # L returns 'long long', and does range checking (LLONG_MIN ... LLONG_MAX)
        self.failUnlessRaises(TypeError, getargs_L, "Hello")
        self.failUnlessEqual(3, getargs_L(3.14))
        self.failUnlessEqual(99, getargs_L(Long()))
        self.failUnlessEqual(99, getargs_L(Int()))

        self.assertRaises(OverflowError, getargs_L, LLONG_MIN-1)
        self.failUnlessEqual(LLONG_MIN, getargs_L(LLONG_MIN))
        self.failUnlessEqual(LLONG_MAX, getargs_L(LLONG_MAX))
        self.assertRaises(OverflowError, getargs_L, LLONG_MAX+1)

        self.failUnlessEqual(42, getargs_L(42))
        self.failUnlessEqual(42, getargs_L(42L))
        self.assertRaises(OverflowError, getargs_L, VERY_LARGE)

    def test_K(self):
        from _testcapi import getargs_K
        # K return 'unsigned long long', no range checking
        self.assertRaises(TypeError, getargs_K, 3.14)
        self.assertRaises(TypeError, getargs_K, Long())
        self.assertRaises(TypeError, getargs_K, Int())
        self.failUnlessEqual(ULLONG_MAX, getargs_K(ULLONG_MAX))
        self.failUnlessEqual(0, getargs_K(0))
        self.failUnlessEqual(0, getargs_K(ULLONG_MAX+1))

        self.failUnlessEqual(42, getargs_K(42))
        self.failUnlessEqual(42, getargs_K(42L))

        self.failUnlessEqual(VERY_LARGE & ULLONG_MAX, getargs_K(VERY_LARGE))


class Tuple_TestCase(unittest.TestCase):
    def test_tuple(self):
        from _testcapi import getargs_tuple

        ret = getargs_tuple(1, (2, 3))
        self.assertEquals(ret, (1,2,3))

        # make sure invalid tuple arguments are handled correctly
        class seq:
            def __len__(self):
                return 2
            def __getitem__(self, n):
                raise ValueError
        self.assertRaises(TypeError, getargs_tuple, 1, seq())


def test_main():
    tests = [Signed_TestCase, Unsigned_TestCase, Tuple_TestCase]
    try:
        from _testcapi import getargs_L, getargs_K
    except ImportError:
        pass # PY_LONG_LONG not available
    else:
        tests.append(LongLong_TestCase)
    test_support.run_unittest(*tests)

if __name__ == "__main__":
    test_main()
