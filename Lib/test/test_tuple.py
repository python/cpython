from test import support, seq_tests
import unittest

import gc
import pickle

class TupleTest(seq_tests.CommonTest):
    type2test = tuple

    def test_getitem_error(self):
        msg = "tuple indices must be integers or slices"
        with self.assertRaisesRegex(TypeError, msg):
            ()['a']

    def test_constructors(self):
        super().test_constructors()
        # calling built-in types without argument must return empty
        self.assertEqual(tuple(), ())
        t0_3 = (0, 1, 2, 3)
        t0_3_bis = tuple(t0_3)
        self.assertTrue(t0_3 is t0_3_bis)
        self.assertEqual(tuple([]), ())
        self.assertEqual(tuple([0, 1, 2, 3]), (0, 1, 2, 3))
        self.assertEqual(tuple(''), ())
        self.assertEqual(tuple('spam'), ('s', 'p', 'a', 'm'))
        self.assertEqual(tuple(x for x in range(10) if x % 2),
                         (1, 3, 5, 7, 9))

    def test_keyword_args(self):
        with self.assertRaisesRegex(TypeError, 'keyword argument'):
            tuple(sequence=())

    def test_truth(self):
        super().test_truth()
        self.assertTrue(not ())
        self.assertTrue((42, ))

    def test_len(self):
        super().test_len()
        self.assertEqual(len(()), 0)
        self.assertEqual(len((0,)), 1)
        self.assertEqual(len((0, 1, 2)), 3)

    def test_iadd(self):
        super().test_iadd()
        u = (0, 1)
        u2 = u
        u += (2, 3)
        self.assertTrue(u is not u2)

    def test_imul(self):
        super().test_imul()
        u = (0, 1)
        u2 = u
        u *= 3
        self.assertTrue(u is not u2)

    def test_tupleresizebug(self):
        # Check that a specific bug in _PyTuple_Resize() is squashed.
        def f():
            for i in range(1000):
                yield i
        self.assertEqual(list(tuple(f())), list(range(1000)))

    # Various tests for hashing of tuples to check that we get few collisions.
    #
    # Earlier versions of the tuple hash algorithm had collisions
    # reported at:
    # - https://bugs.python.org/issue942952
    # - https://bugs.python.org/issue34751
    #
    # Notes:
    # - The hash of tuples is deterministic: if the test passes once on a given
    #   system, it will always pass. So the probabilities mentioned in the
    #   test_hash functions below should be interpreted assuming that the
    #   hashes are random.
    # - Due to the structure in the testsuite inputs, collisions are not
    #   independent. For example, if hash((a,b)) == hash((c,d)), then also
    #   hash((a,b,x)) == hash((c,d,x)). But the quoted probabilities assume
    #   independence anyway.
    # - We limit the hash to 32 bits in the tests to have a good test on
    #   64-bit systems too. Furthermore, this is also a sanity check that the
    #   lower 32 bits of a 64-bit hash are sufficiently random too.
    def test_hash1(self):
        # Check for hash collisions between small integers in range(50) and
        # certain tuples and nested tuples of such integers.
        N=50
        base = list(range(N))
        xp = [(i, j) for i in base for j in base]
        inps = base + [(i, j) for i in base for j in xp] + \
                     [(i, j) for i in xp for j in base] + xp + list(zip(base))
        self.assertEqual(len(inps), 252600)
        hashes = set(hash(x) % 2**32 for x in inps)
        collisions = len(inps) - len(hashes)

        # For a pure random 32-bit hash and N = 252,600 test items, the
        # expected number of collisions equals
        #
        # 2**(-32) * N(N-1)/2 = 7.4
        #
        # We allow up to 15 collisions, which suffices to make the test
        # pass with 99.5% confidence.
        self.assertLessEqual(collisions, 15)

    def test_hash2(self):
        # Check for hash collisions between small integers (positive and
        # negative), tuples and nested tuples of such integers.

        # All numbers in the interval [-n, ..., n] except -1 because
        # hash(-1) == hash(-2).
        n = 5
        A = [x for x in range(-n, n+1) if x != -1]

        B = A + [(a,) for a in A]

        L2 = [(a,b) for a in A for b in A]
        L3 = L2 + [(a,b,c) for a in A for b in A for c in A]
        L4 = L3 + [(a,b,c,d) for a in A for b in A for c in A for d in A]

        # T = list of testcases. These consist of all (possibly nested
        # at most 2 levels deep) tuples containing at most 4 items from
        # the set A.
        T = A
        T += [(a,) for a in B + L4]
        T += [(a,b) for a in L3 for b in B]
        T += [(a,b) for a in L2 for b in L2]
        T += [(a,b) for a in B for b in L3]
        T += [(a,b,c) for a in B for b in B for c in L2]
        T += [(a,b,c) for a in B for b in L2 for c in B]
        T += [(a,b,c) for a in L2 for b in B for c in B]
        T += [(a,b,c,d) for a in B for b in B for c in B for d in B]
        self.assertEqual(len(T), 345130)
        hashes = set(hash(x) % 2**32 for x in T)
        collisions = len(T) - len(hashes)

        # For a pure random 32-bit hash and N = 345,130 test items, the
        # expected number of collisions equals
        #
        # 2**(-32) * N(N-1)/2 = 13.9
        #
        # We allow up to 20 collisions, which suffices to make the test
        # pass with 95.5% confidence.
        self.assertLessEqual(collisions, 20)

    def test_hash3(self):
        # Check for hash collisions between tuples containing 0.0 and 0.5.
        # The hashes of 0.0 and 0.5 itself differ only in one high bit.
        # So this implicitly tests propagation of high bits to low bits.
        from itertools import product
        T = list(product([0.0, 0.5], repeat=18))
        self.assertEqual(len(T), 262144)
        hashes = set(hash(x) % 2**32 for x in T)
        collisions = len(T) - len(hashes)

        # For a pure random 32-bit hash and N = 262,144 test items, the
        # expected number of collisions equals
        #
        # 2**(-32) * N(N-1)/2 = 8.0
        #
        # We allow up to 15 collisions, which suffices to make the test
        # pass with 99.1% confidence.
        self.assertLessEqual(collisions, 15)

    def test_repr(self):
        l0 = tuple()
        l2 = (0, 1, 2)
        a0 = self.type2test(l0)
        a2 = self.type2test(l2)

        self.assertEqual(str(a0), repr(l0))
        self.assertEqual(str(a2), repr(l2))
        self.assertEqual(repr(a0), "()")
        self.assertEqual(repr(a2), "(0, 1, 2)")

    def _not_tracked(self, t):
        # Nested tuples can take several collections to untrack
        gc.collect()
        gc.collect()
        self.assertFalse(gc.is_tracked(t), t)

    def _tracked(self, t):
        self.assertTrue(gc.is_tracked(t), t)
        gc.collect()
        gc.collect()
        self.assertTrue(gc.is_tracked(t), t)

    @support.cpython_only
    def test_track_literals(self):
        # Test GC-optimization of tuple literals
        x, y, z = 1.5, "a", []

        self._not_tracked(())
        self._not_tracked((1,))
        self._not_tracked((1, 2))
        self._not_tracked((1, 2, "a"))
        self._not_tracked((1, 2, (None, True, False, ()), int))
        self._not_tracked((object(),))
        self._not_tracked(((1, x), y, (2, 3)))

        # Tuples with mutable elements are always tracked, even if those
        # elements are not tracked right now.
        self._tracked(([],))
        self._tracked(([1],))
        self._tracked(({},))
        self._tracked((set(),))
        self._tracked((x, y, z))

    def check_track_dynamic(self, tp, always_track):
        x, y, z = 1.5, "a", []

        check = self._tracked if always_track else self._not_tracked
        check(tp())
        check(tp([]))
        check(tp(set()))
        check(tp([1, x, y]))
        check(tp(obj for obj in [1, x, y]))
        check(tp(set([1, x, y])))
        check(tp(tuple([obj]) for obj in [1, x, y]))
        check(tuple(tp([obj]) for obj in [1, x, y]))

        self._tracked(tp([z]))
        self._tracked(tp([[x, y]]))
        self._tracked(tp([{x: y}]))
        self._tracked(tp(obj for obj in [x, y, z]))
        self._tracked(tp(tuple([obj]) for obj in [x, y, z]))
        self._tracked(tuple(tp([obj]) for obj in [x, y, z]))

    @support.cpython_only
    def test_track_dynamic(self):
        # Test GC-optimization of dynamically constructed tuples.
        self.check_track_dynamic(tuple, False)

    @support.cpython_only
    def test_track_subtypes(self):
        # Tuple subtypes must always be tracked
        class MyTuple(tuple):
            pass
        self.check_track_dynamic(MyTuple, True)

    @support.cpython_only
    def test_bug7466(self):
        # Trying to untrack an unfinished tuple could crash Python
        self._not_tracked(tuple(gc.collect() for i in range(101)))

    def test_repr_large(self):
        # Check the repr of large list objects
        def check(n):
            l = (0,) * n
            s = repr(l)
            self.assertEqual(s,
                '(' + ', '.join(['0'] * n) + ')')
        check(10)       # check our checking code
        check(1000000)

    def test_iterator_pickle(self):
        # Userlist iterators don't support pickling yet since
        # they are based on generators.
        data = self.type2test([4, 5, 6, 7])
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            itorg = iter(data)
            d = pickle.dumps(itorg, proto)
            it = pickle.loads(d)
            self.assertEqual(type(itorg), type(it))
            self.assertEqual(self.type2test(it), self.type2test(data))

            it = pickle.loads(d)
            next(it)
            d = pickle.dumps(it, proto)
            self.assertEqual(self.type2test(it), self.type2test(data)[1:])

    def test_reversed_pickle(self):
        data = self.type2test([4, 5, 6, 7])
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            itorg = reversed(data)
            d = pickle.dumps(itorg, proto)
            it = pickle.loads(d)
            self.assertEqual(type(itorg), type(it))
            self.assertEqual(self.type2test(it), self.type2test(reversed(data)))

            it = pickle.loads(d)
            next(it)
            d = pickle.dumps(it, proto)
            self.assertEqual(self.type2test(it), self.type2test(reversed(data))[1:])

    def test_no_comdat_folding(self):
        # Issue 8847: In the PGO build, the MSVC linker's COMDAT folding
        # optimization causes failures in code that relies on distinct
        # function addresses.
        class T(tuple): pass
        with self.assertRaises(TypeError):
            [3,] + T((1,2))

    def test_lexicographic_ordering(self):
        # Issue 21100
        a = self.type2test([1, 2])
        b = self.type2test([1, 2, 0])
        c = self.type2test([1, 3])
        self.assertLess(a, b)
        self.assertLess(b, c)

if __name__ == "__main__":
    unittest.main()
