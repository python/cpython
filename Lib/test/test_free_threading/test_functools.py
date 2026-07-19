import random
import time
import unittest

from functools import lru_cache, cached_property
from threading import Barrier, Thread

from test.support import threading_helper

@threading_helper.requires_working_threading()
class TestLRUCache(unittest.TestCase):

    def _test_concurrent_operations(self, maxsize):
        num_threads = 10
        b = Barrier(num_threads)
        @lru_cache(maxsize=maxsize)
        def func(arg=0):
            return object()


        def thread_func():
            b.wait()
            for i in range(1000):
                r = random.randint(0, 1000)
                if i < 800:
                    func(i)
                elif i < 900:
                    func.cache_info()
                else:
                    func.cache_clear()

        threads = []
        for i in range(num_threads):
            t = Thread(target=thread_func)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass

    def test_concurrent_operations_unbounded(self):
        self._test_concurrent_operations(maxsize=None)

    def test_concurrent_operations_bounded(self):
        self._test_concurrent_operations(maxsize=128)

    def _test_reentrant_cache_clear(self, maxsize):
        num_threads = 10
        b = Barrier(num_threads)
        @lru_cache(maxsize=maxsize)
        def func(arg=0):
            func.cache_clear()
            return object()


        def thread_func():
            b.wait()
            for i in range(1000):
                func(random.randint(0, 10000))

        threads = []
        for i in range(num_threads):
            t = Thread(target=thread_func)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass

    def test_reentrant_cache_clear_unbounded(self):
        self._test_reentrant_cache_clear(maxsize=None)

    def test_reentrant_cache_clear_bounded(self):
        self._test_reentrant_cache_clear(maxsize=128)

    def test_unbounded_cache_idempotent(self):
        all_identical = True

        @lru_cache(maxsize=None)
        def func(arg=0):
            time.sleep(0.01)
            return object()

        def thread_func():
            nonlocal all_identical

            for i in range(1000):
                if func(i) is not func(i):
                    all_identical = False

        threads = []
        for _ in range(10):
            t = Thread(target=thread_func)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass

        self.assertTrue(all_identical)


    def test_cached_property_idempotent(self):
        all_identical = True

        class C:
            @cached_property
            def prop(self):
                time.sleep(0.01)
                return object()

        c = C()

        def thread_func():
            nonlocal all_identical

            if c.prop is not c.prop:
                all_identical = False

        threads = []
        for _ in range(10):
            t = Thread(target=thread_func)
            threads.append(t)

        with threading_helper.start_threads(threads):
            pass

        self.assertTrue(all_identical)


if __name__ == "__main__":
    unittest.main()
