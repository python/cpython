import concurrent.futures
import unittest
import inspect
from threading import Barrier
from unittest import TestCase

from test.support import threading_helper, Py_GIL_DISABLED

try:
    import _testcapi
except ImportError:
    _testcapi = None

threading_helper.requires_working_threading(module=True)


def get_func_annotation(f, b):
    b.wait()
    return inspect.get_annotations(f)


def get_func_annotation_dunder(f, b):
    b.wait()
    return f.__annotations__


def get_func_annotation_capi(f, b):
    b.wait()
    return _testcapi.function_get_annotations(f)


def set_func_annotation(f, b):
    b.wait()
    f.__annotations__ = {'x': int, 'y': int, 'return': int}
    return f.__annotations__


@unittest.skipUnless(Py_GIL_DISABLED, "Enable only in FT build")
class TestFTFuncAnnotations(TestCase):
    NUM_THREADS = 4

    def test_concurrent_read(self):
        def f(x: int) -> int:
            return x + 1

        for _ in range(10):
            with concurrent.futures.ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
                b = Barrier(self.NUM_THREADS)
                futures = {executor.submit(get_func_annotation, f, b): i for i in range(self.NUM_THREADS)}
                for fut in concurrent.futures.as_completed(futures):
                    annotate = fut.result()
                    self.assertIsNotNone(annotate)
                    self.assertEqual(annotate, {'x': int, 'return': int})

            with concurrent.futures.ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
                b = Barrier(self.NUM_THREADS)
                futures = {executor.submit(get_func_annotation_dunder, f, b): i for i in range(self.NUM_THREADS)}
                for fut in concurrent.futures.as_completed(futures):
                    annotate = fut.result()
                    self.assertIsNotNone(annotate)
                    self.assertEqual(annotate, {'x': int, 'return': int})

    def test_concurrent_write(self):
        def bar(x: int, y: float) -> float:
            return y ** x

        for _ in range(10):
            with concurrent.futures.ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
                b = Barrier(self.NUM_THREADS)
                futures = {executor.submit(set_func_annotation, bar, b): i for i in range(self.NUM_THREADS)}
                for fut in concurrent.futures.as_completed(futures):
                    annotate = fut.result()
                    self.assertIsNotNone(annotate)
                    self.assertEqual(annotate, {'x': int, 'y': int, 'return': int})

            # func_get_annotations returns in-place dict, so bar.__annotations__ should be modified as well
            self.assertEqual(bar.__annotations__, {'x': int, 'y': int, 'return': int})

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_concurrent_capi_read_and_write(self):
        # PyFunction_GetAnnotations() lazily computes and stores the
        # annotations dict, so it must hold a critical section on the
        # function object, like the __annotations__ getter and setter do.
        for _ in range(10):
            def foo(x: int) -> int:
                return x + 1

            with concurrent.futures.ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
                b = Barrier(self.NUM_THREADS)
                futures = {}
                for i in range(self.NUM_THREADS):
                    func = get_func_annotation_capi if i % 2 else set_func_annotation
                    futures[executor.submit(func, foo, b)] = i
                for fut in concurrent.futures.as_completed(futures):
                    annotations = fut.result()
                    # foo always has annotations, either computed from
                    # foo.__annotate__ or set by set_func_annotation()
                    self.assertIsInstance(annotations, dict)
