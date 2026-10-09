import unittest
from threading import Thread, Barrier, Event
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class BytesThreading(unittest.TestCase):
    @threading_helper.reap_threads
    def test_conversion_from_mutating_list(self):
        number_of_threads = 10
        number_of_iterations = 10
        barrier = Barrier(number_of_threads)

        x = [1, 2, 3, 4, 5]
        extends = [(ii,) * (2 + ii) for ii in range(number_of_threads)]

        def work(ii):
            barrier.wait()
            for _ in range(100):
                bytes(x)
                x.extend(extends[ii])
                if len(x) > 10:
                    x[:] = [0]

        for it in range(number_of_iterations):
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(Thread(target=work, args=[ii]))
            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()

    def test_racing_join_replace(self):
        # gh-158803: join() must not use a list item that another thread
        # replaces (and frees) concurrently.
        lst = [bytes(10) for _ in range(100)]
        done = Event()

        def writer():
            try:
                for _ in range(100):
                    for i in range(len(lst)):
                        lst[i] = bytearray(10) if i % 2 else bytes(10)
            finally:
                done.set()

        def reader():
            while not done.is_set():
                b''.join(lst)
                b'-'.join(lst)
                bytearray().join(lst)

        threading_helper.run_concurrently([writer] + [reader] * 4)


if __name__ == "__main__":
    unittest.main()
