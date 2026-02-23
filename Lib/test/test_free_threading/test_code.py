import unittest

try:
    import ctypes
except ImportError:
    ctypes = None

from threading import Thread
from unittest import TestCase

from test.support import threading_helper
from test.support.threading_helper import run_concurrently

if ctypes is not None:
    capi = ctypes.pythonapi

    freefunc = ctypes.CFUNCTYPE(None, ctypes.c_voidp)

    RequestCodeExtraIndex = capi.PyUnstable_Eval_RequestCodeExtraIndex
    RequestCodeExtraIndex.argtypes = (freefunc,)
    RequestCodeExtraIndex.restype = ctypes.c_ssize_t

    SetExtra = capi.PyUnstable_Code_SetExtra
    SetExtra.argtypes = (ctypes.py_object, ctypes.c_ssize_t, ctypes.c_voidp)
    SetExtra.restype = ctypes.c_int

    GetExtra = capi.PyUnstable_Code_GetExtra
    GetExtra.argtypes = (
        ctypes.py_object,
        ctypes.c_ssize_t,
        ctypes.POINTER(ctypes.c_voidp),
    )
    GetExtra.restype = ctypes.c_int

# Note: each call to RequestCodeExtraIndex permanently allocates a slot
# (the counter is monotonically increasing), up to MAX_CO_EXTRA_USERS (255).
NTHREADS = 20


@threading_helper.requires_working_threading()
class TestCode(TestCase):
    def test_code_attrs(self):
        """Test concurrent accesses to lazily initialized code attributes"""
        code_objects = []
        for _ in range(1000):
            code_objects.append(compile("a + b", "<string>", "eval"))

        def run_in_thread():
            for code in code_objects:
                self.assertIsInstance(code.co_code, bytes)
                self.assertIsInstance(code.co_freevars, tuple)
                self.assertIsInstance(code.co_varnames, tuple)

        threads = [Thread(target=run_in_thread) for _ in range(2)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()

    @unittest.skipUnless(ctypes, "ctypes is required")
    def test_request_code_extra_index_concurrent(self):
        """Test concurrent calls to RequestCodeExtraIndex"""
        results = []

        def worker():
            idx = RequestCodeExtraIndex(freefunc(0))
            self.assertGreaterEqual(idx, 0)
            results.append(idx)

        run_concurrently(worker_func=worker, nthreads=NTHREADS)

        # Every thread must get a unique index.
        self.assertEqual(len(results), NTHREADS)
        self.assertEqual(len(set(results)), NTHREADS)

    @unittest.skipUnless(ctypes, "ctypes is required")
    def test_code_extra_all_ops_concurrent(self):
        """Test concurrent RequestCodeExtraIndex + SetExtra + GetExtra"""
        LOOP = 100

        def f():
            pass

        code = f.__code__

        def worker():
            idx = RequestCodeExtraIndex(freefunc(0))
            self.assertGreaterEqual(idx, 0)

            for i in range(LOOP):
                ret = SetExtra(code, idx, ctypes.c_voidp(i + 1))
                self.assertEqual(ret, 0)

            for _ in range(LOOP):
                extra = ctypes.c_voidp()
                ret = GetExtra(code, idx, extra)
                self.assertEqual(ret, 0)
                # The slot was set by this thread, so the value must
                # be the last one written.
                self.assertEqual(extra.value, LOOP)

        run_concurrently(worker_func=worker, nthreads=NTHREADS)

    @unittest.skipUnless(ctypes, "ctypes is required")
    def test_code_extra_set_get_concurrent(self):
        """Test concurrent SetExtra + GetExtra on a shared index"""
        LOOP = 100

        def f():
            pass

        code = f.__code__

        idx = RequestCodeExtraIndex(freefunc(0))
        self.assertGreaterEqual(idx, 0)

        def worker():
            for i in range(LOOP):
                ret = SetExtra(code, idx, ctypes.c_voidp(i + 1))
                self.assertEqual(ret, 0)

            for _ in range(LOOP):
                extra = ctypes.c_voidp()
                ret = GetExtra(code, idx, extra)
                self.assertEqual(ret, 0)
                # Value is set by any writer thread.
                self.assertTrue(1 <= extra.value <= LOOP)

        run_concurrently(worker_func=worker, nthreads=NTHREADS)

        # Every thread's last write is LOOP, so the final value must be LOOP.
        extra = ctypes.c_voidp()
        ret = GetExtra(code, idx, extra)
        self.assertEqual(ret, 0)
        self.assertEqual(extra.value, LOOP)


if __name__ == "__main__":
    unittest.main()
