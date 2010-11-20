import unittest
from test import support
from _testcapi import getargs_keywords

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
VERY_LARGE = 0xFF0000121212121212121242

from _testcapi import UCHAR_MAX, USHRT_MAX, UINT_MAX, ULONG_MAX, INT_MAX, \
     INT_MIN, LONG_MIN, LONG_MAX, PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, \
     SHRT_MIN, SHRT_MAX

# fake, they are not defined in Python's header files
LLONG_MAX = 2**63-1
LLONG_MIN = -2**63
ULLONG_MAX = 2**64-1

class Int:
    def __int__(self):
        return 99

class Unsigned_TestCase(unittest.TestCase):
    def test_b(self):
        from _testcapi import getargs_b
        # b returns 'unsigned char', and does range checking (0 ... UCHAR_MAX)
        self.assertRaises(TypeError, getargs_b, 3.14)
        self.assertEqual(99, getargs_b(Int()))

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
        self.assertEqual(99, getargs_B(Int()))

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
        self.assertEqual(99, getargs_H(Int()))

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
        self.assertEqual(99, getargs_I(Int()))

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
        self.assertRaises(TypeError, getargs_k, Int())

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
        self.assertEqual(99, getargs_h(Int()))

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
        self.assertEqual(99, getargs_i(Int()))

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
        self.assertEqual(99, getargs_l(Int()))

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
        self.assertRaises(TypeError, getargs_n, Int())

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
        self.assertEqual(99, getargs_L(Int()))

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
        self.assertRaises(TypeError, getargs_K, Int())
        self.assertEqual(ULLONG_MAX, getargs_K(ULLONG_MAX))
        self.assertEqual(0, getargs_K(0))
        self.assertEqual(0, getargs_K(ULLONG_MAX+1))

        self.assertEqual(42, getargs_K(42))

        self.assertEqual(VERY_LARGE & ULLONG_MAX, getargs_K(VERY_LARGE))


class Tuple_TestCase(unittest.TestCase):
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

class Bytes_TestCase(unittest.TestCase):
    def test_s(self):
        from _testcapi import getargs_s
        self.assertEqual(getargs_s('abc\xe9'), b'abc\xc3\xa9')
        self.assertRaises(TypeError, getargs_s, 'nul:\0')
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
        self.assertRaises(TypeError, getargs_z, 'nul:\0')
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

    def test_y(self):
        from _testcapi import getargs_y
        self.assertRaises(TypeError, getargs_y, 'abc\xe9')
        self.assertEqual(getargs_y(b'bytes'), b'bytes')
        self.assertRaises(TypeError, getargs_y, b'nul:\0')
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
        self.assertEqual(getargs_w_star(bytearray(b'bytearray')), b'[ytearra]')
        self.assertEqual(getargs_w_star(memoryview(b'memoryview')), b'[emoryvie]')
        self.assertRaises(TypeError, getargs_w_star, None)


class Unicode_TestCase(unittest.TestCase):
    def test_u(self):
        from _testcapi import getargs_u
        self.assertEqual(getargs_u('abc\xe9'), 'abc\xe9')
        self.assertRaises(TypeError, getargs_u, 'nul:\0')
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
        self.assertRaises(TypeError, getargs_Z, 'nul:\0')
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


def test_main():
    tests = [
        Signed_TestCase,
        Unsigned_TestCase,
        Tuple_TestCase,
        Keywords_TestCase,
        Bytes_TestCase,
        Unicode_TestCase,
    ]
    try:
        from _testcapi import getargs_L, getargs_K
    except ImportError:
        pass # PY_LONG_LONG not available
    else:
        tests.append(LongLong_TestCase)
    support.run_unittest(*tests)

if __name__ == "__main__":
    test_main()
