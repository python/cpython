import unittest
from threading import Thread, Barrier
from itertools import combinations, product
from test.support import threading_helper


threading_helper.requires_working_threading(module=True)

def test_concurrent_iteration(iterator, number_of_threads):
    barrier = Barrier(number_of_threads)
    def iterator_worker(it):
        barrier.wait()
        while True:
            try:
                _ = next(it)
            except StopIteration:
                return

    worker_threads = []
    for ii in range(number_of_threads):
        worker_threads.append(
            Thread(target=iterator_worker, args=[iterator]))

    with threading_helper.start_threads(worker_threads):
        pass

    barrier.reset()

class ItertoolsThreading(unittest.TestCase):

    @threading_helper.reap_threads
    def test_combinations(self):
        number_of_threads = 10
        number_of_iterations = 24

        for it in range(number_of_iterations):
            iterator = combinations((1, 2, 3, 4, 5), 2)
            test_concurrent_iteration(iterator, number_of_threads)

    @threading_helper.reap_threads
    def test_product(self):
        number_of_threads = 10
        number_of_iterations = 24

        for it in range(number_of_iterations):
            iterator = product((1, 2, 3, 4, 5), (10, 20, 30))
            test_concurrent_iteration(iterator, number_of_threads)


if __name__ == "__main__":
    unittest.main()
