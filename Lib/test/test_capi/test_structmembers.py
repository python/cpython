import unittest
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
import_helper.import_module('_testcapi')
from _testcapi import (_test_structmembersType_OldAPI,
    _test_structmembersType_NewAPI,
    CHAR_MAX, CHAR_MIN, UCHAR_MAX,
    SHRT_MAX, SHRT_MIN, USHRT_MAX,
    INT_MAX, INT_MIN, UINT_MAX,
    LONG_MAX, LONG_MIN, ULONG_MAX,
    LLONG_MAX, LLONG_MIN, ULLONG_MAX,
    PY_SSIZE_T_MAX, PY_SSIZE_T_MIN, SIZE_MAX,
    SIZEOF_INTMAX_T, SIZEOF_INTPTR_T, SIZEOF_PTRDIFF_T, SIZEOF_OFF_T,
    SIZEOF_PID_T, SIZEOF_INT,
    )


class Index:
    def __init__(self, value):
        self.value = value
    def __index__(self):
        return self.value
    def __repr__(self):
        return f'Index({self.value!r})'

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
               12,     # T_LONGLONG
               13,     # T_ULONGLONG
               b"c",   # T_CHAR
               )


class ReadWriteTests:
    def setUp(self):
        self.ts = _make_test_object(self.cls)

    def _test_write(self, name, value, expected=None):
        if expected is None:
            expected = value
        ts = self.ts
        setattr(ts, name, value)
        self.assertEqual(getattr(ts, name), expected)

    def _test_warn(self, name, value, expected=None):
        ts = self.ts
        self.assertWarns(RuntimeWarning, setattr, ts, name, value)
        if expected is not None:
            self.assertEqual(getattr(ts, name), expected)

    def _test_overflow(self, name, value, error=OverflowError):
        ts = self.ts
        self.assertRaises(error, setattr, ts, name, value)

    def _test_int_range(self, name, minval, maxval, *, hardlimit=None,
                        indexlimit=None, negvalueerror=OverflowError, wrap=False):
        if hardlimit is None:
            hardlimit = (minval, maxval)
        ts = self.ts
        self._test_write(name, minval)
        self._test_write(name, maxval, -1 if wrap else maxval)
        hardminval, hardmaxval = hardlimit
        self._test_overflow(name, hardminval-1, error=negvalueerror)
        self._test_overflow(name, hardmaxval+1)
        self._test_overflow(name, 2**1000)
        self._test_overflow(name, -2**1000, error=negvalueerror)
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
            self._test_write(name, Index(maxval), -1 if wrap else maxval)
            self._test_overflow(name, Index(hardminval-1), error=negvalueerror)
            self._test_overflow(name, Index(hardmaxval+1))
            self._test_overflow(name, Index(2**1000))
            self._test_overflow(name, Index(-2**1000), error=negvalueerror)
            if hardminval < minval:
                self._test_warn(name, Index(hardminval))
                self._test_warn(name, Index(minval-1), maxval)
            if maxval < hardmaxval:
                self._test_warn(name, Index(maxval+1), minval)
                self._test_warn(name, Index(hardmaxval))

    def test_bool(self):
        ts = self.ts
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

    def test_longlong(self):
        self._test_int_range('T_LONGLONG', LLONG_MIN, LLONG_MAX)
        self._test_int_range('T_ULONGLONG', 0, ULLONG_MAX,
                             hardlimit=(LONG_MIN, ULLONG_MAX))

    def test_bad_assignments(self):
        ts = self.ts
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
        ts = self.ts
        self.assertEqual(ts.T_STRING_INPLACE, "hi")
        self.assertRaises(TypeError, setattr, ts, "T_STRING_INPLACE", "s")
        self.assertRaises(TypeError, delattr, ts, "T_STRING_INPLACE")

    def test_char(self):
        ts = self.ts
        self.assertEqual(ts.T_CHAR, "c")
        ts.T_CHAR = "z"
        self.assertEqual(ts.T_CHAR, "z")
        self.assertRaises(TypeError, setattr, ts, "T_CHAR", "")
        self.assertRaises(TypeError, setattr, ts, "T_CHAR", b"a")
        self.assertRaises(TypeError, setattr, ts, "T_CHAR", bytearray(b"b"))
        self.assertRaises(TypeError, delattr, ts, "T_STRING_INPLACE")

class ReadWriteTests_OldAPI(ReadWriteTests, unittest.TestCase):
    cls = _test_structmembersType_OldAPI

class ReadWriteTests_NewAPI(ReadWriteTests, unittest.TestCase):
    cls = _test_structmembersType_NewAPI

    def test_size(self):
        self._test_int_range('T_SSIZE', PY_SSIZE_T_MIN, PY_SSIZE_T_MAX, indexlimit=False)
        self._test_int_range('T_SIZE', 0, SIZE_MAX, indexlimit=False)

    def test_int8(self):
        self._test_int_range('T_INT8', -2**7, 2**7-1)
        self._test_int_range('T_UINT8', 0, 2**8-1, negvalueerror=ValueError)
        self._test_int_range('T_XINT8', -2**7, 2**8-1, wrap=True)

    def test_int16(self):
        self._test_int_range('T_INT16', -2**15, 2**15-1)
        self._test_int_range('T_UINT16', 0, 2**16-1, negvalueerror=ValueError)
        self._test_int_range('T_XINT16', -2**15, 2**16-1, wrap=True)

    def test_int32(self):
        self._test_int_range('T_INT32', -2**31, 2**31-1)
        self._test_int_range('T_UINT32', 0, 2**32-1, negvalueerror=ValueError)
        self._test_int_range('T_XINT32', -2**31, 2**32-1, wrap=True)

    def test_int64(self):
        self._test_int_range('T_INT64', -2**63, 2**63-1)
        self._test_int_range('T_UINT64', 0, 2**64-1, negvalueerror=ValueError)
        self._test_int_range('T_XINT64', -2**63, 2**64-1, wrap=True)

    def test_intptr(self):
        bits = 8*SIZEOF_INTPTR_T
        self._test_int_range('T_INTPTR', -2**(bits-1), 2**(bits-1)-1)
        self._test_int_range('T_UINTPTR', 0, 2**bits-1, negvalueerror=ValueError)

    def test_off(self):
        bits = 8*SIZEOF_OFF_T
        self._test_int_range('T_OFF', -2**(bits-1), 2**(bits-1)-1)

    def test_pid(self):
        bits = 8*SIZEOF_PID_T
        self._test_int_range('T_PID', -2**(bits-1), 2**(bits-1)-1)


if __name__ == "__main__":
    unittest.main()
