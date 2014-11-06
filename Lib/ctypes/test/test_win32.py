# Windows specific tests

from ctypes import *
from ctypes.test import requires
import unittest, sys
from test import test_support as support

import _ctypes_test

# Only windows 32-bit has different calling conventions.
@unittest.skipUnless(sys.platform == "win32", 'Windows-specific test')
@unittest.skipUnless(sizeof(c_void_p) == sizeof(c_int),
                     "sizeof c_void_p and c_int differ")
class WindowsTestCase(unittest.TestCase):
    def test_callconv_1(self):
        # Testing stdcall function

        IsWindow = windll.user32.IsWindow
        # ValueError: Procedure probably called with not enough arguments
        # (4 bytes missing)
        self.assertRaises(ValueError, IsWindow)

        # This one should succeed...
        self.assertEqual(0, IsWindow(0))

        # ValueError: Procedure probably called with too many arguments
        # (8 bytes in excess)
        self.assertRaises(ValueError, IsWindow, 0, 0, 0)

    def test_callconv_2(self):
        # Calling stdcall function as cdecl

        IsWindow = cdll.user32.IsWindow

        # ValueError: Procedure called with not enough arguments
        # (4 bytes missing) or wrong calling convention
        self.assertRaises(ValueError, IsWindow, None)

@unittest.skipUnless(sys.platform == "win32", 'Windows-specific test')
class FunctionCallTestCase(unittest.TestCase):
    @unittest.skipUnless('MSC' in sys.version, "SEH only supported by MSC")
    @unittest.skipIf(sys.executable.endswith('_d.exe'),
                     "SEH not enabled in debug builds")
    def test_SEH(self):
        requires("SEH")
        # Call functions with invalid arguments, and make sure
        # that access violations are trapped and raise an
        # exception.
        self.assertRaises(WindowsError, windll.kernel32.GetModuleHandleA, 32)

    def test_noargs(self):
        # This is a special case on win32 x64
        windll.user32.GetDesktopWindow()

@unittest.skipUnless(sys.platform == "win32", 'Windows-specific test')
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

        pt = POINT(15, 25)
        left = c_long.in_dll(dll, 'left')
        top = c_long.in_dll(dll, 'top')
        right = c_long.in_dll(dll, 'right')
        bottom = c_long.in_dll(dll, 'bottom')
        rect = RECT(left, top, right, bottom)
        PointInRect = dll.PointInRect
        PointInRect.argtypes = [POINTER(RECT), POINT]
        self.assertEqual(1, PointInRect(byref(rect), pt))

        ReturnRect = dll.ReturnRect
        ReturnRect.argtypes = [c_int, RECT, POINTER(RECT), POINT, RECT,
                               POINTER(RECT), POINT, RECT]
        ReturnRect.restype = RECT
        for i in range(4):
            ret = ReturnRect(i, rect, pointer(rect), pt, rect,
                         byref(rect), pt, rect)
            # the c function will check and modify ret if something is
            # passed in improperly
            self.assertEqual(ret.left, left.value)
            self.assertEqual(ret.right, right.value)
            self.assertEqual(ret.top, top.value)
            self.assertEqual(ret.bottom, bottom.value)

if __name__ == '__main__':
    unittest.main()
