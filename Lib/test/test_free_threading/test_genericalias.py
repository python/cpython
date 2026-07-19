import threading
import unittest

from test.support import threading_helper


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestGenericAlias(unittest.TestCase):
    def worker(self, it, barrier):
        barrier.wait()
        try:
            next(it)
        except StopIteration:
            pass

    def test_shared_generic_alias_iter(self):
        # See https://github.com/python/cpython/issues/154043
        bar = threading.Barrier(NTHREADS)
        number_of_iterations = 50
        for _ in range(number_of_iterations):
            shared = iter(list[int])
            threading_helper.run_concurrently(
                worker_func=self.worker, nthreads=NTHREADS, args=(shared, bar)
            )


if __name__ == "__main__":
    unittest.main()
