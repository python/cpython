"""Unittests for heapq."""

from test.test_support import verify, vereq, verbose, TestFailed

from heapq import heappush, heappop
import random

def check_invariant(heap):
    # Check the heap invariant.
    for pos, item in enumerate(heap):
        if pos: # pos 0 has no parent
            parentpos = (pos-1) >> 1
            verify(heap[parentpos] <= item)

def test_main():
    # 1) Push 100 random numbers and pop them off, verifying all's OK.
    heap = []
    data = []
    check_invariant(heap)
    for i in range(256):
        item = random.random()
        data.append(item)
        heappush(heap, item)
        check_invariant(heap)
    results = []
    while heap:
        item = heappop(heap)
        check_invariant(heap)
        results.append(item)
    data_sorted = data[:]
    data_sorted.sort()
    vereq(data_sorted, results)
    # 2) Check that the invariant holds for a sorted array
    check_invariant(results)
    # 3) Naive "N-best" algorithm
    heap = []
    for item in data:
        heappush(heap, item)
        if len(heap) > 10:
            heappop(heap)
    heap.sort()
    vereq(heap, data_sorted[-10:])
    # Make user happy
    if verbose:
        print "All OK"

if __name__ == "__main__":
    test_main()
