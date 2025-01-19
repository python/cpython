import concurrent.futures
import unittest
from threading import Thread, Barrier
from unittest import TestCase

from test.support import threading_helper, Py_GIL_DISABLED

threading_helper.requires_working_threading(module=True)


def get_func_annotate(f, b):
    b.wait()
    return f.__annotation__


@unittest.skipUnless(Py_GIL_DISABLED, "Enable only in FT build")
class TestFTFuncAnnotations(TestCase):
    def test_concurrent_read(self):
        def f(x: int) -> int:
            return x + 1

        num_threads = 8
        b = Barrier(num_threads)
        threads = []

        with concurrent.futures.ThreadPoolExecutor(max_workers=num_threads) as executor:
            futures = {executor.submit(get_func_annotate, f, b): i for i in range(num_threads)}
            for fut in concurrent.futures.as_completed(futures):
                annotate = fut.result()
                self.assertIsNotNone(annotate)
                self.assertEqual(annotate, {'x': int, 'return': int})
