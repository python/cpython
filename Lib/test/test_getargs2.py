import unittest
from test import test_support
import sys

import warnings, re
warnings.filterwarnings("ignore", category=DeprecationWarning, module=__name__)

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
     INT_MIN, LONG_MIN, LONG_MAX

from _testcapi import getargs_ul as ul_convert
from _testcapi import getargs_l as l_convert
try:
    from _testcapi import getargs_ll as ll_convert
    from _testcapi import getargs_ull as ull_convert
except ImportError:
    pass

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
        # b returns 'unsigned char', and does range checking (0 ... UCHAR_MAX)
        self.failUnlessEqual(3, ul_convert("b", 3.14))
        self.failUnlessEqual(99, ul_convert("b", Long()))
        self.failUnlessEqual(99, ul_convert("b", Int()))

        self.assertRaises(OverflowError, ul_convert, "b", -1)
        self.failUnlessEqual(0, ul_convert("b", 0))
        self.failUnlessEqual(UCHAR_MAX, ul_convert("b", UCHAR_MAX))
        self.assertRaises(OverflowError, ul_convert, "b", UCHAR_MAX + 1)

        self.failUnlessEqual(42, ul_convert("b", 42))
        self.failUnlessEqual(42, ul_convert("b", 42L))
        self.assertRaises(OverflowError, ul_convert, "b", VERY_LARGE)

    def test_B(self):
        # B returns 'unsigned char', no range checking
        self.failUnless(3 == ul_convert("B", 3.14))
        self.failUnlessEqual(99, ul_convert("B", Long()))
        self.failUnlessEqual(99, ul_convert("B", Int()))

        self.failUnlessEqual(UCHAR_MAX, ul_convert("B", -1))
        self.failUnlessEqual(UCHAR_MAX, ul_convert("B", -1L))
        self.failUnlessEqual(0, ul_convert("B", 0))
        self.failUnlessEqual(UCHAR_MAX, ul_convert("B", UCHAR_MAX))
        self.failUnlessEqual(0, ul_convert("B", UCHAR_MAX+1))

        self.failUnlessEqual(42, ul_convert("B", 42))
        self.failUnlessEqual(42, ul_convert("B", 42L))
        self.failUnlessEqual(UCHAR_MAX & VERY_LARGE, ul_convert("B", VERY_LARGE))

    def test_H(self):
        # H returns 'unsigned short', no range checking
        self.failUnlessEqual(3, ul_convert("H", 3.14))
        self.failUnlessEqual(99, ul_convert("H", Long()))
        self.failUnlessEqual(99, ul_convert("H", Int()))

        self.failUnlessEqual(USHRT_MAX, ul_convert("H", -1))
        self.failUnlessEqual(0, ul_convert("H", 0))
        self.failUnlessEqual(USHRT_MAX, ul_convert("H", USHRT_MAX))
        self.failUnlessEqual(0, ul_convert("H", USHRT_MAX+1))

        self.failUnlessEqual(42, ul_convert("H", 42))
        self.failUnlessEqual(42, ul_convert("H", 42L))

        self.failUnlessEqual(VERY_LARGE & USHRT_MAX, ul_convert("H", VERY_LARGE))

    def test_I(self):
        # I returns 'unsigned int', no range checking
        self.failUnlessEqual(3, ul_convert("I", 3.14))
        self.failUnlessEqual(99, ul_convert("I", Long()))
        self.failUnlessEqual(99, ul_convert("I", Int()))

        self.failUnlessEqual(UINT_MAX, ul_convert("I", -1))
        self.failUnlessEqual(0, ul_convert("I", 0))
        self.failUnlessEqual(UINT_MAX, ul_convert("I", UINT_MAX))
        self.failUnlessEqual(0, ul_convert("I", UINT_MAX+1))

        self.failUnlessEqual(42, ul_convert("I", 42))
        self.failUnlessEqual(42, ul_convert("I", 42L))

        self.failUnlessEqual(VERY_LARGE & UINT_MAX, ul_convert("I", VERY_LARGE))

    def test_k(self):
        # k returns 'unsigned long', no range checking
        # it does not accept float, or instances with __int__
        self.assertRaises(TypeError, ul_convert, "k", 3.14)
        self.assertRaises(TypeError, ul_convert, "k", Long())
        self.assertRaises(TypeError, ul_convert, "k", Int())

        self.failUnlessEqual(ULONG_MAX, ul_convert("k", -1))
        self.failUnlessEqual(0, ul_convert("k", 0))
        self.failUnlessEqual(ULONG_MAX, ul_convert("k", ULONG_MAX))
        self.failUnlessEqual(0, ul_convert("k", ULONG_MAX+1))

        self.failUnlessEqual(42, ul_convert("k", 42))
        self.failUnlessEqual(42, ul_convert("k", 42L))

        self.failUnlessEqual(VERY_LARGE & ULONG_MAX, ul_convert("k", VERY_LARGE))

