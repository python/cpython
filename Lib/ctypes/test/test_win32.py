# Windows specific tests

from ctypes import *
from ctypes.test import is_resource_enabled
import unittest, sys

import _ctypes_test

if sys.platform == "win32":

    class WindowsTestCase(unittest.TestCase):
        def test_callconv_1(self):
            # Testing stdcall function

            IsWindow = windll.user32.IsWindow
            # ValueError: Procedure probably called with not enough arguments (4 bytes missing)
            self.assertRaises(ValueError, IsWindow)

            # This one should succeeed...
            self.failUnlessEqual(0, IsWindow(0))

            # ValueError: Procedure probably called with too many arguments (8 bytes in excess)
            self.assertRaises(ValueError, IsWindow, 0, 0, 0)

        def test_callconv_2(self):
            # Calling stdcall function as cdecl

            IsWindow = cdll.user32.IsWindow

            # ValueError: Procedure called with not enough arguments (4 bytes missing)
            # or wrong calling convention
            self.assertRaises(ValueError, IsWindow, None)

        if is_resource_enabled("SEH"):
            def test_SEH(self):
                # Call functions with invalid arguments, and make sure that access violations
                # are trapped and raise an exception.
                self.assertRaises(WindowsError, windll.kernel32.GetModuleHandleA, 32)

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
        self.failUnlessEqual(1, dll.PointInRect(byref(rect), pt))

if __name__ == '__main__':
    unittest.main()
