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
        barrier = Barrier(NUM_REPR_THREADS + 1, timeout=2)
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
        barrier = Barrier(2, timeout=2)
        s = set()
        done = False

        NUM_ITEMS = 200
        NUM_LOOPS = 200

        def read_set():
            barrier.wait()
            while not done:
                for i in range(NUM_ITEMS):
                    item = i >> 1
                    result = item in s

        def mutate_set():
            nonlocal done
            barrier.wait()
            for i in range(NUM_LOOPS):
                s.clear()
                for j in range(NUM_ITEMS):
                    s.add(j)
                for j in range(NUM_ITEMS):
                    s.discard(j)
                # executes the set_swap_bodies() function
                s.__iand__(set(k for k in range(10, 20)))
            done = True

        threads = [Thread(target=read_set), Thread(target=mutate_set)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

    def test_contains_frozenset(self):
        barrier = Barrier(3, timeout=2)
        done = False

        NUM_LOOPS = 1_000

        # This mutates the key used for contains test, not the container
        # itself.  This works because frozenset allows the key to be a set().
        s = set()

        def mutate_set():
            barrier.wait()
            while not done:
                s.add(0)
                s.add(1)
                s.clear()

        def read_set():
            nonlocal done
            barrier.wait()
            container = frozenset([frozenset([0])])
            self.assertTrue(set([0]) in container)
            for _ in range(NUM_LOOPS):
                # Will return True when {0} is the key and False otherwise
                result = s in container
            done = True

        threads = [
            Thread(target=read_set),
            Thread(target=read_set),
            Thread(target=mutate_set),
        ]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

    def test_contains_hash_mutate(self):
        """Test set contains operation with mutating hash method."""
        barrier = Barrier(2, timeout=2)

        NUM_ITEMS = 20  # should be larger than PySet_MINSIZE
        NUM_LOOPS = 1_000

        s = set(range(NUM_ITEMS))

        class Key:
            def __init__(self):
                self.count = 0
                self.value = 0

            def __hash__(self):
                self.count += 1
                # This intends to trigger the SET_LOOKKEY_CHANGED case
                # of set_lookkey_threadsafe() since calling clear()
                # will cause the 'table' pointer to change.
                if self.count % 2 == 0:
                    s.clear()
                else:
                    s.update(range(NUM_ITEMS))
                return hash(self.value)

            def __eq__(self, other):
                return self.value == other

        key = Key()
        self.assertTrue(key in s)
        self.assertFalse(key in s)
        self.assertTrue(key in s)
        self.assertFalse(key in s)

        def read_set():
            barrier.wait()
            for i in range(NUM_LOOPS):
                result = key in s

        threads = [Thread(target=read_set), Thread(target=read_set)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()


if __name__ == "__main__":
    unittest.main()
