import random
import sys
import unittest

from functools import Placeholder, lru_cache, partial
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


class Callable:
    # Instances have no vectorcall slot, so partial() dispatches them through
    # tp_call (partial_call) instead of partial_vectorcall.
    def __call__(self, x, y=0, *extra, z=0):
        return (x, y, z)


def target(x, y=0, *extra, z=0):
    return (x, y, z)


@threading_helper.requires_working_threading()
class TestPartial(unittest.TestCase):
    # gh-157841: __setstate__() publishes func, args, keywords and the
    # placeholder count as four independent stores with no lock, so a reader
    # can observe a mixture of two states.  Every state written below carries
    # the same value twice -- once reachable as a positional argument, once as
    # a keyword -- so a torn read shows up as a mismatch and not only as a
    # crash.

    @staticmethod
    def state(i):
        if i % 2:
            # one placeholder, so the caller's argument supplies x
            return (target, (Placeholder, i), {"z": i}, None)
        return (target, (i, i), {"z": i}, None)

    def check_coherent(self, p):
        x, y, z = p(0)
        self.assertEqual(y, z, "torn read: mixed the state of two setstate()s")

    def check_reduce_coherent(self, p):
        (_, (func,), (func2, args, keywords, dict_)) = p.__reduce__()
        self.assertIs(func, func2)
        self.assertEqual(args[-1], keywords["z"],
                         "torn read: mixed the state of two setstate()s")

    def check_flatten_coherent(self, p):
        # partial_new() reaches inside p to flatten it, so it reads func, args,
        # keywords and the placeholder count out of a live, mutable object.
        x, y, z = partial(p, 3)()
        self.assertEqual(y, z, "torn read: mixed the state of two setstate()s")

    def _test_racing_setstate(self, p, state, readers):
        num_threads = 10
        ops_per_thread = 1000
        old_interval = sys.getswitchinterval()
        sys.setswitchinterval(1e-6)
        self.addCleanup(sys.setswitchinterval, old_interval)

        b = Barrier(num_threads)
        errors = []

        def thread_func(n):
            b.wait()
            for j in range(ops_per_thread):
                i = j * num_threads + n
                try:
                    if i % 5 == 0:
                        p.__setstate__(state(i))
                    else:
                        readers[i % len(readers)](p)
                except Exception as exc:
                    errors.append(f"{type(exc).__name__}: {exc}")

        threads = [Thread(target=thread_func, args=(n,))
                   for n in range(num_threads)]
        with threading_helper.start_threads(threads):
            pass

        if errors:
            self.fail(f"{len(errors)} failures, first: {errors[0]}")
        p.__setstate__(state(-1))
        self.check_coherent(p)
        self.check_reduce_coherent(p)
        self.check_flatten_coherent(p)

    def test_concurrent_setstate_and_reads(self):
        readers = [self.check_coherent, self.check_reduce_coherent,
                   self.check_flatten_coherent, repr]
        # The starting state is itself coherent, so a reader that runs before
        # the first __setstate__() is not a false failure.
        self._test_racing_setstate(partial(target, 0, 0, z=0), self.state,
                                   readers)

    def test_concurrent_setstate_and_reads_no_vectorcall(self):
        wrapped = Callable()

        def state(i):
            func, args, keywords, dict_ = self.state(i)
            return (wrapped, args, keywords, dict_)

        readers = [lambda p: p(0), self.check_reduce_coherent,
                   self.check_flatten_coherent, repr]
        self._test_racing_setstate(partial(wrapped, 0, 0, z=0), state, readers)

    def test_concurrent_setstate_writers(self):
        # Concurrent writers must not drop a displaced value's reference twice.
        num_threads = 10
        ops_per_thread = 1000
        b = Barrier(num_threads)
        p = partial(target, 0, 0, z=0)
        errors = []

        def thread_func(n):
            b.wait()
            for j in range(ops_per_thread):
                try:
                    p.__setstate__(self.state(j * num_threads + n))
                except Exception as exc:
                    errors.append(f"{type(exc).__name__}: {exc}")

        threads = [Thread(target=thread_func, args=(n,))
                   for n in range(num_threads)]
        with threading_helper.start_threads(threads):
            pass

        if errors:
            self.fail(f"{len(errors)} failures, first: {errors[0]}")
        self.check_coherent(p)
        self.check_reduce_coherent(p)


if __name__ == "__main__":
    unittest.main()