class Signed_TestCase(unittest.TestCase):
    def test_i(self):
        # i returns 'int', and does range checking (INT_MIN ... INT_MAX)
        self.failUnlessEqual(3, l_convert("i", 3.14))
        self.failUnlessEqual(99, l_convert("i", Long()))
        self.failUnlessEqual(99, l_convert("i", Int()))

        self.assertRaises(OverflowError, l_convert, "i", INT_MIN-1)
        self.failUnlessEqual(INT_MIN, l_convert("i", INT_MIN))
        self.failUnlessEqual(INT_MAX, l_convert("i", INT_MAX))
        self.assertRaises(OverflowError, l_convert, "i", INT_MAX+1)

        self.failUnlessEqual(42, l_convert("i", 42))
        self.failUnlessEqual(42, l_convert("i", 42L))
        self.assertRaises(OverflowError, l_convert, "i", VERY_LARGE)

    def test_l(self):
        # l returns 'long', and does range checking (LONG_MIN ... LONG_MAX)
        self.failUnlessEqual(3, l_convert("l", 3.14))
        self.failUnlessEqual(99, l_convert("l", Long()))
        self.failUnlessEqual(99, l_convert("l", Int()))

        self.assertRaises(OverflowError, l_convert, "l", LONG_MIN-1)
        self.failUnlessEqual(LONG_MIN, l_convert("l", LONG_MIN))
        self.failUnlessEqual(LONG_MAX, l_convert("l", LONG_MAX))
        self.assertRaises(OverflowError, l_convert, "l", LONG_MAX+1)

        self.failUnlessEqual(42, l_convert("l", 42))
        self.failUnlessEqual(42, l_convert("l", 42L))
        self.assertRaises(OverflowError, l_convert, "l", VERY_LARGE)


class LongLong_TestCase(unittest.TestCase):
    def test_L(self):
        # L returns 'long long', and does range checking (LLONG_MIN ... LLONG_MAX)

        # XXX There's a bug in getargs.c, format code "L":
        # If you pass something else than a Python long, you
        # get "Bad argument to internal function".

        # So these three tests are commented out:

##        self.failUnlessEqual(3, ll_convert("L", 3.14))
##        self.failUnlessEqual(99, ll_convert("L", Long()))
##        self.failUnlessEqual(99, ll_convert("L", Int()))

        self.assertRaises(OverflowError, ll_convert, "L", LLONG_MIN-1)
        self.failUnlessEqual(LLONG_MIN, ll_convert("L", LLONG_MIN))
        self.failUnlessEqual(LLONG_MAX, ll_convert("L", LLONG_MAX))
        self.assertRaises(OverflowError, ll_convert, "L", LLONG_MAX+1)

        self.failUnlessEqual(42, ll_convert("L", 42))
        self.failUnlessEqual(42, ll_convert("L", 42L))
        self.assertRaises(OverflowError, ll_convert, "L", VERY_LARGE)

    def test_K(self):
        # K return 'unsigned long long', no range checking
        self.assertRaises(TypeError, ull_convert, "K", 3.14)
        self.assertRaises(TypeError, ull_convert, "K", Long())
        self.assertRaises(TypeError, ull_convert, "K", Int())
        self.failUnlessEqual(ULLONG_MAX, ull_convert("K", ULLONG_MAX))
        self.failUnlessEqual(0, ull_convert("K", 0))
        self.failUnlessEqual(0, ull_convert("K", ULLONG_MAX+1))

        self.failUnlessEqual(42, ull_convert("K", 42))
        self.failUnlessEqual(42, ull_convert("K", 42L))

        self.failUnlessEqual(VERY_LARGE & ULLONG_MAX, ull_convert("K", VERY_LARGE))

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(Signed_TestCase))
    suite.addTest(unittest.makeSuite(Unsigned_TestCase))
    try:
        ll_convert
    except NameError:
        pass # PY_LONG_LONG not available
    else:
        suite.addTest(unittest.makeSuite(LongLong_TestCase))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
