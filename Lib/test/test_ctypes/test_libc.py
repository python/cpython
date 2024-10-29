import ctypes
import math
import unittest
from ctypes import (CDLL, CFUNCTYPE, POINTER, create_string_buffer, sizeof,
                    c_void_p, c_char, c_int, c_double, c_size_t)
from test.support import import_helper
_ctypes_test = import_helper.import_module("_ctypes_test")


lib = CDLL(_ctypes_test.__file__)


def three_way_cmp(x, y):
    """Return -1 if x < y, 0 if x == y and 1 if x > y"""
    return (x > y) - (x < y)


class LibTest(unittest.TestCase):
    def test_sqrt(self):
        lib.my_sqrt.argtypes = c_double,
        lib.my_sqrt.restype = c_double
        self.assertEqual(lib.my_sqrt(4.0), 2.0)
        self.assertEqual(lib.my_sqrt(2.0), math.sqrt(2.0))

    @unittest.skipUnless(hasattr(ctypes, "c_double_complex"),
                         "requires C11 complex type and libffi >= 3.3.0")
    def test_csqrt(self):
        lib.my_csqrt.argtypes = ctypes.c_double_complex,
        lib.my_csqrt.restype = ctypes.c_double_complex
        self.assertEqual(lib.my_csqrt(4), 2+0j)
        self.assertAlmostEqual(lib.my_csqrt(-1+0.01j),
                               0.004999937502734214+1.0000124996093955j)
        self.assertAlmostEqual(lib.my_csqrt(-1-0.01j),
                               0.004999937502734214-1.0000124996093955j)

        lib.my_csqrtf.argtypes = ctypes.c_float_complex,
        lib.my_csqrtf.restype = ctypes.c_float_complex
        self.assertAlmostEqual(lib.my_csqrtf(-1+0.01j),
                               0.004999937502734214+1.0000124996093955j)
        self.assertAlmostEqual(lib.my_csqrtf(-1-0.01j),
                               0.004999937502734214-1.0000124996093955j)

        lib.my_csqrtl.argtypes = ctypes.c_longdouble_complex,
        lib.my_csqrtl.restype = ctypes.c_longdouble_complex
        self.assertAlmostEqual(lib.my_csqrtl(-1+0.01j),
                               0.004999937502734214+1.0000124996093955j)
        self.assertAlmostEqual(lib.my_csqrtl(-1-0.01j),
                               0.004999937502734214-1.0000124996093955j)

    def test_qsort(self):
        comparefunc = CFUNCTYPE(c_int, POINTER(c_char), POINTER(c_char))
        lib.my_qsort.argtypes = c_void_p, c_size_t, c_size_t, comparefunc
        lib.my_qsort.restype = None

        def sort(a, b):
            return three_way_cmp(a[0], b[0])

        chars = create_string_buffer(b"spam, spam, and spam")
        lib.my_qsort(chars, len(chars)-1, sizeof(c_char), comparefunc(sort))
        self.assertEqual(chars.raw, b"   ,,aaaadmmmnpppsss\x00")


if __name__ == "__main__":
    unittest.main()
