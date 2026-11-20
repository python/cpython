import unittest
from threading import Thread, Barrier, Event
from test.support import threading_helper


class TestSetRepr(unittest.TestCase):
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


class RaceTestBase:
    def test_contains_mutate(self):
        """Test set contains operation combined with mutation."""
        barrier = Barrier(2, timeout=2)
        s = set()
        done = Event()

        NUM_LOOPS = 1000

        def read_set():
            barrier.wait()
            while not done.is_set():
                for i in range(self.SET_SIZE):
                    item = i >> 1
                    result = item in s

        def mutate_set():
            barrier.wait()
            for i in range(NUM_LOOPS):
                s.clear()
                for j in range(self.SET_SIZE):
                    s.add(j)
                for j in range(self.SET_SIZE):
                    s.discard(j)
                # executes the set_swap_bodies() function
                s.__iand__(set(k for k in range(10, 20)))
            done.set()

        threads = [Thread(target=read_set), Thread(target=mutate_set)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

    def test_contains_frozenset(self):
        barrier = Barrier(3, timeout=2)
        done = Event()

        NUM_LOOPS = 1_000

        # This mutates the key used for contains test, not the container
        # itself.  This works because frozenset allows the key to be a set().
        s = set()

        def mutate_set():
            barrier.wait()
            while not done.is_set():
                s.add(0)
                s.add(1)
                s.clear()

        def read_set():
            barrier.wait()
            container = frozenset([frozenset([0])])
            self.assertTrue(set([0]) in container)
            for _ in range(NUM_LOOPS):
                # Will return True when {0} is the key and False otherwise
                result = s in container
            done.set()

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

        NUM_LOOPS = 1_000
        SET_SIZE = self.SET_SIZE

        s = set(range(SET_SIZE))

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
                    s.update(range(SET_SIZE))
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


@threading_helper.requires_working_threading()
class SmallSetTest(RaceTestBase, unittest.TestCase):
    SET_SIZE = 6  # smaller than PySet_MINSIZE


@threading_helper.requires_working_threading()
class LargeSetTest(RaceTestBase, unittest.TestCase):
    SET_SIZE = 20  # larger than PySet_MINSIZE


class TestSetIteratorExhaustion(unittest.TestCase):
    def test_racing_shared_iterator_exhaustion(self):
        # gh-160573: exhausting a set iterator shared between threads must
        # not release the iterator's reference to the set more than once.
        for tp in (set, frozenset):
            cell = [iter(tp(range(8)))]

            def worker():
                for _ in range(5000):
                    it = cell[0]
                    try:
                        next(it)
                    except StopIteration:
                        cell[0] = iter(tp(range(8)))

            with self.subTest(tp=tp):
                threading_helper.run_concurrently(worker, nthreads=4)

    def test_racing_shared_iterator_exhaustion_contended(self):
        # Same as above, but the set's lock is contended: other threads
        # keep mutating and iterating the set while its iterator is shared.
        s = set(range(8))
        cell = [iter(s)]
        done = Event()

        def exhaust():
            for _ in range(5000):
                it = cell[0]
                try:
                    next(it)
                except StopIteration:
                    cell[0] = iter(s)
                except RuntimeError:
                    # The set changed size during iteration.
                    cell[0] = iter(s)

        def mutate():
            while not done.is_set():
                s.add(100)
                s.discard(100)

        def iterate():
            while not done.is_set():
                try:
                    for _ in s:
                        pass
                except RuntimeError:
                    pass

        def run_exhaust():
            try:
                threading_helper.run_concurrently(exhaust, nthreads=4)
            finally:
                done.set()

        threading_helper.run_concurrently([run_exhaust, mutate, iterate])

    def test_exhausted_iterator_stays_exhausted(self):
        s = {1}
        it = iter(s)
        self.assertEqual(list(it), [1])
        s.add(2)
        self.assertRaises(StopIteration, next, it)
        self.assertEqual(it.__length_hint__(), 0)
        self.assertEqual(list(it.__reduce__()[1][0]), [])


if __name__ == "__main__":
    unittest.main()
