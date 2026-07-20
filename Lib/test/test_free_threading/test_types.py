import unittest
import threading
from typing import TypeVar
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)

NTHREADS = 10


class TestGenericAlias(unittest.TestCase):
    def test_parameters_race(self):
        # gh-153298

        T = TypeVar('T')
        slot = [list[T]]

        def access():
            for _ in range(2000):
                try:
                    _ = slot[0].__parameters__
                except Exception:
                    pass

        def refresh():
            for _ in range(2000):
                slot[0] = list[T]

        threading_helper.run_concurrently([
            *[access for _ in range(6)],
            *[refresh for _ in range(2)],
        ])

    def test_shared_generic_alias_iter(self):
        # See https://github.com/python/cpython/issues/154043
        def worker(it, barrier):
            barrier.wait()
            try:
                next(it)
            except StopIteration:
                pass
                bar = threading.Barrier(NTHREADS)

        bar = threading.Barrier(NTHREADS)
        number_of_iterations = 50
        for _ in range(number_of_iterations):
            shared = iter(list[int])
            threading_helper.run_concurrently(
                worker_func=worker, nthreads=NTHREADS, args=(shared, bar)
            )


if __name__ == "__main__":
    unittest.main()
