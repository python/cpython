import unittest
from threading import Barrier, Thread
from test.support import threading_helper


class TestReversed(unittest.TestCase):
    def test_reversed(self):
        # Test reading out the iterator with multiple threads cannot corrupt
        # the reversed iterator state.
        # The reversed iterator is not guaranteed to be thread safe
        number_of_iterations = 10
        number_of_threads = 10
        size = 1_000

        barrier = Barrier(number_of_threads)
        def work(r):
            barrier.wait()
            while True:
                try:
                     l = r.__length_hint__()
                     next(r)
                except StopIteration:
                    break
                assert 0 <= l <= size
        x = tuple(range(size))

        for _ in range(number_of_iterations):
            r = reversed(x)
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(Thread(target=work, args=[r]))
            threading_helper.start_threads(worker_threads)
            barrier.reset()

if __name__ == "__main__":
    unittest.main()
