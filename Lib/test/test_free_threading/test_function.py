import concurrent.futures
import unittest
import inspect
from threading import Barrier

from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


def get_func_annotation(f, b):
    b.wait()
    return inspect.get_annotations(f)


def get_func_annotation_dunder(f, b):
    b.wait()
    return f.__annotations__


def set_func_annotation(f, b):
    b.wait()
    f.__annotations__ = {'x': int, 'y': int, 'return': int}
    return f.__annotations__


class TestFunction(unittest.TestCase):
    NUM_THREADS = 4

    def test_name_attribute_race(self):
        # gh-153297
        def shared_func():
            return 1

        def setter():
            for i in range(2000):
                shared_func.__name__ = "q_%d_%d" % (id(i), i & 15)

        def reader():
            for _ in range(2000):
                _ = shared_func.__name__
                repr(shared_func)

        threading_helper.run_concurrently([
            *[setter for _ in range(6)],
            *[reader for _ in range(4)],
        ])

    def test_qualname_attribute_race(self):
        # gh-153297
        def shared_func():
            return 1

        def setter():
            for i in range(2000):
                shared_func.__qualname__ = "q_%d_%d" % (id(i), i & 15)

        def reader():
            for _ in range(2000):
                _ = shared_func.__qualname__
                repr(shared_func)

        threading_helper.run_concurrently([
            *[setter for _ in range(6)],
            *[reader for _ in range(4)],
        ])

    def test_concurrent_read_annotations(self):
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

    def test_concurrent_write_annotations(self):
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
