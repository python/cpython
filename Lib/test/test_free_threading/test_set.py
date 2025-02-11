import unittest

from threading import Thread, Barrier
from unittest import TestCase

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestSet(TestCase):
    def test_repr_clear(self):
        """Test repr() of a set while another thread is calling clear()"""
        NUM_ITERS = 10
        NUM_REPR_THREADS = 10
        barrier = Barrier(NUM_REPR_THREADS + 1)
        s = {1, 2, 3, 4, 5, 6, 7, 8}
        expected = {repr(set()), repr(s)}

        def clear_set():
            barrier.wait()
            s.clear()

        def repr_set():
            nonlocal set_repr
            barrier.wait()
            set_reprs.add(repr(s))

        for _ in range(NUM_ITERS):
            set_reprs = set()
            threads = [Thread(target=clear_set)]
            for _ in range(NUM_REPR_THREADS):
                threads.append(Thread(target=repr_set))
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            self.assertEqual(set_reprs, expected)


if __name__ == "__main__":
    unittest.main()
