import _ctypes
import contextlib
import ctypes
import sys
import unittest
from test import support
from ctypes import CFUNCTYPE, c_void_p, c_char_p, c_int, c_double


def callback_func(arg):
    42 / arg
    raise ValueError(arg)


@unittest.skipUnless(sys.platform == "win32", 'Windows-specific test')
class call_function_TestCase(unittest.TestCase):
    # _ctypes.call_function is deprecated and private, but used by
    # Gary Bishp's readline module.  If we have it, we must test it as well.

    def test(self):
        kernel32 = ctypes.windll.kernel32
        kernel32.LoadLibraryA.restype = c_void_p
        kernel32.GetProcAddress.argtypes = c_void_p, c_char_p
        kernel32.GetProcAddress.restype = c_void_p

        hdll = kernel32.LoadLibraryA(b"kernel32")
        funcaddr = kernel32.GetProcAddress(hdll, b"GetModuleHandleA")

        self.assertEqual(_ctypes.call_function(funcaddr, (None,)),
                         kernel32.GetModuleHandleA(None))


class CallbackTracbackTestCase(unittest.TestCase):
    # When an exception is raised in a ctypes callback function, the C
    # code prints a traceback.
    #
    # This test makes sure the exception types *and* the exception
    # value is printed correctly.
    #
    # Changed in 0.9.3: No longer is '(in callback)' prepended to the
    # error message - instead an additional frame for the C code is
    # created, then a full traceback printed.  When SystemExit is
    # raised in a callback function, the interpreter exits.

    @contextlib.contextmanager
    def expect_unraisable(self, exc_type, exc_msg=None):
        with support.catch_unraisable_exception() as cm:
            yield

            self.assertIsInstance(cm.unraisable.exc_value, exc_type)
            if exc_msg is not None:
                self.assertEqual(str(cm.unraisable.exc_value), exc_msg)
            self.assertEqual(cm.unraisable.err_msg,
                             f"Exception ignored on calling ctypes "
                             f"callback function {callback_func!r}")
            self.assertIsNone(cm.unraisable.object)

    def test_ValueError(self):
        cb = CFUNCTYPE(c_int, c_int)(callback_func)
        with self.expect_unraisable(ValueError, '42'):
            cb(42)

    def test_IntegerDivisionError(self):
        cb = CFUNCTYPE(c_int, c_int)(callback_func)
        with self.expect_unraisable(ZeroDivisionError):
            cb(0)

    def test_FloatDivisionError(self):
        cb = CFUNCTYPE(c_int, c_double)(callback_func)
        with self.expect_unraisable(ZeroDivisionError):
            cb(0.0)

    def test_TypeErrorDivisionError(self):
        cb = CFUNCTYPE(c_int, c_char_p)(callback_func)
        err_msg = "unsupported operand type(s) for /: 'int' and 'bytes'"
        with self.expect_unraisable(TypeError, err_msg):
            cb(b"spam")


if __name__ == '__main__':
    unittest.main()
