import unittest
from threading import Thread
from test.support import threading_helper


@threading_helper.reap_threads
@threading_helper.requires_working_threading()
def test_reversed_threading(self):
    # Test reading out the iterator with multiple threads cannot corrupt
    # the reversed iterator.
    # The reversed iterator is not guaranteed to be thread safe
    def work(r, output):
        while True:
            try:
                value = next(r)
            except StopIteration:
                break
            else:
                output.append(value)

    number_of_threads = 10
    x = tuple(range(4_000))
    r = reversed(x)
    worker_threads = []
    output = []
    for ii in range(number_of_threads):
        worker_threads.append(Thread(target=work, args=[r, output]))
    _ = [t.start() for t in worker_threads]
    _ = [t.join() for t in worker_threads]

if __name__ == "__main__":
    unittest.main()
