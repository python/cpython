import unittest
from ctypes import *
import _ctypes_test

class Callbacks(unittest.TestCase):
    functype = CFUNCTYPE

##    def tearDown(self):
##        import gc
##        gc.collect()

    def callback(self, *args):
        self.got_args = args
        return args[-1]

    def check_type(self, typ, arg):
        PROTO = self.functype.im_func(typ, typ)
        result = PROTO(self.callback)(arg)
        if typ == c_float:
            self.assertAlmostEqual(result, arg, places=5)
        else:
            self.assertEqual(self.got_args, (arg,))
            self.assertEqual(result, arg)

        PROTO = self.functype.im_func(typ, c_byte, typ)
        result = PROTO(self.callback)(-3, arg)
        if typ == c_float:
            self.assertAlmostEqual(result, arg, places=5)
        else:
            self.assertEqual(self.got_args, (-3, arg))
            self.assertEqual(result, arg)

    ################

    def test_byte(self):
        self.check_type(c_byte, 42)
        self.check_type(c_byte, -42)

    def test_ubyte(self):
        self.check_type(c_ubyte, 42)

    def test_short(self):
        self.check_type(c_short, 42)
        self.check_type(c_short, -42)

    def test_ushort(self):
        self.check_type(c_ushort, 42)

    def test_int(self):
        self.check_type(c_int, 42)
        self.check_type(c_int, -42)

    def test_uint(self):
        self.check_type(c_uint, 42)

    def test_long(self):
        self.check_type(c_long, 42)
        self.check_type(c_long, -42)

    def test_ulong(self):
        self.check_type(c_ulong, 42)

    def test_longlong(self):
        # test some 64-bit values, positive and negative
        self.check_type(c_longlong, 5948291757245277467)
        self.check_type(c_longlong, -5229388909784190580)
        self.check_type(c_longlong, 42)
        self.check_type(c_longlong, -42)

    def test_ulonglong(self):
        # test some 64-bit values, with and without msb set.
        self.check_type(c_ulonglong, 10955412242170339782)
        self.check_type(c_ulonglong, 3665885499841167458)
        self.check_type(c_ulonglong, 42)

    def test_float(self):
        # only almost equal: double -> float -> double
        import math
        self.check_type(c_float, math.e)
        self.check_type(c_float, -math.e)

    def test_double(self):
        self.check_type(c_double, 3.14)
        self.check_type(c_double, -3.14)

    def test_longdouble(self):
        self.check_type(c_longdouble, 3.14)
        self.check_type(c_longdouble, -3.14)

    def test_char(self):
        self.check_type(c_char, "x")
        self.check_type(c_char, "a")

    # disabled: would now (correctly) raise a RuntimeWarning about
    # a memory leak.  A callback function cannot return a non-integral
    # C type without causing a memory leak.
##    def test_char_p(self):
##        self.check_type(c_char_p, "abc")
##        self.check_type(c_char_p, "def")

    def test_pyobject(self):
        o = ()
        from sys import getrefcount as grc
        for o in (), [], object():
            initial = grc(o)
            # This call leaks a reference to 'o'...
            self.check_type(py_object, o)
            before = grc(o)
            # ...but this call doesn't leak any more.  Where is the refcount?
            self.check_type(py_object, o)
            after = grc(o)
            self.assertEqual((after, o), (before, o))

    def test_unsupported_restype_1(self):
        # Only "fundamental" result types are supported for callback
        # functions, the type must have a non-NULL stgdict->setfunc.
        # POINTER(c_double), for example, is not supported.

        prototype = self.functype.im_func(POINTER(c_double))
        # The type is checked when the prototype is called
        self.assertRaises(TypeError, prototype, lambda: None)

    def test_unsupported_restype_2(self):
        prototype = self.functype.im_func(object)
        self.assertRaises(TypeError, prototype, lambda: None)

    def test_issue_7959(self):
        proto = self.functype.im_func(None)

        class X(object):
            def func(self): pass
            def __init__(self):
                self.v = proto(self.func)

        import gc
        for i in range(32):
            X()
        gc.collect()
        live = [x for x in gc.get_objects()
                if isinstance(x, X)]
        self.assertEqual(len(live), 0)

    def test_issue12483(self):
        import gc
        class Nasty:
            def __del__(self):
                gc.collect()
        CFUNCTYPE(None)(lambda x=Nasty(): None)


