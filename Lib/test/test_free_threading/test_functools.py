import random
import unittest

from functools import lru_cache
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


if __name__ == "__main__":
    unittest.main()
