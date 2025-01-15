import unittest
import sys
from threading import Thread, Barrier

from test.support import threading_helper


class EnumerateThreading(unittest.TestCase):

    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_threading(self):
        number_of_threads = 4
        number_of_iterations = 8
        n = 100
        start = sys.maxsize - 40
        barrier = Barrier(number_of_threads)
        def work(enum):
            barrier.wait()
            while True:
                try:
                    _ = next(enum)
                except StopIteration:
                    break

        for _ in range(number_of_iterations):
            enum = enumerate(range(start, start + n))
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=work, args=[enum]))
            for t in worker_threads:
                t.start()
            for t in worker_threads:
                t.join()

            barrier.reset()

if __name__ == "__main__":
    unittest.main()
