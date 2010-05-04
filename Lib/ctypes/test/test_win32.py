# Windows specific tests

from ctypes import *
from ctypes.test import is_resource_enabled
import unittest, sys

import _ctypes_test

if sys.platform == "win32":
    class FunctionCallTestCase(unittest.TestCase):

        if is_resource_enabled("SEH"):
            def test_SEH(self):
                # Call functions with invalid arguments, and make sure
                # that access violations are trapped and raise an
                # exception.
                self.assertRaises(WindowsError, windll.kernel32.GetModuleHandleA, 32)

        def test_noargs(self):
            # This is a special case on win32 x64
            windll.user32.GetDesktopWindow()

    class TestWintypes(unittest.TestCase):
        def test_HWND(self):
            from ctypes import wintypes
            self.assertEqual(sizeof(wintypes.HWND), sizeof(c_void_p))

        def test_PARAM(self):
            from ctypes import wintypes
            self.assertEqual(sizeof(wintypes.WPARAM),
                                 sizeof(c_void_p))
            self.assertEqual(sizeof(wintypes.LPARAM),
                                 sizeof(c_void_p))

        def test_COMError(self):
            from _ctypes import COMError
            self.assertEqual(COMError.__doc__, "Raised when a COM method call failed.")

            ex = COMError(-1, "text", ("details",))
            self.assertEqual(ex.hresult, -1)
            self.assertEqual(ex.text, "text")
            self.assertEqual(ex.details, ("details",))

class Structures(unittest.TestCase):

    def test_struct_by_value(self):
        class POINT(Structure):
            _fields_ = [("x", c_long),
                        ("y", c_long)]

        class RECT(Structure):
            _fields_ = [("left", c_long),
                        ("top", c_long),
                        ("right", c_long),
                        ("bottom", c_long)]

        dll = CDLL(_ctypes_test.__file__)

        pt = POINT(10, 10)
        rect = RECT(0, 0, 20, 20)
        self.assertEqual(1, dll.PointInRect(byref(rect), pt))

if __name__ == '__main__':
    unittest.main()
