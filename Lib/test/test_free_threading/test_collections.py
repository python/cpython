import unittest
from collections import Counter, OrderedDict, deque
from copy import copy
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class TestDeque(unittest.TestCase):
    def test_copy_race(self):
        # gh-144809: Test that deque copy is thread safe. It previously
        # could raise a "deque mutated during iteration" error.
        d = deque(range(100))

        def mutate():
            for i in range(1000):
                d.append(i)
                if len(d) > 200:
                    d.popleft()

        def copy_loop():
            for _ in range(1000):
                copy(d)

        threading_helper.run_concurrently([mutate, copy_loop])

    def test_index_race_in_ac(self):
        # gh-150750: There was a c_default specified as `Py_SIZE(self)`,
        # it was used without a critical section.

        d = deque(range(100))

        def index():
            for _ in range(10000):
                try:
                    d.index(50)
                except ValueError:
                    pass

        def mutate():
            for _ in range(10000):
                d.append(0)
                d.clear()
                d.extend(range(100))
                d.appendleft(-1)

        threading_helper.run_concurrently(
            [index, *[mutate for _ in range(3)]],
        )


class TestCounter(unittest.TestCase):
    def test_update_concurrent(self):
        # gh-151633: concurrent Counter.update calls must not cause use-after-free
        NTHREADS = 4
        PER_THREAD = 5000
        c = Counter()
        data = ('x',) * PER_THREAD
        threading_helper.run_concurrently(
            c.update, nthreads=NTHREADS, args=(data,)
        )
        self.assertEqual(c['x'], NTHREADS * PER_THREAD)


class TestOrderedDict(unittest.TestCase):
    def test_iterator_update_clear_race(self):
        # gh-151627: OrderedDict iterator construction must not race with
        # concurrent clear()/update() operations that mutate the linked list.
        od = OrderedDict((i, i) for i in range(100))

        def mutate():
            for i in range(5000):
                od.clear()
                od.update(((i, i), (i + 1, i + 1), (i + 2, i + 2)))

        def iterate():
            for _ in range(5000):
                try:
                    for _ in od:
                        pass
                    list(reversed(od))
                except RuntimeError:
                    pass

        threading_helper.run_concurrently(
            [mutate, *[iterate for _ in range(8)]],
        )


    def test_shared_iterator(self):
        # Deque iterators are advanced under the deque's lock, so threads
        # sharing one iterator must together see every item exactly once.
        n = 20000
        for make_iter in (iter, reversed):
            with self.subTest(make_iter=make_iter.__name__):
                d = deque(range(n))
                it = make_iter(d)
                results = []

                def consume():
                    results.append(list(x for x in it))

                threading_helper.run_concurrently([consume] * 4)
                self.assertEqual(sorted(x for r in results for x in r),
                                 list(range(n)))

    def test_iterate_while_mutating(self):
        d = deque(range(100))

        def mutate():
            for i in range(5000):
                d.append(i)
                d.popleft()

        def iterate():
            for _ in range(500):
                it = iter(d)
                try:
                    for _ in it:
                        pass
                except RuntimeError:
                    pass
                it.__length_hint__()
                it.__reduce__()

        threading_helper.run_concurrently([mutate, iterate, iterate])

    def test_bulk_materialize_race(self):
        # list(d) copies the deque under its critical section and must see
        # a consistent snapshot; list(iter(d)) and str.join() first create
        # an iterator, so a mutation in between is reported as by next().
        d = deque(str(i) for i in range(200))

        def mutate():
            for i in range(5000):
                d.append(str(i))
                d.popleft()
                if i % 64 == 0:
                    d.rotate(7)

        def materialize():
            for _ in range(2000):
                self.assertIn(len(list(d)), (200, 201))
                for func in (list, ''.join):
                    try:
                        res = func(iter(d))
                    except RuntimeError:
                        continue
                    if func is list:
                        self.assertIn(len(res), (200, 201))

        threading_helper.run_concurrently([mutate, materialize, materialize])

    def test_shared_iterator_bulk_and_next(self):
        # list.extend(it) and next(it) both advance a shared iterator while
        # holding the deque's lock: every item is produced exactly once.
        n = 20000
        for _ in range(5):
            d = deque(range(n))
            it = iter(d)
            results = []

            def bulk():
                got = []
                got.extend(it)
                results.append(got)

            def step():
                results.append([x for x in it])

            threading_helper.run_concurrently([step, bulk, step, step])
            self.assertEqual(sorted(x for r in results for x in r),
                             list(range(n)))

if __name__ == "__main__":
    unittest.main()
