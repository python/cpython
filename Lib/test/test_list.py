import unittest
import sys
from test import test_support, list_tests

class ListTest(list_tests.CommonTest):
    type2test = list

    def test_truth(self):
        super().test_truth()
        self.assert_(not [])
        self.assert_([42])

    def test_identity(self):
        self.assert_([] is not [])

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

def test_main(verbose=None):
    test_support.run_unittest(ListTest)

    # verify reference counting
    import sys
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in range(len(counts)):
            test_support.run_unittest(ListTest)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print(counts)


if __name__ == "__main__":
    test_main(verbose=True)
