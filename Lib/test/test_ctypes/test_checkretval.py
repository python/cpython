import ctypes
import unittest
from ctypes import CDLL, c_int
from test.support import import_helper
_ctypes_test = import_helper.import_module("_ctypes_test")


class CHECKED(c_int):
    def _check_retval_(value):
        # Receives a CHECKED instance.
        return str(value.value)
    _check_retval_ = staticmethod(_check_retval_)


class Test(unittest.TestCase):
    def test_checkretval(self):
        dll = CDLL(_ctypes_test.__file__)
        self.assertEqual(42, dll._testfunc_p_p(42))

        dll._testfunc_p_p.restype = CHECKED
        self.assertEqual("42", dll._testfunc_p_p(42))

        dll._testfunc_p_p.restype = None
        self.assertEqual(None, dll._testfunc_p_p(42))

        del dll._testfunc_p_p.restype
        self.assertEqual(42, dll._testfunc_p_p(42))

    @unittest.skipUnless(hasattr(ctypes, 'oledll'),
                         'ctypes.oledll is required')
    def test_oledll(self):
        oleaut32 = ctypes.oledll.oleaut32
        self.assertRaises(OSError, oleaut32.CreateTypeLib2, 0, None, None)


if __name__ == "__main__":
    unittest.main()
