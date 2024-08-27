import unittest
from threading import Thread

from test.support import threading_helper


class ZipThreading(unittest.TestCase):
    @staticmethod
    def work(enum):
        while True:
            try:
                next(enum)
            except StopIteration:
                break

    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_threading(self):
        number_of_threads = 8
        number_of_iterations = 8
        n = 40_000
        enum = zip(range(n), range(n))
        for _ in range(number_of_iterations):
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(
                        target=self.work,
                        args=[
                            enum,
                        ],
                    )
                )
            for t in worker_threads:
                t.start()
            for t in worker_threads:
                t.join()


if __name__ == "__main__":
    unittest.main()
