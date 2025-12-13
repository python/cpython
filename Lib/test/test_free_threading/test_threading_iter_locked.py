import time
import unittest
from threading import Thread, Barrier, iter_locked
from test.support import threading_helper


threading_helper.requires_working_threading(module=True)

class non_atomic_iterator:

    def __init__(self, it):
        self.it = iter(it)

    def __iter__(self):
        return self

    def __next__(self):
        a = next(self.it)
        t = time.perf_counter() + 1e-6
        while time.perf_counter() < t:
            pass
        b = next(self.it)
        return a, b

def count():
    i = 0
    while True:
        i += 1
        yield i

class iter_lockedThreading(unittest.TestCase):

    @threading_helper.reap_threads
    def test_iter_locked(self):
        number_of_threads = 10
        number_of_iterations = 8
        barrier = Barrier(number_of_threads)
        def work(it):
            while True:
                try:
                    a, b = next(it)
                    assert a + 1 == b
                except StopIteration:
                    break

        data = tuple(range(400))
        for it in range(number_of_iterations):
            iter_locked_iterator = iter_locked(non_atomic_iterator(data,))
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=work, args=[iter_locked_iterator]))

            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()

    @threading_helper.reap_threads
    def test_iter_locked_generator(self):
        number_of_threads = 5
        number_of_iterations = 4
        barrier = Barrier(number_of_threads)
        def work(it):
            barrier.wait()
            for _ in range(1_000):
                try:
                    next(it)
                except StopIteration:
                    break

        for it in range(number_of_iterations):
            generator = iter_locked(count())
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=work, args=[generator]))

            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()

if __name__ == "__main__":
    unittest.main()
