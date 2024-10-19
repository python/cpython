import unittest
import sys
from threading import Thread

from test.support import threading_helper


class EnumerateThreading(unittest.TestCase):
    @staticmethod
    def work(enum, start):
        while True:
            try:
                value = next(enum)
            except StopIteration:
                break


    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_threading(self):
        number_of_threads = 4
        number_of_iterations = 8
        n = 100
        start = sys.maxsize - 40

        for _ in range(number_of_iterations):
            enum = enumerate(range(start, start + n))
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=self.work, args=[enum, start]))
            for t in worker_threads:
                t.start()
            for t in worker_threads:
                t.join()


if __name__ == "__main__":
    unittest.main()
