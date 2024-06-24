import unittest
from threading import Thread
from test.support import threading_helper


@threading_helper.reap_threads
@threading_helper.requires_working_threading()
def test_reversed_threading(self):
    # Test that when reading out with multiple treads no identical elemenents are returned
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

    if sorted(output) != x:
        raise ValueError('reversed returned value from sequence twice')


if __name__ == "__main__":
    unittest.main()
