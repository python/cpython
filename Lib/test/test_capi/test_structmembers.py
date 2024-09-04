import unittest
from test.support import import_helper
from test.support import warnings_helper

# Skip this test if the _testcapi module isn't available.
import_helper.import_module('_testcapi')
from _testcapi import _test_structmembersType, \
    CHAR_MAX, CHAR_MIN, UCHAR_MAX, \
    SHRT_MAX, SHRT_MIN, USHRT_MAX, \
    INT_MAX, INT_MIN, UINT_MAX, \
    LONG_MAX, LONG_MIN, ULONG_MAX, \
    LLONG_MAX, LLONG_MIN, ULLONG_MAX, \
    PY_SSIZE_T_MAX, PY_SSIZE_T_MIN


class Index:
    def __init__(self, value):
        self.value = value
    def __index__(self):
        return self.value


ts=_test_structmembersType(False,  # T_BOOL
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
                          "hi" # T_STRING_INPLACE
                          )

class ReadWriteTests(unittest.TestCase):

    def _test_write(self, name, value, expected=None):
        if expected is None:
            expected = value
        setattr(ts, name, value)
        self.assertEqual(getattr(ts, name), expected)

    def _test_warn(self, name, value, expected=None):
        self.assertWarns(RuntimeWarning, setattr, ts, name, value)
        if expected is not None:
            self.assertEqual(getattr(ts, name), expected)

    def _test_overflow(self, name, value):
        self.assertRaises(OverflowError, setattr, ts, name, value)

    def _test_int_range(self, name, minval, maxval, *, hardlimit=None,
                        indexlimit=None):
        if hardlimit is None:
            hardlimit = (minval, maxval)
        self._test_write(name, minval)
        self._test_write(name, maxval)
        hardminval, hardmaxval = hardlimit
        self._test_overflow(name, hardminval-1)
        self._test_overflow(name, hardmaxval+1)
        self._test_overflow(name, 2**1000)
        self._test_overflow(name, -2**1000)
        if hardminval < minval:
            self._test_warn(name, hardminval)
            self._test_warn(name, minval-1, maxval)
        if maxval < hardmaxval:
            self._test_warn(name, maxval+1, minval)
            self._test_warn(name, hardmaxval)

        if indexlimit is False:
            self.assertRaises(TypeError, setattr, ts, name, Index(minval))
            self.assertRaises(TypeError, setattr, ts, name, Index(maxval))
        else:
            self._test_write(name, Index(minval), minval)
            self._test_write(name, Index(maxval), maxval)
            self._test_overflow(name, Index(hardminval-1))
            self._test_overflow(name, Index(hardmaxval+1))
            self._test_overflow(name, Index(2**1000))
            self._test_overflow(name, Index(-2**1000))
            if hardminval < minval:
                self._test_warn(name, Index(hardminval))
                self._test_warn(name, Index(minval-1), maxval)
            if maxval < hardmaxval:
                self._test_warn(name, Index(maxval+1), minval)
                self._test_warn(name, Index(hardmaxval))

    def test_bool(self):
        ts.T_BOOL = True
        self.assertIs(ts.T_BOOL, True)
        ts.T_BOOL = False
        self.assertIs(ts.T_BOOL, False)
        self.assertRaises(TypeError, setattr, ts, 'T_BOOL', 1)
        self.assertRaises(TypeError, setattr, ts, 'T_BOOL', 0)
        self.assertRaises(TypeError, setattr, ts, 'T_BOOL', None)

    def test_byte(self):
        self._test_int_range('T_BYTE', CHAR_MIN, CHAR_MAX,
                             hardlimit=(LONG_MIN, LONG_MAX))
        self._test_int_range('T_UBYTE', 0, UCHAR_MAX,
                             hardlimit=(LONG_MIN, LONG_MAX))

    def test_short(self):
        self._test_int_range('T_SHORT', SHRT_MIN, SHRT_MAX,
                             hardlimit=(LONG_MIN, LONG_MAX))
        self._test_int_range('T_USHORT', 0, USHRT_MAX,
                             hardlimit=(LONG_MIN, LONG_MAX))

    def test_int(self):
        self._test_int_range('T_INT', INT_MIN, INT_MAX,
                             hardlimit=(LONG_MIN, LONG_MAX))
        self._test_int_range('T_UINT', 0, UINT_MAX,
                             hardlimit=(LONG_MIN, ULONG_MAX))

    def test_long(self):
        self._test_int_range('T_LONG', LONG_MIN, LONG_MAX)
        self._test_int_range('T_ULONG', 0, ULONG_MAX,
                             hardlimit=(LONG_MIN, ULONG_MAX))

    def test_py_ssize_t(self):
        self._test_int_range('T_PYSSIZET', PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, indexlimit=False)

    @unittest.skipUnless(hasattr(ts, "T_LONGLONG"), "long long not present")
    def test_longlong(self):
        self._test_int_range('T_LONGLONG', LLONG_MIN, LLONG_MAX)
        self._test_int_range('T_ULONGLONG', 0, ULLONG_MAX,
                             hardlimit=(LONG_MIN, ULLONG_MAX))

    def test_bad_assignments(self):
        integer_attributes = [
            'T_BOOL',
            'T_BYTE', 'T_UBYTE',
            'T_SHORT', 'T_USHORT',
            'T_INT', 'T_UINT',
            'T_LONG', 'T_ULONG',
            'T_LONGLONG', 'T_ULONGLONG',
            'T_PYSSIZET'
            ]

        # issue8014: this produced 'bad argument to internal function'
        # internal error
        for nonint in None, 3.2j, "full of eels", {}, []:
            for attr in integer_attributes:
                self.assertRaises(TypeError, setattr, ts, attr, nonint)

    def test_inplace_string(self):
        self.assertEqual(ts.T_STRING_INPLACE, "hi")
        self.assertRaises(TypeError, setattr, ts, "T_STRING_INPLACE", "s")
        self.assertRaises(TypeError, delattr, ts, "T_STRING_INPLACE")


if __name__ == "__main__":
    unittest.main()
