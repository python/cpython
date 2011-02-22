import unittest
from test import support
import ctypes
import gc

MyCallback = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int)
OtherCallback = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_ulonglong)

import _ctypes_test
dll = ctypes.CDLL(_ctypes_test.__file__)

class RefcountTestCase(unittest.TestCase):

    @support.refcount_test
    def test_1(self):
        from sys import getrefcount as grc

        f = dll._testfunc_callback_i_if
        f.restype = ctypes.c_int
        f.argtypes = [ctypes.c_int, MyCallback]

        def callback(value):
            #print "called back with", value
            return value

        self.assertEqual(grc(callback), 2)
        cb = MyCallback(callback)

        self.assertTrue(grc(callback) > 2)
        result = f(-10, cb)
        self.assertEqual(result, -18)
        cb = None

        gc.collect()

        self.assertEqual(grc(callback), 2)


    @support.refcount_test
    def test_refcount(self):
        from sys import getrefcount as grc
        def func(*args):
            pass
        # this is the standard refcount for func
        self.assertEqual(grc(func), 2)

        # the CFuncPtr instance holds atr least one refcount on func:
        f = OtherCallback(func)
        self.assertTrue(grc(func) > 2)

        # and may release it again
        del f
        self.assertTrue(grc(func) >= 2)

        # but now it must be gone
        gc.collect()
        self.assertTrue(grc(func) == 2)

        class X(ctypes.Structure):
            _fields_ = [("a", OtherCallback)]
        x = X()
        x.a = OtherCallback(func)

        # the CFuncPtr instance holds atr least one refcount on func:
        self.assertTrue(grc(func) > 2)

        # and may release it again
        del x
        self.assertTrue(grc(func) >= 2)

        # and now it must be gone again
        gc.collect()
        self.assertEqual(grc(func), 2)

        f = OtherCallback(func)

        # the CFuncPtr instance holds atr least one refcount on func:
        self.assertTrue(grc(func) > 2)

        # create a cycle
        f.cycle = f

        del f
        gc.collect()
        self.assertEqual(grc(func), 2)

class AnotherLeak(unittest.TestCase):
    def test_callback(self):
        import sys

        proto = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_int)
        def func(a, b):
            return a * b * 2
        f = proto(func)

        a = sys.getrefcount(ctypes.c_int)
        f(1, 2)
        self.assertEqual(sys.getrefcount(ctypes.c_int), a)

if __name__ == '__main__':
    unittest.main()
