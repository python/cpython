"""Unittests for heapq."""

from heapq import heappush, heappop, heapify, heapreplace, nlargest, nsmallest
import random
import unittest
from test import test_support


def heapiter(heap):
    # An iterator returning a heap's elements, smallest-first.
    try:
        while 1:
            yield heappop(heap)
    except IndexError:
        pass

class TestHeap(unittest.TestCase):

    def test_push_pop(self):
        # 1) Push 256 random numbers and pop them off, verifying all's OK.
        heap = []
        data = []
        self.check_invariant(heap)
        for i in range(256):
            item = random.random()
            data.append(item)
            heappush(heap, item)
            self.check_invariant(heap)
        results = []
        while heap:
            item = heappop(heap)
            self.check_invariant(heap)
            results.append(item)
        data_sorted = data[:]
        data_sorted.sort()
        self.assertEqual(data_sorted, results)
        # 2) Check that the invariant holds for a sorted array
        self.check_invariant(results)

    def check_invariant(self, heap):
        # Check the heap invariant.
        for pos, item in enumerate(heap):
            if pos: # pos 0 has no parent
                parentpos = (pos-1) >> 1
                self.assert_(heap[parentpos] <= item)

    def test_heapify(self):
        for size in range(30):
            heap = [random.random() for dummy in range(size)]
            heapify(heap)
            self.check_invariant(heap)

    def test_naive_nbest(self):
        data = [random.randrange(2000) for i in range(1000)]
        heap = []
        for item in data:
            heappush(heap, item)
            if len(heap) > 10:
                heappop(heap)
        heap.sort()
        self.assertEqual(heap, sorted(data)[-10:])

    def test_nbest(self):
        # Less-naive "N-best" algorithm, much faster (if len(data) is big
        # enough <wink>) than sorting all of data.  However, if we had a max
        # heap instead of a min heap, it could go faster still via
        # heapify'ing all of data (linear time), then doing 10 heappops
        # (10 log-time steps).
        data = [random.randrange(2000) for i in range(1000)]
        heap = data[:10]
        heapify(heap)
        for item in data[10:]:
            if item > heap[0]:  # this gets rarer the longer we run
                heapreplace(heap, item)
        self.assertEqual(list(heapiter(heap)), sorted(data)[-10:])

    def test_heapsort(self):
        # Exercise everything with repeated heapsort checks
        for trial in xrange(100):
            size = random.randrange(50)
            data = [random.randrange(25) for i in range(size)]
            if trial & 1:     # Half of the time, use heapify
                heap = data[:]
                heapify(heap)
            else:             # The rest of the time, use heappush
                heap = []
                for item in data:
                    heappush(heap, item)
            heap_sorted = [heappop(heap) for i in range(size)]
            self.assertEqual(heap_sorted, sorted(data))

    def test_nsmallest(self):
        data = [random.randrange(2000) for i in range(1000)]
        self.assertEqual(nsmallest(data, 400), sorted(data)[:400])
        self.assertEqual(nsmallest(data, 50), sorted(data)[:50])

    def test_largest(self):
        data = [random.randrange(2000) for i in range(1000)]
        self.assertEqual(nlargest(data, 400), sorted(data, reverse=True)[:400])

def test_main():
    test_support.run_unittest(TestHeap)

if __name__ == "__main__":
    test_main()

