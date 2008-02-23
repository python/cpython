"""Unittests for heapq."""

import random
import unittest
from test import test_support
import sys

# We do a bit of trickery here to be able to test both the C implementation
# and the Python implementation of the module.

# Make it impossible to import the C implementation anymore.
sys.modules['_heapq'] = 0
# We must also handle the case that heapq was imported before.
if 'heapq' in sys.modules:
    del sys.modules['heapq']

# Now we can import the module and get the pure Python implementation.
import heapq as py_heapq

# Restore everything to normal.
del sys.modules['_heapq']
del sys.modules['heapq']

# This is now the module with the C implementation.
import heapq as c_heapq


class TestHeap(unittest.TestCase):
    module = None

    def test_push_pop(self):
        # 1) Push 256 random numbers and pop them off, verifying all's OK.
        heap = []
        data = []
        self.check_invariant(heap)
        for i in range(256):
            item = random.random()
            data.append(item)
            self.module.heappush(heap, item)
            self.check_invariant(heap)
        results = []
        while heap:
            item = self.module.heappop(heap)
            self.check_invariant(heap)
            results.append(item)
        data_sorted = data[:]
        data_sorted.sort()
        self.assertEqual(data_sorted, results)
        # 2) Check that the invariant holds for a sorted array
        self.check_invariant(results)

        self.assertRaises(TypeError, self.module.heappush, [])
        try:
            self.assertRaises(TypeError, self.module.heappush, None, None)
            self.assertRaises(TypeError, self.module.heappop, None)
        except AttributeError:
            pass

    def check_invariant(self, heap):
        # Check the heap invariant.
        for pos, item in enumerate(heap):
            if pos: # pos 0 has no parent
                parentpos = (pos-1) >> 1
                self.assert_(heap[parentpos] <= item)

    def test_heapify(self):
        for size in range(30):
            heap = [random.random() for dummy in range(size)]
            self.module.heapify(heap)
            self.check_invariant(heap)

        self.assertRaises(TypeError, self.module.heapify, None)

    def test_naive_nbest(self):
        data = [random.randrange(2000) for i in range(1000)]
        heap = []
        for item in data:
            self.module.heappush(heap, item)
            if len(heap) > 10:
                self.module.heappop(heap)
        heap.sort()
        self.assertEqual(heap, sorted(data)[-10:])

    def heapiter(self, heap):
        # An iterator returning a heap's elements, smallest-first.
        try:
            while 1:
                yield self.module.heappop(heap)
        except IndexError:
            pass

    def test_nbest(self):
        # Less-naive "N-best" algorithm, much faster (if len(data) is big
        # enough <wink>) than sorting all of data.  However, if we had a max
        # heap instead of a min heap, it could go faster still via
        # heapify'ing all of data (linear time), then doing 10 heappops
        # (10 log-time steps).
        data = [random.randrange(2000) for i in range(1000)]
        heap = data[:10]
        self.module.heapify(heap)
        for item in data[10:]:
            if item > heap[0]:  # this gets rarer the longer we run
                self.module.heapreplace(heap, item)
        self.assertEqual(list(self.heapiter(heap)), sorted(data)[-10:])

        self.assertRaises(TypeError, self.module.heapreplace, None)
        self.assertRaises(TypeError, self.module.heapreplace, None, None)
        self.assertRaises(IndexError, self.module.heapreplace, [], None)

    def test_heapsort(self):
        # Exercise everything with repeated heapsort checks
        for trial in xrange(100):
            size = random.randrange(50)
            data = [random.randrange(25) for i in range(size)]
            if trial & 1:     # Half of the time, use heapify
                heap = data[:]
                self.module.heapify(heap)
            else:             # The rest of the time, use heappush
                heap = []
                for item in data:
                    self.module.heappush(heap, item)
            heap_sorted = [self.module.heappop(heap) for i in range(size)]
            self.assertEqual(heap_sorted, sorted(data))

    def test_merge(self):
        inputs = []
        for i in xrange(random.randrange(5)):
            row = sorted(random.randrange(1000) for j in range(random.randrange(10)))
            inputs.append(row)
        self.assertEqual(sorted(chain(*inputs)), list(self.module.merge(*inputs)))
        self.assertEqual(list(self.module.merge()), [])

    def test_merge_stability(self):
        class Int(int):
            pass
        inputs = [[], [], [], []]
        for i in range(20000):
            stream = random.randrange(4)
            x = random.randrange(500)
            obj = Int(x)
            obj.pair = (x, stream)
            inputs[stream].append(obj)
        for stream in inputs:
            stream.sort()
        result = [i.pair for i in self.module.merge(*inputs)]
        self.assertEqual(result, sorted(result))

    def test_nsmallest(self):
        data = [(random.randrange(2000), i) for i in range(1000)]
        for f in (None, lambda x:  x[0] * 547 % 2000):
            for n in (0, 1, 2, 10, 100, 400, 999, 1000, 1100):
                self.assertEqual(self.module.nsmallest(n, data), sorted(data)[:n])
                self.assertEqual(self.module.nsmallest(n, data, key=f),
                                 sorted(data, key=f)[:n])

    def test_nlargest(self):
        data = [(random.randrange(2000), i) for i in range(1000)]
        for f in (None, lambda x:  x[0] * 547 % 2000):
            for n in (0, 1, 2, 10, 100, 400, 999, 1000, 1100):
                self.assertEqual(self.module.nlargest(n, data),
                                 sorted(data, reverse=True)[:n])
                self.assertEqual(self.module.nlargest(n, data, key=f),
                                 sorted(data, key=f, reverse=True)[:n])

