import unittest

import heapq

from enum import Enum
from threading import Barrier, Lock
from random import shuffle, randint

from test.support import threading_helper
from test.support.threading_helper import run_concurrently
from test import test_heapq


NTHREADS = 10
OBJECT_COUNT = 5_000


class Heap(Enum):
    MIN = 1
    MAX = 2


@threading_helper.requires_working_threading()
class TestHeapq(unittest.TestCase):
    def setUp(self):
        self.test_heapq = test_heapq.TestHeapPython()

    def test_racing_heapify(self):
        heap = list(range(OBJECT_COUNT))
        shuffle(heap)

        run_concurrently(
            worker_func=heapq.heapify, nthreads=NTHREADS, args=(heap,)
        )
        self.test_heapq.check_invariant(heap)

    def test_racing_heappush(self):
        heap = []

        def heappush_func(heap):
            for item in reversed(range(OBJECT_COUNT)):
                heapq.heappush(heap, item)

        run_concurrently(
            worker_func=heappush_func, nthreads=NTHREADS, args=(heap,)
        )
        self.test_heapq.check_invariant(heap)

    def test_racing_heappop(self):
        heap = self.create_heap(OBJECT_COUNT, Heap.MIN)

        # Each thread pops (OBJECT_COUNT / NTHREADS) items
        self.assertEqual(OBJECT_COUNT % NTHREADS, 0)
        per_thread_pop_count = OBJECT_COUNT // NTHREADS

        def heappop_func(heap, pop_count):
            local_list = []
            for _ in range(pop_count):
                item = heapq.heappop(heap)
                local_list.append(item)

            # Each local list should be sorted
            self.assertTrue(self.is_sorted_ascending(local_list))

        run_concurrently(
            worker_func=heappop_func,
            nthreads=NTHREADS,
            args=(heap, per_thread_pop_count),
        )
        self.assertEqual(len(heap), 0)

    def test_racing_heappushpop(self):
        heap = self.create_heap(OBJECT_COUNT, Heap.MIN)
        pushpop_items = self.create_random_list(-5_000, 10_000, OBJECT_COUNT)

        def heappushpop_func(heap, pushpop_items):
            for item in pushpop_items:
                popped_item = heapq.heappushpop(heap, item)
                self.assertTrue(popped_item <= item)

        run_concurrently(
            worker_func=heappushpop_func,
            nthreads=NTHREADS,
            args=(heap, pushpop_items),
        )
        self.assertEqual(len(heap), OBJECT_COUNT)
        self.test_heapq.check_invariant(heap)

    def test_racing_heapreplace(self):
        heap = self.create_heap(OBJECT_COUNT, Heap.MIN)
        replace_items = self.create_random_list(-5_000, 10_000, OBJECT_COUNT)

        def heapreplace_func(heap, replace_items):
            for item in replace_items:
                heapq.heapreplace(heap, item)

        run_concurrently(
            worker_func=heapreplace_func,
            nthreads=NTHREADS,
            args=(heap, replace_items),
        )
        self.assertEqual(len(heap), OBJECT_COUNT)
        self.test_heapq.check_invariant(heap)

    def test_racing_heapify_max(self):
        max_heap = list(range(OBJECT_COUNT))
        shuffle(max_heap)

        run_concurrently(
            worker_func=heapq.heapify_max, nthreads=NTHREADS, args=(max_heap,)
        )
        self.test_heapq.check_max_invariant(max_heap)

    def test_racing_heappush_max(self):
        max_heap = []

        def heappush_max_func(max_heap):
            for item in range(OBJECT_COUNT):
                heapq.heappush_max(max_heap, item)

        run_concurrently(
            worker_func=heappush_max_func, nthreads=NTHREADS, args=(max_heap,)
        )
        self.test_heapq.check_max_invariant(max_heap)

    def test_racing_heappop_max(self):
        max_heap = self.create_heap(OBJECT_COUNT, Heap.MAX)

        # Each thread pops (OBJECT_COUNT / NTHREADS) items
        self.assertEqual(OBJECT_COUNT % NTHREADS, 0)
        per_thread_pop_count = OBJECT_COUNT // NTHREADS

        def heappop_max_func(max_heap, pop_count):
            local_list = []
            for _ in range(pop_count):
                item = heapq.heappop_max(max_heap)
                local_list.append(item)

            # Each local list should be sorted
            self.assertTrue(self.is_sorted_descending(local_list))

        run_concurrently(
            worker_func=heappop_max_func,
            nthreads=NTHREADS,
            args=(max_heap, per_thread_pop_count),
        )
        self.assertEqual(len(max_heap), 0)

    def test_racing_heappushpop_max(self):
        max_heap = self.create_heap(OBJECT_COUNT, Heap.MAX)
        pushpop_items = self.create_random_list(-5_000, 10_000, OBJECT_COUNT)

        def heappushpop_max_func(max_heap, pushpop_items):
            for item in pushpop_items:
                popped_item = heapq.heappushpop_max(max_heap, item)
                self.assertTrue(popped_item >= item)

        run_concurrently(
            worker_func=heappushpop_max_func,
            nthreads=NTHREADS,
            args=(max_heap, pushpop_items),
        )
        self.assertEqual(len(max_heap), OBJECT_COUNT)
        self.test_heapq.check_max_invariant(max_heap)

    def test_racing_heapreplace_max(self):
        max_heap = self.create_heap(OBJECT_COUNT, Heap.MAX)
        replace_items = self.create_random_list(-5_000, 10_000, OBJECT_COUNT)

        def heapreplace_max_func(max_heap, replace_items):
            for item in replace_items:
                heapq.heapreplace_max(max_heap, item)

        run_concurrently(
            worker_func=heapreplace_max_func,
            nthreads=NTHREADS,
            args=(max_heap, replace_items),
        )
        self.assertEqual(len(max_heap), OBJECT_COUNT)
        self.test_heapq.check_max_invariant(max_heap)

    def test_lock_free_list_read(self):
        n, n_threads = 1_000, 10
        l = []
        barrier = Barrier(n_threads * 2)

        count = 0
        lock = Lock()

        def worker():
            with lock:
                nonlocal count
                x = count
                count += 1

            barrier.wait()
            for i in range(n):
                if x % 2:
                    heapq.heappush(l, 1)
                    heapq.heappop(l)
                else:
                    try:
                        l[0]
                    except IndexError:
                        pass

        run_concurrently(worker, n_threads * 2)

    @staticmethod
    def is_sorted_ascending(lst):
        """
        Check if the list is sorted in ascending order (non-decreasing).
        """
        return all(lst[i - 1] <= lst[i] for i in range(1, len(lst)))

    @staticmethod
    def is_sorted_descending(lst):
        """
        Check if the list is sorted in descending order (non-increasing).
        """
        return all(lst[i - 1] >= lst[i] for i in range(1, len(lst)))

    @staticmethod
    def create_heap(size, heap_kind):
        """
        Create a min/max heap where elements are in the range (0, size - 1) and
        shuffled before heapify.
        """
        heap = list(range(OBJECT_COUNT))
        shuffle(heap)
        if heap_kind == Heap.MIN:
            heapq.heapify(heap)
        else:
            heapq.heapify_max(heap)

        return heap

    @staticmethod
    def create_random_list(a, b, size):
        """
        Create a list of random numbers between a and b (inclusive).
        """
        return [randint(-a, b) for _ in range(size)]


if __name__ == "__main__":
    unittest.main()
