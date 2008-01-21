from _testcapi import test_structmembersType, \
    CHAR_MAX, CHAR_MIN, UCHAR_MAX, \
    SHRT_MAX, SHRT_MIN, USHRT_MAX, \
    INT_MAX, INT_MIN, UINT_MAX, \
    LONG_MAX, LONG_MIN, ULONG_MAX, \
    LLONG_MAX, LLONG_MIN, ULLONG_MAX

import warnings, exceptions, unittest, sys
from test import test_support

ts=test_structmembersType(False, 1, 2, 3, 4, 5, 6, 7, 8,
                          9.99999, 10.1010101010)

class ReadWriteTests(unittest.TestCase):
    def test_types(self):
        ts.T_BOOL = True
        self.assertEquals(ts.T_BOOL, True)
        ts.T_BOOL = False
        self.assertEquals(ts.T_BOOL, False)
        self.assertRaises(TypeError, setattr, ts, 'T_BOOL', 1)

        ts.T_BYTE = CHAR_MAX
        self.assertEquals(ts.T_BYTE, CHAR_MAX)
        ts.T_BYTE = CHAR_MIN
        self.assertEquals(ts.T_BYTE, CHAR_MIN)
        ts.T_UBYTE = UCHAR_MAX
        self.assertEquals(ts.T_UBYTE, UCHAR_MAX)

        ts.T_SHORT = SHRT_MAX
        self.assertEquals(ts.T_SHORT, SHRT_MAX)
        ts.T_SHORT = SHRT_MIN
        self.assertEquals(ts.T_SHORT, SHRT_MIN)
        ts.T_USHORT = USHRT_MAX
        self.assertEquals(ts.T_USHORT, USHRT_MAX)

        ts.T_INT = INT_MAX
        self.assertEquals(ts.T_INT, INT_MAX)
        ts.T_INT = INT_MIN
        self.assertEquals(ts.T_INT, INT_MIN)
        ts.T_UINT = UINT_MAX
        self.assertEquals(ts.T_UINT, UINT_MAX)

        ts.T_LONG = LONG_MAX
        self.assertEquals(ts.T_LONG, LONG_MAX)
        ts.T_LONG = LONG_MIN
        self.assertEquals(ts.T_LONG, LONG_MIN)
        ts.T_ULONG = ULONG_MAX
        self.assertEquals(ts.T_ULONG, ULONG_MAX)

        ## T_LONGLONG and T_ULONGLONG may not be present on some platforms
        if hasattr(ts, 'T_LONGLONG'):
            ts.T_LONGLONG = LLONG_MAX
            self.assertEquals(ts.T_LONGLONG, LLONG_MAX)
            ts.T_LONGLONG = LLONG_MIN
            self.assertEquals(ts.T_LONGLONG, LLONG_MIN)

            ts.T_ULONGLONG = ULLONG_MAX
            self.assertEquals(ts.T_ULONGLONG, ULLONG_MAX)

            ## make sure these will accept a plain int as well as a long
            ts.T_LONGLONG = 3
            self.assertEquals(ts.T_LONGLONG, 3)
            ts.T_ULONGLONG = 4
            self.assertEquals(ts.T_ULONGLONG, 4)


class TestWarnings(unittest.TestCase):
    def has_warned(self, w):
        self.assertEqual(w.category, RuntimeWarning)

    def test_byte_max(self):
        with test_support.catch_warning() as w:
            ts.T_BYTE = CHAR_MAX+1
            self.has_warned(w)

    def test_byte_min(self):
        with test_support.catch_warning() as w:
            ts.T_BYTE = CHAR_MIN-1
            self.has_warned(w)

    def test_ubyte_max(self):
        with test_support.catch_warning() as w:
            ts.T_UBYTE = UCHAR_MAX+1
            self.has_warned(w)

    def test_short_max(self):
        with test_support.catch_warning() as w:
            ts.T_SHORT = SHRT_MAX+1
            self.has_warned(w)

    def test_short_min(self):
        with test_support.catch_warning() as w:
            ts.T_SHORT = SHRT_MIN-1
            self.has_warned(w)

    def test_ushort_max(self):
        with test_support.catch_warning() as w:
            ts.T_USHORT = USHRT_MAX+1
            self.has_warned(w)



def test_main(verbose=None):
    # Obscure hack so that this test passes after reloads or repeated calls
    # to test_main (regrtest -R).
    if '__warningregistry__' in globals():
        del globals()['__warningregistry__']
    if hasattr(sys, '__warningregistry__'):
        del sys.__warningregistry__
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main(verbose=True)
