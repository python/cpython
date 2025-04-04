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

        def clear_set():
            barrier.wait()
            s.clear()

        def repr_set():
            barrier.wait()
            set_reprs.append(repr(s))

        for _ in range(NUM_ITERS):
            set_reprs = []
            threads = [Thread(target=clear_set)]
            for _ in range(NUM_REPR_THREADS):
                threads.append(Thread(target=repr_set))
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            for set_repr in set_reprs:
                self.assertIn(set_repr, ("set()", "{1, 2, 3, 4, 5, 6, 7, 8}"))

    def test_contains_mutate(self):
        """Test set contains operation combined with mutation."""
        barrier = Barrier(2)
        s = set()
        done = False

        def read_set():
            barrier.wait()
            while not done:
                for i in range(64):
                    result = (i % 16) in s

        def mutate_set():
            nonlocal done
            barrier.wait()
            for i in range(10):
                s.clear()
                for j in range(16):
                    s.add(j)
                for j in range(16):
                    s.discard(j)
                # executes the set_swap_bodies() function
                s.__iand__(set(k for k in range(10, 20)))
            done = True

        threads = [Thread(target=read_set), Thread(target=mutate_set)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()


if __name__ == "__main__":
    unittest.main()
