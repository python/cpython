import ctypes
import functools
import gc
import math
import sys
import unittest
import warnings
from _ctypes import CTYPES_MAX_ARGCOUNT
from ctypes import (CDLL, cdll, Structure, Union, CFUNCTYPE,
                    ArgumentError, POINTER, sizeof,
                    c_byte, c_ubyte, c_char, c_char_p,
                    c_short, c_ushort, c_int, c_uint,
                    c_long, c_longlong, c_ulonglong, c_ulong,
                    c_float, c_double, c_longdouble, py_object)
from ctypes.util import find_library
from test import support
from test.support import import_helper
_ctypes_test = import_helper.import_module("_ctypes_test")


class Callbacks(unittest.TestCase):
    functype = CFUNCTYPE

    def callback(self, *args):
        self.got_args = args
        return args[-1]

    def check_type(self, typ, arg):
        PROTO = self.functype.__func__(typ, typ)
        result = PROTO(self.callback)(arg)
        if typ == c_float:
            self.assertAlmostEqual(result, arg, places=5)
        else:
            self.assertEqual(self.got_args, (arg,))
            self.assertEqual(result, arg)

        PROTO = self.functype.__func__(typ, c_byte, typ)
        result = PROTO(self.callback)(-3, arg)
        if typ == c_float:
            self.assertAlmostEqual(result, arg, places=5)
        else:
            self.assertEqual(self.got_args, (-3, arg))
            self.assertEqual(result, arg)

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
        self.check_type(c_longlong, 42)
        self.check_type(c_longlong, -42)

    def test_ulonglong(self):
        self.check_type(c_ulonglong, 42)

    def test_float(self):
        # only almost equal: double -> float -> double
        self.check_type(c_float, math.e)
        self.check_type(c_float, -math.e)

    def test_double(self):
        self.check_type(c_double, 3.14)
        self.check_type(c_double, -3.14)

    def test_longdouble(self):
        self.check_type(c_longdouble, 3.14)
        self.check_type(c_longdouble, -3.14)

    def test_char(self):
        self.check_type(c_char, b"x")
        self.check_type(c_char, b"a")

    def test_pyobject(self):
        o = ()
        for o in (), [], object():
            initial = sys.getrefcount(o)
            # This call leaks a reference to 'o'...
            self.check_type(py_object, o)
            before = sys.getrefcount(o)
            # ...but this call doesn't leak any more.  Where is the refcount?
            self.check_type(py_object, o)
            after = sys.getrefcount(o)
            self.assertEqual((after, o), (before, o))

    def test_unsupported_restype_1(self):
        # Only "fundamental" result types are supported for callback
        # functions, the type must have a non-NULL stginfo->setfunc.
        # POINTER(c_double), for example, is not supported.

        prototype = self.functype.__func__(POINTER(c_double))
        # The type is checked when the prototype is called
        self.assertRaises(TypeError, prototype, lambda: None)

    def test_unsupported_restype_2(self):
        prototype = self.functype.__func__(object)
        self.assertRaises(TypeError, prototype, lambda: None)

    def test_issue_7959(self):
        proto = self.functype.__func__(None)

        class X:
            def func(self): pass
            def __init__(self):
                self.v = proto(self.func)

        for i in range(32):
            X()
        gc.collect()
        live = [x for x in gc.get_objects()
                if isinstance(x, X)]
        self.assertEqual(len(live), 0)

    def test_issue12483(self):
        class Nasty:
            def __del__(self):
                gc.collect()
        CFUNCTYPE(None)(lambda x=Nasty(): None)

    @unittest.skipUnless(hasattr(ctypes, 'WINFUNCTYPE'),
                         'ctypes.WINFUNCTYPE is required')
    def test_i38748_stackCorruption(self):
        callback_funcType = ctypes.WINFUNCTYPE(c_long, c_long, c_longlong)
        @callback_funcType
        def callback(a, b):
            c = a + b
            print(f"a={a}, b={b}, c={c}")
            return c
        dll = cdll[_ctypes_test.__file__]
        with support.captured_stdout() as out:
            # With no fix for i38748, the next line will raise OSError and cause the test to fail.
            self.assertEqual(dll._test_i38748_runCallback(callback, 5, 10), 15)
            self.assertEqual(out.getvalue(), "a=5, b=10, c=15\n")

