import unittest
import sys
import test.support as support

from test.support import import_helper

# Skip this test if the _testcapi and _testlimitedcapi modules isn't available.
_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')

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
        check = _testlimitedcapi.pylong_check
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
        check = _testlimitedcapi.pylong_checkexact
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
        fromdouble = _testlimitedcapi.pylong_fromdouble
        float_max = sys.float_info.max
        for value in (5.0, 5.1, 5.9, -5.1, -5.9, 0.0, -0.0, float_max, -float_max):
            with self.subTest(value=value):
                self.assertEqual(fromdouble(value), int(value))
        self.assertRaises(OverflowError, fromdouble, float('inf'))
        self.assertRaises(OverflowError, fromdouble, float('-inf'))
        self.assertRaises(ValueError, fromdouble, float('nan'))

    def test_long_fromvoidptr(self):
        # Test PyLong_FromVoidPtr()
        fromvoidptr = _testlimitedcapi.pylong_fromvoidptr
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
        fromstring = _testlimitedcapi.pylong_fromstring
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

    def test_long_asint(self):
        # Test PyLong_AsInt()
        PyLong_AsInt = _testlimitedcapi.PyLong_AsInt
        from _testcapi import INT_MIN, INT_MAX
        self.check_long_asint(PyLong_AsInt, INT_MIN, INT_MAX)

    def test_long_aslong(self):
        # Test PyLong_AsLong() and PyLong_FromLong()
        aslong = _testlimitedcapi.pylong_aslong
        from _testcapi import LONG_MIN, LONG_MAX
        self.check_long_asint(aslong, LONG_MIN, LONG_MAX)

    def test_long_aslongandoverflow(self):
        # Test PyLong_AsLongAndOverflow()
        aslongandoverflow = _testlimitedcapi.pylong_aslongandoverflow
        from _testcapi import LONG_MIN, LONG_MAX
        self.check_long_asintandoverflow(aslongandoverflow, LONG_MIN, LONG_MAX)

    def test_long_asunsignedlong(self):
        # Test PyLong_AsUnsignedLong() and PyLong_FromUnsignedLong()
        asunsignedlong = _testlimitedcapi.pylong_asunsignedlong
        from _testcapi import ULONG_MAX
        self.check_long_asint(asunsignedlong, 0, ULONG_MAX,
                                      use_index=False)

    def test_long_asunsignedlongmask(self):
        # Test PyLong_AsUnsignedLongMask()
        asunsignedlongmask = _testlimitedcapi.pylong_asunsignedlongmask
        from _testcapi import ULONG_MAX
        self.check_long_asint(asunsignedlongmask, 0, ULONG_MAX, mask=True)

    def test_long_aslonglong(self):
        # Test PyLong_AsLongLong() and PyLong_FromLongLong()
        aslonglong = _testlimitedcapi.pylong_aslonglong
        from _testcapi import LLONG_MIN, LLONG_MAX
        self.check_long_asint(aslonglong, LLONG_MIN, LLONG_MAX)

    def test_long_aslonglongandoverflow(self):
        # Test PyLong_AsLongLongAndOverflow()
        aslonglongandoverflow = _testlimitedcapi.pylong_aslonglongandoverflow
        from _testcapi import LLONG_MIN, LLONG_MAX
        self.check_long_asintandoverflow(aslonglongandoverflow, LLONG_MIN, LLONG_MAX)

    def test_long_asunsignedlonglong(self):
        # Test PyLong_AsUnsignedLongLong() and PyLong_FromUnsignedLongLong()
        asunsignedlonglong = _testlimitedcapi.pylong_asunsignedlonglong
        from _testcapi import ULLONG_MAX
        self.check_long_asint(asunsignedlonglong, 0, ULLONG_MAX, use_index=False)

    def test_long_asunsignedlonglongmask(self):
        # Test PyLong_AsUnsignedLongLongMask()
        asunsignedlonglongmask = _testlimitedcapi.pylong_asunsignedlonglongmask
        from _testcapi import ULLONG_MAX
        self.check_long_asint(asunsignedlonglongmask, 0, ULLONG_MAX, mask=True)

    def test_long_as_ssize_t(self):
        # Test PyLong_AsSsize_t() and PyLong_FromSsize_t()
        as_ssize_t = _testlimitedcapi.pylong_as_ssize_t
        from _testcapi import PY_SSIZE_T_MIN, PY_SSIZE_T_MAX
        self.check_long_asint(as_ssize_t, PY_SSIZE_T_MIN, PY_SSIZE_T_MAX,
                              use_index=False)

    def test_long_as_size_t(self):
        # Test PyLong_AsSize_t() and PyLong_FromSize_t()
        as_size_t = _testlimitedcapi.pylong_as_size_t
        from _testcapi import SIZE_MAX
        self.check_long_asint(as_size_t, 0, SIZE_MAX, use_index=False)

    def test_long_asdouble(self):
        # Test PyLong_AsDouble()
        asdouble = _testlimitedcapi.pylong_asdouble
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
        fromvoidptr = _testlimitedcapi.pylong_fromvoidptr
        asvoidptr = _testlimitedcapi.pylong_asvoidptr
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

    def _test_long_aspid(self, aspid):
        # Test PyLong_AsPid()
        from _testcapi import SIZEOF_PID_T
        bits = 8 * SIZEOF_PID_T
        PID_T_MIN = -2**(bits-1)
        PID_T_MAX = 2**(bits-1) - 1
        self.check_long_asint(aspid, PID_T_MIN, PID_T_MAX)

    def test_long_aspid(self):
        self._test_long_aspid(_testcapi.pylong_aspid)

    def test_long_aspid_limited(self):
        self._test_long_aspid(_testlimitedcapi.pylong_aspid)

    def test_long_asnativebytes(self):
        import math
        from _testcapi import (
            pylong_asnativebytes as asnativebytes,
            SIZE_MAX,
        )

        # Abbreviate sizeof(Py_ssize_t) to SZ because we use it a lot
        SZ = int(math.ceil(math.log(SIZE_MAX + 1) / math.log(2)) / 8)
        MAX_SSIZE = 2 ** (SZ * 8 - 1) - 1
        MAX_USIZE = 2 ** (SZ * 8) - 1
        if support.verbose:
            print(f"SIZEOF_SIZE={SZ}\n{MAX_SSIZE=:016X}\n{MAX_USIZE=:016X}")

        # These tests check that the requested buffer size is correct.
        # This matches our current implementation: We only specify that the
        # return value is a size *sufficient* to hold the result when queried
        # using n_bytes=0. If our implementation changes, feel free to update
        # the expectations here -- or loosen them to be range checks.
        # (i.e. 0 *could* be stored in 1 byte and 512 in 2)
        for v, expect in [
            (0, SZ),
            (512, SZ),
            (-512, SZ),
            (MAX_SSIZE, SZ),
            (MAX_USIZE, SZ + 1),
            (-MAX_SSIZE, SZ),
            (-MAX_USIZE, SZ + 1),
            (2**255-1, 32),
            (-(2**255-1), 32),
            (2**255, 33),
            (-(2**255), 33), # if you ask, we'll say 33, but 32 would do
            (2**256-1, 33),
            (-(2**256-1), 33),
            (2**256, 33),
            (-(2**256), 33),
        ]:
            with self.subTest(f"sizeof-{v:X}"):
                buffer = bytearray(b"\x5a")
                self.assertEqual(expect, asnativebytes(v, buffer, 0, -1),
                    "PyLong_AsNativeBytes(v, <unknown>, 0, -1)")
                self.assertEqual(buffer, b"\x5a",
                    "buffer overwritten when it should not have been")
                # Also check via the __index__ path.
                # We pass Py_ASNATIVEBYTES_NATIVE_ENDIAN | ALLOW_INDEX
                self.assertEqual(expect, asnativebytes(Index(v), buffer, 0, 3 | 16),
                    "PyLong_AsNativeBytes(Index(v), <unknown>, 0, -1)")
                self.assertEqual(buffer, b"\x5a",
                    "buffer overwritten when it should not have been")

        # Test that we populate n=2 bytes but do not overwrite more.
        buffer = bytearray(b"\x99"*3)
        self.assertEqual(2, asnativebytes(4, buffer, 2, 0),  # BE
            "PyLong_AsNativeBytes(v, <3 byte buffer>, 2, 0)  // BE")
        self.assertEqual(buffer, b"\x00\x04\x99")
        self.assertEqual(2, asnativebytes(4, buffer, 2, 1),  # LE
            "PyLong_AsNativeBytes(v, <3 byte buffer>, 2, 1)  // LE")
        self.assertEqual(buffer, b"\x04\x00\x99")

        # We request as many bytes as `expect_be` contains, and always check
        # the result (both big and little endian). We check the return value
        # independently, since the buffer should always be filled correctly even
        # if we need more bytes
        for v, expect_be, expect_n in [
            (0,         b'\x00',                1),
            (0,         b'\x00' * 2,            2),
            (0,         b'\x00' * 8,            min(8, SZ)),
            (1,         b'\x01',                1),
            (1,         b'\x00' * 10 + b'\x01', min(11, SZ)),
            (42,        b'\x2a',                1),
            (42,        b'\x00' * 10 + b'\x2a', min(11, SZ)),
            (-1,        b'\xff',                1),
            (-1,        b'\xff' * 10,           min(11, SZ)),
            (-42,       b'\xd6',                1),
            (-42,       b'\xff' * 10 + b'\xd6', min(11, SZ)),
            # Extracts 255 into a single byte, but requests 2
            # (this is currently a special case, and "should" request SZ)
            (255,       b'\xff',                2),
            (255,       b'\x00\xff',            2),
            (256,       b'\x01\x00',            2),
            (0x80,      b'\x00' * 7 + b'\x80',  min(8, SZ)),
            # Extracts successfully (unsigned), but requests 9 bytes
            (2**63,     b'\x80' + b'\x00' * 7,  9),
            (2**63,     b'\x00\x80' + b'\x00' * 7, 9),
            # Extracts into 8 bytes, but if you provide 9 we'll say 9
            (-2**63,    b'\x80' + b'\x00' * 7,  8),
            (-2**63,    b'\xff\x80' + b'\x00' * 7, 9),

            (2**255-1,      b'\x7f' + b'\xff' * 31,                 32),
            (-(2**255-1),   b'\x80' + b'\x00' * 30 + b'\x01',       32),
            # Request extra bytes, but result says we only needed 32
            (-(2**255-1),   b'\xff\x80' + b'\x00' * 30 + b'\x01',   32),
            (-(2**255-1),   b'\xff\xff\x80' + b'\x00' * 30 + b'\x01', 32),

            # Extracting 256 bits of integer will request 33 bytes, but still
            # copy as many bits as possible into the buffer. So we *can* copy
            # into a 32-byte buffer, though negative number may be unrecoverable
            (2**256-1,      b'\xff' * 32,                           33),
            (2**256-1,      b'\x00' + b'\xff' * 32,                 33),
            (-(2**256-1),   b'\x00' * 31 + b'\x01',                 33),
            (-(2**256-1),   b'\xff' + b'\x00' * 31 + b'\x01',       33),
            (-(2**256-1),   b'\xff\xff' + b'\x00' * 31 + b'\x01',   33),
            # However, -2**255 precisely will extract into 32 bytes and return
            # success. For bigger buffers, it will still succeed, but will
            # return 33
            (-(2**255),     b'\x80' + b'\x00' * 31,                 32),
            (-(2**255),     b'\xff\x80' + b'\x00' * 31,             33),

            # The classic "Windows HRESULT as negative number" case
            #   HRESULT hr;
            #   PyLong_AsNativeBytes(<-2147467259>, &hr, sizeof(HRESULT), -1)
            #   assert(hr == E_FAIL)
            (-2147467259, b'\x80\x00\x40\x05', 4),
        ]:
            with self.subTest(f"{v:X}-{len(expect_be)}bytes"):
                n = len(expect_be)
                # Fill the buffer with dummy data to ensure all bytes
                # are overwritten.
                buffer = bytearray(b"\xa5"*n)
                expect_le = expect_be[::-1]

                self.assertEqual(expect_n, asnativebytes(v, buffer, n, 0),
                    f"PyLong_AsNativeBytes(v, buffer, {n}, <big>)")
                self.assertEqual(expect_be, buffer[:n], "<big>")
                self.assertEqual(expect_n, asnativebytes(v, buffer, n, 1),
                    f"PyLong_AsNativeBytes(v, buffer, {n}, <little>)")
                self.assertEqual(expect_le, buffer[:n], "<little>")

        # Test cases that do not request size for a sign bit when we pass the
        # Py_ASNATIVEBYTES_UNSIGNED_BUFFER flag
        for v, expect_be, expect_n in [
            (255,       b'\xff',                1),
            # We pass a 2 byte buffer so it just uses the whole thing
            (255,       b'\x00\xff',            2),

            (2**63,     b'\x80' + b'\x00' * 7,  8),
            # We pass a 9 byte buffer so it uses the whole thing
            (2**63,     b'\x00\x80' + b'\x00' * 7, 9),

            (2**256-1,  b'\xff' * 32,           32),
            # We pass a 33 byte buffer so it uses the whole thing
            (2**256-1,  b'\x00' + b'\xff' * 32, 33),
        ]:
            with self.subTest(f"{v:X}-{len(expect_be)}bytes-unsigned"):
                n = len(expect_be)
                buffer = bytearray(b"\xa5"*n)
                self.assertEqual(expect_n, asnativebytes(v, buffer, n, 4),
                    f"PyLong_AsNativeBytes(v, buffer, {n}, <big|unsigned>)")
                self.assertEqual(expect_n, asnativebytes(v, buffer, n, 5),
                    f"PyLong_AsNativeBytes(v, buffer, {n}, <little|unsigned>)")

        # Ensure Py_ASNATIVEBYTES_REJECT_NEGATIVE raises on negative value
        with self.assertRaises(ValueError):
            asnativebytes(-1, buffer, 0, 8)

        # Ensure omitting Py_ASNATIVEBYTES_ALLOW_INDEX raises on __index__ value
        with self.assertRaises(TypeError):
            asnativebytes(Index(1), buffer, 0, -1)
        with self.assertRaises(TypeError):
            asnativebytes(Index(1), buffer, 0, 3)

        # Check a few error conditions. These are validated in code, but are
        # unspecified in docs, so if we make changes to the implementation, it's
        # fine to just update these tests rather than preserve the behaviour.
        with self.assertRaises(TypeError):
            asnativebytes('not a number', buffer, 0, -1)

    def test_long_asnativebytes_fuzz(self):
        import math
        from random import Random
        from _testcapi import (
            pylong_asnativebytes as asnativebytes,
            SIZE_MAX,
        )

        # Abbreviate sizeof(Py_ssize_t) to SZ because we use it a lot
        SZ = int(math.ceil(math.log(SIZE_MAX + 1) / math.log(2)) / 8)

        rng = Random()
        # Allocate bigger buffer than actual values are going to be
        buffer = bytearray(260)

        for _ in range(1000):
            n = rng.randrange(1, 256)
            bytes_be = bytes([
                # Ensure the most significant byte is nonzero
                rng.randrange(1, 256),
                *[rng.randrange(256) for _ in range(n - 1)]
            ])
            bytes_le = bytes_be[::-1]
            v = int.from_bytes(bytes_le, 'little')

            expect_1 = expect_2 = (SZ, n)
            if bytes_be[0] & 0x80:
                # All values are positive, so if MSB is set, expect extra bit
                # when we request the size or have a large enough buffer
                expect_1 = (SZ, n + 1)
                # When passing Py_ASNATIVEBYTES_UNSIGNED_BUFFER, we expect the
                # return to be exactly the right size.
                expect_2 = (n,)

            try:
                actual = asnativebytes(v, buffer, 0, -1)
                self.assertIn(actual, expect_1)

                actual = asnativebytes(v, buffer, len(buffer), 0)
                self.assertIn(actual, expect_1)
                self.assertEqual(bytes_be, buffer[-n:])

                actual = asnativebytes(v, buffer, len(buffer), 1)
                self.assertIn(actual, expect_1)
                self.assertEqual(bytes_le, buffer[:n])

                actual = asnativebytes(v, buffer, n, 4)
                self.assertIn(actual, expect_2, bytes_be.hex())
                actual = asnativebytes(v, buffer, n, 5)
                self.assertIn(actual, expect_2, bytes_be.hex())
            except AssertionError as ex:
                value_hex = ''.join(reversed([
                    f'{b:02X}{"" if i % 8 else "_"}'
                    for i, b in enumerate(bytes_le, start=1)
                ])).strip('_')
                if support.verbose:
                    print()
                    print(n, 'bytes')
                    print('hex =', value_hex)
                    print('int =', v)
                    raise
                raise AssertionError(f"Value: 0x{value_hex}") from ex

    def test_long_fromnativebytes(self):
        import math
        from _testcapi import (
            pylong_fromnativebytes as fromnativebytes,
            SIZE_MAX,
        )

        # Abbreviate sizeof(Py_ssize_t) to SZ because we use it a lot
        SZ = int(math.ceil(math.log(SIZE_MAX + 1) / math.log(2)) / 8)
        MAX_SSIZE = 2 ** (SZ * 8 - 1) - 1
        MAX_USIZE = 2 ** (SZ * 8) - 1

        for v_be, expect_s, expect_u in [
            (b'\x00', 0, 0),
            (b'\x01', 1, 1),
            (b'\xff', -1, 255),
            (b'\x00\xff', 255, 255),
            (b'\xff\xff', -1, 65535),
        ]:
            with self.subTest(f"{expect_s}-{expect_u:X}-{len(v_be)}bytes"):
                n = len(v_be)
                v_le = v_be[::-1]

                self.assertEqual(expect_s, fromnativebytes(v_be, n, 0, 1),
                    f"PyLong_FromNativeBytes(buffer, {n}, <big>)")
                self.assertEqual(expect_s, fromnativebytes(v_le, n, 1, 1),
                    f"PyLong_FromNativeBytes(buffer, {n}, <little>)")
                self.assertEqual(expect_u, fromnativebytes(v_be, n, 0, 0),
                    f"PyLong_FromUnsignedNativeBytes(buffer, {n}, <big>)")
                self.assertEqual(expect_u, fromnativebytes(v_le, n, 1, 0),
                    f"PyLong_FromUnsignedNativeBytes(buffer, {n}, <little>)")

                # Check native endian when the result would be the same either
                # way and we can test it.
                if v_be == v_le:
                    self.assertEqual(expect_s, fromnativebytes(v_be, n, -1, 1),
                        f"PyLong_FromNativeBytes(buffer, {n}, <native>)")
                    self.assertEqual(expect_u, fromnativebytes(v_be, n, -1, 0),
                        f"PyLong_FromUnsignedNativeBytes(buffer, {n}, <native>)")

                # Swap the unsigned request for tests and use the
                # Py_ASNATIVEBYTES_UNSIGNED_BUFFER flag instead
                self.assertEqual(expect_u, fromnativebytes(v_be, n, 4, 1),
                    f"PyLong_FromNativeBytes(buffer, {n}, <big|unsigned>)")


if __name__ == "__main__":
    unittest.main()
