from ctypes import *
import unittest, sys

def callback_func(arg):
    42 / arg
    raise ValueError(arg)

if sys.platform == "win32":

    class call_function_TestCase(unittest.TestCase):
        # _ctypes.call_function is deprecated and private, but used by
        # Gary Bishp's readline module.  If we have it, we must test it as well.

        def test(self):
            from _ctypes import call_function
            windll.kernel32.LoadLibraryA.restype = c_void_p
            windll.kernel32.GetProcAddress.argtypes = c_void_p, c_char_p
            windll.kernel32.GetProcAddress.restype = c_void_p

            hdll = windll.kernel32.LoadLibraryA(b"kernel32")
            funcaddr = windll.kernel32.GetProcAddress(hdll, "GetModuleHandleA")

            self.failUnlessEqual(call_function(funcaddr, (None,)),
                                 windll.kernel32.GetModuleHandleA(None))

class CallbackTracbackTestCase(unittest.TestCase):
    # When an exception is raised in a ctypes callback function, the C
    # code prints a traceback.
    #
    # This test makes sure the exception types *and* the exception
    # value is printed correctly.
    #
    # Changed in 0.9.3: No longer is '(in callback)' prepended to the
    # error message - instead a additional frame for the C code is
    # created, then a full traceback printed.  When SystemExit is
    # raised in a callback function, the interpreter exits.

    def capture_stderr(self, func, *args, **kw):
        # helper - call function 'func', and return the captured stderr
        import io
        old_stderr = sys.stderr
        logger = sys.stderr = io.StringIO()
        try:
            func(*args, **kw)
        finally:
            sys.stderr = old_stderr
        return logger.getvalue()

    def test_ValueError(self):
        cb = CFUNCTYPE(c_int, c_int)(callback_func)
        out = self.capture_stderr(cb, 42)
        self.failUnlessEqual(out.splitlines()[-1],
                             "ValueError: 42")

    def test_IntegerDivisionError(self):
        cb = CFUNCTYPE(c_int, c_int)(callback_func)
        out = self.capture_stderr(cb, 0)
        self.failUnlessEqual(out.splitlines()[-1][:19],
                             "ZeroDivisionError: ")

    def test_FloatDivisionError(self):
        cb = CFUNCTYPE(c_int, c_double)(callback_func)
        out = self.capture_stderr(cb, 0.0)
        self.failUnlessEqual(out.splitlines()[-1][:19],
                             "ZeroDivisionError: ")

    def test_TypeErrorDivisionError(self):
        cb = CFUNCTYPE(c_int, c_char_p)(callback_func)
        out = self.capture_stderr(cb, "spam")
        self.failUnlessEqual(out.splitlines()[-1],
                             "TypeError: "
                             "unsupported operand type(s) for /: 'int' and 'str'")

if __name__ == '__main__':
    unittest.main()
