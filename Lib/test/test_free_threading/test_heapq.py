import unittest

import heapq
import operator

from enum import Enum
from threading import Thread, Barrier
from random import shuffle, randint

from test.support import threading_helper


NTHREADS: int = 10
OBJECT_COUNT: int = 5_000


class HeapKind(Enum):
    MIN = 1
    MAX = 2


@threading_helper.requires_working_threading()
class TestHeapq(unittest.TestCase):
    def test_racing_heapify(self):
        heap = list(range(OBJECT_COUNT))
        shuffle(heap)

        def heapify_func(heap: list[int]):
            heapq.heapify(heap)

        self.run_concurrently(
            worker_func=heapify_func, args=(heap,), nthreads=NTHREADS
        )
        self.assertTrue(self.is_min_heap_property_satisfied(heap))

    def test_racing_heappush(self):
        heap = []

        def heappush_func(heap: list[int]):
            for item in reversed(range(OBJECT_COUNT)):
                heapq.heappush(heap, item)

        self.run_concurrently(
            worker_func=heappush_func, args=(heap,), nthreads=NTHREADS
        )
        self.assertTrue(self.is_min_heap_property_satisfied(heap))

    def test_racing_heappop(self):
        heap = list(range(OBJECT_COUNT))
        shuffle(heap)
        heapq.heapify(heap)

        # Each thread pops (OBJECT_COUNT / NTHREADS) items
        self.assertEqual(0, OBJECT_COUNT % NTHREADS)
        per_thread_pop_count = OBJECT_COUNT // NTHREADS

        def heappop_func(heap: list[int], pop_count: int):
            local_list = []
            for _ in range(pop_count):
                item = heapq.heappop(heap)
                local_list.append(item)

            # Each local list should be sorted
            self.assertTrue(self.is_sorted_ascending(local_list))

        self.run_concurrently(
            worker_func=heappop_func,
            args=(heap, per_thread_pop_count),
            nthreads=NTHREADS,
        )
        self.assertEqual(0, len(heap))

    def test_racing_heappushpop(self):
        heap = list(range(OBJECT_COUNT))
        shuffle(heap)
        heapq.heapify(heap)

        pushpop_items = [
            randint(-OBJECT_COUNT, OBJECT_COUNT) for _ in range(OBJECT_COUNT)
        ]

        def heappushpop_func(heap: list[int], pushpop_items: list[int]):
            for item in pushpop_items:
                popped_item = heapq.heappushpop(heap, item)
                self.assertTrue(popped_item <= item)

        self.run_concurrently(
            worker_func=heappushpop_func,
            args=(heap, pushpop_items),
            nthreads=NTHREADS,
        )
        self.assertEqual(OBJECT_COUNT, len(heap))
        self.assertTrue(self.is_min_heap_property_satisfied(heap))

    def test_racing_heapreplace(self):
        heap = list(range(OBJECT_COUNT))
        shuffle(heap)
        heapq.heapify(heap)

        replace_items = [
            randint(-OBJECT_COUNT, OBJECT_COUNT) for _ in range(OBJECT_COUNT)
        ]

        def heapreplace_func(heap: list[int], replace_items: list[int]):
            for item in replace_items:
                popped_item = heapq.heapreplace(heap, item)

        self.run_concurrently(
            worker_func=heapreplace_func,
            args=(heap, replace_items),
            nthreads=NTHREADS,
        )
        self.assertEqual(OBJECT_COUNT, len(heap))
        self.assertTrue(self.is_min_heap_property_satisfied(heap))

    def test_racing_heapify_max(self):
        max_heap = list(range(OBJECT_COUNT))
        shuffle(max_heap)

        def heapify_max_func(max_heap: list[int]):
            heapq.heapify_max(max_heap)

        self.run_concurrently(
            worker_func=heapify_max_func, args=(max_heap,), nthreads=NTHREADS
        )
        self.assertTrue(self.is_max_heap_property_satisfied(max_heap))

    def test_racing_heappush_max(self):
        max_heap = []

        def heappush_max_func(max_heap: list[int]):
            for item in range(OBJECT_COUNT):
                heapq.heappush_max(max_heap, item)

        self.run_concurrently(
            worker_func=heappush_max_func, args=(max_heap,), nthreads=NTHREADS
        )
        self.assertTrue(self.is_max_heap_property_satisfied(max_heap))

    def test_racing_heappop_max(self):
        max_heap = list(range(OBJECT_COUNT))
        shuffle(max_heap)
        heapq.heapify_max(max_heap)

        # Each thread pops (OBJECT_COUNT / NTHREADS) items
        self.assertEqual(0, OBJECT_COUNT % NTHREADS)
        per_thread_pop_count = OBJECT_COUNT // NTHREADS

        def heappop_max_func(max_heap: list[int], pop_count: int):
            local_list = []
            for _ in range(pop_count):
                item = heapq.heappop_max(max_heap)
                local_list.append(item)

            # Each local list should be sorted
            self.assertTrue(self.is_sorted_descending(local_list))

        self.run_concurrently(
            worker_func=heappop_max_func,
            args=(max_heap, per_thread_pop_count),
            nthreads=NTHREADS,
        )
        self.assertEqual(0, len(max_heap))

    def test_racing_heappushpop_max(self):
        max_heap = list(range(OBJECT_COUNT))
        shuffle(max_heap)
        heapq.heapify_max(max_heap)

        pushpop_items = [
            randint(-OBJECT_COUNT, OBJECT_COUNT) for _ in range(OBJECT_COUNT)
        ]

        def heappushpop_max_func(
            max_heap: list[int], pushpop_items: list[int]
        ):
            for item in pushpop_items:
                popped_item = heapq.heappushpop_max(max_heap, item)
                self.assertTrue(popped_item >= item)

        self.run_concurrently(
            worker_func=heappushpop_max_func,
            args=(max_heap, pushpop_items),
            nthreads=NTHREADS,
        )
        self.assertEqual(OBJECT_COUNT, len(max_heap))
        self.assertTrue(self.is_max_heap_property_satisfied(max_heap))

    def test_racing_heapreplace_max(self):
        max_heap = list(range(OBJECT_COUNT))
        shuffle(max_heap)
        heapq.heapify_max(max_heap)

        replace_items = [
            randint(-OBJECT_COUNT, OBJECT_COUNT) for _ in range(OBJECT_COUNT)
        ]

        def heapreplace_max_func(
            max_heap: list[int], replace_items: list[int]
        ):
            for item in replace_items:
                popped_item = heapq.heapreplace_max(max_heap, item)

        self.run_concurrently(
            worker_func=heapreplace_max_func,
            args=(max_heap, replace_items),
            nthreads=NTHREADS,
        )
        self.assertEqual(OBJECT_COUNT, len(max_heap))
        self.assertTrue(self.is_max_heap_property_satisfied(max_heap))

    def is_min_heap_property_satisfied(self, heap: list[object]) -> bool:
        """
        The value of a parent node should be less than or equal to the
        values of its children.
        """
        return self.is_heap_property_satisfied(heap, HeapKind.MIN)

    def is_max_heap_property_satisfied(self, heap: list[object]) -> bool:
        """
        The value of a parent node should be greater than or equal to the
        values of its children.
        """
        return self.is_heap_property_satisfied(heap, HeapKind.MAX)

    @staticmethod
    def is_heap_property_satisfied(
        heap: list[object], heap_kind: HeapKind
    ) -> bool:
        """
        Check if the heap property is satisfied.
        """
        op = operator.le if heap_kind == HeapKind.MIN else operator.ge
        # position 0 has no parent
        for pos in range(1, len(heap)):
            parent_pos = (pos - 1) >> 1
            if not op(heap[parent_pos], heap[pos]):
                return False

        return True

    @staticmethod
    def is_sorted_ascending(lst: list[object]) -> bool:
        """
        Check if the list is sorted in ascending order (non-decreasing).
        """
        return all(lst[i - 1] <= lst[i] for i in range(1, len(lst)))

    @staticmethod
    def is_sorted_descending(lst: list[object]) -> bool:
        """
        Check if the list is sorted in descending order (non-increasing).
        """
        return all(lst[i - 1] >= lst[i] for i in range(1, len(lst)))

    @staticmethod
    def run_concurrently(worker_func, args, nthreads) -> None:
        """
        Run the worker function concurrently in multiple threads.
        """
        barrier = Barrier(NTHREADS)

        def wrapper_func(*args):
            # Wait for all threadss to reach this point before proceeding.
            barrier.wait()
            worker_func(*args)

        workers = []
        for _ in range(nthreads):
            worker = Thread(target=wrapper_func, args=args)
            workers.append(worker)
            worker.start()

        for worker in workers:
            worker.join()


if __name__ == "__main__":
    unittest.main()
