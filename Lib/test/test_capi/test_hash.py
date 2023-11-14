import math
import sys
import unittest
from test.support import import_helper
_testcapi = import_helper.import_module('_testcapi')


SIZEOF_VOID_P = _testcapi.SIZEOF_VOID_P
SIZEOF_PY_HASH_T = SIZEOF_VOID_P


class CAPITest(unittest.TestCase):
    def test_hash_getfuncdef(self):
        # Test PyHash_GetFuncDef()
        hash_getfuncdef = _testcapi.hash_getfuncdef
        func_def = hash_getfuncdef()

        match func_def.name:
            case "fnv":
                self.assertEqual(func_def.hash_bits, 8 * SIZEOF_PY_HASH_T)
                self.assertEqual(func_def.seed_bits, 16 * SIZEOF_PY_HASH_T)
            case "siphash13":
                self.assertEqual(func_def.hash_bits, 64)
                self.assertEqual(func_def.seed_bits, 128)
            case "siphash24":
                self.assertEqual(func_def.hash_bits, 64)
                self.assertEqual(func_def.seed_bits, 128)
            case _:
                self.fail(f"unknown function name: {func_def.name!r}")

        # compare with sys.hash_info
        hash_info = sys.hash_info
        self.assertEqual(func_def.name, hash_info.algorithm)
        self.assertEqual(func_def.hash_bits, hash_info.hash_bits)
        self.assertEqual(func_def.seed_bits, hash_info.seed_bits)

    def test_hash_pointer(self):
        # Test Py_HashPointer()
        hash_pointer = _testcapi.hash_pointer

        UHASH_T_MASK = ((2 ** (8 * SIZEOF_PY_HASH_T)) - 1)
        HASH_T_MAX = (2 ** (8 * SIZEOF_PY_HASH_T - 1) - 1)

        def python_hash_pointer(x):
            # Py_HashPointer() rotates the pointer bits by 4 bits to the right
            x = (x >> 4) | ((x & 15) << (8 * SIZEOF_VOID_P - 4))

            # Convert unsigned uintptr_t (Py_uhash_t) to signed Py_hash_t
            if HASH_T_MAX < x:
                x = (~x) + 1
                x &= UHASH_T_MASK
                x = (~x) + 1
            return x

        if SIZEOF_VOID_P == 8:
            values = (
                0xABCDEF1234567890,
                0x1234567890ABCDEF,
                0xFEE4ABEDD1CECA5E,
            )
        else:
            values = (
                0x12345678,
                0x1234ABCD,
                0xDEADCAFE,
            )

        for value in values:
            expected = python_hash_pointer(value)
            with self.subTest(value=value):
                self.assertEqual(hash_pointer(value), expected,
                                 f"hash_pointer({value:x}) = "
                                 f"{hash_pointer(value):x} != {expected:x}")

        # Py_HashPointer(NULL) returns 0
        self.assertEqual(hash_pointer(0), 0)

        # Py_HashPointer((void*)(uintptr_t)-1) doesn't return -1 but -2
        VOID_P_MAX = -1 & (2 ** (8 * SIZEOF_VOID_P) - 1)
        self.assertEqual(hash_pointer(VOID_P_MAX), -2)

    def test_hash_double(self):
        # Test Py_HashDouble()
        hash_double = _testcapi.hash_double

        def check_number(value, expected):
            self.assertEqual(hash_double(value), (1, expected))

        # test some integers
        integers = [
            *range(1, 30),
            2**30 - 1,
            2 ** 233,
            int(sys.float_info.max),
        ]
        for x in integers:
            with self.subTest(x=x):
                check_number(float(x), hash(x))
                check_number(float(-x), hash(-x))

        # test positive and negative zeros
        check_number(float(0.0), 0)
        check_number(float(-0.0), 0)

        # test +inf and -inf
        inf = float("inf")
        check_number(inf, sys.hash_info.inf)
        check_number(-inf, -sys.hash_info.inf)

        # special float values: compare with Python hash() function
        special_values = (
            math.nextafter(0.0, 1.0),  # smallest positive subnormal number
            sys.float_info.min,        # smallest positive normal number
            sys.float_info.epsilon,
            sys.float_info.max,        # largest positive finite number
        )
        for x in special_values:
            with self.subTest(x=x):
                check_number(x, hash(x))
                check_number(-x, hash(-x))

        # test not-a-number (NaN)
        self.assertEqual(hash_double(float('nan')), (0, sys.hash_info.nan))
