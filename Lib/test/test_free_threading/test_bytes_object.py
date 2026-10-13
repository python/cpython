import unittest
from threading import Event
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class BytesThreading(unittest.TestCase):
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
