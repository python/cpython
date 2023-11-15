import sys
import unittest
from test.support import import_helper
_testcapi = import_helper.import_module('_testcapi')


SIZEOF_PY_HASH_T = _testcapi.SIZEOF_VOID_P


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
