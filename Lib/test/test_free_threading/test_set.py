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

    def test_pop_concurrent(self):
        """Test set.pop() from several threads."""
        NUM_THREADS = 4
        NUM_ITERS = 20

        for _ in range(NUM_ITERS):
            items = set(range(self.SET_SIZE))
            s = set(items)
            barrier = Barrier(NUM_THREADS, timeout=2)
            popped = [[] for _ in range(NUM_THREADS)]

            def pop_set(out):
                barrier.wait()
                while True:
                    try:
                        out.append(s.pop())
                    except KeyError:
                        break

            threads = [Thread(target=pop_set, args=(out,)) for out in popped]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            all_popped = [item for out in popped for item in out]
            self.assertEqual(len(all_popped), len(items))
            self.assertEqual(set(all_popped), items)
            self.assertEqual(len(s), 0)

    def test_add_concurrent(self):
        """Test set.add() with disjoint inputs from several threads."""
        NUM_THREADS = 4
        NUM_ITERS = 20

        for _ in range(NUM_ITERS):
            inputs = [
                range(i * self.SET_SIZE, (i + 1) * self.SET_SIZE)
                for i in range(NUM_THREADS)
            ]
            expected = set().union(*inputs)
            s = set()
            barrier = Barrier(NUM_THREADS, timeout=2)

            def add_items(items):
                barrier.wait()
                for item in items:
                    s.add(item)

            threads = [Thread(target=add_items, args=(items,))
                       for items in inputs]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            self.assertEqual(s, expected)

    def test_remove_discard_concurrent(self):
        """Test set.remove() and set.discard() from several threads."""
        NUM_THREADS = 4
        NUM_ITERS = 20

        for method_name in ("remove", "discard"):
            with self.subTest(method=method_name):
                for _ in range(NUM_ITERS):
                    inputs = [
                        range(i * self.SET_SIZE, (i + 1) * self.SET_SIZE)
                        for i in range(NUM_THREADS)
                    ]
                    untouched = set(range(
                        NUM_THREADS * self.SET_SIZE,
                        (NUM_THREADS + 1) * self.SET_SIZE,
                    ))
                    s = set().union(untouched, *inputs)
                    barrier = Barrier(NUM_THREADS, timeout=2)

                    def remove_items(items):
                        barrier.wait()
                        method = getattr(s, method_name)
                        for item in items:
                            method(item)

                    threads = [Thread(target=remove_items, args=(items,))
                               for items in inputs]
                    for t in threads:
                        t.start()
                    for t in threads:
                        t.join()

                    self.assertEqual(s, untouched)

    def test_copy_clear_concurrent(self):
        """Test set.copy() while another thread clears the set."""
        NUM_ITERS = 20

        for _ in range(NUM_ITERS):
            items = set(range(self.SET_SIZE))
            s = set(items)
            copies = []
            barrier = Barrier(2, timeout=2)

            def copy_set():
                barrier.wait()
                copies.append(s.copy())

            def clear_set():
                barrier.wait()
                s.clear()

            threads = [Thread(target=copy_set), Thread(target=clear_set)]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            self.assertIn(copies[0], (items, set()))
            self.assertEqual(s, set())

    def test_update_concurrent(self):
        """Test updates of one shared set from disjoint source sets."""
        NUM_THREADS = 4
        NUM_ITERS = 20

        sources = [
            set(range(i * self.SET_SIZE, (i + 1) * self.SET_SIZE))
            for i in range(NUM_THREADS)
        ]
        original_sources = [set(source) for source in sources]
        initial = set(range(
            NUM_THREADS * self.SET_SIZE,
            (NUM_THREADS + 1) * self.SET_SIZE,
        ))
        expected = set().union(initial, *sources)

        for _ in range(NUM_ITERS):
            s = set(initial)
            barrier = Barrier(NUM_THREADS, timeout=2)

            def update_set(source):
                barrier.wait()
                s.update(source)

            threads = [Thread(target=update_set, args=(source,))
                        for source in sources]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            self.assertEqual(s, expected)
            self.assertEqual(sources, original_sources)

    def test_update_opposing(self):
        """Test opposing updates of two shared sets."""
        NUM_ITERS = 20

        for _ in range(NUM_ITERS):
            left = set(range(self.SET_SIZE))
            right = set(range(self.SET_SIZE, self.SET_SIZE * 2))
            expected = set(range(self.SET_SIZE * 2))
            barrier = Barrier(2, timeout=2)

            def update_set(target, source):
                barrier.wait()
                target.update(source)

            threads = [
                Thread(target=update_set, args=(left, right)),
                Thread(target=update_set, args=(right, left)),
            ]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            self.assertEqual(left, expected)
            self.assertEqual(right, expected)

    def test_difference_update_concurrent(self):
        """Test set.difference_update() with disjoint source sets."""
        NUM_THREADS = 4
        NUM_ITERS = 20

        sources = [
            set(range(i * self.SET_SIZE, (i + 1) * self.SET_SIZE))
            for i in range(NUM_THREADS)
        ]
        original_sources = [set(source) for source in sources]
        untouched = set(range(
            NUM_THREADS * self.SET_SIZE,
            (NUM_THREADS + 1) * self.SET_SIZE,
        ))
        items = set().union(untouched, *sources)

        for _ in range(NUM_ITERS):
            s = set(items)
            barrier = Barrier(NUM_THREADS, timeout=2)

            def difference_update(source):
                barrier.wait()
                s.difference_update(source)

            threads = [Thread(target=difference_update, args=(source,))
                       for source in sources]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            self.assertEqual(s, untouched)
            self.assertEqual(sources, original_sources)

    def test_difference_update_opposing(self):
        """Test opposing difference updates of two shared sets."""
        NUM_ITERS = 20

        for _ in range(NUM_ITERS):
            left = set(range(self.SET_SIZE))
            right = set(range(1, self.SET_SIZE + 1))
            original_left = set(left)
            original_right = set(right)
            left_only = {0}
            right_only = {self.SET_SIZE}
            barrier = Barrier(2, timeout=2)

            def difference_update(target, source):
                barrier.wait()
                target.difference_update(source)

            threads = [
                Thread(target=difference_update, args=(left, right)),
                Thread(target=difference_update, args=(right, left)),
            ]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            actual = (left, right)
            expected = [
                (left_only, original_right),
                (original_left, right_only),
            ]
            self.assertIn(actual, expected)

    def test_symmetric_difference_update_concurrent(self):
        """Test symmetric difference updates with disjoint source sets."""
        NUM_THREADS = 4
        NUM_ITERS = 20

        sources = [
            set(range(i * self.SET_SIZE, (i + 1) * self.SET_SIZE))
            for i in range(NUM_THREADS)
        ]
        items = set().union(*sources)
        initial = {item for item in items if item % 2 == 0}
        expected = {item for item in items if item % 2 == 1}

        for _ in range(NUM_ITERS):
            s = set(initial)
            barrier = Barrier(NUM_THREADS, timeout=2)

            def symmetric_difference_update(source):
                barrier.wait()
                s.symmetric_difference_update(source)

            threads = [
                Thread(target=symmetric_difference_update, args=(source,))
                for source in sources
            ]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            self.assertEqual(s, expected)

    def test_symmetric_difference_update_opposing(self):
        """Test opposing symmetric difference updates of shared sets."""
        NUM_ITERS = 20

        for _ in range(NUM_ITERS):
            left = set(range(self.SET_SIZE))
            right = set(range(self.SET_SIZE, self.SET_SIZE * 2))
            original_left = set(left)
            original_right = set(right)
            union = set(range(self.SET_SIZE * 2))
            barrier = Barrier(2, timeout=2)

            def symmetric_update(target, source):
                barrier.wait()
                target.symmetric_difference_update(source)

            threads = [
                Thread(target=symmetric_update, args=(left, right)),
                Thread(target=symmetric_update, args=(right, left)),
            ]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            actual = (left, right)
            expected = [
                (union, original_left),
                (original_right, union),
            ]
            self.assertIn(actual, expected)

    # TODO: test_intersection_update_concurrent
    # TODO: test_intersection_update_opposing

@threading_helper.requires_working_threading()
class SmallSetTest(RaceTestBase, unittest.TestCase):
    SET_SIZE = 6  # smaller than PySet_MINSIZE


@threading_helper.requires_working_threading()
class LargeSetTest(RaceTestBase, unittest.TestCase):
    SET_SIZE = 20  # larger than PySet_MINSIZE


if __name__ == "__main__":
    unittest.main()
