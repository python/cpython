import ctypes
import gc
import sys
import unittest
from test import support
from test.support import import_helper, thread_unsafe
from test.support import script_helper
_ctypes_test = import_helper.import_module("_ctypes_test")


MyCallback = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int)
OtherCallback = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_ulonglong)

dll = ctypes.CDLL(_ctypes_test.__file__)

@thread_unsafe('not thread safe')
class RefcountTestCase(unittest.TestCase):
    @support.refcount_test
    def test_1(self):
        f = dll._testfunc_callback_i_if
        f.restype = ctypes.c_int
        f.argtypes = [ctypes.c_int, MyCallback]

        def callback(value):
            return value

        orig_refcount = sys.getrefcount(callback)
        cb = MyCallback(callback)

        self.assertGreater(sys.getrefcount(callback), orig_refcount)
        result = f(-10, cb)
        self.assertEqual(result, -18)
        cb = None

        gc.collect()

        self.assertEqual(sys.getrefcount(callback), orig_refcount)

    @support.refcount_test
    def test_refcount(self):
        def func(*args):
            pass
        orig_refcount = sys.getrefcount(func)

        # the CFuncPtr instance holds at least one refcount on func:
        f = OtherCallback(func)
        self.assertGreater(sys.getrefcount(func), orig_refcount)

        # and may release it again
        del f
        self.assertGreaterEqual(sys.getrefcount(func), orig_refcount)

        # but now it must be gone
        gc.collect()
        self.assertEqual(sys.getrefcount(func), orig_refcount)

        class X(ctypes.Structure):
            _fields_ = [("a", OtherCallback)]
        x = X()
        x.a = OtherCallback(func)

        # the CFuncPtr instance holds at least one refcount on func:
        self.assertGreater(sys.getrefcount(func), orig_refcount)

        # and may release it again
        del x
        self.assertGreaterEqual(sys.getrefcount(func), orig_refcount)

        # and now it must be gone again
        gc.collect()
        self.assertEqual(sys.getrefcount(func), orig_refcount)

        f = OtherCallback(func)

        # the CFuncPtr instance holds at least one refcount on func:
        self.assertGreater(sys.getrefcount(func), orig_refcount)

        # create a cycle
        f.cycle = f

        del f
        gc.collect()
        self.assertEqual(sys.getrefcount(func), orig_refcount)

@thread_unsafe('not thread safe')
class AnotherLeak(unittest.TestCase):
    def test_callback(self):
        proto = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_int)
        def func(a, b):
            return a * b * 2
        f = proto(func)

        a = sys.getrefcount(ctypes.c_int)
        f(1, 2)
        self.assertEqual(sys.getrefcount(ctypes.c_int), a)

    @support.refcount_test
    def test_callback_py_object_none_return(self):
        # bpo-36880: test that returning None from a py_object callback
        # does not decrement the refcount of None.

        for FUNCTYPE in (ctypes.CFUNCTYPE, ctypes.PYFUNCTYPE):
            with self.subTest(FUNCTYPE=FUNCTYPE):
                @FUNCTYPE(ctypes.py_object)
                def func():
                    return None

                # Check that calling func does not affect None's refcount.
                for _ in range(10000):
                    func()


class ModuleIsolationTest(unittest.TestCase):
    def test_finalize(self):
        # check if gc_decref() succeeds
        script = (
            "import ctypes;"
            "import sys;"
            "del sys.modules['_ctypes'];"
            "import _ctypes;"
            "exit()"
        )
        script_helper.assert_python_ok("-c", script)


class PyObjectRestypeTest(unittest.TestCase):
    def test_restype_py_object_with_null_return(self):
        # Test that a function which returns a NULL PyObject *
        # without setting an exception does not crash.
        PyErr_Occurred = ctypes.pythonapi.PyErr_Occurred
        PyErr_Occurred.argtypes = []
        PyErr_Occurred.restype = ctypes.py_object

        # At this point, there's no exception set, so PyErr_Occurred
        # returns NULL. Given the restype is py_object, the
        # ctypes machinery will raise a custom error.
        with self.assertRaisesRegex(ValueError, "PyObject is NULL"):
            PyErr_Occurred()


if __name__ == '__main__':
    unittest.main()
