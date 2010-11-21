from _testcapi import _test_structmembersType, \
    CHAR_MAX, CHAR_MIN, UCHAR_MAX, \
    SHRT_MAX, SHRT_MIN, USHRT_MAX, \
    INT_MAX, INT_MIN, UINT_MAX, \
    LONG_MAX, LONG_MIN, ULONG_MAX, \
    LLONG_MAX, LLONG_MIN, ULLONG_MAX

import unittest
from test import test_support

ts=_test_structmembersType(False, 1, 2, 3, 4, 5, 6, 7, 8,
                          9.99999, 10.1010101010, "hi")

class ReadWriteTests(unittest.TestCase):

    def test_bool(self):
        ts.T_BOOL = True
        self.assertEqual(ts.T_BOOL, True)
        ts.T_BOOL = False
        self.assertEqual(ts.T_BOOL, False)
        self.assertRaises(TypeError, setattr, ts, 'T_BOOL', 1)

    def test_byte(self):
        ts.T_BYTE = CHAR_MAX
        self.assertEqual(ts.T_BYTE, CHAR_MAX)
        ts.T_BYTE = CHAR_MIN
        self.assertEqual(ts.T_BYTE, CHAR_MIN)
        ts.T_UBYTE = UCHAR_MAX
        self.assertEqual(ts.T_UBYTE, UCHAR_MAX)

    def test_short(self):
        ts.T_SHORT = SHRT_MAX
        self.assertEqual(ts.T_SHORT, SHRT_MAX)
        ts.T_SHORT = SHRT_MIN
        self.assertEqual(ts.T_SHORT, SHRT_MIN)
        ts.T_USHORT = USHRT_MAX
        self.assertEqual(ts.T_USHORT, USHRT_MAX)

    def test_int(self):
        ts.T_INT = INT_MAX
        self.assertEqual(ts.T_INT, INT_MAX)
        ts.T_INT = INT_MIN
        self.assertEqual(ts.T_INT, INT_MIN)
        ts.T_UINT = UINT_MAX
        self.assertEqual(ts.T_UINT, UINT_MAX)

    def test_long(self):
        ts.T_LONG = LONG_MAX
        self.assertEqual(ts.T_LONG, LONG_MAX)
        ts.T_LONG = LONG_MIN
        self.assertEqual(ts.T_LONG, LONG_MIN)
        ts.T_ULONG = ULONG_MAX
        self.assertEqual(ts.T_ULONG, ULONG_MAX)

    @unittest.skipUnless(hasattr(ts, "T_LONGLONG"), "long long not present")
    def test_longlong(self):
        ts.T_LONGLONG = LLONG_MAX
        self.assertEqual(ts.T_LONGLONG, LLONG_MAX)
        ts.T_LONGLONG = LLONG_MIN
        self.assertEqual(ts.T_LONGLONG, LLONG_MIN)

        ts.T_ULONGLONG = ULLONG_MAX
        self.assertEqual(ts.T_ULONGLONG, ULLONG_MAX)

        ## make sure these will accept a plain int as well as a long
        ts.T_LONGLONG = 3
        self.assertEqual(ts.T_LONGLONG, 3)
        ts.T_ULONGLONG = 4
        self.assertEqual(ts.T_ULONGLONG, 4)

    def test_inplace_string(self):
        self.assertEqual(ts.T_STRING_INPLACE, "hi")
        self.assertRaises(TypeError, setattr, ts, "T_STRING_INPLACE", "s")
        self.assertRaises(TypeError, delattr, ts, "T_STRING_INPLACE")


class TestWarnings(unittest.TestCase):

    def test_byte_max(self):
        with test_support.check_warnings(('', RuntimeWarning)):
            ts.T_BYTE = CHAR_MAX+1

    def test_byte_min(self):
        with test_support.check_warnings(('', RuntimeWarning)):
            ts.T_BYTE = CHAR_MIN-1

    def test_ubyte_max(self):
        with test_support.check_warnings(('', RuntimeWarning)):
            ts.T_UBYTE = UCHAR_MAX+1

    def test_short_max(self):
        with test_support.check_warnings(('', RuntimeWarning)):
            ts.T_SHORT = SHRT_MAX+1

    def test_short_min(self):
        with test_support.check_warnings(('', RuntimeWarning)):
            ts.T_SHORT = SHRT_MIN-1

    def test_ushort_max(self):
        with test_support.check_warnings(('', RuntimeWarning)):
            ts.T_USHORT = USHRT_MAX+1


def test_main(verbose=None):
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main(verbose=True)
