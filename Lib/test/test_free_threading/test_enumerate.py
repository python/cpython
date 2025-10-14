import unittest
import sys
from threading import Thread, Barrier

from test.support import threading_helper

threading_helper.requires_working_threading(module=True)

class EnumerateThreading(unittest.TestCase):

    @threading_helper.reap_threads
    def test_threading(self):
        number_of_threads = 10
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

        for it in range(number_of_iterations):
            enum = enumerate(tuple(range(start, start + n)))
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=work, args=[enum]))
            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()

if __name__ == "__main__":
    unittest.main()
