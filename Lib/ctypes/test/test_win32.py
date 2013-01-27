# Windows specific tests

from ctypes import *
from ctypes.test import is_resource_enabled
import unittest, sys
from test import support

import _ctypes_test

if sys.platform == "win32" and sizeof(c_void_p) == sizeof(c_int):
    # Only windows 32-bit has different calling conventions.

    class WindowsTestCase(unittest.TestCase):
        def test_callconv_1(self):
            # Testing stdcall function

            IsWindow = windll.user32.IsWindow
            # ValueError: Procedure probably called with not enough arguments (4 bytes missing)
            self.assertRaises(ValueError, IsWindow)

            # This one should succeed...
            self.assertEqual(0, IsWindow(0))

            # ValueError: Procedure probably called with too many arguments (8 bytes in excess)
            self.assertRaises(ValueError, IsWindow, 0, 0, 0)

        def test_callconv_2(self):
            # Calling stdcall function as cdecl

            IsWindow = cdll.user32.IsWindow

            # ValueError: Procedure called with not enough arguments (4 bytes missing)
            # or wrong calling convention
            self.assertRaises(ValueError, IsWindow, None)

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
            if support.HAVE_DOCSTRINGS:
                self.assertEqual(COMError.__doc__,
                                 "Raised when a COM method call failed.")

            ex = COMError(-1, "text", ("details",))
            self.assertEqual(ex.hresult, -1)
            self.assertEqual(ex.text, "text")
            self.assertEqual(ex.details, ("details",))

    class TestWinError(unittest.TestCase):
        def test_winerror(self):
            # see Issue 16169
            import errno
            ERROR_INVALID_PARAMETER = 87
            msg = FormatError(ERROR_INVALID_PARAMETER).strip()
            args = (errno.EINVAL, msg, None, ERROR_INVALID_PARAMETER)

            e = WinError(ERROR_INVALID_PARAMETER)
            self.assertEqual(e.args, args)
            self.assertEqual(e.errno, errno.EINVAL)
            self.assertEqual(e.winerror, ERROR_INVALID_PARAMETER)

            windll.kernel32.SetLastError(ERROR_INVALID_PARAMETER)
            try:
                raise WinError()
            except OSError as exc:
                e = exc
            self.assertEqual(e.args, args)
            self.assertEqual(e.errno, errno.EINVAL)
            self.assertEqual(e.winerror, ERROR_INVALID_PARAMETER)

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
