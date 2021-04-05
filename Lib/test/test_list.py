import sys
from test import list_tests
from test.support import cpython_only
import pickle
import struct
import unittest

class ListTest(list_tests.CommonTest):
    type2test = list

    def test_basic(self):
        self.assertEqual(list([]), [])
        l0_3 = [0, 1, 2, 3]
        l0_3_bis = list(l0_3)
        self.assertEqual(l0_3, l0_3_bis)
        self.assertTrue(l0_3 is not l0_3_bis)
        self.assertEqual(list(()), [])
        self.assertEqual(list((0, 1, 2, 3)), [0, 1, 2, 3])
        self.assertEqual(list(''), [])
        self.assertEqual(list('spam'), ['s', 'p', 'a', 'm'])
        self.assertEqual(list(x for x in range(10) if x % 2),
                         [1, 3, 5, 7, 9])

        if sys.maxsize == 0x7fffffff:
            # This test can currently only work on 32-bit machines.
            # XXX If/when PySequence_Length() returns a ssize_t, it should be
            # XXX re-enabled.
            # Verify clearing of bug #556025.
            # This assumes that the max data size (sys.maxint) == max
            # address size this also assumes that the address size is at
            # least 4 bytes with 8 byte addresses, the bug is not well
            # tested
            #
            # Note: This test is expected to SEGV under Cygwin 1.3.12 or
            # earlier due to a newlib bug.  See the following mailing list
            # thread for the details:

            #     http://sources.redhat.com/ml/newlib/2002/msg00369.html
            self.assertRaises(MemoryError, list, range(sys.maxsize // 2))

        # This code used to segfault in Py2.4a3
        x = []
        x.extend(-y for y in x)
        self.assertEqual(x, [])

    def test_keyword_args(self):
        with self.assertRaisesRegex(TypeError, 'keyword argument'):
            list(sequence=[])

    def test_truth(self):
        super().test_truth()
        self.assertTrue(not [])
        self.assertTrue([42])

    def test_identity(self):
        self.assertTrue([] is not [])

    def test_len(self):
        super().test_len()
        self.assertEqual(len([]), 0)
        self.assertEqual(len([0]), 1)
        self.assertEqual(len([0, 1, 2]), 3)

    def test_overflow(self):
        lst = [4, 5, 6, 7]
        n = int((sys.maxsize*2+2) // len(lst))
        def mul(a, b): return a * b
        def imul(a, b): a *= b
        self.assertRaises((MemoryError, OverflowError), mul, lst, n)
        self.assertRaises((MemoryError, OverflowError), imul, lst, n)

    def test_repr_large(self):
        # Check the repr of large list objects
        def check(n):
            l = [0] * n
            s = repr(l)
            self.assertEqual(s,
                '[' + ', '.join(['0'] * n) + ']')
        check(10)       # check our checking code
        check(1000000)

    def test_iterator_pickle(self):
        orig = self.type2test([4, 5, 6, 7])
        data = [10, 11, 12, 13, 14, 15]
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            # initial iterator
            itorig = iter(orig)
            d = pickle.dumps((itorig, orig), proto)
            it, a = pickle.loads(d)
            a[:] = data
            self.assertEqual(type(it), type(itorig))
            self.assertEqual(list(it), data)

            # running iterator
            next(itorig)
            d = pickle.dumps((itorig, orig), proto)
            it, a = pickle.loads(d)
            a[:] = data
            self.assertEqual(type(it), type(itorig))
            self.assertEqual(list(it), data[1:])

            # empty iterator
            for i in range(1, len(orig)):
                next(itorig)
            d = pickle.dumps((itorig, orig), proto)
            it, a = pickle.loads(d)
            a[:] = data
            self.assertEqual(type(it), type(itorig))
            self.assertEqual(list(it), data[len(orig):])

            # exhausted iterator
            self.assertRaises(StopIteration, next, itorig)
            d = pickle.dumps((itorig, orig), proto)
            it, a = pickle.loads(d)
            a[:] = data
            self.assertEqual(list(it), [])

    def test_reversed_pickle(self):
        orig = self.type2test([4, 5, 6, 7])
        data = [10, 11, 12, 13, 14, 15]
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            # initial iterator
            itorig = reversed(orig)
            d = pickle.dumps((itorig, orig), proto)
            it, a = pickle.loads(d)
            a[:] = data
            self.assertEqual(type(it), type(itorig))
            self.assertEqual(list(it), data[len(orig)-1::-1])

            # running iterator
            next(itorig)
            d = pickle.dumps((itorig, orig), proto)
            it, a = pickle.loads(d)
            a[:] = data
            self.assertEqual(type(it), type(itorig))
            self.assertEqual(list(it), data[len(orig)-2::-1])

            # empty iterator
            for i in range(1, len(orig)):
                next(itorig)
            d = pickle.dumps((itorig, orig), proto)
            it, a = pickle.loads(d)
            a[:] = data
            self.assertEqual(type(it), type(itorig))
            self.assertEqual(list(it), [])

            # exhausted iterator
            self.assertRaises(StopIteration, next, itorig)
            d = pickle.dumps((itorig, orig), proto)
            it, a = pickle.loads(d)
            a[:] = data
            self.assertEqual(list(it), [])

    def test_step_overflow(self):
        a = [0, 1, 2, 3, 4]
        a[1::sys.maxsize] = [0]
        self.assertEqual(a[3::sys.maxsize], [3])

    def test_no_comdat_folding(self):
        # Issue 8847: In the PGO build, the MSVC linker's COMDAT folding
        # optimization causes failures in code that relies on distinct
        # function addresses.
        class L(list): pass
        with self.assertRaises(TypeError):
            (3,) + L([1,2])

    def test_equal_operator_modifying_operand(self):
        # test fix for seg fault reported in bpo-38588 part 2.
        class X:
            def __eq__(self,other) :
                list2.clear()
                return NotImplemented

        class Y:
            def __eq__(self, other):
                list1.clear()
                return NotImplemented

        class Z:
            def __eq__(self, other):
                list3.clear()
                return NotImplemented

        list1 = [X()]
        list2 = [Y()]
        self.assertTrue(list1 == list2)

        list3 = [Z()]
        list4 = [1]
        self.assertFalse(list3 == list4)

    @cpython_only
    def test_preallocation(self):
        iterable = [0] * 10
        iter_size = sys.getsizeof(iterable)

        self.assertEqual(iter_size, sys.getsizeof(list([0] * 10)))
        self.assertEqual(iter_size, sys.getsizeof(list(range(10))))

    @cpython_only
    def test_overallocation(self):
        # bpo-33234: Don't overallocate when initialized from known lengths
        # bpo-38373: Allows list over-allocation to be zero for some lengths
        # bpo-43574: Don't overallocate for list-literals
        sizeof = sys.getsizeof

        # First handle empty list and empty list-literal cases.  Should have no
        # overallocation, including init from iterable of unknown length.
        self.assertEqual(sizeof([]), sizeof(list()))
        self.assertEqual(sizeof([]), sizeof(list(tuple())))
        self.assertEqual(sizeof([]), sizeof(list(x for x in [])))

        # Must use actual list-literals to test the overallocation behavior of
        # compiled list-literals as well as those initialized from them.
        test_literals = [
                [1],
                [1,2],
                [1,2,3],  # Literals of length > 2 are special-cased in compile
                [1,2,3,4],
                [1,2,3,4,5,6,7],
                [1,2,3,4,5,6,7,8], # bpo-38373: Length 8 init won't over-alloc
                [1,2,3,4,5,6,7,8,9],
                ]

        overalloc_amts = []
        for literal in test_literals:
            # Direct check that list-literals do not over-allocate, by
            # calculating the total size of used pointers.
            total_ptr_size = len(literal) * struct.calcsize('P')
            self.assertEqual(sizeof(literal), sizeof([]) + total_ptr_size)

            # Ensure that both list literals, and lists made from an iterable
            # of known size, use the same amount of allocation.
            self.assertEqual(sizeof(literal), sizeof(list(literal)))
            self.assertEqual(sizeof(literal), sizeof(list(tuple(literal))))

            # By contrast, confirm that non-empty lists initialized from an
            # iterable where the length is unknown at the time of
            # initialization, can be overallocated.
            iterated_list = list(x for x in literal)
            overalloc_amts.append(sizeof(iterated_list) - sizeof(literal))
            self.assertGreaterEqual(sizeof(iterated_list), sizeof(literal))

        # bpo-38373: initialized or grown lists are not always over-allocated.
        # Confirm that over-allocation occurs at least some of the time.
        self.assertEqual(True, any(x>0 for x in overalloc_amts))

        # Empty lists should overallocate on initial append/insert (unlike
        # list-literals)
        l1 = []
        l1.append(1)
        self.assertGreater(sizeof(l1), sizeof([1]))
        l2 = []
        l2.insert(0, 1)
        self.assertGreater(sizeof(l2), sizeof([1]))

    def test_count_index_remove_crashes(self):
        # bpo-38610: The count(), index(), and remove() methods were not
        # holding strong references to list elements while calling
        # PyObject_RichCompareBool().
        class X:
            def __eq__(self, other):
                lst.clear()
                return NotImplemented

        lst = [X()]
        with self.assertRaises(ValueError):
            lst.index(lst)

        class L(list):
            def __eq__(self, other):
                str(other)
                return NotImplemented

        lst = L([X()])
        lst.count(lst)

        lst = L([X()])
        with self.assertRaises(ValueError):
            lst.remove(lst)

        # bpo-39453: list.__contains__ was not holding strong references
        # to list elements while calling PyObject_RichCompareBool().
        lst = [X(), X()]
        3 in lst
        lst = [X(), X()]
        X() in lst


if __name__ == "__main__":
    unittest.main()
