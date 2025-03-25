from test.support import threading_helper
import unittest
import ctypes
import threading

threading_helper.requires_working_threading(module=True)


class FreeThreadingTests(unittest.TestCase):

    def test_PyCFuncPtr_new_race(self):
        # See https://github.com/python/cpython/issues/128567

        def raw_func(x):
            pass

        def race():
            barrier.wait()
            def lookup():
                prototype = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
                return prototype(raw_func)

            ctypes_args = ()
            func = lookup()
            packed_args = (ctypes.c_void_p * len(ctypes_args))()
            for argNum in range(len(ctypes_args)):
                packed_args[argNum] = ctypes.cast(ctypes_args[argNum], ctypes.c_void_p)
            func(packed_args)

        num_workers = 20

        barrier = threading.Barrier(num_workers)

        threads = []
        for _ in range(num_workers):
            t = threading.Thread(target=race)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass

    def test_Structure_creation_race(self):
        class POINT(ctypes.Structure):
            _fields_ = [("x", ctypes.c_int), ("y", ctypes.c_int)]

        def race():
            barrier.wait()
            return POINT(1, 2)

        num_workers = 20

        barrier = threading.Barrier(num_workers)

        threads = []
        for _ in range(num_workers):
            t = threading.Thread(target=race)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass

    def test_stginfo_update_race(self):
        # See https://github.com/python/cpython/issues/128570
        def make_nd_memref_descriptor(rank, dtype):
            class MemRefDescriptor(ctypes.Structure):
                _fields_ = [
                    ("aligned", ctypes.POINTER(dtype)),
                ]

            return MemRefDescriptor


        class F32(ctypes.Structure):
            _fields_ = [("f32", ctypes.c_float)]


        def race():
            barrier.wait()
            ctp = F32
            ctypes.pointer(
                ctypes.pointer(make_nd_memref_descriptor(1, ctp)())
            )


        num_workers = 20

        barrier = threading.Barrier(num_workers)

        threads = []
        for _ in range(num_workers):
            t = threading.Thread(target=race)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass
