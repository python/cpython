# Python test set -- built-in functions

import test.test_support, unittest
import sys
import pickle

import warnings
warnings.filterwarnings("ignore", "integer argument expected",
                        DeprecationWarning, "unittest")

class XrangeTest(unittest.TestCase):
    def test_xrange(self):
        self.assertEqual(list(xrange(3)), [0, 1, 2])
        self.assertEqual(list(xrange(1, 5)), [1, 2, 3, 4])
        self.assertEqual(list(xrange(0)), [])
        self.assertEqual(list(xrange(-3)), [])
        self.assertEqual(list(xrange(1, 10, 3)), [1, 4, 7])
        self.assertEqual(list(xrange(5, -5, -3)), [5, 2, -1, -4])

        a = 10
        b = 100
        c = 50

        self.assertEqual(list(xrange(a, a+2)), [a, a+1])
        self.assertEqual(list(xrange(a+2, a, -1L)), [a+2, a+1])
        self.assertEqual(list(xrange(a+4, a, -2)), [a+4, a+2])

        seq = list(xrange(a, b, c))
        self.assert_(a in seq)
        self.assert_(b not in seq)
        self.assertEqual(len(seq), 2)

        seq = list(xrange(b, a, -c))
        self.assert_(b in seq)
        self.assert_(a not in seq)
        self.assertEqual(len(seq), 2)

        seq = list(xrange(-a, -b, -c))
        self.assert_(-a in seq)
        self.assert_(-b not in seq)
        self.assertEqual(len(seq), 2)

        self.assertRaises(TypeError, xrange)
        self.assertRaises(TypeError, xrange, 1, 2, 3, 4)
        self.assertRaises(ValueError, xrange, 1, 2, 0)

        self.assertRaises(OverflowError, xrange, 1e100, 1e101, 1e101)

        self.assertRaises(TypeError, xrange, 0, "spam")
        self.assertRaises(TypeError, xrange, 0, 42, "spam")

        self.assertEqual(len(xrange(0, sys.maxint, sys.maxint-1)), 2)

        self.assertRaises(OverflowError, xrange, -sys.maxint, sys.maxint)
        self.assertRaises(OverflowError, xrange, 0, 2*sys.maxint)

        r = xrange(-sys.maxint, sys.maxint, 2)
        self.assertEqual(len(r), sys.maxint)
        self.assertRaises(OverflowError, xrange, -sys.maxint-1, sys.maxint, 2)

    def test_pickling(self):
        testcases = [(13,), (0, 11), (-22, 10), (20, 3, -1),
                     (13, 21, 3), (-2, 2, 2)]
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            for t in testcases:
                r = xrange(*t)
                self.assertEquals(list(pickle.loads(pickle.dumps(r, proto))),
                                  list(r))


def test_main():
    test.test_support.run_unittest(XrangeTest)

if __name__ == "__main__":
    test_main()
