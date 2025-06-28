import unittest
from threading import Thread, Barrier
from itertools import batched, cycle
from test.support import threading_helper


threading_helper.requires_working_threading(module=True)

class ItertoolsThreading(unittest.TestCase):

    @threading_helper.reap_threads
    def test_batched(self):
        number_of_threads = 10
        number_of_iterations = 20
        barrier = Barrier(number_of_threads)
        def work(it):
            barrier.wait()
            while True:
                try:
                    _ = next(it)
                except StopIteration:
                    break

        data = tuple(range(1000))
        for it in range(number_of_iterations):
            batch_iterator = batched(data, 2)
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=work, args=[batch_iterator]))

            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()

    @threading_helper.reap_threads
    def test_cycle(self):
        number_of_threads = 6
        number_of_iterations = 10
        number_of_cycles = 400

        barrier = Barrier(number_of_threads)
        def work(it):
            barrier.wait()
            for _ in range(number_of_cycles):
                try:
                    next(it)
                except StopIteration:
                    pass

        data = (1, 2, 3, 4)
        for it in range(number_of_iterations):
            cycle_iterator = cycle(data)
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=work, args=[cycle_iterator]))

            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()


if __name__ == "__main__":
    unittest.main()
