"""Unittests for heapq."""

from test.test_support import verify, vereq, verbose, TestFailed

from heapq import heappush, heappop, heapify
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
    # 4) Test heapify.
    for size in range(30):
        heap = [random.random() for dummy in range(size)]
        heapify(heap)
        check_invariant(heap)
    # 5) Less-naive "N-best" algorithm, much faster (if len(data) is big
    #    enough <wink>) than sorting all of data.  However, if we had a max
    #    heap instead of a min heap, it would go much faster still via
    #    heapify'ing all of data (linear time), then doing 10 heappops
    #    (10 log-time steps).
    heap = data[:10]
    heapify(heap)
    for item in data[10:]:
        if item > heap[0]:  # this gets rarer and rarer the longer we run
            heappush(heap, item)
            heappop(heap)
    heap.sort()
    vereq(heap, data_sorted[-10:])
    # Make user happy
    if verbose:
        print "All OK"

if __name__ == "__main__":
    test_main()