class TestHeapPython(TestHeap):
    module = py_heapq

class TestHeapC(TestHeap):
    module = c_heapq


#==============================================================================

class LenOnly:
    "Dummy sequence class defining __len__ but not __getitem__."
    def __len__(self):
        return 10

class GetOnly:
    "Dummy sequence class defining __getitem__ but not __len__."
    def __getitem__(self, ndx):
        return 10

class CmpErr:
    "Dummy element that always raises an error during comparison"
    def __cmp__(self, other):
        raise ZeroDivisionError

def R(seqn):
    'Regular generator'
    for i in seqn:
        yield i

class G:
    'Sequence using __getitem__'
    def __init__(self, seqn):
        self.seqn = seqn
    def __getitem__(self, i):
        return self.seqn[i]

class I:
    'Sequence using iterator protocol'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def next(self):
        if self.i >= len(self.seqn): raise StopIteration
        v = self.seqn[self.i]
        self.i += 1
        return v

class Ig:
    'Sequence using iterator protocol defined with a generator'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        for val in self.seqn:
            yield val

class X:
    'Missing __getitem__ and __iter__'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def next(self):
        if self.i >= len(self.seqn): raise StopIteration
        v = self.seqn[self.i]
        self.i += 1
        return v

class N:
    'Iterator missing next()'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self

class E:
    'Test propagation of exceptions'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def next(self):
        3 // 0

class S:
    'Test immediate stop'
    def __init__(self, seqn):
        pass
    def __iter__(self):
        return self
    def next(self):
        raise StopIteration

from itertools import chain, imap
def L(seqn):
    'Test multiple tiers of iterators'
    return chain(imap(lambda x:x, R(Ig(G(seqn)))))

class TestErrorHandling(unittest.TestCase):
    # only for C implementation
    module = c_heapq

    def test_non_sequence(self):
        for f in (self.module.heapify, self.module.heappop):
            self.assertRaises(TypeError, f, 10)
        for f in (self.module.heappush, self.module.heapreplace,
                  self.module.nlargest, self.module.nsmallest):
            self.assertRaises(TypeError, f, 10, 10)

    def test_len_only(self):
        for f in (self.module.heapify, self.module.heappop):
            self.assertRaises(TypeError, f, LenOnly())
        for f in (self.module.heappush, self.module.heapreplace):
            self.assertRaises(TypeError, f, LenOnly(), 10)
        for f in (self.module.nlargest, self.module.nsmallest):
            self.assertRaises(TypeError, f, 2, LenOnly())

    def test_get_only(self):
        for f in (self.module.heapify, self.module.heappop):
            self.assertRaises(TypeError, f, GetOnly())
        for f in (self.module.heappush, self.module.heapreplace):
            self.assertRaises(TypeError, f, GetOnly(), 10)
        for f in (self.module.nlargest, self.module.nsmallest):
            self.assertRaises(TypeError, f, 2, GetOnly())

    def test_get_only(self):
        seq = [CmpErr(), CmpErr(), CmpErr()]
        for f in (self.module.heapify, self.module.heappop):
            self.assertRaises(ZeroDivisionError, f, seq)
        for f in (self.module.heappush, self.module.heapreplace):
            self.assertRaises(ZeroDivisionError, f, seq, 10)
        for f in (self.module.nlargest, self.module.nsmallest):
            self.assertRaises(ZeroDivisionError, f, 2, seq)

    def test_arg_parsing(self):
        for f in (self.module.heapify, self.module.heappop,
                  self.module.heappush, self.module.heapreplace,
                  self.module.nlargest, self.module.nsmallest):
            self.assertRaises(TypeError, f, 10)

    def test_iterable_args(self):
        for f in (self.module.nlargest, self.module.nsmallest):
            for s in ("123", "", range(1000), ('do', 1.2), xrange(2000,2200,5)):
                for g in (G, I, Ig, L, R):
                    self.assertEqual(f(2, g(s)), f(2,s))
                self.assertEqual(f(2, S(s)), [])
                self.assertRaises(TypeError, f, 2, X(s))
                self.assertRaises(TypeError, f, 2, N(s))
                self.assertRaises(ZeroDivisionError, f, 2, E(s))


#==============================================================================


def test_main(verbose=None):
    from types import BuiltinFunctionType

    test_classes = [TestHeapPython, TestHeapC, TestErrorHandling]
    test_support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

if __name__ == "__main__":
    test_main(verbose=True)
