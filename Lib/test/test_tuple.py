import unittest
from test import test_support, seq_tests

class TupleTest(seq_tests.CommonTest):
    type2test = tuple

    def test_constructors(self):
        super(TupleTest, self).test_len()
        # calling built-in types without argument must return empty
        self.assertEqual(tuple(), ())

    def test_truth(self):
        super(TupleTest, self).test_truth()
        self.assert_(not ())
        self.assert_((42, ))

    def test_len(self):
        super(TupleTest, self).test_len()
        self.assertEqual(len(()), 0)
        self.assertEqual(len((0,)), 1)
        self.assertEqual(len((0, 1, 2)), 3)

    def test_iadd(self):
        super(TupleTest, self).test_iadd()
        u = (0, 1)
        u2 = u
        u += (2, 3)
        self.assert_(u is not u2)

    def test_imul(self):
        super(TupleTest, self).test_imul()
        u = (0, 1)
        u2 = u
        u *= 3
        self.assert_(u is not u2)

    def test_tupleresizebug(self):
        # Check that a specific bug in _PyTuple_Resize() is squashed.
        def f():
            for i in range(1000):
                yield i
        self.assertEqual(list(tuple(f())), range(1000))


def test_main():
    test_support.run_unittest(TupleTest)

if __name__=="__main__":
    test_main()
