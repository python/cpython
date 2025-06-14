import unittest

from threading import Thread, Barrier
from test.support import threading_helper


def run_concurrently(worker_func, args, nthreads):
    """
    Run the worker function concurrently in multiple threads.
    """
    barrier = Barrier(nthreads)

    def wrapper_func(*args):
        # Wait for all threads to reach this point before proceeding.
        barrier.wait()
        worker_func(*args)

    with threading_helper.catch_threading_exception() as cm:
        workers = (
            Thread(target=wrapper_func, args=args) for _ in range(nthreads)
        )
        with threading_helper.start_threads(workers):
            pass

        # If a worker thread raises an exception, re-raise it.
        if cm.exc_value is not None:
            raise cm.exc_value


@threading_helper.requires_working_threading()
class TestFTUtils(unittest.TestCase):
    def test_run_concurrently(self):
        lst = []

        def worker(lst):
            lst.append(42)

        nthreads = 10
        run_concurrently(worker, (lst,), nthreads)
        self.assertEqual(lst, [42] * nthreads)

    def test_run_concurrently_raise(self):
        def worker():
            raise RuntimeError("Error")

        nthreads = 3
        with self.assertRaises(RuntimeError):
            run_concurrently(worker, (), nthreads)


if __name__ == "__main__":
    unittest.main()
