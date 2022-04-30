"""
Here is probably the place to write the docs, since the test-cases
show how the type behave.

Later...
"""

from ctypes import *
from ctypes.test import need_symbol
import sys, unittest

try:
    WINFUNCTYPE
except NameError:
    # fake to enable this test on Linux
    WINFUNCTYPE = CFUNCTYPE

import _ctypes_test
dll = CDLL(_ctypes_test.__file__)
if sys.platform == "win32":
    windll = WinDLL(_ctypes_test.__file__)

class POINT(Structure):
    _fields_ = [("x", c_int), ("y", c_int)]
class RECT(Structure):
    _fields_ = [("left", c_int), ("top", c_int),
                ("right", c_int), ("bottom", c_int)]
class FunctionTestCase(unittest.TestCase):

    def test_mro(self):
        # in Python 2.3, this raises TypeError: MRO conflict among bases classes,
        # in Python 2.2 it works.
        #
        # But in early versions of _ctypes.c, the result of tp_new
        # wasn't checked, and it even crashed Python.
        # Found by Greg Chapman.

        with self.assertRaises(TypeError):
            class X(object, Array):
                _length_ = 5
                _type_ = "i"

        from _ctypes import _Pointer
        with self.assertRaises(TypeError):
            class X(object, _Pointer):
                pass

        from _ctypes import _SimpleCData
        with self.assertRaises(TypeError):
            class X(object, _SimpleCData):
                _type_ = "i"

        with self.assertRaises(TypeError):
            class X(object, Structure):
                _fields_ = []

    @need_symbol('c_wchar')
    def test_wchar_parm(self):
        f = dll._testfunc_i_bhilfd
        f.argtypes = [c_byte, c_wchar, c_int, c_long, c_float, c_double]
        result = f(1, "x", 3, 4, 5.0, 6.0)
        self.assertEqual(result, 139)
        self.assertEqual(type(result), int)

    @need_symbol('c_wchar')
    def test_wchar_result(self):
        f = dll._testfunc_i_bhilfd
        f.argtypes = [c_byte, c_short, c_int, c_long, c_float, c_double]
        f.restype = c_wchar
        result = f(0, 0, 0, 0, 0, 0)
        self.assertEqual(result, '\x00')

    def test_voidresult(self):
        f = dll._testfunc_v
        f.restype = None
        f.argtypes = [c_int, c_int, POINTER(c_int)]
        result = c_int()
        self.assertEqual(None, f(1, 2, byref(result)))
        self.assertEqual(result.value, 3)

    def test_intresult(self):
        f = dll._testfunc_i_bhilfd
        f.argtypes = [c_byte, c_short, c_int, c_long, c_float, c_double]
        f.restype = c_int
        result = f(1, 2, 3, 4, 5.0, 6.0)
        self.assertEqual(result, 21)
        self.assertEqual(type(result), int)

        result = f(-1, -2, -3, -4, -5.0, -6.0)
        self.assertEqual(result, -21)
        self.assertEqual(type(result), int)

        # If we declare the function to return a short,
        # is the high part split off?
        f.restype = c_short
        result = f(1, 2, 3, 4, 5.0, 6.0)
        self.assertEqual(result, 21)
        self.assertEqual(type(result), int)

        result = f(1, 2, 3, 0x10004, 5.0, 6.0)
        self.assertEqual(result, 21)
        self.assertEqual(type(result), int)

        # You cannot assign character format codes as restype any longer
        self.assertRaises(TypeError, setattr, f, "restype", "i")

    def test_floatresult(self):
        f = dll._testfunc_f_bhilfd
        f.argtypes = [c_byte, c_short, c_int, c_long, c_float, c_double]
        f.restype = c_float
        result = f(1, 2, 3, 4, 5.0, 6.0)
        self.assertEqual(result, 21)
        self.assertEqual(type(result), float)

        result = f(-1, -2, -3, -4, -5.0, -6.0)
        self.assertEqual(result, -21)
        self.assertEqual(type(result), float)

    def test_doubleresult(self):
        f = dll._testfunc_d_bhilfd
        f.argtypes = [c_byte, c_short, c_int, c_long, c_float, c_double]
        f.restype = c_double
        result = f(1, 2, 3, 4, 5.0, 6.0)
        self.assertEqual(result, 21)
        self.assertEqual(type(result), float)

        result = f(-1, -2, -3, -4, -5.0, -6.0)
        self.assertEqual(result, -21)
        self.assertEqual(type(result), float)

    def test_longdoubleresult(self):
        f = dll._testfunc_D_bhilfD
        f.argtypes = [c_byte, c_short, c_int, c_long, c_float, c_longdouble]
        f.restype = c_longdouble
        result = f(1, 2, 3, 4, 5.0, 6.0)
        self.assertEqual(result, 21)
        self.assertEqual(type(result), float)

        result = f(-1, -2, -3, -4, -5.0, -6.0)
        self.assertEqual(result, -21)
        self.assertEqual(type(result), float)

    @need_symbol('c_longlong')
    def test_longlongresult(self):
        f = dll._testfunc_q_bhilfd
        f.restype = c_longlong
        f.argtypes = [c_byte, c_short, c_int, c_long, c_float, c_double]
        result = f(1, 2, 3, 4, 5.0, 6.0)
        self.assertEqual(result, 21)

        f = dll._testfunc_q_bhilfdq
        f.restype = c_longlong
        f.argtypes = [c_byte, c_short, c_int, c_long, c_float, c_double, c_longlong]
        result = f(1, 2, 3, 4, 5.0, 6.0, 21)
        self.assertEqual(result, 42)

    def test_stringresult(self):
        f = dll._testfunc_p_p
        f.argtypes = None
        f.restype = c_char_p
        result = f(b"123")
        self.assertEqual(result, b"123")

        result = f(None)
        self.assertEqual(result, None)

    def test_pointers(self):
        f = dll._testfunc_p_p
        f.restype = POINTER(c_int)
        f.argtypes = [POINTER(c_int)]

        # This only works if the value c_int(42) passed to the
        # function is still alive while the pointer (the result) is
        # used.

        v = c_int(42)

        self.assertEqual(pointer(v).contents.value, 42)
        result = f(pointer(v))
        self.assertEqual(type(result), POINTER(c_int))
        self.assertEqual(result.contents.value, 42)

        # This on works...
        result = f(pointer(v))
        self.assertEqual(result.contents.value, v.value)

        p = pointer(c_int(99))
        result = f(p)
        self.assertEqual(result.contents.value, 99)

        arg = byref(v)
        result = f(arg)
        self.assertNotEqual(result.contents, v.value)

        self.assertRaises(ArgumentError, f, byref(c_short(22)))

        # It is dangerous, however, because you don't control the lifetime
        # of the pointer:
        result = f(byref(c_int(99)))
        self.assertNotEqual(result.contents, 99)

    ################################################################
    def test_shorts(self):
        f = dll._testfunc_callback_i_if

        args = []
        expected = [262144, 131072, 65536, 32768, 16384, 8192, 4096, 2048,
                    1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1]

        def callback(v):
            args.append(v)
            return v

        CallBack = CFUNCTYPE(c_int, c_int)

        cb = CallBack(callback)
        f(2**18, cb)
        self.assertEqual(args, expected)

    ################################################################


    def test_callbacks(self):
        f = dll._testfunc_callback_i_if
        f.restype = c_int
        f.argtypes = None

        MyCallback = CFUNCTYPE(c_int, c_int)

        def callback(value):
            #print "called back with", value
            return value

        cb = MyCallback(callback)
        result = f(-10, cb)
        self.assertEqual(result, -18)

        # test with prototype
        f.argtypes = [c_int, MyCallback]
        cb = MyCallback(callback)
        result = f(-10, cb)
        self.assertEqual(result, -18)

        AnotherCallback = WINFUNCTYPE(c_int, c_int, c_int, c_int, c_int)

        # check that the prototype works: we call f with wrong
        # argument types
        cb = AnotherCallback(callback)
        self.assertRaises(ArgumentError, f, -10, cb)


    def test_callbacks_2(self):
        # Can also use simple datatypes as argument type specifiers
        # for the callback function.
        # In this case the call receives an instance of that type
        f = dll._testfunc_callback_i_if
        f.restype = c_int

        MyCallback = CFUNCTYPE(c_int, c_int)

        f.argtypes = [c_int, MyCallback]

        def callback(value):
            #print "called back with", value
            self.assertEqual(type(value), int)
            return value

        cb = MyCallback(callback)
        result = f(-10, cb)
        self.assertEqual(result, -18)

    @need_symbol('c_longlong')
    def test_longlong_callbacks(self):

        f = dll._testfunc_callback_q_qf
        f.restype = c_longlong

        MyCallback = CFUNCTYPE(c_longlong, c_longlong)

        f.argtypes = [c_longlong, MyCallback]

        def callback(value):
            self.assertIsInstance(value, int)
            return value & 0x7FFFFFFF

        cb = MyCallback(callback)

        self.assertEqual(13577625587, f(1000000000000, cb))

    def test_errors(self):
        self.assertRaises(AttributeError, getattr, dll, "_xxx_yyy")
        self.assertRaises(ValueError, c_int.in_dll, dll, "_xxx_yyy")

    def test_byval(self):

        # without prototype
        ptin = POINT(1, 2)
        ptout = POINT()
        # EXPORT int _testfunc_byval(point in, point *pout)
        result = dll._testfunc_byval(ptin, byref(ptout))
        got = result, ptout.x, ptout.y
        expected = 3, 1, 2
        self.assertEqual(got, expected)

        # with prototype
        ptin = POINT(101, 102)
        ptout = POINT()
        dll._testfunc_byval.argtypes = (POINT, POINTER(POINT))
        dll._testfunc_byval.restype = c_int
        result = dll._testfunc_byval(ptin, byref(ptout))
        got = result, ptout.x, ptout.y
        expected = 203, 101, 102
        self.assertEqual(got, expected)

    def test_struct_return_2H(self):
        class S2H(Structure):
            _fields_ = [("x", c_short),
                        ("y", c_short)]
        dll.ret_2h_func.restype = S2H
        dll.ret_2h_func.argtypes = [S2H]
        inp = S2H(99, 88)
        s2h = dll.ret_2h_func(inp)
        self.assertEqual((s2h.x, s2h.y), (99*2, 88*3))

    @unittest.skipUnless(sys.platform == "win32", 'Windows-specific test')
    def test_struct_return_2H_stdcall(self):
        class S2H(Structure):
            _fields_ = [("x", c_short),
                        ("y", c_short)]

        windll.s_ret_2h_func.restype = S2H
        windll.s_ret_2h_func.argtypes = [S2H]
        s2h = windll.s_ret_2h_func(S2H(99, 88))
        self.assertEqual((s2h.x, s2h.y), (99*2, 88*3))

    def test_struct_return_8H(self):
        class S8I(Structure):
            _fields_ = [("a", c_int),
                        ("b", c_int),
                        ("c", c_int),
                        ("d", c_int),
                        ("e", c_int),
                        ("f", c_int),
                        ("g", c_int),
                        ("h", c_int)]
        dll.ret_8i_func.restype = S8I
        dll.ret_8i_func.argtypes = [S8I]
        inp = S8I(9, 8, 7, 6, 5, 4, 3, 2)
        s8i = dll.ret_8i_func(inp)
        self.assertEqual((s8i.a, s8i.b, s8i.c, s8i.d, s8i.e, s8i.f, s8i.g, s8i.h),
                             (9*2, 8*3, 7*4, 6*5, 5*6, 4*7, 3*8, 2*9))

    @unittest.skipUnless(sys.platform == "win32", 'Windows-specific test')
    def test_struct_return_8H_stdcall(self):
        class S8I(Structure):
            _fields_ = [("a", c_int),
                        ("b", c_int),
                        ("c", c_int),
                        ("d", c_int),
                        ("e", c_int),
                        ("f", c_int),
                        ("g", c_int),
                        ("h", c_int)]
        windll.s_ret_8i_func.restype = S8I
        windll.s_ret_8i_func.argtypes = [S8I]
        inp = S8I(9, 8, 7, 6, 5, 4, 3, 2)
        s8i = windll.s_ret_8i_func(inp)
        self.assertEqual(
                (s8i.a, s8i.b, s8i.c, s8i.d, s8i.e, s8i.f, s8i.g, s8i.h),
                (9*2, 8*3, 7*4, 6*5, 5*6, 4*7, 3*8, 2*9))

    def test_sf1651235(self):
        # see https://www.python.org/sf/1651235

        proto = CFUNCTYPE(c_int, RECT, POINT)
        def callback(*args):
            return 0

        callback = proto(callback)
        self.assertRaises(ArgumentError, lambda: callback((1, 2, 3, 4), POINT()))

if __name__ == '__main__':
    unittest.main()
