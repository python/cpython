import unittest
import sys

from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')

NULL = None

class IntSubclass(int):
    pass

class Index:
    def __init__(self, value):
        self.value = value

    def __index__(self):
        return self.value

# use __index__(), not __int__()
class MyIndexAndInt:
    def __index__(self):
        return 10
    def __int__(self):
        return 22


class LongTests(unittest.TestCase):

    def test_compact(self):
        for n in {
            # Edge cases
            *(2**n for n in range(66)),
            *(-2**n for n in range(66)),
            *(2**n - 1 for n in range(66)),
            *(-2**n + 1 for n in range(66)),
            # Essentially random
            *(37**n for n in range(14)),
            *(-37**n for n in range(14)),
        }:
            with self.subTest(n=n):
                is_compact, value = _testcapi.call_long_compact_api(n)
                if is_compact:
                    self.assertEqual(n, value)

    def test_compact_known(self):
        # Sanity-check some implementation details (we don't guarantee
        # that these are/aren't compact)
        self.assertEqual(_testcapi.call_long_compact_api(-1), (True, -1))
        self.assertEqual(_testcapi.call_long_compact_api(0), (True, 0))
        self.assertEqual(_testcapi.call_long_compact_api(256), (True, 256))
        self.assertEqual(_testcapi.call_long_compact_api(sys.maxsize),
                         (False, -1))

    def test_long_check(self):
        # Test PyLong_Check()
        check = _testcapi.pylong_check
        self.assertTrue(check(1))
        self.assertTrue(check(123456789012345678901234567890))
        self.assertTrue(check(-1))
        self.assertTrue(check(True))
        self.assertTrue(check(IntSubclass(1)))
        self.assertFalse(check(1.0))
        self.assertFalse(check(object()))
        # CRASHES check(NULL)

    def test_long_checkexact(self):
        # Test PyLong_CheckExact()
        check = _testcapi.pylong_checkexact
        self.assertTrue(check(1))
        self.assertTrue(check(123456789012345678901234567890))
        self.assertTrue(check(-1))
        self.assertFalse(check(True))
        self.assertFalse(check(IntSubclass(1)))
        self.assertFalse(check(1.0))
        self.assertFalse(check(object()))
        # CRASHES check(NULL)

    def test_long_fromdouble(self):
        # Test PyLong_FromDouble()
        fromdouble = _testcapi.pylong_fromdouble
        float_max = sys.float_info.max
        for value in (5.0, 5.1, 5.9, -5.1, -5.9, 0.0, -0.0, float_max, -float_max):
            with self.subTest(value=value):
                self.assertEqual(fromdouble(value), int(value))
        self.assertRaises(OverflowError, fromdouble, float('inf'))
        self.assertRaises(OverflowError, fromdouble, float('-inf'))
        self.assertRaises(ValueError, fromdouble, float('nan'))

    def test_long_fromvoidptr(self):
        # Test PyLong_FromVoidPtr()
        fromvoidptr = _testcapi.pylong_fromvoidptr
        obj = object()
        x = fromvoidptr(obj)
        y = fromvoidptr(NULL)
        self.assertIsInstance(x, int)
        self.assertGreaterEqual(x, 0)
        self.assertIsInstance(y, int)
        self.assertEqual(y, 0)
        self.assertNotEqual(x, y)

    def test_long_fromstring(self):
        # Test PyLong_FromString()
        fromstring = _testcapi.pylong_fromstring
        self.assertEqual(fromstring(b'123', 10), (123, 3))
        self.assertEqual(fromstring(b'cafe', 16), (0xcafe, 4))
        self.assertEqual(fromstring(b'xyz', 36), (44027, 3))
        self.assertEqual(fromstring(b'123', 0), (123, 3))
        self.assertEqual(fromstring(b'0xcafe', 0), (0xcafe, 6))
        self.assertRaises(ValueError, fromstring, b'cafe', 0)
        self.assertEqual(fromstring(b'-123', 10), (-123, 4))
        self.assertEqual(fromstring(b' -123 ', 10), (-123, 6))
        self.assertEqual(fromstring(b'1_23', 10), (123, 4))
        self.assertRaises(ValueError, fromstring, b'- 123', 10)
        self.assertRaises(ValueError, fromstring, b'', 10)

        self.assertRaises(ValueError, fromstring, b'123', 1)
        self.assertRaises(ValueError, fromstring, b'123', -1)
        self.assertRaises(ValueError, fromstring, b'123', 37)

        self.assertRaises(ValueError, fromstring, '١٢٣٤٥٦٧٨٩٠'.encode(), 0)
        self.assertRaises(ValueError, fromstring, '١٢٣٤٥٦٧٨٩٠'.encode(), 16)

        self.assertEqual(fromstring(b'123\x00', 0), (123, 3))
        self.assertEqual(fromstring(b'123\x00456', 0), (123, 3))
        self.assertEqual(fromstring(b'123\x00', 16), (0x123, 3))
        self.assertEqual(fromstring(b'123\x00456', 16), (0x123, 3))

        # CRASHES fromstring(NULL, 0)
        # CRASHES fromstring(NULL, 16)

    def test_long_fromunicodeobject(self):
        # Test PyLong_FromUnicodeObject()
        fromunicodeobject = _testcapi.pylong_fromunicodeobject
        self.assertEqual(fromunicodeobject('123', 10), 123)
        self.assertEqual(fromunicodeobject('cafe', 16), 0xcafe)
        self.assertEqual(fromunicodeobject('xyz', 36), 44027)
        self.assertEqual(fromunicodeobject('123', 0), 123)
        self.assertEqual(fromunicodeobject('0xcafe', 0), 0xcafe)
        self.assertRaises(ValueError, fromunicodeobject, 'cafe', 0)
        self.assertEqual(fromunicodeobject('-123', 10), -123)
        self.assertEqual(fromunicodeobject(' -123 ', 10), -123)
        self.assertEqual(fromunicodeobject('1_23', 10), 123)
        self.assertRaises(ValueError, fromunicodeobject, '- 123', 10)
        self.assertRaises(ValueError, fromunicodeobject, '', 10)

        self.assertRaises(ValueError, fromunicodeobject, '123', 1)
        self.assertRaises(ValueError, fromunicodeobject, '123', -1)
        self.assertRaises(ValueError, fromunicodeobject, '123', 37)

        self.assertEqual(fromunicodeobject('١٢٣٤٥٦٧٨٩٠', 0), 1234567890)
        self.assertEqual(fromunicodeobject('١٢٣٤٥٦٧٨٩٠', 16), 0x1234567890)

        self.assertRaises(ValueError, fromunicodeobject, '123\x00', 0)
        self.assertRaises(ValueError, fromunicodeobject, '123\x00456', 0)
        self.assertRaises(ValueError, fromunicodeobject, '123\x00', 16)
        self.assertRaises(ValueError, fromunicodeobject, '123\x00456', 16)

        # CRASHES fromunicodeobject(NULL, 0)
        # CRASHES fromunicodeobject(NULL, 16)

    def check_long_asint(self, func, min_val, max_val, *,
                         use_index=True,
                         mask=False,
                         negative_value_error=OverflowError):
        # round trip (object -> C integer -> object)
        values = (0, 1, 1234, max_val)
        if min_val < 0:
            values += (-1, min_val)
        for value in values:
            with self.subTest(value=value):
                self.assertEqual(func(value), value)
                self.assertEqual(func(IntSubclass(value)), value)
                if use_index:
                    self.assertEqual(func(Index(value)), value)

        if use_index:
            self.assertEqual(func(MyIndexAndInt()), 10)
        else:
            self.assertRaises(TypeError, func, Index(42))
            self.assertRaises(TypeError, func, MyIndexAndInt())

        if mask:
            self.assertEqual(func(min_val - 1), max_val)
            self.assertEqual(func(max_val + 1), min_val)
            self.assertEqual(func(-1 << 1000), 0)
            self.assertEqual(func(1 << 1000), 0)
        else:
            self.assertRaises(negative_value_error, func, min_val - 1)
            self.assertRaises(negative_value_error, func, -1 << 1000)
            self.assertRaises(OverflowError, func, max_val + 1)
            self.assertRaises(OverflowError, func, 1 << 1000)
        self.assertRaises(TypeError, func, 1.0)
        self.assertRaises(TypeError, func, b'2')
        self.assertRaises(TypeError, func, '3')
        self.assertRaises(SystemError, func, NULL)

    def check_long_asintandoverflow(self, func, min_val, max_val):
        # round trip (object -> C integer -> object)
        for value in (min_val, max_val, -1, 0, 1, 1234):
            with self.subTest(value=value):
                self.assertEqual(func(value), (value, 0))
                self.assertEqual(func(IntSubclass(value)), (value, 0))
                self.assertEqual(func(Index(value)), (value, 0))

        self.assertEqual(func(MyIndexAndInt()), (10, 0))

        self.assertEqual(func(min_val - 1), (-1, -1))
        self.assertEqual(func(max_val + 1), (-1, +1))

        # CRASHES func(1.0)
        # CRASHES func(NULL)

    def test_long_aslong(self):
        # Test PyLong_AsLong() and PyLong_FromLong()
        aslong = _testcapi.pylong_aslong
        from _testcapi import LONG_MIN, LONG_MAX
        self.check_long_asint(aslong, LONG_MIN, LONG_MAX)

    def test_long_aslongandoverflow(self):
        # Test PyLong_AsLongAndOverflow()
        aslongandoverflow = _testcapi.pylong_aslongandoverflow
        from _testcapi import LONG_MIN, LONG_MAX
        self.check_long_asintandoverflow(aslongandoverflow, LONG_MIN, LONG_MAX)

    def test_long_asunsignedlong(self):
        # Test PyLong_AsUnsignedLong() and PyLong_FromUnsignedLong()
        asunsignedlong = _testcapi.pylong_asunsignedlong
        from _testcapi import ULONG_MAX
        self.check_long_asint(asunsignedlong, 0, ULONG_MAX,
                                      use_index=False)

    def test_long_asunsignedlongmask(self):
        # Test PyLong_AsUnsignedLongMask()
        asunsignedlongmask = _testcapi.pylong_asunsignedlongmask
        from _testcapi import ULONG_MAX
        self.check_long_asint(asunsignedlongmask, 0, ULONG_MAX, mask=True)

    def test_long_aslonglong(self):
        # Test PyLong_AsLongLong() and PyLong_FromLongLong()
        aslonglong = _testcapi.pylong_aslonglong
        from _testcapi import LLONG_MIN, LLONG_MAX
        self.check_long_asint(aslonglong, LLONG_MIN, LLONG_MAX)

    def test_long_aslonglongandoverflow(self):
        # Test PyLong_AsLongLongAndOverflow()
        aslonglongandoverflow = _testcapi.pylong_aslonglongandoverflow
        from _testcapi import LLONG_MIN, LLONG_MAX
        self.check_long_asintandoverflow(aslonglongandoverflow, LLONG_MIN, LLONG_MAX)

    def test_long_asunsignedlonglong(self):
        # Test PyLong_AsUnsignedLongLong() and PyLong_FromUnsignedLongLong()
        asunsignedlonglong = _testcapi.pylong_asunsignedlonglong
        from _testcapi import ULLONG_MAX
        self.check_long_asint(asunsignedlonglong, 0, ULLONG_MAX, use_index=False)

    def test_long_asunsignedlonglongmask(self):
        # Test PyLong_AsUnsignedLongLongMask()
        asunsignedlonglongmask = _testcapi.pylong_asunsignedlonglongmask
        from _testcapi import ULLONG_MAX
        self.check_long_asint(asunsignedlonglongmask, 0, ULLONG_MAX, mask=True)

    def test_long_as_ssize_t(self):
        # Test PyLong_AsSsize_t() and PyLong_FromSsize_t()
        as_ssize_t = _testcapi.pylong_as_ssize_t
        from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX
        self.check_long_asint(as_ssize_t, PY_SSIZE_T_MIN, PY_SSIZE_T_MAX,
                              use_index=False)

    def test_long_as_size_t(self):
        # Test PyLong_AsSize_t() and PyLong_FromSize_t()
        as_size_t = _testcapi.pylong_as_size_t
        from _testcapi import SIZE_MAX
        self.check_long_asint(as_size_t, 0, SIZE_MAX, use_index=False)

    def test_long_asdouble(self):
        # Test PyLong_AsDouble()
        asdouble = _testcapi.pylong_asdouble
        MAX = int(sys.float_info.max)
        for value in (-MAX, MAX, -1, 0, 1, 1234):
            with self.subTest(value=value):
                self.assertEqual(asdouble(value), float(value))
                self.assertIsInstance(asdouble(value), float)

        self.assertEqual(asdouble(IntSubclass(42)), 42.0)
        self.assertRaises(TypeError, asdouble, Index(42))
        self.assertRaises(TypeError, asdouble, MyIndexAndInt())

        self.assertRaises(OverflowError, asdouble, 2 * MAX)
        self.assertRaises(OverflowError, asdouble, -2 * MAX)
        self.assertRaises(TypeError, asdouble, 1.0)
        self.assertRaises(TypeError, asdouble, b'2')
        self.assertRaises(TypeError, asdouble, '3')
        self.assertRaises(SystemError, asdouble, NULL)

    def test_long_asvoidptr(self):
        # Test PyLong_AsVoidPtr()
        fromvoidptr = _testcapi.pylong_fromvoidptr
        asvoidptr = _testcapi.pylong_asvoidptr
        obj = object()
        x = fromvoidptr(obj)
        y = fromvoidptr(NULL)
        self.assertIs(asvoidptr(x), obj)
        self.assertIs(asvoidptr(y), NULL)
        self.assertIs(asvoidptr(IntSubclass(x)), obj)

        # negative values
        M = (1 << _testcapi.SIZEOF_VOID_P * 8)
        if x >= M//2:
            self.assertIs(asvoidptr(x - M), obj)
        if y >= M//2:
            self.assertIs(asvoidptr(y - M), NULL)

        self.assertRaises(TypeError, asvoidptr, Index(x))
        self.assertRaises(TypeError, asvoidptr, object())
        self.assertRaises(OverflowError, asvoidptr, 2**1000)
        self.assertRaises(OverflowError, asvoidptr, -2**1000)
        # CRASHES asvoidptr(NULL)

    def test_long_aspid(self):
        # Test PyLong_AsPid()
        aspid = _testcapi.pylong_aspid
        from _testcapi import SIZEOF_PID_T
        bits = 8 * SIZEOF_PID_T
        PID_T_MIN = -2**(bits-1)
        PID_T_MAX = 2**(bits-1) - 1
        self.check_long_asint(aspid, PID_T_MIN, PID_T_MAX)


if __name__ == "__main__":
    unittest.main()
