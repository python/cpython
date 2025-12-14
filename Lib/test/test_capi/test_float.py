import math
import random
import platform
import sys
import unittest
import warnings

from test.test_capi.test_getargs import (Float, FloatSubclass, FloatSubclass2,
                                         BadIndex2, BadFloat2, Index, BadIndex,
                                         BadFloat)
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')

NULL = None

# For PyFloat_Pack/Unpack*
BIG_ENDIAN = 0
LITTLE_ENDIAN = 1
EPSILON = {
    2: 2.0 ** -11,  # binary16
    4: 2.0 ** -24,  # binary32
    8: 2.0 ** -53,  # binary64
}

HAVE_IEEE_754 = float.__getformat__("double").startswith("IEEE")
INF = float("inf")
NAN = float("nan")


def make_nan(size, sign, quiet, payload=None):
    if size == 8:
        payload_mask = 0x7ffffffffffff
        i = (sign << 63) + (0x7ff << 52) + (quiet << 51)
    elif size == 4:
        payload_mask = 0x3fffff
        i = (sign << 31) + (0xff << 23) + (quiet << 22)
    elif size == 2:
        payload_mask = 0x1ff
        i = (sign << 15) + (0x1f << 10) + (quiet << 9)
    else:
        raise ValueError("size must be either 2, 4, or 8")
    if payload is None:
        payload = random.randint(not quiet, payload_mask)
    return i + payload


