import unittest

from ctypes import *
from ctypes.test import need_symbol

class CHECKED(c_int):
    def _check_retval_(value):
        # Receives a CHECKED instance.
        return str(value.value)
    _check_retval_ = staticmethod(_check_retval_)

class Test(unittest.TestCase):

    def test_checkretval(self):

        import _ctypes_test
        dll = CDLL(_ctypes_test.__file__)
        self.assertEqual(42, dll._testfunc_p_p(42))

        dll._testfunc_p_p.restype = CHECKED
        self.assertEqual("42", dll._testfunc_p_p(42))

        dll._testfunc_p_p.restype = None
        self.assertEqual(None, dll._testfunc_p_p(42))

        del dll._testfunc_p_p.restype
        self.assertEqual(42, dll._testfunc_p_p(42))

    @need_symbol('oledll')
    def test_oledll(self):
        self.assertRaises(OSError,
                              oledll.oleaut32.CreateTypeLib2,
                              0, None, None)

if __name__ == "__main__":
    unittest.main()