try:
    WINFUNCTYPE
except NameError:
    pass
else:
    class StdcallCallbacks(Callbacks):
        functype = WINFUNCTYPE

################################################################

class SampleCallbacksTestCase(unittest.TestCase):

    def test_integrate(self):
        # Derived from some then non-working code, posted by David Foster
        dll = CDLL(_ctypes_test.__file__)

        # The function prototype called by 'integrate': double func(double);
        CALLBACK = CFUNCTYPE(c_double, c_double)

        # The integrate function itself, exposed from the _ctypes_test dll
        integrate = dll.integrate
        integrate.argtypes = (c_double, c_double, CALLBACK, c_long)
        integrate.restype = c_double

        def func(x):
            return x**2

        result = integrate(0.0, 1.0, CALLBACK(func), 10)
        diff = abs(result - 1./3.)

        self.assertLess(diff, 0.01, "%s not less than 0.01" % diff)

    def test_issue_8959_a(self):
        from ctypes.util import find_library
        libc_path = find_library("c")
        if not libc_path:
            return # cannot test
        libc = CDLL(libc_path)

        @CFUNCTYPE(c_int, POINTER(c_int), POINTER(c_int))
        def cmp_func(a, b):
            return a[0] - b[0]

        array = (c_int * 5)(5, 1, 99, 7, 33)

        libc.qsort(array, len(array), sizeof(c_int), cmp_func)
        self.assertEqual(array[:], [1, 5, 7, 33, 99])

    try:
        WINFUNCTYPE
    except NameError:
        pass
    else:
        def test_issue_8959_b(self):
            from ctypes.wintypes import BOOL, HWND, LPARAM
            global windowCount
            windowCount = 0

            @WINFUNCTYPE(BOOL, HWND, LPARAM)
            def EnumWindowsCallbackFunc(hwnd, lParam):
                global windowCount
                windowCount += 1
                return True #Allow windows to keep enumerating

            windll.user32.EnumWindows(EnumWindowsCallbackFunc, 0)

    def test_callback_register_int(self):
        # Issue #8275: buggy handling of callback args under Win64
        # NOTE: should be run on release builds as well
        dll = CDLL(_ctypes_test.__file__)
        CALLBACK = CFUNCTYPE(c_int, c_int, c_int, c_int, c_int, c_int)
        # All this function does is call the callback with its args squared
        func = dll._testfunc_cbk_reg_int
        func.argtypes = (c_int, c_int, c_int, c_int, c_int, CALLBACK)
        func.restype = c_int

        def callback(a, b, c, d, e):
            return a + b + c + d + e

        result = func(2, 3, 4, 5, 6, CALLBACK(callback))
        self.assertEqual(result, callback(2*2, 3*3, 4*4, 5*5, 6*6))

    def test_callback_register_double(self):
        # Issue #8275: buggy handling of callback args under Win64
        # NOTE: should be run on release builds as well
        dll = CDLL(_ctypes_test.__file__)
        CALLBACK = CFUNCTYPE(c_double, c_double, c_double, c_double,
                             c_double, c_double)
        # All this function does is call the callback with its args squared
        func = dll._testfunc_cbk_reg_double
        func.argtypes = (c_double, c_double, c_double,
                         c_double, c_double, CALLBACK)
        func.restype = c_double

        def callback(a, b, c, d, e):
            return a + b + c + d + e

        result = func(1.1, 2.2, 3.3, 4.4, 5.5, CALLBACK(callback))
        self.assertEqual(result,
                         callback(1.1*1.1, 2.2*2.2, 3.3*3.3, 4.4*4.4, 5.5*5.5))


################################################################

if __name__ == '__main__':
    unittest.main()