class CAPIFloatTest(unittest.TestCase):
    def test_check(self):
        # Test PyFloat_Check()
        check = _testlimitedcapi.float_check

        self.assertTrue(check(4.25))
        self.assertTrue(check(FloatSubclass(4.25)))
        self.assertFalse(check(Float()))
        self.assertFalse(check(3))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_checkexact(self):
        # Test PyFloat_CheckExact()
        checkexact = _testlimitedcapi.float_checkexact

        self.assertTrue(checkexact(4.25))
        self.assertFalse(checkexact(FloatSubclass(4.25)))
        self.assertFalse(checkexact(Float()))
        self.assertFalse(checkexact(3))
        self.assertFalse(checkexact(object()))

        # CRASHES checkexact(NULL)

    def test_fromstring(self):
        # Test PyFloat_FromString()
        fromstring = _testlimitedcapi.float_fromstring

        self.assertEqual(fromstring("4.25"), 4.25)
        self.assertEqual(fromstring(b"4.25"), 4.25)
        self.assertRaises(ValueError, fromstring, "4.25\0")
        self.assertRaises(ValueError, fromstring, b"4.25\0")

        self.assertEqual(fromstring(bytearray(b"4.25")), 4.25)

        self.assertEqual(fromstring(memoryview(b"4.25")), 4.25)
        self.assertEqual(fromstring(memoryview(b"4.255")[:-1]), 4.25)
        self.assertRaises(TypeError, fromstring, memoryview(b"4.25")[::2])

        self.assertRaises(TypeError, fromstring, 4.25)

        # CRASHES fromstring(NULL)

    def test_fromdouble(self):
        # Test PyFloat_FromDouble()
        fromdouble = _testlimitedcapi.float_fromdouble

        self.assertEqual(fromdouble(4.25), 4.25)

    def test_asdouble(self):
        # Test PyFloat_AsDouble()
        asdouble = _testlimitedcapi.float_asdouble

        class BadFloat3:
            def __float__(self):
                raise RuntimeError

        self.assertEqual(asdouble(4.25), 4.25)
        self.assertEqual(asdouble(-1.0), -1.0)
        self.assertEqual(asdouble(42), 42.0)
        self.assertEqual(asdouble(-1), -1.0)
        self.assertEqual(asdouble(2**1000), float(2**1000))

        self.assertEqual(asdouble(FloatSubclass(4.25)), 4.25)
        self.assertEqual(asdouble(FloatSubclass2(4.25)), 4.25)
        self.assertEqual(asdouble(Index()), 99.)

        self.assertRaises(TypeError, asdouble, BadIndex())
        self.assertRaises(TypeError, asdouble, BadFloat())
        self.assertRaises(RuntimeError, asdouble, BadFloat3())
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(asdouble(BadIndex2()), 1.)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(asdouble(BadFloat2()), 4.25)
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning, asdouble, BadFloat2())
        self.assertRaises(TypeError, asdouble, object())
        self.assertRaises(TypeError, asdouble, NULL)

    def test_getinfo(self):
        # Test PyFloat_GetInfo()
        getinfo = _testlimitedcapi.float_getinfo

        self.assertEqual(getinfo(), sys.float_info)

    def test_getmax(self):
        # Test PyFloat_GetMax()
        getmax = _testlimitedcapi.float_getmax

        self.assertEqual(getmax(), sys.float_info.max)

    def test_getmin(self):
        # Test PyFloat_GetMax()
        getmin = _testlimitedcapi.float_getmin

        self.assertEqual(getmin(), sys.float_info.min)

    def test_pack(self):
        # Test PyFloat_Pack2(), PyFloat_Pack4() and PyFloat_Pack8()
        pack = _testcapi.float_pack

        self.assertEqual(pack(2, 1.5, BIG_ENDIAN), b'>\x00')
        self.assertEqual(pack(4, 1.5, BIG_ENDIAN), b'?\xc0\x00\x00')
        self.assertEqual(pack(8, 1.5, BIG_ENDIAN),
                         b'?\xf8\x00\x00\x00\x00\x00\x00')
        self.assertEqual(pack(2, 1.5, LITTLE_ENDIAN), b'\x00>')
        self.assertEqual(pack(4, 1.5, LITTLE_ENDIAN), b'\x00\x00\xc0?')
        self.assertEqual(pack(8, 1.5, LITTLE_ENDIAN),
                         b'\x00\x00\x00\x00\x00\x00\xf8?')

    def test_unpack(self):
        # Test PyFloat_Unpack2(), PyFloat_Unpack4() and PyFloat_Unpack8()
        unpack = _testcapi.float_unpack

        self.assertEqual(unpack(b'>\x00', BIG_ENDIAN), 1.5)
        self.assertEqual(unpack(b'?\xc0\x00\x00', BIG_ENDIAN), 1.5)
        self.assertEqual(unpack(b'?\xf8\x00\x00\x00\x00\x00\x00', BIG_ENDIAN),
                         1.5)
        self.assertEqual(unpack(b'\x00>', LITTLE_ENDIAN), 1.5)
        self.assertEqual(unpack(b'\x00\x00\xc0?', LITTLE_ENDIAN), 1.5)
        self.assertEqual(unpack(b'\x00\x00\x00\x00\x00\x00\xf8?', LITTLE_ENDIAN),
                         1.5)

    def test_pack_unpack_roundtrip(self):
        pack = _testcapi.float_pack
        unpack = _testcapi.float_unpack

        large = 2.0 ** 100
        values = [1.0, 1.5, large, 1.0/7, math.pi]
        if HAVE_IEEE_754:
            values.extend((INF, NAN))
        for value in values:
            for size in (2, 4, 8,):
                if size == 2 and value == large:
                    # too large for 16-bit float
                    continue
                rel_tol = EPSILON[size]
                for endian in (BIG_ENDIAN, LITTLE_ENDIAN):
                    with self.subTest(value=value, size=size, endian=endian):
                        data = pack(size, value, endian)
                        value2 = unpack(data, endian)
                        if math.isnan(value):
                            self.assertTrue(math.isnan(value2), (value, value2))
                        elif size < 8:
                            self.assertTrue(math.isclose(value2, value, rel_tol=rel_tol),
                                            (value, value2))
                        else:
                            self.assertEqual(value2, value)

    @unittest.skipUnless(HAVE_IEEE_754, "requires IEEE 754")
    def test_pack_unpack_roundtrip_for_nans(self):
        pack = _testcapi.float_pack
        unpack = _testcapi.float_unpack

        for _ in range(10):
            for size in (2, 4, 8):
                sign = random.randint(0, 1)
                if sys.maxsize != 2147483647:  # not it 32-bit mode
                    signaling = random.randint(0, 1)
                else:
                    # Skip sNaN's on x86 (32-bit).  The problem is that sNaN
                    # doubles become qNaN doubles just by the C calling
                    # convention, there is no way to preserve sNaN doubles
                    # between C function calls with the current
                    # PyFloat_Pack/Unpack*() API.  See also gh-130317 and
                    # e.g. https://developercommunity.visualstudio.com/t/155064
                    signaling = 0
                    if platform.machine().startswith('parisc'):
                        # HP PA RISC uses 0 for quiet, see:
                        # https://en.wikipedia.org/wiki/NaN#Encoding
                        signaling = 1
                i = make_nan(size, sign, not signaling)
                data = bytes.fromhex(f'{i:x}')
                for endian in (BIG_ENDIAN, LITTLE_ENDIAN):
                    with self.subTest(data=data, size=size, endian=endian):
                        data1 = data if endian == BIG_ENDIAN else data[::-1]
                        value = unpack(data1, endian)
                        data2 = pack(size, value, endian)
                        self.assertTrue(math.isnan(value))
                        self.assertEqual(data1, data2)

    @unittest.skipUnless(HAVE_IEEE_754, "requires IEEE 754")
    @unittest.skipUnless(sys.maxsize != 2147483647, "requires 64-bit mode")
    def test_pack_unpack_nans_for_different_formats(self):
        pack = _testcapi.float_pack
        unpack = _testcapi.float_unpack

        for endian in (BIG_ENDIAN, LITTLE_ENDIAN):
            with self.subTest(endian=endian):
                byteorder = "big" if endian == BIG_ENDIAN else "little"

                # Convert sNaN to qNaN, if payload got truncated
                data = make_nan(8, 0, False, 0x80001).to_bytes(8, byteorder)
                snan_low = unpack(data, endian)
                qnan4 = make_nan(4, 0, True, 0).to_bytes(4, byteorder)
                qnan2 = make_nan(2, 0, True, 0).to_bytes(2, byteorder)
                self.assertEqual(pack(4, snan_low, endian), qnan4)
                self.assertEqual(pack(2, snan_low, endian), qnan2)

                # Preserve NaN type, if payload not truncated
                data = make_nan(8, 0, False, 0x80000000001).to_bytes(8, byteorder)
                snan_high = unpack(data, endian)
                snan4 = make_nan(4, 0, False, 16384).to_bytes(4, byteorder)
                snan2 = make_nan(2, 0, False, 2).to_bytes(2, byteorder)
                self.assertEqual(pack(4, snan_high, endian), snan4)
                self.assertEqual(pack(2, snan_high, endian), snan2)


if __name__ == "__main__":
    unittest.main()