if hasattr(ctypes, 'WINFUNCTYPE'):
    class StdcallCallbacks(Callbacks):
        functype = ctypes.WINFUNCTYPE


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
        libc_path = find_library("c")
        if not libc_path:
            self.skipTest('could not find libc')
        libc = CDLL(libc_path)

        @CFUNCTYPE(c_int, POINTER(c_int), POINTER(c_int))
        def cmp_func(a, b):
            return a[0] - b[0]

        array = (c_int * 5)(5, 1, 99, 7, 33)

        libc.qsort(array, len(array), sizeof(c_int), cmp_func)
        self.assertEqual(array[:], [1, 5, 7, 33, 99])

    @unittest.skipUnless(hasattr(ctypes, 'WINFUNCTYPE'),
                         'ctypes.WINFUNCTYPE is required')
    def test_issue_8959_b(self):
        from ctypes.wintypes import BOOL, HWND, LPARAM
        global windowCount
        windowCount = 0

        @ctypes.WINFUNCTYPE(BOOL, HWND, LPARAM)
        def EnumWindowsCallbackFunc(hwnd, lParam):
            global windowCount
            windowCount += 1
            return True #Allow windows to keep enumerating

        user32 = ctypes.windll.user32
        user32.EnumWindows(EnumWindowsCallbackFunc, 0)

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

    def test_callback_large_struct(self):
        class Check: pass

        # This should mirror the structure in Modules/_ctypes/_ctypes_test.c
        class X(Structure):
            _fields_ = [
                ('first', c_ulong),
                ('second', c_ulong),
                ('third', c_ulong),
            ]

        def callback(check, s):
            check.first = s.first
            check.second = s.second
            check.third = s.third
            # See issue #29565.
            # The structure should be passed by value, so
            # any changes to it should not be reflected in
            # the value passed
            s.first = s.second = s.third = 0x0badf00d

        check = Check()
        s = X()
        s.first = 0xdeadbeef
        s.second = 0xcafebabe
        s.third = 0x0bad1dea

        CALLBACK = CFUNCTYPE(None, X)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_large_struct
        func.argtypes = (X, CALLBACK)
        func.restype = None
        # the function just calls the callback with the passed structure
        func(s, CALLBACK(functools.partial(callback, check)))
        self.assertEqual(check.first, s.first)
        self.assertEqual(check.second, s.second)
        self.assertEqual(check.third, s.third)
        self.assertEqual(check.first, 0xdeadbeef)
        self.assertEqual(check.second, 0xcafebabe)
        self.assertEqual(check.third, 0x0bad1dea)
        # See issue #29565.
        # Ensure that the original struct is unchanged.
        self.assertEqual(s.first, check.first)
        self.assertEqual(s.second, check.second)
        self.assertEqual(s.third, check.third)

    # gh-49960: callbacks may return structures and unions by value.

    def test_callback_return_small_struct(self):
        class SmallRet(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]

        CALLBACK = CFUNCTYPE(SmallRet)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_small_struct
        func.argtypes = (CALLBACK,)
        func.restype = SmallRet

        result = func(CALLBACK(lambda: SmallRet(17, 42)))
        self.assertEqual((result.a, result.b), (17, 42))

    def test_callback_return_struct_reaches_c(self):
        # The C helper sums the fields itself, proving the bytes actually
        # reached the C caller rather than only round-tripping in Python.
        class SmallRet(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]

        CALLBACK = CFUNCTYPE(SmallRet)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_small_struct_sum
        func.argtypes = (CALLBACK,)
        func.restype = c_long

        self.assertEqual(func(CALLBACK(lambda: SmallRet(300, 45))), 345)

    def test_callback_return_large_struct(self):
        # Mirrors `Test` in Modules/_ctypes/_ctypes_test.c: >8 bytes, so it
        # is returned via a hidden pointer rather than in registers.
        class X(Structure):
            _fields_ = [("first", c_ulong),
                        ("second", c_ulong),
                        ("third", c_ulong)]

        CALLBACK = CFUNCTYPE(X)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_large_struct
        func.argtypes = (CALLBACK,)
        func.restype = X

        result = func(CALLBACK(lambda: X(0xdeadbeef, 0xcafebabe, 0x0bad1dea)))
        self.assertEqual(result.first, 0xdeadbeef)
        self.assertEqual(result.second, 0xcafebabe)
        self.assertEqual(result.third, 0x0bad1dea)

    def test_callback_return_float_struct(self):
        # All-float struct: SSE class on x86-64, HFA on AArch64.
        class FloatRet(Structure):
            _fields_ = [("x", c_double), ("y", c_double)]

        CALLBACK = CFUNCTYPE(FloatRet)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_float_struct
        func.argtypes = (CALLBACK,)
        func.restype = FloatRet

        result = func(CALLBACK(lambda: FloatRet(1.5, -2.25)))
        self.assertEqual((result.x, result.y), (1.5, -2.25))

    def test_callback_return_union(self):
        class UnionRet(Union):
            _fields_ = [("i", c_int), ("f", c_float)]

        CALLBACK = CFUNCTYPE(UnionRet)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_union
        func.argtypes = (CALLBACK,)
        func.restype = UnionRet

        result = func(CALLBACK(lambda: UnionRet(i=0x41424344)))
        self.assertEqual(result.i, 0x41424344)

    def test_callback_return_struct_repeatedly(self):
        # Guards against per-call leaks and against regressing into the
        # "memory leak in callback function" RuntimeWarning path.
        class SmallRet(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]

        CALLBACK = CFUNCTYPE(SmallRet)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_small_struct_sum
        func.argtypes = (CALLBACK,)
        func.restype = c_long

        cb = CALLBACK(lambda: SmallRet(1, 2))
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            for _ in range(100):
                self.assertEqual(func(cb), 3)

    def test_callback_return_struct_subclass(self):
        # isinstance semantics: a subclass instance is acceptable.
        class SmallRet(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]

        class SubRet(SmallRet):
            pass

        CALLBACK = CFUNCTYPE(SmallRet)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_small_struct_sum
        func.argtypes = (CALLBACK,)
        func.restype = c_long

        self.assertEqual(func(CALLBACK(lambda: SubRet(4, 5))), 9)

    def test_callback_return_struct_wrong_type(self):
        class SmallRet(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]

        class Other(Structure):
            _fields_ = [("q", c_int)]

        CALLBACK = CFUNCTYPE(SmallRet)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_small_struct_sum
        func.argtypes = (CALLBACK,)
        func.restype = c_long

        for bad in (None, 42, Other(1)):
            with self.subTest(bad=bad):
                def cb(bad=bad):
                    return bad
                with support.catch_unraisable_exception() as cm:
                    # The buffer is zeroed on failure, so the sum is 0.
                    self.assertEqual(func(CALLBACK(cb)), 0)
                    self.assertIsInstance(cm.unraisable.exc_value, TypeError)
                    self.assertEqual(
                        cm.unraisable.err_msg,
                        f"Exception ignored while converting result "
                        f"of ctypes callback function {cb!r}")

    def test_callback_return_struct_with_pointer(self):
        # gh-49960 / bpo-5710 discussion: the struct is copied by value, so
        # any pointer it contains must reference memory the caller keeps
        # alive. Here `keepalive` does exactly that.
        class WithPtr(Structure):
            _fields_ = [("s", c_char_p)]

        keepalive = b"hello"

        CALLBACK = CFUNCTYPE(WithPtr)
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_cbk_ret_ptr_struct
        func.argtypes = (CALLBACK,)
        func.restype = WithPtr

        result = func(CALLBACK(lambda: WithPtr(keepalive)))
        self.assertEqual(result.s, b"hello")

    def test_callback_too_many_args(self):
        def func(*args):
            return len(args)

        # valid call with nargs <= CTYPES_MAX_ARGCOUNT
        proto = CFUNCTYPE(c_int, *(c_int,) * CTYPES_MAX_ARGCOUNT)
        cb = proto(func)
        args1 = (1,) * CTYPES_MAX_ARGCOUNT
        self.assertEqual(cb(*args1), CTYPES_MAX_ARGCOUNT)

        # invalid call with nargs > CTYPES_MAX_ARGCOUNT
        args2 = (1,) * (CTYPES_MAX_ARGCOUNT + 1)
        with self.assertRaises(ArgumentError):
            cb(*args2)

        # error when creating the type with too many arguments
        with self.assertRaises(ArgumentError):
            CFUNCTYPE(c_int, *(c_int,) * (CTYPES_MAX_ARGCOUNT + 1))

    def test_convert_result_error(self):
        def func():
            return ("tuple",)

        proto = CFUNCTYPE(c_int)
        ctypes_func = proto(func)
        with support.catch_unraisable_exception() as cm:
            # don't test the result since it is an uninitialized value
            result = ctypes_func()

            self.assertIsInstance(cm.unraisable.exc_value, TypeError)
            self.assertEqual(cm.unraisable.err_msg,
                             f"Exception ignored while converting result "
                             f"of ctypes callback function {func!r}")
            self.assertIsNone(cm.unraisable.object)


if __name__ == '__main__':
    unittest.main()
