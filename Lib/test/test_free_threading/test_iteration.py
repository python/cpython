import threading
import unittest
from test import support

# The race conditions these tests were written for only happen every now and
# then, even with the current numbers. To find rare race conditions, bumping
# these up will help, but it makes the test runtime highly variable under
# free-threading. Overhead is much higher under ThreadSanitizer, but it's
# also much better at detecting certain races, so we don't need as many
# items/threads.
if support.check_sanitizer(thread=True):
    NUMITEMS = 1000
    NUMTHREADS = 2
else:
    NUMITEMS = 100000
    NUMTHREADS = 5
NUMMUTATORS = 2

class ContendedTupleIterationTest(unittest.TestCase):
    def make_testdata(self, n):
        return tuple(range(n))

    def assert_iterator_results(self, results, expected):
        # Most iterators are not atomic (yet?) so they can skip or duplicate
        # items, but they should not invent new items (like the range
        # iterator currently does).
        extra_items = set(results) - set(expected)
        self.assertEqual(set(), extra_items)

    def run_threads(self, func, *args, numthreads=NUMTHREADS):
        threads = []
        for _ in range(numthreads):
            t = threading.Thread(target=func, args=args)
            t.start()
            threads.append(t)
        return threads

    def test_iteration(self):
        """Test iteration over a shared container"""
        seq = self.make_testdata(NUMITEMS)
        results = []
        start = threading.Barrier(NUMTHREADS)
        def worker():
            idx = 0
            start.wait()
            for item in seq:
                idx += 1
            results.append(idx)
        threads = self.run_threads(worker)
        for t in threads:
            t.join()
        # Each thread has its own iterator, so results should be entirely predictable.
        self.assertEqual(results, [NUMITEMS] * NUMTHREADS)

    def test_shared_iterator(self):
        """Test iteration over a shared iterator"""
        seq = self.make_testdata(NUMITEMS)
        it = iter(seq)
        results = []
        start = threading.Barrier(NUMTHREADS)
        def worker():
            items = []
            start.wait()
            # We want a tight loop, so put items in the shared list at the end.
            for item in it:
                items.append(item)
            results.extend(items)
        threads = self.run_threads(worker)
        for t in threads:
            t.join()
        self.assert_iterator_results(results, seq)

class ContendedListIterationTest(ContendedTupleIterationTest):
    def make_testdata(self, n):
        return list(range(n))

    def test_iteration_while_mutating(self):
        """Test iteration over a shared mutating container."""
        seq = self.make_testdata(NUMITEMS)
        results = []
        start = threading.Barrier(NUMTHREADS + NUMMUTATORS)
        endmutate = threading.Event()
        def mutator():
            orig = seq[:]
            # Make changes big enough to cause resizing of the list, with
            # items shifted around for good measure.
            replacement = (orig * 3)[NUMITEMS//2:]
            start.wait()
            while not endmutate.is_set():
                seq.extend(replacement)
                seq[:0] = orig
                seq.__imul__(2)
                seq.extend(seq)
                seq[:] = orig
        def worker():
            items = []
            start.wait()
            # We want a tight loop, so put items in the shared list at the end.
            for item in seq:
                items.append(item)
            results.extend(items)
        mutators = ()
        try:
            threads = self.run_threads(worker)
            mutators = self.run_threads(mutator, numthreads=NUMMUTATORS)
            for t in threads:
                t.join()
        finally:
            endmutate.set()
            for m in mutators:
                m.join()
        self.assert_iterator_results(results, list(seq))


class ContendedRangeIterationTest(ContendedTupleIterationTest):
    def make_testdata(self, n):
        return range(n)

    def assert_iterator_results(self, results, expected):
        # Range iterators that are shared between threads will (right now)
        # sometimes produce items after the end of the range, sometimes
        # _far_ after the end of the range. That should be fixed, but for
        # now, let's just check they're integers that could have resulted
        # from stepping beyond the range bounds.
        extra_items = set(results) - set(expected)
        for item in extra_items:
            self.assertEqual((item - expected.start) % expected.step, 0)
