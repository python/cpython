import unittest
from test.support import import_helper
from test.support import warnings_helper

# Skip this test if the _testcapi module isn't available.
import_helper.import_module('_testcapi')
from _testcapi import (_test_structmembersType_OldAPI,
    _test_structmembersType_NewAPI,
    CHAR_MAX, CHAR_MIN, UCHAR_MAX,
    SHRT_MAX, SHRT_MIN, USHRT_MAX,
    INT_MAX, INT_MIN, UINT_MAX,
    LONG_MAX, LONG_MIN, ULONG_MAX,
    LLONG_MAX, LLONG_MIN, ULLONG_MAX,
    PY_SSIZE_T_MAX, PY_SSIZE_T_MIN,
    )

# There are two classes: one using <structmember.h> and another using
# `Py_`-prefixed API. They should behave the same in Python

def _make_test_object(cls):
    return cls(False,  # T_BOOL
               1,      # T_BYTE
               2,      # T_UBYTE
               3,      # T_SHORT
               4,      # T_USHORT
               5,      # T_INT
               6,      # T_UINT
               7,      # T_LONG
               8,      # T_ULONG
               23,     # T_PYSSIZET
               9.99999,# T_FLOAT
               10.1010101010, # T_DOUBLE
               "hi",   # T_STRING_INPLACE
               )


class ReadWriteTests:
    def setUp(self):
        self.ts = _make_test_object(self.cls)

    def test_bool(self):
        ts = self.ts
        ts.T_BOOL = True
        self.assertEqual(ts.T_BOOL, True)
        ts.T_BOOL = False
        self.assertEqual(ts.T_BOOL, False)
        self.assertRaises(TypeError, setattr, ts, 'T_BOOL', 1)

    def test_byte(self):
        ts = self.ts
        ts.T_BYTE = CHAR_MAX
        self.assertEqual(ts.T_BYTE, CHAR_MAX)
        ts.T_BYTE = CHAR_MIN
        self.assertEqual(ts.T_BYTE, CHAR_MIN)
        ts.T_UBYTE = UCHAR_MAX
        self.assertEqual(ts.T_UBYTE, UCHAR_MAX)

    def test_short(self):
        ts = self.ts
        ts.T_SHORT = SHRT_MAX
        self.assertEqual(ts.T_SHORT, SHRT_MAX)
        ts.T_SHORT = SHRT_MIN
        self.assertEqual(ts.T_SHORT, SHRT_MIN)
        ts.T_USHORT = USHRT_MAX
        self.assertEqual(ts.T_USHORT, USHRT_MAX)

    def test_int(self):
        ts = self.ts
        ts.T_INT = INT_MAX
        self.assertEqual(ts.T_INT, INT_MAX)
        ts.T_INT = INT_MIN
        self.assertEqual(ts.T_INT, INT_MIN)
        ts.T_UINT = UINT_MAX
        self.assertEqual(ts.T_UINT, UINT_MAX)

    def test_long(self):
        ts = self.ts
        ts.T_LONG = LONG_MAX
        self.assertEqual(ts.T_LONG, LONG_MAX)
        ts.T_LONG = LONG_MIN
        self.assertEqual(ts.T_LONG, LONG_MIN)
        ts.T_ULONG = ULONG_MAX
        self.assertEqual(ts.T_ULONG, ULONG_MAX)

    def test_py_ssize_t(self):
        ts = self.ts
        ts.T_PYSSIZET = PY_SSIZE_T_MAX
        self.assertEqual(ts.T_PYSSIZET, PY_SSIZE_T_MAX)
        ts.T_PYSSIZET = PY_SSIZE_T_MIN
        self.assertEqual(ts.T_PYSSIZET, PY_SSIZE_T_MIN)

    def test_longlong(self):
        ts = self.ts
        if not hasattr(ts, "T_LONGLONG"):
            self.skipTest("long long not present")

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

    def test_bad_assignments(self):
        ts = self.ts
        integer_attributes = [
            'T_BOOL',
            'T_BYTE', 'T_UBYTE',
            'T_SHORT', 'T_USHORT',
            'T_INT', 'T_UINT',
            'T_LONG', 'T_ULONG',
            'T_PYSSIZET'
            ]
        if hasattr(ts, 'T_LONGLONG'):
            integer_attributes.extend(['T_LONGLONG', 'T_ULONGLONG'])

        # issue8014: this produced 'bad argument to internal function'
        # internal error
        for nonint in None, 3.2j, "full of eels", {}, []:
            for attr in integer_attributes:
                self.assertRaises(TypeError, setattr, ts, attr, nonint)

    def test_inplace_string(self):
        ts = self.ts
        self.assertEqual(ts.T_STRING_INPLACE, "hi")
        self.assertRaises(TypeError, setattr, ts, "T_STRING_INPLACE", "s")
        self.assertRaises(TypeError, delattr, ts, "T_STRING_INPLACE")

class ReadWriteTests_OldAPI(ReadWriteTests, unittest.TestCase):
    cls = _test_structmembersType_OldAPI

class ReadWriteTests_NewAPI(ReadWriteTests, unittest.TestCase):
    cls = _test_structmembersType_NewAPI

class TestWarnings:
    def setUp(self):
        self.ts = _make_test_object(self.cls)

    def test_byte_max(self):
        ts = self.ts
        with warnings_helper.check_warnings(('', RuntimeWarning)):
            ts.T_BYTE = CHAR_MAX+1

    def test_byte_min(self):
        ts = self.ts
        with warnings_helper.check_warnings(('', RuntimeWarning)):
            ts.T_BYTE = CHAR_MIN-1

    def test_ubyte_max(self):
        ts = self.ts
        with warnings_helper.check_warnings(('', RuntimeWarning)):
            ts.T_UBYTE = UCHAR_MAX+1

    def test_short_max(self):
        ts = self.ts
        with warnings_helper.check_warnings(('', RuntimeWarning)):
            ts.T_SHORT = SHRT_MAX+1

    def test_short_min(self):
        ts = self.ts
        with warnings_helper.check_warnings(('', RuntimeWarning)):
            ts.T_SHORT = SHRT_MIN-1

    def test_ushort_max(self):
        ts = self.ts
        with warnings_helper.check_warnings(('', RuntimeWarning)):
            ts.T_USHORT = USHRT_MAX+1

class TestWarnings_OldAPI(TestWarnings, unittest.TestCase):
    cls = _test_structmembersType_OldAPI

class TestWarnings_NewAPI(TestWarnings, unittest.TestCase):
    cls = _test_structmembersType_NewAPI


if __name__ == "__main__":
    unittest.main()
